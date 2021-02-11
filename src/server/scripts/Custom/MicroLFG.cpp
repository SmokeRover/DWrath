/*
MicroLFG's primary goal is to allow solo, semi-pop and full-pop groups to recieve instant LFG queues

ADD TOGGLES TO ENABLE/DISABLE IN CONFIG for entire server
ADD INGAME CHAT TOGGLES TO OPT INTO MicroLFG, might not be possible
ADD A VERSION IDENTIFIER FOR ANNOUNCE

If I can figure something out it will also allow for the LFG portion to continue working and find players to join the group

Based on work from
Traesh https://github.com/Traesh
Conan513 https://github.com/conan513
Made into a module by Micrah https://github.com/milestorme/
LEO33 http://leo33.info
qyh214 https://github.com/qyh214

*/

#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "World.h"
#include "LFGMgr.h"
#include "Chat.h"
#include "Opcodes.h"


class micro_lfg_announce : public PlayerScript
{
public:
	micro_lfg_announce() : PlayerScript("micro_lfg_announce") {}

	void OnLogin(Player* player, bool /*firstlogin*/) override
	{
		//	Annonce from config
        if (sConfigMgr->GetBoolDefault("MicroLFG.Announce", true))
        {
            ChatHandler(player->GetSession()).SendSysMessage("This server is running MicroLFG.");
        }
	}
};

class micro_lfg : public PlayerScript
{
public:
	micro_lfg() : PlayerScript("micro_lfg"){}
	void OnLogin(Player* /*player*/, bool /*firstlogin*/) override
	{
		if (sConfigMgr->GetIntDefault("MicroLFG.Enable", true))
		{
			if (!sLFGMgr->IsSoloLFG())
			{
				sLFGMgr->ToggleSoloLFG();
			}
		}
	}
};


void AddMicroLFGScripts()
{
	new micro_lfg_announce();
	new micro_lfg();
}