/*  Will find players with a level difference greater than 7 when compared to the highest level player.
EG highest lvl = 60, any players that are 7 or more levels lower than 60.
  When players are found, use a modifier based on the level difference and use that to change experience gain rates.

 So if a player lvl 60 is grouped with a lvl 30, a modifier using the 30 lvl difference will apply to
the lvl 30 player, used as a method of level boosting.

  Try and make it an opt-in thing. EG ".grplvl enable" will enable the experience modifier
will require enabling the command for non-gm players.
*/

// Using custom_dwrath_character_stats needed to adjust how InstanceBalance handled clearing buffs
// Going forward, any toggles I may create will be stored in the dwrath_character_stats table
// Need to add a way to delete rows when a character is deleted

// Needs logic for creating and applying the exerience boost
// Needs logic for adjusting & removing the boost when a player joins, leaves or the group disbands.
// Needs logic for if the player disables GroupLevel while boosted.

#include "Config.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Group.h"
#include "Chat.h"
#include "Opcodes.h"
#include "ScriptPCH.h"
#include "DatabaseEnv.h"
#include "WorldSession.h"

uint8 maxDiff = 7;
std::ostringstream ss;

// LOGIN CLASS
class grouplevel_login : public PlayerScript
{
public:
    grouplevel_login() : PlayerScript("grouplevel_login"){}
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
        void OnAddMemberGL(Group* group, ObjectGuid guid, Player* player) override
        {
            GetGroupLevels(group, player);
            guid = player->GetGUID();
        }

        void GetGroupLevels(Group * group, Player * player)
        {
            QueryResult result = CharacterDatabase.PQuery("SELECT `GroupLevelTog` FROM `custom_dwrath_character_stats` WHERE GUID = %u", player->GetGUID());
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
                                player->GetGroup();
                                modifyExperience(player, memberLvl);
                            }
                        }
                    }
                }
            }
        }

        void modifyExperience(Player* player, int memberLvl)
        {
            ss << "|cffFF0000[GroupLevel] |cffFF8000" << player->GetName() << ". Modify Experience RAN. memberLvl = %i";
            ChatHandler(player->GetSession()).PSendSysMessage(ss.str().c_str(), memberLvl);
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

    static bool HandleGLEnableCommand(ChatHandler* handler)
    {
        Player* me = handler->GetSession()->GetPlayer();
        CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET GroupLevelTog = 1 WHERE GUID = %u", me->GetGUID());
        QueryResult result = CharacterDatabase.PQuery("SELECT `GroupLevelTog` FROM `custom_dwrath_character_stats` WHERE GUID = %u", me->GetGUID());
        handler->PSendSysMessage("Enabling GroupLevel boosting.");
        return true;
    }

    static bool HandleGLDisableCommand(ChatHandler* handler)
    {
        Player* me = handler->GetSession()->GetPlayer();
        CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET GroupLevelTog = 0 WHERE GUID = %u", me->GetGUID());
        QueryResult result = CharacterDatabase.PQuery("SELECT `GroupLevelTog` FROM `custom_dwrath_character_stats` WHERE GUID = %u", me->GetGUID());
        handler->PSendSysMessage("Disabling GroupLevel boosting.");
        return true;
    }

    static bool HandleGLMultiplierCommand(ChatHandler* handler)
    {
        Player* me = handler->GetSession()->GetPlayer();
        QueryResult result = CharacterDatabase.PQuery("SELECT `GroupLevelTog` FROM `custom_dwrath_character_stats` WHERE GUID = %u", me->GetGUID());
        bool gltoggle = (*result)[0].GetUInt32();
        handler->PSendSysMessage("GroupLevel Stats- Toggled = %b, Level Difference = PLHD, Boost Rate = PLHD", gltoggle);
        return true;
    }
};

void Add_GroupLevel() {
    new grouplevel_login();
    new grouplevel_handler();
    new grouplevel_commands();
}
