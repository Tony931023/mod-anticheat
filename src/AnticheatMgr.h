/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SC_ACMGR_H
#define SC_ACMGR_H

#include "Common.h"
#include "SharedDefines.h"
#include "ScriptMgr.h"
#include "AnticheatData.h"
#include "Chat.h"
#include "ObjectGuid.h"

class Player;
class AnticheatData;

enum ReportTypes
{
    SPEED_HACK_REPORT = 0,
    FLY_HACK_REPORT = 1,
    WALK_WATER_HACK_REPORT = 2,
    JUMP_HACK_REPORT = 3,
    TELEPORT_PLANE_HACK_REPORT = 4,
    CLIMB_HACK_REPORT = 5,
    TELEPORT_HACK_REPORT = 6,
    IGNORE_CONTROL_REPORT = 7,
    ZAXIS_HACK_REPORT = 8
   // MAX_REPORT_TYPES
};

enum DetectionTypes
{
    SPEED_HACK_DETECTION            = 1,
    FLY_HACK_DETECTION              = 2,
    WALK_WATER_HACK_DETECTION       = 4,
    JUMP_HACK_DETECTION             = 8,
    TELEPORT_PLANE_HACK_DETECTION   = 16,
    CLIMB_HACK_DETECTION            = 32,
    TELEPORT_HACK_DETECTION         = 64,
    IGNORE_CONTROL_DETECTION        = 128,
    ZAXIS_HACK_DETECTION            = 256
};

// GUID is the key.
typedef std::map<ObjectGuid, AnticheatData> AnticheatPlayersDataMap;

class AnticheatMgr
{
    AnticheatMgr();
    ~AnticheatMgr();

    public:
    static AnticheatMgr* instance()
        {
           static AnticheatMgr* instance = new AnticheatMgr();
           return instance;
        }

        void StartHackDetection(Player* player, MovementInfo movementInfo, uint32 opcode);
        void SavePlayerData(Player* player);

        void HandlePlayerLogin(Player* player);
        void HandlePlayerLogout(Player* player);

        uint32 GetTotalReports(ObjectGuid guid);
        float GetAverage(ObjectGuid guid);
        uint32 GetTypeReports(ObjectGuid guid, uint8 type);

        void AnticheatGlobalCommand(ChatHandler* handler);
        void AnticheatDeleteCommand(ObjectGuid guid);

        void ResetDailyReportStates();
    private:
        void SpeedHackDetection(Player* player, MovementInfo movementInfo);
        void FlyHackDetection(Player* player, MovementInfo movementInfo);
        void WalkOnWaterHackDetection(Player* player, MovementInfo movementInfo);
        void JumpHackDetection(Player* player, MovementInfo movementInfo,uint32 opcode);
        void TeleportPlaneHackDetection(Player* player, MovementInfo);
        void ClimbHackDetection(Player* player,MovementInfo movementInfo,uint32 opcode);
        void TeleportHackDetection(Player* player, MovementInfo movementInfo);
        void IgnoreControlHackDetection(Player* player, MovementInfo movementInfo);
        void ZAxisHackDetection(Player* player, MovementInfo movementInfo);
        void BuildReport(Player* player,uint16 reportType);
		

        bool MustCheckTempReports(uint8 type);

        AnticheatPlayersDataMap m_Players;                        ///< Player data
};

#define sAnticheatMgr AnticheatMgr::instance()

#endif
