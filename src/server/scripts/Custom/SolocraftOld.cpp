#include <map>

#include "Config.h"
#include "DatabaseEnv.h"
#include "ScriptMgr.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Pet.h"
#include "Map.h"
#include "Group.h"
#include "InstanceScript.h"
#include "Chat.h"

bool SoloCraftEnable = 1;
bool SoloCraftAnnounceModule = 1;
bool SoloCraftDebuffEnable = 1;
float SolocraftStatMultiplier = 100.0;
int D649H10 = 10;
int D649H25 = 25;

class SolocraftConfig : public WorldScript
{
public:
    SolocraftConfig() : WorldScript("SolocraftConfig"){}
    void SetInitialWorldSettings()
    {
        SoloCraftEnable = sConfigMgr->GetBoolDefault("Solocraft.Enable", 1);
        SoloCraftAnnounceModule = sConfigMgr->GetBoolDefault("Solocraft.Announce", 1);
        SoloCraftDebuffEnable = sConfigMgr->GetBoolDefault("SoloCraft.Debuff.Enable", 1);
        //@todo Include multipliers, level difference
    }
};

class SolocraftAnnounce : public PlayerScript
{
public:
    SolocraftAnnounce() : PlayerScript("SolocraftAnnounce"){}
    void OnLogin(Player* Player, bool /*firstLogin*/) override
    {
        if (SoloCraftEnable)
        {
            if (SoloCraftAnnounceModule)
            {
                ChatHandler(Player->GetSession()).SendSysMessage("OLD |cff4CFF00SoloCraft Balance |r is running.");
            }
        }
    }
};

class solocraft_player_instance_handler : public PlayerScript {
    public:
        solocraft_player_instance_handler() : PlayerScript("solocraft_player_instance_handler") {}

        void OnMapChanged(Player* player) override {
            if (sConfigMgr->GetBoolDefault("Solocraft.Enable", true))
            {
                // NEED TO FIX THIS SO THE SCRIPT ONLY EXECUTES IN DUNGEONS/RAIDS
                // CURRENTLY EXECUTES ON ALL MAPS AND KEEPS STACKING STAT BUFFS
                Map* map = player->GetMap();
                // if (map ==)
                int difficulty = CalculateDifficulty(map, player);
                //int dunLevel = CalculateDungeonLevel(map, player);
                int numInGroup = GetNumInGroup(player);
                ApplyBuffs(player, map, difficulty/*, dunLevel*/, numInGroup);
            }
        }

    private:
        std::map<ObjectGuid, int> _unitDifficulty;

        int CalculateDifficulty(Map* map, Player* /*player*/) {
            int difficulty = 1;
            if (map) {
                if (map->Is25ManRaid()) {
                    if (map->IsHeroic() && map->GetId() == 649)
                    {
                        difficulty = D649H25;
                    }
                    else
                    {
                        difficulty = 25;
                    }
                }
                else if (map->IsHeroic()) {
                    if (map->IsHeroic() && map->GetId() == 649)
                    {
                        difficulty = D649H10;
                    }
                    else
                    {
                        difficulty = 10;
                    }
                }
                else if (map->IsRaid()) {
                    difficulty = 40;
                }
                else if (map->IsDungeon()) {
                    difficulty = 5;
                }
            }
            return difficulty;
        }

        /*
        //DungeonLevel from new solocraft, reckon is probably where shit broke.
        int CalculateDungeonLevel(Map* map, Player* ///*player)
        {
            if (dungeons.find(map->GetId()) == dungeons.end())
            {
                return SolocraftDungeonLevel;
            }
            else
            {
                return dungeons[map->GetId()];
            }
        }*/

        int GetNumInGroup(Player* player) {
            int numInGroup = 1;
            Group* group = player->GetGroup();
            if (group) {
                Group::MemberSlotList const& groupMembers = group->GetMemberSlots();
                numInGroup = groupMembers.size();
            }
            return numInGroup;
        }

