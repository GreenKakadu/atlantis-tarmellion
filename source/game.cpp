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
#ifdef WIN32
#include <memory.h>  // Needed for memcpy on windows
#endif

#include <string.h>
#include <math.h>

#include "game.h"
#include "unit.h"
#include "fileio.h"
#include "astring.h"
#include <gamedata.h>
#include "items.h"

Game *thisgame;

Game::Game() {
	gameStatus = GAME_STATUS_UNINIT;
	ppUnits = 0;
	maxppunits = 0;
	thisgame=this;
}

Game::~Game() {
	delete ppUnits;
	ppUnits = 0;
	maxppunits = 0;
}

int Game::TurnNumber() {
	return (year-1)*12 + month + 1;
}

// ALT, 25-Jul-2000
// Default work order procedure
void Game::DefaultWorkOrder() {
	forlist(&regions) {
		ARegion * r = (ARegion *) elem;
		if (r->type == R_NEXUS) continue;
		forlist(&r->objects) {
			Object * o = (Object *) elem;
			forlist(&o->units) {
				Unit * u = (Unit *) elem;
				if (u->monthorders || u->faction->IsNPC() ||
						(Globals->TAX_PILLAGE_MONTH_LONG &&
						 u->taxing != TAX_NONE))
					continue;
				if (u->GetFlag(FLAG_AUTOTAX) &&
						(Globals->TAX_PILLAGE_MONTH_LONG && u->Taxers())) {
					u->taxing = TAX_AUTO;
				} else {
					if (Globals->DEFAULT_WORK_ORDER) ProcessWorkOrder(u, 0);
				}
			}
		}
	}
}


AString Game::GetXtraMap(ARegion * reg,int type) {
	int i;
	switch (type) {
	case 0:
		return (reg->IsStartingCity() ? "!" : (reg->HasShaft() ? "*" : " "));
	case 1:
		i = reg->CountWMons();
		return (i ? ((AString) i) : (AString(" ")));
	case 2:
		forlist(&reg->objects) {
			Object * o = (Object *) elem;
			if (!(ObjectDefs[o->type].flags & ObjectType::CANENTER)) {
				if (o->units.First()) {
					return "*";
				} else {
					return ".";
				}
			}
		}
		return " ";
	case 3:
		if (reg->gate) return "*";
		return " ";
	}
	return (" ");
}

void Game::WriteSurfaceMap(Aoutfile *f, ARegionArray *pArr, int type) {
	ARegion *reg;
	int yy = 0;
	int xx = 0;
	f->PutStr(AString("Map (") + xx*32 + "," + yy*16 + ")");
	for (int y=0; y < pArr->y; y+=2) {
		AString temp;
		int x;
		for (x=0; x< pArr->x; x+=2) {
			reg = pArr->GetRegion(x+xx*32,y+yy*16);
			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg,type);
			temp += "  ";
		}
		f->PutStr(temp);
		temp = "  ";
		for (x=1; x< pArr->x; x+=2) {
			reg = pArr->GetRegion(x+xx*32,y+yy*16+1);
			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg,type);
			temp += "  ";
		}
		f->PutStr(temp);
	}
	f->PutStr("");
}

void Game::WriteUnderworldMap(Aoutfile *f, ARegionArray *pArr, int type) {
	ARegion *reg, *reg2;
	int xx = 0;
	int yy = 0;
	f->PutStr(AString("Map (") + xx*32 + "," + yy*16 + ")");
	for (int y=0; y< pArr->y; y+=2) {
		AString temp = " ";
		AString temp2;
		int x;
		for (x=0; x< pArr->x; x+=2) {
			reg = pArr->GetRegion(x+xx*32,y+yy*16);
			reg2 = pArr->GetRegion(x+xx*32+1,y+yy*16+1);

			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg,type);

			if (reg2->neighbors[D_NORTH])
				temp += "|";
			else
				temp += " ";

			temp += " ";

			if (reg->neighbors[D_SOUTHWEST])
				temp2 += "/";
			else
				temp2 += " ";

			temp2 += " ";
			if (reg->neighbors[D_SOUTHEAST])
				temp2 += "\\";
			else
				temp2 += " ";

			temp2 += " ";
		}
		f->PutStr(temp);
		f->PutStr(temp2);

		temp = " ";
		temp2 = "  ";
		for (x=1; x< pArr->x; x+=2) {
			reg = pArr->GetRegion(x+xx*32,y+yy*16+1);
			reg2 = pArr->GetRegion(x+xx*32-1,y+yy*16);

			if (reg2->neighbors[D_SOUTH])
				temp += "|";
			else
				temp += " ";

			temp += AString(" ");
			temp += AString(GetRChar(reg));
			temp += GetXtraMap(reg,type);

			if (reg->neighbors[D_SOUTHWEST])
				temp2 += "/";
			else
				temp2 += " ";

			temp2 += " ";
			if (reg->neighbors[D_SOUTHEAST])
				temp2 += "\\";
			else
				temp2 += " ";

			temp2 += " ";
		}
		f->PutStr(temp);
		f->PutStr(temp2);
	}
	f->PutStr("");
}

int Game::ViewMap(const AString & typestr,const AString & mapfile) {
	int type = 0;
	if (AString(typestr) == "wmon") type = 1;
	if (AString(typestr) == "lair") type = 2;
	if (AString(typestr) == "gate") type = 3;

	Aoutfile f;
	if (f.OpenByName(mapfile) == -1) return 0;

	switch (type) {
	case 0:
		f.PutStr("Geographical Map");
		break;
	case 1:
		f.PutStr("Wandering Monster Map");
		break;
	case 2:
		f.PutStr("Lair Map");
		break;
	case 3:
		f.PutStr("Gate Map");
		break;
	}

	int i;
	for(i = 0; i < regions.numLevels; i++) {
		f.PutStr("");
		ARegionArray *pArr = regions.pRegionArrays[ i ];
		switch(pArr->levelType) {
		case ARegionArray::LEVEL_NEXUS:
			f.PutStr(AString("Level ") + i + ": Nexus");
			break;
		case ARegionArray::LEVEL_SURFACE:
			f.PutStr(AString("Level ") + i + ": Surface");
			WriteSurfaceMap(&f, pArr, type);
			break;
		case ARegionArray::LEVEL_UNDERWORLD:
			f.PutStr(AString("Level ") + i + ": Underworld");
			WriteUnderworldMap(&f, pArr, type);
			break;
		case ARegionArray::LEVEL_UNDERDEEP:
			f.PutStr(AString("Level ") + i + ": Underdeep");
			WriteUnderworldMap(&f, pArr, type);
			break;
		}
	}

	f.Close();

	return 1;
}

int Game::NewGame() {
	factionseq = 1;
	guardfaction = 0;
	monfaction = 0;
	unitseq = 1;
	SetupUnitNums();
	shipseq = 1000;
	year = 1;
	month = -1;
	gameStatus = GAME_STATUS_NEW;

	//
	// Seed the random number generator with a different value each time.
	//
	seedrandomrandom();

	CreateWorld();
	CreateNPCFactions();

	if (Globals->CITY_MONSTERS_EXIST)
		CreateCityMons();
	if (Globals->WANDERING_MONSTERS_EXIST)
		CreateWMons();
	if (Globals->LAIR_MONSTERS_EXIST)
		CreateLMons();
	if (Globals->LAIR_MONSTERS_EXIST)
		CreateVMons();

	return 1;
}

int Game::OpenGame() {
	return OpenGame("game.in");
}

int Game::OpenGame(const char * gamefile) {
	//
	// The order here must match the order in SaveGame
	//
	Ainfile f;
	if (f.OpenByName(gamefile) == -1) return 0;

	//
	// Read in Globals
	//
	AString *s1 = f.GetStr();
	if (!s1) return 0;
	AString *s2 = s1->gettoken();
	delete s1;
	if (!s2) return 0;
	if (! (*s2 == "atlantis_game")) {
		delete s2;
		f.Close();
		return 0;
	}
	delete s2;

	ATL_VER eVersion = f.GetInt();
	Awrite(AString("Saved Game Engine Version: ") + ATL_VER_STRING(eVersion));
	if (ATL_VER_MAJOR(eVersion) != ATL_VER_MAJOR(CURRENT_ATL_VER) ||
			ATL_VER_MINOR(eVersion) != ATL_VER_MINOR(CURRENT_ATL_VER)) {
		Awrite("Incompatible Engine versions!");
		return 0;
	}
	if (ATL_VER_PATCH(eVersion) > ATL_VER_PATCH(CURRENT_ATL_VER)) {
		Awrite("This game was created with a more recent Atlantis Engine!");
		return 0;
	}

	AString *gameName = f.GetStr();
	if (!gameName) {
		return 0;
	}
	if (!(*gameName == Globals->RULESET_NAME)) {
		Awrite("Incompatible rule-set!");
		delete gameName;
		return 0;
	}
	delete gameName;

	ATL_VER gVersion = f.GetInt();
	Awrite(AString("Saved Rule-Set Version: ") + ATL_VER_STRING(gVersion));

	if (ATL_VER_MAJOR(gVersion) < ATL_VER_MAJOR(Globals->RULESET_VERSION)) {
		Awrite(AString("Upgrading to ") +
				ATL_VER_STRING(MAKE_ATL_VER(ATL_VER_MAJOR(Globals->RULESET_VERSION), 0, 0)));
		if (!UpgradeMajorVersion(gVersion)) {
			Awrite("Unable to upgrade!  Aborting!");
			return 0;
		}
		gVersion = MAKE_ATL_VER(ATL_VER_MAJOR(Globals->RULESET_VERSION), 0, 0);
	}
	if (ATL_VER_MINOR(gVersion) < ATL_VER_MINOR(Globals->RULESET_VERSION)) {
		Awrite(AString("Upgrading to ") +
				ATL_VER_STRING(MAKE_ATL_VER(
						ATL_VER_MAJOR(Globals->RULESET_VERSION),
						ATL_VER_MINOR(Globals->RULESET_VERSION), 0)));
		if (! UpgradeMinorVersion(gVersion)) {
			Awrite("Unable to upgrade!  Aborting!");
			return 0;
		}
		gVersion = MAKE_ATL_VER(ATL_VER_MAJOR(gVersion),
				ATL_VER_MINOR(Globals->RULESET_VERSION), 0);
	}
	if (ATL_VER_PATCH(gVersion) < ATL_VER_PATCH(Globals->RULESET_VERSION)) {
		Awrite(AString("Upgrading to ") +
				ATL_VER_STRING(Globals->RULESET_VERSION));
		if (! UpgradePatchLevel(gVersion)) {
			Awrite("Unable to upgrade!  Aborting!");
			return 0;
		}
		gVersion = MAKE_ATL_VER(ATL_VER_MAJOR(gVersion),
				ATL_VER_MINOR(gVersion),
				ATL_VER_PATCH(Globals->RULESET_VERSION));
	}

	// LLS
	// when we get the stuff above fixed, we'll remove this junk
	// add a small hack to check to see whether we need to populate
	// ocean lairs
	doExtraInit = f.GetInt();
	if (doExtraInit > 0)
		year = doExtraInit;
	else
		year = f.GetInt();

	month = f.GetInt();
	seedrandom(f.GetInt());
	factionseq = f.GetInt();
	unitseq = f.GetInt();
	shipseq = f.GetInt();
	guardfaction = f.GetInt();
	monfaction = f.GetInt();

	//
	// Read in the Factions
	//
	int i = f.GetInt();

	for (int j=0; j<i; j++) {
		Faction * temp = new Faction;
		temp->Readin(&f, eVersion);
		factions.Add(temp);
	}

	//
	// Read in the ARegions
	//
	i = regions.ReadRegions(&f, &factions, eVersion);
	if (!i) return 0;

	// here we add ocean lairs
	if (doExtraInit > 0) CreateOceanLairs();

	FixBoatNums();
	FixGateNums();
	SetupUnitNums();

	f.Close();
	return 1;
}

