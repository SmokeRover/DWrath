/*  Will find players with a level difference greater than 7 when compared to the highest level player.
EG highest lvl = 60, any players that are 7 or more levels lower than 60.
  When players are found, use a modifier based on the level difference and use that to change experience gain rates.

 So if a player lvl 60 is grouped with a lvl 30, a modifier using the 30 lvl difference will apply to
the lvl 30 player, used as a method of level boosting.
*/

// Using custom_dwrath_character_stats needed to adjust how InstanceBalance handled clearing buffs
// Going forward, any toggles I may create will be stored in the dwrath_character_stats table
// Need to add a way to delete rows when a character is deleted

// Needs logic for when the player goes offline while in a group, reactvating boost on log in
// Need logic for removing boost when a member goes offline
// Needs logic for the player leveling up in the group.

// Uninviting an offline player crashes the server
// Probably need to specify if the player is in world before running the GetGroupLevel?

// When lvl 1 grouped with lvl 80-  Exp gain for lvl 1 is 1xp
// When lvl is opted-               Exp gain for lvl 1 is 3xp
//                                  WHY IT DO???
// Seems like a group XP modifier elsewhere in trinity is sticking its finger in my pie

#include "Config.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Group.h"
#include "Chat.h"
#include "Opcodes.h"
#include "ScriptPCH.h"
#include "DatabaseEnv.h"
#include "WorldSession.h"
#include <Windows.h>
// ADD maxDiff to config
uint8 maxDiff = 7;
std::ostringstream ss;

// LOGIN CLASS
class grouplevel_login : public PlayerScript
{
public:
    grouplevel_login() : PlayerScript("grouplevel_login") {}
    // ADD a check for config setting
    void OnLogin(Player* player, bool) override
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT 'GUID' FROM `custom_dwrath_character_stats` WHERE GUID = %u", player->GetGUID());
        if (!result)
        {
            CharacterDatabase.PExecute("REPLACE INTO custom_dwrath_character_stats (GUID) VALUES (%u)", player->GetGUID());
        }
        std::ostringstream ss;
        ss << "|cff4CFF00GroupLevel |r is running.";
        ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str());
    }
};

// CORE CLASS
class grouplevel_handler : public GroupScript
{
public:
    grouplevel_handler() : GroupScript("grouplevel_handler") {}
    //  Custom overrides in ScriptMgr.cpp, ScriptMgr.h . Saves me breaking the originals
    void OnAddMemberGL(Group* group, ObjectGuid guid, Player* player) override
    {
        GetGroupLevels(group, player);
        guid = player->GetGUID();
    }

    void OnRemoveMemberGL(Group* group, ObjectGuid guid, RemoveMethod /*method*/, ObjectGuid /*kicker*/, char const* /*reason*/, Player* player) override
    {
        GetGroupLevels(group, player);
        guid = player->GetGUID();
        //method = GROUP_REMOVEMETHOD_DEFAULT;
        //reason = NULL;
        ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". REMOVE RAN";
        ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str());
        ss.str("");
    }

    void OnDisbandGL(Group* group, Player* player) override
    {
        GetGroupLevels(group, player);
        ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". DISBAND RAN";
        ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str());
        ss.str("");
    }

    //Checks if player is toggled, in a group, iterates the group, for loops each member
    //  higher level than player+diff applies modifier for each loop

    //MAKE THE LOOP SEEK OUT THE HIGHEST LEVEL PLAYER AND USE THEM AS THE BOOSTER
    void GetGroupLevels(Group* group, Player* player)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT `GroupLevelTog` FROM `custom_dwrath_character_stats` WHERE GUID = %u", player->GetGUID());
        CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET BoostedXP = 1 WHERE GUID = %u", player->GetGUID());
        player->SetBoostedXP(1);
        bool gltoggle = (*result)[0].GetUInt32();
        if ((gltoggle == true))
        {
            ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". ENABLED";
            ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str());
            ss.str("");
            group = player->GetGroup();
            if (group) {
                ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". IF IN GROUP";
                ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str());
                ss.str("");
                for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())     // Gets first player in group and then iterates through the rest.
                {
                    ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". FOR LOOP";
                    ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str());
                    ss.str("");
                    if (Player* member = itr->GetSource())                     // Unsure what exactly GetSource does, probably gets the current member deets
                    {
                        ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". DO DIFF";
                        ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str());
                        ss.str("");
                        int memberLvl = member->GetLevel();                    //  Gets the level of the member
                        if (memberLvl >= (player->GetLevel() + maxDiff))         // Compares members level with the player + difference
                        {
                            modifyExperience(player, memberLvl);
                        }else
                        {
                            CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET BoostedXP = 1 WHERE GUID = %u", player->GetGUID());
                            player->SetBoostedXP(1);
                        }
                    }
                }
            }
            else
            {
                CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET BoostedXP = 1 WHERE GUID = %u", player->GetGUID());
                player->SetBoostedXP(1);
            }
        }
    }

    void modifyExperience(Player* player, int memberLvl)
    {
        ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". Modify Experience RAN. memberLvl = %i";
        ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str(), memberLvl);
        ss.str("");

        int pLvl = player->GetLevel();
        int boostedXP = floor((memberLvl - (pLvl-maxDiff)) / 10); // EG. (memberlvl = 80, pLvl = 15, maxDiff = 7) (80 - (15-7))/10) = 7.2 rounded down = 7
        if (boostedXP < 1)
        {
            boostedXP = 1;
        }
        ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". Boosted XP rate would be. boostedXP = %i";
        ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str(), boostedXP);
        ss.str("");

        applyExperience(player, boostedXP);
    }

    // Saves boostedXP to the database, applies the boostedXP to the player.
    void applyExperience(Player* player, int boostedXP)
    {
        CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET BoostedXP = %i WHERE GUID = %u", boostedXP, player->GetGUID());
        player->SetBoostedXP(boostedXP);
        ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". BOOST APPLIED. boostedXP = %i";
        ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str(), boostedXP);
        ss.str("");
    }
};


