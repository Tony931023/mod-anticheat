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

#include "AnticheatMgr.h"
#include "Log.h"
#include "MapMgr.h"
#include "Player.h"
#include "Configuration/Config.h"

#define CLIMB_ANGLE 1.87f

#define LANG_ANTICHEAT_ALERT 30087
#define LANG_ANTICHEAT_TELEPORT 30088
#define LANG_ANTICHEAT_IGNORECONTROL 30089

AnticheatMgr::AnticheatMgr()
{
}

AnticheatMgr::~AnticheatMgr()
{
    m_Players.clear();
}

void AnticheatMgr::JumpHackDetection(Player* player, MovementInfo /* movementInfo */, uint32 opcode)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectJumpHack", true))
        return;

    ObjectGuid key = player->GetGUID();

    if (m_Players[key].GetLastOpcode() == MSG_MOVE_JUMP && opcode == MSG_MOVE_JUMP)
    {
        BuildReport(player, JUMP_HACK_REPORT);
        LOG_INFO("module", "AnticheatMgr:: Jump-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    }
}

void AnticheatMgr::WalkOnWaterHackDetection(Player* player, MovementInfo  movementInfo)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectWaterWalkHack", true))
        return;

    // ghost can water walk
    if (player->HasAuraType(SPELL_AURA_GHOST))
        return;

    // Prevents the False Positive for water walking when you ressurrect.
    // Aura 15007 (Resurrection sickness) is given while dead before returning back to life.
    if (player->HasAuraType(SPELL_AURA_GHOST) && player->HasAura(15007))
        return;

    ObjectGuid key = player->GetGUID();
    /* Thanks to @LilleCarl */
    if (m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && movementInfo.HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
    {
        if (player->HasAuraType(SPELL_AURA_WATER_WALK) || player->HasAuraType(SPELL_AURA_FEATHER_FALL) ||
            player->HasAuraType(SPELL_AURA_SAFE_FALL))
        {
            return;
        }

    }
    else if (!m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && !movementInfo.HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
    {
        return;
    }

    if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
    {
        LOG_INFO("module", "AnticheatMgr:: Walk on Water - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    }
    BuildReport(player, WALK_WATER_HACK_REPORT);
}

void AnticheatMgr::FlyHackDetection(Player* player, MovementInfo  movementInfo)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectFlyHack", true))
    {
        return;
    }

    if (player->HasAuraType(SPELL_AURA_FLY) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED))//overkill but wth
    {
        return;
    }

    /*Thanks to @LilleCarl for info to check extra flag*/
    bool stricterChecks = true;
    if (sConfigMgr->GetOption<bool>("Anticheat.StricterFlyHackCheck", false))
    {
        stricterChecks = !(movementInfo.HasMovementFlag(MOVEMENTFLAG_ASCENDING) && !player->IsInWater());
    }

    if (!movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY) && !movementInfo.HasMovementFlag(MOVEMENTFLAG_FLYING) && stricterChecks)
    {
        return;
    }

    if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
    {
        LOG_INFO("module", "AnticheatMgr:: Fly-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
    }

    BuildReport(player, FLY_HACK_REPORT);
}

void AnticheatMgr::TeleportPlaneHackDetection(Player* player, MovementInfo movementInfo)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectTelePlaneHack", true))
        return;

    ObjectGuid key = player->GetGUID();

    if (m_Players[key].GetLastMovementInfo().pos.GetPositionZ() != 0 ||
        movementInfo.pos.GetPositionZ() != 0)
        return;

    if (movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING))
        return;

    float x, y, z;
    player->GetPosition(x, y, z);
    float ground_Z = player->GetMap()->GetHeight(x, y, z);
    float z_diff = fabs(ground_Z - z);

    // we are not really walking there
    if (z_diff > 1.0f)
    {
        if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
        {
            LOG_INFO("module", "AnticheatMgr:: Teleport To Plane - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
        }

        BuildReport(player, TELEPORT_PLANE_HACK_REPORT);
    }
}

void AnticheatMgr::IgnoreControlHackDetection(Player* player, MovementInfo movementInfo)
{
    float x, y;
    player->GetPosition(x, y);
    ObjectGuid key = player->GetGUID();

    if (sConfigMgr->GetOption<bool>("Anticheat.IgnoreControlHack", true))
    {
        if (player->HasUnitState(UNIT_STATE_ROOT) && !player->GetVehicle())
        {
            bool unrestricted = movementInfo.pos.GetPositionX() != x || movementInfo.pos.GetPositionY() != y;
            if (unrestricted)
            {
                if (m_Players[key].GetTotalReports() > sConfigMgr->GetOption<uint32>("Anticheat.ReportsForIngameWarnings", 70))
                {
                    // display warning at the center of the screen, hacky way?
                    std::string str = "";
                    str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Possible Ignore Control Hack Detected!";
                    WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
                    data << str;
                    sWorld->SendGlobalGMMessage(&data);
                    // need better way to limit chat spam
                    if (m_Players[key].GetTotalReports() >= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Min", 70) && m_Players[key].GetTotalReports() <= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Max", 80))
                    {
                        sWorld->SendGMText(LANG_ANTICHEAT_IGNORECONTROL, player->GetName().c_str());
                    }
                }
                if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
                {
                    LOG_INFO("module", "AnticheatMgr:: Ignore Control - Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
                }

                BuildReport(player, IGNORE_CONTROL_REPORT);
            }
        }
    }
}

void AnticheatMgr::ZAxisHackDetection(Player* player, MovementInfo movementInfo)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectZaxisHack", true))
        return;

    // if we are a ghost we can walk on water may false flag z -axis
    if (player->HasAuraType(SPELL_AURA_GHOST))
        return;

    if (movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING))
        return;

    ObjectGuid key = player->GetGUID();

    float lastX = m_Players[key].GetLastMovementInfo().pos.GetPositionX();
    float newX = movementInfo.pos.GetPositionX();

    float lastY = m_Players[key].GetLastMovementInfo().pos.GetPositionY();
    float newY = movementInfo.pos.GetPositionY();

    float xDiff = fabs(lastX - newX);
    float yDiff = fabs(lastY - newY);

    float groundZ_vmap = player->GetMap()->GetHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), true, 50.0f);
    float groundZ_dyntree = player->GetMap()->GetDynamicMapTree().getHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 50.0f, player->GetPhaseMask());
    float groundZ = std::max<float>(groundZ_vmap, groundZ_dyntree);

    if (m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && movementInfo.HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
        return;

    switch (player->GetAreaId())
    {
        case 4281: //Acherus: The Ebon Hold
        case 4342: //Acherus: The Ebon Hold
            return;
        break;
            default:
        break;// Should never happen
    }

    if ((xDiff || yDiff) && m_Players[key].GetLastMovementInfo().pos.GetPositionZ() == movementInfo.pos.GetPositionZ()
        && player->GetPositionZ() >= groundZ + 5.0f)
    {
        if (m_Players[key].GetTotalReports() > sConfigMgr->GetOption<uint32>("Anticheat.ReportsForIngameWarnings", 70))
        {
            // display warning at the center of the screen, hacky way?
            std::string str = "";
            str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Possible Ignore Zaxis Hack Detected!";
            WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
            data << str;
            sWorld->SendGlobalGMMessage(&data);
            // need better way to limit chat spam
            if (m_Players[key].GetTotalReports() >= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Min", 70) && m_Players[key].GetTotalReports() <= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Max", 80))
            {
                sWorld->SendGMText(LANG_ANTICHEAT_ALERT, player->GetName().c_str());
            }
        }
        if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
        {
            LOG_INFO("module", "AnticheatMgr:: Ignore Zaxis Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
        }
 
        BuildReport(player, ZAXIS_HACK_REPORT);
    }
 
}