int Game::SaveGame() {
	return SaveGame("game.out");
}

int Game::SaveGame(const char * gamefile) {
	Aoutfile f;
	if (f.OpenByName(gamefile) == -1) return 0;

	//
	// Write out Globals
	//
	f.PutStr("atlantis_game");
	f.PutInt(CURRENT_ATL_VER);
	f.PutStr(Globals->RULESET_NAME);
	f.PutInt(Globals->RULESET_VERSION);

	// mark the fact that we created ocean lairs
	f.PutInt(-1);

	f.PutInt(year);
	f.PutInt(month);
	f.PutInt(getrandom(10000));
	f.PutInt(factionseq);
	f.PutInt(unitseq);
	f.PutInt(shipseq);
	f.PutInt(guardfaction);
	f.PutInt(monfaction);

	//
	// Write out the Factions
	//
	f.PutInt(factions.Num());

	forlist(&factions)
		((Faction *) elem)->Writeout(&f);

	//
	// Write out the ARegions
	//
	regions.WriteRegions(&f);

	f.Close();
	return 1;
}

void Game::DummyGame() {
	//
	// No need to set anything up; we're just syntax checking some orders.
	//
}

#define PLAYERS_FIRST_LINE "AtlantisPlayerStatus"

int Game::WritePlayers() {
	Aoutfile f;
	if (f.OpenByName("players.out") == -1) return 0;

	f.PutStr(PLAYERS_FIRST_LINE);
	f.PutStr(AString("Version: ") + CURRENT_ATL_VER);
	f.PutStr(AString("TurnNumber: ") + TurnNumber());

	if (gameStatus == GAME_STATUS_UNINIT)
		return 0;
	else if (gameStatus == GAME_STATUS_NEW)
		f.PutStr(AString("GameStatus: New"));
	else if (gameStatus == GAME_STATUS_RUNNING)
		f.PutStr(AString("GameStatus: Running"));
	else if (gameStatus == GAME_STATUS_FINISHED)
		f.PutStr(AString("GameStatus: Finished"));

	f.PutStr("");

	forlist(&factions) {
		Faction *fac = (Faction *) elem;
		fac->WriteFacInfo(&f);
	}

	f.Close();
	return 1;
}

int Game::ReadPlayers() {
	Aorders f;
	if (f.OpenByName("players.in") == -1) return 0;

	AString *pLine = 0;
	AString *pToken = 0;

	//
	// Default: failure.
	//
	int rc = 0;

	do {
		//
		// The first line of the file should match.
		//
		pLine = f.GetLine();
		if (!(*pLine == PLAYERS_FIRST_LINE)) break;
		SAFE_DELETE(pLine);

		//
		// Get the file version number.
		//
		pLine = f.GetLine();
		pToken = pLine->gettoken();
		if (!pToken || !(*pToken == "Version:")) break;
		SAFE_DELETE(pToken);

		pToken = pLine->gettoken();
		if (!pToken) break;

		int nVer = pToken->value();
		if (ATL_VER_MAJOR(nVer) != ATL_VER_MAJOR(CURRENT_ATL_VER) ||
				ATL_VER_MINOR(nVer) != ATL_VER_MINOR(CURRENT_ATL_VER) ||
				ATL_VER_PATCH(nVer) > ATL_VER_PATCH(CURRENT_ATL_VER)) {
			Awrite("The players.in file is not compatible with this "
					"version of Atlantis.");
			break;
		}
		SAFE_DELETE(pToken);
		SAFE_DELETE(pLine);

		//
		// Ignore the turn number line.
		//
		pLine = f.GetLine();
		SAFE_DELETE(pLine);

		//
		// Next, the game status.
		//
		pLine = f.GetLine();
		pToken = pLine->gettoken();
		if (!pToken || !(*pToken == "GameStatus:")) break;
		SAFE_DELETE(pToken);

		pToken = pLine->gettoken();
		if (!pToken) break;
		if (*pToken == "New")
			gameStatus = GAME_STATUS_NEW;
		else if (*pToken == "Running")
			gameStatus = GAME_STATUS_RUNNING;
		else if (*pToken == "Finished")
			gameStatus = GAME_STATUS_FINISHED;
		else {
			//
			// The status doesn't seem to be valid.
			//
			break;
		}
		SAFE_DELETE(pToken);

		//
		// Now, we should have a list of factions.
		//
		pLine = f.GetLine();
		Faction *pFac = 0;

		int lastWasNew = 0;

		//
		// OK, set our return code to success; we'll set it to fail below
		// if necessary.
		//
		rc = 1;

		while(pLine) {
			pToken = pLine->gettoken();
			if (!pToken) {
				SAFE_DELETE(pLine);
				pLine = f.GetLine();
				continue;
			}

			if (*pToken == "Faction:") {
				//
				// Get the new faction
				//
				SAFE_DELETE(pToken);
				pToken = pLine->gettoken();
				if (!pToken) {
					rc = 0;
					break;
				}

				if (*pToken == "new") {
					pFac = AddFaction(1);
					if (!pFac) {
						Awrite("Failed to add a new faction!");
						rc = 0;
						break;
					}

					lastWasNew = 1;
				} else if (*pToken == "newNoLeader") {
					pFac = AddFaction(0);
					if (!pFac) {
						Awrite("Failed to add a new faction!");
						rc = 0;
						break;
					}
					lastWasNew = 1;
				} else {
					if (pFac && lastWasNew) {
						WriteNewFac(pFac);
					}
					int nFacNum = pToken->value();
					pFac = GetFaction(&factions, nFacNum);
					lastWasNew = 0;
				}
			} else if (pFac) {
				if (!ReadPlayersLine(pToken, pLine, pFac, lastWasNew)) {
					rc = 0;
					break;
				}
			}

			SAFE_DELETE(pToken);
			SAFE_DELETE(pLine);
			pLine = f.GetLine();
		}
		if (pFac && lastWasNew) WriteNewFac(pFac);
	} while(0);

	SAFE_DELETE(pLine);
	SAFE_DELETE(pToken);
	f.Close();

	return rc;
}

Unit *Game::ParseGMUnit(AString *tag, Faction *pFac) {
	char *str = tag->Str();
	if (*str == 'g' && *(str+1) == 'm') {
		AString p = AString(str+2);
		int gma = p.value();
		forlist(&regions) {
			ARegion *reg = (ARegion *)elem;
			forlist(&reg->objects) {
				Object *obj = (Object *)elem;
				forlist(&obj->units) {
					Unit *u = (Unit *)elem;
					if (u->faction->num == pFac->num && u->gm_alias == gma)
						return u;
				}
			}
		}
	} else {
		int v = tag->value();
		if ((unsigned int)v >= maxppunits) return NULL;
		return GetUnit(v);
	}
	return NULL;
}

