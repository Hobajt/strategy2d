#include "tools.h"
#include "context.h"

using namespace eng;

#define OP_STACK_SIZE 16

#define MAX_PLAYERS_COUNT 8

const char* toolType2str(int toolType);
const char* tileIdx2name(int idx);

const char* unitType2name(int idx, int race);
const char* buildingType2name(int idx, int race);

//===== EditorTool =====

EditorTool::EditorTool(EditorContext& context_) : context(context_) {}

//===== SelectionTool =====

SelectionTool::SelectionTool(EditorContext& context_) : EditorTool(context_) {}

void SelectionTool::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Selection Tool");
    ImGui::Separator();
    //TODO:
#endif
}

void SelectionTool::Render() {
    //TODO:
}

void SelectionTool::OnLMB(int state) {
    //TODO:
}

//===== PaintTool =====

PaintTool::PaintTool(EditorContext& context_) : EditorTool(context_) {}

void PaintTool::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Tile Painting Tool");
    ImGui::Separator();

    ImGui::PushID(0);
    if(ImGui::BeginListBox("Tile to paint:")) {
        for(int i = 0; i < TileType::COUNT; i++) {
            if(ImGui::Selectable(tileIdx2name(i), tileType == i)) {
                tileType = i;
            }
        }
        ImGui::EndListBox();
    }
    ImGui::PopID();

    ImGui::Separator();
    if(ImGui::DragInt("Brush size", &brushSize)) {
        UpdateBrushSize(brushSize);
    }

    ImGui::Separator();
    ImGui::Checkbox("Randomize variations", &randomVariations);
    if(!randomVariations) {
        if(ImGui::DragInt("Variation value", &variationValue)) {
            if(variationValue < 0) variationValue = 0;
            else if(variationValue > 100) variationValue = 100;
        }
    }

    ImGui::Separator();
    ImGui::Checkbox("Visualize stroke bounds", &viz_limits);
    ImGui::Checkbox("Visualize paint reach", &viz_paintReach);
#endif
}

void PaintTool::CustomSignal(int state, int id) {
    UpdateBrushSize(brushSize+state);
}

void PaintTool::Render() {
    Camera& cam = Camera::Get();

    if(hover) {
        //hovering tile coordinate (in map space)
        glm::ivec2 coord = glm::ivec2(cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f) + 0.5f);

        //corner points for the highlights
        int xm = std::max(0, coord.x-bl);
        int xM = std::min(coord.x+br, paint.Size().x);
        int ym = std::max(0, coord.y-bl);
        int yM = std::min(coord.y+br, paint.Size().y);

        //highlight rectangle parameters
        float width = 0.1f;
        float zIdx = -0.1f;
        glm::vec4 clr = glm::vec4(1.f, 0.f, 0.f, 1.f);

        //render quads as highlight borders
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, ym)), zIdx), glm::vec2(xM - xm + width, width) * cam.Mult(), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, yM)), zIdx), glm::vec2(xM - xm + width, width) * cam.Mult(), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, ym)), zIdx), glm::vec2(width, yM - ym) * cam.Mult(), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xM, ym)), zIdx), glm::vec2(width, yM - ym) * cam.Mult(), clr));
    }
    hover = false;

    //highlight current stroke's min/max coords
    if(viz_limits && painting) {
        float zIdx = -0.06f;
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(session_min)), zIdx), cam.Mult(), glm::vec4(0.f, 1.f, 0.f, 0.5f)));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(session_max-1)), zIdx), cam.Mult(), glm::vec4(0.f, 0.f, 1.f, 0.5f)));
    }

    //highlight tiles selected by current stroke
    float zIdx = -0.05f;
    glm::vec4 clr_paint = glm::vec4(1.f, 0.f, 0.f, 0.3f);
    glm::vec4 clr_reach = glm::vec4(0.f, 0.f, 1.f, 0.3f);
    glm::vec4 clr_debug = glm::vec4(.4f, 1.f, .2f, 0.3f);

    if(painting || viz_paintReach) {
        for(int y = 0; y < paint.Size().y; y++) {
            for(int x = 0; x < paint.Size().x; x++) {
                if(HAS_FLAG(paint(y, x), PaintFlags::MARKED_FOR_PAINT)) {
                    //highlight current stroke selection
                    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(x, y)), zIdx), cam.Mult(), clr_paint));
                }
                else if (viz_paintReach && HAS_FLAG(paint(y, x), PaintFlags::FINALIZED)) {
                    //highlight affected tiles from the last stroke
                    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(x, y)), zIdx), cam.Mult(), clr_reach));
                }
                // if(HAS_FLAG(paint(y, x), PaintFlags::DEBUG)) {
                //     //highlight current stroke selection
                //     Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(x, y)), zIdx), cam.Mult(), clr_debug));
                // }
            }
        }
    }
}

