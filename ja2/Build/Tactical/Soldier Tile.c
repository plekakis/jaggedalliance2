#ifdef PRECOMPILEDHEADERS
	#include "Tactical All.h"
#else
	#include <wchar.h>
	#include <stdio.h>
	#include <string.h>
	#include "wcheck.h"
	#include "Render Fun.h"
	#include "stdlib.h"
	#include "debug.h"
	#include "MemMan.h"
	#include "Overhead Types.h"
	#include "Soldier Control.h"
	#include "Animation Cache.h"
	#include "Animation Data.h"
	#include "Animation Control.h"
	#include "container.h"
	#include <math.h>
	#include "pathai.h"
	#include "Random.h"
	#include "worldman.h"
	#include "Isometric Utils.h"
	#include "Render Dirty.h"
	#include "renderworld.h"
	#include "sys globals.h"
	#include "video.h"
	#include "points.h"
	#include "Win util.h"
	#include "Sound Control.h" 
	#include "lighting.h"
	#include "weapons.h"
	#include "vobject_blitters.h"
	#include "Handle UI.h"
	#include "soldier ani.h"
	#include "Event pump.h"
	#include "opplist.h"
	#include "ai.h"
	#include "interface.h"
	#include "lighting.h"
	#include "faces.h"
	#include "Soldier Profile.h"
	#include "gap.h"
	#include "interface panels.h"
	#include "campaign.h"

	#ifdef NETWORKED
	#include "Networking.h"
	#include "NetworkEvent.h"
	#endif

	#include "structure wrap.h"
	#include "items.h"
	#include "Soundman.h"
	#include "soldier tile.h"
	#include "soldier add.h"
	#include "fov.h"
	#include "Font Control.h"
	#include "message.h"
	#include "Text.h"
#endif

extern INT8		gbNumMercsUntilWaitingOver;
extern UINT8	gubWaitingForAllMercsToExitCode;


#define NEXT_TILE_CHECK_DELAY		700

#ifdef JA2BETAVERSION

void OutputDebugInfoForTurnBasedNextTileWaiting( SOLDIERTYPE * pSoldier )
{
	if ( (gTacticalStatus.uiFlags & INCOMBAT) && (pSoldier->usPathDataSize > 0) )
	{
		UINT32	uiLoop;
		UINT16	usTemp;
		UINT16	usNewGridNo;

		usNewGridNo = NewGridNo( pSoldier->sGridNo, DirectionInc( (UINT8)pSoldier->usPathingData[ pSoldier->usPathIndex ] ) );

		// provide more info!!
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("  Soldier path size %d, index %d", pSoldier->usPathDataSize, pSoldier->usPathIndex ) );
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("  Who is at blocked gridno: %d", WhoIsThere2( usNewGridNo, pSoldier->bLevel ) ) );

		for ( uiLoop = 0; uiLoop < pSoldier->usPathDataSize; uiLoop++ )
		{
			if ( uiLoop > pSoldier->usPathIndex )
			{					
				usTemp = NewGridNo( usTemp, DirectionInc( (UINT8)pSoldier->usPathingData[ uiLoop ] ) );
				DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("  Soldier path[%d]: %d == gridno %d", uiLoop, pSoldier->usPathingData[uiLoop], usTemp ) );
			}
			else if ( uiLoop == pSoldier->usPathIndex )
			{
				usTemp = usNewGridNo;
				DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("  Soldier path[%d]: %d == gridno %d", uiLoop, pSoldier->usPathingData[uiLoop], usTemp ) );
			}
			else
			{
				DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("  Soldier path[%d]: %d", uiLoop, pSoldier->usPathingData[uiLoop] ) );
			}
		}

	}
}
#endif