int Game::ReadPlayersLine(AString *pToken, AString *pLine, Faction *pFac,
		int newPlayer) {
	AString *pTemp = 0;

	if (*pToken == "Name:") {
		pTemp = pLine->StripWhite();
		if (pTemp) {
			if (newPlayer) *pTemp += AString(" (") + (pFac->num) + ")";
			pFac->SetNameNoChange(pTemp);
		}
	} else if (*pToken == "RewardTimes")
		pFac->TimesReward();
	else if (*pToken == "Email:") {
		pTemp = pLine->gettoken();
		if (pTemp) {
			delete pFac->address;
			pFac->address = pTemp;
			pTemp = 0;
		}
	} else if (*pToken == "Password:") {
		pTemp = pLine->StripWhite();
		delete pFac->password;
		if (pTemp) {
			pFac->password = pTemp;
			pTemp = 0;
		} else
			pFac->password = 0;
	} else if (*pToken == "Reward:") {
		pTemp = pLine->gettoken();
		int nAmt = pTemp->value();
		pFac->Event(AString("Reward of ") + nAmt + " silver.");
		pFac->unclaimed += nAmt;
	} else if (*pToken == "SendTimes:") {
		// get the token, but otherwise ignore it
		pTemp = pLine->gettoken();
		pFac->times = pTemp->value();
	} else if (*pToken == "LastOrders:") {
		// Read this line and correctly set the lastorders for this
		// faction if the game itself isn't maintaining them.
		pTemp = pLine->gettoken();
		if (Globals->LASTORDERS_MAINTAINED_BY_SCRIPTS)
			pFac->lastorders = pTemp->value();
	} else if (*pToken == "Loc:") {
		int x, y, z;
		pTemp = pLine->gettoken();
		if (pTemp) {
			x = pTemp->value();
			delete pTemp;
			pTemp = pLine->gettoken();
			if (pTemp) {
				y = pTemp->value();
				delete pTemp;
				pTemp = pLine->gettoken();
				if (pTemp) {
					z = pTemp->value();
					ARegion *pReg = regions.GetRegion(x, y, z);
					if (pReg) pFac->pReg = pReg;
					else {
						Awrite(AString("Invalid Loc:")+x+","+y+","+z+
								" in faction " + pFac->num);
						pFac->pReg = NULL;
					}
				}
			}
		}
	} else if (*pToken == "StartLoc:") {
		int x, y, z;
		pTemp = pLine->gettoken();
		if (pTemp) {
			x = pTemp->value();
			delete pTemp;
			pTemp = pLine->gettoken();
			if (pTemp) {
				y = pTemp->value();
				delete pTemp;
				pTemp = pLine->gettoken();
				if (pTemp) {
					z = pTemp->value();
					ARegion *pReg = regions.GetRegion(x, y, z);
					if (pReg) {
						pFac->pStartLoc = pReg;
						if (!pFac->pReg) pFac->pReg = pReg;
					} else {
						Awrite(AString("Invalid StartLoc:")+x+","+y+","+z+
								" in faction " + pFac->num);
						pFac->pStartLoc = NULL;
					}
				}
			}
		}
       } else if (*pToken == "Market:") {
		// Create or alters a market.
		int error = 0;
		if (!pFac->pReg && pFac->pReg->town) {
			Awrite(AString("Markets can only be altered in a ") +
				"region with a valid town for faction "
				+ pFac->num);
			error = 1;
		}
		int type;
		if (!error) {
			pTemp = pLine->gettoken();
			if (*pTemp == AString("BUY")) {
				type = M_BUY;
			} else if (*pTemp == AString("SELL")) {
				type = M_SELL;
			} else {
				Awrite(AString("Invalid Market type specfied"));
				error = 1;
			}
		}
		int item;
		if (!error) {
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("Market: must specify item"));
				error = 1;
			} else {
				item = ParseEnabledItem(pTemp);
			}
		}
		int price,minpop,maxpop,minamt,maxamt;
		if (!error) {
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("Market: insufficent arguments"));
				error = 1;
			} else {
				price = pTemp->value();
			}
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("Market: insufficent arguments"));
				error = 1;
			} else {
				minpop = pTemp->value();
			}
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("Market: insufficent arguments"));
				error = 1;
			} else {
				minamt = pTemp->value();
			}
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("Market: insufficent arguments"));
				error = 1;
			} else {
				maxpop = pTemp->value();
			}
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("Market: insufficent arguments"));
				error = 1;
			} else {
				maxamt = pTemp->value();
			}
		}
		// Find if the market currently exists.
		int found = 0;
		forlist((&pFac->pReg->markets)) {
			Market * m = (Market *) elem;
			if (m->type == type && m->item == item) {
				if (maxamt == 0) {
					// remove the market.
					pFac->pReg->markets.Remove(elem);
				} else {
					m->baseprice = price;
					m->minpop = minpop;
					m->minamt = minamt;
					m->maxpop = maxpop;
					m->maxamt = maxamt;
				}
				found = 1;
			}
		}
		if (!found && maxamt > 0) {
			Market *m = new Market(type,item,price,price,minpop,maxpop,
			minamt,maxamt);
			pFac->pReg->markets.Add((AListElem *)m);
		}
	} else if (*pToken == "NewUnit:") {
		// Creates a new unit in the location specified by a Loc: line
		// with a gm_alias of whatever is after the NewUnit: tag.
		if (!pFac->pReg) {
			Awrite(AString("NewUnit is not valid without a Loc: ") +
					"for faction "+ pFac->num);
		} else {
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("NewUnit: must be followed by an alias ") +
						"in faction "+pFac->num);
			} else {
				int val = pTemp->value();
				if (!val) {
					Awrite(AString("NewUnit: must be followed by an alias ") +
							"in faction "+pFac->num);
				} else {
					Unit *u = GetNewUnit(pFac);
					u->gm_alias = val;
					u->MoveUnit(pFac->pReg->GetDummy());
					u->Event("Is given to your faction.");
				}
			}
		}
	} else if (*pToken == "Race:") {
		pTemp = pLine->gettoken();
		if (!pTemp)
	  		Awrite(AString("Race: needs to specify a race."));
		else {
			if (*pTemp == "none")
				pFac->race = -1;
			else {
				int it = ParseEnabledItem(pTemp);
				if (ItemDefs[it].type == IT_MAN)
					pFac->race = it;
				else
					Awrite(AString("Race: needs to specify a race.")+ *+pTemp);
			}
		}
	} else if (*pToken == "Item:") {
		pTemp = pLine->gettoken();
		if (!pTemp) {
			Awrite(AString("Item: needs to specify a unit in faction ") +
					pFac->num);
		} else {
			Unit *u = ParseGMUnit(pTemp, pFac);
			if (!u) {
				Awrite(AString("Item: needs to specify a unit in faction ") +
						pFac->num);
			} else {
				if (u->faction->num != pFac->num) {
					Awrite(AString("Item: unit ")+ u->num +
							" doesn't belong to " + "faction " + pFac->num);
				} else {
					delete pTemp;
					pTemp = pLine->gettoken();
					if (!pTemp) {
						Awrite(AString("Must specify a number of items to ") +
								"give for Item: in faction " + pFac->num);
					} else {
						int v = pTemp->value();
						if (!v) {
							Awrite(AString("Must specify a number of ") +
										"items to give for Item: in " +
										"faction " + pFac->num);
						} else {
							delete pTemp;
							pTemp = pLine->gettoken();
							if (!pTemp) {
								Awrite(AString("Must specify a valid item ") +
										"to give for Item: in faction " +
										pFac->num);
							} else {
								int it = ParseAllItems(pTemp);
								if (it == -1) {
									Awrite(AString("Must specify a valid ") +
											"item to give for Item: " +
											*pTemp +
											" in faction " + pFac->num);
								} else {
									int has = u->items.GetNum(it);
									u->items.SetNum(it, has + v);
									if (!u->gm_alias) {
										u->Event(AString("Is given ") +
												ItemString(it, v) +
												" by the gods.");
									}
									u->faction->DiscoverItem(it, 0, 1);
								}
							}
						}
					}
				}
			}
		}
	} else if (*pToken == "Skill:") {
		pTemp = pLine->gettoken();
		if (!pTemp) {
			Awrite(AString("Skill: needs to specify a unit in faction ") +
					pFac->num);
		} else {
			Unit *u = ParseGMUnit(pTemp, pFac);
			if (!u) {
				Awrite(AString("Skill: needs to specify a unit: ") + *pTemp+
						"in faction " + pFac->num);
			} else {
				if (u->faction->num != pFac->num) {
					Awrite(AString("Skill: unit ")+ u->num +
							" doesn't belong to " + "faction " + pFac->num);
				} else {
					delete pTemp;
					pTemp = pLine->gettoken();
					if (!pTemp) {
						Awrite(AString("Must specify a valid skill for ") +
								"Skill: in faction " + pFac->num);
					} else {
						int sk = ParseSkill(pTemp);
						if (sk == -1) {
							Awrite(AString("Must specify a valid skill for ")+
									"Skill: in faction " + pFac->num);
						} else {
							delete pTemp;
							pTemp = pLine->gettoken();
							if (!pTemp) {
								Awrite(AString("Must specify a days for ") +
										"Skill: in faction " + pFac->num);
							} else {
								int days = pTemp->value() * u->GetMen();
								if (!days) {
									Awrite(AString("Must specify a days for ")+
											"Skill: in faction " + pFac->num);
								} else {
									int odays = u->skills.GetDays(sk);
									u->skills.SetDays(sk, odays + days);
									u->AdjustSkills();
									int lvl = u->GetRealSkill(sk);
									if (lvl > pFac->skills.GetDays(sk)) {
										for( int i = pFac->skills.GetDays(sk) + 1; i <= lvl; i++ ) {
											pFac->shows.Add(new ShowSkill(sk,i));
										}
										pFac->skills.SetDays(sk, lvl);
									}
									if (!u->gm_alias) {
										u->Event(AString("Is taught ") +
												days + " days of " +
												SkillStrs(sk) +
												" by the gods.");
									}
									/*
									 * This is NOT quite the same, but the gods
									 * are more powerful than mere mortals
									 */
									int mage = (SkillDefs[sk].flags &
											SkillType::MAGIC);
									int app = (SkillDefs[sk].flags &
											SkillType::APPRENTICE);
									if (mage) {
										u->type = U_MAGE;
									}
									if (app && u->type == U_NORMAL) {
										u->type = U_APPRENTICE;
									}
								}
							}
						}
					}
				}
			}
		}
	} else if (*pToken == "Order:") {
		pTemp = pLine->StripWhite();
		if (*pTemp == "quit") {
			pFac->quit = QUIT_BY_GM;
		} else {
			// handle this as a unit order
			delete pTemp;
			pTemp = pLine->gettoken();
			if (!pTemp) {
				Awrite(AString("Order: needs to specify a unit in faction ") +
						pFac->num);
			} else {
				Unit *u = ParseGMUnit(pTemp, pFac);
				if (!u) {
					Awrite(AString("Order: needs to specify a unit: ") + *pTemp +
							" in faction " + pFac->num);
				} else {
					if (u->faction->num != pFac->num) {
						Awrite(AString("Order: unit ")+ u->num +
								" doesn't belong to " + "faction " +
								pFac->num);
					} else {
						delete pTemp;
						AString saveorder = *pLine;
						int getatsign = pLine->getat();
						pTemp = pLine->gettoken();
						if (!pTemp) {
							Awrite(AString("Order: must provide unit order ")+
									"for faction "+pFac->num);
						} else {
							int o = Parse1Order(pTemp);
							if (o == -1 || o == O_ATLANTIS || o == O_END ||
									o == O_UNIT || o == O_FORM ||
									o == O_ENDFORM) {
								Awrite(AString("Order: invalid order given ")+
										"for faction "+pFac->num);
							} else {
								if (getatsign) {
									u->oldorders.Add(new AString(saveorder));
								}
								ProcessOrder(o, u, pLine, NULL);
							}
						}
					}
				}
			}
		}
	} else {
		pTemp = new AString(*pToken + *pLine);
		pFac->extraPlayers.Add(pTemp);
		pTemp = 0;
	}

	if (pTemp) delete pTemp;
	return 1;
}

void Game::WriteNewFac(Faction *pFac) {
	AString *strFac = new AString(AString("Adding ") +
		   *(pFac->address) + ".");
	newfactions.Add(strFac);
}

int Game::DoOrdersCheck(const AString &strOrders, const AString &strCheck) {
	Aorders ordersFile;
	if (ordersFile.OpenByName(strOrders) == -1) {
		Awrite("No such orders file!");
		return 0;
	}

	Aoutfile checkFile;
	if (checkFile.OpenByName(strCheck) == -1) {
		Awrite("Couldn't open the orders check file!");
		return 0;
	}

	OrdersCheck check;
	check.pCheckFile = &checkFile;

	ParseOrders(0, &ordersFile, &check);

	ordersFile.Close();
	checkFile.Close();

	return 1;
}

int Game::RunGame() {
	Awrite("Setting Up Turn...");
	PreProcessTurn();

	Awrite("Reading the Gamemaster File...");
	if (!ReadPlayers()) return 0;

	if (gameStatus == GAME_STATUS_FINISHED) {
		Awrite("This game is finished!");
		return 0;
	}
	gameStatus = GAME_STATUS_RUNNING;

	Awrite("Reading the Orders File...");
	ReadOrders();

	if (Globals->MAX_INACTIVE_TURNS != -1) {
		Awrite("QUITting Inactive Factions...");
		RemoveInactiveFactions();
	}

	Awrite("Running the Turn...");
	RunOrders();

	Awrite("Writing the Report File...");
	WriteReport();
	Awrite("");
	battles.DeleteAll();

	Awrite("Writing Playerinfo File...");
	WritePlayers();

	Awrite("Removing Dead Factions...");
	DeleteDeadFactions();
	Awrite("done");

	return 1;
}

int Game::EditGame(int *pSaveGame) {
	*pSaveGame = 0;

	Awrite("Editing an Atlantis Game: ");
	do {
		int exit = 0;

		Awrite("Main Menu");
		Awrite("  1) Find a region...");
		Awrite("  2) Find a unit...");
		Awrite("  q) Quit without saving.");
		Awrite("  x) Exit and save.");
		Awrite("> ");

		AString *pStr = AGetString();
		Awrite("");

		if (*pStr == "q") {
			exit = 1;
			Awrite("Quiting without saving.");
		} else if (*pStr == "x") {
			exit = 1;
			*pSaveGame = 1;
			Awrite("Exit and save.");
		} else if (*pStr == "1") {
			ARegion *pReg = EditGameFindRegion();
			if (pReg) EditGameRegion(pReg);
		} else if (*pStr == "2")
			EditGameFindUnit();
		else
			Awrite("Select from the menu.");

		delete pStr;
		if (exit) break;
	} while (1);

	return 1;
}

