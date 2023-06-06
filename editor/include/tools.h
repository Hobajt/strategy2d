#pragma once

#include <vector>
#include <unordered_map>

#include <engine/engine.h>

class EditorContext;

namespace ToolType { enum { SELECT, TILE_PAINT, OBJECT_PLACEMENT, COUNT }; }
const char* toolType2str(int toolType);

//===== OperationRecord =====

struct OperationRecord {
    struct Entry {
        glm::ivec2 pos;
        int idx;
        int var;
        
        //painting  = prev_type, prev_variation
        //placement = obj_idx, action_id (add/remove)
    public:
        Entry() = default;
        Entry(const glm::ivec2& pos_, int idx_, int var_) : pos(pos_), idx(idx_), var(var_) {}
    };
public:
    bool paint;
    std::vector<Entry> actions;
};

//===== EditorTool =====

class EditorTool {
public:
    EditorTool(EditorContext& tools);
    EditorTool(EditorContext&&) = delete;

    virtual void GUI_Update() {}

    virtual void CustomSignal(int state, int id) {}
    virtual void Render() {}

    virtual void OnLMB(int state) = 0;
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

    void ClearPaint();
    void ClearAllPaint();

    void ApplyPaint();
    OperationRecord CreateOpRecord();
private:
    bool* paint = nullptr;
    glm::ivec2 size;

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
};

//===== ObjectPlacementTool =====

class ObjectPlacementTool : public EditorTool {
public:
    ObjectPlacementTool(EditorContext& tools);

    virtual void GUI_Update() override;

    virtual void Render() override;

    virtual void OnLMB(int state) override;
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
