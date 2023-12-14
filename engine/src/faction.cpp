#include "engine/game/faction.h"
#include "engine/game/gameobject.h"

#include "engine/game/resources.h"
#include "engine/utils/dbg_gui.h"

#include "engine/game/player_controller.h"
#include "engine/game/controllers.h"

#define POPULATION_CAP 200

namespace eng {

    namespace FactionControllerID {
        int RandomAIMindset() {
            //TODO:
            return INVALID;
            // return Random::UniformInt() ...;
        }
    }

    const char* GameResourceName(int res_idx) {
        constexpr static std::array<const char*, 3> names = { "Gold", "Lumber", "Oil" };
        return names[res_idx];
    }

    FactionsFile::FactionEntry::FactionEntry(int controllerID_, int race_, const std::string& name_, int colorIdx_)
        : controllerID(controllerID_), race(race_), name(name_), colorIdx(colorIdx_), techtree(Techtree{}) {}

    //===== FactionController =====

    int FactionController::idCounter = 0;

    FactionControllerRef FactionController::CreateController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize) {
        switch(entry.controllerID) {
            case FactionControllerID::LOCAL_PLAYER:
                return std::make_shared<PlayerFactionController>(std::move(entry), mapSize);
            case FactionControllerID::NATURE:
                return std::make_shared<NatureFactionController>(std::move(entry), mapSize);
            default:
                return std::make_shared<FactionController>(std::move(entry), mapSize, entry.controllerID);
            // default:
            //     ENG_LOG_ERROR("Invalid FactionControllerID encountered.");
            //     throw std::runtime_error("");
        }
    }

    FactionController::FactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize, int controllerID_)
        : id(idCounter++), name(std::move(entry.name)), techtree(std::move(entry.techtree)), colorIdx(entry.colorIdx), race(entry.race), controllerID(controllerID_) {}

    FactionsFile::FactionEntry FactionController::Export() {
        FactionsFile::FactionEntry entry = {};

        entry.controllerID = controllerID;
        entry.colorIdx = colorIdx;
        entry.race = race;
        entry.name = name;
        entry.techtree = techtree;
        entry.occlusionData = ExportOcclusion();

        return entry;
    }

    void FactionController::ObjectAdded(const FactionObjectDataRef& data) {
        ASSERT_MSG(data != nullptr, "FactionController::ObjectAdded - data is nullptr.");
        // ENG_LOG_INFO("ADDING {}, {} ({})", data->name, data->num_id, data->objectType);
        switch(data->objectType) {
            case ObjectType::BUILDING:
                stats.buildingCount++;
                if(data->num_id[1] < BuildingType::COUNT) {
                    stats.buildings[data->num_id[1]]++;
                }
                break;
            case ObjectType::UNIT:
                stats.unitCount++;
                break;
            default:
                ENG_LOG_ERROR("FactionController::ObjectAdded - invalid object type (neither building nor unit - {})", data->objectType);
                throw std::runtime_error("shouldn't happen");
                break;
        }
        PopulationUpdate();
    }

    void FactionController::ObjectRemoved(const FactionObjectDataRef& data) {
        ASSERT_MSG(data != nullptr, "FactionController::ObjectRemoved - data is nullptr.");
        // ENG_LOG_INFO("REMOVING {}, {} ({})", data->name, data->num_id, data->objectType);
        switch(data->objectType) {
            case ObjectType::BUILDING:
                stats.buildingCount--;
                if(data->num_id[1] < BuildingType::COUNT) {
                    stats.buildings[data->num_id[1]]--;
                    if(stats.buildings[data->num_id[1]] < 0) {
                        ENG_LOG_WARN("FactionController::ObjectRemoved - negative count detected!");
                        stats.buildings[data->num_id[1]] = 0;
                    }
                }
                if(stats.buildingCount < 0) {
                    ENG_LOG_WARN("FactionController::ObjectRemoved - negative count detected!");
                    stats.buildingCount = 0;
                }
                break;
            case ObjectType::UNIT:
                stats.unitCount--;
                if(stats.unitCount < 0) {
                    ENG_LOG_WARN("FactionController::ObjectRemoved - negative count detected!");
                    stats.unitCount = 0;
                }
                break;
            default:
                ENG_LOG_ERROR("FactionController::ObjectAdded - invalid object type (neither building nor unit - {})", data->objectType);
                throw std::runtime_error("shouldn't happen");
                break;
        }
        PopulationUpdate();
    }

    void FactionController::ObjectUpgraded(const FactionObjectDataRef& old_data, const FactionObjectDataRef& new_data) {
        if(old_data->objectType != ObjectType::BUILDING)
            return;
        
        int old_type = old_data->num_id[1];
        int new_type = new_data->num_id[1];

        stats.buildings[old_type]--;
        stats.buildings[new_type]++;

        if(stats.buildingCount < 0) {
            ENG_LOG_WARN("FactionController::ObjectUpgraded - negative count detected!");
            stats.buildingCount = 0;
        }

        stats.tier = 0;
        if(stats.buildings[BuildingType::CASTLE] != 0)
            stats.tier = 3;
        else if(stats.buildings[BuildingType::KEEP] != 0)
            stats.tier = 2;
        else if(stats.buildings[BuildingType::TOWN_HALL] != 0)
            stats.tier = 1;
    }

    void FactionController::AddDropoffPoint(const Building& building) {
        //TODO: maybe add some assertion checks? - coords overlap, multiple entries with the same ID, ...
        dropoff_points.push_back({ building.MinPos(), building.MaxPos(), building.OID(), building.DropoffMask() });

        ENG_LOG_TRACE("Faction {} - registered dropoff point '{}' (mask={}, count={})", ID(), building, building.DropoffMask(), dropoff_points.size());
    }

    void FactionController::RemoveDropoffPoint(const Building& building) {
        buildingMapCoords entry = { building.MinPos(), building.MaxPos(), building.OID(), building.DropoffMask() };
        auto pos = std::find(dropoff_points.begin(), dropoff_points.end(), entry);
        if(pos != dropoff_points.end()) {
            dropoff_points.erase(pos);
        }
        else {
            ENG_LOG_WARN("Dropoff point entry not found.");
        }

        ENG_LOG_TRACE("Faction {} - removed dropoff point '{}' (mask={}, count={})", ID(), building, building.DropoffMask(), dropoff_points.size());
    }

    bool FactionController::CanBeBuilt(int buildingID, bool orcBuildings) const {
        //have a precomputed bitmap - condition check for each building type
        //have values for both races, in case you somehow obtain worker of the other race
        //do resources check here as well
        //dont handle map placement validation here, handled elsewhere
        return true;
    }

    void FactionController::RefundResources(const glm::ivec3& refund) {
        ASSERT_MSG(refund.x >= 0 && refund.y >= 0 && refund.z >= 0, "Cannot refund negative values.");
        resources += refund;
        ENG_LOG_FINE("FactionController::RefundResources - {} returned (val={})", refund, resources);
    }

    BuildingDataRef FactionController::FetchBuildingData(int buildingID, bool orcBuildings) {
        if(CanBeBuilt(buildingID, orcBuildings))
            return Resources::LoadBuilding(buildingID, orcBuildings);
        else
            return nullptr;
    }

    int FactionController::UnitUpgradeTier(bool attack_upgrade, int upgrade_type, bool isOrc) {
        return Tech().UnitUpgradeTier(attack_upgrade, upgrade_type, isOrc);
    }

    int FactionController::ProductionBoost(int res_idx) {
        switch(res_idx) {
            case 0:     //gold
                return (stats.tier == 3) * 25 + (stats.tier == 2) * 10;
            case 1:     //wood
                return (stats.buildings[BuildingType::LUMBER_MILL] > 0) * 25;
            case 2:     //oil
                return (stats.buildings[BuildingType::OIL_REFINERY] > 0) * 25;
            default:
                ENG_LOG_ERROR("There's only 3 resource types (idx={})", res_idx);
                throw std::runtime_error("invalid resource type");
        }
        return 10;
    }

    bool FactionController::ActionButtonSetup(GUI::ActionButtonDescription& btn, bool isOrc) const {
        switch(btn.command_id) {
            case GUI::ActionButton_CommandType::RESEARCH:
                return techtree.SetupResearchButton(btn, isOrc);
            case GUI::ActionButton_CommandType::TRAIN:
                //requirements check (building preconditions)
                if(TrainingPreconditionsMet(btn.payload_id)) {
                    techtree.SetupTrainButton(btn, isOrc);
                    return true;
                }
                else {
                    return false;
                }
                break;
            case GUI::ActionButton_CommandType::CAST:
                return techtree.SetupSpellButton(btn, isOrc);
            case GUI::ActionButton_CommandType::BUILD:
                return BuildingPreconditionsMet(btn.payload_id);
            default:
                return true;
        }
    }

    bool FactionController::TrainingPreconditionsMet(int unit_type) const {
        switch(unit_type) {
            case UnitType::ARCHER:
            case UnitType::RANGER:
                return (stats.buildings[BuildingType::LUMBER_MILL] > 0);
            case UnitType::KNIGHT:
            case UnitType::PALADIN:
                return (stats.buildings[BuildingType::STABLES] > 0);
            case UnitType::BALLISTA:
                return (stats.buildings[BuildingType::BLACKSMITH] > 0);
            case UnitType::BATTLESHIP:
            case UnitType::TRANSPORT:
                return (stats.buildings[BuildingType::FOUNDRY] > 0);
            case UnitType::SUBMARINE:
                return (stats.buildings[BuildingType::INVENTOR] > 0);
            default:
                return true;
        }
    }

    bool FactionController::BuildingPreconditionsMet(int building_type) const {
        switch(building_type) {
            case BuildingType::STABLES:
            case BuildingType::INVENTOR:
                return (stats.tier >= 2);
            case BuildingType::CHURCH:
            case BuildingType::WIZARD_TOWER:
            case BuildingType::DRAGON_ROOST:
                return (stats.tier >= 3);
            case BuildingType::SHIPYARD:
                return stats.buildings[BuildingType::LUMBER_MILL] > 0;
            case BuildingType::FOUNDRY:
            case BuildingType::OIL_REFINERY:
                return stats.buildings[BuildingType::SHIPYARD] > 0;
            default:
                return true;
        }
    }

    bool FactionController::CastConditionCheck(const Unit& src, int payload_id) const {
        //TODO:
        //also do resource check here
        return true;
    }

    void FactionController::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::PushID(this);
        ImGui::Text("ID: %d | Race: %s | Name: %s", id, race == 0 ? "HU" : "OC", name.c_str());
        if(ImGui::CollapsingHeader("Techtree")) {
            ImGui::PushID(&techtree);
            techtree.DBG_GUI();
            ImGui::PopID();
        }

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        // flags |= ImGuiTableFlags_NoHostExtendY;
        flags |= ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV;
        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float cell_width = TEXT_BASE_WIDTH * 3.f;

        if(ImGui::CollapsingHeader("Stats")) {
            ImGui::Text("Population: %d/%d", population[0], population[1]);
            ImGui::Text("Resources: %d G, %d W, %d O", resources[0], resources[1], resources[2]);
            ImGui::Text("Units: %d, Buildings:  %d", stats.unitCount, stats.buildingCount);

            ImGui::Text("By building type:");
            if (ImGui::BeginTable("table1", stats.buildings.size(), flags)) {
                for(int x = 0; x < stats.buildings.size(); x++)
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

                ImGuiStyle& style = ImGui::GetStyle();
                ImGui::TableNextRow();
                for(int i = 0; i < stats.buildings.size(); i++) {
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", stats.buildings[i]);
                }
                
                ImGui::EndTable();
            }
            ImGui::Text("Tier: %d", stats.tier);
        }
        ImGui::Separator();

        Inner_DBG_GUI();
        ImGui::PopID();