void SetDelayedTileWaiting( SOLDIERTYPE *pSoldier, INT16 sCauseGridNo, INT8 bValue )
{
	UINT8		ubPerson;

	// Cancel AI Action
	// CancelAIAction( pSoldier, TRUE );

	pSoldier->fDelayedMovement = bValue;
	pSoldier->sDelayedMovementCauseGridNo = sCauseGridNo;

	RESETTIMECOUNTER( pSoldier->NextTileCounter, NEXT_TILE_CHECK_DELAY );

	// ATE: Now update realtime movement speed....
	// check if guy exists here...
	ubPerson = WhoIsThere2( sCauseGridNo, pSoldier->bLevel );

	// There may not be anybody there, but it's reserved by them!
	if ( ( gpWorldLevelData[ sCauseGridNo ].uiFlags & MAPELEMENT_MOVEMENT_RESERVED ) )
	{
		ubPerson = gpWorldLevelData[ sCauseGridNo ].ubReservedSoldierID;
	}

	if ( ubPerson != NOBODY )
	{
		// if they are our own team members ( both )
		if ( MercPtrs[ ubPerson ]->bTeam == gbPlayerNum && pSoldier->bTeam == gbPlayerNum )
		{
			// Here we have another guy.... save his stats so we can use them for
			// speed determinations....
			pSoldier->bOverrideMoveSpeed = ubPerson;
			pSoldier->fUseMoverrideMoveSpeed = TRUE;
		}
	}
}


void SetFinalTile( SOLDIERTYPE *pSoldier, INT16 sGridNo, BOOLEAN fGivenUp )
{
	// OK, If we were waiting for stuff, do it here...

	// ATE: Disabled stuff below, made obsolete by timeout...
	//if ( pSoldier->ubWaitActionToDo  )
	//{
	//	pSoldier->ubWaitActionToDo = 0;
	//	gbNumMercsUntilWaitingOver--;
	//}
	pSoldier->sFinalDestination = pSoldier->sGridNo;

	#ifdef JA2BETAVERSION
		if ( gTacticalStatus.uiFlags & INCOMBAT )
		{
			OutputDebugInfoForTurnBasedNextTileWaiting( pSoldier );
		}
	#endif

  if ( pSoldier->bTeam == gbPlayerNum  && fGivenUp )
  {
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[ NO_PATH_FOR_MERC ], pSoldier->name );
  }

	EVENT_StopMerc( pSoldier, pSoldier->sGridNo, pSoldier->bDirection );

}


void MarkMovementReserved( SOLDIERTYPE *pSoldier, INT16 sGridNo )
{
	// Check if we have one reserrved already, and free it first!
	if ( pSoldier->sReservedMovementGridNo != NOWHERE )
	{
		UnMarkMovementReserved( pSoldier );
	}

	// For single-tiled mercs, set this gridno
	gpWorldLevelData[ sGridNo ].uiFlags |= MAPELEMENT_MOVEMENT_RESERVED;

	// Save soldier's reserved ID #
	gpWorldLevelData[ sGridNo ].ubReservedSoldierID = pSoldier->ubID;

	pSoldier->sReservedMovementGridNo = sGridNo;
}

void UnMarkMovementReserved( SOLDIERTYPE *pSoldier )
{
	INT16 sNewGridNo;

	sNewGridNo = GETWORLDINDEXFROMWORLDCOORDS(pSoldier->dYPos, pSoldier->dXPos );

	// OK, if NOT in fence anim....
	if ( pSoldier->usAnimState == HOPFENCE && pSoldier->sReservedMovementGridNo != sNewGridNo )
	{
		return;
	}

	// For single-tiled mercs, unset this gridno
	// See if we have one reserved!
	if ( pSoldier->sReservedMovementGridNo != NOWHERE )
	{
		gpWorldLevelData[ pSoldier->sReservedMovementGridNo ].uiFlags &= (~MAPELEMENT_MOVEMENT_RESERVED);

		pSoldier->sReservedMovementGridNo = NOWHERE;
	}
}

