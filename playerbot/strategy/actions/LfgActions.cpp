#include "botpch.h"
#include "../../playerbot.h"
#include "LfgActions.h"
#include "../ItemVisitors.h"
//#include "../../AiFactory.h"
//#include "../../PlayerbotAIConfig.h"
//#include "../ItemVisitors.h"
//#include "../../RandomPlayerbotMgr.h"
//#include "../../../../game/LFGMgr.h"

using namespace ai;

bool LfgJoinAction::Execute(Event event)
{
    return JoinLFG();
}

#ifdef MANGOSBOT_TWO
LFGRoleMask LfgJoinAction::GetRoles()
{
    if (!sRandomPlayerbotMgr.IsRandomBot(bot))
    {
        if (ai->IsTank(bot))
            return LFG_ROLE_MASK_TANK;
        if (ai->IsHeal(bot))
            return LFG_ROLE_MASK_HEALER;
        else
            return LFG_ROLE_MASK_DAMAGE;
    }

    int spec = AiFactory::GetPlayerSpecTab(bot);
    switch (bot->getClass())
    {
    case CLASS_DRUID:
        if (spec == 2)
            return LFG_ROLE_MASK_HEALER;
        else if (spec == 1 && bot->GetLevel() >= 20)
            return LFG_ROLE_MASK_TANK_DAMAGE;
        else
            return LFG_ROLE_MASK_DAMAGE;
        break;
    case CLASS_PALADIN:
        if (spec == 1)
            return LFG_ROLE_MASK_TANK;
        else if (spec == 0)
            return LFG_ROLE_MASK_HEALER;
        else
            return LFG_ROLE_MASK_DAMAGE;
        break;
    case CLASS_PRIEST:
        if (spec != 2)
            return LFG_ROLE_MASK_HEALER;
        else
            return LFG_ROLE_MASK_DAMAGE;
        break;
    case CLASS_SHAMAN:
        if (spec == 2)
            return LFG_ROLE_MASK_HEALER;
        else
            return LFG_ROLE_MASK_DAMAGE;
        break;
    case CLASS_WARRIOR:
        if (spec == 2)
            return LFG_ROLE_MASK_TANK;
        else
            return LFG_ROLE_MASK_DAMAGE;
        break;
#ifdef MANGOSBOT_TWO
    case CLASS_DEATH_KNIGHT:
        if (spec == 0)
            return LFG_ROLE_MASK_TANK;
        else
            return LFG_ROLE_MASK_DAMAGE;
        break;
#endif
    default:
        return LFG_ROLE_MASK_DAMAGE;
        break;
    }

    return LFG_ROLE_MASK_DAMAGE;
}
#endif

bool LfgJoinAction::SetRoles()
{
#ifdef MANGOSBOT_TWO
    LFGPlayerState* pState = sLFGMgr.GetLFGPlayerState(bot->GetObjectGuid());
    if (!pState)
        return false;

    pState->SetRoles(GetRoles());
#endif

    return true;
}

