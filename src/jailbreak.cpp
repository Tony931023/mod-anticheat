#include "Configuration/Config.h"
#include "Player.h"
#include "Creature.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "Chat.h"
#include "AnticheatMgr.h"

bool JailBreakEnabled;

class JailBreak : public PlayerScript
{
public:
    JailBreak() : PlayerScript("JailBreak") { }

    std::string playername;
    uint32 mapId;
    std::string maparea;

    void OnMapChanged(Player* player)
    {
		LOG_INFO("module", "AnticheatMgr:: OnMapChanged retorna 24 ");
          if (!sConfigMgr->GetOption<bool>("Anticheat.JailbreakCheck", true))
		  {
            if (player->GetSession()->GetSecurity() >= SEC_GAMEMASTER)
				return;
			
            QueryResult result = CharacterDatabase.Query("SELECT `jail` FROM `antihack_jail` WHERE `guid` = '{}'", player->GetGUID().GetCounter());
			
            playername = player->GetName();
            mapId =  player->GetMap()->GetId();
			maparea = player->GetZoneId();
            
            if (!result){
				LOG_INFO("module", "AnticheatMgr:: Osin resultado 37 ");
			return;}
			
			if  (player->GetMapId() == 1) {
				
				LOG_INFO("module", "AnticheatMgr:: getmap==1 retorna 39 ");
				return;
				
			}
			
			else {
				if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true)) {
					LOG_INFO("module", "AnticheatMgr:: Prison break attempt detected by the player {} , on the map {} ", playername, mapId);
				}
				
				// display warning at the center of the screen, hacky way?
				std::string str = "";
				str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Auto Jailed for attempted prison break!";
				WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
				data << str;
				sWorld->SendGlobalGMMessage(&data);

				WorldLocation loc;
				loc = WorldLocation(1, 16226.5f, 16403.6f, -64.5f, 3.2f); // GM Jail Location
				player->TeleportTo(loc);
				player->SetHomebind(loc, 876); // GM Jail Homebind location
				player->CastSpell(player, 38505); // Shackle him in place to ensure no exploit happens for jail break attempt
				
			}
            
        }

    }

    
};

void AddJailBreakScripts()
{
    new JailBreak();
}
