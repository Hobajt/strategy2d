#pragma once

#include <vector>
#include <unordered_map>

#include <engine/engine.h>

class EditorContext;

namespace ToolType { enum { SELECT, TILE_PAINT, OBJECT_PLACEMENT, STARTING_LOCATION, COUNT }; }
const char* toolType2str(int toolType);

//===== OperationRecord =====

namespace ObjectOperation {
    enum { ADD, REMOVE, MOVE };
}//namespace ObjectOperation

struct OperationRecord {
    //TileRecord fields:
    //  paint == true  -> original meaning
    //  paint == false -> tileType=obj_idx, variation=action_id (action=add or remove)
public:
    bool paint;
    std::vector<eng::TileRecord> actions;
};

namespace UndoRedoUpdateType { enum { NONE = 0, ID_UPDATE = 1, }; }

struct UndoRedoUpdateData {
    int flag = 0;
    std::pair<eng::ObjectID, eng::ObjectID> id_update;
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
    virtual void LevelLoaded(const glm::ivec2& size) {}

    //Return true when operation matches given tool (and was successfully undone).
    virtual bool UndoOperation(OperationRecord& op, UndoRedoUpdateData& info_update) { return false; }
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
private:
    bool selection = false;
    glm::vec2 pos;
    glm::vec2 size;

    eng::ObjectID targetID = eng::ObjectID();
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
    virtual void LevelLoaded(const glm::ivec2& size) override;

    virtual bool UndoOperation(OperationRecord& op, UndoRedoUpdateData& info_update) override;
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
    virtual void OnRMB(int state) override;

    virtual bool UndoOperation(OperationRecord& op, UndoRedoUpdateData& info_update) override;
private:
    void RenderTileHighlight(const glm::ivec2& location, const glm::ivec2& size);
    void RenderObjectViz(const glm::ivec2& location, const glm::ivec2& size, const eng::FactionObjectDataRef& data);

    bool IsLocationValid(const glm::ivec2& location, const glm::ivec2& size, const eng::FactionObjectDataRef& data);

    eng::ObjectID AddObject(const glm::ivec3& num_id, const glm::ivec2& location, int factionIdx, bool add_op_record = true);
    eng::ObjectID AddObject(const eng::FactionObjectDataRef& data, const glm::ivec2& location, int factionIdx, bool add_op_record = true);
    int MoveObject(const eng::ObjectID& id, const glm::ivec2& new_pos, bool add_op_record = true, glm::ivec2* out_prev_pos = nullptr);
private:
    bool drag = false;
    eng::ObjectID dragID = eng::ObjectID();
    glm::vec2 dragCoords = glm::vec2(0.f);

    int objIdx = -1;
    int objRace = 0;
    glm::ivec3 numID = glm::ivec3(-1);
    bool place_buildings = false;
    eng::FactionObjectDataRef objdata = nullptr;
    eng::TextureRef shadows = nullptr;

    eng::FactionControllerRef faction = nullptr;
    int factionIdx = 0;
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

    glm::ivec2 PlayerCameraPosition() const;

    virtual void NewLevelCreated(const glm::ivec2& size) override;
    virtual void LevelLoaded(const glm::ivec2& size) override;
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
    void LevelLoaded(const glm::ivec2& size);

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
    void UndoRedo_InfoUpdate(const UndoRedoUpdateData& data);
private:
    EditorContext& context;

    std::unordered_map<int,EditorTool*> tools;
    EditorTool* currentTool = nullptr;

    eng::RingBuffer<OperationRecord> ops_undo;
    eng::RingBuffer<OperationRecord> ops_redo;
};
