#pragma once

#include <vector>
#include <unordered_map>

#include <engine/engine.h>

class EditorContext;

namespace ToolType { enum { SELECT, TILE_PAINT, OBJECT_PLACEMENT, STARTING_LOCATION, COUNT }; }
const char* toolType2str(int toolType);

//===== OperationRecord =====

struct OperationRecord {
    //TileRecord fields:
    //  paint == true  -> original meaning
    //  paint == false -> tileType=obj_idx, variation=action_id (action=add or remove)
public:
    bool paint;
    std::vector<eng::TileRecord> actions;
};

//===== EditorTool =====

class EditorTool {
public:
    EditorTool(EditorContext& tools);
    EditorTool(EditorContext&&) = delete;

    virtual void GUI_Update() {}

    virtual void CustomSignal(int state, int id) {}

    virtual void Render() {}
    virtual void Render_NotSelected() {}

    virtual void OnLMB(int state) = 0;
    virtual void OnRMB(int state) {}
    virtual void OnHover() { hover = true; }

    virtual void NewLevelCreated(const glm::ivec2& size) {}

    //Return true when operation matches given tool (and was successfully undone).
    virtual bool UndoOperation(OperationRecord& op) { return false; }
protected:
    EditorContext& context;
    bool hover = false;
};

//===== SelectionTool =====

class SelectionTool : public EditorTool {
public:
    SelectionTool(EditorContext& tools);

    virtual void GUI_Update() override;

    virtual void Render() override;

    virtual void OnLMB(int state) override;
};

//===== PaintTool =====

class PaintTool : public EditorTool {
public:
    PaintTool(EditorContext& tools);
    ~PaintTool();

    PaintTool(const PaintTool&) = delete;
    PaintTool(PaintTool&&) = delete;

    virtual void GUI_Update() override;

    virtual void CustomSignal(int state, int id) override;
    virtual void Render() override;

    virtual void OnLMB(int state) override;

    virtual void NewLevelCreated(const glm::ivec2& size) override;

    virtual bool UndoOperation(OperationRecord& op) override;
private:
    void UpdateBrushSize(int newSize);

    void OnStrokeStart();
    void Stroke_MarkRegion(const glm::ivec2& coords, int brushLeft, int brushRight);
    void OnStrokeFinish();

    void ApplyPaint();
private:
    eng::PaintBitmap paint;

    glm::ivec2 session_min;
    glm::ivec2 session_max;
    bool painting = false;

    //==== options ====

    int brushSize = 1;
    int bl = 0;
    int br = 1;

    int tileType = eng::TileType::GROUND1;

    bool randomVariations = true;
    int variationValue = 0;

    bool viz_limits = false;
    bool viz_paintReach = false;
};

//===== ObjectPlacementTool =====

class ObjectPlacementTool : public EditorTool {
public:
    ObjectPlacementTool(EditorContext& tools);

    virtual void GUI_Update() override;

    virtual void Render() override;

    virtual void OnLMB(int state) override;
};

//===== StartingLocationTool =====

class StartingLocationTool : public EditorTool {
public:
    StartingLocationTool(EditorContext& tools);

    virtual void GUI_Update() override;
    virtual void Render() override;
    virtual void Render_NotSelected() override;
    virtual void OnLMB(int state) override;
    virtual void OnRMB(int state) override;
private:
    int LocationIdx(const glm::ivec2& loc) const;
    void InnerRender();

    void LocationsUpdated();
private:
    std::vector<glm::ivec2> startingLocations;
    bool renderStartingLocations = true;

    bool drag = false;
    int dragIdx = -1;
    glm::vec2 dragCoords = glm::vec2(0.f);
};

//===== EditorTools =====

struct EditorTools {
public:
    EditorTools(EditorContext& context);
    EditorTools(EditorContext&& context) = delete;
    ~EditorTools();

    void GUI_Update();

    void SwitchTool(int toolType);

    void NewLevelCreated(const glm::ivec2& size);

    void CustomSignal(int state, int id = 0);
    void Render();

    void OnLMB(int state);
    void OnRMB(int state);
    void OnHover();

    const std::unordered_map<int,EditorTool*>& GetTools() const;
    bool IsToolSelected(int toolType) const;

    void PushOperation(OperationRecord&& op);

    void Undo();
    void Redo();
private:
    EditorContext& context;

    std::unordered_map<int,EditorTool*> tools;
    EditorTool* currentTool = nullptr;

    eng::RingBuffer<OperationRecord> ops_undo;
    eng::RingBuffer<OperationRecord> ops_redo;
};
