#ifdef PRECOMPILEDHEADERS
	#include "AI All.h"
#else
	#include "ai.h"
	#include "AIInternals.h"
	#include "opplist.h"
	#include "Soldier Profile.h"
	#include "Quests.h"
#endif

void CallAvailableEnemiesTo( INT16 sGridNo )
{
	INT32	iLoop;
	INT32	iLoop2;
	SOLDIERTYPE * pSoldier;

	// All enemy teams become aware of a very important "noise" coming from here!
	for (iLoop = 0; iLoop < LAST_TEAM; iLoop++)
	{
		// if this team is active
		if (gTacticalStatus.Team[iLoop].bTeamActive)
		{
			// if this team is computer-controlled, and isn't the CIVILIAN "team"
			if (!(gTacticalStatus.Team[iLoop].bHuman) && (iLoop != CIV_TEAM))
			{
				// make this team (publicly) aware of the "noise"
				gsPublicNoiseGridno[iLoop] = sGridNo;
				gubPublicNoiseVolume[iLoop] = MAX_MISC_NOISE_DURATION;

				// new situation for everyone;
				iLoop2 = gTacticalStatus.Team[ iLoop ].bFirstID;
				for ( pSoldier = MercPtrs[iLoop2]; iLoop2 <= gTacticalStatus.Team[ iLoop ].bLastID; iLoop2++, pSoldier++ )
				{
					if (pSoldier->bActive && pSoldier->bInSector && pSoldier->bLife >= OKLIFE)
					{
						SetNewSituation( pSoldier );
						WearGasMaskIfAvailable( pSoldier );
					}
				}
 
			}

		}

	}
}

void CallAvailableTeamEnemiesTo( INT16 sGridno, INT8 bTeam )
{
	INT32	iLoop2;
	SOLDIERTYPE * pSoldier;

	// All enemy teams become aware of a very important "noise" coming from here!
	// if this team is active
	if (gTacticalStatus.Team[bTeam].bTeamActive)
	{
		// if this team is computer-controlled, and isn't the CIVILIAN "team"
		if (!(gTacticalStatus.Team[bTeam].bHuman) && (bTeam != CIV_TEAM))
		{
			// make this team (publicly) aware of the "noise"
			gsPublicNoiseGridno[bTeam] = sGridno;
			gubPublicNoiseVolume[bTeam] = MAX_MISC_NOISE_DURATION;

			// new situation for everyone;
			iLoop2 = gTacticalStatus.Team[ bTeam ].bFirstID;
			for ( pSoldier = MercPtrs[iLoop2]; iLoop2 <= gTacticalStatus.Team[ bTeam ].bLastID; iLoop2++, pSoldier++ )
			{
				if (pSoldier->bActive && pSoldier->bInSector && pSoldier->bLife >= OKLIFE)
				{
					SetNewSituation( pSoldier );
					WearGasMaskIfAvailable( pSoldier );
				}
			}

		}
	}
}

void CallAvailableKingpinMenTo( INT16 sGridNo )
{
	// like call all enemies, but only affects civgroup KINGPIN guys with 
	// NO PROFILE

	INT32	iLoop2;
	SOLDIERTYPE * pSoldier;

	// All enemy teams become aware of a very important "noise" coming from here!
	// if this team is active
	if (gTacticalStatus.Team[CIV_TEAM].bTeamActive)
	{
		// make this team (publicly) aware of the "noise"
		gsPublicNoiseGridno[CIV_TEAM] = sGridNo;
		gubPublicNoiseVolume[CIV_TEAM] = MAX_MISC_NOISE_DURATION;

		// new situation for everyone...

		iLoop2 = gTacticalStatus.Team[ CIV_TEAM ].bFirstID;
		for ( pSoldier = MercPtrs[iLoop2]; iLoop2 <= gTacticalStatus.Team[ CIV_TEAM ].bLastID; iLoop2++, pSoldier++ )
		{
			if (pSoldier->bActive && pSoldier->bInSector && pSoldier->bLife >= OKLIFE && pSoldier->ubCivilianGroup == KINGPIN_CIV_GROUP && pSoldier->ubProfile == NO_PROFILE)
			{
				SetNewSituation( pSoldier );
			}
		}
	}
}