void PaintTool::OnLMB(int state) {
    Camera& cam = Camera::Get();
    glm::ivec2 coord = glm::ivec2(cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f) + 0.5f);

    switch(state) {
        case InputButtonState::DOWN:
            OnStrokeStart();
        case InputButtonState::PRESSED:
            Stroke_MarkRegion(coord, bl, br);
            hover = true;
            break;
        case InputButtonState::UP:
            OnStrokeFinish();
            break;
    }
}

void PaintTool::NewLevelCreated(const glm::ivec2& size_) {
    paint = PaintBitmap(size_);
}

bool PaintTool::UndoOperation(OperationRecord& op, UndoRedoUpdateData& info_update) {
    if(!op.paint)
        return false;

    context.level.map.UndoChanges(op.actions);
    paint.Clear();
    return true;
}

void PaintTool::UpdateBrushSize(int newSize) {
    brushSize = newSize;
    if(brushSize < 1)
        brushSize = 1;
    br = (brushSize+1) / 2;
    bl = brushSize - br;
}

void PaintTool::OnStrokeStart() {
    paint.Clear();
    painting = true;
}

void PaintTool::Stroke_MarkRegion(const glm::ivec2& coords, int brushLeft, int brushRight) {
    glm::ivec2 min = glm::max(coords - brushLeft, glm::ivec2(0));
    glm::ivec2 max = glm::min(coords + brushRight, paint.Size());

    session_min = glm::min(session_min, min);
    session_max = glm::max(session_max, max);

    for(int y = min.y; y < max.y; y++) {
        for(int x = min.x; x < max.x; x++) {
            paint(y, x) = PaintFlags::MARKED_FOR_PAINT;
        }
    }
}

void PaintTool::OnStrokeFinish() {
    ApplyPaint();
    painting = false;
}

void PaintTool::ApplyPaint() {
    OperationRecord op = {};
    op.paint = true;

    context.level.map.ModifyTiles(paint, tileType, randomVariations, variationValue, &op.actions);
    context.tools.PushOperation(std::move(op));
}

//===== ObjectPlacementTool =====

ObjectPlacementTool::ObjectPlacementTool(EditorContext& context_) : EditorTool(context_), shadows(TextureGenerator::GetTexture(TextureGenerator::Params::ShadowsTexture(256,256,8))) {}

void ObjectPlacementTool::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Object Placement Tool");
    ImGui::Separator();

    if(context.level.info.custom_game) {
        ImGui::Text("Objects can only be placed in non-custom game.");
        if(ImGui::Button("Change to scenario")) {
            context.level.info.custom_game = false;
        }
        return;
    }

    ImGui::Text("ctrl + LMB - placement");
    ImGui::Text("LMB - move object (dragging)");
    ImGui::Text("RMB - removal");
    ImGui::Separator();
    
    ImGui::Text("Mode:");
    ImGui::SameLine();
    if(ImGui::Button(objRace ? "Human" : "Orc")) {
        objRace = 1 - objRace;
        objdata = nullptr;
    }
    ImGui::SameLine();
    if(ImGui::Button(place_buildings ? "Buildings" : "Units")) {
        place_buildings = !place_buildings;
        objdata = nullptr;
        objIdx = -1;
    }
    ImGui::Separator();
    ImGui::Text("FactionIdx:");
    if(ImGui::SliderInt("Faction idx", &factionIdx, 0, context.level.factions.size()-1)) {
        faction = context.level.factions[factionIdx];
    }
    ImGui::Separator();

    ImGui::PushID(0);
    if(place_buildings) {
        if(ImGui::BeginListBox("Buildings:")) {
            for(int i = 0; i < BuildingType::COUNT; i++) {
                if(ImGui::Selectable(buildingType2name(i, objRace), objIdx == i)) {
                    objIdx = i;
                }
            }
            for(int i = 101; i < 101+BuildingType::COUNT2; i++) {
                if(ImGui::Selectable(buildingType2name(i, objRace), objIdx == i)) {
                    objIdx = i;
                }
            }
            ImGui::EndListBox();
        }
    }
    else {
        if(ImGui::BeginListBox("Units:")) {
            for(int i = 0; i < UnitType::COUNT; i++) {
                if(ImGui::Selectable(unitType2name(i, objRace), objIdx == i)) {
                    objIdx = i;
                }
            }
            for(int i = 101; i < 101+UnitType::COUNT2; i++) {
                if(ImGui::Selectable(unitType2name(i, objRace), objIdx == i)) {
                    objIdx = i;
                }
            }
            ImGui::EndListBox();
        }
    }
    ImGui::PopID();

    glm::ivec3 prev_numID = numID;
    numID = glm::ivec3(place_buildings ? ObjectType::BUILDING : ObjectType::UNIT, objIdx, objRace);
    if(numID != prev_numID) {
        objdata = (objIdx >= 0) ? std::dynamic_pointer_cast<FactionObjectData>(Resources::LoadObject(numID)) : nullptr;
    }