void AnticheatMgr::TeleportHackDetection(Player* player, MovementInfo movementInfo)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectTelePortHack", true))
        return;

    ObjectGuid key = player->GetGUID();

    if (m_Players[key].GetLastMovementInfo().pos.GetPositionX() == movementInfo.pos.GetPositionX())
        return;

    float lastX = m_Players[key].GetLastMovementInfo().pos.GetPositionX();
    float newX = movementInfo.pos.GetPositionX();

    float lastY = m_Players[key].GetLastMovementInfo().pos.GetPositionY();
    float newY = movementInfo.pos.GetPositionY();

    float xDiff = fabs(lastX - newX);
    float yDiff = fabs(lastY - newY);

    if ((xDiff >= 50.0f || yDiff >= 50.0f) && !player->CanTeleport())
    {
        if (m_Players[key].GetTotalReports() > sConfigMgr->GetOption<uint32>("Anticheat.ReportsForIngameWarnings", 70))
        {
            // display warning at the center of the screen, hacky way?
            std::string str = "";
            str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Possible Teleport Hack Detected!";
            WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
            data << str;
            sWorld->SendGlobalGMMessage(&data);
            // need better way to limit chat spam
            if (m_Players[key].GetTotalReports() >= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Min", 70) && m_Players[key].GetTotalReports() <= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Max", 80))
            {
                sWorld->SendGMText(LANG_ANTICHEAT_TELEPORT, player->GetName().c_str());
            }
        }
        if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
        {
            LOG_INFO("module", "AnticheatMgr:: Teleport-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
        }

        BuildReport(player, TELEPORT_HACK_REPORT);
    }
    else if (player->CanTeleport())
        player->SetCanTeleport(false);
}