INT8 TileIsClear( SOLDIERTYPE *pSoldier, INT8 bDirection,  INT16 sGridNo, INT8 bLevel )
{
	UINT8		ubPerson;
	INT16		sTempDestGridNo;
	INT16		sNewGridNo;
	BOOLEAN	fSwapInDoor = FALSE;

	if ( sGridNo == NOWHERE )
	{
		return( MOVE_TILE_CLEAR );
	}

	ubPerson = WhoIsThere2( sGridNo, bLevel );


	if ( ubPerson != NO_SOLDIER )
	{
		// If this us?
		if ( ubPerson != pSoldier->ubID )
		{
			// OK, set flag indicating we are blocked by a merc....
			if ( pSoldier->bTeam != gbPlayerNum ) // CJC: shouldn't this be in all cases???
		//if ( 0 )
			{
				pSoldier->fBlockedByAnotherMerc = TRUE;
				// Set direction we were trying to goto
				pSoldier->bBlockedByAnotherMercDirection = bDirection;

				// Are we only temporarily blocked?
				// Check if our final destination is = our gridno
				if ( ( MercPtrs[ ubPerson ]->sFinalDestination == MercPtrs[ ubPerson ]->sGridNo )  )
				{
					return( MOVE_TILE_STATIONARY_BLOCKED );
				}
				else
				{
					// OK, if buddy who is blocking us is trying to move too...
					// And we are in opposite directions...
					if ( MercPtrs[ ubPerson ]->fBlockedByAnotherMerc && MercPtrs[ ubPerson ]->bBlockedByAnotherMercDirection == gOppositeDirection[ bDirection ] )
					{
						// OK, try and get a path around buddy....
						// We have to temporarily make buddy stopped...
						sTempDestGridNo = MercPtrs[ ubPerson ]->sFinalDestination;
						MercPtrs[ ubPerson ]->sFinalDestination = MercPtrs[ ubPerson ]->sGridNo;

						if ( PlotPath( pSoldier, pSoldier->sFinalDestination, NO_COPYROUTE, NO_PLOT, TEMPORARY, pSoldier->usUIMovementMode, NOT_STEALTH, FORWARD, pSoldier->bActionPoints ) )
						{
							pSoldier->bPathStored = FALSE;
							// OK, make guy go here...
							EVENT_GetNewSoldierPath( pSoldier, pSoldier->sFinalDestination, pSoldier->usUIMovementMode );
							// Restore final dest....
							MercPtrs[ ubPerson ]->sFinalDestination = sTempDestGridNo;
							pSoldier->fBlockedByAnotherMerc = FALSE;

							// Is the next tile blocked too?
							sNewGridNo = NewGridNo( (UINT16)pSoldier->sGridNo, DirectionInc( (UINT8)guiPathingData[ 0 ] ) );

							return( TileIsClear( pSoldier, (UINT8)guiPathingData[ 0 ], sNewGridNo, pSoldier->bLevel ) );
						} 
						else
						{

								// Not for multi-tiled things...
								if ( !( pSoldier->uiStatusFlags & SOLDIER_MULTITILE ) )
								{
									// Is the next movement cost for a door?
									if ( DoorTravelCost( pSoldier, sGridNo, gubWorldMovementCosts[ sGridNo ][ bDirection ][ pSoldier->bLevel ], (BOOLEAN)( pSoldier->bTeam == gbPlayerNum ), NULL ) == TRAVELCOST_DOOR )
									{
										fSwapInDoor = TRUE;
									}

									// If we are to swap and we're near a door, open door first and then close it...?


									// Swap now!
									MercPtrs[ ubPerson ]->fBlockedByAnotherMerc = FALSE;

									// Restore final dest....
									MercPtrs[ ubPerson ]->sFinalDestination = sTempDestGridNo;
				
									// Swap merc positions.....
									SwapMercPositions( pSoldier, MercPtrs[ ubPerson ] );
									
									// With these two guys swapped, they should try and continue on their way....
									// Start them both again along their way...
									EVENT_GetNewSoldierPath( pSoldier, pSoldier->sFinalDestination, pSoldier->usUIMovementMode );
									EVENT_GetNewSoldierPath( MercPtrs[ ubPerson ], MercPtrs[ ubPerson ]->sFinalDestination, MercPtrs[ ubPerson ]->usUIMovementMode );					
								}
						}
					}
					return( MOVE_TILE_TEMP_BLOCKED );
				}
			}
			else
			{
				//return( MOVE_TILE_STATIONARY_BLOCKED );
				// ATE: OK, put some smartshere...
				// If we are waiting for more than a few times, change to stationary...
				if ( MercPtrs[ ubPerson ]->fDelayedMovement >= 105 )
				{
					// Set to special 'I want to walk through people' value
					pSoldier->fDelayedMovement = 150;

					return( MOVE_TILE_STATIONARY_BLOCKED );
				}
				if ( MercPtrs[ ubPerson ]->sGridNo == MercPtrs[ ubPerson ]->sFinalDestination )
				{
					return( MOVE_TILE_STATIONARY_BLOCKED );
				}
				return( MOVE_TILE_TEMP_BLOCKED );
			}
		}
	}

	if ( ( gpWorldLevelData[ sGridNo ].uiFlags & MAPELEMENT_MOVEMENT_RESERVED ) )
	{
		if ( gpWorldLevelData[ sGridNo ].ubReservedSoldierID != pSoldier->ubID )
		{
			return( MOVE_TILE_TEMP_BLOCKED );
		}
	}

	// Are we clear of structs?
	if ( !NewOKDestination( pSoldier, sGridNo, FALSE, pSoldier->bLevel ) )
	{
			// ATE: Fence cost is an exclusiuon here....
			if ( gubWorldMovementCosts[ sGridNo ][ bDirection ][ pSoldier->bLevel ] != TRAVELCOST_FENCE )
			{
				// ATE: HIdden structs - we do something here... reveal it!
				if ( gubWorldMovementCosts[ sGridNo ][ bDirection ][ pSoldier->bLevel ] == TRAVELCOST_HIDDENOBSTACLE )
				{
					gpWorldLevelData[ sGridNo ].uiFlags|=MAPELEMENT_REVEALED;
					gpWorldLevelData[ sGridNo ].uiFlags|=MAPELEMENT_REDRAW;
					SetRenderFlags(RENDER_FLAG_MARKED);
					RecompileLocalMovementCosts( (UINT16)sGridNo );
				}

				// Unset flag for blocked by soldier...
				pSoldier->fBlockedByAnotherMerc = FALSE;
				return( MOVE_TILE_STATIONARY_BLOCKED );
			}
			else
			{
#if 0
				// Check if there is a reserved marker here at least....
				sNewGridNo = NewGridNo( sGridNo, DirectionInc( bDirection ) );

				if ( ( gpWorldLevelData[ sNewGridNo ].uiFlags & MAPELEMENT_MOVEMENT_RESERVED ) )
				{
					if ( gpWorldLevelData[ sNewGridNo ].ubReservedSoldierID != pSoldier->ubID )
					{
						return( MOVE_TILE_TEMP_BLOCKED );
					}
				}
#endif
			}
	}

	// Unset flag for blocked by soldier...
	pSoldier->fBlockedByAnotherMerc = FALSE;

	return( MOVE_TILE_CLEAR );

}



