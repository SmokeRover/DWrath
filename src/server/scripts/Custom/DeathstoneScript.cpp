#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "Chat.h"

std::ostringstream ssss;

/*
Like a hearthstone but for when you died and wanna get back quick

Need to make the script execute. Probably need to create a spell
that sucks
*/

class deathstone_script : public ItemScript
{
public:
    deathstone_script() : ItemScript("deathstone_script") { }

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& targets) override
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT `d_map` `d_x` `d_y` `d_z` FROM `custom_dwrath_character_stats` WHERE GUID = %u", player->GetGUID());
        int d_map = (*result)[0].GetUInt32();
        float d_x = (*result)[1].GetFloat();
        float d_y = (*result)[2].GetFloat();
        float d_z = (*result)[3].GetFloat();

        if (d_map != -1) {
            player->TeleportTo(d_map, d_x, d_y, d_z, 0);
            CharacterDatabase.PExecute("UPDATE custom_dwrath_character_stats SET d_map = -1 WHERE GUID = %u", player->GetGUID());
            ssss << "|cffFF0000[DEATHSTONE] |cffFF8000" << player->GetName() << ". d_map is -1"; ChatHandler(player->GetSession()).PSendSysMessage(ssss.str().c_str()); ssss.str("");

            return true;
        }
        else
        {
            ssss << "|cffFF0000[DEATHSTONE] |cffFF8000" << player->GetName() << ". d_map is something"; ChatHandler(player->GetSession()).PSendSysMessage(ssss.str().c_str()); ssss.str("");
            return false;
        }
    }
};

//"SELECT mapId, posX, posY, posZ, orientation FROM corpse WHERE guid = ?", CONNECTION_ASYNC;
void Add_DeathstoneScript() {
    new deathstone_script();
}
