/*
 * Damage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Damage.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../CStack.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"


static const std::string EFFECT_NAME = "core:damage";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Damage, EFFECT_NAME);

Damage::Damage(const int level)
	: StackEffect(level)
{
}

Damage::~Damage() = default;

void Damage::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	StacksInjured stacksInjured;
	int64_t damageToDisplay = 0;
	uint32_t killed = 0;

	const auto rawDamage = m->getEffectValue();

	const CStack * firstTarget = nullptr;

	for(auto & t : target)
	{
		auto s = m->cb->battleGetStackByID(t.unitValue->unitId());
		if(s && s->alive())
		{
			BattleStackAttacked bsa;
			bsa.damageAmount = m->owner->adjustRawDamage(m->caster, s, rawDamage);
			damageToDisplay += bsa.damageAmount;
			bsa.stackAttacked = s->unitId();
			bsa.attackerID = -1;
			s->prepareAttacked(bsa, rng);
			killed += bsa.killedAmount;

			stacksInjured.stacks.push_back(bsa);

			if(!firstTarget)
				firstTarget = s;
		}
	}

	if(!stacksInjured.stacks.empty())
	{
		{
			MetaString line;

			line.addTxt(MetaString::GENERAL_TXT, 376);
			line.addReplacement(MetaString::SPELL_NAME, m->getSpellIndex());
			line.addReplacement(damageToDisplay);

			stacksInjured.battleLog.push_back(line);
		}

		{
			MetaString line;
			const int textId = (killed > 1) ? 379 : 378;
			line.addTxt(MetaString::GENERAL_TXT, textId);
			const bool multiple = stacksInjured.stacks.size() > 1;

			if(killed > 1)
				line.addReplacement(killed);

            if(killed > 1)
				if(multiple || !firstTarget)
					line.addReplacement(MetaString::GENERAL_TXT, 43);
				else
					firstTarget->addNameReplacement(line, true);
			else
				if(multiple || !firstTarget)
					line.addReplacement(MetaString::GENERAL_TXT, 42);
				else
					firstTarget->addNameReplacement(line, false);

			stacksInjured.battleLog.push_back(line);
		}

		server->sendAndApply(&stacksInjured);
	}
}

void Damage::apply(IBattleState * battleState, const Mechanics * m, const EffectTarget & target) const
{
	const auto rawDamage = m->getEffectValue();

	for(auto & t : target)
	{
		const battle::Unit * unit = t.unitValue;
		if(unit && unit->alive())
		{
			auto amount = m->owner->adjustRawDamage(m->caster, unit, rawDamage);

			CStackStateInfo info;
			auto state = unit->asquire();
			state->damage(amount);
			state->toInfo(info);
			battleState->updateUnit(info);
		}
	}
}

void Damage::serializeJsonEffect(JsonSerializeFormat & handler)
{
	//TODO: Damage::serializeJsonEffect
}


} // namespace effects
} // namespace spells