BOOLEAN HandleNextTile( SOLDIERTYPE *pSoldier, INT8 bDirection, INT16 sGridNo, INT16 sFinalDestTile )
{
	INT8 bBlocked;
	INT16	bOverTerrainType;

	// Check for blocking if in realtime 
	///if ( ( gTacticalStatus.uiFlags & REALTIME ) || !( gTacticalStatus.uiFlags & INCOMBAT ) )

	// ATE: If not on visible tile, return clear ( for path out of map )
	if ( !GridNoOnVisibleWorldTile( sGridNo ) )
	{
		return( TRUE );
	}

	// If animation state is crow, iall is clear
	if ( pSoldier->usAnimState == CROW_FLY )
	{
		return( TRUE );
	}

	{
		bBlocked = TileIsClear( pSoldier, bDirection, sGridNo, pSoldier->bLevel );

		// Check if we are blocked...
		if ( bBlocked != MOVE_TILE_CLEAR )
		{
			// Is the next gridno our destination?
			// OK: Let's check if we are NOT walking off screen
			if ( sGridNo == sFinalDestTile && pSoldier->ubWaitActionToDo == 0 && (pSoldier->bTeam == gbPlayerNum || pSoldier->sAbsoluteFinalDestination == NOWHERE) )
			{
				// Yah, well too bad, stop here.
				SetFinalTile( pSoldier, pSoldier->sGridNo, FALSE );

				return( FALSE );
			}
			// CHECK IF they are stationary
			else if ( bBlocked == MOVE_TILE_STATIONARY_BLOCKED )
			{
				// Stationary, 
				{
					INT16 sOldFinalDest;

					// Maintain sFinalDest....
					sOldFinalDest = pSoldier->sFinalDestination;				
					#ifdef JA2BETAVERSION
						if ( gTacticalStatus.uiFlags & INCOMBAT )
						{
							OutputDebugInfoForTurnBasedNextTileWaiting( pSoldier );
						}
					#endif
					EVENT_StopMerc( pSoldier, pSoldier->sGridNo, pSoldier->bDirection );
					// Restore...
					pSoldier->sFinalDestination = sOldFinalDest;

					SetDelayedTileWaiting( pSoldier, sGridNo, 1 );

					return( FALSE );
				}
			}
			else
			{
				{
					INT16 sOldFinalDest;

					// Maintain sFinalDest....
					sOldFinalDest = pSoldier->sFinalDestination;				
					#ifdef JA2BETAVERSION
						if ( gTacticalStatus.uiFlags & INCOMBAT )
						{
							OutputDebugInfoForTurnBasedNextTileWaiting( pSoldier );
						}
					#endif
					EVENT_StopMerc( pSoldier, pSoldier->sGridNo, pSoldier->bDirection );
					// Restore...
					pSoldier->sFinalDestination = sOldFinalDest;
					
					// Setting to two means: try and wait until this tile becomes free....
					SetDelayedTileWaiting( pSoldier, sGridNo, 100 );
				}

				return( FALSE );
			}
		}
		else
		{
			// Mark this tile as reserverd ( until we get there! )
			if ( !( (gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT) ) )
			{
				MarkMovementReserved( pSoldier, sGridNo );
			}

			bOverTerrainType = gpWorldLevelData[sGridNo].ubTerrainID;

			// Check if we are going into water!
			if ( bOverTerrainType == LOW_WATER || bOverTerrainType == MED_WATER || bOverTerrainType == DEEP_WATER )
			{
				// Check if we are of prone or crawl height and change stance accordingly....
				switch( gAnimControl[ pSoldier->usAnimState ].ubHeight )
				{
					case ANIM_PRONE:
					case ANIM_CROUCH:

						// Change height to stand
						pSoldier->fContinueMoveAfterStanceChange = TRUE;
						SendChangeSoldierStanceEvent( pSoldier, ANIM_STAND );
						break;
				}

				// Check animation
				// Change to walking
				if ( pSoldier->usAnimState == RUNNING )
				{
					ChangeSoldierState( pSoldier, WALKING, 0 , FALSE );
				}
			}
		}
	}
	return( TRUE );
}