#endif
    }

    void FactionController::PopulationUpdate() {
        population = glm::ivec2(stats.unitCount, std::min(stats.buildings[BuildingType::TOWN_HALL] + stats.buildings[BuildingType::FARM] * 4, POPULATION_CAP));
    }

    //===== DiplomacyMatrix =====

    DiplomacyMatrix::DiplomacyMatrix(int factionCount_) : factionCount(factionCount_) {
        relation = new int[factionCount * factionCount];
        for(int i = 0; i < factionCount * factionCount; i++)
            relation[i] = 0;
        //TODO: could maybe only use half of the matrix, since it's a symmetrical relation
    }

    DiplomacyMatrix::DiplomacyMatrix(int factionCount, const std::vector<glm::ivec3>& relations) : DiplomacyMatrix(factionCount) {
        for(auto& entry : relations) {
            operator()(entry.x, entry.y) = entry.z;
            operator()(entry.y, entry.x) = entry.z;
        }
    }

    DiplomacyMatrix::~DiplomacyMatrix() {
        Release();
    }

    DiplomacyMatrix::DiplomacyMatrix(DiplomacyMatrix&& d) noexcept {
        Move(std::move(d));
    }

    DiplomacyMatrix& DiplomacyMatrix::operator=(DiplomacyMatrix&& d) noexcept {
        Release();
        Move(std::move(d));
        return *this;
    }

    bool DiplomacyMatrix::AreHostile(int f1, int f2) const {
        ASSERT_MSG(relation != nullptr, "DiplomacyMatrix is not initialized properly!");

        //invalid faction idx (can just mean empty tile)
        if((unsigned int)(f1) >= factionCount || (unsigned int)(f2) >= factionCount)
            return false;
        return this->operator()(f1, f2) != 0;
    }

    std::vector<glm::ivec3> DiplomacyMatrix::Export() const { 
        std::vector<glm::ivec3> res;

        for(int i = 0; i < factionCount; i++) {
            for(int j = i+1; j < factionCount; j++) {
                int val = operator()(i,j);
                if(val != 0) {
                    res.push_back(glm::ivec3(i, j, val));
                }
            }
        }

        return res;
    }

    void DiplomacyMatrix::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        if(relation == nullptr || factionCount < 1) {
            ImGui::Text("Uninitialized or invalid");
            return;
        }

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        // flags |= ImGuiTableFlags_NoHostExtendY;
        flags |= ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV;

        int num_cols = factionCount;
        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float cell_width = TEXT_BASE_WIDTH * 3.f;

        ImU32 clr_red    = ImGui::GetColorU32(ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
        ImU32 clr_green  = ImGui::GetColorU32(ImVec4(0.1f, 1.0f, 0.1f, 1.0f));
        ImU32 clr_gray   = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImU32 clr_purple = ImGui::GetColorU32(ImVec4(0.59f, 0.21f, 0.76f, 1.0f));

        if (ImGui::BeginTable("table1", num_cols, flags)) {
            for(int x = 0; x < num_cols; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

            ImGuiStyle& style = ImGui::GetStyle();
            for(int y = 0; y < factionCount; y++) {
                ImGui::TableNextRow();
                for(int x = 0; x < factionCount; x++) {
                    ImGui::TableNextColumn();

                    int value = operator()(y, x);
                    ImGui::Text("%d", value);
                    if(x != y)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, (value == 0) ? clr_gray : clr_red);
                    else
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, clr_purple);
                }
            }
            ImGui::EndTable();
        }