#endif
}

void ObjectPlacementTool::Render() {
    if(context.level.info.custom_game)
        return;

    //faction reset, in case faction count changes
    if(factionIdx >= context.level.factions.size() || faction == nullptr) {
        factionIdx = 0;
        faction = context.level.factions[factionIdx];
    }

    FactionObjectDataRef data = drag ? context.level.objects.GetObject(dragID).DataF() : objdata;
    if(data == nullptr)
        return;
    glm::ivec2 object_size = glm::ivec2(data->size);

    Camera& cam = Camera::Get();
    Input& input = Input::Get();

    //hovering tile coordinate (in map space)
    glm::ivec2 coord = glm::ivec2(cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f) + 0.5f);
    if(data->navigationType != NavigationBit::GROUND && data->objectType != ObjectType::BUILDING) {
        coord = make_even(coord);
        if(!place_buildings) object_size = glm::ivec2(2);
    }

    //render object preview if dragging or when in placement mode
    if(data != nullptr && (drag || input.ctrl)) {
        RenderTileHighlight(coord, object_size);
        RenderObjectViz(coord, object_size, objdata);
    }
}

void ObjectPlacementTool::OnLMB(int state) {
    Input& input = Input::Get();
    Camera& cam = Camera::Get();

    glm::vec2 coord_f = cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f);
    glm::ivec2 coord = glm::ivec2(coord_f + 0.5f);

    switch(state) {
        case InputButtonState::DOWN:
        {
            if(input.ctrl) {
                //object placement
                AddObject(objdata, coord);
            }
            else {
                ObjectID id = context.level.map.ObjectIDAt(coord);
                if(ObjectID::IsValid(id)) {
                    //object drag started
                    drag = true;
                    dragID = id;
                }
            }
            break;
        }
        case InputButtonState::PRESSED:
            dragCoords = coord_f;
            break;
        case InputButtonState::UP:
            if(drag) {
                MoveObject(dragID, coord);
            }
            drag = false;
            dragID = ObjectID();
            break;
    }
}

void ObjectPlacementTool::OnRMB(int state) {
    if(state != InputButtonState::DOWN)
        return;

    Input& input = Input::Get();
    Camera& cam = Camera::Get();

    glm::vec2 coord_f = cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f);
    glm::ivec2 coord = glm::ivec2(coord_f + 0.5f);
    
    //object removal - retrieval & validation
    ObjectID id = context.level.map.ObjectIDAt(coord);
    if(!ObjectID::IsValid(id)) {
        LOG_TRACE("ObjectPlacementTool::Remove - nothing to remove");
        return;
    }
    FactionObject& obj = context.level.objects.GetObject(id);

    //add undo/redo entry
    OperationRecord op = {};
    op.paint = false;
    op.actions.push_back(TileRecord(obj.OID(), ObjectOperation::REMOVE, obj.Position(), obj.NumID()));
    context.tools.PushOperation(std::move(op));

    /* HOW TO HANDLE OBJECT RESTORATION (from undo operation):
        - need to keep the removed object params in the memory (for as long as the undo record exists)
        - the most basic approach is to just track the object NumID(), and then recreate it
        - this doesn't account for any object stats modifications tho (health, etc.)
    */

    //object removal
    obj.Kill(true);
    LOG_TRACE("ObjectPlacementTool::RemoveObject - removed {} from {}.", obj.OID(), obj.Position());
}

