#pragma once

#include "../Action.h"
#include "../../PlayerbotAIConfig.h"

namespace ai
{
    class CastSpellAction : public Action
    {
    public:
        CastSpellAction(PlayerbotAI* ai, string spell) : Action(ai, spell),
            range(ai->GetRange("spell"))
        {
            this->spell = spell;
        }

		virtual string GetTargetName() { return "current target"; };
        virtual bool Execute(Event event);
        virtual bool isPossible();
		virtual bool isUseful();
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_SINGLE; }

		virtual NextAction** getPrerequisites()
		{
            if (spell == "mount")
                return NULL;
            if (range > ai->GetRange("spell"))
				return NULL;
			else if (range > ATTACK_DISTANCE)
				return NextAction::merge( NextAction::array(0, new NextAction("reach spell"), NULL), Action::getPrerequisites());
			else
				return NextAction::merge( NextAction::array(0, new NextAction("reach melee"), NULL), Action::getPrerequisites());
		}

    protected:
        string spell;
		float range;
    };

	//---------------------------------------------------------------------------------------------------------------------
	class CastAuraSpellAction : public CastSpellAction
	{
	public:
		CastAuraSpellAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell) {}

		virtual bool isUseful();
	};

    //---------------------------------------------------------------------------------------------------------------------
    class CastMeleeSpellAction : public CastSpellAction
    {
    public:
        CastMeleeSpellAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell) {
			range = ATTACK_DISTANCE;

            Unit* target = AI_VALUE(Unit*, "current target");
            if (target)
                range = max(5.0f, bot->GetCombinedCombatReach(target, true));

                //range = target->GetCombinedCombatReach();
		}
    };

    //---------------------------------------------------------------------------------------------------------------------
    class CastDebuffSpellAction : public CastAuraSpellAction
    {
    public:
        CastDebuffSpellAction(PlayerbotAI* ai, string spell) : CastAuraSpellAction(ai, spell) {}
    };

    class CastDebuffSpellOnAttackerAction : public CastAuraSpellAction
    {
    public:
        CastDebuffSpellOnAttackerAction(PlayerbotAI* ai, string spell) : CastAuraSpellAction(ai, spell) {}
        Value<Unit*>* GetTargetValue()
        {
            return context->GetValue<Unit*>("attacker without aura", spell);
        }
        virtual string getName() { return spell + " on attacker"; }
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_AOE; }
    };

	class CastBuffSpellAction : public CastAuraSpellAction
	{
	public:
		CastBuffSpellAction(PlayerbotAI* ai, string spell) : CastAuraSpellAction(ai, spell)
		{
            range = ai->GetRange("spell");
		}

        virtual string GetTargetName() { return "self target"; }
	};

	class CastEnchantItemAction : public CastSpellAction
	{
	public:
	    CastEnchantItemAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell)
		{
            range = ai->GetRange("spell");
		}

        virtual bool isPossible();
        virtual string GetTargetName() { return "self target"; }
	};

    //---------------------------------------------------------------------------------------------------------------------

    class CastHealingSpellAction : public CastAuraSpellAction
    {
    public:
        CastHealingSpellAction(PlayerbotAI* ai, string spell, uint8 estAmount = 15.0f) : CastAuraSpellAction(ai, spell)
		{
            this->estAmount = estAmount;
            range = ai->GetRange("spell");
        }
		virtual string GetTargetName() { return "self target"; }
        virtual bool isUseful();
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_AOE; }

    protected:
        uint8 estAmount;
    };

    class CastAoeHealSpellAction : public CastHealingSpellAction
    {
    public:
    	CastAoeHealSpellAction(PlayerbotAI* ai, string spell, uint8 estAmount = 15.0f) : CastHealingSpellAction(ai, spell, estAmount) {}
		virtual string GetTargetName() { return "party member to heal"; }
        virtual bool isUseful();
    };

	class CastCureSpellAction : public CastSpellAction
	{
	public:
		CastCureSpellAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell)
		{
            range = ai->GetRange("spell");
		}

		virtual string GetTargetName() { return "self target"; }
	};

	class PartyMemberActionNameSupport {
	public:
		PartyMemberActionNameSupport(string spell)
		{
			name = string(spell) + " on party";
		}

		virtual string getName() { return name; }

	private:
		string name;
	};

    class HealPartyMemberAction : public CastHealingSpellAction, public PartyMemberActionNameSupport
    {
    public:
        HealPartyMemberAction(PlayerbotAI* ai, string spell, uint8 estAmount = 15.0f) :
			CastHealingSpellAction(ai, spell, estAmount), PartyMemberActionNameSupport(spell) {}

		virtual string GetTargetName() { return "party member to heal"; }
		virtual string getName() { return PartyMemberActionNameSupport::getName(); }
    };

	class ResurrectPartyMemberAction : public CastSpellAction
	{
	public:
		ResurrectPartyMemberAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell) {}

		virtual string GetTargetName() { return "party member to resurrect"; }
	};
    //---------------------------------------------------------------------------------------------------------------------

    class CurePartyMemberAction : public CastSpellAction, public PartyMemberActionNameSupport
    {
    public:
        CurePartyMemberAction(PlayerbotAI* ai, string spell, uint32 dispelType) :
			CastSpellAction(ai, spell), PartyMemberActionNameSupport(spell)
        {
            this->dispelType = dispelType;
        }

		virtual Value<Unit*>* GetTargetValue();
		virtual string getName() { return PartyMemberActionNameSupport::getName(); }

    protected:
        uint32 dispelType;
    };

    //---------------------------------------------------------------------------------------------------------------------

    class BuffOnPartyAction : public CastBuffSpellAction, public PartyMemberActionNameSupport
    {
    public:
        BuffOnPartyAction(PlayerbotAI* ai, string spell) :
			CastBuffSpellAction(ai, spell), PartyMemberActionNameSupport(spell) {}
    public:
		virtual Value<Unit*>* GetTargetValue();
		virtual string getName() { return PartyMemberActionNameSupport::getName(); }
    };

    //---------------------------------------------------------------------------------------------------------------------

    class CastShootAction : public CastSpellAction
    {
    public:
        CastShootAction(PlayerbotAI* ai) : CastSpellAction(ai, "shoot")
        {
            Item* const pItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
            if (pItem)
            {
                spell = "shoot";

                switch (pItem->GetProto()->SubClass)
                {
                case ITEM_SUBCLASS_WEAPON_GUN:
                    spell += " gun";
                    break;
                case ITEM_SUBCLASS_WEAPON_BOW:
                    spell += " bow";
                    break;
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    spell += " crossbow";
                    break;
                }
            }
        }
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_NONE; }
    };

    //heal

