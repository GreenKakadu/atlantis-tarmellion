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
//
#include <gamedata.h>
#include "gamedefs.h"

//
// Define the various globals for this game.
//
// If you change any of these, it is incumbent on you, the GM to change
// the html file containing the rules to correctly reflect the changes!
//

static int am[] = { 0, 4, 8, 12, 16, 20, 24 };
int *allowedMages = am;
int allowedMagesSize = sizeof(am) / sizeof(am[0]);

static int aa[] = { 0, 3, 5, 7, 11, 15, 20 };
int *allowedApprentices = aa;
int allowedApprenticesSize = sizeof(aa) / sizeof(aa[0]);

static int aw[] = { 0, 5, 10, 15, 20, 25, 30 };
int *allowedTaxes = aw;
int allowedTaxesSize = sizeof(aw) / sizeof(aw[0]);

static int at[] = { 0, 6, 12, 18, 24, 30, 36 };
int *allowedTrades = at;
int allowedTradesSize = sizeof(at) / sizeof(at[0]);

static int uw[] = { 0, 0, 0 };
int *UnderworldCities = uw;
int UnderworldCityLevels = sizeof(uw) / sizeof(uw[0]);

static int ud[] = { 0, 0, 0 };
int *UnderdeepCities = ud;
int UnderdeepCityLevels = sizeof(ud) / sizeof(ud[0]);

static int su[] = { 4 };
int *SurfaceCities = su;
int SurfaceCityLevels = sizeof(su) / sizeof(su[0]);

