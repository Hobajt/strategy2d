#pragma once

#include <memory>
#include <vector>
#include <tuple>

#include "engine/game/map.h"
#include "engine/game/gameobject.h"
#include "engine/game/techtree.h"

namespace eng {

    class Level;
    class Building;

    //can setup enum for the IDs as well (PLAYER_LOCAL, PLAYER_REMOTE1, PLAYER_REMOTEn, AI_EASY, ...)
    namespace FactionControllerID {
        enum { INVALID = 0, LOCAL_PLAYER, NATURE, COUNT };

        int RandomAIMindset();
    }

    namespace ResourceBit { enum { GOLD = 1, WOOD = 2, OIL = 4 }; }
    const char* GameResourceName(int res_idx);

    //===== FactionsFile =====

    //Struct for serialization.
    struct FactionsFile {
        struct FactionEntry {
            int id;
            int controllerID;
            int colorIdx;
            int race;
            std::string name;
            Techtree techtree;
            std::vector<uint32_t> occlusionData;
        public:
            FactionEntry() = default;
            FactionEntry(int controllerID, int race, const std::string& name, int colorIdx, int id);
        };
    public:
        std::vector<FactionEntry> factions;
        std::vector<glm::ivec3> diplomacy;
    };

    //Wrapper for various stat values tracked within the faction controller objects.
    struct FactionStats {
        int unitCount = 0;
        int buildingCount = 0;
        int minionCount = 0;
        std::array<int, BuildingType::COUNT> buildings = {0};
        int tier = 0;
    };

    //===== FactionsEditor =====

    class Factions;

    //For editor implementation, has deeper access to the Factions data.
    class FactionsEditor {
    protected:
        DiplomacyMatrix& Diplomacy(Factions& factions);

        void AddFaction(Factions& factions, const char* name, int controllerID, int colorIdx);
        void RemoveFaction(Factions& factions, int idx);
        void UpdateFactionCount(Factions& factions, int count);
        void CheckForRequiredFactions(Factions& factions, bool& has_player, bool& has_nature);

        void Faction_Name(FactionController& f, const char* name);
        int& Faction_Race(FactionController& f);
        int& Faction_Color(FactionController& f);
        int& Faction_ControllerType(FactionController& f);
    };
    
    //===== FactionController =====

    class FactionController;
    using FactionControllerRef = std::shared_ptr<FactionController>;