void AnticheatMgr::StartHackDetection(Player* player, MovementInfo movementInfo, uint32 opcode)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0))
        return;

    if (player->IsGameMaster())
        return;

    ObjectGuid key = player->GetGUID();

    if (player->IsInFlight() || player->GetTransport() || player->GetVehicle())
    {
        m_Players[key].SetLastMovementInfo(movementInfo);
        m_Players[key].SetLastOpcode(opcode);
        return;
    }

    SpeedHackDetection(player, movementInfo);
    FlyHackDetection(player, movementInfo);
    WalkOnWaterHackDetection(player, movementInfo);
    JumpHackDetection(player, movementInfo, opcode);
    TeleportPlaneHackDetection(player, movementInfo);
    ClimbHackDetection(player, movementInfo, opcode);
    TeleportHackDetection(player, movementInfo);
    IgnoreControlHackDetection(player, movementInfo);
    ZAxisHackDetection(player, movementInfo);
    m_Players[key].SetLastMovementInfo(movementInfo);
    m_Players[key].SetLastOpcode(opcode);
}

// basic detection
void AnticheatMgr::ClimbHackDetection(Player* player, MovementInfo movementInfo, uint32 opcode)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectClimbHack", true))
        return;

    ObjectGuid key = player->GetGUID();

    if (opcode != MSG_MOVE_HEARTBEAT ||
        m_Players[key].GetLastOpcode() != MSG_MOVE_HEARTBEAT)
        return;

    // in this case we don't care if they are "legal" flags, they are handled in another parts of the Anticheat Manager.
    if (player->IsInWater() ||
        player->IsFlying() ||
        player->IsFalling())
        return;

    Position playerPos = player->GetPosition();

    float deltaZ = fabs(playerPos.GetPositionZ() - movementInfo.pos.GetPositionZ());
    float deltaXY = movementInfo.pos.GetExactDist2d(&playerPos);

    float angle = Position::NormalizeOrientation(tan(deltaZ / deltaXY));

    if (angle > CLIMB_ANGLE)
    {
        if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
        {
            LOG_INFO("module", "AnticheatMgr:: Climb-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
        }

        BuildReport(player, CLIMB_HACK_REPORT);
    }
}

