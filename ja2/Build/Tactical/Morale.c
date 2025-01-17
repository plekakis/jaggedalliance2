#ifdef PRECOMPILEDHEADERS
	#include "Tactical All.h"
#else
	#include <stdlib.h>
	#include "Morale.h"
	#include "Overhead.h"
	#include "Soldier Profile.h"
	#include "dialogue control.h"
	#include "Map Screen Interface.h"
	#include "message.h"
	#include "assignments.h"
	#include "Strategic Movement.h"
	#include "Strategic Status.h"
	#include "SkillCheck.h"
	#include "drugs and alcohol.h"
	#include "StrategicMap.h"
	#include "Debug.h"
	#include "mapscreen.h"
#endif

#define MORALE_MOD_MAX 50		// morale *mod* range is -50 to 50, if you change this, check the decay formulas!

#define	DRUG_EFFECT_MORALE_MOD			150
#define	ALCOHOL_EFFECT_MORALE_MOD		160

#define HOURS_BETWEEN_STRATEGIC_DECAY 3

#define PHOBIC_LIMIT -20


// macros
#define SOLDIER_IN_SECTOR( pSoldier, sX, sY, bZ )		( !pSoldier->fBetweenSectors && ( pSoldier->sSectorX == sX ) && ( pSoldier->sSectorY == sY ) && ( pSoldier->bSectorZ == bZ ) )



MoraleEvent gbMoraleEvent[NUM_MORALE_EVENTS] =
{
	// TACTICAL = Short Term Effect, STRATEGIC = Long Term Effect
	{	TACTICAL_MORALE_EVENT,			+4},	//	MORALE_KILLED_ENEMY
	{ TACTICAL_MORALE_EVENT,		  -5},	//	MORALE_SQUADMATE_DIED,		// in same sector (not really squad)... IN ADDITION to strategic loss of morale
	{ TACTICAL_MORALE_EVENT,			-1},	//	MORALE_SUPPRESSED,				// up to 4 times per turn
	{ TACTICAL_MORALE_EVENT,			-2},	//	MORALE_AIRSTRIKE,
	{ TACTICAL_MORALE_EVENT,			+2},	//	MORALE_DID_LOTS_OF_DAMAGE,
	{ TACTICAL_MORALE_EVENT,			-3},	//	MORALE_TOOK_LOTS_OF_DAMAGE,
	{ STRATEGIC_MORALE_EVENT,			-5},	//	MORALE_KILLED_CIVILIAN,
	{ STRATEGIC_MORALE_EVENT,			+4},	//	MORALE_BATTLE_WON,
	{ STRATEGIC_MORALE_EVENT,			-5},	//	MORALE_RAN_AWAY,
	{ STRATEGIC_MORALE_EVENT,			+2},	//	MORALE_HEARD_BATTLE_WON,
	{ STRATEGIC_MORALE_EVENT,			-2},	//	MORALE_HEARD_BATTLE_LOST,
	{ STRATEGIC_MORALE_EVENT,			+5},	//	MORALE_TOWN_LIBERATED,
	{ STRATEGIC_MORALE_EVENT,			-5},	//	MORALE_TOWN_LOST,
	{ STRATEGIC_MORALE_EVENT,			+8},	//	MORALE_MINE_LIBERATED,
	{ STRATEGIC_MORALE_EVENT,			-8},	//	MORALE_MINE_LOST,
	{ STRATEGIC_MORALE_EVENT,			+3},	//	MORALE_SAM_SITE_LIBERATED,
	{ STRATEGIC_MORALE_EVENT,			-3},	//	MORALE_SAM_SITE_LOST,
	{ STRATEGIC_MORALE_EVENT,		 -15},	//	MORALE_BUDDY_DIED,
	{ STRATEGIC_MORALE_EVENT,			+5},	//	MORALE_HATED_DIED,
	{ STRATEGIC_MORALE_EVENT,			-5},	//	MORALE_TEAMMATE_DIED,			// not in same sector
	{ STRATEGIC_MORALE_EVENT,			+5},	//	MORALE_LOW_DEATHRATE,
	{ STRATEGIC_MORALE_EVENT,			-5},	//	MORALE_HIGH_DEATHRATE,
	{ STRATEGIC_MORALE_EVENT,			+2},	//	MORALE_GREAT_MORALE,
	{ STRATEGIC_MORALE_EVENT,			-2},	//	MORALE_POOR_MORALE,
	{ TACTICAL_MORALE_EVENT,		 -10},	//  MORALE_DRUGS_CRASH
	{ TACTICAL_MORALE_EVENT,		 -10},	//  MORALE_ALCOHOL_CRASH
	{ STRATEGIC_MORALE_EVENT,		 +15},	//  MORALE_MONSTER_QUEEN_KILLED
	{ STRATEGIC_MORALE_EVENT,		 +25},	//  MORALE_DEIDRANNA_KILLED
	{ TACTICAL_MORALE_EVENT,			-1},	//	MORALE_CLAUSTROPHOBE_UNDERGROUND,
	{ TACTICAL_MORALE_EVENT,			-5},	//	MORALE_INSECT_PHOBIC_SEES_CREATURE,
	{ TACTICAL_MORALE_EVENT,			-1},	//	MORALE_NERVOUS_ALONE,
	{	STRATEGIC_MORALE_EVENT,		  -5},	//	MORALE_MERC_CAPTURED,
	{ STRATEGIC_MORALE_EVENT,			-5},	//	MORALE_MERC_MARRIED,
	{ STRATEGIC_MORALE_EVENT,			+8},	//	MORALE_QUEEN_BATTLE_WON,
	{ STRATEGIC_MORALE_EVENT,			+5},	//  MORALE_SEX,
};

