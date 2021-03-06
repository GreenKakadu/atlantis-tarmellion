// START A3HEADER
//
// This source file is part of the Atlantis PBM game program.
// Copyright (C) 1995-1999 Geoff Dunbar
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program, in the file license.txt. If not, write
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// See the Atlantis Project web page for details:
// http://www.prankster.com/project
//
// END A3HEADER
#include "game.h"
#include <gamedata.h>

void Game::ProcessCastOrder(Unit * u,AString * o, OrdersCheck *pCheck) {
	int val;
	AString * token = o->gettoken();
	if (!token) {
		ParseError(pCheck, u, 0, "CAST: No skill given.");
		return;
	}

	int sk = ParseSkill(token);
	delete token;
	if (sk==-1) {
		ParseError(pCheck, u, 0, "CAST: Invalid skill.");
		return;
	}

	if (!(SkillDefs[sk].flags & SkillType::MAGIC)) {
		ParseError(pCheck, u, 0, "CAST: That is not a magic skill.");
		return;
	}
	if (!(SkillDefs[sk].flags & SkillType::CAST)) {
		ParseError(pCheck, u, 0, "CAST: That skill cannot be CAST.");
		return;
	}

	if (!pCheck) {
		//
		// XXX -- should be error checking spells
		//
		switch(sk) {
			case S_MIND_READING:
				ProcessMindReading(u,o, pCheck);
				break;
			case S_ENCHANT_RINGS:
			case S_ENCHANT_AMULETS:
			case S_CREATE_CHARMS:
			case S_ENCHANT_WEAPONS:
			case S_ENCHANT_ARMOR:
			case S_WOLF_LORE:
			case S_SUMMON_DRAGON:
			case S_SUMMON_WYRM:
			case S_CREATE_PHANTASMAL_BEASTS:
			case S_CREATE_PHANTASMAL_UNDEAD:
			case S_CREATE_PHANTASMAL_DEMONS:
			  ProcessItemSpell(u,o,sk,pCheck);
			  break;
			case S_CONSTRUCT_PORTAL:
			case S_CONSTRUCT_GATE:
			case S_ENGRAVE_RUNES_OF_WARDING:
			case S_SUMMON_IMPS:
			case S_SUMMON_DEMON:
			case S_SUMMON_BALROG:
			case S_SUMMON_SKELETONS:
			case S_RAISE_UNDEAD:
			case S_SUMMON_LICH:
			case S_EARTH_LORE:
			case S_CREATE_RING_OF_INVISIBILITY:
			case S_CREATE_CLOAK_OF_INVULNERABILITY:
			case S_CREATE_STAFF_OF_MIND:
			case S_CREATE_STAFF_OF_ICE:
			case S_CREATE_STAFF_OF_FIRE:
			case S_CREATE_STAFF_OF_LIGHTNING:
			case S_CREATE_AMULET_OF_TRUE_SEEING:
			case S_CREATE_AMULET_OF_PROTECTION:
			case S_CREATE_CORONET_OF_PROTECTION:
			case S_CREATE_RUNESWORD:
			case S_CREATE_SHIELDSTONE:
			case S_CREATE_MAGIC_CARPET:
			case S_CREATE_FLAMING_SWORD:
			case S_CREATE_ICE_SWORD:
			case S_CREATE_SOUL_EATER:
			case S_CREATE_FOOD:
				ProcessGenericSpell(u,sk, pCheck);
				break;
			case S_CLEAR_SKIES:
				val = SkillDefs[S_CLEAR_SKIES].rangeIndex;
				if (val != -1)
					ProcessRegionSpell(u, o, sk, pCheck);
				else
					ProcessGenericSpell(u, sk, pCheck);
				break;
			case S_FARSIGHT:
			case S_TELEPORTATION:
			case S_WEATHER_LORE:
				ProcessRegionSpell(u, o, sk, pCheck);
				break;
			case S_BIRD_LORE:
				ProcessBirdLore(u,o, pCheck);
				break;
			case S_INVISIBILITY:
				ProcessInvisibility(u,o, pCheck);
				break;
			case S_GATE_LORE:
				ProcessCastGateLore(u,o, pCheck);
				break;
			case S_PORTAL_LORE:
				ProcessCastPortalLore(u,o, pCheck);
				break;
//			case S_CREATE_PHANTASMAL_BEASTS:
//				ProcessPhanBeasts(u,o, pCheck);
//				break;
//			case S_CREATE_PHANTASMAL_UNDEAD:
//				ProcessPhanUndead(u,o, pCheck);
//				break;
//			case S_CREATE_PHANTASMAL_DEMONS:
//				ProcessPhanDemons(u,o, pCheck);
//				break;
		}
	}
}

void Game::ProcessItemSpell(Unit *u, AString *o, int skill, OrdersCheck *pCheck) {
	AString *token = o->gettoken();
	if (!token) {
		u->Error("CAST: Must specify item.");
		return;
	}
	int item = ParseEnabledItem(token);
	delete token;

	// handle phantasmal creatures
	if( skill == S_CREATE_PHANTASMAL_BEASTS ||
		skill == S_CREATE_PHANTASMAL_UNDEAD ||
		skill == S_CREATE_PHANTASMAL_DEMONS )
	{
		if( ItemDefs[item].type & IT_MONSTER ) {
			if( ItemDefs[item].index != MONSTER_ILLUSION ) {
				token = new AString( "i" );
				*token += ItemDefs[item].abr;
				item = ParseEnabledItem(token);
				delete token;
			}
		}
	}

	if (item == -1) {
		u->Error("CAST: Invalid item specified.");
		return;
	}

	// This should never happen
	if (ItemDefs[item].mSkill != skill) {
		u->Error("CAST: Does not posses required skill.");
	}

	int num = 1;
	token = o->gettoken();

	if (token) {
		num = token->value();
		delete token;
	}
	
	CastItemOrder *order = new CastItemOrder;
	order->item = item;
	order->spell = skill;
	order->level = ItemDefs[item].mLevel;
	order->num = num;

	u->ClearCastOrders();
	u->castorders = order;
}

void Game::ProcessMindReading(Unit *u,AString *o, OrdersCheck *pCheck) {
	UnitId *id = ParseUnit(o);

	if (!id) {
		u->Error("CAST: No unit specified.");
		return;
	}

	CastMindOrder *order = new CastMindOrder;
	order->id = id;
	order->spell = S_MIND_READING;
	order->level = 1;

	u->ClearCastOrders();
	u->castorders = order;
}

void Game::ProcessBirdLore(Unit *u,AString *o, OrdersCheck *pCheck) {
	AString *token = o->gettoken();

	if (!token) {
		u->Error("CAST: Missing arguments.");
		return;
	}

	if (*token == "eagle") {
		CastIntOrder *order = new CastIntOrder;
		order->spell = S_BIRD_LORE;
		order->level = 3;
		u->ClearCastOrders();
		u->castorders = order;
		return;
	}

	if (*token == "direction") {
		delete token;
		token = o->gettoken();

		if (!token) {
			u->Error("CAST: Missing arguments.");
			return;
		}

		int dir = ParseDir(token);
		delete token;
		if (dir == -1 || dir > NDIRS) {
			u->Error("CAST: Invalid direction.");
			return;
		}

		CastIntOrder *order = new CastIntOrder;
		order->spell = S_BIRD_LORE;
		order->level = 1;
		order->target = dir;
		u->ClearCastOrders();
		u->castorders = order;

		return;
	}

	u->Error("CAST: Invalid arguments.");
	delete token;
}

void Game::ProcessInvisibility(Unit *u,AString *o, OrdersCheck *pCheck) {
	AString *token = o->gettoken();

	if (!token || !(*token == "units")) {
		u->Error("CAST: Must specify units to render invisible.");
		return;
	}
	delete token;

	CastUnitsOrder *order;
	if (u->castorders && u->castorders->type == O_CAST &&
		((CastOrder *) u->castorders)->spell == S_INVISIBILITY &&
		((CastOrder *) u->castorders)->level == 1) {
		order = (CastUnitsOrder *) u->castorders;
	} else {
		order = new CastUnitsOrder;
		order->spell = S_INVISIBILITY;
		order->level = 1;
		u->ClearCastOrders();
		u->castorders = order;
	}

	UnitId *id = ParseUnit(o);
	while (id) {
		order->units.Add(id);
		id = ParseUnit(o);
	}
}

void Game::ProcessPhanDemons(Unit *u,AString *o, OrdersCheck *pCheck) {
	CastIntOrder *order = new CastIntOrder;
	order->spell = S_CREATE_PHANTASMAL_DEMONS;
	order->level = 0;
	order->target = 1;

	AString *token = o->gettoken();

	if (!token) {
		u->Error("CAST: Illusion to summon must be given.");
		delete order;
		return;
	}

	if (*token == "imp" || *token == "imps")
		order->level = 1;

	if (*token == "demon" || *token == "demons")
		order->level = 3;

	if (*token == "balrog" || *token == "balrogs")
		order->level = 5;

	delete token;

	if (!order->level) {
		u->Error("CAST: Can't summon that illusion.");
		delete order;
		return;
	}

	token = o->gettoken();

	if (!token) {
		order->target = 1;
	} else {
		order->target = token->value();
		delete token;
	}

	u->ClearCastOrders();
	u->castorders = order;
}

void Game::ProcessPhanUndead(Unit *u,AString *o, OrdersCheck *pCheck) {
	CastIntOrder *order = new CastIntOrder;
	order->spell = S_CREATE_PHANTASMAL_UNDEAD;
	order->level = 0;
	order->target = 1;

	AString *token = o->gettoken();

	if (!token) {
		u->Error("CAST: Must specify which illusion to summon.");
		delete order;
		return;
	}

	if (*token == "skeleton" || *token == "skeletons")
		order->level = 1;

	if (*token == "undead")
		order->level = 3;

	if (*token == "lich" || *token == "liches")
		order->level = 5;

	delete token;

	if (!order->level) {
		u->Error("CAST: Must specify which illusion to summon.");
		delete order;
		return;
	}

	token = o->gettoken();

	if (token) {
		order->target = token->value();
		delete token;
	} else {
		order->target = 1;
	}

	u->ClearCastOrders();
	u->castorders = order;
}