void AnticheatMgr::SpeedHackDetection(Player* player, MovementInfo movementInfo)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectSpeedHack", true))
        return;

    ObjectGuid key = player->GetGUID();

    // We also must check the map because the movementFlag can be modified by the client.
    // If we just check the flag, they could always add that flag and always skip the speed hacking detection.

    if (m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && player->GetMapId())
        switch (player->GetMapId())
        {
            case 369: //Transport: DEEPRUN TRAM
            case 607: //Transport: Strands of the Ancients
            case 582: //Transport: Rut'theran to Auberdine
            case 584: //Transport: Menethil to Theramore
            case 586: //Transport: Exodar to Auberdine
            case 587: //Transport: Feathermoon Ferry
            case 588: //Transport: Menethil to Auberdine
            case 589: //Transport: Orgrimmar to Grom'Gol
            case 590: //Transport: Grom'Gol to Undercity
            case 591: //Transport: Undercity to Orgrimmar
            case 592: //Transport: Borean Tundra Test
            case 593: //Transport: Booty Bay to Ratchet
            case 594: //Transport: Howling Fjord Sister Mercy (Quest)
            case 596: //Transport: Naglfar
            case 610: //Transport: Tirisfal to Vengeance Landing
            case 612: //Transport: Menethil to Valgarde
            case 613: //Transport: Orgrimmar to Warsong Hold
            case 614: //Transport: Stormwind to Valiance Keep
            case 620: //Transport: Moa'ki to Unu'pe
            case 621: //Transport: Moa'ki to Kamagua
            case 622: //Transport: Orgrim's Hammer
            case 623: //Transport: The Skybreaker
            case 641: //Transport: Alliance Airship BG
            case 642: //Transport: Horde Airship BG
            case 647: //Transport: Orgrimmar to Thunder Bluff
            case 672: //Transport: The Skybreaker (Icecrown Citadel Raid)
            case 673: //Transport: Orgrim's Hammer (Icecrown Citadel Raid)
            case 712: //Transport: The Skybreaker (IC Dungeon)
            case 713: //Transport: Orgrim's Hammer (IC Dungeon)
            case 718: //Transport: The Mighty Wind (Icecrown Citadel Raid)
                return;
            break;
                default:
            break;// Should never happen
        }

    switch (player->GetAreaId())
    {
        case 4281: //Acherus: The Ebon Hold
        case 4342: //Acherus: The Ebon Hold
            return;
        break;
            default:
        break;// Should never happen
    }

    uint32 distance2D = (uint32)movementInfo.pos.GetExactDist2d(&m_Players[key].GetLastMovementInfo().pos);
    uint8 moveType = 0;

    // we need to know HOW is the player moving
    // TO-DO: Should we check the incoming movement flags?
    if (player->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        moveType = MOVE_SWIM;
    else if (player->IsFlying())
        moveType = MOVE_FLIGHT;
    else if (player->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
        moveType = MOVE_WALK;
    else
        moveType = MOVE_RUN;

    // how many yards the player can do in one sec.
    // We remove the added speed for jumping because otherwise permanently jumping doubles your allowed speed
    uint32 speedRate = (uint32)(player->GetSpeed(UnitMoveType(moveType)));

    // how long the player took to move to here.
    uint32 timeDiff = getMSTimeDiff(m_Players[key].GetLastMovementInfo().time, movementInfo.time);

    if (int32(timeDiff) < 0)
    {
        BuildReport(player, SPEED_HACK_REPORT);
        timeDiff = 1;
    }

    if (!timeDiff)
        timeDiff = 1;

    // Exceptions
    switch (player->getClass())
    {
        case CLASS_ROGUE:
            // Killing Spree
            if (player->GetAura(51690))
                return;
            break;
        case CLASS_MAGE:
            // Blink
            if (player->GetAura(1953))
                return;
            break;
        default:
            break;// Should never happen
    }

    // this is the distance doable by the player in 1 sec, using the time done to move to this point.
    uint32 clientSpeedRate = distance2D * 1000 / timeDiff;

    // We did the (uint32) cast to accept a margin of tolerance
    // We check the last MovementInfo for the falling flag since falling down a hill and sliding a bit triggered a false positive
    if ((clientSpeedRate > speedRate * 1.25f) && !m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_FALLING))
    {
        if (!player->CanTeleport())
        {
            if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
            {
                LOG_INFO("module", "AnticheatMgr:: Speed-Hack detected player {} ({})", player->GetName(), player->GetGUID().ToString());
            }

            BuildReport(player, SPEED_HACK_REPORT);
        }
        else if (player->CanTeleport())
            player->SetCanTeleport(false);
    }
}