bool LfgJoinAction::JoinLFG()
{
#ifdef MANGOSBOT_ZERO
    //ItemCountByQuality visitor;
    //IterateItems(&visitor, ITERATE_ITEMS_IN_EQUIP);
    //bool raid = (urand(0, 100) < 50 && visitor.count[ITEM_QUALITY_EPIC] >= 5 && (bot->GetLevel() == 60 || bot->GetLevel() == 70 || bot->GetLevel() == 80));

    MeetingStoneSet stones = sLFGMgr.GetDungeonsForPlayer(bot);
    if (!stones.size())
        return false;

    vector<uint32> dungeons = sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()];
    if (!dungeons.size())
        return false;

    vector<MeetingStoneInfo> selected;
    for (vector<uint32>::iterator i = dungeons.begin(); i != dungeons.end(); ++i)
    {
        uint32 zoneId = 0;
        uint32 dungeonId = (*i & 0xFFFF);
        zoneId = ((*i >> 16) & 0xFFFF);

        // join only if close to closest graveyard
        if (zoneId)
        {
            WorldSafeLocsEntry const* ClosestGrave = nullptr;
            ClosestGrave = sWorldSafeLocsStore.LookupEntry<WorldSafeLocsEntry>(zoneId);

            bool inCity = false;
            AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(bot->GetAreaId());
            if (areaEntry)
            {
                if (areaEntry->zone)
                    areaEntry = GetAreaEntryByAreaID(areaEntry->zone);

                if (areaEntry && areaEntry->flags & AREA_FLAG_CAPITAL)
                    inCity = true;
            }

            if (ClosestGrave)
            {
                if (!(inCity || bot->GetMapId() == ClosestGrave->map_id))
                    continue;
            }
            else
                continue;
        }

        for (MeetingStoneSet::iterator i = stones.begin(); i != stones.end(); ++i)
        {
            if (i->area == dungeonId)
                selected.push_back(*i);
        }
    }

    if (!selected.size())
        return false;

    uint32 dungeon = urand(0, selected.size() - 1);
    MeetingStoneInfo stoneInfo = selected[dungeon];
    BotRoles botRoles = AiFactory::GetPlayerRoles(bot);
    string _botRoles;
    switch (botRoles)
    {
    case BOT_ROLE_TANK:
        _botRoles = "Tank";
        break;
    case BOT_ROLE_HEALER:
        _botRoles = "Healer";
        break;
    case BOT_ROLE_DPS:
    default:
        _botRoles = "Dps";
        break;
    }

    if (botRoles & BOT_ROLE_TANK && botRoles & BOT_ROLE_DPS)
        _botRoles = "Tank/Dps";
    /*for (MeetingStoneSet::const_iterator itr = stones.begin(); itr != stones.end(); ++itr)
    {
        auto data = *itr;

        idx.push_back(data.area);
    }

    if (idx.empty())
        return false;*/

    sLog.outBasic("Bot #%d %s:%d <%s>: uses LFG, Dungeon - %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), stoneInfo.name, _botRoles);

    sLFGMgr.AddToQueue(bot, stoneInfo.area);
#endif
#ifdef MANGOSBOT_ONE
    uint32 zoneLFG = 0;
    uint32 questLFG = 0;
    uint32 questZoneLFG = 0;
    string questName;
    string zoneName;
    string lfgName;
    uint32 needMembers = 0;
    LfgType lfgType = LFG_TYPE_NONE;
    TravelState state = TRAVEL_STATE_IDLE;
    TravelStatus status = TRAVEL_STATUS_NONE;

    GrouperType grouperType = ai->GetGrouperType();

    if (grouperType == GrouperType::SOLO)
        return false;

    bool isLFG = false; // member
    bool isLFM = false; // leader

    if (grouperType == GrouperType::MEMBER)
        isLFG = true;

    if (grouperType >= GrouperType::LEADER_2)
        isLFM = true;

    Group* group = bot->GetGroup();

    // don't use lfg if already has group
    if (isLFG && group)
        return false;

    needMembers = uint8(grouperType) - 1;

    // don't use lfg if not leader or group has enough members
    if (isLFM && group)
    {
        if (group->IsFull())
            return false;

        if (ai->GetGroupMaster() != bot)
            return false;

        uint32 memberCount = group->GetMembersCount();

        if (memberCount >= uint8(grouperType))
            return false;

        needMembers = uint8(grouperType) - memberCount;
    }

    bool groupQ = false;
    TravelTarget* target = bot->GetPlayerbotAI()->GetAiObjectContext()->GetValue<TravelTarget*>("travel target")->Get();
    if (target)
    {
        state = target->getTravelState();
        status = target->getStatus();
        // queue only if quest not completed
        if (state < TRAVEL_STATE_TRAVEL_HAND_IN_QUEST)
        {
            Quest const* quest = NULL;
            if (target->getDestination())
                quest = target->getDestination()->GetQuestTemplate();

            if (quest)
            {
                Quest const* qinfo = sObjectMgr.GetQuestTemplate(quest->GetQuestId());
                if (qinfo)
                {
                    // only group quests are allowed for quest lfg
                    if (qinfo->GetType() == QUEST_TYPE_ELITE)
                        groupQ = true;

                    questLFG = qinfo->GetQuestId();
                    questZoneLFG = qinfo->GetZoneOrSort();
                    questName = qinfo->GetTitle();
                }
            }
        }
    }

    /*AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(bot->GetZoneId());
    // check if area has no parent zone
    if (areaEntry && !areaEntry->zone)
    {
        zoneLFG = areaEntry->ID;
        zoneName = areaEntry->area_name[0];
    }*/

    // only use lfg zone if current quest leads there
    if (questZoneLFG)
    {
        AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(questZoneLFG);
        // check if area has no parent zone
        if (areaEntry && !areaEntry->zone)
        {
            zoneLFG = areaEntry->ID;
            zoneName = areaEntry->area_name[0];
        }
    }
    else if (!bot->IsTaxiFlying())
    {
        AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(bot->GetZoneId());
        // check if area has no parent zone
        if (areaEntry && !areaEntry->zone)
        {
            zoneLFG = areaEntry->ID;
            zoneName = areaEntry->area_name[0];
        }
    }

    bool joinedLFG = false;
    bool realLFG = false;
    // if bot wants to lead
    if (isLFM)
    {
        // lfm for current elite quest
        if (questLFG && groupQ)
        {
            lfgType = LFG_TYPE_QUEST;
            lfgName = questName;
            WorldPacket p;
            uint32 temp = questLFG | (lfgType << 24);
            p << temp;
            bot->GetSession()->HandleSetLfmOpcode(p);
            bot->GetSession()->HandleLfmClearAutoFillOpcode(p);
            joinedLFG = true;
        }
        // lfm for current quest zone or just current zone
        else if (zoneLFG && (questZoneLFG || (status >= TRAVEL_STATUS_COOLDOWN || state >= TRAVEL_STATE_TRAVEL_HAND_IN_QUEST || (state == TRAVEL_STATE_IDLE && !urand(0, 4)))))
        {
            for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
            {
                if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i))
                {
                    if (dungeon->maxlevel < bot->GetLevel())
                        continue;
                    if (dungeon->minlevel > bot->GetLevel())
                        continue;
                    // skip enemy faction
                    if (dungeon->faction == 0 && bot->GetTeam() == ALLIANCE)
                        continue;
                    if (dungeon->faction == 1 && bot->GetTeam() == HORDE)
                        continue;

                    // check by zone name...
                    if (dungeon->name[0] == zoneName)
                    {
                        lfgType = LFG_TYPE_ZONE;
                        lfgName = zoneName;
                        WorldPacket p;
                        uint32 temp = dungeon->ID | (lfgType << 24);
                        p << temp;
                        bot->GetSession()->HandleSetLfmOpcode(p);
                        bot->GetSession()->HandleLfmClearAutoFillOpcode(p);
                        joinedLFG = true;
                        break;
                    }
                }
            }
        }
        // lfm for random dungeon if nothing else to do
        else if (status >= TRAVEL_STATUS_COOLDOWN || state >= TRAVEL_STATE_TRAVEL_HAND_IN_QUEST || (state == TRAVEL_STATE_IDLE && !urand(0, 4)))
        {
            vector<uint32> dungeons;
            for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
            {
                if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i))
                {
                    if (dungeon->maxlevel < bot->GetLevel())
                        continue;
                    if (dungeon->minlevel > bot->GetLevel())
                        continue;

                    // only normal dungeons
                    if (dungeon->type != 1)
                        continue;

                    // skip enemy faction
                    if (dungeon->faction == 0 && bot->GetTeam() == ALLIANCE)
                        continue;
                    if (dungeon->faction == 1 && bot->GetTeam() == HORDE)
                        continue;

                    dungeons.push_back(dungeon->ID);
                }
            }
            if (!dungeons.empty())
            {
                uint32 dungeonId = dungeons[urand(0, dungeons.size() - 1)];
                if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(dungeonId))
                {
                    lfgType = LFG_TYPE_DUNGEON;
                    lfgName = dungeon->name[0];
                    WorldPacket p;
                    uint32 temp = dungeon->ID | (lfgType << 24);
                    p << temp;
                    bot->GetSession()->HandleSetLfmOpcode(p);
                    bot->GetSession()->HandleLfmClearAutoFillOpcode(p);
                    joinedLFG = true;
                }
            }
        }
        // try to LFM for current dungeon
        if (target && group && !group->IsFull())
        {
            // if moving to boss
            if (target->getDestination() && target->getDestination()->getName() == "BossTravelDestination")
            {
                WorldPosition* location = target->getPosition();
                uint32 targetAreaFlag = GetAreaFlagByMapId(location->mapid);
                if (targetAreaFlag)
                {
                    AreaTableEntry const* areaEntry = GetAreaEntryByAreaFlagAndMap(targetAreaFlag, location->mapid);
                    if (areaEntry)
                    {
                        if (areaEntry->zone)
                            areaEntry = GetAreaEntryByAreaID(areaEntry->zone);

                        if (areaEntry && !areaEntry->zone)
                        {
                            for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
                            {
                                if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i))
                                {
                                    if (dungeon->maxlevel < bot->GetLevel())
                                        continue;
                                    if (dungeon->minlevel > bot->GetLevel())
                                        continue;

                                    // only normal dungeons
                                    if (dungeon->type != 1)
                                        continue;

                                    // skip enemy faction
                                    if (dungeon->faction == 0 && bot->GetTeam() == ALLIANCE)
                                        continue;
                                    if (dungeon->faction == 1 && bot->GetTeam() == HORDE)
                                        continue;

                                    // check by zone name, doesn't work for some dungeons
                                    if (dungeon->name[0] == areaEntry->area_name[0])
                                    {
                                        lfgType = LFG_TYPE_DUNGEON;
                                        lfgName = dungeon->name[0];
                                        WorldPacket p;
                                        uint32 temp = dungeon->ID | (lfgType << 24);
                                        p << temp;
                                        bot->GetSession()->HandleSetLfmOpcode(p);
                                        bot->GetSession()->HandleLfmSetAutoFillOpcode(p);

                                        // set auto invite if real player in queue
                                        vector<uint32> player_dungeons = sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()];
                                        if (!player_dungeons.empty())
                                        {
                                            for (vector<uint32>::iterator i = player_dungeons.begin(); i != player_dungeons.end(); ++i)
                                            {
                                                uint32 zoneId = 0;
                                                uint32 entry = (*i & 0xFFFF);
                                                uint32 type = ((*i >> 8) & 0xFFFF);
                                                zoneId = ((*i >> 16) & 0xFFFF);

                                                // only lfg
                                                if (type != 0)
                                                    continue;

                                                // join only if close to closest graveyard
                                                if (zoneId)
                                                {
                                                    WorldSafeLocsEntry const* ClosestGrave = nullptr;
                                                    ClosestGrave = sWorldSafeLocsStore.LookupEntry<WorldSafeLocsEntry>(zoneId);

                                                    bool inCity = false;
                                                    AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(bot->GetAreaId());
                                                    if (areaEntry)
                                                    {
                                                        if (areaEntry->zone)
                                                            areaEntry = GetAreaEntryByAreaID(areaEntry->zone);

                                                        if (areaEntry && areaEntry->flags & AREA_FLAG_CAPITAL)
                                                            inCity = true;
                                                    }

                                                    if (ClosestGrave)
                                                    {
                                                        if (!(inCity || bot->GetMapId() == ClosestGrave->map_id))
                                                            continue;
                                                    }
                                                    else
                                                        continue;
                                                }

                                                if (LFGDungeonEntry const* player_dungeon = sLFGDungeonStore.LookupEntry(entry))
                                                {
                                                    if (player_dungeon->ID == dungeon->ID)
                                                        bot->GetSession()->HandleLfmSetAutoFillOpcode(p);

                                                    realLFG = true;
                                                }
                                            }
                                        }
                                        joinedLFG = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // set auto invite if real player in queue 
        vector<uint32> player_dungeons = sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()];
        if (!player_dungeons.empty() && !group)
        {
            for (vector<uint32>::iterator i = player_dungeons.begin(); i != player_dungeons.end(); ++i)
            {
                uint32 zoneId = 0;
                uint32 entry = (*i & 0xFFFF) & 0xFF;
                uint32 type = ((*i & 0xFFFF) >> 8);
                zoneId = (*i >> 16) & 0xFFFF;

                // leader only queue if player is lfg, not lfm
                if (type != 0)
                    continue;

                // join only if close to closest graveyard
                if (zoneId)
                {
                    WorldSafeLocsEntry const* ClosestGrave = nullptr;
                    ClosestGrave = sWorldSafeLocsStore.LookupEntry<WorldSafeLocsEntry>(zoneId);

                    bool inCity = false;
                    AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(bot->GetAreaId());
                    if (areaEntry)
                    {
                        if (areaEntry->zone)
                            areaEntry = GetAreaEntryByAreaID(areaEntry->zone);

                        if (areaEntry && areaEntry->flags & AREA_FLAG_CAPITAL)
                            inCity = true;
                    }

                    if (ClosestGrave)
                    {
                        if (!(inCity || bot->GetMapId() == ClosestGrave->map_id))
                            continue;
                    }
                    else
                        continue;
                }

                if (LFGDungeonEntry const* player_dungeon = sLFGDungeonStore.LookupEntry(entry))
                {
                    if (player_dungeon->maxlevel < bot->GetLevel())
                        continue;
                    if (player_dungeon->minlevel > bot->GetLevel())
                        continue;

                    // only normal dungeons
                    if (player_dungeon->type != 1)
                        continue;

                    // skip enemy faction
                    if (player_dungeon->faction == 0 && bot->GetTeam() == ALLIANCE)
                        continue;
                    if (player_dungeon->faction == 1 && bot->GetTeam() == HORDE)
                        continue;

                    lfgType = LFG_TYPE_DUNGEON;
                    lfgName = player_dungeon->name[0];
                    WorldPacket p;
                    uint32 temp = player_dungeon->ID | (lfgType << 24);
                    p << temp;
                    bot->GetSession()->HandleSetLfmOpcode(p);
                    bot->GetSession()->HandleLfmSetAutoFillOpcode(p);
                    joinedLFG = true;
                    realLFG = true;
                    break;
                }
            }
        }
    }
    else if (isLFG) // bot is alone and looks for group
    {
        // lfg slot 1 for current elite quest
        if (questLFG && groupQ)
        {
            lfgType = LFG_TYPE_QUEST;
            lfgName = questName;
            WorldPacket p;
            p << uint32(0); // lfg slot
            uint32 temp = questLFG | (lfgType << 24);
            p << temp;
            bot->GetSession()->HandleSetLfgOpcode(p);
            bot->GetSession()->HandleLfgClearAutoJoinOpcode(p);
            joinedLFG = true;
        }
        // lfg slot 2 for current quest zone or just current zone
        if (zoneLFG && (questZoneLFG || (status >= TRAVEL_STATUS_COOLDOWN || state >= TRAVEL_STATE_TRAVEL_HAND_IN_QUEST || (state == TRAVEL_STATE_IDLE && !urand(0, 4))))) // use second lfg slot for zone lfg
        {
            for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
            {
                if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i))
                {
                    if (dungeon->maxlevel < bot->GetLevel())
                        continue;
                    if (dungeon->minlevel > bot->GetLevel())
                        continue;

                    // skip enemy faction
                    if (dungeon->faction == 0 && bot->GetTeam() == ALLIANCE)
                        continue;
                    if (dungeon->faction == 1 && bot->GetTeam() == HORDE)
                        continue;

                    // check by zone name...
                    if (dungeon->name[0] == zoneName)
                    {
                        lfgType = LFG_TYPE_ZONE;
                        lfgName = zoneName;
                        WorldPacket p;
                        p << uint32(1); // lfg slot
                        uint32 temp = dungeon->ID | (lfgType << 24);
                        p << temp;
                        bot->GetSession()->HandleSetLfgOpcode(p);
                        bot->GetSession()->HandleLfgClearAutoJoinOpcode(p);
                        joinedLFG = true;
                        break;
                    }
                }
            }
        }
        // lfg for dungeon if real player used LFM with auto invite
        vector<uint32> player_dungeons = sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()];
        if (!player_dungeons.empty())
        {
            for (vector<uint32>::iterator i = player_dungeons.begin(); i != player_dungeons.end(); ++i)
            {
                uint32 zoneId = 0;
                uint32 entry = (*i & 0xFFFF) & 0xFF;
                uint32 type = ((*i & 0xFFFF) >> 8);
                zoneId = (*i >> 16) & 0xFFFF;

                // members queue only if player is lfm, not lfg
                if (type != 1)
                    continue;

                // join only if close to closest graveyard
                if (zoneId)
                {
                    WorldSafeLocsEntry const* ClosestGrave = nullptr;
                    ClosestGrave = sWorldSafeLocsStore.LookupEntry<WorldSafeLocsEntry>(zoneId);

                    bool inCity = false;
                    AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(bot->GetAreaId());
                    if (areaEntry)
                    {
                        if (areaEntry->zone)
                            areaEntry = GetAreaEntryByAreaID(areaEntry->zone);

                        if (areaEntry && areaEntry->flags & AREA_FLAG_CAPITAL)
                            inCity = true;
                    }

                    if (ClosestGrave)
                    {
                        if (!(inCity || bot->GetMapId() == ClosestGrave->map_id))
                            continue;
                    }
                    else
                        continue;
                }

                if (LFGDungeonEntry const* player_dungeon = sLFGDungeonStore.LookupEntry(entry))
                {
                    if (player_dungeon->maxlevel < bot->GetLevel())
                        continue;
                    if (player_dungeon->minlevel > bot->GetLevel())
                        continue;

                    // only normal dungeons
                    //if (player_dungeon->type != 1)
                    //    continue;

                    // skip enemy faction
                    if (player_dungeon->faction == 0 && bot->GetTeam() == ALLIANCE)
                        continue;
                    if (player_dungeon->faction == 1 && bot->GetTeam() == HORDE)
                        continue;

                    lfgType = LFG_TYPE_DUNGEON;
                    lfgName = player_dungeon->name[0];
                    WorldPacket p;
                    p << uint32(2); // lfg slot
                    uint32 temp = player_dungeon->ID | (lfgType << 24);
                    p << temp;
                    bot->GetSession()->HandleSetLfgOpcode(p);
                    bot->GetSession()->HandleLfgSetAutoJoinOpcode(p);
                    joinedLFG = true;
                    realLFG = true;
                    break;
                }
            }
        }
        // lfg slot 3 for random dungeon if not very busy
        else if (status >= TRAVEL_STATUS_COOLDOWN || state >= TRAVEL_STATE_TRAVEL_HAND_IN_QUEST || (state == TRAVEL_STATE_IDLE && !urand(0, 4)))
        {
            vector<uint32> dungeons;
            for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
            {
                if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i))
                {
                    if (dungeon->maxlevel < bot->GetLevel())
                        continue;
                    if (dungeon->minlevel > bot->GetLevel())
                        continue;

                    // only normal dungeons
                    if (dungeon->type != 1)
                        continue;

                    // skip enemy faction
                    if (dungeon->faction == 0 && bot->GetTeam() == ALLIANCE)
                        continue;
                    if (dungeon->faction == 1 && bot->GetTeam() == HORDE)
                        continue;

                    dungeons.push_back(dungeon->ID);
                }
            }
            if (!dungeons.empty())
            {
                uint32 dungeonId = dungeons[urand(0, dungeons.size() - 1)];
                if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(dungeonId))
                {
                    lfgType = LFG_TYPE_DUNGEON;
                    lfgName = dungeon->name[0];
                    WorldPacket p;
                    p << uint32(2); // lfg slot
                    uint32 temp = dungeon->ID | (lfgType << 24);
                    p << temp;
                    bot->GetSession()->HandleSetLfgOpcode(p);
                    bot->GetSession()->HandleLfgClearAutoJoinOpcode(p);

                    // set auto invite if real players in queue
                    /*vector<uint32> player_dungeons = sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()];
                    if (!player_dungeons.empty())
                    {
                        for (vector<uint32>::iterator i = player_dungeons.begin(); i != player_dungeons.end(); ++i)
                        {
                            uint32 zoneId = 0;
                            uint32 entry = (*i & 0xFFFF) & 0xFF;
                            uint32 type = ((*i & 0xFFFF) >> 8);
                            zoneId = (*i >> 16) & 0xFFFF;

                            // join only if in current zone
                            if (bot->GetZoneId() != zoneId)
                                continue;

                            // members queue only if player is lfm, not lfg
                            if (type != 1)
                                continue;

                            if (LFGDungeonEntry const* player_dungeon = sLFGDungeonStore.LookupEntry(entry))
                            {
                                if (player_dungeon->ID == dungeon->ID)
                                    bot->GetSession()->HandleLfgSetAutoJoinOpcode(p);
                            }
                        }
                    }*/
                    joinedLFG = true;
                }
            }
        }
    }
    
    if (!joinedLFG)
        return false;

    // set comment
    BotRoles botRoles = AiFactory::GetPlayerRoles(bot);
    string _botRoles;
    switch (botRoles)
    {
    case BOT_ROLE_TANK:
        _botRoles = "Tank";
        break;
    case BOT_ROLE_HEALER:
        _botRoles = "Healer";
        break;
    case BOT_ROLE_DPS:
    default:
        _botRoles = "Dps";
        break;
    }

    if (botRoles & BOT_ROLE_TANK && botRoles & BOT_ROLE_DPS)
        _botRoles = "Tank or Dps";

    WorldPacket p;
    string lfgComment;
    if (isLFM)
    {
        if (!group)
            lfgComment += _botRoles + ", ";

        lfgComment = "LF " + to_string(needMembers) + " more";
        if (questLFG)
            lfgComment += ", doing " + questName;
    }
    if (isLFG)
    {
        lfgComment = _botRoles;
        if (groupQ)
            lfgComment += ", lfg for " + questName;
        else if (questLFG)
            lfgComment += ", doing " + questName;
    }
    if (lfgType == LFG_TYPE_DUNGEON)
    {
        string _gs = to_string(bot->GetPlayerbotAI()->GetEquipGearScore(bot, false, false));
        lfgComment += ", GS " + _gs;
    }

    p << lfgComment;// +", GS - " + _gs;
    bot->GetSession()->HandleSetLfgCommentOpcode(p);
    string lfgGroup = isLFG ? "LFG" : "LFM";
    string lfgOption = lfgType == LFG_TYPE_QUEST ? "Quest" : (lfgType == LFG_TYPE_ZONE ? "Zone" : "Dungeon");

    if (realLFG)
        sLog.outBasic("Bot #%d %s:%d <%s>: uses %s, %s - %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), lfgGroup.c_str(), lfgOption.c_str(), lfgName.c_str(), _botRoles.c_str());
    else
        sLog.outDetail("Bot #%d %s:%d <%s>: uses %s, %s - %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), lfgGroup.c_str(), lfgOption.c_str(), lfgName.c_str(), _botRoles.c_str());
#endif
#ifdef MANGOSBOT_TWO
    LFGPlayerState* pState = sLFGMgr.GetLFGPlayerState(bot->GetObjectGuid());

    // check if already in lfg
    if (pState->GetDungeons() && !pState->GetDungeons()->empty())
        return false;

    // set roles
    if (!SetRoles())
        return false;

    ItemCountByQuality visitor;
    IterateItems(&visitor, ITERATE_ITEMS_IN_EQUIP);
    bool random = urand(0, 100) < 20;
    bool heroic = urand(0, 100) < 50 && (visitor.count[ITEM_QUALITY_EPIC] >= 3 || visitor.count[ITEM_QUALITY_RARE] >= 10) && bot->GetLevel() >= 70;
    bool raid = !heroic && (urand(0, 100) < 50 && visitor.count[ITEM_QUALITY_EPIC] >= 5 && (bot->GetLevel() == 60 || bot->GetLevel() == 70 || bot->GetLevel() == 80));

    LFGDungeonSet list;
    vector<uint32> selected;

    vector<uint32> dungeons = sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()];
    if (!dungeons.size())
        return false;

    for (vector<uint32>::iterator i = dungeons.begin(); i != dungeons.end(); ++i)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(*i);
        if (!dungeon || (dungeon->type != LFG_TYPE_RANDOM_DUNGEON && dungeon->type != LFG_TYPE_DUNGEON && dungeon->type != LFG_TYPE_HEROIC_DUNGEON &&
            dungeon->type != LFG_TYPE_RAID))
            continue;

        uint32 botLevel = bot->GetLevel();
        if (dungeon->minlevel && botLevel < dungeon->minlevel)
            continue;

        if (dungeon->minlevel && botLevel > dungeon->minlevel + 10)
            continue;

        if (dungeon->maxlevel && botLevel > dungeon->maxlevel)
            continue;

        selected.push_back(dungeon->ID);
        list.insert(dungeon);
    }

    if (!selected.size())
        return false;

    if (list.empty())
        return false;

    bool many = list.size() > 1;
    LFGDungeonEntry const* dungeon = *list.begin();

    /*for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i);
        if (!dungeon || (dungeon->type != LFG_TYPE_RANDOM_DUNGEON && dungeon->type != LFG_TYPE_DUNGEON && dungeon->type != LFG_TYPE_HEROIC_DUNGEON &&
            dungeon->type != LFG_TYPE_RAID))
            continue;

        uint32 botLevel = bot->GetLevel();
        if (dungeon->minlevel && botLevel < dungeon->minlevel)
            continue;

        if (dungeon->minlevel && botLevel > dungeon->minlevel + 10)
            continue;

        if (dungeon->maxlevel && botLevel > dungeon->maxlevel)
            continue;

        if (heroic && !dungeon->difficulty)
            continue;

        if (raid && dungeon->type != LFG_TYPE_RAID)
            continue;

        if (!random && !raid && !heroic && dungeon->type != LFG_TYPE_DUNGEON)
            continue;

        list.insert(dungeon);
    }

    if (list.empty() && !random)
        return false;*/

    // check role for console msg
    string _roles = "multiple roles";

    if (pState->HasRole(ROLE_TANK))
        _roles = "TANK";
    if (pState->HasRole(ROLE_HEALER))
        _roles = "HEAL";
    if (pState->HasRole(ROLE_DAMAGE))
        _roles = "DPS";

    pState->SetType(LFG_TYPE_DUNGEON);
    sLog.outBasic("Bot #%d %s:%d <%s>: queues LFG, Dungeon as %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), _roles, many ? "several dungeons" : dungeon->name[0]);

    /*if (lfgState->IsSingleRole())
    {
        if (lfgState->HasRole(ROLE_TANK))
            _roles = "TANK";
        if (lfgState->HasRole(ROLE_HEALER))
            _roles = "HEAL";
        if (lfgState->HasRole(ROLE_DAMAGE))
            _roles = "DPS";
    }*/

    /*LFGDungeonEntry const* dungeon;

    if(!random)
        dungeon = *list.begin();

    bool many = list.size() > 1;

    if (random)
    {
        LFGDungeonSet randList = sLFGMgr.GetRandomDungeonsForPlayer(bot);
        for (LFGDungeonSet::const_iterator itr = randList.begin(); itr != randList.end(); ++itr)
        {
            LFGDungeonEntry const* dungeon = *itr;

            if (!dungeon)
                continue;

            idx.push_back(dungeon->ID);
        }
        if (idx.empty())
            return false;

        // choose random dungeon
        dungeon = sLFGDungeonStore.LookupEntry(idx[urand(0, idx.size() - 1)]);
        list.insert(dungeon);

        pState->SetType(LFG_TYPE_RANDOM_DUNGEON);
        sLog.outBasic("Bot #%d %s:%d <%s>: queues LFG, Random Dungeon as %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), _roles, dungeon->name[0]);
        return true;
    }
    else if (heroic)
    {
        pState->SetType(LFG_TYPE_HEROIC_DUNGEON);
        sLog.outBasic("Bot #%d %s:%d <%s>: queues LFG, Heroic Dungeon as %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), _roles, many ? "several dungeons" : dungeon->name[0]);
    }
    else if (raid)
    {
        pState->SetType(LFG_TYPE_RAID);
        sLog.outBasic("Bot #%d  %s:%d <%s>: queues LFG, Raid as %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), _roles, many ? "several dungeons" : dungeon->name[0]);
    }
    else
    {
        pState->SetType(LFG_TYPE_DUNGEON);
        sLog.outBasic("Bot #%d %s:%d <%s>: queues LFG, Dungeon as %s (%s)", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), _roles, many ? "several dungeons" : dungeon->name[0]);
    }*/

    // Set Raid Browser comment
    string _gs = to_string(bot->GetEquipGearScore());
    pState->SetComment("Bot " + _roles + " GS:" + _gs + " for LFG");


    pState->SetDungeons(list);
    sLFGMgr.Join(bot);

#endif
    return true;
}

