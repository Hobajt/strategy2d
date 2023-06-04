#pragma once

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

    void CustomSignal(int state, int id = 0);
    void Render();

    void OnLMB(int state);
    void OnHover();

    const std::unordered_map<int,EditorTool*>& GetTools() const;
    bool IsToolSelected(int toolType) const;
private:
    EditorContext& context;

    std::unordered_map<int,EditorTool*> tools;
    EditorTool* currentTool = nullptr;
};