#ifndef MANGOSBOT_ZERO
    class CastGiftOfTheNaaruAction : public CastHealingSpellAction
    {
    public:
        CastGiftOfTheNaaruAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "gift of the naaru") {}
    };
#endif
    class CastCannibalizeAction : public CastHealingSpellAction
    {
    public:
        CastCannibalizeAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "cannibalize") {}
    };

    //buff

#ifndef MANGOSBOT_ZERO
    class CastManaTapAction : public CastBuffSpellAction
    {
    public:
        CastManaTapAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "mana tap") {}
    };

    class CastArcaneTorrentAction : public CastBuffSpellAction
    {
    public:
        CastArcaneTorrentAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "arcane torrent") {}
    };
#endif
    class CastShadowmeldAction : public CastBuffSpellAction
    {
    public:
        CastShadowmeldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "shadowmeld") {}
    };

    class CastBerserkingAction : public CastBuffSpellAction
    {
    public:
        CastBerserkingAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "berserking") {}
    };

    class CastBloodFuryAction : public CastBuffSpellAction
    {
    public:
        CastBloodFuryAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "blood fury") {}
    };

    class CastStoneformAction : public CastBuffSpellAction
    {
    public:
        CastStoneformAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "stoneform") {}
    };

    class CastPerceptionAction : public CastBuffSpellAction
    {
    public:
        CastPerceptionAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "perception") {}
    };

    //spells 

    class CastWarStompAction : public CastSpellAction
    {
    public:
        CastWarStompAction(PlayerbotAI* ai) : CastSpellAction(ai, "war stomp") {}
    };

    //cc breakers

    class CastWillOfTheForsakenAction : public CastSpellAction
    {
    public:
        CastWillOfTheForsakenAction(PlayerbotAI* ai) : CastSpellAction(ai, "will of the forsaken") {}
    };

    class CastEscapeArtistAction : public CastSpellAction
    {
    public:
        CastEscapeArtistAction(PlayerbotAI* ai) : CastSpellAction(ai, "escape artist") {}
    };