bool LfgRoleCheckAction::Execute(Event event)
{
#ifdef MANGOSBOT_TWO
    Group* group = bot->GetGroup();
    if (group)
    {
        LFGPlayerState* pState = sLFGMgr.GetLFGPlayerState(bot->GetObjectGuid());
        LFGRoleMask currentRoles = pState->GetRoles();
        LFGRoleMask newRoles = GetRoles();
        if (currentRoles == newRoles) return false;
        
        pState->SetRoles(newRoles);

        sLFGMgr.UpdateRoleCheck(group);

        sLog.outBasic("Bot #%d %s:%d <%s>: LFG roles checked", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName());

        return true;
    }
#endif

    return false;
}

bool LfgAcceptAction::Execute(Event event)
{
#ifdef MANGOSBOT_TWO
    LFGPlayerState* pState = sLFGMgr.GetLFGPlayerState(bot->GetObjectGuid());
    if (!pState || pState->GetState() != LFG_STATE_PROPOSAL)
        return false;

    uint32 id = AI_VALUE(uint32, "lfg proposal");
    if (id)
    {
        //if (urand(0, 1 + 10 / sPlayerbotAIConfig.randomChangeMultiplier))
        //    return false;

        if (bot->IsInCombat() || bot->IsDead())
        {
            sLog.outBasic("Bot #%d %s:%d <%s> is in combat and refuses LFG proposal %d", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), id);
            sLFGMgr.UpdateProposal(id, bot->GetObjectGuid(), false);
            return true;
        }

        sLog.outBasic("Bot #%d %s:%d <%s> accepts LFG proposal %d", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), id);
        ai->GetAiObjectContext()->GetValue<uint32>("lfg proposal")->Set(0);
        bot->clearUnitState(UNIT_STAT_ALL_STATE);
        sLFGMgr.UpdateProposal(id, bot->GetObjectGuid(), true);

        if (sRandomPlayerbotMgr.IsRandomBot(bot) && !bot->GetGroup())
        {
            sRandomPlayerbotMgr.Refresh(bot);
            ai->ResetStrategies();
            //bot->TeleportToHomebind();
        }

        ai->Reset();

        return true;
    }

    WorldPacket p(event.getPacket());

    uint32 dungeon;
    uint8 state;
    p >> dungeon >> state >> id;

    ai->GetAiObjectContext()->GetValue<uint32>("lfg proposal")->Set(id);