BOOLEAN gfSomeoneSaidMoraleQuote = FALSE;


INT8 GetMoraleModifier( SOLDIERTYPE * pSoldier )
{
	if (pSoldier->uiStatusFlags & SOLDIER_PC)
	{
		if (pSoldier->bMorale > 50)
		{
			// give +1 at 55, +3 at 65, up to +5 at 95 and above
			return( (pSoldier->bMorale - 45) / 10 );
		}
		else
		{
			// give penalties down to -20 at 0 (-2 at 45, -4 by 40...)
			return( (pSoldier->bMorale - 50) * 2 / 5 );
		}
	}
	else
	{
		// use AI morale
		switch( pSoldier->bAIMorale )
		{
			case MORALE_HOPELESS:
				return( -15 );
			case MORALE_WORRIED:
				return( -7 );
			case MORALE_CONFIDENT:
				return( 2 );
			case MORALE_FEARLESS:
				return( 5 );
			default:
				return( 0 );
		}

	}
}

void DecayTacticalMorale( SOLDIERTYPE * pSoldier )
{
	// decay the tactical morale modifier
	if (pSoldier->bTacticalMoraleMod != 0)
	{
		// decay the modifier!
		if (pSoldier->bTacticalMoraleMod > 0)
		{
			pSoldier->bTacticalMoraleMod = __max( 0, pSoldier->bTacticalMoraleMod - (8 - pSoldier->bTacticalMoraleMod / 10) );
		}
		else
		{
			pSoldier->bTacticalMoraleMod = __min( 0, pSoldier->bTacticalMoraleMod + (6 + pSoldier->bTacticalMoraleMod / 10) );
		}
	}
}

void DecayStrategicMorale( SOLDIERTYPE * pSoldier )
{
	// decay the modifier!
	if (pSoldier->bStrategicMoraleMod > 0)
	{
		pSoldier->bStrategicMoraleMod = __max( 0, pSoldier->bStrategicMoraleMod - (8 - pSoldier->bStrategicMoraleMod / 10) );
	}
	else
	{
		pSoldier->bStrategicMoraleMod = __min( 0, pSoldier->bStrategicMoraleMod + (6 + pSoldier->bStrategicMoraleMod / 10) );
	}
}