static GameDefs g = {
	"Tarmellion Battleground",				// RULESET_NAME
	MAKE_ATL_VER( 1, 0, 1 ), // RULESET_VERSION

	2, /* FOOT_SPEED */
	4, /* HORSE_SPEED */
	4, /* SHIP_SPEED */
	4, /* FLY_SPEED */
	12, /* MAX_SPEED */

	10, /* STUDENTS_PER_TEACHER */
	10, /* MAINTENANCE_COST */
	20, /* LEADER_COST */

	0,  /* MAINTAINENCE_MULTIPLIER */
	GameDefs::MULT_NONE, /* MULTIPLIER_USE */

	33, /* STARVE_PERCENT */
	GameDefs::STARVE_NONE, /* SKILL_STARVATION */

	15000, /* START_MONEY */
	4, /* WORK_FRACTION */
	8, /* ENTERTAIN_FRACTION */
	20, /* ENTERTAIN_INCOME */

	50, /* TAX_INCOME */

	5, /* HEALS_PER_MAN */

	50, /* GUARD_REGEN */ /* percent */
	100, /* CITY_GUARD */ /*number of guards in village*/
	50, /* GUARD_MONEY */
	4000, /* CITY_POP */

	25, /* WMON_FREQUENCY */
	25, /* LAIR_FREQUENCY */

	6, /* FACTION_POINTS */

	60, /* TIMES_REWARD */

	1, // TOWNS_EXIST
	GameDefs::RACIAL_LEADERS, // LEADERS_EXIST
	1, // SKILL_LIMIT_NONLEADERS
	0, // MAGE_NONLEADERS
	1, // RACES_EXIST
	1, // GATES_EXIST
	1, // FOOD_ITEMS_EXIST
	1, // CITY_MONSTERS_EXIST
	1, // WANDERING_MONSTERS_EXIST
	1, // LAIR_MONSTERS_EXIST
	1, // WEATHER_EXISTS
	1, // OPEN_ENDED
	0, // NEXUS_EXISTS
	0, // CONQUEST_GAME

	1, // RANDOM_ECONOMY
	1, // VARIABLE_ECONOMY

	40, // CITY_MARKET_NORMAL_AMT
	15, // CITY_MARKET_ADVANCED_AMT
	8, // CITY_MARKET_TRADE_AMT
	1, // CITY_MARKET_MAGIC_AMT
	1,  // MORE_PROFITABLE_TRADE_GOODS

	50,	// BASE_MAN_COST
	0, // LASTORDERS_MAINTAINED_BY_SCRIPTS
	4, // MAX_INACTIVE_TURNS

	0, // EASIER_UNDERWORLD

	1, // DEFAULT_WORK_ORDER

	GameDefs::FACLIM_FACTION_TYPES, // FACTION_LIMIT_TYPE

	GameDefs::WFLIGHT_MUST_LAND,	// FLIGHT_OVER_WATER

	0,   // SAFE_START_CITIES
	2000, // AMT_START_CITY_GUARDS
	1,   // START_CITY_GUARDS_PLATE
	5,   // START_CITY_MAGES
	6,   // START_CITY_TACTICS
	0,   // APPRENTICES_EXIST

	"Tarmellion Battleground", // WORLD_NAME

	1,  // NEXUS_GATE_OUT
	0,  // NEXUS_IS_CITY
	1,	// BATTLE_FACTION_INFO
	1,	// ALLOW_WITHDRAW
	10,	// CITY_RENAME_COST
	0,	// TAX_PILLAGE_MONTH_LONG
	0,	// MULTI_HEX_NEXUS
	0,	// UNDERWORLD_LEVELS
	0,	// UNDERDEEP_LEVELS
	0,	// ABYSS_LEVEL
	100,	// TOWN_PROBABILITY
	0,	// TOWN_SPREAD
	1,	// TOWNS_NOT_ADJACENT
	0,	// LESS_ARCTIC_TOWNS
	0,	// ARCHIPELAGO
	0,	// LAKES_EXIST
	GameDefs::NO_EFFECT, // LAKE_WAGE_EFFECT
	0,	// LAKESIDE_IS_COASTAL
	30,	// ODD_TERRAIN
	1,	// IMPROVED_FARSIGHT
	1,	// GM_REPORT
	1,	// DECAY
	1,	// LIMITED_MAGES_PER_BUILDING
	(GameDefs::REPORT_SHOW_REGION | GameDefs::REPORT_SHOW_STRUCTURES | GameDefs::REPORT_SHOW_GUARDS), // TRANSIT_REPORT
	1,  // MARKETS_SHOW_ADVANCED_ITEMS
	GameDefs::PREPARE_NORMAL,	// USE_PREPARE_COMMAND
	15,	// MONSTER_ADVANCE_MIN_PERCENT
	2,	// MONSTER_ADVANCE_HOSTILE_PERCENT
	0,	// HAVE_EMAIL_SPECIAL_COMMANDS
	0,	// START_CITIES_START_UNLIMITED
	1,	// PROPORTIONAL_AMTS_USAGE
	1,  // USE_WEAPON_ARMOR_COMMAND
	6,  // MONSTER_NO_SPOILS
	12,  // MONSTER_SPOILS_RECOVERY
	0,   // ALLOW_ASSASSINATION
	0,  // MAX_ASSASSIN_FREE_ATTACKS
	0,  // RELEASE_MONSTERS
	1,  // CHECK_MONSTER_CONTROL_MID_TURN
	1,  // DETECT_GATE_NUMBERS
	GameDefs::ARMY_ROUT_HITS_INDIVIDUAL,  // ARMY_ROUT
	1,	// FULL_TRUESEEING_BONUS
	1,	// FULL_INVIS_ON_SELF
	0,	// MONSTER_BATTLE_REGEN
	GameDefs::TAX_TARMELLION, // WHO_CAN_TAX
	6,	// SKILL_PRACTISE_AMOUNT
	0,	// UPKEEP_MINIMUM_FOOD
	-1,	// UPKEEP_MAXIMUM_FOOD
	10,	// UPKEEP_FOOD_VALUE
	0,	// PREVENT_SAIL_THROUGH
	0,	// ALLOW_TRIVIAL_PORTAGE
	0,      // AUTOMATIC_FOOD
        1,      // CITY_TAX_COST;
        1,      // TOWN_TAX_COST;
        1,      // VILLAGE_TAX_COST;
        1,      // CITY_TRADE_COST;
        1,      // TOWN_TRADE_COST;
        1,      // VILLAGE_TRADE_COST;
	GameDefs::FP_OLD,       // FP_DISTRIBUTION
	1,      // FP_FACTOR
	0,	// ACN_IS_STARTING_CITY
	1,	// TARMELLION_SUMMONING
	1,  // AGGREGATE_BATTLE_REPORTS
	1,	// MULTIPLE_MAGES_PER_UNIT
	1,	// DISABLE_RESTART

	1,  // CODE_TEST

};

GameDefs * Globals = &g;