#endif
    return true;
}

bool LfgLeaveAction::Execute(Event event)
{
    // Don't leave if lfg strategy enabled
    //if (ai->HasStrategy("lfg", BOT_STATE_NON_COMBAT))
    //    return false;
#ifdef MANGOSBOT_ZERO
    LFGPlayerQueueInfo qInfo;
    sLFGMgr.GetPlayerQueueInfo(&qInfo, bot->GetObjectGuid());
    AreaTableEntry const* area = GetAreaEntryByAreaID(qInfo.areaId);
    if (area)
    {
        sLog.outBasic("Bot #%d %s:%d <%s>: leaves LFG queue to %s after %u minutes", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), area->area_name[0], (qInfo.timeInLFG / 60000));
        sLFGMgr.RemovePlayerFromQueue(bot->GetObjectGuid(), PLAYER_CLIENT_LEAVE);
    }
#endif
#ifdef MANGOSBOT_ONE
    bool isLFG = false;
    bool isLFM = false;
    for (int i = 0; i < MAX_LOOKING_FOR_GROUP_SLOT; ++i)
        if (!bot->m_lookingForGroup.slots[i].Empty())
            isLFG = true;

    if (!bot->m_lookingForGroup.more.Empty())
        isLFM = true;

    if (isLFG || isLFM)
    {
        sLog.outDetail("Bot #%d %s:%d <%s>: leaves %s", bot->GetGUIDLow(), bot->GetTeam() == ALLIANCE ? "A" : "H", bot->GetLevel(), bot->GetName(), isLFG ? "LFG" : "LFM");
        WorldPacket p;
        if (isLFG) // clear lfg
            bot->GetSession()->HandleLfgClearOpcode(p);
        if (isLFM) // clear lfm
            bot->GetSession()->HandleLfmClearOpcode(p);

        // clear comment
        string empty;
        p << empty;
        bot->GetSession()->HandleSetLfgCommentOpcode(p);
    }