void AnticheatMgr::HandlePlayerLogin(Player* player)
{
    // we must delete this to prevent errors in case of crash
    CharacterDatabase.Execute("DELETE FROM players_reports_status WHERE guid={}", player->GetGUID().GetCounter());
    // we initialize the pos of lastMovementPosition var.
    m_Players[player->GetGUID()].SetPosition(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
    QueryResult resultDB = CharacterDatabase.Query("SELECT * FROM daily_players_reports WHERE guid={};", player->GetGUID().GetCounter());

    if (resultDB)
        m_Players[player->GetGUID()].SetDailyReportState(true);
}

void AnticheatMgr::HandlePlayerLogout(Player* player)
{
    // TO-DO Make a table that stores the cheaters of the day, with more detailed information.

    // We must also delete it at logout to prevent have data of offline players in the db when we query the database (IE: The GM Command)
    CharacterDatabase.Execute("DELETE FROM players_reports_status WHERE guid={}", player->GetGUID().GetCounter());
    // Delete not needed data from the memory.
    m_Players.erase(player->GetGUID());
}

void AnticheatMgr::SavePlayerData(Player* player)
{//                                                                  1       2         3            4           5            6                 7                     8             9               10
    CharacterDatabase.Execute("REPLACE INTO players_reports_status (guid,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports,creation_time) VALUES ({},{},{},{},{},{},{},{},{},{});", player->GetGUID().GetCounter(), m_Players[player->GetGUID()].GetAverage(), m_Players[player->GetGUID()].GetTotalReports(), m_Players[player->GetGUID()].GetTypeReports(SPEED_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(FLY_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(JUMP_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(WALK_WATER_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(TELEPORT_PLANE_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(CLIMB_HACK_REPORT), m_Players[player->GetGUID()].GetCreationTime());
}

uint32 AnticheatMgr::GetTotalReports(ObjectGuid guid)
{
    return m_Players[guid].GetTotalReports();
}

float AnticheatMgr::GetAverage(ObjectGuid guid)
{
    return m_Players[guid].GetAverage();
}

uint32 AnticheatMgr::GetTypeReports(ObjectGuid guid, uint8 type)
{
    return m_Players[guid].GetTypeReports(type);
}

bool AnticheatMgr::MustCheckTempReports(uint8 type)
{
    if (type == JUMP_HACK_REPORT)
        return false;

    if (type == TELEPORT_HACK_REPORT)
        return false;

    if (type == IGNORE_CONTROL_REPORT)
        return false;

    if (type == ZAXIS_HACK_REPORT)
        return false;

    return true;
}

//
// Dear maintainer:
//
// Once you are done trying to 'optimize' this script,
// and have identify potentionally if there was a terrible
// mistake that was here or not, please increment the
// following counter as a warning to the next guy:
//
// total_hours_wasted_here = 42
//

void AnticheatMgr::BuildReport(Player* player, uint16 reportType)
{
    ObjectGuid key = player->GetGUID();

    if (MustCheckTempReports(reportType))
    {
        uint32 actualTime = getMSTime();

        if (!m_Players[key].GetTempReportsTimer(reportType))
            m_Players[key].SetTempReportsTimer(actualTime, reportType);

        if (getMSTimeDiff(m_Players[key].GetTempReportsTimer(reportType), actualTime) < 3000)
        {
            m_Players[key].SetTempReports(m_Players[key].GetTempReports(reportType) + 1, reportType);

            if (m_Players[key].GetTempReports(reportType) < 3)
                return;
        }
        else
        {
            m_Players[key].SetTempReportsTimer(actualTime, reportType);
            m_Players[key].SetTempReports(1, reportType);
            return;
        }
    }

    // generating creationTime for average calculation
    if (!m_Players[key].GetTotalReports())
        m_Players[key].SetCreationTime(getMSTime());

    // increasing total_reports
    m_Players[key].SetTotalReports(m_Players[key].GetTotalReports() + 1);
    // increasing specific cheat report
    m_Players[key].SetTypeReports(reportType, m_Players[key].GetTypeReports(reportType) + 1);

    // diff time for average calculation
    uint32 diffTime = getMSTimeDiff(m_Players[key].GetCreationTime(), getMSTime()) / IN_MILLISECONDS;

    if (diffTime > 0)
    {
        // Average == Reports per second
        float average = float(m_Players[key].GetTotalReports()) / float(diffTime);
        m_Players[key].SetAverage(average);
    }

    if (sConfigMgr->GetOption<uint32>("Anticheat.MaxReportsForDailyReport", 70) < m_Players[key].GetTotalReports())
    {
        if (!m_Players[key].GetDailyReportState())
        {//                                                                 1     2          3             4           5            6                 7                     8             9             10
            CharacterDatabase.Execute("REPLACE INTO daily_players_reports (guid,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports,creation_time) VALUES ({},{},{},{},{},{},{},{},{},{});", player->GetGUID().GetCounter(), m_Players[player->GetGUID()].GetAverage(), m_Players[player->GetGUID()].GetTotalReports(), m_Players[player->GetGUID()].GetTypeReports(SPEED_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(FLY_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(JUMP_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(WALK_WATER_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(TELEPORT_PLANE_HACK_REPORT), m_Players[player->GetGUID()].GetTypeReports(CLIMB_HACK_REPORT), m_Players[player->GetGUID()].GetCreationTime());
            m_Players[key].SetDailyReportState(true);
        }
    }

    if (m_Players[key].GetTotalReports() > sConfigMgr->GetOption<uint32>("Anticheat.ReportsForIngameWarnings", 70))
    {
        // display warning at the center of the screen, hacky way?
        std::string str = "";
        str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Possible cheater!";
        WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
        data << str;
        sWorld->SendGlobalGMMessage(&data);
        // need better way to limit chat spam
        if (m_Players[key].GetTotalReports() >= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Min", 70) && m_Players[key].GetTotalReports() <= sConfigMgr->GetOption<uint32>("Anticheat.ReportinChat.Max", 80))
        {
            sWorld->SendGMText(LANG_ANTICHEAT_ALERT, player->GetName().c_str(), player->GetName().c_str());
        }
    }

    if (sConfigMgr->GetOption<bool>("Anticheat.KickPlayer", true) && m_Players[key].GetTotalReports() > sConfigMgr->GetOption<uint32>("Anticheat.ReportsForKick", 70))
    {
        if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
        {
            LOG_INFO("module", "AnticheatMgr:: Reports reached assigned threshhold and counteracted by kicking player {} ({})", player->GetName(), player->GetGUID().ToString());
        }
        // display warning at the center of the screen, hacky way?
        std::string str = "";
        str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Auto Kicked for Reaching Cheat Threshhold!";
        WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
        data << str;
        sWorld->SendGlobalGMMessage(&data);

        player->GetSession()->KickPlayer(true);
        if (sConfigMgr->GetOption<bool>("Anticheat.AnnounceKick", true))
        {
            std::string plr = player->GetName();
            std::string tag_colour = "7bbef7";
            std::string plr_colour = "ff0000";
            std::ostringstream stream;
            stream << "|CFF" << plr_colour << "[AntiCheat]|r|CFF" << tag_colour <<
                " Player |r|cff" << plr_colour << plr << "|r|cff" << tag_colour <<
                " has been kicked by the Anticheat Module.|r";
            sWorld->SendServerMessage(SERVER_MSG_STRING, stream.str().c_str());
        }
    }

    if (sConfigMgr->GetOption<bool>("Anticheat.BanPlayer", true) && m_Players[key].GetTotalReports() > sConfigMgr->GetOption<uint32>("Anticheat.ReportsForBan", 70))
    {
        if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
        {
            LOG_INFO("module", "AnticheatMgr:: Reports reached assigned threshhold and counteracted by banning player {} ({})", player->GetName(), player->GetGUID().ToString());
        }
        // display warning at the center of the screen, hacky way?
        std::string str = "";
        str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Auto Banned Account for Reaching Cheat Threshhold!";
        WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
        data << str;
        sWorld->SendGlobalGMMessage(&data);

        std::string accountName;
        AccountMgr::GetName(player->GetSession()->GetAccountId(), accountName);
        sBan->BanAccount(accountName, "0s", "Anticheat module Auto Banned Account for Reach Cheat Threshhold", "Server");

        if (sConfigMgr->GetOption<bool>("Anticheat.AnnounceBan", true))
        {
            std::string plr = player->GetName();
            std::string tag_colour = "7bbef7";
            std::string plr_colour = "ff0000";
            std::ostringstream stream;
            stream << "|CFF" << plr_colour << "[AntiCheat]|r|CFF" << tag_colour <<
                " Player |r|cff" << plr_colour << plr << "|r|cff" << tag_colour <<
                " has been Banned by the Anticheat Module.|r";
            sWorld->SendServerMessage(SERVER_MSG_STRING, stream.str().c_str());
        }
    }

    if (sConfigMgr->GetOption<bool>("Anticheat.JailPlayer", true) && m_Players[key].GetTotalReports() > sConfigMgr->GetOption<uint32>("Anticheat.ReportsForJail", 70))
    {
        if (sConfigMgr->GetOption<bool>("Anticheat.WriteLog", true))
        {
            LOG_INFO("module", "AnticheatMgr:: Reports reached assigned threshhold and counteracted by jailing player {} ({})", player->GetName(), player->GetGUID().ToString());
        }
        // display warning at the center of the screen, hacky way?
        std::string str = "";
        str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Auto Jailed Account for Reaching Cheat Threshhold!";
        WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
        data << str;
        sWorld->SendGlobalGMMessage(&data);

        WorldLocation loc;
        loc = WorldLocation(1, 16226.5f, 16403.6f, -64.5f, 3.2f); // GM Jail Location
        player->TeleportTo(loc);
        player->SetHomebind(loc, 876); // GM Jail Homebind location
        player->CastSpell(player, 38505); // Shackle him in place to ensure no exploit happens for jail break attempt

        if (sConfigMgr->GetOption<bool>("Anticheat.AnnounceJail", true))
        {
            std::string plr = player->GetName();
            std::string tag_colour = "7bbef7";
            std::string plr_colour = "ff0000";
            std::ostringstream stream;
            stream << "|CFF" << plr_colour << "[AntiCheat]|r|CFF" << tag_colour <<
                " Player |r|cff" << plr_colour << plr << "|r|cff" << tag_colour <<
                " has been Jailed by the Anticheat Module.|r";
            sWorld->SendServerMessage(SERVER_MSG_STRING, stream.str().c_str());
        }
    }
}

void AnticheatMgr::AnticheatGlobalCommand(ChatHandler* handler)
{
    // MySQL will sort all for us, anyway this is not the best way we must only save the anticheat data not whole player's data!.
    ObjectAccessor::SaveAllPlayers();

    QueryResult resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY average ASC LIMIT 3;");
    if (!resultDB)
    {
        handler->PSendSysMessage("No players found.");
        return;
    }
    else
    {
        handler->SendSysMessage("=============================");
        handler->PSendSysMessage("Players with the lowest averages:");
        do
        {
            Field* fieldsDB = resultDB->Fetch();

            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(fieldsDB[0].Get<uint32>());
            float average = fieldsDB[1].Get<float>();
            uint32 total_reports = fieldsDB[2].Get<uint32>();

            if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
                handler->PSendSysMessage("Player: %s Average: %f Total Reports: %u", player->GetName().c_str(), average, total_reports);

        } while (resultDB->NextRow());
    }

    resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY total_reports DESC LIMIT 3;");

    // this should never happen
    if (!resultDB)
    {
        handler->PSendSysMessage("No players found.");
        return;
    }
    else
    {
        handler->PSendSysMessage("=============================");
        handler->PSendSysMessage("Players with the more reports:");
        do
        {
            Field* fieldsDB = resultDB->Fetch();

            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(fieldsDB[0].Get<uint32>());
            float average = fieldsDB[1].Get<float>();
            uint32 total_reports = fieldsDB[2].Get<uint32>();

            if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
                handler->PSendSysMessage("Player: %s Total Reports: %u Average: %f", player->GetName().c_str(), total_reports, average);

        } while (resultDB->NextRow());
    }
}

void AnticheatMgr::AnticheatDeleteCommand(ObjectGuid guid)
{
    if (!guid)
    {
        for (AnticheatPlayersDataMap::iterator it = m_Players.begin(); it != m_Players.end(); ++it)
        {
            (*it).second.SetTotalReports(0);
            (*it).second.SetAverage(0);
            (*it).second.SetCreationTime(0);
            for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
            {
                (*it).second.SetTempReports(0, i);
                (*it).second.SetTempReportsTimer(0, i);
                (*it).second.SetTypeReports(i, 0);
            }
        }
        CharacterDatabase.Execute("DELETE FROM players_reports_status;");
    }
    else
    {
        m_Players[guid].SetTotalReports(0);
        m_Players[guid].SetAverage(0);
        m_Players[guid].SetCreationTime(0);
        for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
        {
            m_Players[guid].SetTempReports(0, i);
            m_Players[guid].SetTempReportsTimer(0, i);
            m_Players[guid].SetTypeReports(i, 0);
        }
        CharacterDatabase.Execute("DELETE FROM players_reports_status WHERE guid={};", guid.GetCounter());
    }
}

void AnticheatMgr::ResetDailyReportStates()
{
    for (AnticheatPlayersDataMap::iterator it = m_Players.begin(); it != m_Players.end(); ++it)
        m_Players[(*it).first].SetDailyReportState(false);
}