bool ObjectPlacementTool::UndoOperation(OperationRecord& op, UndoRedoUpdateData& info_update) {
    if(op.paint)
        return false;
    TileRecord& r = op.actions.at(0);
    int& action = r.tileType;

    switch(action) {
        case ObjectOperation::ADD:
        {
            FactionObject* obj = nullptr;
            if(!context.level.objects.GetObject(r.id, obj)) {
                LOG_WARN("ObjectPlacementTool::UndoOperation - failed to undo Object-Add, object not found.");
                return false;
            }
            obj->Kill(true);
            action = ObjectOperation::REMOVE;
            break;
        }
        case ObjectOperation::REMOVE:
        {
            ObjectID id = AddObject(r.num_id, r.pos, false);
            if(!ObjectID::IsValid(id)) {
                LOG_WARN("ObjectPlacementTool::UndoOperation - failed to undo Object-Remove, object restoration failed.");
                return false;
            }
            info_update.flag = UndoRedoUpdateType::ID_UPDATE;
            info_update.id_update = { r.id, id };

            r.id = id;
            action = ObjectOperation::ADD;
            break;
        }
        case ObjectOperation::MOVE:
        {
            glm::ivec2 old_pos = glm::ivec2(0);
            int res = MoveObject(r.id, r.pos, false, &old_pos);
            switch(res) {
                case 1:
                    LOG_WARN("ObjectPlacementTool::UndoOperation - failed to undo Object-Move, object not found.");
                    return false;
                case 2:
                    LOG_WARN("ObjectPlacementTool::UndoOperation - failed to undo Object-Move, location is invalid.");
                    return false;
                case 0:
                    break;
                default:
                    LOG_WARN("ObjectPlacementTool::UndoOperation - failed to undo Object-Move, unknown error.");
                    return false;
            }
            r.pos = old_pos;
            action = ObjectOperation::MOVE;
            break;
        }
        default:
            LOG_WARN("ObjectPlacementTool::UndoOperation - invalid action type.");
            return false;
    }
    
    return true;
}

void ObjectPlacementTool::RenderTileHighlight(const glm::ivec2& location, const glm::ivec2& size) {
    Camera& cam = Camera::Get();

    //corner points for the highlights
    int xm = std::max(0, location.x);
    int ym = std::max(0, location.y);
    int xM = std::min(location.x+size.x, context.level.MapSize().x);
    int yM = std::min(location.y+size.y, context.level.MapSize().y);

    //highlight rectangle parameters
    float width = 0.1f;
    float zIdx = -0.1f;
    glm::vec4 clr = glm::vec4(1.f, 0.f, 0.f, 1.f);

    //render quads as highlight borders
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, ym)), zIdx), glm::vec2(xM - xm + width, width) * cam.Mult(), clr));
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, yM)), zIdx), glm::vec2(xM - xm + width, width) * cam.Mult(), clr));
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, ym)), zIdx), glm::vec2(width, yM - ym) * cam.Mult(), clr));
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xM, ym)), zIdx), glm::vec2(width, yM - ym) * cam.Mult(), clr));
} 

