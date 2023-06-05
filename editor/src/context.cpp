#include "context.h"

#include <random>

using namespace eng;

//===== EditorMap =====

EditorMap::EditorMap() : map(eng::Map()) {
    Init(map.Size());
}

EditorMap::EditorMap(const eng::Map& map_) : map(map_.Clone()), data(&map_) {
    Init(map.Size());
}

EditorMap::~EditorMap() {
    Release();
}

EditorMap::EditorMap(EditorMap&& m) noexcept {
    Move(std::move(m));
}

EditorMap& EditorMap::operator=(EditorMap&& m) noexcept {
    Release();
    Move(std::move(m));
    return *this;
}

void EditorMap::PaintRegion(const glm::ivec2& coord, int bl, int br, int tileType, bool randomVariations, int variationValue) {
    glm::ivec2 min = glm::max(coord - bl, glm::ivec2(0));
    glm::ivec2 max = glm::min(coord + br, map.Size());

    session_min = glm::min(session_min, min);
    session_max = glm::max(session_max, max);

    for(int y = min.y; y < max.y; y++) {
        for(int x = min.x; x < max.x; x++) {
            bool& flagged = bitmap[y*map.Width()+x];
            if(!flagged) {
                int var = randomVariations ? dist(gen) : variationValue;
                map.ModifyTile(y, x, tileType, var);
                flagged = true;
            }
        }
    }
}

void EditorMap::ClearPaint() {
    int w = map.Width();
    for(int y = session_min.y; y < session_max.y; y++) {
        for(int x = session_min.x; x < session_max.x; x++) {
            bitmap[y*w+x] = false;
        }
    }

    session_min = map.Size();
    session_max = glm::ivec2(0);
}

void EditorMap::Render() {
    map.Render();

    Camera& cam = Camera::Get();

    if(viz_state % 2 == 1) {
        //highlight tiles selected by current stroke
        float zIdx = -0.05f;
        glm::ivec2 size = map.Size();
        glm::vec4 clr = glm::vec4(1.f, 0.f, 0.f, 0.3f);
        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                if(bitmap[y*size.x+x]) {
                    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(x, y)), zIdx), cam.Mult(), clr));
                }
            }
        }
    }

    if(viz_state > 1) {
        //highlight current stroke's min/max coords
        float zIdx = -0.06f;
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(session_min)), zIdx), cam.Mult(), glm::vec4(0.f, 1.f, 0.f, 0.5f)));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(session_max-1)), zIdx), cam.Mult(), glm::vec4(0.f, 0.f, 1.f, 0.5f)));
    }
}

void EditorMap::DBG_VIZ(bool stroke_path, bool stroke_limits) {
    viz_state = int(stroke_path) + int(stroke_limits) * 2;
}

OperationRecord EditorMap::CreateOpRecord() {
    ASSERT_MSG(data != nullptr, "EditorMap wasn't properly initialized.");

    OperationRecord op = {};
    op.paint = true;

    int w = map.Width();
    for(int y = session_min.y; y < session_max.y; y++) {
        for(int x = session_min.x; x < session_max.x; x++) {
            if(bitmap[y*w+x]) {
                const TileData& td = data->GetTile(y, x);
                op.actions.push_back(OperationRecord::Entry(glm::ivec2(x, y), td.type, td.variation));
            }
        }
    }

    return op;
}

void EditorMap::RevertOperation(OperationRecord& op) {
    ASSERT_MSG(op.paint == true, "UndoPaintChanges - invalid operation type.");

    for(OperationRecord::Entry& entry : op.actions) {
        map.ModifyTile(entry.pos.y, entry.pos.x, entry.idx, entry.var);

        //invert the operation (can be used for redo)
        const TileData& td = data->GetTile(entry.pos.y, entry.pos.x);
        entry.idx = td.type;
        entry.var = td.variation;
    }
}

void EditorMap::SyncToLevelData(eng::Map& data) {
    data.OverrideMapData(map);

    for(int i = 0; i < map.Area(); i++)
        bitmap[i] = false;

    session_min = map.Size();
    session_max = glm::ivec2(0);
}

void EditorMap::Init(const glm::ivec2& size) {
    int area = size.x * size.y;

    bitmap = new bool[area];
    for(int i = 0; i < area; i++)
        bitmap[i] = false;
    
    session_min = size;
    session_max = glm::ivec2(0);

    gen = std::mt19937(std::random_device{}());
    dist = std::uniform_int_distribution(100);
}

void EditorMap::Move(EditorMap&& m) noexcept {
    map = std::move(m.map);
    bitmap = m.bitmap;
    data = m.data;

    session_min = m.session_min;
    session_max = m.session_max;
    viz_state = m.viz_state;

    gen = std::move(m.gen);
    dist = std::move(m.dist);

    m.bitmap = nullptr;
    m.data = nullptr;
}

void EditorMap::Release() noexcept {
    if(bitmap != nullptr)
        delete[] bitmap;
}

//===== EditorLevel =====

void EditorLevel::CommitPaintChanges() {
    map.SyncToLevelData(level.map);
}

void EditorLevel::UndoPaintChanges(OperationRecord& op) {
    map.RevertOperation(op);
    CommitPaintChanges();
}


//===== EditorContext =====

EditorContext::EditorContext() : input(EditorInputHandler(*this)), tools(EditorTools(*this)) {
    components.push_back(std::make_shared<HotkeysMenu>(*this));
    components.push_back(std::make_shared<DiplomacyMenu>(*this));
    components.push_back(std::make_shared<TechtreeMenu>(*this));
    components.push_back(std::make_shared<ToolsMenu>(*this));
    components.push_back(std::make_shared<InfoMenu>(*this));
    components.push_back(std::make_shared<FileMenu>(*this));
}

void EditorContext::Terrain_SetupNew(const glm::ivec2& size, const eng::TilesetRef& tileset) {
    level.level = Level(size, tileset);
    level.map = EditorMap(level.level.map);

    //camera reset
    Camera& camera = Camera::Get();
    camera.SetBounds(size);
    camera.Position(glm::vec2(size) * 0.5f - 0.5f);
    camera.ZoomToFit(glm::vec2(size) + 1.f);

    std::static_pointer_cast<InfoMenu, EditorComponent>(GetComponent<InfoMenu>())->NewLevelCreated();
}

int EditorContext::Terrain_Load(const std::string& filepath) {
    //TODO:
    return 1;
}

int EditorContext::Terrain_Save(const std::string& filepath) {
    //TODO:
    return 1;
}