        void ApplyBuffs(Player* player, Map* map, int difficulty,/*, dunLevel*/ int numInGroup) {
            //ClearBuffs(player, map);
            int SpellPowerBonus = 0;
            if (difficulty != 0)
            {
                std::ostringstream ss;
                //@todo add dunLevel statements yadayada
                float GroupDifficulty = GetGroupDifficulty(player);
                //Check for buff or debuff player on dungeon enter
                if (GroupDifficulty >= difficulty && SoloCraftDebuffEnable == 1)
                {
                    //GroupDifficulty exceeds dungeon setting -- Debuffs player
                    difficulty = (-abs(difficulty)) + (difficulty / numInGroup);
                    difficulty = roundf(difficulty * 100) / 100;
                }
                else
                {
                    //GroupDifficulty does not exceed dungeon setting -- Buffs player
                    difficulty = difficulty / numInGroup;
                    difficulty = roundf(difficulty * 100) / 100;
                }
                //Modify player stats
                for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)   //STATS in SharedDefines.h
                {
                    //Buff player
                    player->HandleStatFlatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, difficulty* SolocraftStatMultiplier, true); //UNIT_MOD in Unit.h line 391
                }
                //Set player health
                player->SetFullHealth();//In Unit.h line 1524
                //Modify spellcaster stats
                if (player->GetPowerType() == POWER_MANA)
                {
                    //Buff mana
                    player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
                    //Buff spellpower
                    if (difficulty > 0)//Wont buff debuffed players
                    {
                        SpellPowerBonus = static_cast<int>((player->GetLevel() * SolocraftStatMultiplier) * difficulty); // math pulled out of some dudes ass
                        player->ApplySpellPowerBonus(SpellPowerBonus, true);
                    }
                }
                //Announcements
                if (difficulty > 0)
                {
                    // Announce to player - Buff
                    ss << "|cffFF0000[SoloCraft] |cffFF8000" << player->GetName() << " entered %s  - Difficulty Offset: %i. Spellpower Bonus: %i";
                    ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str(), map->GetMapName(), difficulty, SpellPowerBonus);
                }
                else
                {
                    // Announce to player - Debuff
                    ss << "|cffFF0000[SoloCraft] |cffFF8000" << player->GetName() << " entered %s  - |cffFF0000BE ADVISED - You have been debuffed by offset: %i. |cffFF8000 A group member already inside has the dungeon's full buff offset.  No Spellpower buff will be applied to spell casters.  ALL group members must exit the dungeon and re-enter to receive a balanced offset.";
                    ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str(), map->GetMapName(), difficulty);
                }
                // Save Player Dungeon Offsets to Database
                CharacterDatabase.PExecute("REPLACE INTO custom_solocraft_character_stats (GUID, Difficulty, GroupSize, SpellPower, Stats) VALUES (%u, %i, %u, %i, %f)", player->GetGUID(), difficulty, numInGroup, SpellPowerBonus, SolocraftStatMultiplier);
            }
            else
            {
                ClearBuffs(player, map);//Clears buffs
            }

            //OLD SOLOCRAFT DIFFICULTY SCRIPT
            /*if (difficulty > 1) {
                //InstanceMap *instanceMap = map->ToInstanceMap();
                //InstanceScript *instanceScript = instanceMap->GetInstanceScript();

                //BARNARD EDIT Following if statement for formchanges added
                uint32 pFormCheck = FORM_NONE;
                if (pFormCheck != pFormCheck) {
                    ChatHandler(player->GetSession()).PSendSysMessage("DEBUG: Form changed on %s (difficulty = %d, numInGroup = %d)",
                        map->GetMapName(), difficulty, numInGroup);
                }

                ChatHandler(player->GetSession()).PSendSysMessage("Entered %s (difficulty = %d, numInGroup = %d)",
                    map->GetMapName(), difficulty, numInGroup);

                _unitDifficulty[player->GetGUID()] = difficulty;
                for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i) {                                              //Default 100
                    player->ApplyStatPctModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(difficulty * 175));
                }
                player->SetFullHealth();
                if (player->GetPowerType() == POWER_MANA) {
                    player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
                }
            }*/
        }

        float GetGroupDifficulty(Player* player)
        {
            float GroupDifficulty = 0.0;
            Group* group = player->GetGroup();
            if (group)
            {
                Group::MemberSlotList const& groupMembers = group->GetMemberSlots();
                for (Group::member_citerator itr = groupMembers.begin(); itr != groupMembers.end(); ++itr)
                {
                    if (itr->guid != player->GetGUID())
                    {
                        QueryResult result = CharacterDatabase.PQuery("SELECT `GUID`, `Difficulty`, `GroupSize` FROM `custom_solocraft_character_stats` WHERE GUID = %u", itr->guid);
                        if (result)
                        {
                            if ((*result)[1].GetFloat() > 0)
                            {
                                GroupDifficulty = GroupDifficulty + (*result)[1].GetFloat();
                            }
                        }
                    }
                }
            }
            return GroupDifficulty;
        }

        void ClearBuffs(Player* player, Map* map)
        {
            //Database query to get offset from the last instance player exited
            QueryResult result = CharacterDatabase.PQuery("SELECT `GUID`, `Difficulty`, `GroupSize`, `SpellPower`, `Stats` FROM `custom_solocraft_character_stats` WHERE GUID = %u", player->GetGUID());
            if (result)
            {
                float difficulty = (*result)[1].GetFloat();
                int SpellPowerBonus = (*result)[3].GetUInt32();
                float StatsMultPct = (*result)[4].GetFloat();
                //sLog->outError("Map difficulty: %f", difficulty);
                // Inform the player
                std::ostringstream ss;
                ss << "|cffFF0000[SoloCraft] |cffFF8000" << player->GetName() << " exited to %s - Reverting Difficulty Offset: %0.2f. Spellpower Bonus Removed: %i";
                ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str(), map->GetMapName(), difficulty, SpellPowerBonus);
                // Clear the buffs
                for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
                {
                    player->HandleStatFlatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, difficulty * StatsMultPct, false);
                }
                if (player->GetPowerType() == POWER_MANA && difficulty > 0)
                {
                    // remove spellpower bonus
                    player->ApplySpellPowerBonus(SpellPowerBonus, false);
                }
                //Remove database entry as the player is no longer in an instance
                CharacterDatabase.PExecute("DELETE FROM custom_solocraft_character_stats WHERE GUID = %u", player->GetGUID());
            }

            //OLD SOLOCRAFT CLEAR BUFFS
            /*std::map<ObjectGuid, int>::iterator unitDifficultyIterator = _unitDifficulty.find(player->GetGUID());
            //Was if now while
            if (unitDifficultyIterator != _unitDifficulty.end()) {
                int difficulty = unitDifficultyIterator->second;
                _unitDifficulty.erase(unitDifficultyIterator);

                ChatHandler(player->GetSession()).PSendSysMessage("Left to %s (removing difficulty = %d)",
                    map->GetMapName(), difficulty);

                for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i) {
                    player->ApplyStatPctModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, 100.f / (1.f + float(difficulty * 175) / 100.f) - 100.f);
                }
            }*/
        }
};


void AddSC_solocraft() {
    new SolocraftConfig();
    new SolocraftAnnounce();
    new solocraft_player_instance_handler();
}