void Game::ProcessPhanBeasts(Unit *u,AString *o, OrdersCheck *pCheck) {
	CastIntOrder *order = new CastIntOrder;
	order->spell = S_CREATE_PHANTASMAL_BEASTS;
	order->level = 0;
	order->target = 1;

	AString *token = o->gettoken();

	if (!token) {
		u->Error("CAST: Must specify which illusion to summon.");
		delete order;
		return;
	}

	if (*token == "wolf" || *token == "wolves")
		order->level = 1;
	if (*token == "eagle" || *token == "eagles")
		order->level = 3;
	if (*token == "dragon" || *token == "dragon")
		order->level = 5;

	delete token;
	if (!order->level) {
		delete order;
		u->Error("CAST: Must specify which illusion to summon.");
		return;
	}

	token = o->gettoken();
	if (token) {
		order->target = token->value();
		delete token;
	}

	u->ClearCastOrders();
	u->castorders = order;
}

void Game::ProcessGenericSpell(Unit *u,int spell, OrdersCheck *pCheck) {
	CastOrder *orders = new CastOrder;
	orders->spell = spell;
	orders->level = 1;
	u->ClearCastOrders();
	u->castorders = orders;
}

void Game::ProcessRegionSpell(Unit *u, AString *o, int spell, OrdersCheck *pCheck) {
	AString *token = o->gettoken();
	int x = -1;
	int y = -1;
	int z = -1;
	int val = SkillDefs[spell].rangeIndex;
	RangeType *range = NULL;
	if (val != -1) range = &RangeDefs[val];

	if (token) {
		if (*token == "region") {
			delete token;
			token = o->gettoken();
			if (!token) {
				u->Error("CAST: Region X coordinate not specified.");
				return;
			}
			x = token->value();
			delete token;

			token = o->gettoken();
			if (!token) {
				u->Error("CAST: Region Y coordinate not specified.");
				return;
			}
			y = token->value();
			delete token;

			if (range && (range->flags & RangeType::RNG_CROSS_LEVELS)) {
				token = o->gettoken();
				if (token) {
					z = token->value();
					delete token;
					if (z < 0 || (z >= Globals->UNDERWORLD_LEVELS +
								Globals->UNDERDEEP_LEVELS +
								Globals->ABYSS_LEVEL + 2)) {
						u->Error("CAST: Invalid Z coordinate specified.");
						return;
					}
				}
			}
		} else {
			delete token;
		}
	}
	if (x == -1) x = u->object->region->xloc;
	if (y == -1) y = u->object->region->yloc;
	if (z == -1) z = u->object->region->zloc;

	CastRegionOrder *order;
	if (spell == S_TELEPORTATION)
		order = new TeleportOrder;
	else
		order = new CastRegionOrder;
	order->spell = spell;
	order->level = 1;
	order->xloc = x;
	order->yloc = y;
	order->zloc = z;

	u->ClearCastOrders();
	/* Teleports happen late in the turn! */
	if (spell == S_TELEPORTATION)
		u->teleportorders = (TeleportOrder *)order;
	else
		u->castorders = order;
}

void Game::ProcessCastPortalLore(Unit *u,AString *o, OrdersCheck *pCheck) {
	AString *token = o->gettoken();
	if (!token) {
		u->Error("CAST: Requires a target mage.");
		return;
	}
	int gate = token->value();
	delete token;
	token = o->gettoken();

	if (!token) {
		u->Error("CAST: No units to teleport.");
		return;
	}

	if (!(*token == "units")) {
		u->Error("CAST: No units to teleport.");
		delete token;
		return;
	}

	TeleportOrder *order;

	if (u->teleportorders && u->teleportorders->spell == S_PORTAL_LORE) {
		order = u->teleportorders;
	} else {
		order = new TeleportOrder;
		u->ClearCastOrders();
		u->teleportorders = order;
	}

	order->gate = gate;
	order->spell = S_PORTAL_LORE;
	order->level = 1;

	UnitId *id = ParseUnit(o);
	while(id) {
		order->units.Add(id);
		id = ParseUnit(o);
	}
}

void Game::ProcessCastGateLore(Unit *u,AString *o, OrdersCheck *pCheck) {
	AString *token = o->gettoken();

	if (!token) {
		u->Error("CAST: Missing argument.");
		return;
	}

	if ((*token) == "gate") {
		delete token;
		token = o->gettoken();

		if (!token) {
			u->Error("CAST: Requires a target gate.");
			return;
		}

		TeleportOrder *order;

		if (u->teleportorders && u->teleportorders->spell == S_GATE_LORE &&
				u->teleportorders->gate == token->value()) {
			order = u->teleportorders;
		} else {
			order = new TeleportOrder;
			u->ClearCastOrders();
			u->teleportorders = order;
		}

		order->gate = token->value();
		order->spell = S_GATE_LORE;
		order->level = 3;

		delete token;

		token = o->gettoken();

		if (!token) return;
		if (!(*token == "units")) {
			delete token;
			return;
		}

		UnitId *id = ParseUnit(o);
		while(id) {
			order->units.Add(id);
			id = ParseUnit(o);
		}
		return;
	}

	if ((*token) == "random") {
		TeleportOrder *order;

		if (u->teleportorders && u->teleportorders->spell == S_GATE_LORE &&
				u->teleportorders->gate == -1) {
			order = u->teleportorders;
		} else {
			order = new TeleportOrder;
			u->ClearCastOrders();
			u->teleportorders = order;
		}

		order->gate = -1;
		order->spell = S_GATE_LORE;
		order->level = 1;

		delete token;

		token = o->gettoken();

		if (!token) return;
		if (!(*token == "units")) {
			delete token;
			return;
		}

		UnitId *id = ParseUnit(o);
		while(id) {
			order->units.Add(id);
			id = ParseUnit(o);
		}
		return;
	}

	if ((*token) == "detect") {
		delete token;
		u->ClearCastOrders();
		CastOrder *to = new CastOrder;
		to->spell = S_GATE_LORE;
		to->level = 2;
		u->castorders = to;
		return;
	}

	delete token;
	u->Error("CAST: Invalid argument.");
}

void Game::RunACastOrder(ARegion * r,Object *o,Unit * u) {
	if (u->type != U_MAGE) {
		u->Error("CAST: Unit is not a mage.");
		return;
	}

	if (u->castorders->level == 0) {
		u->castorders->level = u->GetSkill(u->castorders->spell);
	}

	if (u->GetSkill(u->castorders->spell) < u->castorders->level ||
			u->castorders->level == 0) {
		u->Error("CAST: Skill level isn't that high.");
		return;
	}

	int sk = u->castorders->spell;

	// Check for spell failure
	int failChance = GetSpellFailureChance( u, sk );
	if( getrandom( 100 ) < failChance ) {
		u->Event( AString("Attempts to cast ") + SkillDefs[sk].name + " but the spell fails." );
		return;
	}

	switch (sk) {
		case S_MIND_READING:
			RunMindReading(r,u);
			break;
		case S_ENCHANT_ARMOR:
			RunEnchantArmor(r,u);
			break;
		case S_ENCHANT_WEAPONS:
			RunEnchantSwords(r,u);
			break;
		case S_CONSTRUCT_GATE:
			RunConstructGate(r,u,sk);
			break;
		case S_ENGRAVE_RUNES_OF_WARDING:
			RunEngraveRunes(r,o,u);
			break;
		case S_ENCHANT_RINGS:
		case S_ENCHANT_AMULETS:
		case S_CREATE_CHARMS:
			RunCreateArtifact(r,u,sk,((CastItemOrder *)u->castorders)->item);
			break;
		case S_CONSTRUCT_PORTAL:
			RunCreateArtifact(r,u,sk,I_PORTAL);
			break;
		case S_CREATE_RING_OF_INVISIBILITY:
			RunCreateArtifact(r,u,sk,I_RINGOFI);
			break;
		case S_CREATE_CLOAK_OF_INVULNERABILITY:
			RunCreateArtifact(r,u,sk,I_CLOAKOFINVULNERABILITY);
			break;
		case S_CREATE_STAFF_OF_ICE:
			RunCreateArtifact(r,u,sk,I_STAFFOFICE);
			break;
		case S_CREATE_STAFF_OF_MIND:
			RunCreateArtifact(r,u,sk,I_STAFFOFMIND);
			break;
		case S_CREATE_STAFF_OF_FIRE:
			RunCreateArtifact(r,u,sk,I_STAFFOFF);
			break;
		case S_CREATE_STAFF_OF_LIGHTNING:
			RunCreateArtifact(r,u,sk,I_STAFFOFL);
			break;
		case S_CREATE_AMULET_OF_TRUE_SEEING:
			RunCreateArtifact(r,u,sk,I_AMULETOFTS);
			break;
		case S_CREATE_CORONET_OF_PROTECTION:
			RunCreateArtifact(r,u,sk,I_CORONETOFPROTECTION);
			break;
		case S_CREATE_AMULET_OF_PROTECTION:
			RunCreateArtifact(r,u,sk,I_AMULETOFP);
			break;
		case S_CREATE_RUNESWORD:
			RunCreateArtifact(r,u,sk,I_RUNESWORD);
			break;
		case S_CREATE_SHIELDSTONE:
			RunCreateArtifact(r,u,sk,I_SHIELDSTONE);
			break;
		case S_CREATE_MAGIC_CARPET:
			RunCreateArtifact(r,u,sk,I_MCARPET);
			break;
		case S_CREATE_ICE_SWORD:
			RunCreateArtifact(r,u,sk,I_ICESWORD);
			break;
		case S_CREATE_SOUL_EATER:
			RunCreateArtifact(r,u,sk,I_SOULEATER);
			break;
		case S_CREATE_FLAMING_SWORD:
			RunCreateArtifact(r,u,sk,I_FLAMINGSWORD);
			break;
		case S_SUMMON_IMPS:
			RunSummonImps(r,u);
			break;
		case S_SUMMON_DEMON:
			RunSummonDemon(r,u);
			break;
		case S_SUMMON_BALROG:
			RunSummonBalrog(r,u);
			break;
		case S_SUMMON_LICH:
			RunSummonLich(r,u);
			break;
		case S_RAISE_UNDEAD:
			RunRaiseUndead(r,u);
			break;
		case S_SUMMON_SKELETONS:
			RunSummonSkeletons(r,u);
			break;
		case S_SUMMON_DRAGON:
			RunDragonLore(r,u);
			break;
		case S_SUMMON_WYRM:
			RunDragonLore(r,u);
			break;
		case S_BIRD_LORE:
			RunBirdLore(r,u);
			break;
		case S_WOLF_LORE:
			RunWolfLore(r,u);
			break;
		case S_INVISIBILITY:
			RunInvisibility(r,u);
			break;
		case S_CREATE_PHANTASMAL_DEMONS:
			RunPhanDemons(r,u);
			break;
		case S_CREATE_PHANTASMAL_UNDEAD:
			RunPhanUndead(r,u);
			break;
		case S_CREATE_PHANTASMAL_BEASTS:
			RunPhanBeasts(r,u);
			break;
		case S_GATE_LORE:
			RunDetectGates(r,o,u);
			break;
		case S_FARSIGHT:
			RunFarsight(r,u);
			break;
		case S_EARTH_LORE:
			RunEarthLore(r,u);
			break;
		case S_WEATHER_LORE:
			RunWeatherLore(r, u);
			break;
		case S_CLEAR_SKIES:
			RunClearSkies(r,u);
			break;
		case S_CREATE_FOOD:
			RunCreateFood(r, u);
			break;
	}

}