void ObjectPlacementTool::RenderObjectViz(const glm::ivec2& location, const glm::ivec2& size, const FactionObjectDataRef& data) {
    Camera& cam = Camera::Get();
    Input& input = Input::Get();

    SpriteGroup& sg = data->animData->GetGraphics(BuildingAnimationType::IDLE);
    glm::ivec2 sz = size;

    float zIdx = -0.5f;
    int nav_type = data->navigationType;
    bool preconditions = IsLocationValid(location, sz, data);

    //render object sprite
    if(place_buildings) {
        sg.Render(glm::vec3(cam.map2screen(location), zIdx), data->size * cam.Mult(), glm::ivec4(0), 0, 0);
    }
    else {
        UnitData* unit = static_cast<UnitData*>(data.get());
        glm::vec2 render_size = unit->size * unit->scale;
        bool render_centered = (nav_type == NavigationBit::GROUND);
        glm::vec2 pos = location;
        if(render_centered) {
            pos = (pos + 0.5f) - render_size * 0.5f;
        }
        pos = cam.map2screen(pos);
        sg.Render(glm::vec3(pos, zIdx), render_size * cam.Mult(), glm::ivec4(0), 0, 0);
    }
    
    //render colored overlay, signaling buildability for each tile
    for(int y = 0; y < sz.y; y++) {
        for(int x = 0; x < sz.x; x++) {
            glm::ivec2 pos = glm::ivec2(location.x + x, location.y + y);
            bool within_bounds = context.level.map.IsWithinBounds(pos);
            glm::vec4 clr = ((!place_buildings || context.level.map.IsBuildable(pos, nav_type, glm::ivec2(-1))) && preconditions && within_bounds) ? glm::vec4(0.f, 0.62f, 0.f, 1.f) : glm::vec4(0.62f, 0.f, 0.f, 1.f);
            Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(pos), zIdx-1e-3f), cam.Mult(), clr, shadows));
        }
    }
}

bool ObjectPlacementTool::IsLocationValid(const glm::ivec2& location, const glm::ivec2& size, const FactionObjectDataRef& data) {
    bool preconditions;
    if(data->objectType == ObjectType::BUILDING) {
        BuildingData* building = static_cast<BuildingData*>(data.get());
        preconditions = context.level.map.CoastCheck(location, size, building->coastal);
    }
    else if(data->navigationType != NavigationBit::WATER) {
        preconditions = context.level.map.CanSpawn(location, data->navigationType);
    }
    else {
        preconditions = true;
        for(int y = 0; y < 2; y++) {
            for(int x = 0; x < 2; x++) {
                preconditions &= context.level.map.CanSpawn(location + glm::ivec2(x,y), data->navigationType);
            }
        }
    }
    return preconditions;
}

ObjectID ObjectPlacementTool::AddObject(const glm::ivec3& num_id, const glm::ivec2& location, bool add_op_record) {
    GameObjectDataRef data_go = Resources::LoadObject(num_id);
    FactionObjectDataRef data = std::dynamic_pointer_cast<FactionObjectData>(data_go);
    return AddObject(data, location, add_op_record);
}

ObjectID ObjectPlacementTool::AddObject(const FactionObjectDataRef& data, const glm::ivec2& location, bool add_op_record) {
    ASSERT_MSG(data != nullptr, "ObjectPlacementTool::AddObject - data should never be null at this point.");

    glm::ivec2 coord = location;
    if(data->navigationType != NavigationBit::GROUND && data->objectType != ObjectType::BUILDING) {
        coord = make_even(coord);
    }

    if(!IsLocationValid(coord, data->size, data)) {
        LOG_TRACE("ObjectPlacementTool::AddObject - invalid placement location.");
        return ObjectID();
    }

    ObjectID id = ObjectID();
    switch(data->objectType) {
        case ObjectType::UNIT:
            id = context.level.objects.EmplaceUnit(context.level, std::static_pointer_cast<UnitData>(data), faction, coord, false);
            break;
        case ObjectType::BUILDING:
            id = context.level.objects.EmplaceBuilding(context.level, std::static_pointer_cast<BuildingData>(data), faction, coord, true);
            break;
        default:
            LOG_TRACE("ObjectPlacementTool::AddObject - invalid object type.");
            return ObjectID();
    }

    //add undo/redo entry
    if(add_op_record) {
        OperationRecord op = {};
        op.paint = false;
        op.actions.push_back(TileRecord(id, ObjectOperation::ADD, coord, data->num_id));
        context.tools.PushOperation(std::move(op));
    }

    //object placement automatically turns the game type to scenario
    context.level.info.custom_game = false;

    LOG_TRACE("ObjectPlacementTool::AddObject - added {} at {}.", id, coord);
    return id;
}