void DecayTacticalMoraleModifiers( void )
{
	SOLDIERTYPE * pSoldier;
	UINT8					ubLoop, ubLoop2;
	BOOLEAN				fHandleNervous;

	ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
	for ( pSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pSoldier++ )
	{	
		//if the merc is active, in Arulco
		// CJC: decay modifiers while asleep! or POW!
		if ( pSoldier->bActive && pSoldier->ubProfile != NO_PROFILE && 
															!(pSoldier->bAssignment == IN_TRANSIT ||
																	pSoldier->bAssignment == ASSIGNMENT_DEAD ) )
		{
			// only let morale mod decay if it is positive while merc is a POW
			if ( pSoldier->bAssignment == ASSIGNMENT_POW && pSoldier->bTacticalMoraleMod < 0 )
			{
				continue;
			}

			switch( gMercProfiles[ pSoldier->ubProfile ].bPersonalityTrait )
			{
				case CLAUSTROPHOBIC:
					if ( pSoldier->bSectorZ > 0 )
					{						
						// underground, no recovery... in fact, if tact morale is high, decay
						if ( pSoldier->bTacticalMoraleMod > PHOBIC_LIMIT )
						{
							HandleMoraleEvent( pSoldier, MORALE_CLAUSTROPHOBE_UNDERGROUND, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
						}
						continue;
					}
					break;
				case NERVOUS:
					if ( pSoldier->bMorale < 50 )
					{
						if (pSoldier->ubGroupID != 0 && PlayerIDGroupInMotion( pSoldier->ubGroupID ))
 						{
							if ( NumberOfPeopleInSquad( pSoldier->bAssignment ) == 1 )
							{
								fHandleNervous = TRUE;
							}
							else
							{
								fHandleNervous = FALSE;
							}
						}
						else if ( pSoldier->bActive && pSoldier->bInSector )
						{
							if ( DistanceToClosestFriend( pSoldier ) > NERVOUS_RADIUS )
							{
								fHandleNervous = TRUE;
							}
							else
							{
								fHandleNervous = FALSE;
							}
						}
						else 
						{
							// look for anyone else in same sector
							fHandleNervous = TRUE;
							for ( ubLoop2 = gTacticalStatus.Team[ gbPlayerNum ].bFirstID; ubLoop2 <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop2++ )
							{
								if ( MercPtrs[ ubLoop2 ] != pSoldier && MercPtrs[ ubLoop2 ]->bActive && MercPtrs[ ubLoop2 ]->sSectorX == pSoldier->sSectorX && MercPtrs[ ubLoop2 ]->sSectorY == pSoldier->sSectorY && MercPtrs[ ubLoop2 ]->bSectorZ == pSoldier->bSectorZ )
								{
									// found someone!
									fHandleNervous = FALSE;
									break;
								}
							}														
						}

						if ( fHandleNervous )
						{
							if ( pSoldier->bTacticalMoraleMod == PHOBIC_LIMIT )
							{
								// don't change morale
								continue;
							}

							// alone, no recovery... in fact, if tact morale is high, decay
							if ( !(pSoldier->usQuoteSaidFlags & SOLDIER_QUOTE_SAID_PERSONALITY) )
							{
								TacticalCharacterDialogue( pSoldier, QUOTE_PERSONALITY_TRAIT );
								pSoldier->usQuoteSaidFlags |= SOLDIER_QUOTE_SAID_PERSONALITY;
							}
							HandleMoraleEvent( pSoldier, MORALE_NERVOUS_ALONE, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
							continue;
						}
					}
			}

			DecayTacticalMorale( pSoldier );
			RefreshSoldierMorale( pSoldier );
		}
	}
}

void DecayStrategicMoraleModifiers( void )
{
	SOLDIERTYPE * pSoldier;
	UINT8					ubLoop;

	ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
	for ( pSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pSoldier++ )
	{	
		//if the merc is active, in Arulco
		// CJC: decay modifiers while asleep! or POW!
		if ( pSoldier->bActive && pSoldier->ubProfile != NO_PROFILE && 
															!(pSoldier->bAssignment == IN_TRANSIT ||
																pSoldier->bAssignment == ASSIGNMENT_DEAD ) )
		{
			// only let morale mod decay if it is positive while merc is a POW
			if ( pSoldier->bAssignment == ASSIGNMENT_POW && pSoldier->bStrategicMoraleMod < 0 )
			{
				continue;
			}

			DecayStrategicMorale( pSoldier );
			RefreshSoldierMorale( pSoldier );
		}
	}
}



void RefreshSoldierMorale( SOLDIERTYPE * pSoldier )
{
	INT32		iActualMorale;

	if ( pSoldier->fMercAsleep )
	{
		// delay this till later!
		return;
	}

	// CJC, April 19, 1999: added up to 20% morale boost according to progress
	iActualMorale = DEFAULT_MORALE + (INT32) pSoldier->bTeamMoraleMod + (INT32) pSoldier->bTacticalMoraleMod + (INT32) pSoldier->bStrategicMoraleMod + (INT32) (CurrentPlayerProgressPercentage() / 5);

	// ATE: Modify morale based on drugs....
	iActualMorale  += ( ( pSoldier->bDrugEffect[ DRUG_TYPE_ADRENALINE ] * DRUG_EFFECT_MORALE_MOD ) / 100 );
	iActualMorale  += ( ( pSoldier->bDrugEffect[ DRUG_TYPE_ALCOHOL ] * ALCOHOL_EFFECT_MORALE_MOD ) / 100 );

	iActualMorale = __min( 100, iActualMorale );
	iActualMorale = __max( 0, iActualMorale );
	pSoldier->bMorale = (INT8) iActualMorale;

	// update mapscreen as needed
	fCharacterInfoPanelDirty = TRUE; 
}


void UpdateSoldierMorale( SOLDIERTYPE * pSoldier, UINT8 ubType, INT8 bMoraleMod )
{
	MERCPROFILESTRUCT *		pProfile;
	INT32									iMoraleModTotal;

	if ( !pSoldier->bActive || ( pSoldier->bLife < CONSCIOUSNESS ) ||
		 ( pSoldier->uiStatusFlags & SOLDIER_VEHICLE ) || AM_A_ROBOT( pSoldier ) || AM_AN_EPC( pSoldier ) )
	{
		return;
	}

	if ( ( pSoldier->bAssignment == ASSIGNMENT_DEAD ) ||
			 ( pSoldier->bAssignment == ASSIGNMENT_POW ) ||
			 ( pSoldier->bAssignment == IN_TRANSIT ) )
	{
		return;
	}


	if ( pSoldier->ubProfile == NO_PROFILE )
	{
		return;
	}

	pProfile = &(gMercProfiles[ pSoldier->ubProfile ]);

	if (bMoraleMod > 0)
	{
		switch( pProfile->bAttitude )
		{
			case ATT_OPTIMIST:
			case ATT_AGGRESSIVE:
				bMoraleMod += 1;
				break;
			case ATT_PESSIMIST:
				bMoraleMod -= 1;
				break;
			default:
				break;
		}
		if (bMoraleMod < 0)
		{
			// can't change a positive event into a negative one!
			bMoraleMod = 0;
		}
	}
	else
	{
		switch( pProfile->bAttitude )
		{
			case ATT_OPTIMIST:
				bMoraleMod += 1;
				break;
			case ATT_PESSIMIST:
				bMoraleMod -= 1;
				break;
			case ATT_COWARD:
				bMoraleMod -= 2;
			default:
				break;
		}
		if (pSoldier->bLevel == 1)
		{
			bMoraleMod--;
		}
		else if (pSoldier->bLevel > 5)
		{
			bMoraleMod++;
		}
		if (bMoraleMod > 0)
		{
			// can't change a negative event into a positive one!
			bMoraleMod = 0;
		}
	}
	// apply change!
	if (ubType == TACTICAL_MORALE_EVENT)
	{
		iMoraleModTotal = (INT32) pSoldier->bTacticalMoraleMod + (INT32) bMoraleMod;
		iMoraleModTotal = __min( iMoraleModTotal, MORALE_MOD_MAX );
		iMoraleModTotal = __max( iMoraleModTotal, -MORALE_MOD_MAX );
		pSoldier->bTacticalMoraleMod = (INT8) iMoraleModTotal;
	}
	else if ( gTacticalStatus.fEnemyInSector && !pSoldier->bInSector ) // delayed strategic
	{
		iMoraleModTotal = (INT32) pSoldier->bDelayedStrategicMoraleMod + (INT32) bMoraleMod;
		iMoraleModTotal = __min( iMoraleModTotal, MORALE_MOD_MAX );
		iMoraleModTotal = __max( iMoraleModTotal, -MORALE_MOD_MAX );
		pSoldier->bDelayedStrategicMoraleMod = (INT8) iMoraleModTotal;
	}
	else // strategic
	{		
		iMoraleModTotal = (INT32) pSoldier->bStrategicMoraleMod + (INT32) bMoraleMod;
		iMoraleModTotal = __min( iMoraleModTotal, MORALE_MOD_MAX );
		iMoraleModTotal = __max( iMoraleModTotal, -MORALE_MOD_MAX );
		pSoldier->bStrategicMoraleMod = (INT8) iMoraleModTotal;
	}

	RefreshSoldierMorale( pSoldier );

	if ( !pSoldier->fMercAsleep )
	{
		if ( !gfSomeoneSaidMoraleQuote )
		{
			// Check if we're below a certain value and warn
			if ( pSoldier->bMorale < 35 )
			{
				// Have we said this quote yet?
				if ( !(	pSoldier->usQuoteSaidFlags & SOLDIER_QUOTE_SAID_LOW_MORAL ) )
				{
					gfSomeoneSaidMoraleQuote = TRUE;

					// ATE: Amde it a DELAYED QUOTE - will be delayed by the dialogue Q until it's our turn...
					DelayedTacticalCharacterDialogue( pSoldier, QUOTE_STARTING_TO_WHINE );			
					pSoldier->usQuoteSaidFlags |= SOLDIER_QUOTE_SAID_LOW_MORAL;
				}
			}
		}
	}

	// Reset flag!
	if ( pSoldier->bMorale > 65 )
	{
		pSoldier->usQuoteSaidFlags &= (~SOLDIER_QUOTE_SAID_LOW_MORAL );
	}

}


void HandleMoraleEventForSoldier( SOLDIERTYPE * pSoldier, INT8 bMoraleEvent )
{
	UpdateSoldierMorale( pSoldier, gbMoraleEvent[bMoraleEvent].ubType, gbMoraleEvent[bMoraleEvent].bChange );
}


void HandleMoraleEvent( SOLDIERTYPE *pSoldier, INT8 bMoraleEvent, INT16 sMapX, INT16 sMapY, INT8 bMapZ )
{
	UINT8									ubLoop;
	SOLDIERTYPE *					pTeamSoldier;
	MERCPROFILESTRUCT *		pProfile;

	gfSomeoneSaidMoraleQuote = FALSE;

	// NOTE: Many morale events are NOT attached to a specific player soldier at all!
	// Those that do need it have Asserts on a case by case basis below
	if (pSoldier == NULL)
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "Handling morale event %d at X=%d, Y=%d,Z=%d", bMoraleEvent, sMapX, sMapY, bMapZ ) );
	}
	else
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "Handling morale event %d for %S at X=%d, Y=%d, Z=%d", bMoraleEvent, pSoldier->name, sMapX, sMapY, bMapZ ) );
	}


	switch( bMoraleEvent )
	{
		case MORALE_KILLED_ENEMY:
		case MORALE_DID_LOTS_OF_DAMAGE:
		case MORALE_DRUGS_CRASH:
		case MORALE_ALCOHOL_CRASH:
		case MORALE_SUPPRESSED:
		case MORALE_TOOK_LOTS_OF_DAMAGE:
		case MORALE_HIGH_DEATHRATE:
		case MORALE_SEX:
			// needs specific soldier!
			Assert( pSoldier );
			// affects the soldier only
			HandleMoraleEventForSoldier( pSoldier, bMoraleEvent );
			break;

		case MORALE_CLAUSTROPHOBE_UNDERGROUND:
		case MORALE_INSECT_PHOBIC_SEES_CREATURE:
		case MORALE_NERVOUS_ALONE:
			// needs specific soldier!
			Assert( pSoldier );
			// affects the soldier only, should be ignored if tactical morale mod is -20 or less
			if ( pSoldier->bTacticalMoraleMod > PHOBIC_LIMIT )
			{
				HandleMoraleEventForSoldier( pSoldier, bMoraleEvent );
			}
			break;

		case MORALE_BATTLE_WON:
			// affects everyone to varying degrees
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive )
				{
					if ( SOLDIER_IN_SECTOR( pTeamSoldier, sMapX, sMapY, bMapZ ) )
					{
						HandleMoraleEventForSoldier( pTeamSoldier, MORALE_BATTLE_WON );
					}
					else
					{
						HandleMoraleEventForSoldier( pTeamSoldier, MORALE_HEARD_BATTLE_WON );
					}
				}
			}
			break;
		case MORALE_RAN_AWAY:
			// affects everyone to varying degrees
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive )
				{
					// CJC: adding to SOLDIER_IN_SECTOR check special stuff because the old sector values might
					// be appropriate (because in transit going out of that sector!)

					if ( SOLDIER_IN_SECTOR( pTeamSoldier, sMapX, sMapY, bMapZ ) || ( pTeamSoldier->fBetweenSectors && ((pTeamSoldier->ubPrevSectorID % 16) + 1) == sMapX && ((pTeamSoldier->ubPrevSectorID / 16) + 1) == sMapY && ( pTeamSoldier->bSectorZ == bMapZ ) ) )
					{
						switch ( gMercProfiles[ pTeamSoldier->ubProfile ].bAttitude )
						{
							case ATT_AGGRESSIVE:
								// double the penalty - these guys REALLY hate running away
								HandleMoraleEventForSoldier( pTeamSoldier, MORALE_RAN_AWAY );
								HandleMoraleEventForSoldier( pTeamSoldier, MORALE_RAN_AWAY );
								break;
							case ATT_COWARD:
								// no penalty - cowards are perfectly happy to avoid fights!
								break;
							default:
								HandleMoraleEventForSoldier( pTeamSoldier, MORALE_RAN_AWAY );
								break;
						}
					}
					else
					{
						HandleMoraleEventForSoldier( pTeamSoldier, MORALE_HEARD_BATTLE_LOST );
					}
				}
			}
			break;

		case MORALE_TOWN_LIBERATED:
		case MORALE_TOWN_LOST:
		case MORALE_MINE_LIBERATED:
		case MORALE_MINE_LOST:
		case MORALE_SAM_SITE_LIBERATED:
		case MORALE_SAM_SITE_LOST:
		case MORALE_KILLED_CIVILIAN:
		case MORALE_LOW_DEATHRATE:
		case MORALE_HEARD_BATTLE_WON:
		case MORALE_HEARD_BATTLE_LOST:
		case MORALE_MONSTER_QUEEN_KILLED:
		case MORALE_DEIDRANNA_KILLED:
			// affects everyone, everywhere
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive )
				{
					HandleMoraleEventForSoldier( pTeamSoldier, bMoraleEvent );
				}
			}
			break;

		case MORALE_POOR_MORALE:
		case MORALE_GREAT_MORALE:
		case MORALE_AIRSTRIKE:
			// affects every in sector
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive && SOLDIER_IN_SECTOR( pTeamSoldier, sMapX, sMapY, bMapZ ) )
				{
					HandleMoraleEventForSoldier( pTeamSoldier, bMoraleEvent );
				}
			}
			break;

		case MORALE_MERC_CAPTURED:
			// needs specific soldier! (for reputation, not here)
			Assert( pSoldier );

			// affects everyone
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive )
				{
					HandleMoraleEventForSoldier( pTeamSoldier, bMoraleEvent );
				}
			}
			break;
		case MORALE_TEAMMATE_DIED:
			// needs specific soldier!
			Assert( pSoldier );

			// affects everyone, in sector differently than not, extra bonuses if it's a buddy or hated merc
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive && pTeamSoldier->ubProfile != NO_PROFILE)
				{				
					pProfile = &(gMercProfiles[ pTeamSoldier->ubProfile ]);

					if (HATED_MERC( pProfile, pSoldier->ubProfile ))
					{
						// yesss!
						HandleMoraleEventForSoldier( pTeamSoldier, MORALE_HATED_DIED );
					}
					else
					{
						if ( SOLDIER_IN_SECTOR( pTeamSoldier, sMapX, sMapY, bMapZ ) )
						{
							// mate died in my sector!  tactical morale mod
							HandleMoraleEventForSoldier( pTeamSoldier, MORALE_SQUADMATE_DIED );
						}

						// this is handled for everyone even if in sector, as it's a strategic morale mod
						HandleMoraleEventForSoldier( pTeamSoldier, MORALE_TEAMMATE_DIED );

						if (BUDDY_MERC( pProfile, pSoldier->ubProfile ))
						{
							// oh no!  buddy died!
							HandleMoraleEventForSoldier( pTeamSoldier, MORALE_BUDDY_DIED );
						}
					}
				}
			}
			break;

		case MORALE_MERC_MARRIED:
			// female mercs get unhappy based on how sexist they are (=hate men)
			// gentlemen males get unhappy too
			
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive && pTeamSoldier->ubProfile != NO_PROFILE )
				{
					if ( WhichHated( pTeamSoldier->ubProfile, pSoldier->ubProfile ) != -1 )
					{
						// we hate 'em anyways
						continue;
					}

					if ( gMercProfiles[ pTeamSoldier->ubProfile ].bSex == FEMALE )
					{
						switch( gMercProfiles[ pTeamSoldier->ubProfile ].bSexist )
						{
							case SOMEWHAT_SEXIST:
								HandleMoraleEventForSoldier( pTeamSoldier, MORALE_MERC_MARRIED );
								break;
							case VERY_SEXIST:
								// handle TWICE!
								HandleMoraleEventForSoldier( pTeamSoldier, MORALE_MERC_MARRIED );
								HandleMoraleEventForSoldier( pTeamSoldier, MORALE_MERC_MARRIED );
								break;
							default:
								break;
						}
					}
					else
					{
						switch( gMercProfiles[ pTeamSoldier->ubProfile ].bSexist )
						{
							case GENTLEMAN:
								HandleMoraleEventForSoldier( pTeamSoldier, MORALE_MERC_MARRIED );
								break;
							default:
								break;
						}
					}
					
				}
			}			
			break;

		default:
			// debug message
			ScreenMsg( MSG_FONT_RED, MSG_BETAVERSION, L"Invalid morale event type = %d.  AM/CC-1", bMoraleEvent );
			break;
	}


	// some morale events also impact the player's reputation with the mercs back home
	switch( bMoraleEvent )
	{
		case MORALE_HIGH_DEATHRATE:
			ModifyPlayerReputation(REPUTATION_HIGH_DEATHRATE);
			break;
		case MORALE_LOW_DEATHRATE:
			ModifyPlayerReputation(REPUTATION_LOW_DEATHRATE);
			break;
		case MORALE_POOR_MORALE:
			ModifyPlayerReputation(REPUTATION_POOR_MORALE);
			break;
		case MORALE_GREAT_MORALE:
			ModifyPlayerReputation(REPUTATION_GREAT_MORALE);
			break;
		case MORALE_BATTLE_WON:
			ModifyPlayerReputation(REPUTATION_BATTLE_WON);
			break;
		case MORALE_RAN_AWAY:
		case MORALE_HEARD_BATTLE_LOST:
			ModifyPlayerReputation(REPUTATION_BATTLE_LOST);
			break;
		case MORALE_TOWN_LIBERATED:
			ModifyPlayerReputation(REPUTATION_TOWN_WON);
			break;
		case MORALE_TOWN_LOST:
			ModifyPlayerReputation(REPUTATION_TOWN_LOST);
			break;
		case MORALE_TEAMMATE_DIED:
			// impact depends on that dude's level of experience
			ModifyPlayerReputation((UINT8) (pSoldier->bExpLevel * REPUTATION_SOLDIER_DIED));
			break;
		case MORALE_MERC_CAPTURED:
			// impact depends on that dude's level of experience
			ModifyPlayerReputation((UINT8) (pSoldier->bExpLevel * REPUTATION_SOLDIER_CAPTURED));
			break;
		case MORALE_KILLED_CIVILIAN:
			ModifyPlayerReputation(REPUTATION_KILLED_CIVILIAN);
			break;
		case MORALE_MONSTER_QUEEN_KILLED:
			ModifyPlayerReputation(REPUTATION_KILLED_MONSTER_QUEEN);
			break;
		case MORALE_DEIDRANNA_KILLED:
			ModifyPlayerReputation(REPUTATION_KILLED_DEIDRANNA);
			break;

		default:
			// no reputation impact
			break;
	}
}