int Game::GetRegionInRange(ARegion *r, ARegion *tar, Unit *u, int spell) {
	int level = u->GetSkill(spell);
	if (!level) {
		u->Error("CAST: You don't know that spell.");
		return 0;
	}

	int val = SkillDefs[spell].rangeIndex;
	if (val == -1) {
		u->Error("CAST: Spell is not castable at range.");
		return 0;
	}

	RangeType *range = &RangeDefs[val];

	int rtype = regions.GetRegionArray(r->zloc)->levelType;
	if ((rtype == ARegionArray::LEVEL_NEXUS) && !(range->flags & RangeType::RNG_NEXUS_SOURCE)) {
		u->Error("CAST: Spell does not work from the Nexus.");
		return 0;
	}

	if (!tar) {
		u->Error("CAST: No such region.");
		return 0;
	}

	rtype = regions.GetRegionArray(tar->zloc)->levelType;
	if ((rtype == ARegionArray::LEVEL_NEXUS) &&
			!(range->flags & RangeType::RNG_NEXUS_TARGET)) {
		u->Error("CAST: Spell does not work to the Nexus.");
		return 0;
	}

	if ((rtype != ARegionArray::LEVEL_SURFACE) &&
			(range->flags & RangeType::RNG_SURFACE_ONLY)) {
		u->Error("CAST: Spell can only target regions on the surface.");
		return 0;
	}
	if (!(range->flags&RangeType::RNG_CROSS_LEVELS) && (r->zloc != tar->zloc)) {
		u->Error("CAST: Spell is not able to work across levels.");
		return 0;
	}

	int maxdist;
	switch(range->rangeClass) {
		default:
		case RangeType::RNG_ABSOLUTE:
			maxdist = 1;
			break;
		case RangeType::RNG_LEVEL:
			maxdist = level;
			break;
		case RangeType::RNG_LEVEL2:
			maxdist = level * level;
			break;
		case RangeType::RNG_LEVEL3:
			maxdist = level * level * level;
			break;
	}
	maxdist *= range->rangeMult;

	int dist;
	if (range->flags & RangeType::RNG_CROSS_LEVELS)
		dist = regions.GetPlanarDistance(tar, r, range->crossLevelPenalty);
	else
		dist = regions.GetDistance(tar, r);
	if (dist > maxdist) {
		u->Error("CAST: Target region out of range.");
		return 0;
	}
	return 1;
}

void Game::RunMindReading(ARegion *r,Unit *u) {
	CastMindOrder *order = (CastMindOrder *) u->castorders;
	int level = u->GetSkill(S_MIND_READING);

	Unit *tar = r->GetUnitId(order->id,u->faction->num);
	if (!tar) {
		u->Error("No such unit.");
		return;
	}

	AString temp = AString("Casts Mind Reading: ") + *(tar->name) + ", " +
		*(tar->faction->name);
	u->Practise(S_MIND_READING);

	if (level < 4) {
		u->Event(temp + ".");
		return;
	}

	temp += tar->items.Report(2,5,0) + ". Skills: ";
	temp += tar->skills.Report(tar->GetMen()) + ".";

	u->Event(temp);
}

// void Game::RunSummonItem(ARegion *r, Unit *u)
// {
//   CastItemOrder *order = (CastItemOrder *) u->castorders;
//   int skill = order->spell;
//   int level = u->GetSkill(skill);
//   int item = order->item;

//   if (ItemDefs[item].summon == -1) {
//	 u->Error("CAST: not a valid item");
//	 return;
//   }
//   SummonType *st = &SummonDefs[summon];
//   if (st->mLevel > level) {
//	 u->Error("CAST: insufficent level to summon item.");
//	 return;
//   }

//   int max = ItemDefs[item].mOut * level;

//   for (int c=0; c<sizeof(st->mInput)/sizeof(Materials); c++) {
//	 int it = st->mInput[c].item;
//	 int amt = st->mInput[c].amt;
//	 if (it != -1) {
//	   if (u->items.GetNum(it)/amt > max) {
// 	max = u->items.GetNum(it)/amt;
//	   }
//	 }
//   }
//   u->items.SetNum(item, u->items.GetNum(item) + max);
//   for (int c=0; c<sizeof(st->mInput)/sizeof(Materials); c++) {
//	 int it = st->mInput[c].item;
//	 int amt = st->mInput[c].amt;
//	 u->items.SetNum(u->items.GetNum(it) - max * amt);
//   }
//   u->Event(AString("Enchants ") + ItemString(item,max) + ".");
//   u->Practise(skill);
//   if (st->notify != -1) {
//	 r->NotifySpell(u,st->notify, &regions);
//   }
// }

void Game::RunEnchantArmor(ARegion *r,Unit *u) {
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	int level = u->GetSkill(S_ENCHANT_ARMOR);
	int item = order->item;

	// First check skill and level
	if (ItemDefs[item].mSkill != S_ENCHANT_ARMOR) {
		u->Error("CAST: item can not be enchanted.");
		return;
	}
	if (ItemDefs[item].mLevel > level) {
		u->Error("CAST: insufficent level to enchant.");
		return;
	}

	int max = ItemDefs[item].mOut * level * level * u->GetMen();
	int num = 0;
	int count = 0;
	unsigned int c;
	int found;

	// Figure out how many components there are
	for(c=0; c<sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
		if (ItemDefs[item].mInput[c].item != -1) count++;
	}

	while(max) {
		int i, a;
		found = 0;
		// See if we have enough of all items
		for(c=0; c<sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
			i = ItemDefs[item].mInput[c].item;
			a = ItemDefs[item].mInput[c].amt;
			if (i != -1) {
				if (u->items.GetNum(i) >= a) found++;
			}
		}
		// We do not, break.
		if (found != count) break;

		// Decrement our inputs
		for(c=0; c<sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
			i = ItemDefs[item].mInput[c].item;
			a = ItemDefs[item].mInput[c].amt;
			if (i != -1) {
				u->items.SetNum(i, u->items.GetNum(i) - a);
			}
		}
		// We've made one.
		num++;
		max--;
	}

	u->items.SetNum(item,u->items.GetNum(item) + num);
	u->Event(AString("Enchants ") + ItemString(item,num) + ".");
	u->Practise(S_ENCHANT_ARMOR);
	r->NotifySpell(u,S_ARTIFACT_LORE, &regions);
}

void Game::RunEnchantSwords(ARegion *r,Unit *u) {
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	int level = u->GetSkill(S_ENCHANT_WEAPONS);
	int item = order->item;

	// First check skill and level
	if (ItemDefs[item].mSkill != S_ENCHANT_WEAPONS) {
		u->Error("CAST: item can not be enchanted.");
		return;
	}
	if (ItemDefs[item].mLevel > level) {
		u->Error("CAST: insufficent level to enchant.");
		return;
	}

	int max = ItemDefs[item].mOut * level * level * u->GetMen();
	int num = 0;
	int count = 0;
	unsigned int c;
	int found;

	// Figure out how many components there are
	for(c=0; c<sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
		if (ItemDefs[item].mInput[c].item != -1) count++;
	}

	while(max) {
		int i, a;
		found = 0;
		// See if we have enough of all items
		for(c=0; c<sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
			i = ItemDefs[item].mInput[c].item;
			a = ItemDefs[item].mInput[c].amt;
			if (i != -1) {
				if (u->items.GetNum(i) >= a) found++;
			}
		}
		// We do not, break.
		if (found != count) break;

		// Decrement our inputs
		for(c=0; c<sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
			i = ItemDefs[item].mInput[c].item;
			a = ItemDefs[item].mInput[c].amt;
			if (i != -1) {
				u->items.SetNum(i, u->items.GetNum(i) - a);
			}
		}
		// We've made one.
		num++;
		max--;
	}

	u->items.SetNum(item,u->items.GetNum(item) + num);
	u->Event(AString("Enchants ") + ItemString(item,num)  + ".");
	u->Practise(S_ENCHANT_WEAPONS);
	r->NotifySpell(u,S_ARTIFACT_LORE, &regions);
}