int ObjectPlacementTool::MoveObject(const ObjectID& id, const glm::ivec2& new_pos, bool add_op_record, glm::ivec2* out_prev_pos) {
    FactionObject* obj = nullptr;
    if(!context.level.objects.GetObject(id, obj)) {
        LOG_TRACE("ObjectPlacementTool::MoveObject - failed to move {} to {} (object not found).", id, new_pos);
        return 1;
    }
    if(!IsLocationValid(new_pos, obj->Data()->size, obj->DataF())) {
        LOG_TRACE("ObjectPlacementTool::MoveObject - failed to move {} to {} (invalid location).", id, new_pos);
        return 2;
    }

    if(add_op_record) {
        //add undo/redo entry
        OperationRecord op = {};
        op.paint = false;
        op.actions.push_back(TileRecord(obj->OID(), ObjectOperation::MOVE, obj->Position(), obj->NumID()));
        context.tools.PushOperation(std::move(op));
    }

    if(out_prev_pos != nullptr) {
        *out_prev_pos = obj->Position();
    }
    
    obj->MoveTo(new_pos);
    LOG_TRACE("ObjectPlacementTool::MoveObject - moved {} to {}.", id, new_pos);
    return 0;
}

//===== StartingLocationTool =====

StartingLocationTool::StartingLocationTool(EditorContext& context_) : EditorTool(context_) {}

void StartingLocationTool::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Starting Location Tool");
    ImGui::Separator();

    if(!context.level.info.custom_game) {
        ImGui::Text("Starting locations are ignored for non-custom games.");
        return;
    }

    ImGui::Checkbox("Render locations", &renderStartingLocations);
    ImGui::Separator();

    Level& level = context.level;
    float f = 0.1f * ImGui::GetWindowSize().x;

    ImGui::Text("Starting locations:");
    ImGui::Indent(f);
    ImGui::PushID(&startingLocations);
    int eraseIdx = -1;
    for(int i = 0; i < (int)startingLocations.size(); i++) {
        glm::ivec2& pos = startingLocations[i];
        ImGui::Text("[%d] - (%3d, %3d)", i, pos.x, pos.y);
        ImGui::SameLine();
        ImGui::PushID(i);
        if(ImGui::Button("X")) {
            eraseIdx = i;
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    ImGui::Unindent(f);

    if(eraseIdx >= 0) {
        startingLocations.erase(startingLocations.begin() + eraseIdx);
    }

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::Indent(f);
    ImGui::Text("ctrl + LMB click = place new location (max %d)", MAX_PLAYERS_COUNT);
    ImGui::Text("LMB - drag existing location around the map");
    ImGui::Text("RMB - delete location (or use GUI buttons)");
    ImGui::Unindent(f);
#endif
}

void StartingLocationTool::Render() {
    InnerRender();
}

void StartingLocationTool::Render_NotSelected() {
    InnerRender();
}

void StartingLocationTool::OnLMB(int state) {
    if(!context.level.info.custom_game)
        return;

    Input& input = Input::Get();
    Camera& cam = Camera::Get();

    glm::vec2 coord_f = cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f);
    glm::ivec2 coord = glm::ivec2(coord_f + 0.5f);

    //TODO: how to handle terrain issues - placing location over trees/placing trees under location
    //TODO: add support for undo/redo logic

    //can validate the location during placement here
    //when changing tiles, probably just remove invalid locations
    
    switch(state) {
        case InputButtonState::DOWN:
        {
            int idx = LocationIdx(coord);
            if(idx < 0 && input.ctrl && startingLocations.size() < MAX_PLAYERS_COUNT) {
                //create new
                startingLocations.push_back(coord);
                LocationsUpdated();
            }
            else if(idx >= 0 && !input.ctrl) {
                //location dragging start
                drag = true;
                dragIdx = idx;
            }
            break;
        }
        case InputButtonState::PRESSED:
            dragCoords = coord_f;
            break;
        case InputButtonState::UP:
            if(drag) {
                ASSERT_MSG(dragIdx < (int)startingLocations.size(), "Something went wrong, dragIdx is out of range.");
                if(LocationIdx(coord) < 0) {
                    startingLocations[dragIdx] = coord;
                    LocationsUpdated();
                }
            }
            drag = false;
            dragIdx = -1;
            break;
    }
}

void StartingLocationTool::OnRMB(int state) {
    if(!context.level.info.custom_game)
        return;

    if(state == InputButtonState::DOWN) {
        Camera& cam = Camera::Get();

        glm::vec2 coord_f = cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f);
        glm::ivec2 coord = glm::ivec2(coord_f + 0.5f);

        int idx = LocationIdx(coord);
        if(idx >= 0) {
            startingLocations.erase(startingLocations.begin() + idx);
            LocationsUpdated();
        }
    }
}