BOOLEAN HandleNextTileWaiting( SOLDIERTYPE *pSoldier )
{
	// Buddy is waiting to continue his path
	INT8		bBlocked, bPathBlocked;
	INT16		sCost;
	INT16   sNewGridNo, sCheckGridNo;
	UINT8		ubDirection, bCauseDirection;
	UINT8		ubPerson;
	UINT8		fFlags = 0;

 
	if ( pSoldier->fDelayedMovement )
	{
		if ( TIMECOUNTERDONE( pSoldier->NextTileCounter, NEXT_TILE_CHECK_DELAY ) )
		{
			RESETTIMECOUNTER( pSoldier->NextTileCounter, NEXT_TILE_CHECK_DELAY );

			// Get direction from gridno...
			bCauseDirection = (INT8)GetDirectionToGridNoFromGridNo( pSoldier->sGridNo, pSoldier->sDelayedMovementCauseGridNo );

			bBlocked = TileIsClear( pSoldier, bCauseDirection, pSoldier->sDelayedMovementCauseGridNo, pSoldier->bLevel );

			// If we are waiting for a temp blockage.... continue to wait
			if ( pSoldier->fDelayedMovement >= 100 &&  bBlocked == MOVE_TILE_TEMP_BLOCKED )
			{
				// ATE: Increment 1
				pSoldier->fDelayedMovement++;

				// Are we close enough to give up? ( and are a pc )
				if ( pSoldier->fDelayedMovement > 120 )
				{
					// Quit...
					SetFinalTile( pSoldier, pSoldier->sGridNo, TRUE );
					pSoldier->fDelayedMovement = FALSE;
				}
				return( TRUE );
			}

			// Try new path if anything but temp blockage!
			if ( bBlocked != MOVE_TILE_TEMP_BLOCKED )
			{
				// Set to normal delay
				if ( pSoldier->fDelayedMovement >= 100 && pSoldier->fDelayedMovement != 150 )
				{
					pSoldier->fDelayedMovement = 1;
				}

				// Default to pathing through people
				fFlags = PATH_THROUGH_PEOPLE;

				// Now, if we are in the state where we are desparently trying to get out...
				// Use other flag
				// CJC: path-through-people includes ignoring person at dest
				/*
				if ( pSoldier->fDelayedMovement >= 150 )
				{
					fFlags = PATH_IGNORE_PERSON_AT_DEST;
				}
				*/

				// Check destination first!
				if ( pSoldier->sAbsoluteFinalDestination == pSoldier->sFinalDestination )
				{
					// on last lap of scripted move, make sure we get to final dest
					sCheckGridNo = pSoldier->sAbsoluteFinalDestination;					
				}
				else if (!NewOKDestination( pSoldier, pSoldier->sFinalDestination, TRUE, pSoldier->bLevel ))
				{
					if ( pSoldier->fDelayedMovement >= 150 )
					{
						// OK, look around dest for the first one!
						sCheckGridNo = FindGridNoFromSweetSpot( pSoldier, pSoldier->sFinalDestination, 6, &ubDirection );

						if ( sCheckGridNo == NOWHERE )
						{
							// If this is nowhere, try harder!
							sCheckGridNo = FindGridNoFromSweetSpot( pSoldier, pSoldier->sFinalDestination, 16, &ubDirection );
						}
					}
					else
					{
						// OK, look around dest for the first one!
						sCheckGridNo = FindGridNoFromSweetSpotThroughPeople( pSoldier, pSoldier->sFinalDestination, 6, &ubDirection );

						if ( sCheckGridNo == NOWHERE )
						{
							// If this is nowhere, try harder!
							sCheckGridNo = FindGridNoFromSweetSpotThroughPeople( pSoldier, pSoldier->sFinalDestination, 16, &ubDirection );
						}
					}
				}
				else
				{
					sCheckGridNo = pSoldier->sFinalDestination;
				}

				// Try another path to destination
				// ATE: Allow path to exit grid!
				if ( pSoldier->ubWaitActionToDo == 1 && gubWaitingForAllMercsToExitCode == WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO )
				{
					gfPlotPathToExitGrid = TRUE;
				}

				sCost = (INT16) FindBestPath( pSoldier, sCheckGridNo, pSoldier->bLevel, pSoldier->usUIMovementMode, NO_COPYROUTE, fFlags );
				gfPlotPathToExitGrid = FALSE;
				
				// Can we get there
				if ( sCost > 0 )
				{
					// Is the next tile blocked too?
					sNewGridNo = NewGridNo( (UINT16)pSoldier->sGridNo, DirectionInc( (UINT8)guiPathingData[ 0 ] ) );

					bPathBlocked = TileIsClear( pSoldier, (UINT8)guiPathingData[ 0 ], sNewGridNo, pSoldier->bLevel );

					if ( bPathBlocked == MOVE_TILE_STATIONARY_BLOCKED )
					{
						// Try to path around everyone except dest person

						if ( pSoldier->ubWaitActionToDo == 1 && gubWaitingForAllMercsToExitCode == WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO )
						{
							gfPlotPathToExitGrid = TRUE;
						}

						sCost = (INT16) FindBestPath( pSoldier, sCheckGridNo, pSoldier->bLevel, pSoldier->usUIMovementMode, NO_COPYROUTE, PATH_IGNORE_PERSON_AT_DEST );

						gfPlotPathToExitGrid = FALSE;

						// Is the next tile in this new path blocked too?
						sNewGridNo = NewGridNo( (UINT16)pSoldier->sGridNo, DirectionInc( (UINT8)guiPathingData[ 0 ] ) );

						bPathBlocked = TileIsClear( pSoldier, (UINT8)guiPathingData[ 0 ], sNewGridNo, pSoldier->bLevel );

						// now working with a path which does not go through people				
						pSoldier->ubDelayedMovementFlags &= (~DELAYED_MOVEMENT_FLAG_PATH_THROUGH_PEOPLE);
					}
					else
					{
						// path through people worked fine
						if ( pSoldier->fDelayedMovement < 150 )
						{
							pSoldier->ubDelayedMovementFlags |= DELAYED_MOVEMENT_FLAG_PATH_THROUGH_PEOPLE;
						}
					}

					// Are we clear?
					if ( bPathBlocked == MOVE_TILE_CLEAR )
					{
						// Go for it path!
						if ( pSoldier->ubWaitActionToDo == 1 && gubWaitingForAllMercsToExitCode == WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO )
						{
							gfPlotPathToExitGrid = TRUE;
						}

						//pSoldier->fDelayedMovement = FALSE;
						// ATE: THis will get set in EENT_GetNewSoldierPath....
						pSoldier->usActionData = sCheckGridNo;

						pSoldier->bPathStored = FALSE;

						EVENT_GetNewSoldierPath( pSoldier, sCheckGridNo, pSoldier->usUIMovementMode );
						gfPlotPathToExitGrid = FALSE;

						return( TRUE );
					}
				}

				pSoldier->fDelayedMovement++;

				if ( pSoldier->fDelayedMovement == 99 )
				{
					// Cap at 99
					pSoldier->fDelayedMovement = 99;
				}

				// Do we want to force a swap?
				if (pSoldier->fDelayedMovement == 3 && (pSoldier->sAbsoluteFinalDestination != NOWHERE || gTacticalStatus.fAutoBandageMode) )
				{
					// with person who is in the way?
					ubPerson = WhoIsThere2( pSoldier->sDelayedMovementCauseGridNo, pSoldier->bLevel );

					// if either on a mission from god, or two AI guys not on stationary...
					if ( ubPerson != NOBODY && ( pSoldier->ubQuoteRecord != 0 || ( pSoldier->bTeam != gbPlayerNum && pSoldier->bOrders != STATIONARY && MercPtrs[ ubPerson ]->bTeam != gbPlayerNum && MercPtrs[ ubPerson ]->bOrders != STATIONARY ) || (pSoldier->bTeam == gbPlayerNum && gTacticalStatus.fAutoBandageMode && !(MercPtrs[ ubPerson ]->bTeam == CIV_TEAM && MercPtrs[ ubPerson ]->bOrders == STATIONARY ) ) ) )
					{					
						// Swap now!
						//MercPtrs[ ubPerson ]->fBlockedByAnotherMerc = FALSE;

						// Restore final dest....
						//MercPtrs[ ubPerson ]->sFinalDestination = sTempDestGridNo;

						// Swap merc positions.....
						SwapMercPositions( pSoldier, MercPtrs[ ubPerson ] );
						
						// With these two guys swapped, we should try to continue on our way....				
						pSoldier->fDelayedMovement = FALSE;

						// We must calculate the path here so that we can give it the "through people" parameter
						if ( gTacticalStatus.fAutoBandageMode && pSoldier->sAbsoluteFinalDestination == NOWHERE )
						{
							FindBestPath( pSoldier, pSoldier->sFinalDestination, pSoldier->bLevel, pSoldier->usUIMovementMode, COPYROUTE, PATH_THROUGH_PEOPLE );
						}
						else if ( pSoldier->sAbsoluteFinalDestination != NOWHERE && !FindBestPath( pSoldier, pSoldier->sAbsoluteFinalDestination, pSoldier->bLevel, pSoldier->usUIMovementMode, COPYROUTE, PATH_THROUGH_PEOPLE ) )
						{
							// check to see if we're there now!
							if ( pSoldier->sGridNo == pSoldier->sAbsoluteFinalDestination )
							{
								NPCReachedDestination( pSoldier, FALSE );
								pSoldier->bNextAction = AI_ACTION_WAIT;
								pSoldier->usNextActionData = 500;
								return( TRUE );
							}
						}
						pSoldier->bPathStored = TRUE;

						EVENT_GetNewSoldierPath( pSoldier, pSoldier->sAbsoluteFinalDestination, pSoldier->usUIMovementMode );
						//EVENT_GetNewSoldierPath( MercPtrs[ ubPerson ], MercPtrs[ ubPerson ]->sFinalDestination, MercPtrs[ ubPerson ]->usUIMovementMode );					
					}

				}

				// Are we close enough to give up? ( and are a pc )
				if ( pSoldier->fDelayedMovement > 20 && pSoldier->fDelayedMovement != 150)
				{
					if ( PythSpacesAway( pSoldier->sGridNo, pSoldier->sFinalDestination ) < 5 && pSoldier->bTeam == gbPlayerNum )
					{
						// Quit...
						SetFinalTile( pSoldier, pSoldier->sGridNo, FALSE );
						pSoldier->fDelayedMovement = FALSE;
					}
				}

				// Are we close enough to give up? ( and are a pc )
				if ( pSoldier->fDelayedMovement > 170 )
				{
					if ( PythSpacesAway( pSoldier->sGridNo, pSoldier->sFinalDestination ) < 5 && pSoldier->bTeam == gbPlayerNum )
					{
						// Quit...
						SetFinalTile( pSoldier, pSoldier->sGridNo, FALSE );
						pSoldier->fDelayedMovement = FALSE;
					}
				}

			}
		}
	}
	return( TRUE );
}


