#include "engine/game/faction.h"
#include "engine/game/gameobject.h"

#include "engine/game/resources.h"
#include "engine/utils/dbg_gui.h"

#include "engine/game/player_controller.h"
#include "engine/game/controllers.h"

namespace eng {

    //===== Techtree =====

    void Techtree::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        //....
#endif
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
                return std::make_shared<FactionController>(std::move(entry), mapSize);
            // default:
            //     ENG_LOG_ERROR("Invalid FactionControllerID encountered.");
            //     throw std::runtime_error("");
        }
    }

    FactionController::FactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize)
        : id(idCounter++), name(std::move(entry.name)), techtree(std::move(entry.techtree)), colorIdx(entry.colorIdx), race(entry.race) {}

    void FactionController::AddDropoffPoint(const Building& building) {
        //TODO: maybe add some assertion checks? - coords overlap, multiple entries with the same ID, ...
        dropoff_points.push_back({ building.MinPos(), building.MaxPos(), building.OID(), building.DropoffMask() });

        //TODO: print out legit faction identifier
        ENG_LOG_TRACE("Faction {} - registered dropoff point '{}' (mask={}, count={})", 0, building, building.DropoffMask(), dropoff_points.size());
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

        //TODO: print out legit faction identifier
        ENG_LOG_TRACE("Faction {} - removed dropoff point '{}' (mask={}, count={})", 0, building, building.DropoffMask(), dropoff_points.size());
    }

    bool FactionController::CanBeBuilt(int buildingID, bool orcBuildings) const {
        //have a precomputed bitmap - condition check for each building type
        //have values for both races, in case you somehow obtain worker of the other race
        //do resources check on the fly
        return true;
    }

    BuildingDataRef FactionController::FetchBuildingData(int buildingID, bool orcBuildings) {
        if(CanBeBuilt(buildingID, orcBuildings))
            return Resources::LoadBuilding(buildingID, orcBuildings);
        else
            return nullptr;
    }

    int FactionController::UnitUpgradeTier(bool attack_upgrade, int upgrade_type) {
        //TODO: lookup in the research values, translate input arguments to proper research index
        return 2;
    }

    int FactionController::ProductionBoost(int res_idx) {
        //TODO: decide, based on building existence (lumber mill? -> lumber boost)
        //gold boost is computed based on town hall tier (0, 10, 25)
        return 10;
    }

    bool FactionController::ButtonConditionCheck(const FactionObject& src, const GUI::ActionButtonDescription& btn) const {
        //TODO: 
        //decode the action, that's supposed to be triggered by given button
        //evaluate conditions for said action (for example action is build -> check whether given building (payload_id) can be built)
        return true;
    }

    void FactionController::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Text("ID: %d | Name: %s", id, name.c_str());
        if(ImGui::CollapsingHeader("Techtree")) {
            techtree.DBG_GUI();
        }
        ImGui::Separator();

        Inner_DBG_GUI();
#endif
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