int StartingLocationTool::LocationIdx(const glm::ivec2& loc) const {
    auto pos = std::find(startingLocations.begin(), startingLocations.end(), loc);
    return (pos != startingLocations.end()) ? (pos - startingLocations.begin()) : -1;
}

void StartingLocationTool::InnerRender() {
    if(!context.level.info.custom_game)
        return;
    
    if(renderStartingLocations) {
        Camera& cam = Camera::Get();
        const Sprite& tilemap = context.level.map.GetTileset()->Tilemap();

        for(int i = 0; i < (int)startingLocations.size(); i++) {
            glm::vec2 v = (i != dragIdx || !drag) ? glm::vec2(startingLocations[i]) : dragCoords;
            glm::vec3 pos = glm::vec3(cam.map2screen(v), -1e-2f);
            tilemap.Render(pos, cam.Mult(), 0, 1, (float)i);
        }
    }
}

void StartingLocationTool::LocationsUpdated() {
    context.level.info.startingLocations = startingLocations;
}

//===== EditorTools =====

EditorTools::EditorTools(EditorContext& context_)
    : context(context_), ops_undo(RingBuffer<OperationRecord>(OP_STACK_SIZE)), ops_redo(RingBuffer<OperationRecord>(OP_STACK_SIZE)) {
    tools.insert({ ToolType::SELECT, new SelectionTool(context_) });
    tools.insert({ ToolType::TILE_PAINT, new PaintTool(context_) });
    tools.insert({ ToolType::OBJECT_PLACEMENT, new ObjectPlacementTool(context_) });
    tools.insert({ ToolType::STARTING_LOCATION, new StartingLocationTool(context_) });

    currentTool = tools.at(ToolType::SELECT);
}

EditorTools::~EditorTools() {
    for(auto& [id, tool] : tools) {
        delete tool;
    }
    tools.clear();
}

void EditorTools::GUI_Update() {
    if(currentTool != nullptr) {
        currentTool->GUI_Update();
    }
    else {
        ENGINE_IF_GUI(ImGui::Text("No tool selected."));
    }
}

void EditorTools::SwitchTool(int toolType) {
    currentTool = tools.at(toolType);
}

void EditorTools::NewLevelCreated(const glm::ivec2& size) {
    for(auto& [id, tool] : tools) {
        tool->NewLevelCreated(size);
    }
}

void EditorTools::CustomSignal(int state, int id) {
    if(currentTool != nullptr)
        currentTool->CustomSignal(state, id);
}

void EditorTools::Render() {
    if(currentTool != nullptr)
        currentTool->Render();
    
    for(auto& [key, tool] : tools) {
        if(tool != currentTool)
            tool->Render_NotSelected();
    }
}

void EditorTools::OnLMB(int state) {
    if(currentTool != nullptr)
        currentTool->OnLMB(state);
}

void EditorTools::OnRMB(int state) {
    if(currentTool != nullptr)
        currentTool->OnRMB(state);
}

void EditorTools::OnHover() {
    if(currentTool != nullptr)
        currentTool->OnHover();
}

const std::unordered_map<int,EditorTool*>& EditorTools::GetTools() const {
    return tools;
}

bool EditorTools::IsToolSelected(int toolType) const {
    return tools.at(toolType) == currentTool;
}

void EditorTools::PushOperation(OperationRecord&& op) {
    ops_undo.Push(std::move(op));
    ops_redo.Clear();
}

void EditorTools::Undo() {
    if(ops_undo.Size() > 0) {
        LOG_TRACE("Editor::Undo ({} left)", ops_undo.Size()-1);
        OperationRecord op = ops_undo.Pop();

        UndoRedoUpdateData info_update = {};
        for(auto& [id, tool] : tools) {
            if(tool->UndoOperation(op, info_update)) {
                ops_redo.Push(op);
                // LOG_INFO("REDO = [P: {}, A: {}, ID: {}]", ops_redo.PeekAt(0).paint, ops_redo.PeekAt(0).actions[0].tileType, ops_redo.PeekAt(0).actions[0].id);
                break;
            }
        }

        UndoRedo_InfoUpdate(info_update);
    }
    else {
        LOG_TRACE("Editor::Undo - there nothing to undo");
    }
}