BOOLEAN TeleportSoldier( SOLDIERTYPE *pSoldier, INT16 sGridNo, BOOLEAN fForce )
{
	INT16 sX, sY;

	// Check dest...
	if ( NewOKDestination( pSoldier, sGridNo, TRUE, 0 ) || fForce )
	{
		// TELEPORT TO THIS LOCATION!
		sX = CenterX( sGridNo );
		sY = CenterY( sGridNo );
		EVENT_SetSoldierPosition( pSoldier, (FLOAT) sX, (FLOAT) sY );	

		pSoldier->sFinalDestination = sGridNo;

		// Make call to FOV to update items...
		RevealRoofsAndItems(pSoldier, TRUE, TRUE, pSoldier->bLevel, TRUE );
	
		// Handle sight!
		HandleSight(pSoldier,SIGHT_LOOK | SIGHT_RADIO);

		// Cancel services...
		GivingSoldierCancelServices( pSoldier );

		// Change light....
		if ( pSoldier->bLevel == 0 )
		{
			if(pSoldier->iLight!=(-1))
				LightSpriteRoofStatus(pSoldier->iLight, FALSE );
		}
		else
		{
			if(pSoldier->iLight!=(-1))
				LightSpriteRoofStatus(pSoldier->iLight, TRUE );
		}
		return( TRUE );
	}

	return( FALSE );
}