ARegion *Game::EditGameFindRegion() {
	ARegion *ret = 0;
	int x, y, z;
	AString *pStr = 0, *pToken = 0;
	Awrite("Region coords (x y z):");
	do {
		pStr = AGetString();

		pToken = pStr->gettoken();
		if (!pToken) {
			Awrite("No such region.");
			break;
		}
		x = pToken->value();
		SAFE_DELETE(pToken);

		pToken = pStr->gettoken();
		if (!pToken) {
			Awrite("No such region.");
			break;
		}
		y = pToken->value();
		SAFE_DELETE(pToken);

		pToken = pStr->gettoken();
		if (pToken) {
			z = pToken->value();
			SAFE_DELETE(pToken);
		} else
			z = 0;

		ARegion *pReg = regions.GetRegion(x, y, z);
		if (!pReg) {
			Awrite("No such region.");
			break;
		}

		ret = pReg;
	} while (0);

	if (pStr) delete pStr;
	if (pToken) delete pToken;

	return ret;
}

void Game::EditGameFindUnit() {
	AString *pStr;
	Awrite("Which unit number?");
	pStr = AGetString();
	int num = pStr->value();
	delete pStr;
	Unit *pUnit = GetUnit(num);
	if (!pUnit) {
		Awrite("No such unit!");
		return;
	}
	EditGameUnit(pUnit);
}

void Game::EditGameRegion( ARegion *pReg ) { 
	do { 
		Awrite( AString( "Region: " ) + pReg->ShortPrint( &regions ) ); 
		Awrite( " 1) Edit objects..." ); 
		Awrite( " q) Return to previous menu." ); 

		int exit = 0; 
		AString *pStr = AGetString(); 
		if( *pStr == "1" )
			EditGameRegionObjects( pReg ); 
		else if( *pStr == "q" ) 
			exit = 1; 
		else 
			Awrite( "Select from the menu." ); 
		delete pStr; 

		if (exit) 
			break; 
	} while( 1 ); 
}

void Game::EditGameRegionObjects( ARegion *pReg ) { 
	do { 
		Awrite( AString("Region: ") + pReg->ShortPrint(&regions)); 
		Awrite(""); 
		int i = 0; 
		forlist (&(pReg->objects)) { 
			Object * obj = (Object *)elem; 
			Awrite(AString(i) + ". " + *obj->name + " : " + 
			ObjectDefs[obj->type].name); 
			i++; 
		} 
		Awrite(""); 

		Awrite(" [a] [object type] to add object"); 
		Awrite(" [d] [index] to delete object"); 
		Awrite(" [n] [index] [name] to rename object"); 
		Awrite(" q) Return to previous menu."); 

		int exit = 0; 
		AString *pStr = AGetString(); 
		if (*pStr == "q")
			exit = 1; 
		else { 
			AString *pToken = 0; 
			do { 
				pToken = pStr->gettoken(); 
				if (!pToken) { 
					Awrite("Try again."); 
					break; 
				} 

				if (*pToken == "a") { // add object 
					SAFE_DELETE(pToken); 
	
					pToken = pStr->gettoken(); 
					if (!pToken) { 
						Awrite("Try again."); 
						break; 
					} 

					int objType = ParseObject(pToken); 
					if ((objType == -1) || (ObjectDefs[objType].flags & ObjectType::DISABLED)) { 
						Awrite("No such object."); 
						break; 
					} 
					SAFE_DELETE(pToken); 
	
					Object *o = new Object(pReg); 
					o->type = objType; 
					o->incomplete = 0; 
					o->inner = -1; 
					if (o->IsBoat()) { 
						o->num = shipseq++; 
						o->name = new AString(AString("Ship") + " [" + o->num + "]"); 
					} else { 
						o->num = pReg->buildingseq++; 
						o->name = new AString(AString("Building") + " [" + o->num + "]"); 
					} 
					pReg->objects.Add(o); 
				}
	
				else if (*pToken == "d") { // delete object 
					SAFE_DELETE(pToken); 
	
					pToken = pStr->gettoken(); 
					if (!pToken) { 
						Awrite("Try again."); 
						break; 
					} 
	
					int index = pToken->value(); 
					if ((index < 1) || (index >= pReg->objects.Num())) { 
						Awrite( "Incorrect index." ); 
						break; 
					} 
					SAFE_DELETE(pToken); 
	
					int i = 0; 
					AListElem *tmp = pReg->objects.First(); 
					for (i = 0; i < index; i++) tmp = pReg->objects.Next(tmp); 
					pReg->objects.Remove(tmp); 
				}
	
				else if (*pToken == "n") { // rename object 
					SAFE_DELETE( pToken ); 

					pToken = pStr->gettoken(); 
					if (!pToken) { 
						Awrite( "Try again." ); 
						break; 
					} 

					int index = pToken->value(); 
					if ((index < 1) || (index >= pReg->objects.Num())) { 
						Awrite("Incorrect index."); 
						break; 
					} 
					SAFE_DELETE(pToken); 

					pToken = pStr->gettoken(); 
					if (!pToken) { 
						Awrite("No name given."); 
						break; 
					} 

					int i = 0; 
					Object *tmp = (Object *)pReg->objects.First(); 
					for (i = 0; i < index; i++)
						tmp = (Object *)pReg->objects.Next(tmp); 

					AString * newname = pToken->getlegal(); 
					SAFE_DELETE(pToken); 
					if (newname) { 
						delete tmp->name; 
						*newname += AString(" [") + tmp->num + "]"; 
						tmp->name = newname; 
					} 
				} 

			} while (0); 
			if (pToken) delete pToken; 
		}
		if(pStr) delete pStr; 
	
		if (exit) break; 
	} while (1); 
}

void Game::EditGameUnit(Unit *pUnit) {
	do {
		Awrite(AString("Unit: ") + *(pUnit->name));
		Awrite(AString("  in ") +
			pUnit->object->region->ShortPrint(&regions));
		Awrite("  1) Edit items...");
		Awrite("  2) Edit skills...");
		Awrite("  3) Move unit...");
		Awrite("  q) Stop editing this unit.");

		int exit = 0;
		AString *pStr = AGetString();
		if (*pStr == "1")
			EditGameUnitItems(pUnit);
		else if (*pStr == "2")
			EditGameUnitSkills(pUnit);
		else if (*pStr == "3")
			EditGameUnitMove(pUnit);
		else if (*pStr == "q")
			exit = 1;
		else
			Awrite("Select from the menu.");

		delete pStr;

		if (exit) break;
	} while (1);
}

void Game::EditGameUnitItems(Unit *pUnit) {
	do {
		int exit = 0;
		Awrite(AString("Unit items: ") + pUnit->items.Report(2, 1, 1));
		Awrite("  [item] [number] to update an item.");
		Awrite("  q) Stop editing the items.");
		AString *pStr = AGetString();
		if (*pStr == "q")
			exit = 1;
		else {
			AString *pToken = 0;
			do {
				pToken = pStr->gettoken();
				if (!pToken) {
					Awrite("Try again.");
					break;
				}

				int itemNum = ParseAllItems(pToken);
				if (itemNum == -1) {
					Awrite("No such item.");
					break;
				}
				if (ItemDefs[itemNum].flags & ItemType::DISABLED) {
					Awrite("No such item.");
					break;
				}
				SAFE_DELETE(pToken);

				int num;
				pToken = pStr->gettoken();
				if (!pToken) num = 0;
				else num = pToken->value();

				pUnit->items.SetNum(itemNum, num);
				/* Mark it as known about for 'shows' */
				pUnit->faction->items.SetNum(itemNum, 1);
			} while (0);
			if (pToken) delete pToken;
		}
		if (pStr) delete pStr;

		if (exit) break;
	} while (1);
}

void Game::EditGameUnitSkills(Unit *pUnit) {
	do {
		int exit = 0;
		Awrite(AString("Unit skills: ") +
			pUnit->skills.Report(pUnit->GetMen()));
		Awrite("  [skill] [days] to update a skill.");
		Awrite("  q) Stop editing the skills.");
		AString *pStr = AGetString();
		if (*pStr == "q") exit = 1;
		else {
			AString *pToken = 0;
			do {
				pToken = pStr->gettoken();
				if (!pToken) {
					Awrite("Try again.");
					break;
				}

				int skillNum = ParseSkill(pToken);
				if (skillNum == -1) {
					Awrite("No such skill.");
					break;
				}
				if (SkillDefs[skillNum].flags & SkillType::DISABLED) {
					Awrite("No such skill.");
					break;
				}
				SAFE_DELETE(pToken);

				int days;
				pToken = pStr->gettoken();
				if (!pToken) {
					days = 0;
				} else {
					days = pToken->value();
				}

				if ((SkillDefs[skillNum].flags & SkillType::MAGIC) &&
						(pUnit->type != U_MAGE)) {
					pUnit->type = U_MAGE;
				}
				if ((SkillDefs[skillNum].flags & SkillType::APPRENTICE) &&
						(pUnit->type == U_NORMAL)) {
					pUnit->type = U_APPRENTICE;
				}
				pUnit->skills.SetDays(skillNum, days * pUnit->GetMen());
				int lvl = pUnit->GetRealSkill(skillNum);
				if (lvl > pUnit->faction->skills.GetDays(skillNum)) {
					pUnit->faction->skills.SetDays(skillNum, lvl);
				}
			} while (0);
			delete pToken;
		}
		delete pStr;

		if (exit) break;
	} while (1);
}

void Game::EditGameUnitMove(Unit *pUnit) {
	ARegion *pReg = EditGameFindRegion();
	if (!pReg) return;

	pUnit->MoveUnit(pReg->GetDummy());
}

void Game::PreProcessTurn() {
	month++;
	if (month>11) {
		month = 0;
		year++;
	}
	SetupUnitNums();
	{
		forlist(&factions) {
			((Faction *) elem)->DefaultOrders();
			//((Faction *) elem)->TimesReward();
		}
	}
	{
		forlist(&regions) {
			ARegion *pReg = (ARegion *) elem;
			if (Globals->WEATHER_EXISTS) pReg->SetWeather(regions.GetWeather(pReg, month));
			pReg->DefaultOrders();
		}
	}
}

void Game::ClearOrders(Faction * f) {
	forlist(&regions) {
		ARegion * r = (ARegion *) elem;
		forlist(&r->objects) {
			Object * o = (Object *) elem;
			forlist(&o->units) {
				Unit * u = (Unit *) elem;
				if (u->faction == f) u->ClearOrders();
			}
		}
	}
}

void Game::ReadOrders() {
	forlist(&factions) {
		Faction *fac = (Faction *) elem;
		if (!fac->IsNPC()) {
			AString str = "orders.";
			str += fac->num;

				Aorders file;
			if (file.OpenByName(str) != -1) {
				ParseOrders(fac->num, &file, 0);
				file.Close();
			}
			DefaultWorkOrder();
		}
	}
}