#ifdef MANGOSBOT_TWO
    class CastEveryManforHimselfAction : public CastSpellAction
    {
    public:
        CastEveryManforHimselfAction(PlayerbotAI* ai) : CastSpellAction(ai, "every man for himself") {}
    };
#endif

    class CastSpellOnEnemyHealerAction : public CastSpellAction
    {
    public:
        CastSpellOnEnemyHealerAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell) {}
        Value<Unit*>* GetTargetValue()
        {
            return context->GetValue<Unit*>("enemy healer target", spell);
        }
        virtual string getName() { return spell + " on enemy healer"; }
    };

    class CastSnareSpellAction : public CastDebuffSpellAction
    {
    public:
        CastSnareSpellAction(PlayerbotAI* ai, string spell) : CastDebuffSpellAction(ai, spell) {}
        Value<Unit*>* GetTargetValue()
        {
            return context->GetValue<Unit*>("snare target", spell);
        }
        virtual string getName() { return spell + " on snare target"; }
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_NONE; }
    };

    class CastCrowdControlSpellAction : public CastBuffSpellAction
    {
    public:
        CastCrowdControlSpellAction(PlayerbotAI* ai, string spell) : CastBuffSpellAction(ai, spell) {}
        Value<Unit*>* GetTargetValue()
        {
            return context->GetValue<Unit*>("cc target", getName());
        }
        virtual bool Execute(Event event) { return ai->CastSpell(getName(), GetTarget()); }
        virtual bool isPossible() { return ai->CanCastSpell(getName(), GetTarget(), true); }
        virtual bool isUseful() { return true; }
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_NONE; }
    };

    class CastProtectSpellAction : public CastSpellAction
    {
    public:
        CastProtectSpellAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell) {}
        virtual string GetTargetName() { return "party member to protect"; }
        virtual bool isUseful()
        {
            return GetTarget() && !ai->HasAura(spell, GetTarget());
        }
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_NONE; }
    };

    //--------------------//
    //   Vehicle Actions  //
    //--------------------//

    class CastVehicleSpellAction : public CastSpellAction
    {
    public:
        CastVehicleSpellAction(PlayerbotAI* ai, string spell) : CastSpellAction(ai, spell)
        {
            range = 120.0f;
        }
        virtual string GetTargetName() { return "current target"; }
        virtual bool Execute(Event event);
        virtual bool isUseful();
        virtual bool isPossible();
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_NONE; }
    protected:
        WorldObject* spellTarget;
    };

    class CastHurlBoulderAction : public CastVehicleSpellAction
    {
    public:
        CastHurlBoulderAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "hurl boulder") {}
    };

    class CastSteamRushAction : public CastVehicleSpellAction
    {
    public:
        CastSteamRushAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "steam rush") {}
    };

    class CastRamAction : public CastVehicleSpellAction
    {
    public:
        CastRamAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "ram") {}
    };

    class CastNapalmAction : public CastVehicleSpellAction
    {
    public:
        CastNapalmAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "napalm") {}
    };

    class CastFireCannonAction : public CastVehicleSpellAction
    {
    public:
        CastFireCannonAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "fire cannon") {}
    };

    class CastSteamBlastAction : public CastVehicleSpellAction
    {
    public:
        CastSteamBlastAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "steam blast") {}
    };

    class CastIncendiaryRocketAction : public CastVehicleSpellAction
    {
    public:
        CastIncendiaryRocketAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "incendiary rocket") {}
    };

    class CastRocketBlastAction : public CastVehicleSpellAction
    {
    public:
        CastRocketBlastAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "rocket blast") {}
    };

    class CastGlaiveThrowAction : public CastVehicleSpellAction
    {
    public:
        CastGlaiveThrowAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "glaive throw") {}
    };

    class CastBladeSalvoAction : public CastVehicleSpellAction
    {
    public:
        CastBladeSalvoAction(PlayerbotAI* ai) : CastVehicleSpellAction(ai, "blade salvo") {}
    };
}