void EditorTools::Redo() {
    if(ops_redo.Size() > 0) {
        LOG_TRACE("Editor::Redo ({} left)", ops_redo.Size()-1);
        OperationRecord op = ops_redo.Pop();

        UndoRedoUpdateData info_update = {};
        for(auto& [id, tool] : tools) {
            if(tool->UndoOperation(op, info_update)) {
                ops_undo.Push(op);
                // LOG_INFO("UNDO = [P: {}, A: {}, ID: {}]", ops_undo.PeekAt(0).paint, ops_undo.PeekAt(0).actions[0].tileType, ops_undo.PeekAt(0).actions[0].id);
                break;
            }
        }

        UndoRedo_InfoUpdate(info_update);
    }
    else {
        LOG_TRACE("Editor::Redo - there nothing to redo");
    }
}

void EditorTools::UndoRedo_InfoUpdate(const UndoRedoUpdateData& data) {
    switch(data.flag) {
        case UndoRedoUpdateType::ID_UPDATE:
            for(int i = 0; i < ops_undo.Size(); i++) {
                OperationRecord& op = ops_undo.PeekAt(i);
                if(!op.paint && op.actions[0].id == data.id_update.first) {
                    op.actions[0].id = data.id_update.second;
                }
            }
            for(int i = 0; i < ops_redo.Size(); i++) {
                OperationRecord& op = ops_redo.PeekAt(i);
                if(!op.paint && op.actions[0].id == data.id_update.first) {
                    op.actions[0].id = data.id_update.second;
                }
            }
            break;
    }
}

//=============================================

const char* toolType2str(int toolType) {
    static const char* names[ToolType::COUNT] = { "SELECT", "TILE_PAINT", "OBJECT_PLACEMENT", "STARTING_LOCATION" };
    return (toolType < ToolType::COUNT) ? names[toolType] : "INVALID";
}

const char* tileIdx2name(int idx) {
    static const char* names[] = {
        "GROUND1", "GROUND2", "MUD", "MUD2",
        "WALL_BROKEN", "ROCK_BROKEN", "TREES_FELLED",
        "WATER",
        "ROCK", "WALL_HU", "WALL_HU_DAMAGED", "WALL_OC", "WALL_OC_DAMAGED",
        "TREES"
    };
    return (idx < TileType::COUNT) ? names[idx] : "INVALID TILE TYPE";
}

const char* unitType2name(int idx, int race) {
    static std::array<const char*,UnitType::COUNT> names = {
        "PEASANT", "FOOTMAN", "ARCHER", "RANGER", "BALLISTA", "KNIGHT", "PALADIN",
        "TANKER", "DESTROYER", "BATTLESHIP", "SUBMARINE", "TRANSPORT",
        "ROFLCOPTER", "DEMOSQUAD",
        "MAGE",
        "GRYPHON"
    };

    static std::array<const char*,UnitType::COUNT2> names2 = {
        "SEAL", "PIG", "SHEEP", "SKELETON", "EYE"
    };
    
    if(idx < UnitType::COUNT)
        return names[idx];
    else if((idx - 101) < UnitType::COUNT2 && (idx - 101) >= 0)
        return names2[idx-101];
    else
        return "INVALID UNIT TYPE";
}

const char* buildingType2name(int idx, int race) {
    static std::array<const char*,BuildingType::COUNT> names = {
        "TOWN_HALL", "BARRACKS", "FARM", "LUMBER_MILL", "BLACKSMITH", "TOWER",
        "SHIPYARD", "FOUNDRY", "OIL_REFINERY", "OIL_PLATFORM",
        "INVENTOR", "STABLES", "CHURCH", "WIZARD_TOWER", "DRAGON_ROOST",
        "GUARD_TOWER", "CANNON_TOWER",
        "KEEP", "CASTLE"
    };

    static std::array<const char*,BuildingType::COUNT2> names2 = {
        "GOLD_MINE", "OIL_PATCH"
    };
    
    if(idx < BuildingType::COUNT)
        return names[idx];
    else if((idx - 101) < BuildingType::COUNT2 && (idx - 101) >= 0)
        return names2[idx-101];
    else
        return "INVALID BUILDING TYPE";
}