#endif
#ifdef MANGOSBOT_TWO
    // Don't leave if already invited / in dungeon
    if (sLFGMgr.GetLFGPlayerState(bot->GetObjectGuid())->GetState() > LFG_STATE_QUEUED)
        return false;

    sLFGMgr.Leave(bot);
#endif
    return true;
}

bool LfgLeaveAction::isUseful()
{
#ifdef MANGOSBOT_ZERO
    if (!sLFGMgr.IsPlayerInQueue(bot->GetObjectGuid()))
        return false;
    else
    {
        LFGPlayerQueueInfo qInfo;
        sLFGMgr.GetPlayerQueueInfo(&qInfo, bot->GetObjectGuid());
        if (qInfo.timeInLFG < (5 * MINUTE * IN_MILLISECONDS))
            return false;
    }

    if (bot->GetGroup() && bot->GetGroup()->GetLeaderGuid() != bot->GetObjectGuid())
    {
        if (sLFGMgr.IsPlayerInQueue(bot->GetGroup()->GetLeaderGuid()))
            return false;
    }

    if ((ai->GetMaster() && !ai->GetMaster()->GetPlayerbotAI()))
    {
        return false;
    }
#endif
    return true;
}

bool LfgTeleportAction::Execute(Event event)
{
#ifdef MANGOSBOT_TWO
    bool out = false;

    WorldPacket p(event.getPacket());
    if (!p.empty())
    {
        p.rpos(0);
        p >> out;
    }

    bot->clearUnitState(UNIT_STAT_ALL_STATE);
    sLFGMgr.Teleport(bot, out);
#endif
    return true;
}