void CallEldinTo( INT16 sGridNo )
{
	// like call all enemies, but only affects Eldin
	SOLDIERTYPE * pSoldier;

	// Eldin becomes aware of a very important "noise" coming from here!
	// So long as he hasn't already heard a noise a sec ago...
	if (gTacticalStatus.Team[CIV_TEAM].bTeamActive)
	{
		// new situation for Eldin
		pSoldier = FindSoldierByProfileID( ELDIN, FALSE );
		if ( pSoldier && pSoldier->bActive && pSoldier->bInSector && pSoldier->bLife >= OKLIFE && (pSoldier->bAlertStatus == STATUS_GREEN || pSoldier->ubNoiseVolume < (MAX_MISC_NOISE_DURATION / 2) ) )
		{
			if ( SoldierToLocationLineOfSightTest( pSoldier, sGridNo, (UINT8) MaxDistanceVisible(), TRUE ) )
			{
				// sees the player now!
				TriggerNPCWithIHateYouQuote( ELDIN );
				SetNewSituation( pSoldier );
			}
			else
			{
				pSoldier->sNoiseGridno = sGridNo;
				pSoldier->ubNoiseVolume = MAX_MISC_NOISE_DURATION;
				pSoldier->bAlertStatus = STATUS_RED;
				if ( (pSoldier->bAction != AI_ACTION_GET_CLOSER) || CheckFact( FACT_MUSEUM_ALARM_WENT_OFF, 0 ) == FALSE )
				{
					CancelAIAction( pSoldier, TRUE );
					pSoldier->bNextAction = AI_ACTION_GET_CLOSER;
					pSoldier->usNextActionData = sGridNo;
					RESETTIMECOUNTER( pSoldier->AICounter, 100 );
				}
				// otherwise let AI handle this normally
//				SetNewSituation( pSoldier );
				// reduce any delay to minimal
			}
			SetFactTrue( FACT_MUSEUM_ALARM_WENT_OFF );
		}
	}
}