    //TODO: make abstract (once I implement additional controllers)
    class FactionController {
        friend class FactionsEditor;
    public:
        //Factory method, creates proper controller type (based on controllerID).
        static FactionControllerRef CreateController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize);
    public:
        FactionController() = default;
        FactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize, int controllerID);

        int ID() const { return id; }
        int Race() const { return race; }
        int ControllerID() const { return controllerID; }
        std::string Name() const { return name; }

        virtual int GetColorIdx(const glm::ivec3& num_id) const { return colorIdx; }

        FactionsFile::FactionEntry Export();

        //triggered when spawning objects, to increment faction's stats & counters (such as population)
        void ObjectAdded(const FactionObjectDataRef& data);
        void ObjectRemoved(const FactionObjectDataRef& data);
        void ObjectUpgraded(const FactionObjectDataRef& old_data, const FactionObjectDataRef& new_data);

        void AddDropoffPoint(const Building& building);
        void RemoveDropoffPoint(const Building& building);
        const std::vector<buildingMapCoords>& DropoffPoints() const { return dropoff_points; }

        void RefundResources(const glm::ivec3& refund);
        void ResourcesUptick(int resource_type);

        //Returns true if all the conditions for given building are met (including resources check).
        bool CanBeBuilt(int buildingID, bool orcBuildings) const;

        //Returns data object for particular type of building. Returns nullptr if the building cannot be built.
        BuildingDataRef FetchBuildingData(int buildingID, bool orcBuildings);

        //Lookup research tier for unit upgrades.
        int UnitUpgradeTier(bool attack_upgrade, int upgrade_type, bool isOrc);

        virtual void Update(Level& level) {}

        //Only for player controller, signals that GUI panel needs to be updated.
        virtual void SignalGUIUpdate() {}
        virtual void SignalGUIUpdate(const FactionObject& obj) {}

        glm::ivec3 Resources() const { return resources; }
        glm::ivec2 Population() const { return population; }

        int ProductionBoost(int res_idx);

        //Button condition check, button description may get modified by calling the method.
        //Doesn't check for resources, returns false if given button doesn't meet the conditions and shouldn't be displayed.
        bool ActionButtonSetup(GUI::ActionButtonDescription& btn, bool isOrc) const;

        bool TrainingPreconditionsMet(int unit_type) const;
        bool BuildingPreconditionsMet(int building_type) const;
        bool UpgradePreconditionsMet(int building_type) const;
        bool AdvancedStructures_ButtonCheck(GUI::ActionButtonDescription& btn) const;

        //Check for both the spell availability and price, returns true if both acceptable.
        bool CastConditionCheck(const Unit& src, int payload_id) const;

        Techtree& Tech() { return techtree; }
        const Techtree& Tech() const { return techtree; }

        void DBG_GUI();
    private:
        virtual void Inner_DBG_GUI() {}
        virtual std::vector<uint32_t> ExportOcclusion() { return {}; }

        void PopulationUpdate();
    private:
        Techtree techtree;
        FactionStats stats;
        std::vector<buildingMapCoords> dropoff_points;

        int id = -1;
        int colorIdx = 0;
        std::string name = "unnamed_faction";
        int controllerID = -1;

        int race = 0;
        glm::ivec3 resources = glm::ivec3(0);
        glm::ivec2 population = glm::ivec2(0);

        static int idCounter;
    };

    //===== DiplomacyMatrix =====

    //Describes relationships between individual factions.
    class DiplomacyMatrix {
    public:
        DiplomacyMatrix() = default;

        DiplomacyMatrix(int factionCount);
        DiplomacyMatrix(int factionCount, const std::vector<glm::ivec3>& relations);
        ~DiplomacyMatrix();

        //copy disabled
        DiplomacyMatrix(const DiplomacyMatrix&) = delete;
        DiplomacyMatrix& operator=(const DiplomacyMatrix&) const = delete;

        //move enabled
        DiplomacyMatrix(DiplomacyMatrix&&) noexcept;
        DiplomacyMatrix& operator=(DiplomacyMatrix&&) noexcept;

        bool AreHostile(int f1, int f2) const;

        //Relations in the form of bitmap.
        //Index in the list identifies faction A, individual bit positions in the value then identify faction B.
        //Bit=1 means hostile relationship.
        const std::vector<int>& Bitmap() const { return relations_bitmap; }

        std::vector<glm::ivec3> Export() const;
        
        void DBG_GUI();
        void EditableGUI(int factionCount);
    private:
        int& operator()(int y, int x);
        const int& operator()(int y, int x) const;

        void Move(DiplomacyMatrix&&) noexcept;
        void Release() noexcept;
    private:
        int* relation = nullptr;
        int factionCount = 0;

        std::vector<int> relations_bitmap = std::vector<int>(32);
    };

    //===== Factions =====

    class PlayerFactionController;
    using PlayerFactionControllerRef = std::shared_ptr<PlayerFactionController>;

    class Factions {
        friend class FactionsEditor;
    public:
        Factions() = default;
        Factions(FactionsFile&& data, const glm::ivec2& mapSize, Map& map);

        const DiplomacyMatrix& Diplomacy() const { return diplomacy; }

        PlayerFactionControllerRef Player();
        FactionControllerRef Nature();

        FactionControllerRef operator[](int i);
        const FactionControllerRef operator[](int i) const;

        std::vector<FactionControllerRef>::iterator begin() { return factions.begin(); }
        std::vector<FactionControllerRef>::iterator end() { return factions.end(); }
        std::vector<FactionControllerRef>::const_iterator begin() const { return factions.cbegin(); }
        std::vector<FactionControllerRef>::const_iterator end() const { return factions.cend(); }

        size_t size() const { return factions.size(); }

        void Update(Level& level);

        bool IsInitialized() const { return initialized; }

        //Returns false if provided faction isn't tracked from within this object.
        bool IsValidFaction(const FactionControllerRef& faction) const;

        FactionsFile Export();

        void DBG_GUI();
    private:
        std::vector<FactionControllerRef> factions;
        DiplomacyMatrix diplomacy;
        bool initialized = false;

        PlayerFactionControllerRef player = nullptr;
        FactionControllerRef nature = nullptr;
    };
    
}//namespace eng