bool LfgJoinAction::isUseful()
{
    if (!sPlayerbotAIConfig.randomBotJoinLfg)
    {
        //ai->ChangeStrategy("-lfg", BOT_STATE_NON_COMBAT);
        return false;
    }

#ifdef MANGOSBOT_TWO
    if (bot->GetLevel() < 15)
        return false;
#endif

    // don't use if active player master
    if (ai->HasRealPlayerMaster())
        return false;

    if (bot->GetGroup() && bot->GetGroup()->GetLeaderGuid() != bot->GetObjectGuid())
    {
        //ai->ChangeStrategy("-lfg", BOT_STATE_NON_COMBAT);
        return false;
    }

    if (bot->IsBeingTeleported())
        return false;

    if (bot->InBattleGround())
        return false;

    if (bot->InBattleGroundQueue())
        return false;

    if (bot->IsDead())
        return false;

    if (!sRandomPlayerbotMgr.IsRandomBot(bot))
        return false;

    Map* map = bot->GetMap();
    if (map && map->Instanceable())
        return false;
    
#ifdef MANGOSBOT_ZERO
    if (sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()].empty())
        return false;

    if (sLFGMgr.IsPlayerInQueue(bot->GetObjectGuid()))
        return false;

    ClassRoles botRoles = sLFGMgr.CalculateTalentRoles(bot);

    RolesPriority prio = sLFGMgr.getPriority((Classes)bot->getClass(), (ClassRoles)botRoles);
    if (prio < LFG_PRIORITY_NORMAL)
        return false;

    if (bot->GetGroup() && bot->GetGroup()->IsFull())
        return false;
#endif
#ifdef MANGOSBOT_ONE
    bool isLFG = false;
    bool isLFM = false;
    for (int i = 0; i < MAX_LOOKING_FOR_GROUP_SLOT; ++i)
        if (!bot->m_lookingForGroup.slots[i].Empty())
            isLFG = true;

    if (!bot->m_lookingForGroup.more.Empty())
        isLFM = true;

    if ((isLFG || isLFM) && sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()].empty())
        return false;
#endif
#ifdef MANGOSBOT_TWO

    if (sRandomPlayerbotMgr.LfgDungeons[bot->GetTeam()].empty())
        return false;

    if (sLFGMgr.GetQueueInfo(bot->GetObjectGuid()))
        return false;

    LFGPlayerState* pState = sLFGMgr.GetLFGPlayerState(bot->GetObjectGuid());
    if (pState->GetState() != LFG_STATE_NONE)
        return false;
#endif
    return true;
}