INT16 MostImportantNoiseHeard( SOLDIERTYPE *pSoldier, INT32 *piRetValue, BOOLEAN * pfClimbingNecessary, BOOLEAN * pfReachable )
{
	UINT32 uiLoop;
	INT8 * pbPersOL, * pbPublOL;
	INT16 *psLastLoc,*psNoiseGridNo;
	INT8 * pbNoiseLevel;
	INT8 *pbLastLevel;
	UINT8 *pubNoiseVolume;
	INT32 iDistAway;
	INT32	iNoiseValue, iBestValue = -10000;
	INT16 sBestGridNo = NOWHERE;
	INT8	bBestLevel = 0;
	INT16 sClimbingGridNo;
	BOOLEAN fClimbingNecessary = FALSE;
	SOLDIERTYPE * pTemp;	

	pubNoiseVolume = &gubPublicNoiseVolume[pSoldier->bTeam];
	psNoiseGridNo = &gsPublicNoiseGridno[pSoldier->bTeam];
	pbNoiseLevel = &gbPublicNoiseLevel[pSoldier->bTeam];

	psLastLoc = gsLastKnownOppLoc[pSoldier->ubID];

	// hang pointers at start of this guy's personal and public opponent opplists
	pbPersOL = pSoldier->bOppList;
	pbPublOL = gbPublicOpplist[pSoldier->bTeam];

	// look through this man's personal & public opplists for opponents heard
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pTemp = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pTemp || !pTemp->bLife)
			continue;          // next merc

		// if this merc is neutral/on same side, he's not an opponent
		if ( CONSIDERED_NEUTRAL( pSoldier, pTemp ) || (pSoldier->bSide == pTemp->bSide))
			continue;          // next merc

		pbPersOL = pSoldier->bOppList + pTemp->ubID;
		pbPublOL = gbPublicOpplist[pSoldier->bTeam] + pTemp->ubID;
		psLastLoc = gsLastKnownOppLoc[pSoldier->ubID] + pTemp->ubID;
		pbLastLevel = gbLastKnownOppLevel[pSoldier->ubID] + pTemp->ubID;

		// if this guy's been personally heard within last 3 turns
		if (*pbPersOL < NOT_HEARD_OR_SEEN)
		{
			// calculate how far this noise was, and its relative "importance"
			iDistAway = SpacesAway(pSoldier->sGridNo,*psLastLoc);
			iNoiseValue = (*pbPersOL) * iDistAway;               // always a negative number!

			if (iNoiseValue > iBestValue)
			{
				iBestValue = iNoiseValue;
				sBestGridNo = *psLastLoc;
				bBestLevel = *pbLastLevel;
			}
		}

		// if this guy's been publicly heard within last 3 turns
		if (*pbPublOL < NOT_HEARD_OR_SEEN)
		{
			// calculate how far this noise was, and its relative "importance"
			iDistAway = SpacesAway(pSoldier->sGridNo,gsPublicLastKnownOppLoc[pSoldier->bTeam][pTemp->ubID]);
			iNoiseValue = (*pbPublOL) * iDistAway;               // always a negative number!

			if (iNoiseValue > iBestValue)
			{
				iBestValue = iNoiseValue;
				sBestGridNo = gsPublicLastKnownOppLoc[pSoldier->bTeam][pTemp->ubID];
				bBestLevel = gbPublicLastKnownOppLevel[pSoldier->bTeam][pTemp->ubID];
			}
		}

	}

	// if any "misc. noise" was also heard recently
	if (pSoldier->sNoiseGridno != NOWHERE)
	{
		if ( pSoldier->bNoiseLevel != pSoldier->bLevel || PythSpacesAway( pSoldier->sGridNo, pSoldier->sNoiseGridno ) >= 6 || SoldierTo3DLocationLineOfSightTest( pSoldier, pSoldier->sNoiseGridno, pSoldier->bNoiseLevel, 0, (UINT8) MaxDistanceVisible(), FALSE ) == 0 )
		{
			// calculate how far this noise was, and its relative "importance"
			iDistAway = SpacesAway(pSoldier->sGridNo,pSoldier->sNoiseGridno);
			iNoiseValue = ((pSoldier->ubNoiseVolume / 2) - 6) * iDistAway;

			if (iNoiseValue > iBestValue)
			{
				iBestValue = iNoiseValue;
				sBestGridNo = pSoldier->sNoiseGridno;
				bBestLevel = pSoldier->bNoiseLevel;
			}
		}
		else
		{
			// we are there or near
			pSoldier->sNoiseGridno = NOWHERE;        // wipe it out, not useful anymore
			pSoldier->ubNoiseVolume = 0;
		}
	}


	// if any recent PUBLIC "misc. noise" is also known
	if ( (pSoldier->bTeam != CIV_TEAM) || ( pSoldier->ubCivilianGroup == KINGPIN_CIV_GROUP ) )
	{
	
		if (*psNoiseGridNo != NOWHERE)
		{
			// if we are NOT there (at the noise gridno)
			if ( *pbNoiseLevel != pSoldier->bLevel || PythSpacesAway( pSoldier->sGridNo, *psNoiseGridNo ) >= 6 || SoldierTo3DLocationLineOfSightTest( pSoldier, *psNoiseGridNo, *pbNoiseLevel, 0, (UINT8) MaxDistanceVisible(), FALSE ) == 0 )
			{
				// calculate how far this noise was, and its relative "importance"
				iDistAway = SpacesAway(pSoldier->sGridNo,*psNoiseGridNo);
				iNoiseValue = ((*pubNoiseVolume / 2) - 6) * iDistAway;

				if (iNoiseValue > iBestValue)
				{
					iBestValue = iNoiseValue;
					sBestGridNo = *psNoiseGridNo;
					bBestLevel = *pbNoiseLevel;
				}
			}
		}

	}

	if (sBestGridNo != NOWHERE && pfReachable )
	{
		*pfReachable = TRUE;

		// make civs not walk to noises outside their room if on close patrol/onguard
		if ( pSoldier->bOrders <= CLOSEPATROL && (pSoldier->bTeam == CIV_TEAM || pSoldier->ubProfile != NO_PROFILE ) )
		{
			UINT8	ubRoom, ubNewRoom;

			// any other combo uses the default of ubRoom == 0, set above
			if ( InARoom( pSoldier->usPatrolGrid[0], &ubRoom ) )
			{
				if ( !InARoom( pSoldier->usPatrolGrid[0], &ubNewRoom ) || ubRoom != ubNewRoom )
				{
					*pfReachable = FALSE;
				}
			}
		}

		if ( *pfReachable )
		{
			// if there is a climb involved then we should store the location 
			// of where we have to climb to instead
			sClimbingGridNo = GetInterveningClimbingLocation( pSoldier, sBestGridNo, bBestLevel, &fClimbingNecessary );
			if ( fClimbingNecessary )
			{
				if ( sClimbingGridNo == NOWHERE )
				{
					// can't investigate!
					*pfReachable = FALSE;
				}
				else
				{
					sBestGridNo = sClimbingGridNo;
					fClimbingNecessary = TRUE;
				}
			}
			else
			{
				fClimbingNecessary = FALSE;
			} 
		}
	}

	if ( piRetValue )
	{
		*piRetValue = iBestValue;
	}

	if ( pfClimbingNecessary )
	{
		*pfClimbingNecessary = fClimbingNecessary;
	}

