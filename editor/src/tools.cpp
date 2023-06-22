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

bool PaintTool::UndoOperation(OperationRecord& op) {
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
    if(ops_undo.Size() > 0) {
        LOG_TRACE("Editor::Undo ({} left)", ops_undo.Size()-1);
        OperationRecord op = ops_undo.Pop();

        for(auto& [id, tool] : tools) {
            if(tool->UndoOperation(op)) {
                ops_redo.Push(op);
                break;
            }
        }
    }
    else {
        LOG_TRACE("Editor::Undo - there nothing to undo");
    }
}

void EditorTools::Redo() {
    if(ops_redo.Size() > 0) {
        LOG_TRACE("Editor::Redo ({} left)", ops_redo.Size()-1);
        OperationRecord op = ops_redo.Pop();

        for(auto& [id, tool] : tools) {
            if(tool->UndoOperation(op)) {
                ops_undo.Push(op);
                break;
            }
        }
    }
    else {
        LOG_TRACE("Editor::Redo - there nothing to redo");
    }
}