void Game::MakeFactionReportLists() {
	FactionVector vector(factionseq);

	forlist(&regions) {
		vector.ClearVector();

		ARegion *reg = (ARegion *) elem;
		forlist(&reg->farsees) {
			Faction *fac = ((Farsight *) elem)->faction;
			vector.SetFaction(fac->num, fac);
		}
		{
			forlist(&reg->passers) {
				Faction *fac = ((Farsight *)elem)->faction;
				vector.SetFaction(fac->num, fac);
			}
		}
		{
			forlist(&reg->objects) {
				Object *obj = (Object *) elem;

				forlist(&obj->units) {
					Unit *unit = (Unit *) elem;
					vector.SetFaction(unit->faction->num, unit->faction);
				}
			}
		}

		for (int i=0; i<vector.vectorsize; i++) {
			if (vector.GetFaction(i)) {
				ARegionPtr *ptr = new ARegionPtr;
				ptr->ptr = reg;
				vector.GetFaction(i)->present_regions.Add(ptr);
			}
		}
	}
}

void Game::WriteReport() {
	Areport f;

	MakeFactionReportLists();
	CountAllMages();
	if (Globals->APPRENTICES_EXIST) CountAllApprentices();

	forlist(&factions) {
	Faction * fac = (Faction *) elem;
	AString str = "report.";
	str = str + fac->num;

		if (!fac->IsNPC() ||
		   ((((month == 0) && (year == 1)) || Globals->GM_REPORT) &&
			(fac->num == 1))) {
			int i = f.OpenByName(str);
			if (i != -1) {
				fac->WriteReport(&f, this);
				f.Close();
			}
		}
//	Adot();
	cout << fac->num << " " << flush;
	}
}

void Game::DeleteDeadFactions() {
	forlist(&factions) {
		Faction * fac = (Faction *) elem;
		if (!fac->IsNPC() && !fac->exists) {
			factions.Remove(fac);
			forlist((&factions))
				((Faction *) elem)->RemoveAttitude(fac->num);
	   		 delete fac;
		}
	}
}

Faction *Game::AddFaction(int newleader) {
	//
	// set up faction
	//
	Faction *temp = new Faction(factionseq);
	if (!newleader) temp->noStartLeader = 1;
	AString x("NoAddress");
	temp->SetAddress(x);
	temp->lastorders = TurnNumber();

	if (SetupFaction(temp)) {
		factions.Add(temp);
		factionseq++;
		return (temp);
	} else {
		delete temp;
		return 0;
	}
}

void Game::ViewFactions() {
	forlist((&factions))
	((Faction *) elem)->View();
}

void Game::SetupUnitSeq() {
	int max = 0;
	forlist(&regions) {
		ARegion *r = (ARegion *)elem;
		forlist(&r->objects) {
			Object *o = (Object *)elem;
			forlist(&o->units) {
				Unit *u = (Unit *)elem;
				if (u && u->num > max) max = u->num;
			}
		}
	}
	unitseq = max+1;
}

void Game::FixGateNums() {
	for(int i=1; i <= regions.numberofgates; i++) {
		ARegion *tar = regions.FindGate(i);
		if (tar) continue; // This gate exists, continue
		int done = 0;
		while(!done) {
			// We have a missing gate, add it

			// Get the z coord, exclude the nexus (and the abyss as well)
			int z = getrandom(regions.numLevels);
			ARegionArray *arr = regions.GetRegionArray(z);
			if (arr->levelType == ARegionArray::LEVEL_NEXUS) continue;

			// Get a random hex within that level
			int x = getrandom(arr->x);
			int y = getrandom(arr->y);
			tar = arr->GetRegion(x, y);
			if (!tar) continue;

			// Make sure the hex can have a gate and doesn't already
			if ((TerrainDefs[tar->type].similar_type==R_OCEAN) || tar->gate)
				continue;
			tar->gate = i;
			done = 1;
		}
	}
}

void Game::FixBoatNums() {
	forlist(&regions) {
		ARegion *r = (ARegion *)elem;
		forlist(&r->objects) {
			Object *o = (Object *)elem;
			if (ObjectIsShip(o->type) && (o->num < 100)) {
				o->num = shipseq++;
				o->SetName(new AString("Ship"));
			}
		}
	}
}

void Game::SetupUnitNums() {
	if (ppUnits) delete ppUnits;

	SetupUnitSeq();

	maxppunits = unitseq+10000;

	ppUnits = new Unit *[ maxppunits ];

	unsigned int i;
	for(i = 0; i < maxppunits ; i++) ppUnits[ i ] = 0;

	forlist(&regions) {
		ARegion * r = (ARegion *) elem;
		forlist(&r->objects) {
			Object * o = (Object *) elem;
			forlist(&o->units) {
				Unit *u = (Unit *) elem;
				i = u->num;
				if ((i > 0) && (i < maxppunits)) {
					if (!ppUnits[i]) ppUnits[ u->num ] = u;
					else {
						Awrite(AString("Error: Unit number ") + i +
							" multiply defined.");
						if ((unitseq > 0) && (unitseq < maxppunits)) {
							u->num = unitseq;
							ppUnits[unitseq++] = u;
						}
					}
				} else {
					Awrite(AString("Error: Unit number ")+i+
							" out of range.");
					if ((unitseq > 0) && (unitseq < maxppunits)) {
						u->num = unitseq;
						ppUnits[unitseq++] = u;
					}
				}
			}
		}
	}
}

Unit *Game::GetNewUnit(Faction *fac, int an) {
	unsigned int i;
	for(i = 1; i < unitseq; i++) {
		if (!ppUnits[ i ]) {
			Unit *pUnit = new Unit(i, fac, an);
			ppUnits[ i ] = pUnit;
			return (pUnit);
		}
	}

	Unit *pUnit = new Unit(unitseq, fac, an);
	ppUnits[ unitseq ] = pUnit;
	unitseq++;
	if (unitseq >= maxppunits) {
		Unit **temp = new Unit*[maxppunits+10000];
		memcpy(temp, ppUnits, maxppunits*sizeof(Unit *));
		maxppunits += 10000;
		delete ppUnits;
		ppUnits = temp;
	}

	return pUnit;
}

Unit *Game::GetUnit(int num) {
	if (num < 0 || (unsigned int)num >= maxppunits) return NULL;
	return ppUnits[ num ];
}

void Game::CountAllMages() {
	forlist(&factions) 
		((Faction *) elem)->nummages = 0;
	{
		forlist(&regions) {
			ARegion * r = (ARegion *) elem;
			forlist(&r->objects) {
				Object * o = (Object *) elem;
				forlist(&o->units) {
					Unit * u = (Unit *) elem;
					if (u->type == U_MAGE) u->faction->nummages += u->GetMen();
				}
			}
		}
	}
}

// LLS
void Game::UnitFactionMap() {
	Aoutfile f;
	unsigned int i;
	Unit *u;

	Awrite("Opening units.txt");
	if (f.OpenByName("units.txt") == -1) {
		Awrite("Couldn't open file!");
	} else {
		Awrite(AString("Writing ") + unitseq + " units");
		for (i = 1; i < unitseq; i++) {
			u = GetUnit(i);
			if (!u) {
				Awrite("doesn't exist");
			} else {
				Awrite(AString(i) + ":" + u->faction->num);
				f.PutStr(AString(i) + ":" + u->faction->num);
			}
		}
	}
	f.Close();
}


//The following function added by Creative PBM February 2000
void Game::RemoveInactiveFactions() {
	if (Globals->MAX_INACTIVE_TURNS == -1) return;

	int cturn;
	cturn = TurnNumber();
	forlist(&factions) {
		Faction *fac = (Faction *) elem;
		if ((cturn - fac->lastorders) >= Globals->MAX_INACTIVE_TURNS && !fac->IsNPC()) {
			fac->quit = QUIT_BY_GM;
		}
	}
}

void Game::CountAllApprentices() {
	if (!Globals->APPRENTICES_EXIST) return;

	forlist(&factions) {
		((Faction *)elem)->numapprentices = 0;
	}
	{
		forlist(&regions) {
			ARegion *r = (ARegion *)elem;
			forlist(&r->objects) {
				Object *o = (Object *)elem;
				forlist(&o->units) {
					Unit *u = (Unit *)elem;
					if (u->type == U_APPRENTICE)
						u->faction->numapprentices++;
				}
			}
		}
	}
}

int Game::CountApprentices(Faction *pFac) {
	int i = 0;
	forlist(&regions) {
		ARegion *r = (ARegion *)elem;
		forlist(&r->objects) {
			Object *o = (Object *)elem;
			forlist(&o->units) {
				Unit *u = (Unit *)elem;
				if (u->faction == pFac && u->type == U_APPRENTICE) i++;
			}
		}
	}
	return i;
}

int Game::AllowedMages(Faction *pFac) {
	int points = pFac->type[F_MAGIC];

        switch (Globals->FP_DISTRIBUTION){
	case GameDefs::FP_OLD:
		if (points < 0) points = 0;
		if (points > allowedMagesSize - 1) points = allowedMagesSize - 1;
		return allowedMages[points];
	case GameDefs::FP_LIN:
		return (int) (points*1.6*Globals->FP_FACTOR);
	case GameDefs::FP_SQR:
		return (int) (points*points/46.875);
	case GameDefs::FP_SQRT:
		return (int) (13.9*sqrt((double) points));
	case GameDefs::FP_LIN_SQRT:
		return (int) (points*sqrt((double) points)/5.4);
	case GameDefs::FP_EQU:
		return points*Globals->FP_FACTOR;
	}
	return 0; // shouldn't ever be reached
}

int Game::AllowedApprentices(Faction *pFac) {
	int points = pFac->type[F_MAGIC];

        switch (Globals->FP_DISTRIBUTION){
	case GameDefs::FP_OLD:
		if (points < 0) points = 0;
		if (points > allowedApprenticesSize - 1) points = allowedApprenticesSize - 1;
		return allowedApprentices[points];
	case GameDefs::FP_LIN:
		return (int) (points*1.6*Globals->FP_FACTOR*2);
	case GameDefs::FP_SQR:
		return (int) (points*points/46.875*2);
	case GameDefs::FP_SQRT:
		return (int) (13.9*sqrt((double) points)*2);
	case GameDefs::FP_LIN_SQRT:
		return (int) (points*sqrt((double) points)/5.4*2);
	case GameDefs::FP_EQU:
		return points*Globals->FP_FACTOR*2;
	}
	return 0; // shouldn't ever be reached

}

int Game::AllowedTaxes(Faction *pFac) {
	int points = pFac->type[F_WAR];

        switch (Globals->FP_DISTRIBUTION){
	case GameDefs::FP_OLD:
		if (points < 0) points = 0;
		if (points > allowedTaxesSize - 1) points = allowedTaxesSize - 1;
		return allowedTaxes[points];
	case GameDefs::FP_LIN:
		return (int) (points*1.6*Globals->FP_FACTOR);
	case GameDefs::FP_SQR:
		return (int) (points*points/46.875);
	case GameDefs::FP_SQRT:
		return (int) (13.9*sqrt((double) points));
	case GameDefs::FP_LIN_SQRT:
		return (int) (points*sqrt((double) points)/5.4);
	case GameDefs::FP_EQU:
		return points*Globals->FP_FACTOR;
	}
	return 0; // shouldn't ever be reached
}