void Game::RunCreateFood(ARegion *r,Unit *u) {
	int level = u->GetSkill(S_CREATE_FOOD);

	if( Globals->TARMELLION_SUMMONING ) {
		// Don't need inputs for this spell for tarmellion
		int num = level * 10 * u->GetMen();

		u->items.SetNum(I_FOOD,u->items.GetNum(I_FOOD) + num);
		u->Event(AString("Creates ") + num + " food.");
		u->Practise(S_CREATE_FOOD);
		r->NotifySpell(u,S_EARTH_LORE, &regions);

	} else {

		int max = ItemDefs[I_FOOD].mOut * level * u->GetMen();
		int num = 0;
		int count = 0;
		unsigned int c;
		int found;

		// Figure out how many components there are
		for (c=0; c<sizeof(ItemDefs[I_FOOD].mInput)/sizeof(Materials); c++) {
			if (ItemDefs[I_FOOD].mInput[c].item != -1) count++;
		}

		while (max) {
			int i, a;
			found = 0;
			// See if we have enough of all items
			for(c=0; c<sizeof(ItemDefs[I_FOOD].mInput)/sizeof(Materials); c++) {
				i = ItemDefs[I_FOOD].mInput[c].item;
				a = ItemDefs[I_FOOD].mInput[c].amt;
				if (i != -1) {
					if (u->items.GetNum(i) >= a) found++;
				}
			}
			// We do not, break.
			if (found != count) break;

			// Decrement our inputs
			for(c=0; c<sizeof(ItemDefs[I_FOOD].mInput)/sizeof(Materials); c++) {
				i = ItemDefs[I_FOOD].mInput[c].item;
				a = ItemDefs[I_FOOD].mInput[c].amt;
				if (i != -1) {
					u->items.SetNum(i, u->items.GetNum(i) - a);
				}
			}
			// We've made one.
			num++;
			max--;
		}

		u->items.SetNum(I_FOOD,u->items.GetNum(I_FOOD) + num);
		u->Event(AString("Creates ") + num + " food.");
		u->Practise(S_CREATE_FOOD);
		r->NotifySpell(u,S_EARTH_LORE, &regions);
	}
}

void Game::RunConstructGate(ARegion *r,Unit *u, int spell) {
	if (TerrainDefs[r->type].similar_type == R_OCEAN) {
		u->Error("Gates may not be constructed at sea.");
		return;
	}

	if (r->gate) {
		u->Error("There is already a gate in that region.");
		return;
	}

	if (u->GetMoney() < 1000) {
		u->Error("Can't afford to construct a Gate.");
		return;
	}
	u->SetMoney(u->GetMoney() - 1000);

	int level = u->GetSkill(spell);
	int chance = level * 20;
	if (getrandom(100) >= chance) {
		u->Event("Attempts to construct a gate, but fails.");
		return;
	}

	u->Event(AString("Constructs a Gate in ")+r->ShortPrint(&regions)+".");
	u->Practise(S_CONSTRUCT_GATE);
	regions.numberofgates++;
	r->gate = regions.numberofgates;
	r->NotifySpell(u,S_ARTIFACT_LORE, &regions);
}

void Game::RunEngraveRunes(ARegion *r,Object *o,Unit *u) {
	if (!o->IsBuilding()) {
		u->Error("Runes of Warding may only be engraved on a building.");
		return;
	}

	if (o->incomplete > 0) {
		u->Error("Runes of Warding may only be engraved on a completed "
				"building.");
		return;
	}

	int level = u->GetSkill(S_ENGRAVE_RUNES_OF_WARDING);

	switch (level) {
		case 5:
			if (o->type == O_MAGICALFORTRESS) break;
		case 4:
			if (o->type == O_CITADEL) break;
		case 3:
			if (o->type == O_CASTLE) break;
		case 2:
			if (o->type == O_FORT) break;
			if (o->type == O_MAGICALTOWER) break;
		case 1:
			if (o->type == O_TOWER) break;
		default:
			u->Error("Not high enough level to engrave Runes of Warding on "
					"that building.");
			return;
	}

	if (u->GetMoney() < 600) {
		u->Error("Can't afford to engrave Runes of Warding.");
		return;
	}

	u->SetMoney(u->GetMoney() - 600);
	if (o->type == O_MAGICALFORTRESS) {
		o->runes = 5;
	} else if (o->type == O_MAGICALTOWER) {
		o->runes = 4;
	} else {
		o->runes = 3;
	}
	u->Event(AString("Engraves Runes of Warding on ") + *(o->name) + ".");
	u->Practise(S_ENGRAVE_RUNES_OF_WARDING);
	r->NotifySpell(u,S_ARTIFACT_LORE, &regions);
}

void Game::RunSummonBalrog(ARegion *r,Unit *u) {
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, I_BALROG, S_SUMMON_BALROG );
		if( success ) r->NotifySpell( u, S_DEMON_LORE, &regions );
	} else {
		if (u->items.GetNum(I_BALROG)) {
			u->Error("Can't control more than one balrog.");
			return;
		}

		int level = u->GetSkill(S_SUMMON_BALROG);

		int num = (level * 20 * u->GetMen() + getrandom(100)) / 100;

		u->items.SetNum(I_BALROG,u->items.GetNum(I_BALROG) + num);
		u->Event(AString("Summons ") + ItemString(I_BALROG,num) + ".");
		u->Practise(S_SUMMON_BALROG);
		r->NotifySpell(u,S_DEMON_LORE, &regions);
	}
}

void Game::RunSummonDemon(ARegion *r,Unit *u) {
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, I_DEMON, S_SUMMON_DEMON );
		if( success ) r->NotifySpell( u, S_DEMON_LORE, &regions );
	} else {
		u->items.SetNum(I_DEMON,u->items.GetNum(I_DEMON) + u->GetMen() );
		u->Event(AString("Summons ") + ItemString(I_DEMON,u->GetMen()) + ".");
		u->Practise(S_SUMMON_DEMON);
		r->NotifySpell(u,S_DEMON_LORE, &regions);
	}
}

void Game::RunSummonImps(ARegion *r,Unit *u) {
	int level = u->GetSkill(S_SUMMON_IMPS);

	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, I_IMP, S_SUMMON_IMPS );
		if( success ) r->NotifySpell( u, S_DEMON_LORE, &regions );
	} else {
		u->items.SetNum(I_IMP,u->items.GetNum(I_IMP) + level * u->GetMen());
		u->Event(AString("Summons ") + ItemString(I_IMP,level * u->GetMen()) + ".");
		u->Practise(S_SUMMON_IMPS);
		r->NotifySpell(u,S_DEMON_LORE, &regions);
	}
}

void Game::RunCreateArtifact(ARegion *r,Unit *u,int skill,int item) {
	int level = u->GetSkill(skill);
	unsigned int c;
	for(c = 0; c < sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
		if (ItemDefs[item].mInput[c].item == -1) continue;
		int amt = u->items.GetNum(ItemDefs[item].mInput[c].item);
		int cost = ItemDefs[item].mInput[c].amt;
		if (amt < cost) {
			u->Error(AString("Doesn't have sufficient ") +
					ItemDefs[ItemDefs[item].mInput[c].item].name +
					" to create that.");
			return;
		}
	}

	// Deduct the costs
	for(c = 0; c < sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
		if (ItemDefs[item].mInput[c].item == -1) continue;
		int amt = u->items.GetNum(ItemDefs[item].mInput[c].item);
		int cost = ItemDefs[item].mInput[c].amt;
		u->items.SetNum(ItemDefs[item].mInput[c].item, amt-cost);
	}

	int num;
	if( ItemDefs[item].flags & ItemType::CHANCEOUT )
		num = (level * ItemDefs[item].mOut + getrandom(100)) / 100;
	else
		num = ItemDefs[item].mOut * level;
// TODO multiple creation fo mages

	u->items.SetNum(item,u->items.GetNum(item) + num);
	u->Event(AString("Creates ") + ItemString(item,num) + ".");
	u->Practise(skill);
	r->NotifySpell(u,S_ARTIFACT_LORE, &regions);
}

void Game::RunSummonLich(ARegion *r,Unit *u) {
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, I_LICH, S_SUMMON_LICH );
		if( success ) r->NotifySpell( u, S_NECROMANCY, &regions );
	} else {
		int level = u->GetSkill(S_SUMMON_LICH);

		int num = ((2 * level * level * u->GetMen()) + getrandom(100))/100;

		u->items.SetNum(I_LICH,u->items.GetNum(I_LICH) + num);
		u->Event(AString("Summons ") + ItemString(I_LICH,num) + ".");
		u->Practise(S_SUMMON_LICH);
		r->NotifySpell(u,S_NECROMANCY, &regions);
	}
}

void Game::RunRaiseUndead(ARegion *r,Unit *u) {
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, I_UNDEAD, S_RAISE_UNDEAD );
		if( success ) r->NotifySpell( u, S_NECROMANCY, &regions );
	} else {
		int level = u->GetSkill(S_RAISE_UNDEAD);

		int num = ((10 * level * level * u->GetMen()) + getrandom(100))/100;

		u->items.SetNum(I_UNDEAD,u->items.GetNum(I_UNDEAD) + num);
		u->Event(AString("Raises ") + ItemString(I_UNDEAD,num) + ".");
		u->Practise(S_RAISE_UNDEAD);
		r->NotifySpell(u,S_NECROMANCY, &regions);
	}
}

