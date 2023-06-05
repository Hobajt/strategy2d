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
        "GROUND", "MUD",
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
            if(ImGui::Selectable(tileIdx2name(i), selectedTileType == i)) {
                selectedTileType = i;
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
    if(ImGui::Checkbox("visualize stroke path", &viz_stroke))
        context.level.map.DBG_VIZ(viz_stroke, viz_limits);
    if(ImGui::Checkbox("visualize stroke bounds", &viz_limits))
        context.level.map.DBG_VIZ(viz_stroke, viz_limits);
#endif
}

void PaintTool::CustomSignal(int state, int id) {
    UpdateBrushSize(brushSize+state);
}

void PaintTool::Render() {
    if(hover) {
        Camera& cam = Camera::Get();
        glm::ivec2 ms = context.level.map.Map().Size();

        //hovering tile coordinate (in map space)
        glm::ivec2 coord = glm::ivec2(cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f) + 0.5f);

        //corner points for the highlights
        int xm = std::max(0, coord.x-bl);
        int xM = std::min(coord.x+br, ms.x);
        int ym = std::max(0, coord.y-bl);
        int yM = std::min(coord.y+br, ms.y);

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
}

void PaintTool::OnLMB(int state) {
    Camera& cam = Camera::Get();
    EditorMap& map = context.level.map;
    glm::ivec2 bounds = map.Map().Size();

    glm::ivec2 coord = glm::ivec2(cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f) + 0.5f);

    switch(state) {
        case InputButtonState::DOWN:
            map.ClearPaint();
        case InputButtonState::PRESSED:
            map.PaintRegion(coord, bl, br, selectedTileType, randomVariations, variationValue);
            hover = true;
            break;
        case InputButtonState::UP:
            context.tools.PushOperation(map.CreateOpRecord());
            context.level.CommitPaintChanges();
            break;
    }
}

void PaintTool::UpdateBrushSize(int newSize) {
    brushSize = newSize;
    if(brushSize < 1)
        brushSize = 1;
    br = (brushSize+1) / 2;
    bl = brushSize - br;
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
        
        if(op.paint) {
            context.level.UndoPaintChanges(op);
            ops_redo.Push(op);
        }
        else {
            //TODO: undo for placement op
        }
    }
}

void EditorTools::Redo() {
    LOG_TRACE("Editor::Redo");
    if(ops_redo.Size() > 0) {
        OperationRecord op = ops_redo.Pop();
        
        if(op.paint) {
            context.level.UndoPaintChanges(op);
            ops_undo.Push(op);
        }
        else {
            //TODO: redo for placement op
        }
    }
}
