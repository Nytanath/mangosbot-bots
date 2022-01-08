#include "botpch.h"
#include "../../playerbot.h"
#include "DruidMultipliers.h"
#include "HealDruidStrategy.h"

using namespace ai;

class HealDruidStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    HealDruidStrategyActionNodeFactory()
    {
    }
private:
};

HealDruidStrategy::HealDruidStrategy(PlayerbotAI* ai) : GenericDruidStrategy(ai)
{
    actionNodeFactories.Add(new HealDruidStrategyActionNodeFactory());
}
/*
void HealDruidStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericDruidStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "enemy out of spell",
        NextAction::array(0, new NextAction("reach spell", ACTION_NORMAL + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "tree form",
        NextAction::array(0, new NextAction("tree form", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "medium health",
        NextAction::array(0, new NextAction("regrowth", ACTION_MEDIUM_HEAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "party member medium health",
        NextAction::array(0, new NextAction("regrowth on party", ACTION_MEDIUM_HEAL + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "almost full health",
        NextAction::array(0, new NextAction("rejuvenation", ACTION_LIGHT_HEAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "party member almost full health",
        NextAction::array(0, new NextAction("rejuvenation on party", ACTION_LIGHT_HEAL + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe heal",
        NextAction::array(0, new NextAction("tranquility", ACTION_MEDIUM_HEAL + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "party member to heal out of spell range",
        NextAction::array(0, new NextAction("reach party member to heal", ACTION_CRITICAL_HEAL + 1), NULL)));
}*/

void HealDruidStrategy::InitTriggers(std::list<TriggerNode*>& triggers)
{

GenericDruidStrategy::InitTriggers(triggers);

// Basic, move to being in range
triggers.push_back(new TriggerNode(
    "enemy out of spell",
    NextAction::array(0, new NextAction("reach spell", ACTION_NORMAL + 9), NULL)));

triggers.push_back(new TriggerNode(
    "party member to heal out of spell range",
    NextAction::array(0, new NextAction("reach party member to heal", ACTION_CRITICAL_HEAL + 1), NULL)));

// Basic, use tree of life 
triggers.push_back(new TriggerNode(
    "tree form", 
    NextAction::array(0, new NextAction("tree form", ACTION_HIGH + 1), NULL)));

// Self healing 
// High health (Use HoTs)
triggers.push_back(new TriggerNode(
    "almost full health",
    NextAction::array(0, new NextAction("life bloom", ACTION_LIGHT_HEAL), NULL)));

triggers.push_back(new TriggerNode(
    "almost full health", 
    NextAction::array(0, new NextAction("regrowth", ACTION_LIGHT_HEAL + 1), NULL)));

triggers.push_back(new TriggerNode(
    "almost full health",
    NextAction::array(0, new NextAction("wild growth", ACTION_LIGHT_HEAL + 2), NULL)));

// Medium health (Use casts)
triggers.push_back(new TriggerNode(
    "medium health",
    NextAction::array(0, new NextAction("rejuvenation", ACTION_MEDIUM_HEAL), NULL)));

triggers.push_back(new TriggerNode(
    "party member medium health",
    NextAction::array(0, new NextAction("nourish", ACTION_MEDIUM_HEAL + 1), NULL)));

// Critical (Use cooldowns)
triggers.push_back(new TriggerNode(
    "critical health",
    NextAction::array(0, new NextAction("healing touch", ACTION_CRITICAL_HEAL), NULL)));

triggers.push_back(new TriggerNode(
    "critical health",
    NextAction::array(0, new NextAction("swiftmend", ACTION_CRITICAL_HEAL + 1), NULL)));


// High health management (Use only HoTs)
triggers.push_back(new TriggerNode("party member almost full health", NextAction::array(0, new NextAction("life bloom on party", ACTION_LIGHT_HEAL), NULL)));
triggers.push_back(new TriggerNode("party member almost full health", NextAction::array(0, new NextAction("regrowth on party", ACTION_LIGHT_HEAL + 1), NULL)));
triggers.push_back(new TriggerNode("party member almost full health", NextAction::array(0, new NextAction("wild growth on party", ACTION_LIGHT_HEAL + 2), NULL)));
// Medium Health Management  (Start using casted spells)
triggers.push_back(new TriggerNode("party member medium health", NextAction::array(0, new NextAction("rejuvenation on party", ACTION_MEDIUM_HEAL), NULL)));
triggers.push_back(new TriggerNode("party member medium health", NextAction::array(0, new NextAction("nourish on party", ACTION_MEDIUM_HEAL + 1), NULL)));
// Critical Health Management (Start using long spells and cooldowns)
triggers.push_back(new TriggerNode("party member critical health", NextAction::array(0, new NextAction("healing touch on party", ACTION_CRITICAL_HEAL), NULL)));
triggers.push_back(new TriggerNode("party member critical health", NextAction::array(0, new NextAction("swiftmend on party", ACTION_CRITICAL_HEAL + 1), NULL)));


// AoE healing 
triggers.push_back(new TriggerNode("medium aoe heal", NextAction::array(0, new NextAction("tranquility", ACTION_MEDIUM_HEAL + 3), NULL)));
}