#endif
    }

    int& DiplomacyMatrix::operator()(int y, int x) {
        ASSERT_MSG((unsigned int)(y) < factionCount && (unsigned int)(x) < factionCount, "DiplomacyMatrix - factionIdx out of bounds.");
        return relation[x + y*factionCount];
    }

    const int& DiplomacyMatrix::operator()(int y, int x) const {
        ASSERT_MSG((unsigned int)(y) < factionCount && (unsigned int)(x) < factionCount, "DiplomacyMatrix - factionIdx out of bounds.");
        return relation[x + y*factionCount];
    }


    void DiplomacyMatrix::Move(DiplomacyMatrix&& d) noexcept {
        relation = d.relation;
        factionCount = d.factionCount;
        d.factionCount = 0;
        d.relation = nullptr;
    }

    void DiplomacyMatrix::Release() noexcept {
        if(relation != nullptr) {
            delete[] relation;
            factionCount = 0;
        }
    }

    //===== Factions =====

    Factions::Factions(FactionsFile&& data, const glm::ivec2& mapSize) : initialized(true), player(nullptr), diplomacy(DiplomacyMatrix((int)data.factions.size(), data.diplomacy)) {
        for(FactionsFile::FactionEntry& entry : data.factions) {
            factions.push_back(FactionController::CreateController(std::move(entry), mapSize));

            //setup reference to player faction controller
            if(entry.controllerID == FactionControllerID::LOCAL_PLAYER) {
                ASSERT_MSG(player == nullptr, "There can never be more than 1 local player owned factions!");
                player = std::static_pointer_cast<PlayerFactionController>(factions.back());
            }
        }

        if(player == nullptr) {
            initialized = false;
        }
    }

    FactionControllerRef Factions::operator[](int i) {
        return factions.at(i);
    }

    const FactionControllerRef Factions::operator[](int i) const {
        return factions.at(i);
    }

    void Factions::Update(Level& level) {
        ASSERT_MSG(initialized, "Factions are not initialized properly!");
        
        for(auto& faction : factions) {
            faction->Update(level);
        }
    }

    FactionsFile Factions::Export() {
        FactionsFile file = {};

        for(FactionControllerRef& faction : factions) {
            file.factions.push_back(faction->Export());
        }

        file.diplomacy = diplomacy.Export();

        return file;
    }

    void Factions::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Factions");

        ImGui::Text("Faction count: %d", (int)factions.size());
        if(ImGui::CollapsingHeader("Diplomacy")) {
            diplomacy.DBG_GUI();
        }
        ImGui::Separator();

        char buf[256];
        for(int i = 0; i < (int)factions.size(); i++) {
            snprintf(buf, sizeof(buf), "Faction[%d]", i);
            if(ImGui::CollapsingHeader(buf)) {
                ImGui::Indent();
                factions[i]->DBG_GUI();
                ImGui::Unindent();
            }
        }

        ImGui::End();
#endif
    }

}//namespace eng