// CHAT COMMANDS
using namespace Trinity::ChatCommands;
class grouplevel_commands : public CommandScript
{
public:
    grouplevel_commands() : CommandScript("grouplevel_commands"){}
    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable GLCommandTable =
        {
            {"enable", HandleGLEnableCommand, rbac::RBAC_ROLE_PLAYER, Console::Yes},
            {"disable", HandleGLDisableCommand, rbac::RBAC_ROLE_PLAYER, Console::Yes},
            {"multipliers", HandleGLMultiplierCommand, rbac::RBAC_ROLE_PLAYER, Console::Yes},
        };
        static ChatCommandTable commandTable =
        {
            {"grplvl", GLCommandTable},
        };
        return commandTable;
    }

    // Will enable and disable while in a group
    static bool HandleGLEnableCommand(ChatHandler* handler)
    {
        Player* me = handler->GetSession()->GetPlayer();
        Group* group = me->GetGroup();
        grouplevel_commands* hGroupCheck{};
        CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET GroupLevelTog = 1 WHERE GUID = %u", me->GetGUID());
        QueryResult result = CharacterDatabase.PQuery("SELECT `BoostedXP` FROM `custom_dwrath_character_stats` WHERE GUID = %u", me->GetGUID());
        unsigned int boostedXP = (*result)[0].GetUInt32();
        handler->PSendSysMessage("Enabling GroupLevel boosting. Boosted = %i", boostedXP);
        Sleep(10); // Wait a few mSeconds for DB to update
        hGroupCheck->handleGroupCheck(me, group);
        return true;
    }

    void handleGroupCheck(Player* player, Group* group)
    {
        grouplevel_handler* glHandler{};
        glHandler->GetGroupLevels(group, player);
    }

    static bool HandleGLDisableCommand(ChatHandler* handler)
    {
        Player* me = handler->GetSession()->GetPlayer();
        CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET GroupLevelTog = 0, BoostedXP = 1 WHERE GUID = %u", me->GetGUID());
        QueryResult result = CharacterDatabase.PQuery("SELECT `GroupLevelTog` FROM `custom_dwrath_character_stats` WHERE GUID = %u", me->GetGUID());
        me->SetBoostedXP(1);
        handler->PSendSysMessage("Disabling GroupLevel boosting.");
        return true;
    }

    static bool HandleGLMultiplierCommand(ChatHandler* handler)
    {
        Player* me = handler->GetSession()->GetPlayer();
        QueryResult resultTog = CharacterDatabase.PQuery("SELECT `GroupLevelTog` FROM `custom_dwrath_character_stats` WHERE GUID = %u", me->GetGUID());
        QueryResult resultBoost = CharacterDatabase.PQuery("SELECT `BoostedXP` FROM `custom_dwrath_character_stats` WHERE GUID = %u", me->GetGUID());
        bool gltoggle = (*resultTog)[0].GetUInt32();
        unsigned int boostedXP = (*resultBoost)[0].GetUInt32();
        handler->PSendSysMessage("GroupLevel Stats- Toggled = %b, Level Difference = PLHD, boostedXP = %i", gltoggle, boostedXP);
        return true;
    }
};

void Add_GroupLevel() {
    new grouplevel_login();
    new grouplevel_handler();
    new grouplevel_commands();
}