int Game::AllowedTrades(Faction *pFac) {
	int points = pFac->type[F_TRADE];

        switch (Globals->FP_DISTRIBUTION){
	case GameDefs::FP_OLD:
		if (points < 0) points = 0;
		if (points > allowedTradesSize - 1) points = allowedTradesSize - 1;
		return allowedTrades[points];
	case GameDefs::FP_LIN:
		return (int) (points*1.6*Globals->FP_FACTOR);
	case GameDefs::FP_SQR:
		return (int) (points*points/46.875);
	case GameDefs::FP_SQRT:
		return (int) (13.9*sqrt((double) points));
	case GameDefs::FP_LIN_SQRT:
		return (int) (points*sqrt((double) points)/5.4);
	case GameDefs::FP_EQU:
		return points*Globals->FP_FACTOR;
	}
	return 0; // shouldn't ever be reached
}

int Game::UpgradeMajorVersion(int savedVersion) {
	return 1;
}

int Game::UpgradeMinorVersion(int savedVersion) {
	return 1;
}

int Game::UpgradePatchLevel(int savedVersion) {
	return 1;
}

// This will get called if we load an older version of the database which
// didn't have ocean lairs
void Game::CreateOceanLairs() {
	// here's where we add the creation.
	forlist (&regions) {
		ARegion * r = (ARegion *) elem;
		if (TerrainDefs[r->type].similar_type == R_OCEAN) {
			r->LairCheck();
		}
	}
}

void Game::MidProcessUnitExtra(ARegion *r, Unit *u) {
	if (Globals->CHECK_MONSTER_CONTROL_MID_TURN)
		MonsterCheck(r, u);
}

void Game::PostProcessUnitExtra(ARegion *r, Unit *u) {
	if (!Globals->CHECK_MONSTER_CONTROL_MID_TURN)
		MonsterCheck(r, u);
}

void Game::MonsterCheck(ARegion *r, Unit *u) {
	if (u->type != U_WMON) {
		int escape = 0;
		int totlosses = 0;
		int level;
		int skill;
		int top;
		//int dragons = 0; // number of good dragons found in unit // not used yet
		//int wyrms = 0; // number of evil dragons found in unit // not used yet

		forlist (&u->items) {
			Item *i = (Item *) elem;
			if (!i->num) continue;
			/* XXX -- This should be genericized -- heavily! */
			/* GGO: how about the new monsters? */
			level = 1;

			// Tarmellion summoning works a little differently...
			if( Globals->TARMELLION_SUMMONING ) {
				if( !( ItemDefs[i->type].type & IT_MONSTER ) ) continue;

				int allowed = GetAllowedMonsters( u, i->type );
				if( allowed < 0 ) {
					// lose some monsters
					int lose = -allowed;
					if( lose > i->num ) lose = i->num;
					u->items.SetNum(i->type, i->num - lose);
					AString temp = AString( lose ) + " ";
					temp += (lose == 1?ItemDefs[i->type].name:ItemDefs[i->type].names);
					temp += " escape.";
					u->Event( temp );
				}
				continue;
			}

			if (i->type == I_IMP || i->type == I_DEMON || i->type == I_BALROG) {
				top = i->num * i->num;
				if (i->type == I_IMP) skill = S_SUMMON_IMPS;
				if (i->type == I_DEMON) skill = S_SUMMON_DEMON;
				if (i->type == I_BALROG) skill = S_SUMMON_BALROG;
				level = u->GetSkill(skill);
				if (!level) {
					/* Something does escape */
					escape = 10000;
				} else {
					int bottom = level * level;
					if (i->type == I_IMP) bottom *= 4;
					bottom = bottom * bottom;
					if (i->type != I_BALROG) bottom *= 20;
					else bottom *= 4;
					int chance = (top * 10000)/bottom;
					if (chance > escape) escape = chance;
				}
			}

			if (i->type==I_SKELETON || i->type==I_UNDEAD || i->type==I_LICH) {
				int losses = (i->num + getrandom(10)) / 10;
				u->items.SetNum(i->type,i->num - losses);
				totlosses += losses;
			}

			if (i->type==I_WOLF || i->type==I_EAGLE || 
                            i->type==I_STEELDRAGON || i->type==I_COPPERDRAGON || i->type==I_BRONZEDRAGON || i->type==I_SILVERDRAGON || i->type==I_GOLDENDRAGON ||
                            i->type==I_GREENDRAGON || i->type==I_BROWNDRAGON || i->type==I_BLUEDRAGON || i->type==I_REDDRAGON || i->type==I_WHITEDRAGON || i->type==I_BLACKDRAGON ) {
				if (i->type == I_WOLF) skill = S_WOLF_LORE;
				if (i->type == I_GIANTEAGLE) skill = S_BIRD_LORE;
				if (i->type == I_STEELDRAGON) skill = S_SUMMON_DRAGON;
				if (i->type == I_COPPERDRAGON) skill = S_SUMMON_DRAGON;
				if (i->type == I_BRONZEDRAGON) skill = S_SUMMON_DRAGON;
				if (i->type == I_SILVERDRAGON) skill = S_SUMMON_DRAGON;
				if (i->type == I_GOLDENDRAGON) skill = S_SUMMON_DRAGON;
				if (i->type == I_GREENDRAGON) skill = S_SUMMON_WYRM;
				if (i->type == I_BROWNDRAGON) skill = S_SUMMON_WYRM;
				if (i->type == I_BLUEDRAGON) skill = S_SUMMON_WYRM;
				if (i->type == I_REDDRAGON) skill = S_SUMMON_WYRM;
				if (i->type == I_WHITEDRAGON) skill = S_SUMMON_WYRM;
				if (i->type == I_BLACKDRAGON) skill = S_SUMMON_WYRM;
				level = u->GetSkill(skill);
				if (!level) {
					if (Globals->WANDERING_MONSTERS_EXIST &&
							Globals->RELEASE_MONSTERS) {
						Faction *mfac = GetFaction(&factions, monfaction);
						Unit *mon = GetNewUnit(mfac, 0);
						int mondef = ItemDefs[i->type].index;
						mon->MakeWMon(MonDefs[mondef].name, i->type, i->num);
						mon->MoveUnit(r->GetDummy());
						// This will be zero unless these are set. (0 means
						// full spoils)
						mon->free = Globals->MONSTER_NO_SPOILS +
							Globals->MONSTER_SPOILS_RECOVERY;
					}
					u->Event(AString("Loses control of ") +
							ItemString(i->type, i->num) + ".");
					u->items.SetNum(i->type, 0);
				}
			}
		}

		if (totlosses) {
			u->Event(AString(totlosses) + " undead decay into nothingness.");
		}

		if (escape > getrandom(10000)) {
			if (!Globals->WANDERING_MONSTERS_EXIST) {
				u->items.SetNum(I_IMP, 0);
				u->items.SetNum(I_DEMON, 0);
				u->items.SetNum(I_BALROG, 0);
				u->Event("Summoned demons vanish.");
			} else {
				Faction *mfac = GetFaction(&factions,monfaction);
				if (u->items.GetNum(I_IMP)) {
					Unit *mon = GetNewUnit(mfac, 0);
					mon->MakeWMon(MonDefs[MONSTER_IMPS].name,I_IMP,
							u->items.GetNum(I_IMP));
					mon->MoveUnit(r->GetDummy());
					u->items.SetNum(I_IMP,0);
					// This will be zero unless these are set. (0 means
					// full spoils)
					mon->free = Globals->MONSTER_NO_SPOILS +
						Globals->MONSTER_SPOILS_RECOVERY;
				}
				if (u->items.GetNum(I_DEMON)) {
					Unit *mon = GetNewUnit(mfac, 0);
					mon->MakeWMon(MonDefs[MONSTER_DEMONS].name,I_DEMON,
							u->items.GetNum(I_DEMON));
					mon->MoveUnit(r->GetDummy());
					u->items.SetNum(I_DEMON,0);
					// This will be zero unless these are set. (0 means
					// full spoils)
					mon->free = Globals->MONSTER_NO_SPOILS +
						Globals->MONSTER_SPOILS_RECOVERY;
				}
				if (u->items.GetNum(I_BALROG)) {
					Unit *mon = GetNewUnit(mfac, 0);
					mon->MakeWMon(MonDefs[MONSTER_BALROG].name,I_BALROG,
							u->items.GetNum(I_BALROG));
					mon->MoveUnit(r->GetDummy());
					u->items.SetNum(I_BALROG,0);
					// This will be zero unless these are set. (0 means
					// full spoils)
					mon->free = Globals->MONSTER_NO_SPOILS +
						Globals->MONSTER_SPOILS_RECOVERY;
				}
				u->Event("Controlled demons break free!");
			}
		}
	}
}

void Game::CheckUnitMaintenance(int consume) {
	int max = -1;
	int lastmax = -1;
	// Find maximum priority.
	for (int it=0;it<NITEMS;it++) {
		if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
			if (max < 0 || ItemDefs[it].pValue > max) max = ItemDefs[it].pValue;
		}
	}
	// While priority found
	while (max >= 0) {
		lastmax = max;
		max = -1;
		for (int it=0;it<NITEMS;it++) {
			if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
				// Consume items of present priority.
				if (ItemDefs[it].pValue == lastmax)
					CheckUnitMaintenanceItem(it, Globals->UPKEEP_FOOD_VALUE, consume);
				// Find next priority.
				if (ItemDefs[it].pValue > max && ItemDefs[it].pValue < lastmax) 
					max = ItemDefs[it].pValue;
			}
		}
	}
}

void Game::CheckFactionMaintenance(int con) {
	int max = -1;
	int lastmax = -1;
	// Find maximum priority.
	for (int it=0;it<NITEMS;it++) {
		if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
			if (max < 0 || ItemDefs[it].pValue > max)
				max = ItemDefs[it].pValue;
		}
	}
	// While priority found
	while (max >= 0) {
		lastmax = max;
		max = -1;
		for (int it=0;it<NITEMS;it++) {
			if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
				// Consume items of present priority.
				if (ItemDefs[it].pValue == lastmax)
					CheckFactionMaintenanceItem(it, Globals->UPKEEP_FOOD_VALUE, con);
				// Find next priority.
				if (ItemDefs[it].pValue > max && ItemDefs[it].pValue < lastmax)
					max = ItemDefs[it].pValue;
			}
		}
	}
}

void Game::CheckAllyMaintenance() {
	int max = -1;
	int lastmax = -1;
	// Find maximum priority.
	for (int it=0;it<NITEMS;it++) {
		if (ItemDefs[it].attributes & ItemType::CAN_CONSUME)
			if (max < 0 || ItemDefs[it].pValue > max) max = ItemDefs[it].pValue;
	}
	// While priority found
	while (max >= 0) {
		lastmax = max;
		max = -1;
		for (int it=0;it<NITEMS;it++) {
			if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
				// Consume items of present priority.
				if (ItemDefs[it].pValue == lastmax)
					CheckAllyMaintenanceItem(it, Globals->UPKEEP_FOOD_VALUE);
				// Find next priority.
				if (ItemDefs[it].pValue > max && ItemDefs[it].pValue < lastmax)
					max = ItemDefs[it].pValue;
			}
		}
	}
}