void Game::RunSummonSkeletons(ARegion *r,Unit *u) {
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, I_SKELETON, S_SUMMON_SKELETONS );
		if( success ) r->NotifySpell( u, S_NECROMANCY, &regions );
	} else {
		int level = u->GetSkill(S_SUMMON_SKELETONS);

		int num = ((40 * level * level * u->GetMen()) + getrandom(100))/100;

		u->items.SetNum(I_SKELETON,u->items.GetNum(I_SKELETON) + num);
		u->Event(AString("Summons ") + ItemString(I_SKELETON,num) + ".");
		u->Practise(S_SUMMON_SKELETONS);
		r->NotifySpell(u,S_NECROMANCY, &regions);
	}
}

/*
void Game::RunDragonLore(ARegion *r, Unit *u) {
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	int item = order->item;
	int skill = order->spell;
	int level = u->GetSkill(skill);

	// First check skill and level
	if (ItemDefs[item].mSkill != skill) {
		u->Error("CAST: item can not be summoned.");
		return;
	}
	if (ItemDefs[item].mLevel > level) {
		u->Error("CAST: insufficent level to summon.");
		return;
	}

	// Handle all dragon summoning stuff here rather than adding more
	// variables in the item definition
	int num = u->items.GetNum(item);
	int controlLevel = 0;
	if( skill == S_SUMMON_DRAGON )
		controlLevel = u->GetSkill(S_DRAGON_LORE);
	if( skill == S_SUMMON_WYRM )
		controlLevel = u->GetSkill(S_DRAGON_WYRM);

	int numAllowed = 0;
	switch( item ) {
		case I_STEELDRAGON:
			num += u->items.GetNum(I_STEELDRAGON_S);
			num += u->items.GetNum(I_STEELDRAGON_O);
			num += u->items.GetNum(I_STEELDRAGON_A);
			num += u->items.GetNum(I_STEELDRAGON_B);
			numAllowed = controlLevel;
			break;
		case I_COPPERDRAGON:
			num += u->items.GetNum(I_COPPERDRAGON_S);
			num += u->items.GetNum(I_COPPERDRAGON_O);
			num += u->items.GetNum(I_COPPERDRAGON_A);
			num += u->items.GetNum(I_COPPERDRAGON_B);
			numAllowed = (controlLevel + 1) / 2;
			break;
		case I_BRONZEDRAGON:
			num += u->items.GetNum(I_BRONZEDRAGON_S);
			num += u->items.GetNum(I_BRONZEDRAGON_O);
			num += u->items.GetNum(I_BRONZEDRAGON_A);
			num += u->items.GetNum(I_BRONZEDRAGON_B);
			numAllowed = controlLevel / 2;
			break;
		case I_SILVERDRAGON:
			num += u->items.GetNum(I_SILVERDRAGON_S);
			num += u->items.GetNum(I_SILVERDRAGON_O);
			num += u->items.GetNum(I_SILVERDRAGON_A);
			num += u->items.GetNum(I_SILVERDRAGON_B);
			numAllowed = (controlLevel - 1) / 2;
			break;
		case I_GOLDENDRAGON:
			num += u->items.GetNum(I_GOLDENDRAGON_S);
			num += u->items.GetNum(I_GOLDENDRAGON_O);
			num += u->items.GetNum(I_GOLDENDRAGON_A);
			num += u->items.GetNum(I_GOLDENDRAGON_B);
			numAllowed = 1;
			break;
		case I_GREENDRAGON:
			num += u->items.GetNum(I_GREENDRAGON_S);
			num += u->items.GetNum(I_GREENDRAGON_O);
			num += u->items.GetNum(I_GREENDRAGON_A);
			num += u->items.GetNum(I_GREENDRAGON_B);
			numAllowed = controlLevel;
			break;
		case I_BROWNDRAGON:
			num += u->items.GetNum(I_BROWNDRAGON_S);
			num += u->items.GetNum(I_BROWNDRAGON_O);
			num += u->items.GetNum(I_BROWNDRAGON_A);
			num += u->items.GetNum(I_BROWNDRAGON_B);
			numAllowed = (controlLevel + 1) / 2;
			break;
		case I_BLUEDRAGON:
			num += u->items.GetNum(I_BLUEDRAGON_S);
			num += u->items.GetNum(I_BLUEDRAGON_O);
			num += u->items.GetNum(I_BLUEDRAGON_A);
			num += u->items.GetNum(I_BLUEDRAGON_B);
			numAllowed = (controlLevel - 1) / 2;
			break;
		case I_REDDRAGON:
			num += u->items.GetNum(I_REDDRAGON_S);
			num += u->items.GetNum(I_REDDRAGON_O);
			num += u->items.GetNum(I_REDDRAGON_A);
			num += u->items.GetNum(I_REDDRAGON_B);
			numAllowed = 1;
			break;
		case I_WHITEDRAGON:
			num += u->items.GetNum(I_WHITEDRAGON_S);
			num += u->items.GetNum(I_WHITEDRAGON_O);
			num += u->items.GetNum(I_WHITEDRAGON_A);
			num += u->items.GetNum(I_WHITEDRAGON_B);
			numAllowed = 1;
			break;
		case I_BLACKDRAGON:
			num += u->items.GetNum(I_BLACKDRAGON_S);
			num += u->items.GetNum(I_BLACKDRAGON_O);
			num += u->items.GetNum(I_BLACKDRAGON_A);
			num += u->items.GetNum(I_BLACKDRAGON_B);
			numAllowed = 1;
			break;
	}

	if (num >= numAllowed) {
		u->Error(AString("Mage may not summon more ")+ItemDefs[item].names+".");
		return;
	}

	int chance = level * level * 4;
	if (getrandom(100) < chance) {
		u->items.SetNum(item,num + 1);
		u->Event(AString("Summons a ")+ItemDefs[item].name+".");
	} else {
		u->Event("Attempts to summon a dragon, but fails.");
	}
	u->Practise(skill);
	r->NotifySpell(u,S_EARTH_LORE, &regions);
}
*/

void Game::RunDragonLore(ARegion *r, Unit *u) {
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	int item = order->item;
	int skill = order->spell;

	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, item, skill );
		if( success ) r->NotifySpell( u, S_EARTH_LORE, &regions );
	} else {

		int level = u->GetSkill(skill);

		// First check skill and level
		if (ItemDefs[item].mSkill != skill) {
			u->Error("CAST: item can not be summoned.");
			return;
		}
		if (ItemDefs[item].mLevel > level) {
			u->Error("CAST: insufficent level to summon.");
			return;
		}
		int num = u->items.GetNum(item);
		if (num >= level * u->GetMen()) {
			u->Error(AString("Mage may not summon more ")+ItemDefs[item].names+".");
			return;
		}

		int chance = level * level * 4;
		if (getrandom(100) < chance) {
			u->items.SetNum(item,num + 1);
			u->Event(AString("Summons a ")+ItemDefs[item].name+".");
		} else {
			u->Event("Attempts to summon a dragon, but fails.");
		}
		u->Practise(skill);
		r->NotifySpell(u,S_EARTH_LORE, &regions);
	}
}

void Game::RunBirdLore(ARegion *r,Unit *u) {
	CastIntOrder *order = (CastIntOrder *) u->castorders;
	int type = regions.GetRegionArray(r->zloc)->levelType;

	if (type != ARegionArray::LEVEL_SURFACE) {
		AString error = "CAST: Bird Lore may only be cast on the surface of ";
		error += Globals->WORLD_NAME;
		error += ".";
		u->Error(error.Str());
		return;
	}

	if (order->level < 3) {
		// Send Bird Spy
		int dir = order->target;
		ARegion *tar = r->neighbors[dir];
		if (!tar) {
			u->Error("CAST: No such region.");
			return;
		}

		Farsight *f = new Farsight;
		f->faction = u->faction;
		f->level = u->GetSkill(S_BIRD_LORE);
		tar->farsees.Add(f);
		u->Event(AString("Sends birds to spy on ") +
				tar->Print(&regions) + ".");
		u->Practise(S_BIRD_LORE);
		r->NotifySpell(u,S_EARTH_LORE, &regions);
		return;
	} else {
		// Summon eagle
		if( Globals->TARMELLION_SUMMONING ) {
			int success = RunSummonCreature( u, I_EAGLE, S_BIRD_LORE );
			if( success ) r->NotifySpell( u, S_EARTH_LORE, &regions );
		} else {

			int level = u->GetSkill(S_BIRD_LORE);
			int num = u->items.GetNum(I_EAGLE);
			int max = level * level * u->GetMen() - num;

			if (max <= 0) {
				u->Error("Mage may not summon more eagles.");
				return;
			}

			int summon = (level + 1) / 2;
			if (summon > max) summon = max;

			u->Event(AString("Casts Bird Lore, summoning ") +
					ItemString(I_EAGLE,summon) + ".");
			u->items.SetNum(I_EAGLE,num + summon);
			u->Practise(S_BIRD_LORE);
			r->NotifySpell(u,S_EARTH_LORE, &regions);
		}
	}
}

void Game::RunWolfLore(ARegion *r,Unit *u) {
	//deactivated for balancing reasons
	//if (TerrainDefs[r->type].similar_type != R_MOUNTAIN &&
	//	TerrainDefs[r->type].similar_type != R_TUNDRA && 
	//	TerrainDefs[r->type].similar_type != R_FOREST) {
	//	u->Error("CAST: Can only summon wolves in hill, mountain, "
	//							 "tundra and forest regions.");
	//	return;
	//}
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	int item = order->item;
	if( Globals->TARMELLION_SUMMONING ) {
	    int success = RunSummonCreature( u, item, S_WOLF_LORE );
	    if ( success ) r->NotifySpell( u, S_EARTH_LORE, &regions );
	} else {
		int level = u->GetSkill(S_WOLF_LORE);
		int num = u->items.GetNum(I_WOLF);
		int max = level * level * u->GetMen() - num;

		if (max <= 0) {
			u->Error("Mage may not summon more wolves.");
			return;
		}

		int summon = (level + 1) / 2;
		if (summon > max) summon = max;

		u->Event(AString("Casts Wolf Lore, summoning ") +
				ItemString(I_WOLF,summon) + ".");
		u->items.SetNum(I_WOLF,num + summon);
		u->Practise(S_WOLF_LORE);
		r->NotifySpell(u,S_EARTH_LORE, &regions);
	}
}