void HourlyMoraleUpdate( void )
{
	INT8									bMercID, bOtherID;
	INT8									bActualTeamOpinion;
	INT8									bTeamMoraleModChange, bTeamMoraleModDiff;
	INT8									bOpinion=-1;
	INT32									iTotalOpinions;
	INT8									bNumTeamMembers;
	INT8									bHighestTeamLeadership = 0;
	INT8									bLastTeamID;
	SOLDIERTYPE *					pSoldier;
	SOLDIERTYPE *					pOtherSoldier;
	MERCPROFILESTRUCT *		pProfile;
	BOOLEAN								fSameGroupOnly;
	static INT8						bStrategicMoraleUpdateCounter = 0;
	BOOLEAN								fFoundHated = FALSE;
	INT8									bHated;

	bMercID = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
	bLastTeamID = gTacticalStatus.Team[ gbPlayerNum ].bLastID;

	// loop through all mercs to calculate their morale
  for ( pSoldier = MercPtrs[ bMercID ]; bMercID <= bLastTeamID; bMercID++,pSoldier++)
	{	
		//if the merc is active, in Arulco, and conscious, not POW
		if ( pSoldier->bActive && pSoldier->ubProfile != NO_PROFILE && 
																!(pSoldier->bAssignment == IN_TRANSIT ||
																pSoldier->fMercAsleep == TRUE ||
																pSoldier->bAssignment == ASSIGNMENT_DEAD ||
																pSoldier->bAssignment == ASSIGNMENT_POW) )
		{
			// calculate the guy's opinion of the people he is with
			pProfile = &(gMercProfiles[ pSoldier->ubProfile ]);

			// if we're moving
			if (pSoldier->ubGroupID != 0 && PlayerIDGroupInMotion( pSoldier->ubGroupID ))
			{
				// we only check our opinions of people in our squad
				fSameGroupOnly = TRUE;
			}
			else
			{
				fSameGroupOnly = FALSE;
			}
			fFoundHated = FALSE;

			// reset counts to calculate average opinion
			iTotalOpinions = 0;
			bNumTeamMembers = 0;

			// let people with high leadership affect their own morale
			bHighestTeamLeadership = EffectiveLeadership( pSoldier );

			// loop through all other mercs
			bOtherID = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pOtherSoldier = MercPtrs[ bOtherID ]; bOtherID <= bLastTeamID; bOtherID++,pOtherSoldier++)
			{
				// skip past ourselves and all inactive mercs
				if (bOtherID != bMercID && pOtherSoldier->bActive && pOtherSoldier->ubProfile != NO_PROFILE &&
																														!(pOtherSoldier->bAssignment == IN_TRANSIT ||
																															 pOtherSoldier->fMercAsleep == TRUE ||
																															 pOtherSoldier->bAssignment == ASSIGNMENT_DEAD ||
																															 pOtherSoldier->bAssignment == ASSIGNMENT_POW))
				{
					if (fSameGroupOnly)
					{
						// all we have to check is the group ID
						if (pSoldier->ubGroupID != pOtherSoldier->ubGroupID)
						{
							continue;
						}
					}
					else
					{
						// check to see if the location is the same
						if (pOtherSoldier->sSectorX != pSoldier->sSectorX ||
							  pOtherSoldier->sSectorY != pSoldier->sSectorY ||
								pOtherSoldier->bSectorZ != pSoldier->bSectorZ)
						{
							continue;
						}
						
						// if the OTHER soldier is in motion then we don't do anything!
						if (pOtherSoldier->ubGroupID != 0 && PlayerIDGroupInMotion( pOtherSoldier->ubGroupID ))
						{
							continue;
						}
					}
					bOpinion = pProfile->bMercOpinion[ pOtherSoldier->ubProfile ];
					if (bOpinion == HATED_OPINION)
					{

						bHated = WhichHated( pSoldier->ubProfile, pOtherSoldier->ubProfile );
						if ( bHated >= 2 )
						{
							// learn to hate which has become full-blown hatred, full strength
							fFoundHated = TRUE;
							break;
						}
						else 
						{
							// scale according to how close to we are to snapping
							//KM : Divide by 0 error found.  Wrapped into an if statement.
							if( pProfile->bHatedTime[ bHated ] )
							{
								bOpinion = ((INT32) bOpinion) * (pProfile->bHatedTime[ bHated ] - pProfile->bHatedCount[ bHated ]) / pProfile->bHatedTime[ bHated ];
							}

							if ( pProfile->bHatedCount[ bHated ] <= pProfile->bHatedTime[ bHated ] / 2 )
							{
								// Augh, we're teamed with someone we hate!  We HATE this!!  Ignore everyone else!		
								fFoundHated = TRUE;
								break;
							}
							// otherwise just mix this opinion in with everyone else... 
						}
					}
					iTotalOpinions += bOpinion;
					bNumTeamMembers++;
					if ( EffectiveLeadership( pOtherSoldier ) > bHighestTeamLeadership)
					{
						bHighestTeamLeadership = EffectiveLeadership( pOtherSoldier );
					}
				}
			}

			if (fFoundHated)
			{
				// If teamed with someone we hated, team opinion is automatically minimum
				bActualTeamOpinion = HATED_OPINION;
			}
			else if (bNumTeamMembers > 0)
			{
				bActualTeamOpinion = (INT8) (iTotalOpinions / bNumTeamMembers);
				// give bonus/penalty for highest leadership value on team
				bActualTeamOpinion += (bHighestTeamLeadership - 50) / 10;
			}
			else // alone
			{
				bActualTeamOpinion = 0;
			}

			// reduce to a range of HATED through BUDDY
			if (bActualTeamOpinion > BUDDY_OPINION)
			{
				bActualTeamOpinion = BUDDY_OPINION;
			}
			else if (bActualTeamOpinion < HATED_OPINION)
			{
				bActualTeamOpinion = HATED_OPINION;
			}

			// shift morale from team by ~10%

			// this should range between -75 and +75
			bTeamMoraleModDiff = bActualTeamOpinion - pSoldier->bTeamMoraleMod;
			if (bTeamMoraleModDiff > 0)
			{
				bTeamMoraleModChange = 1 + bTeamMoraleModDiff / 10;
			}
			else if (bTeamMoraleModDiff < 0)
			{
				bTeamMoraleModChange = -1 + bTeamMoraleModDiff / 10;
			}
			else
			{
				bTeamMoraleModChange = 0;
			}
			pSoldier->bTeamMoraleMod += bTeamMoraleModChange;
			pSoldier->bTeamMoraleMod = __min( pSoldier->bTeamMoraleMod, MORALE_MOD_MAX );
			pSoldier->bTeamMoraleMod = __max( pSoldier->bTeamMoraleMod, -MORALE_MOD_MAX );
			
			// New, December 3rd, 1998, by CJC --
			// If delayed strategic modifier exists then incorporate it in strategic mod
			if ( pSoldier->bDelayedStrategicMoraleMod )
			{
				pSoldier->bStrategicMoraleMod += pSoldier->bDelayedStrategicMoraleMod;
				pSoldier->bDelayedStrategicMoraleMod = 0;
				pSoldier->bStrategicMoraleMod = __min( pSoldier->bStrategicMoraleMod, MORALE_MOD_MAX );
				pSoldier->bStrategicMoraleMod = __max( pSoldier->bStrategicMoraleMod, -MORALE_MOD_MAX );
			}

			// refresh the morale value for the soldier based on the recalculated team modifier
			RefreshSoldierMorale( pSoldier );
		}
	}

	bStrategicMoraleUpdateCounter++;

	if ( bStrategicMoraleUpdateCounter == HOURS_BETWEEN_STRATEGIC_DECAY )
	{
		DecayStrategicMoraleModifiers();
		bStrategicMoraleUpdateCounter = 0;
	}

}