void Game::CheckUnitHunger() {
	int max = -1;
	int lastmax = -1;
	// Find maximum priority.
	for (int it=0;it<NITEMS;it++) {
		if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
			if (max < 0 || ItemDefs[it].pValue > max)
				max = ItemDefs[it].pValue;
		}
	}
	// While priority found
	while (max >= 0) {
		lastmax = max;
		max = -1;
		for (int it=0;it<NITEMS;it++) {
			if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
				// Consume items of present priority.
				if (ItemDefs[it].pValue == lastmax)
					CheckUnitHungerItem(it, Globals->UPKEEP_FOOD_VALUE);
				// Find next priority.
				if (ItemDefs[it].pValue > max && ItemDefs[it].pValue < lastmax)
					max = ItemDefs[it].pValue;
			}
		}
	}
}

void Game::CheckFactionHunger() {
	int max = -1;
	int lastmax = -1;
	// Find maximum priority.
	for (int it=0;it<NITEMS;it++) {
		if (ItemDefs[it].attributes & ItemType::CAN_CONSUME)
			if (max < 0 || ItemDefs[it].pValue > max)
				max = ItemDefs[it].pValue;
	}
	// While priority found
	while (max >= 0) {
		lastmax = max;
		max = -1;
		for (int it=0;it<NITEMS;it++) {
			if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
				// Consume items of present priority.
				if (ItemDefs[it].pValue == lastmax)
					CheckFactionHungerItem(it, Globals->UPKEEP_FOOD_VALUE);
				// Find next priority.
				if (ItemDefs[it].pValue > max && ItemDefs[it].pValue < lastmax)
					max = ItemDefs[it].pValue;
			}
		}
	}
}

void Game::CheckAllyHunger() {
	int max = -1;
	int lastmax = -1;
	// Find maximum priority.
	for (int it=0;it<NITEMS;it++) {
		if (ItemDefs[it].attributes & ItemType::CAN_CONSUME)
			if (max < 0 || ItemDefs[it].pValue > max)
				max = ItemDefs[it].pValue;
	}
	// While priority found
	while (max >= 0) {
		lastmax = max;
		max = -1;
		for (int it=0;it<NITEMS;it++) {
			if (ItemDefs[it].attributes & ItemType::CAN_CONSUME) {
				// Consume items of present priority.
				if (ItemDefs[it].pValue == lastmax)
					CheckAllyHungerItem(it, Globals->UPKEEP_FOOD_VALUE);
				// Find next priority.
				if (ItemDefs[it].pValue > max && ItemDefs[it].pValue < lastmax)
					max = ItemDefs[it].pValue;
			}
		}
	}
}

char Game::GetRChar(ARegion * r) {
	int t = r->type;
	char c;
	switch (t) {
		case R_OCEAN:
		case R_T_OCEAN1:
		case R_LAKE:
		case R_T_LAKE3:
		case R_CE_OCEAN:
		case R_CE_OCEAN1:
			 return ' ';
		case R_PLAIN:
		case R_ISLAND_PLAIN:
		case R_T_PLAIN1:
		case R_T_PLAIN2:
		case R_T_PLAIN3:
		case R_CE_GDPLAIN:
		case R_CE_GDPLAIN1:
			c = 'p'; break;
		case R_T_PLAIN4:
		case R_T_PLAIN5:
		case R_T_PLAIN6:
		case R_CE_EVPLAIN:
		case R_CE_EVPLAIN1:
			c = 'q'; break;
		case R_CE_GDGRASSLAND:
		case R_CE_GDGRASSLAND1:
			c = 'g'; break;
		case R_CE_EVGRASSLAND:
		case R_CE_EVGRASSLAND1:
			c = 'a'; break;
		case R_FOREST:
		case R_T_FOREST1:
		case R_T_FOREST2:
		case R_T_FOREST3:
		case R_CE_GDFOREST:
		case R_CE_GDFOREST1:
		case R_UFOREST:
		case R_T_UNDERFOREST1:
		case R_T_UNDERFOREST2:
		case R_T_UNDERFOREST3:
		case R_CE_GDUFOREST:
		case R_CE_GDDFOREST:
		case R_CE_GDDFOREST1:
		case R_CE_GDUFOREST1:
		case R_T_MYSTFOREST1:
		case R_CE_MYSTFOREST:
		case R_CE_MYSTFOREST1:
			c = 'f'; break;
		case R_T_FOREST4:
		case R_T_FOREST5:
		case R_T_FOREST6:
		case R_CE_EVFOREST:
		case R_CE_EVFOREST1:
		case R_T_DEEPFOREST1:
		case R_T_DEEPFOREST2:
		case R_T_DEEPFOREST3:
		case R_CE_EVUFOREST:
		case R_CE_EVUFOREST1:
		case R_CE_EVDFOREST:
		case R_CE_EVDFOREST1:
		case R_T_MYSTFOREST2:
			c = 'e'; break;
		case R_MOUNTAIN:
		case R_ISLAND_MOUNTAIN:
		case R_T_MOUNTAIN1:
		case R_T_MOUNTAIN2:
		case R_T_MOUNTAIN3:
		case R_CE_GDMOUNTAIN:
		case R_CE_GDMOUNTAIN1:
		case R_T_MINES1:
		case R_T_MINES2:
		case R_T_MINES3:
			c = 'm'; break;
		case R_T_MOUNTAIN4:
		case R_T_MOUNTAIN5:
		case R_T_MOUNTAIN6:
		case R_CE_EVMOUNTAIN:
		case R_CE_EVMOUNTAIN1:
		case R_T_MINES4:
		case R_T_MINES5:
		case R_T_MINES6:
			c = 'n'; break;
		case R_T_HILL1:
		case R_T_HILL2:
		case R_T_HILL3:
		case R_CE_GDHILL:
		case R_CE_GDHILL1:
			c = 'h'; break;
		case R_T_HILL4:
		case R_T_HILL5:
		case R_T_HILL6:
		case R_CE_EVHILL:
		case R_CE_EVHILL1:
			c = 'k'; break;
		case R_SWAMP:
		case R_ISLAND_SWAMP:
		case R_T_SWAMP1:
		case R_T_SWAMP2:
		case R_T_SWAMP3:
		case R_CE_GDSWAMP:
		case R_CE_GDSWAMP1:
			c = 's'; break;
		case R_T_SWAMP4:
		case R_T_SWAMP5:
		case R_T_SWAMP6:
		case R_CE_EVSWAMP:
		case R_CE_EVSWAMP1:
			c = 'r'; break;
		case R_JUNGLE:
		case R_T_JUNGLE1:
		case R_T_JUNGLE2:
		case R_T_JUNGLE3:
		case R_CE_GDJUNGLE:
		case R_CE_GDJUNGLE1:
			c = 'j'; break;
		case R_T_JUNGLE4:
		case R_T_JUNGLE5:
		case R_T_JUNGLE6:
		case R_CE_EVJUNGLE:
		case R_CE_EVJUNGLE1:
			c = 'o'; break;
		case R_DESERT:
		case R_T_DESERT1:
		case R_T_DESERT2:
		case R_T_DESERT3:
		case R_CE_GDDESERT:
		case R_CE_GDDESERT1:
			c = 'd'; break;
		case R_T_DESERT4:
		case R_T_DESERT5:
		case R_T_DESERT6:
		case R_CE_EVDESERT:
		case R_CE_EVDESERT1:
		case R_CE_WASTELAND:
		case R_CE_WASTELAND1:
			c = 'z'; break;
		case R_T_LAKE1:
		case R_T_LAKE2:
		case R_CE_GDLAKE:
		case R_CE_NELAKE:
		case R_CE_EVLAKE:
		case R_CE_GDULAKE:
		case R_CE_NEULAKE:
		case R_CE_EVULAKE:
			c = 'l'; break;
		case R_TUNDRA:
		case R_T_TUNDRA1:
		case R_T_TUNDRA2:
		case R_T_TUNDRA3:
		case R_CE_GDTUNDRA:
		case R_CE_GDTUNDRA1:
			c = 't'; break;
		case R_T_TUNDRA4:
		case R_T_TUNDRA5:
		case R_T_TUNDRA6:
		case R_CE_EVTUNDRA:
		case R_CE_EVTUNDRA1:
		case R_T_CHAMBER1:
		case R_T_CHAMBER2:
		case R_T_CHAMBER3:
			c = 'u'; break;
		case R_CAVERN:
		case R_T_CAVERN1:
		case R_T_CAVERN2:
		case R_T_CAVERN3:
		case R_CE_GDCAVERN:
		case R_CE_GDCAVERN1:
		case R_CE_GDDCAVERN:
		case R_CE_GDDCAVERN1:
			c = 'c'; break;
		case R_T_CAVERN4:
		case R_T_CAVERN5:
		case R_T_CAVERN6:
		case R_CE_EVCAVERN:
		case R_CE_EVCAVERN1:
		case R_CE_EVDCAVERN:
		case R_CE_EVDCAVERN1:
			c = 'h'; break;
		case R_TUNNELS:
		case R_T_TUNNELS1:
		case R_T_TUNNELS2:
		case R_T_TUNNELS3:
		case R_T_TUNNELS4:
		case R_T_TUNNELS5:
		case R_T_TUNNELS6:
		case R_CE_TUNNELS:
		case R_CE_TUNNELS1:
		case R_CE_DTUNNELS:
		case R_CE_DTUNNELS1:
			c = 't'; break;
		case R_VOLCANO:
		case R_T_VOLCANO1:
		case R_T_VOLCANO2:
		case R_T_CHAMBER4:
		case R_T_CHAMBER5:
		case R_T_CHAMBER6:
			c = 'v'; break;
		case R_T_GROTTO1:
		case R_T_GROTTO2:
			c = 'g'; break;
		case R_CE_ICE:
		case R_CE_GLACIER:
			c = 'i'; break;
		case R_CE_CRYSTALCAVERN:
		case R_CE_BLUECAVERN:
		case R_CE_REDCAVERN:
		case R_CE_YELLOWCAVERN:
		case R_CE_ORANGECAVERN:
		case R_CE_GREENCAVERN:
		case R_CE_VIOLETCAVERN:
		case R_CE_BLACKCAVERN:
		case R_CE_WHITECAVERN:
			c = 'x'; break;

		default: return '?';
	}
	if (r->town) c = (c - 'a') + 'A';
	return c;
}

void Game::CreateNPCFactions() {
	Faction *f;
	AString *temp;
	if (Globals->CITY_MONSTERS_EXIST) {
		f = new Faction(factionseq++);
		guardfaction = f->num;
		temp = new AString("The Guardsmen");
		f->SetName(temp);
		f->SetNPC();
		f->lastorders = 0;
		factions.Add(f);
	}
	// Only create the monster faction if wandering monsters or lair
	// monsters exist.
	if (Globals->LAIR_MONSTERS_EXIST || Globals->WANDERING_MONSTERS_EXIST) {
		f = new Faction(factionseq++);
		monfaction = f->num;
		temp = new AString("Creatures");
		f->SetName(temp);
		f->SetNPC();
		f->lastorders = 0;
		factions.Add(f);
	}
}