void Game::RunInvisibility(ARegion *r,Unit *u) {
	CastUnitsOrder *order = (CastUnitsOrder *) u->castorders;
	int max = u->GetSkill(S_INVISIBILITY);
	max = max * max * u->GetMen();

	int num = 0;
	forlist (&(order->units)) {
		Unit *tar = r->GetUnitId((UnitId *) elem,u->faction->num);
		if (!tar) continue;
		if (tar->GetAttitude(r,u) < A_FRIENDLY) continue;
		num += tar->GetSoldiers();
	}

	if (num > max) {
		u->Error("CAST: Can't render that many men invisible.");
		return;
	}

	if (!num) {
		u->Error("CAST: No valid targets to turn invisible.");
		return;
	}
	{
		forlist (&(order->units)) {
			Unit *tar = r->GetUnitId((UnitId *) elem,u->faction->num);
			if (!tar) continue;
			if (tar->GetAttitude(r,u) < A_FRIENDLY) continue;
			tar->SetFlag(FLAG_INVIS,1);
			tar->Event(AString("Is rendered invisible by ") +
					*(u->name) + ".");
		}
	}

	r->NotifySpell(u,S_ILLUSION, &regions);
	u->Practise(S_INVISIBILITY);
	u->Event("Casts invisibility.");
}

void Game::RunPhanDemons(ARegion *r,Unit *u) {
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, order->item, S_CREATE_PHANTASMAL_DEMONS );
		if( success ) r->NotifySpell( u, S_ILLUSION, &regions );
	} else {
		int level = u->GetSkill(S_CREATE_PHANTASMAL_DEMONS);
		int create = order->num;
		if( create <= 0 ) create = 1;
		int max;

		if (order->item == I_IIMP) {
			max = level * level * 4;
		} else {
			if (order->item == I_IDEMON) {
				max = (level - 2) * (level - 2);
			} else {
				max = 1;
			}
		}

		max *= u->GetMen();
		max -= u->items.GetNum(order->item);
		if( max <= 0 || create > max ) {
			u->Error("CAST: Can't create that many Phantasmal Demons.");
			return;
		}

		if( create > max ) create = max;
		
		u->items.SetNum(order->item, create);
		u->Event("Casts Create Phantasmal Demons.");
		u->Practise(S_CREATE_PHANTASMAL_DEMONS);
		r->NotifySpell(u,S_ILLUSION, &regions);
	}
}

void Game::RunPhanUndead(ARegion *r,Unit *u) {
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, order->item, S_CREATE_PHANTASMAL_UNDEAD );
		if( success ) r->NotifySpell( u, S_ILLUSION, &regions );
	} else {
		int level = u->GetSkill(S_CREATE_PHANTASMAL_UNDEAD);
		int create = order->num;
		if( create <= 0 ) create = 1;
		int max;

		if (order->item == I_ISKELETON) {
			max = level * level * 4;
		} else {
			if (order->item == I_IUNDEAD) {
				max = (level - 2) * (level - 2);
			} else {
				max = 1;
			}
		}

		max *= u->GetMen();
		max -= u->items.GetNum(order->item);
		if( max <= 0 || create > max ) {
			u->Error("CAST: Can't create that many Phantasmal Undead.");
			return;
		}

		if( create > max ) create = max;
		
		u->items.SetNum(order->item, create);
		u->Event("Casts Create Phantasmal Undead.");
		u->Practise(S_CREATE_PHANTASMAL_UNDEAD);
		r->NotifySpell(u,S_ILLUSION, &regions);
	}
}

void Game::RunPhanBeasts(ARegion *r,Unit *u) {
	CastItemOrder *order = (CastItemOrder *) u->castorders;
	if( Globals->TARMELLION_SUMMONING ) {
		int success = RunSummonCreature( u, order->item, S_CREATE_PHANTASMAL_BEASTS );
		if( success ) r->NotifySpell( u, S_ILLUSION, &regions );
	} else {
		int level = u->GetSkill(S_CREATE_PHANTASMAL_BEASTS);
		int create = order->num;
		if( create <= 0 ) create = 1;
		int max;

		if (order->item == I_IWOLF) {
			max = level * level * 4;
		} else {
			if (order->item == I_IEAGLE) {
				max = (level - 2) * (level - 2);
			} else {
				max = 1;
			}
		}

		max *= u->GetMen();
		max -= u->items.GetNum(order->item);
		if( max <= 0 || create > max ) {
			u->Error("CAST: Can't create that many Phantasmal Beasts.");
			return;
		}

		if( create > max ) create = max;
		
		u->items.SetNum(order->item, create);
		u->Event("Casts Create Phantasmal Beasts.");
		u->Practise(S_CREATE_PHANTASMAL_BEASTS);
		r->NotifySpell(u,S_ILLUSION, &regions);
	}
}

void Game::RunEarthLore(ARegion *r,Unit *u) {
	int level = u->GetSkill(S_EARTH_LORE);

	if (level > r->earthlore) r->earthlore = level;
	int amt = r->Wages() * level * 2 * u->GetMen();

	u->items.SetNum(I_SILVER,u->items.GetNum(I_SILVER) + amt);
	u->Event(AString("Casts Earth Lore, raising ") + amt + " silver.");
	u->Practise(S_EARTH_LORE);
	r->NotifySpell(u,S_EARTH_LORE, &regions);
}

void Game::RunClearSkies(ARegion *r, Unit *u) {
	ARegion *tar = r;
	AString temp = "Casts Clear Skies";
	int val;

	CastRegionOrder *order = (CastRegionOrder *)u->castorders;

	val = SkillDefs[S_CLEAR_SKIES].rangeIndex;
	if (val != -1) {
		tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
		val = GetRegionInRange(r, tar, u, S_CLEAR_SKIES);
		if (!val) return;
		temp += " on ";
		temp += tar->ShortPrint(&regions);
	}
	temp += ", ";
	int level = u->GetSkill(S_CLEAR_SKIES);
	if (level > r->clearskies) r->clearskies = level;
	int amt = r->Wages() * level * 3 * u->GetMen();
	u->items.SetNum(I_SILVER,u->items.GetNum(I_SILVER) + amt);
	temp += "raising ";
	temp += amt;
	temp += " silver.";
	u->Event(temp);
	u->Practise(S_CLEAR_SKIES);
	r->NotifySpell(u,S_WEATHER_LORE, &regions);
}

void Game::RunWeatherLore(ARegion *r, Unit *u) {
	ARegion *tar;
	int val;

	CastRegionOrder *order = (CastRegionOrder *)u->castorders;

	tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
	val = GetRegionInRange(r, tar, u, S_WEATHER_LORE);
	if (!val) return;

	int level = u->GetSkill(S_WEATHER_LORE);
	int months = 3;
	if (level >= 5) months = 12;
	else if (level >= 3) months = 6;

	AString temp = "Casts Weather Lore on ";
	temp += tar->ShortPrint(&regions);
	temp += ". It will be ";
	int weather, futuremonth;
	for(int i = 0; i <= months; i++) {
		futuremonth = (month + i)%12;
		weather=regions.GetWeather(tar, futuremonth);
		temp += SeasonNames[weather];
		temp += " in ";
		temp += MonthNames[futuremonth];
		if (i < (months-1))
			temp += ", ";
		else if (i == (months-1))
			temp += " and ";
		else
			temp += ".";
	}
	u->Event(temp);
	u->Practise(S_WEATHER_LORE);
	r->NotifySpell(u, S_WEATHER_LORE, &regions);
}

void Game::RunFarsight(ARegion *r,Unit *u) {
	ARegion *tar;
	int val;

	CastRegionOrder *order = (CastRegionOrder *)u->castorders;

	tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
	val = GetRegionInRange(r, tar, u, S_FARSIGHT);
	if (!val) return;

	Farsight *f = new Farsight;
	f->faction = u->faction;
	f->level = u->GetSkill(S_FARSIGHT);
	f->unit = u;
	tar->farsees.Add(f);
	AString temp = "Casts Farsight on ";
	temp += tar->ShortPrint(&regions);
	temp += ".";
	u->Event(temp);
	u->Practise(S_FARSIGHT);
}

void Game::RunDetectGates(ARegion *r,Object *o,Unit *u) {
	int level = u->GetSkill(S_GATE_LORE);

	if (level == 1) {
		u->Error("CAST: Casting Gate Lore at level 1 has no effect.");
		return;
	}

	u->Event("Casts Gate Lore, detecting nearby Gates:");
	int found = 0;
	for (int i=0; i<NDIRS; i++) {
		ARegion *tar = r->neighbors[i];
		if (tar) {
			if (tar->gate) {
				if (Globals->DETECT_GATE_NUMBERS) {
					u->Event(tar->Print(&regions) + " contains Gate " +
							tar->gate + ".");
				} else {
					u->Event(tar->Print(&regions) + " contains a Gate.");
				}
				found = 1;
			}
		}
	}
	if (!found)
		u->Event("There are no nearby Gates.");
	u->Practise(S_GATE_LORE);
}

void Game::RunTeleport(ARegion *r,Object *o,Unit *u) {
	ARegion *tar;
	int val;

	CastRegionOrder *order = (CastRegionOrder *)u->teleportorders;

	tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
	val = GetRegionInRange(r, tar, u, S_TELEPORTATION);
	if (!val) return;

	int level = u->GetSkill(S_TELEPORTATION);
	int maxweight = level * 15 * u->GetMen();

	if (u->Weight() > maxweight) {
		u->Error("CAST: Can't carry that much when teleporting.");
		return;
	}

	// Presume they had to open the portal to see if target is ocean
	u->Practise(S_TELEPORTATION);

	if (TerrainDefs[tar->type].similar_type == R_OCEAN) {
		u->Error(AString("CAST: ") + tar->Print(&regions) + " is an ocean.");
		return;
	}

	u->Event(AString("Teleports to ") + tar->Print(&regions) + ".");
	u->MoveUnit(tar->GetDummy());
}

