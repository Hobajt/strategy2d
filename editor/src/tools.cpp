#include "tools.h"
#include "context.h"

using namespace eng;

#define OP_STACK_SIZE 16

const char* toolType2str(int toolType) {
    static const char* names[ToolType::COUNT] = { "SELECT", "TILE_PAINT", "OBJECT_PLACEMENT" };
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

PaintTool::~PaintTool() {
    delete[] paint;
}

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
    ImGui::Checkbox("visualize stroke bounds", &viz_limits);
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
        int xM = std::min(coord.x+br, size.x);
        int ym = std::max(0, coord.y-bl);
        int yM = std::min(coord.y+br, size.y);

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
    glm::vec4 clr = glm::vec4(1.f, 0.f, 0.f, 0.3f);
    for(int y = 0; y < size.y; y++) {
        for(int x = 0; x < size.x; x++) {
            if(paint[y*size.x+x]) {
                Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(x, y)), zIdx), cam.Mult(), clr));
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
    delete[] paint;

    size = size_;
    int area = size.x * size.y;

    paint = new bool[area];
    for(int i = 0; i < area; i++) 
        paint[i] = false;
}

bool PaintTool::UndoOperation(OperationRecord& op) {
    if(!op.paint)
        return false;

    Map& map = context.level.map;
    for(OperationRecord::Entry& entry : op.actions) {
        const TileData& td = map.GetTile(entry.pos.y, entry.pos.x);
        int idx = td.type;
        int var = td.variation;

        map.ModifyTile(entry.pos.y, entry.pos.x, entry.idx, entry.var, false);

        //invert the operation (can be used for redo)
        entry.idx = idx;
        entry.var = var;
    }

    map.UpdateTileIndices();
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
    ClearPaint();
    painting = true;
}

void PaintTool::Stroke_MarkRegion(const glm::ivec2& coords, int brushLeft, int brushRight) {
    glm::ivec2 min = glm::max(coords - brushLeft, glm::ivec2(0));
    glm::ivec2 max = glm::min(coords + brushRight, size);

    session_min = glm::min(session_min, min);
    session_max = glm::max(session_max, max);

    for(int y = min.y; y < max.y; y++) {
        for(int x = min.x; x < max.x; x++) {
            paint[y*size.x+x] = true;
        }
    }
}

void PaintTool::OnStrokeFinish() {
    ApplyPaint();
    context.tools.PushOperation(CreateOpRecord());
    painting = false;
}

void PaintTool::ClearPaint() {
    for(int y = session_min.y; y < session_max.y; y++) {
        for(int x = session_min.x; x < session_max.x; x++) {
            paint[y*size.x+x] = false;
        }
    }
    session_min = size;
    session_max = glm::ivec2(0);
}

void PaintTool::ClearAllPaint() {
    for(int i = 0; i < size.x * size.y; i++)
        paint[i] = false;
    session_min = size;
    session_max = glm::ivec2(0);
}

void PaintTool::ApplyPaint() {
    context.level.map.ModifyTiles(paint, tileType, randomVariations, variationValue);
    session_min = size;
    session_max = glm::ivec2(0);
}

OperationRecord PaintTool::CreateOpRecord() {
    OperationRecord op = {};
    op.paint = true;

    Map& map = context.level.map;
    for(int y = session_min.y; y < session_max.y; y++) {
        for(int x = session_min.x; x < session_max.x; x++) {
            if(paint[y*size.x+x]) {
                const TileData& td = map.GetTile(y, x);
                op.actions.push_back(OperationRecord::Entry(glm::ivec2(x, y), td.type, td.variation));
            }
        }
    }
    return op;
}

//===== ObjectPlacementTool =====

ObjectPlacementTool::ObjectPlacementTool(EditorContext& context_) : EditorTool(context_) {}

void ObjectPlacementTool::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Object Placement Tool");
    ImGui::Separator();
    //TODO:
#endif
}

void ObjectPlacementTool::Render() {
    //TODO:
}

void ObjectPlacementTool::OnLMB(int state) {
    //TODO:
}

//===== EditorTools =====

EditorTools::EditorTools(EditorContext& context_)
    : context(context_), ops_undo(RingBuffer<OperationRecord>(OP_STACK_SIZE)), ops_redo(RingBuffer<OperationRecord>(OP_STACK_SIZE)) {
    tools.insert({ ToolType::SELECT, new SelectionTool(context_) });
    tools.insert({ ToolType::TILE_PAINT, new PaintTool(context_) });
    tools.insert({ ToolType::OBJECT_PLACEMENT, new ObjectPlacementTool(context_) });

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
}

void EditorTools::OnLMB(int state) {
    if(currentTool != nullptr)
        currentTool->OnLMB(state);
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
    LOG_TRACE("Editor::Undo");
    if(ops_undo.Size() > 0) {
        OperationRecord op = ops_undo.Pop();

        for(auto& [id, tool] : tools) {
            if(tool->UndoOperation(op)) {
                ops_redo.Push(op);
                break;
            }
        }
    }
}

void EditorTools::Redo() {
    LOG_TRACE("Editor::Redo");
    if(ops_redo.Size() > 0) {
        OperationRecord op = ops_redo.Pop();

        for(auto& [id, tool] : tools) {
            if(tool->UndoOperation(op)) {
                ops_undo.Push(op);
                break;
            }
        }
    }
}
