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
float StatMultiplier = 20.0;
float SpellStatMultiplier = 2.0;
int D5MAN = 5;
int D10MAN = 10;
int D25MAN = 25;
int D40MAN = 40;
int D649H10 = 10;
int D649H25 = 25;

//@todo Check all instances recieve buffs.

// Adjust modifiers for good balance, currently way too easy. Magic nerds insta-kill mobs in an instance of their own level when solo

// Add alternate ApplyBuff for different raid tiers, or add math solution that takes tiers into consideration.

// Add levelDifference to decline buff to overleveled players.

// Player stats may set to 0 if they log out in a dungeon and are removed outside the instance.
// Seems like the game might consider the outside map as an instance if it tries loading the instance
// map but opts to remove the player afterwards.

// Never mind the druid crap, im dumb

class SolocraftConfig : public WorldScript
{
public:
    SolocraftConfig() : WorldScript("SolocraftConfig") {
        void SetInitialWorldSettings();
        {
            SoloCraftEnable = sConfigMgr->GetBoolDefault("Solocraft.Enable", 1);
            SoloCraftAnnounceModule = sConfigMgr->GetBoolDefault("Solocraft.Announce", 1);
            SoloCraftDebuffEnable = sConfigMgr->GetBoolDefault("Solocraft.Debuff.Enable", 1);
            StatMultiplier = sConfigMgr->GetFloatDefault("InstanceBalance.StatMultiplier", 20.0f);
            SpellStatMultiplier = sConfigMgr->GetFloatDefault("InstanceBalance.SpellStatMultiplier", 2.0f);
            D5MAN = sConfigMgr->GetIntDefault("InstanceBalance.5MAN.Dungeon", 5);
            D10MAN = sConfigMgr->GetIntDefault("InstanceBalance.10MAN.Dungeon", 10);
            D25MAN = sConfigMgr->GetIntDefault("InstanceBalance.25MAN.Dungeon", 25);
            D40MAN = sConfigMgr->GetIntDefault("InstanceBalance.40MAN.Dungeon", 40);
            D649H10 = sConfigMgr->GetIntDefault("InstanceBalance.D649H10.Dungeon", 10);
            D649H25 = sConfigMgr->GetIntDefault("InstanceBalance.D649H25.Dungeon", 25);
            //@todo Include multipliers, level difference
        }
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
                std::ostringstream ss;
                ss << "|cff4CFF00InstanceBalance |r is running. Stat Mod: %f Spell Mod: %f";
                //ChatHandler(Player->GetSession()).SendSysMessage("|cff4CFF00InstanceBalance |r is running. Stat Mod:%f Spell Mod:%f");
                ChatHandler(Player->GetSession()).PSendSysMessage(ss.str().c_str(), StatMultiplier, SpellStatMultiplier);
            }
        }
    }
};

class solocraft_player_instance_handler : public PlayerScript
{
public:
    solocraft_player_instance_handler() : PlayerScript("solocraft_player_instance_handler") {}

    void OnMapChanged(Player* player) override
    {
        if (sConfigMgr->GetBoolDefault("Solocraft.Enable", true))
        {
            Map* map = player->GetMap();
            int difficulty = CalculateDifficulty(map, player);
            //int dunLevel = CalculateDungeonLevel(map, player);
            int numInGroup = GetNumInGroup(player);
            ApplyBuffs(player, map, difficulty/*, dunLevel*/, numInGroup);
        }
    }

private:
    std::map<ObjectGuid, int> _unitDifficulty;
    int CalculateDifficulty(Map* map, Player* /*player*/)
    {
        int difficulty = 1;
        if (map)
        {
            if (map->Is25ManRaid())
            {
                 if (map->IsHeroic() && map->GetId() == 649)
                 {
                    return D649H25;
                 }
                 else
                 {
                    return D5MAN;
                 }
            }
        else if (map->IsHeroic())
        {
            if (map->IsHeroic() && map->GetId() == 649)
            {
                return D649H10;
            }
            else
            {
                return D10MAN;
            }
        }
        else if (map->IsRaid())
        {
            return D40MAN;
        }
        else if (map->IsDungeon())
        {
            return D5MAN;
        }
            return 0;
        }
        return difficulty;
    }

    int GetNumInGroup(Player* player)
    {
        int numInGroup = 1;
        Group* group = player->GetGroup();
        if (group)
        {
            Group::MemberSlotList const& groupMembers = group->GetMemberSlots();
            numInGroup = groupMembers.size();
        }
        return numInGroup;
    }

    void ApplyBuffs(Player* player, Map* map, int difficulty,/*, dunLevel*/ int numInGroup) {
        int SpellPowerBonus = 0;
        if (difficulty != 0)
        {
            std::ostringstream ss;
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
                player->HandleStatFlatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, difficulty* StatMultiplier, true); //UNIT_MOD in Unit.h line 391
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
                    SpellPowerBonus = static_cast<int>((player->GetLevel() * SpellStatMultiplier) * difficulty); // math pulled out of some dudes ass
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
            CharacterDatabase.PExecute("REPLACE INTO custom_solocraft_character_stats (GUID, Difficulty, GroupSize, SpellPower, Stats) VALUES (%u, %i, %u, %i, %f)", player->GetGUID(), difficulty, numInGroup, SpellPowerBonus, StatMultiplier);
        }
        else
        {
            ClearBuffs(player, map);//Clears buffs
        }
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
                        if ((*result)[1].GetUInt32() > 0)
                        {
                            GroupDifficulty = GroupDifficulty + (*result)[1].GetUInt32();
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
            int difficulty = (*result)[1].GetUInt32();
            int SpellPowerBonus = (*result)[3].GetUInt32();
            float StatsMultPct = (*result)[4].GetUInt32();
            // Inform the player
            std::ostringstream ss;
            ss << "|cffFF0000[SoloCraft] |cffFF8000" << player->GetName() << " exited to %s - Reverting Difficulty Offset: %i. Spellpower Bonus Removed: %i";
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
    }
};


void AddSC_solocraft() {
    new SolocraftConfig();
    new SolocraftAnnounce();
    new solocraft_player_instance_handler();
}