void Game::RunGateJump(ARegion *r,Object *o,Unit *u) {
	int level = u->GetSkill(S_GATE_LORE);
	int nexgate = 0;
	if (!level) {
		u->Error("CAST: Unit doesn't have that skill.");
		return;
	}

	TeleportOrder *order = u->teleportorders;

	if (order->gate != -1 && level < 3) {
		u->Error("CAST: Unit Doesn't know Gate Lore at that level.");
		return;
	}

	nexgate = Globals->NEXUS_GATE_OUT &&
		(TerrainDefs[r->type].similar_type == R_NEXUS);
	if (!r->gate && !nexgate) {
		u->Error("CAST: There is no gate in that region.");
		return;
	}

	int maxweight = 10;
	if (order->gate != -1) level -= 2;
	switch (level) {
		case 1:
			maxweight = 15;
			break;
		case 2:
			maxweight = 100;
			break;
		case 3:
		case 4:
		case 5:
			maxweight = 1000;
			break;
	}

	maxweight *= u->GetMen();
	int weight = u->Weight();

	forlist (&(order->units)) {
		Unit *taru = r->GetUnitId((UnitId *) elem,u->faction->num);
		if (taru && taru != u) weight += taru->Weight();
	}

	if (weight > maxweight) {
		u->Error("CAST: That mage cannot carry that much weight through "
				"a Gate.");
		return;
	}

	ARegion *tar;
	if (order->gate == -1) {
		int good = 0;
		int gatenum = getrandom(regions.numberofgates);
		tar = regions.FindGate(gatenum+1);

		if (tar && tar->zloc == r->zloc) good = 1;
		if (tar && nexgate && tar->zloc == ARegionArray::LEVEL_SURFACE)
			good = 1;

		while(!good) {
			gatenum = getrandom(regions.numberofgates);
			tar = regions.FindGate(gatenum+1);
			if (tar && tar->zloc == r->zloc) good = 1;
			if (tar && nexgate && tar->zloc == ARegionArray::LEVEL_SURFACE)
				good = 1;
		}

		u->Event("Casts Random Gate Jump.");
		u->Practise(S_GATE_LORE);
	} else {
		if (order->gate < 1 || order->gate > regions.numberofgates) {
			u->Error("CAST: No such target gate.");
			return;
		}

		tar = regions.FindGate(order->gate);
		if (!tar) {
			u->Error("CAST: No such target gate.");
			return;
		}

		u->Event("Casts Gate Jump.");
		u->Practise(S_GATE_LORE);
	}

	int comma = 0;
	AString unitlist; {
		forlist(&(order->units)) {
			Location *loc = r->GetLocation((UnitId *) elem,u->faction->num);
			if (loc) {
				/* Don't do the casting unit yet */
				if (loc->unit == u) {
					delete loc;
					continue;
				}

				if (loc->unit->GetAttitude(r,u) < A_ALLY) {
					u->Error("CAST: Unit is not allied.");
				} else {
					if (comma) {
						unitlist += AString(", ") + AString(loc->unit->num);
					} else {
						unitlist += AString(loc->unit->num);
						comma = 1;
					}

					loc->unit->Event(AString("Is teleported through a ") +
							"Gate to " + tar->Print(&regions) + " by " +
							*u->name + ".");
					loc->unit->MoveUnit(tar->GetDummy());
					if (loc->unit != u) loc->unit->ClearCastOrders();
				}
				delete loc;
			} else {
				u->Error("CAST: No such unit.");
			}
		}
	}

	u->Event(AString("Jumps through a Gate to ") +
			tar->Print(&regions) + ".");
	u->Practise(S_GATE_LORE);
	if (comma) {
		u->Event(unitlist + " follow through the Gate.");
	}
	u->MoveUnit(tar->GetDummy());
}

void Game::RunPortalLore(ARegion *r,Object *o,Unit *u) {
	int level = u->GetSkill(S_PORTAL_LORE);
	TeleportOrder *order = u->teleportorders;

	if (!level) {
		u->Error("CAST: Doesn't know Portal Lore.");
		return;
	}

	if (!u->items.GetNum(I_PORTAL)) {
		u->Error("CAST: Unit doesn't have a Portal.");
		return;
	}

	int maxweight = 50 * level * u->GetMen();
	int maxdist = 2 * level * level;
	int weight = 0;
	forlist (&(order->units)) {
		Unit *taru = r->GetUnitId((UnitId *) elem,u->faction->num);
		if (taru) weight += taru->Weight();
	}

	if (weight > maxweight) {
		u->Error("CAST: That mage cannot teleport that much weight through a "
				"Portal.");
		return;
	}

	Location *tar = regions.FindUnit(order->gate);
	if (!tar) {
		u->Error("CAST: No such target mage.");
		return;
	}

	if (tar->unit->faction->GetAttitude(u->faction->num) < A_FRIENDLY) {
		u->Error("CAST: Target mage is not friendly.");
		return;
	}

	if (tar->unit->type != U_MAGE) {
		u->Error("CAST: Target is not a mage.");
		return;
	}

	if (!tar->unit->items.GetNum(I_PORTAL)) {
		u->Error("CAST: Target does not have a Portal.");
		return;
	}

	if (regions.GetDistance(r,tar->region) > maxdist) {
		u->Error("CAST: Can't Portal Jump that far.");
		return;
	}

	u->Event("Casts Portal Jump.");
	u->Practise(S_PORTAL_LORE);

	{
		forlist(&(order->units)) {
			Location *loc = r->GetLocation((UnitId *) elem,u->faction->num);
			if (loc) {
				if (loc->unit->GetAttitude(r,u) < A_ALLY) {
					u->Error("CAST: Unit is not allied.");
				} else {
					loc->unit->Event(AString("Is teleported to ") +
							tar->region->Print(&regions) +
							" by " + *u->name + ".");
					loc->unit->MoveUnit(tar->obj);
					if (loc->unit != u) loc->unit->ClearCastOrders();
				}
				delete loc;
			} else {
				u->Error("CAST: No such unit.");
			}
		}
	}

	delete tar;
}

void Game::RunTeleportOrders() {
	forlist(&regions) {
		ARegion * r = (ARegion *) elem;
		forlist(&r->objects) {
			Object * o = (Object *) elem;
			int foundone = 1;
			while (foundone) {
				foundone = 0;
				forlist(&o->units) {
					Unit * u = (Unit *) elem;
					if (u->teleportorders) {
						foundone = 1;
						switch (u->teleportorders->spell) {
							case S_GATE_LORE:
								RunGateJump(r,o,u);
								break;
							case S_TELEPORTATION:
								RunTeleport(r,o,u);
								break;
							case S_PORTAL_LORE:
								RunPortalLore(r,o,u);
								break;
						}
						delete u->teleportorders;
						u->teleportorders = 0;
						break;
					}
				}
			}
		}
	}
}

/// Summon a creature
/// Put all tarmellion summoning stuff here rather than add more stuff to the item defs
int Game::RunSummonCreature(Unit *u, int creature, int skill)
{

//	// First check skill and level
//	if (ItemDefs[creature].mSkill != skill) {
//		u->Error("CAST: Cannot summon this creature.");
//		return 0;
//	}
//	if (ItemDefs[creature].mLevel > level) {
//		u->Error("CAST: Not skilled enough to summon this creature.");
//		return 0;
//	}


	// How many more can this unit summon?
	int max = GetAllowedMonsters( u, creature );
	if (max <= 0 ) {
		u->Error(AString("Mage may not summon more ")+ItemDefs[creature].names+".");
		return 0;
	}


	// Check how many of this creature we can summon at once
	int level = u->GetSkill( skill );
	int summoned;
	switch( creature ) {
		case I_WOLF:
		case I_EAGLE:
		case I_IMP:
			summoned = (level + 1) / 2;
			break;
		case I_SKELETON:
		case I_ISKELETON:
		case I_IIMP:
		case I_IWOLF:
			summoned = 2;
			break;
		default:
			summoned = 1;
	}

	summoned *= u->GetMen();
	if( summoned > max ) summoned = max;

	u->items.SetNum(creature, u->items.GetNum(creature) + summoned);
	u->Event(AString("Summons ") + ItemString( creature, summoned ) );

	u->Practise(skill);
	return 1;
//	r->NotifySpell(u,S_EARTH_LORE, &regions);
}

/// Returns the skill that is checked when determining how many monsters
/// of this type can be controlled
int Game::GetMonsterControlSkill( int monster )
{
	switch( monster ) {
		case I_IWOLF:
		case I_IEAGLE:
		case I_ISTEELDRAGON:
		case I_ICOPPERDRAGON:
		case I_IBRONZEDRAGON:
		case I_ISILVERDRAGON:
		case I_IGOLDENDRAGON:
		case I_IGREENDRAGON:
		case I_IBROWNDRAGON:
		case I_IBLUEDRAGON:
		case I_IREDDRAGON:
		case I_IWHITEDRAGON:
		case I_IBLACKDRAGON:
			return S_CREATE_PHANTASMAL_BEASTS;
		case I_IIMP:
		case I_IDEMON:
		case I_IBALROG:
			return S_CREATE_PHANTASMAL_DEMONS;
		case I_ISKELETON:
		case I_IUNDEAD:
		case I_ILICH:
			return S_CREATE_PHANTASMAL_UNDEAD;
		case I_STEELDRAGON:
		case I_COPPERDRAGON:
		case I_BRONZEDRAGON:
		case I_SILVERDRAGON:
		case I_GOLDENDRAGON:
			return S_DRAGON_LORE;
		case I_GREENDRAGON:
		case I_BROWNDRAGON:
		case I_BLUEDRAGON:
		case I_REDDRAGON:
		case I_WHITEDRAGON:
		case I_BLACKDRAGON:
			return S_WYRM_LORE;
		case I_WOLF:
	        case I_ICEWOLF:
			return S_WOLF_LORE;
		case I_EAGLE:
		case I_GIANTEAGLE:
			return S_BIRD_LORE;
		case I_IMP:
			return S_SUMMON_IMPS;
		case I_DEMON:
			return S_SUMMON_DEMON;
		case I_BALROG:
			return S_SUMMON_BALROG;
		case I_SKELETON:
			return S_SUMMON_SKELETONS;
		case I_UNDEAD:
			return S_RAISE_UNDEAD;
		case I_LICH:
			return S_SUMMON_LICH;
		default:
			return -1;
	}
}