void Game::CreateCityMon(ARegion *pReg, int percent, int needmage) {
	int skilllevel;
	int AC = 0;
	int IV = 0;
	int num;
	if (pReg->type == R_NEXUS || pReg->IsStartingCity()) {
		skilllevel = TOWN_CITY + 1;
		num = Globals->AMT_START_CITY_GUARDS;
		if (Globals->SAFE_START_CITIES || (pReg->type == R_NEXUS) || (TerrainDefs[pReg->type].flags&TerrainType::ISNEXUS)) {
			IV = 1;
			if (TerrainDefs[pReg->type].flags&TerrainType::ISNEXUS)
				if (pReg->xloc==0&&pReg->yloc==0) num=100;
				else num=10;
		}
		AC = 1;
	} else {
		skilllevel = pReg->town->TownType() + 1;
		num = Globals->CITY_GUARD * skilllevel;
	}
	if (pReg->race != -1) {
		AString *s;
		num = num * percent / 100;
		Faction *pFac = GetFaction(&factions, guardfaction);
		Unit *u = GetNewUnit(pFac);
		if (IV) s = new AString("Peacekeepers");
		else s = new AString("City Guard");
		u->SetName(s);
		u->type = U_GUARD;
		u->guard = GUARD_GUARD;
		int race;
		int *gitems;
		if (Globals->LEADERS_EXIST == GameDefs::RACIAL_LEADERS) {
			race = pReg->race;
			int leader = ManDefs[ItemDefs[race].index].minority;
			race = (leader == -1 ? race : leader);
		} else if (Globals->LEADERS_EXIST == GameDefs::NORMAL_LEADERS)
			race = I_LEADERS;
		else
			race = pReg->race;
		gitems = ManDefs[ItemDefs[race].index].guarditems;
		u->SetMen(race,num);
		if (IV) u->items.SetNum(I_AMULETOFI,num);
		u->SetMoney(num * Globals->GUARD_MONEY);
		u->SetSkill(S_COMBAT,skilllevel);
		if (AC) {
			// weapons
			if (gitems[5]!=-1) {
				u->items.SetNum(gitems[5],num);
				//Awrite("setting weapon: gitems[5]");
				//Awrite(gitems[5]);
			} else if (gitems[0]!=-1) {
				u->items.SetNum(gitems[0],num);
				//Awrite("setting weapon: gitems[0]");
				//Awrite(gitems[0]);
			} else { 
				u->items.SetNum(I_SWORD, num);
				//Awrite("setting weapon: I_SWORD");
			}
			// armor
			if (Globals->START_CITY_GUARDS_PLATE) {
				if (gitems[6]!=-1) {
					u->items.SetNum(gitems[6],num);
					//Awrite("setting armor: gitems[6]");
					//Awrite(gitems[6]);
				} else if (gitems[1]!=-1) {
					u->items.SetNum(gitems[1],num);
					//Awrite("setting armor: gitems[1]");
					//Awrite(gitems[1]);
				} else {
					u->items.SetNum(I_IRONPLATEARMOR, num);
					//Awrite("setting armor: I_IRONPLATEARMOR");
				}
			}
			// equiptment 3 (starting city guards only)
			if (gitems[4]!=-1) {
				u->items.SetNum(gitems[4],num);
				//Awrite("setting city guard equiptment: gitems[4]");
				//Awrite(gitems[4]);
			}
			u->SetSkill(S_OBSERVATION,10);
			if (Globals->START_CITY_TACTICS)
				u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);
		} else {
			// weapon
			if (gitems[0]!=-1) {
				u->items.SetNum(gitems[0],num);
				//Awrite("setting weapon: gitems[0]");
				//Awrite(gitems[0]);
			} else {
				u->items.SetNum(I_SWORD, num);
				//Awrite("setting weapon: I_SWORD");
			}
			// armor
			if (Globals->START_CITY_GUARDS_PLATE)
				if (gitems[1]!=-1) {
					u->items.SetNum(gitems[1],num);
					//Awrite("setting armor: gitems[1]");
					//Awrite(gitems[1]);
				}
			u->SetSkill(S_OBSERVATION,skilllevel);
		}
		// equiptment 1 (shield if introduced)
		if (gitems[2]!=-1) {
			u->items.SetNum(gitems[2],num);
			//Awrite("setting shield: gitems[2]");
			//Awrite(gitems[2]);
		}
		// equitement 2
		if (gitems[3]!=-1) {
			u->items.SetNum(gitems[3],num);
			//Awrite("setting equiptmentz: gitems[3]");
			//Awrite(gitems[3]);
		}
		u->SetFlag(FLAG_HOLDING,1);
		u->MoveUnit(pReg->GetDummy());

		if (AC && Globals->START_CITY_MAGES && needmage) {
			u = GetNewUnit(pFac);
			s = new AString("City Mage");
			u->SetName(s);
			u->type = U_GUARDMAGE;
			u->SetMen(I_LEADERS,1);
			if (IV) u->items.SetNum(I_AMULETOFI,1);
			u->SetMoney(Globals->GUARD_MONEY);
			u->SetSkill(S_FORCE,Globals->START_CITY_MAGES);
			u->SetSkill(S_FIRE,Globals->START_CITY_MAGES);
			if (Globals->START_CITY_TACTICS)
				u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);
			u->combat = S_FIRE;
			u->SetFlag(FLAG_BEHIND, 1);
			u->SetFlag(FLAG_HOLDING, 1);
			u->MoveUnit(pReg->GetDummy());
		}
	}
}

void Game::AdjustCityMons(ARegion *r) {
	int needguard = 1;
	int needmage = 1;
	forlist(&r->objects) {
		Object * o = (Object *) elem;
		forlist(&o->units) {
			Unit * u = (Unit *) elem;
			if (u->type == U_GUARD || u->type == U_GUARDMAGE) {
				AdjustCityMon(r, u);
				/* Don't create new city guards if we have some */
				needguard = 0;
				if (u->type == U_GUARDMAGE)
					needmage = 0;
			}
			if (u->guard == GUARD_GUARD) needguard = 0;
		}
	}

	if (needguard && (getrandom(100) < Globals->GUARD_REGEN))
		CreateCityMon(r, 10, needmage);
}

void Game::AdjustCityMon(ARegion *r, Unit *u) {
	int towntype;
	int AC = 0;
	int men;
	int IV = 0;
	if (r->type == R_NEXUS || r->IsStartingCity()) {
		towntype = TOWN_CITY;
		AC = 1;
		if (u->type == U_GUARDMAGE)
			men = 1;
		else {
			men = u->GetMen() + (Globals->AMT_START_CITY_GUARDS/10);
			if (men > Globals->AMT_START_CITY_GUARDS)
				men = Globals->AMT_START_CITY_GUARDS;
		}
		if (Globals->SAFE_START_CITIES || (r->type == R_NEXUS) || (TerrainDefs[r->type].flags&TerrainType::ISNEXUS)) {
			IV = 1;
			if (TerrainDefs[r->type].flags&TerrainType::ISNEXUS)
				if (r->xloc==0&&r->yloc==0) men=100;
				else men=10;
		}
	} else {
		towntype = r->town->TownType();
		men = u->GetMen() + (Globals->CITY_GUARD/10)*(towntype+1);
		if (men > Globals->CITY_GUARD * (towntype+1))
			men = Globals->CITY_GUARD * (towntype+1);
	}

	int race;
	int *gitems;
	if (Globals->LEADERS_EXIST == GameDefs::RACIAL_LEADERS) {
		race = r->race;
		int leader = ManDefs[ItemDefs[race].index].minority;
		race = (leader == -1 ? race : leader);
	} else if (Globals->LEADERS_EXIST == GameDefs::NORMAL_LEADERS)
		race = I_LEADERS;
	else
		race = r->race;
	gitems = ManDefs[ItemDefs[race].index].guarditems;
	u->SetMen(race,men);

	if (IV) u->items.SetNum(I_AMULETOFI,men);

	if (u->type == U_GUARDMAGE) {
		if (Globals->START_CITY_TACTICS)
			u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);
		u->SetSkill(S_FORCE, Globals->START_CITY_MAGES);
		u->SetSkill(S_FIRE, Globals->START_CITY_MAGES);
		u->combat = S_FIRE;
		u->SetFlag(FLAG_BEHIND, 1);
		u->SetMoney(Globals->GUARD_MONEY);
	} else {
		u->SetMoney(men * Globals->GUARD_MONEY);
		u->SetSkill(S_COMBAT,towntype + 1);
		if (AC) {
			// weapon
			if (gitems[5]!=-1) {
				u->items.SetNum(gitems[5],men);
				//Awrite("setting weapon: gitems[5]");
				//Awrite(gitems[5]);
			} else if (gitems[0]!=-1) {
				u->items.SetNum(gitems[0],men);
				//Awrite("setting weapon: gitems[0]");
				//Awrite(gitems[0]);
			} else {
				u->items.SetNum(I_SWORD, men);
				//Awrite("setting weapon: I_SWORD");
			}
			// armor
			if (Globals->START_CITY_GUARDS_PLATE) {
				if (gitems[6]!=-1) {
					u->items.SetNum(gitems[6],men);
					//Awrite("setting armor: gitems[6]");
					//Awrite(gitems[6]);
				} else if (gitems[1]!=-1) {
					u->items.SetNum(gitems[1],men);
					//Awrite("setting armor: gitems[1]");
					//Awrite(gitems[1]);
				} else {
					u->items.SetNum(I_IRONPLATEARMOR, men);
					//Awrite("setting armor: I_IRONPLATEARMOR");
				}
			}
			// equiptment 3 (starting city guards only)
			if (gitems[4]!=-1) {
				u->items.SetNum(gitems[4],men);
				//Awrite("setting city guard equiptment: gitems[4]");
				//Awrite(gitems[4]);
			}
			u->SetSkill(S_OBSERVATION,10);
			if (Globals->START_CITY_TACTICS)
				u->SetSkill(S_TACTICS, Globals->START_CITY_TACTICS);

		} else {
			// weapon
			if (gitems[0]!=-1) {
				u->items.SetNum(gitems[0],men);
				//Awrite("setting weapon: gitems[0]");
				//Awrite(gitems[0]);
			} else {
				u->items.SetNum(I_SWORD, men);
				//Awrite("setting weapon: I_SWORD");
			}
			// armor
			if (Globals->START_CITY_GUARDS_PLATE)
				if (gitems[1]!=-1) {
					u->items.SetNum(gitems[1],men);
					//Awrite("setting armor: gitems[1]");
					//Awrite(gitems[1]);
				}
			u->SetSkill(S_OBSERVATION,towntype + 1);
		}
		// equiptment 1 (shield if introduced)
		if (gitems[2]!=-1) {
			u->items.SetNum(gitems[2],men);
			//Awrite("setting shield: gitems[2]");
			//Awrite(gitems[2]);
		}
		// equitement 2
		if (gitems[3]!=-1) {
			u->items.SetNum(gitems[3],men);
			//Awrite("setting equiptmentz: gitems[3]");
			//Awrite(gitems[3]);
		}
	}
}