// Swaps 2 soldier positions...
void SwapMercPositions( SOLDIERTYPE *pSoldier1, SOLDIERTYPE *pSoldier2 )
{
	INT16 sGridNo1, sGridNo2;

	// OK, save positions...
	sGridNo1 = pSoldier1->sGridNo;
	sGridNo2 = pSoldier2->sGridNo;

	// OK, remove each.....
	RemoveSoldierFromGridNo( pSoldier1 );
	RemoveSoldierFromGridNo( pSoldier2 );

	// OK, test OK destination for each.......
	if ( NewOKDestination( pSoldier1, sGridNo2, TRUE, 0 ) && NewOKDestination( pSoldier2, sGridNo1, TRUE, 0 ) )
	{
		// OK, call teleport function for each.......
		TeleportSoldier( pSoldier1, sGridNo2, FALSE );
		TeleportSoldier( pSoldier2, sGridNo1, FALSE );
	}
	else
	{
		// Place back...
		TeleportSoldier( pSoldier1, sGridNo1, TRUE );
		TeleportSoldier( pSoldier2, sGridNo2, TRUE );
	}
}


BOOLEAN CanExchangePlaces( SOLDIERTYPE *pSoldier1, SOLDIERTYPE *pSoldier2, BOOLEAN fShow )
{
	// NB checks outside of this function 
	if ( EnoughPoints( pSoldier1, AP_EXCHANGE_PLACES, 0, fShow ) )
	{
	  if ( EnoughPoints( pSoldier2, AP_EXCHANGE_PLACES, 0, fShow ) )
	  {
      if ( ( gAnimControl[ pSoldier2->usAnimState ].uiFlags & ANIM_MOVING ) )
      {
        return( FALSE );
      }

      if ( ( gAnimControl[ pSoldier1->usAnimState ].uiFlags & ANIM_MOVING ) && !(gTacticalStatus.uiFlags & INCOMBAT) )
      {
        return( FALSE );
      }

		  if ( pSoldier2->bSide == 0 )
		  {
			  return( TRUE );
		  }

      // hehe - don't allow animals to exchange places
      if ( pSoldier2->uiStatusFlags & ( SOLDIER_ANIMAL ) )
      {
        return( FALSE );
      }

      // must NOT be hostile, must NOT have stationary orders OR militia team, must be >= OKLIFE
      if ( pSoldier2->bNeutral && pSoldier2->bLife >= OKLIFE && 
			     pSoldier2->ubCivilianGroup != HICKS_CIV_GROUP && 
			   ( ( pSoldier2->bOrders != STATIONARY || pSoldier2->bTeam == MILITIA_TEAM ) || 
			   ( pSoldier2->sAbsoluteFinalDestination != NOWHERE && pSoldier2->sAbsoluteFinalDestination != pSoldier2->sGridNo ) ) )
      {
        return( TRUE );
      }

		  if ( fShow )
		  {
        if ( pSoldier2->ubProfile == NO_PROFILE )
        {
				  ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[ REFUSE_EXCHANGE_PLACES ] );
        }
        else
        {
				  ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, gzLateLocalizedString[3], pSoldier2->name );
        }
		  }

		  // ATE: OK, reduce this guy's next ai counter....
		  pSoldier2->uiAIDelay = 100;

		  return( FALSE );
	  }
	  else
	  {
		  return( FALSE );
	  }
  }
	else
	{
		return( FALSE );
	}
	return( TRUE );
}
