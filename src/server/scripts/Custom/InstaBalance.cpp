/*
Some sort of Solocraft or VAS Autobalance implementation, maybe a mix.

Intent is to scale instances based on players in group.

Instances will scale solely on the number of players in the group and alter creatures in the dungeons stats based on some algorithm

For dungeons i will probably use 5 tiers based on group fullness.
5 is default dungeon stats.
2 is altered by dividing stats by 3.
1 is divided by 5.

Need to investigate how health, damage, armour, mana, rage, whatever are assigned to mobs in worldserver. Will probably hijack or override to alter them.


*/

//@todo
/*

#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "World.h"
#include "Chat.h"
#include "Unit.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "Map.h"
#include "Language.h"
#include <vector>
#include "Log.h"
#include "Group.h"

#include "DataMap.h"
#include "DBCStores.h"
*/