/// Check how many monsters this unit is controlling.
/// Returns how many more monsters the unit is allowed of this type
int Game::GetAllowedMonsters(Unit *u, int creature )
{
	int skill = GetMonsterControlSkill( creature );
	int controlLevel = u->GetSkill(skill);

	// Check how many of this creature we're allowed, and how many we can summon.
	// Very creature dependant with ceran creatures disabled
	int num = 0;
	int numAllowed = 0;
	switch( creature ) {
		case I_STEELDRAGON:
		//case I_STEELDRAGON_S:
		//case I_STEELDRAGON_O:
		//case I_STEELDRAGON_A:
		//case I_STEELDRAGON_B:
			num += u->items.GetNum(I_STEELDRAGON);
			//num += u->items.GetNum(I_STEELDRAGON_S);
			//num += u->items.GetNum(I_STEELDRAGON_O);
			//num += u->items.GetNum(I_STEELDRAGON_A);
			//num += u->items.GetNum(I_STEELDRAGON_B);
			numAllowed = controlLevel;
			break;
		case I_COPPERDRAGON:
		//case I_COPPERDRAGON_S:
		//case I_COPPERDRAGON_O:
		//case I_COPPERDRAGON_A:
		//case I_COPPERDRAGON_B:
			num += u->items.GetNum(I_COPPERDRAGON);
			//num += u->items.GetNum(I_COPPERDRAGON_S);
			//num += u->items.GetNum(I_COPPERDRAGON_O);
			//num += u->items.GetNum(I_COPPERDRAGON_A);
			//num += u->items.GetNum(I_COPPERDRAGON_B);
			numAllowed = (controlLevel + 1) / 2;
			break;
		case I_BRONZEDRAGON:
		//case I_BRONZEDRAGON_S:
		//case I_BRONZEDRAGON_O:
		//case I_BRONZEDRAGON_A:
		//case I_BRONZEDRAGON_B:
			num += u->items.GetNum(I_BRONZEDRAGON);
			//num += u->items.GetNum(I_BRONZEDRAGON_S);
			//num += u->items.GetNum(I_BRONZEDRAGON_O);
			//num += u->items.GetNum(I_BRONZEDRAGON_A);
			//num += u->items.GetNum(I_BRONZEDRAGON_B);
			numAllowed = controlLevel / 2;
			break;
		case I_SILVERDRAGON:
		//case I_SILVERDRAGON_S:
		//case I_SILVERDRAGON_O:
		//case I_SILVERDRAGON_A:
		//case I_SILVERDRAGON_B:
			num += u->items.GetNum(I_SILVERDRAGON);
			//num += u->items.GetNum(I_SILVERDRAGON_S);
			//num += u->items.GetNum(I_SILVERDRAGON_O);
			//num += u->items.GetNum(I_SILVERDRAGON_A);
			//num += u->items.GetNum(I_SILVERDRAGON_B);
			numAllowed = (controlLevel - 1) / 2;
			break;
		case I_GOLDENDRAGON:
		//case I_GOLDENDRAGON_S:
		//case I_GOLDENDRAGON_O:
		//case I_GOLDENDRAGON_A:
		//case I_GOLDENDRAGON_B:
			num += u->items.GetNum(I_GOLDENDRAGON);
			//num += u->items.GetNum(I_GOLDENDRAGON_S);
			//num += u->items.GetNum(I_GOLDENDRAGON_O);
			//num += u->items.GetNum(I_GOLDENDRAGON_A);
			//num += u->items.GetNum(I_GOLDENDRAGON_B);
			numAllowed = 1;
			break;
		case I_GREENDRAGON:
		//case I_GREENDRAGON_S:
		//case I_GREENDRAGON_O:
		//case I_GREENDRAGON_A:
		//case I_GREENDRAGON_B:
			num += u->items.GetNum(I_GREENDRAGON);
			//num += u->items.GetNum(I_GREENDRAGON_S);
			//num += u->items.GetNum(I_GREENDRAGON_O);
			//num += u->items.GetNum(I_GREENDRAGON_A);
			//num += u->items.GetNum(I_GREENDRAGON_B);
			numAllowed = controlLevel;
			break;
		case I_BROWNDRAGON:
		//case I_BROWNDRAGON_S:
		//case I_BROWNDRAGON_O:
		//case I_BROWNDRAGON_A:
		//case I_BROWNDRAGON_B:
			num += u->items.GetNum(I_BROWNDRAGON);
			//num += u->items.GetNum(I_BROWNDRAGON_S);
			//num += u->items.GetNum(I_BROWNDRAGON_O);
			//num += u->items.GetNum(I_BROWNDRAGON_A);
			//num += u->items.GetNum(I_BROWNDRAGON_B);
			numAllowed = (controlLevel + 1) / 2;
			break;
		case I_BLUEDRAGON:
		//case I_BLUEDRAGON_S:
		//case I_BLUEDRAGON_O:
		//case I_BLUEDRAGON_A:
		//case I_BLUEDRAGON_B:
			num += u->items.GetNum(I_BLUEDRAGON);
			//num += u->items.GetNum(I_BLUEDRAGON_S);
			//num += u->items.GetNum(I_BLUEDRAGON_O);
			//num += u->items.GetNum(I_BLUEDRAGON_A);
			//num += u->items.GetNum(I_BLUEDRAGON_B);
			numAllowed = (controlLevel - 1) / 2;
			break;
		case I_REDDRAGON:
		//case I_REDDRAGON_S:
		//case I_REDDRAGON_O:
		//case I_REDDRAGON_A:
		//case I_REDDRAGON_B:
			num += u->items.GetNum(I_REDDRAGON);
			//num += u->items.GetNum(I_REDDRAGON_S);
			//num += u->items.GetNum(I_REDDRAGON_O);
			//num += u->items.GetNum(I_REDDRAGON_A);
			//num += u->items.GetNum(I_REDDRAGON_B);
			numAllowed = 1;
			break;
		case I_WHITEDRAGON:
		//case I_WHITEDRAGON_S:
		//case I_WHITEDRAGON_O:
		//case I_WHITEDRAGON_A:
		//case I_WHITEDRAGON_B:
			num += u->items.GetNum(I_WHITEDRAGON);
			//num += u->items.GetNum(I_WHITEDRAGON_S);
			//num += u->items.GetNum(I_WHITEDRAGON_O);
			//num += u->items.GetNum(I_WHITEDRAGON_A);
			//num += u->items.GetNum(I_WHITEDRAGON_B);
			numAllowed = 1;
			break;
		case I_BLACKDRAGON:
		//case I_BLACKDRAGON_S:
		//case I_BLACKDRAGON_O:
		//case I_BLACKDRAGON_A:
		//case I_BLACKDRAGON_B:
			num += u->items.GetNum(I_BLACKDRAGON);
			//num += u->items.GetNum(I_BLACKDRAGON_S);
			//num += u->items.GetNum(I_BLACKDRAGON_O);
			//num += u->items.GetNum(I_BLACKDRAGON_A);
			//num += u->items.GetNum(I_BLACKDRAGON_B);
			numAllowed = 1;
			break;
		case I_WOLF:
		case I_EAGLE:
		case I_IMP:
		case I_SKELETON:
		case I_IWOLF:
		case I_ISKELETON:
		case I_IIMP:
			num += u->items.GetNum(creature);
			numAllowed = controlLevel * controlLevel;
			break;
	        case I_ICEWOLF:
		case I_DEMON:
		case I_UNDEAD:
		case I_LICH:
		case I_IEAGLE:
		case I_IDEMON:
		case I_IUNDEAD:
			num += u->items.GetNum(creature);
			numAllowed = controlLevel;
			break;
		case I_BALROG:
		case I_IBALROG:
			num += u->items.GetNum(creature);
			numAllowed = 1;
			break;
		case I_ILICH:
			num += u->items.GetNum(creature);
			numAllowed = controlLevel * 2;
			break;
		case I_ISTEELDRAGON:
		case I_ICOPPERDRAGON:
		case I_IBRONZEDRAGON:
		case I_ISILVERDRAGON:
		case I_IGOLDENDRAGON:
		case I_IGREENDRAGON:
		case I_IBROWNDRAGON:
		case I_IBLUEDRAGON:
		case I_IREDDRAGON:
		case I_IWHITEDRAGON:
		case I_IBLACKDRAGON:
			num += u->items.GetNum( I_ISTEELDRAGON );
			num += u->items.GetNum( I_ICOPPERDRAGON );
			num += u->items.GetNum( I_IBRONZEDRAGON );
			num += u->items.GetNum( I_ISILVERDRAGON );
			num += u->items.GetNum( I_IGOLDENDRAGON );
			num += u->items.GetNum( I_IGREENDRAGON );
			num += u->items.GetNum( I_IBROWNDRAGON );
			num += u->items.GetNum( I_IBLUEDRAGON );
			num += u->items.GetNum( I_IREDDRAGON );
			num += u->items.GetNum( I_IWHITEDRAGON );
			num += u->items.GetNum( I_IBLACKDRAGON );
			numAllowed = controlLevel;
			break;


	}
	numAllowed *= u->GetMen();

	int max = numAllowed - num;
	return max;
}