#ifdef DEBUGDECISIONS
	if (sBestGridNo != NOWHERE)
		AINumMessage("MOST IMPORTANT NOISE HEARD FROM GRID #",sBestGridNo);
#endif

	return(sBestGridNo);
}


INT16 WhatIKnowThatPublicDont(SOLDIERTYPE *pSoldier, UINT8 ubInSightOnly)
{
	UINT8 ubTotal = 0;
	UINT32 uiLoop;
	INT8 *pbPersOL,*pbPublOL;
	SOLDIERTYPE * pTemp;

	// if merc knows of a more important misc. noise than his team does
	if (!(CREATURE_OR_BLOODCAT( pSoldier )) && (pSoldier->ubNoiseVolume > gubPublicNoiseVolume[pSoldier->bTeam]))
	{
		// the difference in volume is added to the "new info" total
		ubTotal += pSoldier->ubNoiseVolume - gubPublicNoiseVolume[pSoldier->bTeam];
	}

	// hang pointers at start of this guy's personal and public opponent opplists
	pbPersOL = &(pSoldier->bOppList[0]);
	pbPublOL = &(gbPublicOpplist[pSoldier->bTeam][0]);

	// for every opponent
//	for (iLoop = 0; iLoop < MAXMERCS; iLoop++,pbPersOL++,pbPublOL++)
//	{
//	pTemp = &(Menptr[iLoop]);


	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pTemp = MercSlots[ uiLoop ];
	
		// if this merc is inactive, at base, on assignment, or dead
		if (!pTemp)
		{
			continue;          // next merc
		}

		// if this merc is neutral/on same side, he's not an opponent
		if ( CONSIDERED_NEUTRAL( pSoldier, pTemp ) || (pSoldier->bSide == pTemp->bSide))
		{
			continue;          // next merc
		}

		pbPersOL = pSoldier->bOppList + pTemp->ubID;
		pbPublOL = gbPublicOpplist[pSoldier->bTeam] + pTemp->ubID;


		// if we're only interested in guys currently is sight, and he's not
		if (ubInSightOnly)
		{
			if ((*pbPersOL == SEEN_CURRENTLY) && (*pbPublOL != SEEN_CURRENTLY))
			{
				// just count the number of them
				ubTotal++;
			}
		}
		else
		{
			// add value of personal knowledge compared to public knowledge to total
			ubTotal += gubKnowledgeValue[*pbPublOL - OLDEST_HEARD_VALUE][*pbPersOL - OLDEST_HEARD_VALUE];
		}
	}

#ifdef DEBUGDECISIONS
	if (ubTotal > 0)
	{
		AINumMessage("WHAT I KNOW THAT PUBLIC DON'T = ",ubTotal);
	}
#endif

	return(ubTotal);
}