void DailyMoraleUpdate(SOLDIERTYPE *pSoldier)
{
	if ( pSoldier->ubProfile == NO_PROFILE )
	{
		return;
	}

	// CJC: made per hour now
/*
	// decay the merc's strategic morale modifier
	if (pSoldier->bStrategicMoraleMod != 0)
	{
		// decay the modifier!
		DecayStrategicMorale( pSoldier );

		// refresh the morale value for the soldier based on the recalculated modifier
		RefreshSoldierMorale( pSoldier );
	}
*/

	// check death rate vs. merc's tolerance once/day (ignores buddies!)
	if ( MercThinksDeathRateTooHigh( pSoldier->ubProfile ) )
	{
		// too high, morale takes a hit
		HandleMoraleEvent( pSoldier, MORALE_HIGH_DEATHRATE, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
	}

	// check his morale vs. his morale tolerance once/day (ignores buddies!)
	if ( MercThinksHisMoraleIsTooLow( pSoldier ) )
	{
		// too low, morale sinks further (merc's in a funk and things aren't getting better)
		HandleMoraleEvent( pSoldier, MORALE_POOR_MORALE, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
	}
	else if ( pSoldier->bMorale >= 75 )
	{
		// very high morale, merc is cheerleading others
		HandleMoraleEvent( pSoldier, MORALE_GREAT_MORALE, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
	}

}