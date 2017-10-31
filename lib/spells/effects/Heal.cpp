/*
 * Heal.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Heal.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../CStack.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../serializer/JsonSerializeFormat.h"


static const std::string EFFECT_NAME = "core:heal";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Heal, EFFECT_NAME);

Heal::Heal(const int level)
	: StackEffect(level),
	healLevel(EHealLevel::HEAL),
	healPower(EHealPower::PERMANENT),
	minFullUnits(0)
{

}

Heal::~Heal() = default;

void Heal::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	auto hpGained = m->getEffectValue();
	BattleStacksChanged pack;

	for(auto & oneTarget : target)
	{
		const battle::Unit * unit = oneTarget.unitValue;

		if(unit)
		{
			auto unitHPgained = m->caster->getSpellBonus(m->owner, hpGained, unit);

			auto state = unit->asquire();
			state->heal(unitHPgained, healLevel, healPower);

			CStackStateInfo info;
			state->toInfo(info);

			info.stackId = unit->unitId();
			info.healthDelta = unitHPgained;
			if(unitHPgained > 0)
				pack.changedStacks.push_back(info);
		}
	}
	if(!pack.changedStacks.empty())
		server->sendAndApply(&pack);
}

void Heal::apply(IBattleState * battleState, const Mechanics * m, const EffectTarget & target) const
{
	auto hpGained = m->getEffectValue();

	for(auto & oneTarget : target)
	{
		const battle::Unit * unit = oneTarget.unitValue;

		if(unit)
		{
			auto unitHPgained = m->caster->getSpellBonus(m->owner, hpGained, unit);

			auto state = unit->asquire();
			state->heal(unitHPgained, healLevel, healPower);

			CStackStateInfo info;
			state->toInfo(info);

			info.stackId = unit->unitId();
			info.healthDelta = unitHPgained;
			if(unitHPgained > 0)
				battleState->updateUnit(info);
		}
	}
}

bool Heal::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	const bool onlyAlive = healLevel == EHealLevel::HEAL;
	const bool validInGenaral = unit->isValidTarget(!onlyAlive);

	if(!validInGenaral)
		return false;

	auto hpGained = m->getEffectValue();
	auto insuries = unit->getTotalHealth() - unit->getAvailableHealth();

	if(insuries == 0)
		return false;

	if(hpGained < minFullUnits * unit->unitMaxHealth())
		return false;

	if(unit->isDead())
	{
		//check if alive unit blocks resurrection
		for(const BattleHex & hex : CStack::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()))
		{
			auto blocking = m->cb->battleGetUnitsIf([hex, unit](const  battle::Unit * s)
			{
				return s->isValidTarget(true) && s->coversPos(hex) && s != unit;
			});

			if(!blocking.empty())
				return false;
		}
	}
	return true;
}

void Heal::serializeJsonEffect(JsonSerializeFormat & handler)
{
	static const std::vector<std::string> HEAL_LEVEL_MAP =
	{
		"heal",
		"resurrect",
		"overHeal"
	};

	static const std::vector<std::string> HEAL_POWER_MAP =
	{
		"oneBattle",
		"permanent"
	};

	handler.serializeEnum("healLevel", healLevel, EHealLevel::HEAL, HEAL_LEVEL_MAP);
	handler.serializeEnum("healPower", healPower, EHealPower::PERMANENT, HEAL_POWER_MAP);
	handler.serializeInt("minFullUnits", minFullUnits);
}


} // namespace effects
} // namespace spells
