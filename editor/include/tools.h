#pragma once

#include <vector>
#include <unordered_map>

#include <engine/engine.h>

class EditorContext;

namespace ToolType { enum { SELECT, TILE_PAINT, OBJECT_PLACEMENT, COUNT }; }
const char* toolType2str(int toolType);

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

    virtual void GUI_Update() override;

    virtual void CustomSignal(int state, int id) override;
    virtual void Render() override;

    virtual void OnLMB(int state) override;
private:
    void UpdateBrushSize(int newSize);
private:
    int brushSize = 1;
    int bl = 0;
    int br = 1;

    int selectedTileType = eng::TileType::GROUND;

    bool randomVariations = true;
    int variationValue = 0;

    bool viz_stroke = false;
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

//===== OperationStack =====

class OperationStack {
public:
    struct Operation {
        struct Entry {
            glm::ivec2 pos;
            //painting  = prev_type, prev_variation
            //placement = obj_idx, action_id (add/remove)
            int idx;
            int var;
        public:
            Entry() = default;
            Entry(const glm::ivec2& pos_, int idx_, int var_) : pos(pos_), idx(idx_), var(var_) {}
        };
        bool paint;
        std::vector<Entry> actions;
    };
public:
    OperationStack();
    OperationStack(int capacity);
    ~OperationStack();

    OperationStack(const OperationStack&) = delete;
    OperationStack& operator=(const OperationStack&) = delete;

    OperationStack(OperationStack&&) = delete;
    OperationStack& operator=(OperationStack&&) = delete;

    void Push(Operation&& op);
    Operation Pop();

    int Size() const { return size; }
private:
    Operation* buffer = nullptr;
    int capacity;
    
    int head;
    int size;
};

//===== EditorTools =====

struct EditorTools {
public:
    EditorTools(EditorContext& context);
    EditorTools(EditorContext&& context) = delete;
    ~EditorTools();

    void GUI_Update();

    void SwitchTool(int toolType);

    void CustomSignal(int state, int id = 0);
    void Render();

    void OnLMB(int state);
    void OnHover();

    const std::unordered_map<int,EditorTool*>& GetTools() const;
    bool IsToolSelected(int toolType) const;

    void PushOperation(OperationStack::Operation&& op);
private:
    EditorContext& context;

    std::unordered_map<int,EditorTool*> tools;
    EditorTool* currentTool = nullptr;

    OperationStack ops;
};
