#ifdef PRECOMPILEDHEADERS
	#include "TileEngine All.h"
	#include "end game.h"
	#include "Morale.h"
#else
	#include <stdio.h>
	#include <string.h>
	#include "wcheck.h"
	#include "stdlib.h"
	#include "debug.h"
	#include "soldier control.h"
	#include "weapons.h"
	#include "handle items.h"
	#include "worlddef.h"	
	#include "worldman.h"
	#include "rotting corpses.h"
	#include "tile cache.h"
	#include "isometric utils.h"
	#include "animation control.h"
	#include "utilities.h"
	#include "game clock.h"
	#include "soldier create.h"
	#include "renderworld.h"
	#include "soldier add.h"
	#include "explosion control.h"
	#include "tile animation.h"
	#include "sound control.h"
	#include "weapons.h"
	#include "handle items.h"
	#include "world items.h"
	#include "structure wrap.h"
	#include "tiledef.h"
	#include "tiledat.h"
	#include "interactive tiles.h"
	#include "SaveLoadMap.h"
	#include "Handle Doors.h"
	#include "Message.h"
	#include "Random.h"
	#include "smokeeffects.h"
	#include "handle ui.h"
	#include "pathai.h"
	#include "pits.h"
	#include "campaign Types.h"
	#include "strategicmap.h"
	#include "strategic.h"
	#include "Action Items.h"
	#include "Soldier Profile.h"
	#include "Quests.h"
	#include "Interface Dialogue.h"
	#include "LightEffects.h"
	#include "AI.h"
	#include "Soldier tile.h"
	#include "lighting.h"
	#include "Render Fun.h"
	#include "Opplist.h"
	#include "smell.h"
	#include "GameSettings.h"
	#include "Keys.h"
	#include "Interface.h"
	#include "WorldDat.h"
	#include "Map Information.h"
#endif

#include "Soldier Macros.h"


// MODULE FOR EXPLOSIONS

// Spreads the effects of explosions...
BOOLEAN ExpAffect( INT16 sBombGridNo, INT16 sGridNo, UINT32 uiDist, UINT16 usItem, UINT8 ubOwner, INT16 sSubsequent, BOOLEAN *pfMercHit, INT8 bLevel, INT32 iSmokeEffectID );

extern INT8	 gbSAMGraphicList[ NUMBER_OF_SAMS ];
extern  void AddToShouldBecomeHostileOrSayQuoteList( UINT8 ubID );
extern void RecompileLocalMovementCostsForWall( INT16 sGridNo, UINT8 ubOrientation );
void FatigueCharacter( SOLDIERTYPE *pSoldier );

UINT8		ubTransKeyFrame[ NUM_EXP_TYPES ] =
{
	0,
	17,
	28,
	24,
	1,
	1,
	1,
	1,
	1,
};

UINT8		ubDamageKeyFrame[ NUM_EXP_TYPES ] =
{
	0,
	3,
	5,
	5,
	5,
	18,
	18,
	18,
	18,
};


UINT32	uiExplosionSoundID[ NUM_EXP_TYPES ] =
{
	EXPLOSION_1,
	EXPLOSION_1,
	EXPLOSION_BLAST_2,  //LARGE
	EXPLOSION_BLAST_2,
	EXPLOSION_1,
	AIR_ESCAPING_1,
	AIR_ESCAPING_1,
	AIR_ESCAPING_1,
	AIR_ESCAPING_1,
};


CHAR8	zBlastFilenames[][70] =
{
	"",
	"TILECACHE\\ZGRAV_D.STI",
	"TILECACHE\\ZGRAV_C.STI",
	"TILECACHE\\ZGRAV_B.STI",
	"TILECACHE\\shckwave.STI",
	"TILECACHE\\WAT_EXP.STI",
	"TILECACHE\\TEAR_EXP.STI",
	"TILECACHE\\TEAR_EXP.STI",
	"TILECACHE\\MUST_EXP.STI",
};

CHAR8	sBlastSpeeds[] =
{	
	0,
	80,
	80,
	80,
	20,
	80,
	80,
	80,
	80,
};

#define BOMB_QUEUE_DELAY (1000 + Random( 500 ) )

#define MAX_BOMB_QUEUE 40
ExplosionQueueElement		gExplosionQueue[MAX_BOMB_QUEUE];
UINT8										gubElementsOnExplosionQueue = 0;
BOOLEAN									gfExplosionQueueActive = FALSE;

BOOLEAN									gfExplosionQueueMayHaveChangedSight = FALSE;
UINT8										gubPersonToSetOffExplosions = NOBODY;

INT16			gsTempActionGridNo = NOWHERE;

extern UINT8 gubInterruptProvoker;

#define		NUM_EXPLOSION_SLOTS					100

// GLOBAL FOR SMOKE LISTING
EXPLOSIONTYPE			gExplosionData[ NUM_EXPLOSION_SLOTS ];
UINT32						guiNumExplosions = 0;


INT32 GetFreeExplosion( void );
void RecountExplosions( void );
void GenerateExplosionFromExplosionPointer( EXPLOSIONTYPE *pExplosion );
void HandleBuldingDestruction( INT16 sGridNo, UINT8 ubOwner );


INT32 GetFreeExplosion( void )
{
	UINT32 uiCount;

	for(uiCount=0; uiCount < guiNumExplosions; uiCount++)
	{
		if(( gExplosionData[uiCount].fAllocated==FALSE ) )
			return( (INT32)uiCount );
	}

	if( guiNumExplosions < NUM_EXPLOSION_SLOTS )
		return( (INT32) guiNumExplosions++ );

	return( -1 );
}

void RecountExplosions( void )
{
	INT32 uiCount;

	for(uiCount=guiNumExplosions-1; (uiCount >=0) ; uiCount--)
	{
		if( ( gExplosionData[uiCount].fAllocated ) )
		{
			guiNumExplosions=(UINT32)(uiCount+1);
			break;
		}
	}
}




// GENERATE EXPLOSION
void InternalIgniteExplosion( UINT8 ubOwner, INT16 sX, INT16 sY, INT16 sZ, INT16 sGridNo, UINT16 usItem, BOOLEAN fLocate, INT8 bLevel )
{
	EXPLOSION_PARAMS	ExpParams ;


	// Double check that we are using an explosive!
	if ( !( Item[ usItem ].usItemClass & IC_EXPLOSV ) )
	{
		return;
	}

	// Increment attack counter...

	if (gubElementsOnExplosionQueue == 0)
	{
		// single explosion, disable sight until the end, and set flag
		// to check sight at end of attack
		
		gTacticalStatus.uiFlags |= (DISALLOW_SIGHT | CHECK_SIGHT_AT_END_OF_ATTACK);
	}


	gTacticalStatus.ubAttackBusyCount++;
	DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Incrementing Attack: Explosion gone off, COunt now %d", gTacticalStatus.ubAttackBusyCount ) );


	// OK, go on!
	ExpParams.uiFlags			= EXPLOSION_FLAG_USEABSPOS;
	ExpParams.ubOwner			= ubOwner;
	ExpParams.ubTypeID		= Explosive[ Item[ usItem ].ubClassIndex ].ubAnimationID;
	ExpParams.sX					= sX;
	ExpParams.sY					= sY;
	ExpParams.sZ					= sZ;
	ExpParams.sGridNo			= sGridNo;
	ExpParams.usItem			= usItem;
	ExpParams.fLocate			= fLocate;
	ExpParams.bLevel			= bLevel;

	GenerateExplosion( &ExpParams );
			
}

void IgniteExplosion( UINT8 ubOwner, INT16 sX, INT16 sY, INT16 sZ, INT16 sGridNo, UINT16 usItem, INT8 bLevel )
{
	InternalIgniteExplosion( ubOwner, sX, sY, sZ, sGridNo, usItem, TRUE, bLevel );
}

void GenerateExplosion( EXPLOSION_PARAMS *pExpParams )
{
	EXPLOSIONTYPE		*pExplosion;
	UINT32		uiFlags;
	UINT8			ubOwner;
	UINT8			ubTypeID;
	INT16			sX;
	INT16			sY;
	INT16			sZ;
	INT16			sGridNo;
	UINT16		usItem;
	INT32			iIndex;
	INT8			bLevel;

	// Assign param values
	uiFlags				= pExpParams->uiFlags;
	ubOwner				= pExpParams->ubOwner;
	ubTypeID			= pExpParams->ubTypeID;
	sX						= pExpParams->sX;
	sY						= pExpParams->sY;
	sZ						= pExpParams->sZ;
	sGridNo				= pExpParams->sGridNo;
	usItem				= pExpParams->usItem;
	bLevel				= pExpParams->bLevel;


	{
		// GET AND SETUP EXPLOSION INFO IN TABLE....
		iIndex = GetFreeExplosion( );

		if ( iIndex == -1 )
		{
			return;
		}

		// OK, get pointer...
		pExplosion = &( gExplosionData[ iIndex ] );

		memset( pExplosion, 0, sizeof( EXPLOSIONTYPE ) );

		// Setup some data...
		memcpy( &(pExplosion->Params), pExpParams, sizeof( EXPLOSION_PARAMS ) ); 
		pExplosion->fAllocated = TRUE;
		pExplosion->iID = iIndex;

		GenerateExplosionFromExplosionPointer( pExplosion );
	}
	
	// ATE: Locate to explosion....
	if ( pExpParams->fLocate )
	{
		LocateGridNo( sGridNo );
	}
}


void GenerateExplosionFromExplosionPointer( EXPLOSIONTYPE *pExplosion )
{
	UINT32		uiFlags;
	UINT8			ubOwner;
	UINT8			ubTypeID;
	INT16			sX;
	INT16			sY;
	INT16			sZ;
	INT16			sGridNo;
	UINT16		usItem;
	UINT8			ubTerrainType;
	INT8			bLevel;
  UINT32    uiSoundID;

	ANITILE_PARAMS	AniParams;

	// Assign param values
	uiFlags				= pExplosion->Params.uiFlags;
	ubOwner				= pExplosion->Params.ubOwner;
	ubTypeID			= pExplosion->Params.ubTypeID;
	sX						= pExplosion->Params.sX;
	sY						= pExplosion->Params.sY;
	sZ						= pExplosion->Params.sZ;
	sGridNo				= pExplosion->Params.sGridNo;
	usItem				= pExplosion->Params.usItem;
	bLevel				= pExplosion->Params.bLevel;

  // If Z value given is 0 and bLevel > 0, make z heigher
  if ( sZ == 0 && bLevel > 0 )
  {
    sZ = ROOF_LEVEL_HEIGHT;
  }

	pExplosion->iLightID = -1;

	// OK, if we are over water.... use water explosion...
	ubTerrainType = gpWorldLevelData[sGridNo].ubTerrainID;

	// Setup explosion!
	memset( &AniParams, 0, sizeof( ANITILE_PARAMS ) );

	AniParams.sGridNo							= sGridNo;
	AniParams.ubLevelID						= ANI_TOPMOST_LEVEL;
	AniParams.sDelay							= sBlastSpeeds[ ubTypeID ];
	AniParams.sStartFrame					= pExplosion->sCurrentFrame;
	AniParams.uiFlags							= ANITILE_CACHEDTILE | ANITILE_FORWARD | ANITILE_EXPLOSION;

	if ( ubTerrainType == LOW_WATER || ubTerrainType == MED_WATER || ubTerrainType == DEEP_WATER )
	{
		// Change type to water explosion...
		ubTypeID = WATER_BLAST;
		AniParams.uiFlags						|= ANITILE_ALWAYS_TRANSLUCENT;
	}


	if ( sZ < WALL_HEIGHT )
	{
		AniParams.uiFlags |= ANITILE_NOZBLITTER;
	}

	if ( uiFlags & EXPLOSION_FLAG_USEABSPOS )
	{
		AniParams.sX									= sX;
		AniParams.sY									= sY;
		AniParams.sZ									= sZ;

		//AniParams.uiFlags							|= ANITILE_USEABSOLUTEPOS;
	}

	AniParams.ubKeyFrame1					= ubTransKeyFrame[ ubTypeID ];
	AniParams.uiKeyFrame1Code			= ANI_KEYFRAME_BEGIN_TRANSLUCENCY;

	if ( !( uiFlags & EXPLOSION_FLAG_DISPLAYONLY ) )
	{
		AniParams.ubKeyFrame2					= ubDamageKeyFrame[ ubTypeID ];
		AniParams.uiKeyFrame2Code			= ANI_KEYFRAME_BEGIN_DAMAGE;
	}
	AniParams.uiUserData					= usItem;
	AniParams.ubUserData2					= ubOwner;
	AniParams.uiUserData3					= pExplosion->iID;


	strcpy( AniParams.zCachedFile, zBlastFilenames[ ubTypeID ] );

	CreateAnimationTile( &AniParams );

	//  set light source....
	if ( pExplosion->iLightID == -1 )
	{
		// DO ONLY IF WE'RE AT A GOOD LEVEL
		if ( ubAmbientLightLevel >= MIN_AMB_LEVEL_FOR_MERC_LIGHTS )
		{
			if( ( pExplosion->iLightID = LightSpriteCreate("L-R04.LHT", 0 ) ) != (-1) )
			{
				LightSpritePower( pExplosion->iLightID, TRUE );

				LightSpritePosition( pExplosion->iLightID, (INT16)(sX/CELL_X_SIZE), (INT16)(sY/CELL_Y_SIZE) );
			}
		}
	}

  uiSoundID = uiExplosionSoundID[ ubTypeID ];

  if ( uiSoundID == EXPLOSION_1 )
  {
      // Randomize
     if ( Random( 2 ) == 0 )
     {
      uiSoundID = EXPLOSION_ALT_BLAST_1;
     }
  }
	
	PlayJA2Sample( uiSoundID, RATE_11025, SoundVolume( HIGHVOLUME, sGridNo ), 1, SoundDir( sGridNo ) );			

}



void UpdateExplosionFrame( INT32 iIndex, INT16 sCurrentFrame )
{
	gExplosionData[ iIndex ].sCurrentFrame = sCurrentFrame;
}

void RemoveExplosionData( INT32 iIndex )
{
	gExplosionData[ iIndex ].fAllocated = FALSE;

	if ( gExplosionData[ iIndex ].iLightID != -1 )
	{
		LightSpriteDestroy( gExplosionData[ iIndex ].iLightID );
	}

}


void HandleFencePartnerCheck( INT16 sStructGridNo )
{
	STRUCTURE *pFenceStructure, *pFenceBaseStructure;
	LEVELNODE *pFenceNode;
	INT8		bFenceDestructionPartner = -1;
	UINT32	uiFenceType;
	UINT16	usTileIndex;

	pFenceStructure = FindStructure( sStructGridNo, STRUCTURE_FENCE );

	if ( pFenceStructure )
	{
		// How does our explosion partner look?
		if ( pFenceStructure->pDBStructureRef->pDBStructure->bDestructionPartner < 0 )
		{
			// Find level node.....
			pFenceBaseStructure = FindBaseStructure( pFenceStructure );

			// Get LEVELNODE for struct and remove!
			pFenceNode = FindLevelNodeBasedOnStructure( pFenceBaseStructure->sGridNo, pFenceBaseStructure );

			// Get type from index...
			GetTileType( pFenceNode->usIndex, &uiFenceType );

			bFenceDestructionPartner = -1 * ( pFenceBaseStructure->pDBStructureRef->pDBStructure->bDestructionPartner );

			// Get new index
			GetTileIndexFromTypeSubIndex( uiFenceType, (INT8)( bFenceDestructionPartner ), &usTileIndex );					

			//Set a flag indicating that the following changes are to go the the maps, temp file
			ApplyMapChangesToMapTempFile( TRUE );

			// Remove it!
			RemoveStructFromLevelNode( pFenceBaseStructure->sGridNo, pFenceNode );

			// Add it!
			AddStructToHead( pFenceBaseStructure->sGridNo, (UINT16)( usTileIndex ) );

			ApplyMapChangesToMapTempFile( FALSE );
		}
	}
}




BOOLEAN ExplosiveDamageStructureAtGridNo( STRUCTURE * pCurrent, STRUCTURE **ppNextCurrent,  INT16 sGridNo, INT16 sWoundAmt, UINT32 uiDist, BOOLEAN *pfRecompileMovementCosts, BOOLEAN fOnlyWalls, BOOLEAN fSubSequentMultiTilesTransitionDamage, UINT8 ubOwner, INT8 bLevel )
{
	INT16 sX, sY;
	STRUCTURE		*pBase, *pWallStruct, *pAttached, *pAttachedBase;
	LEVELNODE *pNode = NULL, *pNewNode = NULL, *pAttachedNode;
	INT16 sNewGridNo, sStructGridNo;
	INT16	sNewIndex, sSubIndex;
	UINT16 usObjectIndex, usTileIndex;
	UINT8	 ubNumberOfTiles, ubLoop;
	DB_STRUCTURE_TILE	**	ppTile;
	INT8	bDestructionPartner=-1;
	INT8		bDamageReturnVal;
	BOOLEAN	fContinue;
	UINT32	uiTileType;
	INT16		sBaseGridNo;
	BOOLEAN	fExplosive;

	// ATE: Check for O3 statue for special damage..
	// note we do this check every time explosion goes off in game, but it's
	// an effiecnent check...
	if ( DoesO3SectorStatueExistHere( sGridNo ) && uiDist <= 1 )
	{
		ChangeO3SectorStatue( TRUE );
		return( TRUE );
	}

	// Get xy
	sX = CenterX( sGridNo );
	sY = CenterY( sGridNo );

	// ATE: Continue if we are only looking for walls
	if ( fOnlyWalls && !( pCurrent->fFlags & STRUCTURE_WALLSTUFF ) )
	{
		return( TRUE );
	}

	if ( bLevel > 0 )
	{
		return( TRUE );
	}

	// Is this a corpse?
	if ( ( pCurrent->fFlags & STRUCTURE_CORPSE ) && gGameSettings.fOptions[ TOPTION_BLOOD_N_GORE ] && sWoundAmt > 10 )
	{
		// Spray corpse in a fine mist....
		if ( uiDist <= 1 )
		{
			// Remove corpse...
			VaporizeCorpse( sGridNo, pCurrent->usStructureID );
		}
	}
	else if ( !( pCurrent->fFlags & STRUCTURE_PERSON ) )
	{
		// Damage structure!
		if ( ( bDamageReturnVal = DamageStructure( pCurrent, (UINT8)sWoundAmt, STRUCTURE_DAMAGE_EXPLOSION, sGridNo, sX, sY, NOBODY ) ) != 0 )
		{
			fContinue = FALSE;

			pBase = FindBaseStructure( pCurrent );
			
			sBaseGridNo = pBase->sGridNo;

			// if the structure is openable, destroy all items there
			if ( pBase->fFlags & STRUCTURE_OPENABLE && !(pBase->fFlags & STRUCTURE_DOOR ) )
			{
				RemoveAllUnburiedItems( pBase->sGridNo, bLevel );
			}

			fExplosive = ( ( pCurrent->fFlags & STRUCTURE_EXPLOSIVE ) != 0 );

			// Get LEVELNODE for struct and remove!
			pNode = FindLevelNodeBasedOnStructure( pBase->sGridNo, pBase );

      // ATE: if we have completely destroyed a structure,
      // and this structure should have a in-between explosion partner,
      // make damage code 2 - which means only damaged - the normal explosion
      // spreading will cause it do use the proper peices..
      if ( bDamageReturnVal == 1 && pBase->pDBStructureRef->pDBStructure->bDestructionPartner < 0 )
      {
        bDamageReturnVal = 2;
      }

			if ( bDamageReturnVal == 1 )
			{
				fContinue = TRUE;
			}
			// Check for a damaged looking graphic...
			else if ( bDamageReturnVal == 2 )
			{
				if ( pBase->pDBStructureRef->pDBStructure->bDestructionPartner < 0 )
				{
					// We swap to another graphic!
					// It's -ve and 1-based, change to +ve, 1 based
					bDestructionPartner = ( -1 * pBase->pDBStructureRef->pDBStructure->bDestructionPartner );

					GetTileType( pNode->usIndex, &uiTileType );

					fContinue = 2;
				}
			}

			if ( fContinue )
			{
				// Remove the beast!
				while ( (*ppNextCurrent) != NULL && (*ppNextCurrent)->usStructureID == pCurrent->usStructureID )
				{
					// the next structure will also be deleted so we had better
					// skip past it!
					(*ppNextCurrent) = (*ppNextCurrent)->pNext;
				}

				// Replace with explosion debris if there are any....
				// ( and there already sin;t explosion debris there.... )
				if ( pBase->pDBStructureRef->pDBStructure->bDestructionPartner > 0 )
				{
					// Alrighty add!
					
					// Add to every gridno structure is in
					ubNumberOfTiles = pBase->pDBStructureRef->pDBStructure->ubNumberOfTiles;
					ppTile = pBase->pDBStructureRef->ppTile;

					bDestructionPartner = pBase->pDBStructureRef->pDBStructure->bDestructionPartner;

					// OK, destrcution index is , as default, the partner, until we go over the first set of explsion
					// debris...
					if ( bDestructionPartner > 39 )
					{
						GetTileIndexFromTypeSubIndex( SECONDEXPLDEBRIS, (INT8)( bDestructionPartner - 40 ), &usTileIndex );					
					}
					else
					{
						GetTileIndexFromTypeSubIndex( FIRSTEXPLDEBRIS, bDestructionPartner, &usTileIndex );
					}

					// Free all the non-base tiles; the base tile is at pointer 0
					for (ubLoop = BASE_TILE; ubLoop < ubNumberOfTiles; ubLoop++)
					{
						if ( !(ppTile[ ubLoop ]->fFlags & TILE_ON_ROOF ) )
						{
							sStructGridNo = pBase->sGridNo + ppTile[ubLoop]->sPosRelToBase;
							// there might be two structures in this tile, one on each level, but we just want to
							// delete one on each pass
							
							if ( !TypeRangeExistsInObjectLayer( sStructGridNo, FIRSTEXPLDEBRIS, SECONDEXPLDEBRIS, &usObjectIndex ) )
							{
								//Set a flag indicating that the following changes are to go the the maps, temp file
								ApplyMapChangesToMapTempFile( TRUE );

								AddObjectToHead( sStructGridNo, (UINT16)(usTileIndex + Random( 3 ) ) );

								ApplyMapChangesToMapTempFile( FALSE );
							}
						}
					}

					// IF we are a wall, add debris for the other side
					if ( pCurrent->fFlags & STRUCTURE_WALLSTUFF )
					{
						switch( pCurrent->ubWallOrientation )
						{
							case OUTSIDE_TOP_LEFT:
							case INSIDE_TOP_LEFT:

									sStructGridNo = NewGridNo( pBase->sGridNo, DirectionInc( SOUTH ) );
									if ( !TypeRangeExistsInObjectLayer( sStructGridNo, FIRSTEXPLDEBRIS, SECONDEXPLDEBRIS, &usObjectIndex ) )
									{
										//Set a flag indicating that the following changes are to go the the maps, temp file
										ApplyMapChangesToMapTempFile( TRUE );

										AddObjectToHead( sStructGridNo, (UINT16)(usTileIndex + Random( 3 ) ) );

										ApplyMapChangesToMapTempFile( FALSE );
									}
									break;

							case OUTSIDE_TOP_RIGHT:
							case INSIDE_TOP_RIGHT:

									sStructGridNo = NewGridNo( pBase->sGridNo, DirectionInc( EAST ) );
									if ( !TypeRangeExistsInObjectLayer( sStructGridNo, FIRSTEXPLDEBRIS, SECONDEXPLDEBRIS, &usObjectIndex ) )
									{
										//Set a flag indicating that the following changes are to go the the maps, temp file
										ApplyMapChangesToMapTempFile( TRUE );

										AddObjectToHead( sStructGridNo, (UINT16)(usTileIndex + Random( 3 ) ) );

										ApplyMapChangesToMapTempFile( FALSE );
									}
									break;
						}
					}
				}
				// Else look for fences, walk along them to change to destroyed peices...
				else if ( pCurrent->fFlags & STRUCTURE_FENCE )
				{
					// walk along based on orientation
					switch( pCurrent->ubWallOrientation )
					{
						case OUTSIDE_TOP_RIGHT:
						case INSIDE_TOP_RIGHT:

								sStructGridNo		= NewGridNo( pBase->sGridNo, DirectionInc( SOUTH ) );
								HandleFencePartnerCheck( sStructGridNo );
								sStructGridNo		= NewGridNo( pBase->sGridNo, DirectionInc( NORTH ) );
								HandleFencePartnerCheck( sStructGridNo );
								break;

						case OUTSIDE_TOP_LEFT:
						case INSIDE_TOP_LEFT:

								sStructGridNo = NewGridNo( pBase->sGridNo, DirectionInc( EAST ) );
								HandleFencePartnerCheck( sStructGridNo );
								sStructGridNo = NewGridNo( pBase->sGridNo, DirectionInc( WEST ) );
								HandleFencePartnerCheck( sStructGridNo );
								break;
					}
				}

				// OK, Check if this is a wall, then search and change other walls based on this
				if ( pCurrent->fFlags & STRUCTURE_WALLSTUFF )
				{
					// ATE
					// Remove any decals in tile....
					// Use tile database for this as apposed to stuct data
					RemoveAllStructsOfTypeRange( pBase->sGridNo, FIRSTWALLDECAL, FOURTHWALLDECAL );
					RemoveAllStructsOfTypeRange( pBase->sGridNo, FIFTHWALLDECAL, EIGTHWALLDECAL );

					// Alrighty, now do this
					// Get orientation
					// based on orientation, go either x or y dir
					// check for wall in both _ve and -ve directions
					// if found, replace!
					switch( pCurrent->ubWallOrientation )
					{
						case OUTSIDE_TOP_LEFT:
						case INSIDE_TOP_LEFT:
								
							// Move WEST
							sNewGridNo = NewGridNo( pBase->sGridNo, DirectionInc( WEST ) );

							pNewNode = GetWallLevelNodeAndStructOfSameOrientationAtGridno( sNewGridNo, pCurrent->ubWallOrientation, &pWallStruct );

							if ( pNewNode != NULL )
							{
								if ( pWallStruct->fFlags & STRUCTURE_WALL )
								{
									if ( pCurrent->ubWallOrientation == OUTSIDE_TOP_LEFT )
									{
										sSubIndex = 48;
									}
									else
									{
										sSubIndex = 52;
									}

									// Replace!
									GetTileIndexFromTypeSubIndex( gTileDatabase[ pNewNode->usIndex ].fType, sSubIndex, &sNewIndex );

									//Set a flag indicating that the following changes are to go the the maps temp file
									ApplyMapChangesToMapTempFile( TRUE );

									RemoveStructFromLevelNode( sNewGridNo, pNewNode );
									AddWallToStructLayer( sNewGridNo, sNewIndex, TRUE );

									ApplyMapChangesToMapTempFile( FALSE );

								}
							}

							// Move in EAST
							sNewGridNo = NewGridNo( pBase->sGridNo, DirectionInc( EAST ) );

							pNewNode = GetWallLevelNodeAndStructOfSameOrientationAtGridno( sNewGridNo, pCurrent->ubWallOrientation, &pWallStruct );

							if ( pNewNode != NULL )
							{
								if ( pWallStruct->fFlags & STRUCTURE_WALL )
								{
									if ( pCurrent->ubWallOrientation == OUTSIDE_TOP_LEFT )
									{
										sSubIndex = 49;
									}
									else
									{
										sSubIndex = 53;
									}

									// Replace!
									GetTileIndexFromTypeSubIndex( gTileDatabase[ pNewNode->usIndex ].fType, sSubIndex, &sNewIndex );

									//Set a flag indicating that the following changes are to go the the maps, temp file
									ApplyMapChangesToMapTempFile( TRUE );

									RemoveStructFromLevelNode( sNewGridNo, pNewNode );
									AddWallToStructLayer( sNewGridNo, sNewIndex, TRUE );

									ApplyMapChangesToMapTempFile( FALSE );
								}
							}

							// look for attached structures in same tile
							sNewGridNo = pBase->sGridNo;
							pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_LEFT_WALL );
							while (pAttached)
							{
								pAttachedBase = FindBaseStructure( pAttached );
								if (pAttachedBase)
								{
				          // Remove the beast!
				          while ( (*ppNextCurrent) != NULL && (*ppNextCurrent)->usStructureID == pAttachedBase->usStructureID )
				          {
					          // the next structure will also be deleted so we had better
					          // skip past it!
					          (*ppNextCurrent) = (*ppNextCurrent)->pNext;
				          }

									pAttachedNode = FindLevelNodeBasedOnStructure( pAttachedBase->sGridNo, pAttachedBase );
									if (pAttachedNode)
									{
										ApplyMapChangesToMapTempFile( TRUE );
										RemoveStructFromLevelNode( pAttachedBase->sGridNo, pAttachedNode );
										ApplyMapChangesToMapTempFile( FALSE );
									}
									else
									{
										// error!
										#ifdef JA2BETAVERSION
											ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
										#endif
										break;
									}
								}
								else
								{
									// error!
									#ifdef JA2BETAVERSION
										ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
									#endif
									break;
								}
								// search for another, from the start of the list
								pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_LEFT_WALL );
							}

							// Move in SOUTH, looking for attached structures to remove
							sNewGridNo = NewGridNo( pBase->sGridNo, DirectionInc( SOUTH ) );
							pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_LEFT_WALL );
							while (pAttached)
							{
								pAttachedBase = FindBaseStructure( pAttached );
								if (pAttachedBase)
								{
									pAttachedNode = FindLevelNodeBasedOnStructure( pAttachedBase->sGridNo, pAttachedBase );
									if (pAttachedNode)
									{
										ApplyMapChangesToMapTempFile( TRUE );
										RemoveStructFromLevelNode( pAttachedBase->sGridNo, pAttachedNode );
										ApplyMapChangesToMapTempFile( FALSE );
									}
									else
									{
										// error!
										#ifdef JA2BETAVERSION
											ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
										#endif
										break;
									}
								}
								else
								{
									// error!
									#ifdef JA2BETAVERSION
										ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
									#endif
									break;
								}
								// search for another, from the start of the list
								pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_LEFT_WALL );
							}
							break;

						case OUTSIDE_TOP_RIGHT:
						case INSIDE_TOP_RIGHT:

							// Move in NORTH
							sNewGridNo = NewGridNo( pBase->sGridNo, DirectionInc( NORTH ) );

							pNewNode = GetWallLevelNodeAndStructOfSameOrientationAtGridno( sNewGridNo, pCurrent->ubWallOrientation, &pWallStruct );

							if ( pNewNode != NULL )
							{
								if ( pWallStruct->fFlags & STRUCTURE_WALL )
								{
									if ( pCurrent->ubWallOrientation == OUTSIDE_TOP_RIGHT )
									{
										sSubIndex = 51;
									}
									else
									{
										sSubIndex = 55;
									}

									// Replace!
									GetTileIndexFromTypeSubIndex( gTileDatabase[ pNewNode->usIndex ].fType, sSubIndex, &sNewIndex );

									//Set a flag indicating that the following changes are to go the the maps, temp file
									ApplyMapChangesToMapTempFile( TRUE );

									RemoveStructFromLevelNode( sNewGridNo, pNewNode );
									AddWallToStructLayer( sNewGridNo, sNewIndex, TRUE );

									ApplyMapChangesToMapTempFile( FALSE );
								}
							}

							// Move in SOUTH
							sNewGridNo = NewGridNo( pBase->sGridNo, DirectionInc( SOUTH ) );

							pNewNode = GetWallLevelNodeAndStructOfSameOrientationAtGridno( sNewGridNo, pCurrent->ubWallOrientation, &pWallStruct );

							if ( pNewNode != NULL )
							{
								if ( pWallStruct->fFlags & STRUCTURE_WALL )
								{
									if ( pCurrent->ubWallOrientation == OUTSIDE_TOP_RIGHT )
									{
										sSubIndex = 50;
									}
									else
									{
										sSubIndex = 54;
									}

									// Replace!
									GetTileIndexFromTypeSubIndex( gTileDatabase[ pNewNode->usIndex ].fType, sSubIndex, &sNewIndex );

									//Set a flag indicating that the following changes are to go the the maps, temp file
									ApplyMapChangesToMapTempFile( TRUE );

									RemoveStructFromLevelNode( sNewGridNo, pNewNode );
									AddWallToStructLayer( sNewGridNo, sNewIndex, TRUE );

									ApplyMapChangesToMapTempFile( FALSE );
								}
							}

							// looking for attached structures to remove in base tile
							sNewGridNo = pBase->sGridNo;
							pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_RIGHT_WALL );
							while (pAttached)
							{
								pAttachedBase = FindBaseStructure( pAttached );
								if (pAttachedBase)
								{
									pAttachedNode = FindLevelNodeBasedOnStructure( pAttachedBase->sGridNo, pAttachedBase );
									if (pAttachedNode)
									{
										ApplyMapChangesToMapTempFile( TRUE );
										RemoveStructFromLevelNode( pAttachedBase->sGridNo, pAttachedNode );
										ApplyMapChangesToMapTempFile( FALSE );
									}
									else
									{
										// error!
										#ifdef JA2BETAVERSION
											ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
										#endif
										break;
									}
								}
								else
								{
									// error!
									#ifdef JA2BETAVERSION
										ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
									#endif
									break;
								}
								// search for another, from the start of the list
								pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_RIGHT_WALL );
							}

							// Move in EAST, looking for attached structures to remove
							sNewGridNo = NewGridNo( pBase->sGridNo, DirectionInc( EAST ) );
							pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_RIGHT_WALL );
							while (pAttached)
							{
								pAttachedBase = FindBaseStructure( pAttached );
								if (pAttachedBase)
								{
									pAttachedNode = FindLevelNodeBasedOnStructure( pAttachedBase->sGridNo, pAttachedBase );
									if (pAttachedNode)
									{
										ApplyMapChangesToMapTempFile( TRUE );
										RemoveStructFromLevelNode( pAttachedBase->sGridNo, pAttachedNode );
										ApplyMapChangesToMapTempFile( FALSE );
									}
									else
									{
										// error!
										#ifdef JA2BETAVERSION
											ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
										#endif
										break;
									}
								}
								else
								{
									// error!
									#ifdef JA2BETAVERSION
										ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Problems removing structure attached to wall at %d", sNewGridNo );
									#endif
									break;
								}
								// search for another, from the start of the list
								pAttached = FindStructure( sNewGridNo, STRUCTURE_ON_RIGHT_WALL );
							}

							break;
					}

					// CJC, Sept 16: if we destroy any wall of the brothel, make Kingpin's men hostile!
					if ( gWorldSectorX == 5 && gWorldSectorY == MAP_ROW_C && gbWorldSectorZ == 0 )
					{
						UINT8			ubRoom;
						BOOLEAN		fInRoom;

						fInRoom = InARoom( sGridNo, &ubRoom );
						if ( !fInRoom )
						{
							// try to south
							fInRoom = InARoom( (INT16)( sGridNo + DirectionInc( SOUTH ) ), &ubRoom );
							if ( !fInRoom )
							{
								// try to east
								fInRoom = InARoom( (INT16)( sGridNo + DirectionInc( EAST ) ), &ubRoom );
							}
						}

						if ( fInRoom && IN_BROTHEL( ubRoom ) )
						{
							CivilianGroupChangesSides( KINGPIN_CIV_GROUP );
						}
					}

				}

				// OK, we need to remove the water from the fountain 
				// Lots of HARD CODING HERE :(
				// Get tile type
				GetTileType( pNode->usIndex, &uiTileType );
				// Check if we are a fountain!
				if ( stricmp( gTilesets[ giCurrentTilesetID ].TileSurfaceFilenames[ uiTileType ], "fount1.sti" ) == 0 )
				{
					// Yes we are!
					// Remove water....
					ApplyMapChangesToMapTempFile( TRUE );
					GetTileIndexFromTypeSubIndex( uiTileType, 1, &sNewIndex );
					RemoveStruct( sBaseGridNo, sNewIndex );
					RemoveStruct( sBaseGridNo, sNewIndex );
					GetTileIndexFromTypeSubIndex( uiTileType, 2, &sNewIndex );
					RemoveStruct( sBaseGridNo, sNewIndex );
					RemoveStruct( sBaseGridNo, sNewIndex );
					GetTileIndexFromTypeSubIndex( uiTileType, 3, &sNewIndex );
					RemoveStruct( sBaseGridNo, sNewIndex );
					RemoveStruct( sBaseGridNo, sNewIndex );
					ApplyMapChangesToMapTempFile( FALSE );
				}


				// Remove any interactive tiles we could be over!
				BeginCurInteractiveTileCheck( INTILE_CHECK_SELECTIVE );

				if ( pCurrent->fFlags & STRUCTURE_WALLSTUFF )
				{
					RecompileLocalMovementCostsForWall( pBase->sGridNo, pBase->ubWallOrientation );
				}

				// Remove!
				//Set a flag indicating that the following changes are to go the the maps, temp file
				ApplyMapChangesToMapTempFile( TRUE );
				RemoveStructFromLevelNode( pBase->sGridNo, pNode );
				ApplyMapChangesToMapTempFile( FALSE );

				// OK, if we are to swap structures, do it now...
				if ( fContinue == 2 )
				{
					// We have a levelnode...
					// Get new index for new grpahic....
					GetTileIndexFromTypeSubIndex( uiTileType, bDestructionPartner, &usTileIndex );					

					ApplyMapChangesToMapTempFile( TRUE );

					AddStructToHead( sBaseGridNo, usTileIndex );

					ApplyMapChangesToMapTempFile( FALSE );


				}

				// Rerender world!
				// Reevaluate world movement costs, reduncency!
				gTacticalStatus.uiFlags |= NOHIDE_REDUNDENCY;
				// FOR THE NEXT RENDER LOOP, RE-EVALUATE REDUNDENT TILES
				InvalidateWorldRedundency( );
				SetRenderFlags(RENDER_FLAG_FULL);
				// Movement costs!
				( *pfRecompileMovementCosts ) = TRUE;

				{
					// Make secondary explosion if eplosive....
					if ( fExplosive )
					{
						InternalIgniteExplosion( ubOwner, CenterX( sBaseGridNo ), CenterY( sBaseGridNo ), 0, sBaseGridNo, STRUCTURE_EXPLOSION, FALSE, bLevel );
					}
				}

				if ( fContinue == 2 )
				{
					return( FALSE );
				}
			}
		
			// 2 is NO DAMAGE
			return( 2 );
		}
	}

	return( 1 );
	
}

STRUCTURE *gStruct;

void ExplosiveDamageGridNo( INT16 sGridNo, INT16 sWoundAmt, UINT32 uiDist, BOOLEAN *pfRecompileMovementCosts, BOOLEAN fOnlyWalls, INT8 bMultiStructSpecialFlag, BOOLEAN fSubSequentMultiTilesTransitionDamage, UINT8 ubOwner, INT8 bLevel )
{
	STRUCTURE							* pCurrent, *pNextCurrent, *pStructure;
	STRUCTURE *						pBaseStructure;
	INT16									sDesiredLevel;
	DB_STRUCTURE_TILE			**ppTile;
	UINT8									ubLoop, ubLoop2;
	INT16									sNewGridNo, sNewGridNo2, sBaseGridNo;
	BOOLEAN								fToBreak = FALSE;
	BOOLEAN								fMultiStructure = FALSE;
	UINT8									ubNumberOfTiles;
	BOOLEAN								fMultiStructSpecialFlag = FALSE;
	BOOLEAN								fExplodeDamageReturn = FALSE;

	// Based on distance away, damage any struct at this gridno
	// OK, loop through structures and damage!
	pCurrent			 =  gpWorldLevelData[ sGridNo ].pStructureHead;
	sDesiredLevel	 = STRUCTURE_ON_GROUND;

	// This code gets a little hairy because 
	// (1) we might need to destroy the currently-examined structure
	while (pCurrent != NULL)
	{
		// ATE: These are for the chacks below for multi-structs....
		pBaseStructure = FindBaseStructure( pCurrent );

		if ( pBaseStructure )
		{
			sBaseGridNo = pBaseStructure->sGridNo;
			ubNumberOfTiles = pBaseStructure->pDBStructureRef->pDBStructure->ubNumberOfTiles;
			fMultiStructure = ( ( pBaseStructure->fFlags & STRUCTURE_MULTI ) != 0 );
      ppTile = MemAlloc( sizeof( DB_STRUCTURE_TILE ) * ubNumberOfTiles );
      memcpy( ppTile, pBaseStructure->pDBStructureRef->ppTile, sizeof( DB_STRUCTURE_TILE ) * ubNumberOfTiles );

			if ( bMultiStructSpecialFlag == -1 )
			{
				// Set it!
				bMultiStructSpecialFlag = ( ( pBaseStructure->fFlags & STRUCTURE_SPECIAL ) != 0 );
			}

			if ( pBaseStructure->fFlags & STRUCTURE_EXPLOSIVE )
			{
				// ATE: Set hit points to zero....
				pBaseStructure->ubHitPoints = 0;
			}
		}
		else
		{
			fMultiStructure = FALSE;
		}

		pNextCurrent = pCurrent->pNext;
		gStruct = pNextCurrent;

		// Check level!
		if (pCurrent->sCubeOffset == sDesiredLevel )
		{
			fExplodeDamageReturn = ExplosiveDamageStructureAtGridNo( pCurrent, &pNextCurrent,  sGridNo, sWoundAmt, uiDist, pfRecompileMovementCosts, fOnlyWalls, 0, ubOwner, bLevel );

			// Are we overwritting damage due to multi-tile...?
			if ( fExplodeDamageReturn )
			{
				if ( fSubSequentMultiTilesTransitionDamage == 2)
				{
					fExplodeDamageReturn = 2;
				}
				else
				{
					fExplodeDamageReturn = 1;
				}
			}

			if ( !fExplodeDamageReturn )
			{
				fToBreak = TRUE;
			}
		}

		// OK, for multi-structs...
		// AND we took damage...
		if ( fMultiStructure && !fOnlyWalls && fExplodeDamageReturn == 0 )
		{
			// ATE: Don't after first attack...
			if ( uiDist > 1 )
			{
		    if ( pBaseStructure )
		    {
          MemFree( ppTile );
        }
				return;
			}

			{

				for ( ubLoop = BASE_TILE; ubLoop < ubNumberOfTiles; ubLoop++)
				{
					sNewGridNo = sBaseGridNo + ppTile[ubLoop]->sPosRelToBase;
					
					// look in adjacent tiles
					for ( ubLoop2 = 0; ubLoop2 < NUM_WORLD_DIRECTIONS; ubLoop2++ )
					{
						sNewGridNo2 = NewGridNo( sNewGridNo, DirectionInc( ubLoop2 ) );
						if ( sNewGridNo2 != sNewGridNo && sNewGridNo2 != sGridNo )
						{
							pStructure = FindStructure( sNewGridNo2, STRUCTURE_MULTI );
							if ( pStructure ) 
							{
								fMultiStructSpecialFlag = ( ( pStructure->fFlags & STRUCTURE_SPECIAL ) != 0 );

								if ( ( bMultiStructSpecialFlag == fMultiStructSpecialFlag ) )
								{
									// If we just damaged it, use same damage value....
									if ( fMultiStructSpecialFlag )
									{
										ExplosiveDamageGridNo( sNewGridNo2, sWoundAmt, uiDist, pfRecompileMovementCosts, fOnlyWalls, bMultiStructSpecialFlag, 1, ubOwner, bLevel );
									}
									else
									{
										ExplosiveDamageGridNo( sNewGridNo2, sWoundAmt, uiDist, pfRecompileMovementCosts, fOnlyWalls, bMultiStructSpecialFlag, 2, ubOwner, bLevel );
									}

									{
										InternalIgniteExplosion( ubOwner, CenterX( sNewGridNo2 ), CenterY( sNewGridNo2 ), 0, sNewGridNo2, RDX, FALSE, bLevel );
									}

									fToBreak = TRUE;
								}
							}
						}
					}
				}
			}
			if ( fToBreak )
			{
				break;
			}
		}

		if ( pBaseStructure )
		{
      MemFree( ppTile );
    }

		pCurrent = pNextCurrent;
	}

}


BOOLEAN DamageSoldierFromBlast( UINT8 ubPerson, UINT8 ubOwner, INT16 sBombGridNo, INT16 sWoundAmt, INT16 sBreathAmt, UINT32 uiDist, UINT16 usItem, INT16 sSubsequent )
{
	 SOLDIERTYPE *pSoldier;
	 INT16 sNewWoundAmt = 0;
	 UINT8		ubDirection;

	 pSoldier = MercPtrs[ ubPerson ];   // someone is here, and they're gonna get hurt

	 if (!pSoldier->bActive || !pSoldier->bInSector || !pSoldier->bLife )
		 return( FALSE );

	 if ( pSoldier->ubMiscSoldierFlags & SOLDIER_MISC_HURT_BY_EXPLOSION )
	 {
		// don't want to damage the guy twice
		return( FALSE );
	 }

	 // Direction to center of explosion
	 ubDirection = (UINT8)GetDirectionFromGridNo( sBombGridNo, pSoldier );

	 // Increment attack counter...
	 gTacticalStatus.ubAttackBusyCount++;
	 DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Incrementing Attack: Explosion dishing out damage, Count now %d", gTacticalStatus.ubAttackBusyCount ) );

	 sNewWoundAmt = sWoundAmt - __min( sWoundAmt, 35 ) * ArmourVersusExplosivesPercent( pSoldier ) / 100;
	 if ( sNewWoundAmt < 0 )
	 {
		sNewWoundAmt = 0;
	 }
	 EVENT_SoldierGotHit( pSoldier, usItem, sNewWoundAmt, sBreathAmt, ubDirection, (INT16)uiDist, ubOwner, 0, ANIM_CROUCH, sSubsequent, sBombGridNo );

	 pSoldier->ubMiscSoldierFlags |= SOLDIER_MISC_HURT_BY_EXPLOSION;

	 if ( ubOwner != NOBODY && MercPtrs[ ubOwner ]->bTeam == gbPlayerNum && pSoldier->bTeam != gbPlayerNum )
	 {
		ProcessImplicationsOfPCAttack( MercPtrs[ ubOwner ], &pSoldier, REASON_EXPLOSION );
	 }

	 return( TRUE );
}

BOOLEAN DishOutGasDamage( SOLDIERTYPE * pSoldier, EXPLOSIVETYPE * pExplosive, INT16 sSubsequent, BOOLEAN fRecompileMovementCosts, INT16 sWoundAmt, INT16 sBreathAmt, UINT8 ubOwner )
{
 INT8		bPosOfMask = NO_SLOT;

 if (!pSoldier->bActive || !pSoldier->bInSector || !pSoldier->bLife || AM_A_ROBOT( pSoldier ) )
 {
	 return( fRecompileMovementCosts );
 }

 if ( pExplosive->ubType == EXPLOSV_CREATUREGAS )
 {
	 if ( pSoldier->uiStatusFlags & SOLDIER_MONSTER )
	 {
		// unaffected by own gas effects
		return( fRecompileMovementCosts );
	 }
	 if ( sSubsequent && pSoldier->fHitByGasFlags & HIT_BY_CREATUREGAS )
	 {
		// already affected by creature gas this turn
		return( fRecompileMovementCosts );				
	 }
 }
 else // no gas mask help from creature attacks
	// ATE/CJC: gas stuff
	{
	 if ( pExplosive->ubType == EXPLOSV_TEARGAS )
	 {
		 if ( AM_A_ROBOT( pSoldier ) )
		 {
			return( fRecompileMovementCosts );
		 }
		
		// ignore whether subsequent or not if hit this turn 
		 if ( pSoldier->fHitByGasFlags & HIT_BY_TEARGAS )
		 {
			// already affected by creature gas this turn
			return( fRecompileMovementCosts );				
		 }
	 }
	 else if ( pExplosive->ubType == EXPLOSV_MUSTGAS )
	 {
		 if ( AM_A_ROBOT( pSoldier ) )
		 {
			return( fRecompileMovementCosts );
		 }
		
		 if ( sSubsequent && pSoldier->fHitByGasFlags & HIT_BY_MUSTARDGAS )
		 {
			// already affected by creature gas this turn
			return( fRecompileMovementCosts );				
		 }

	 }
	
	 if ( pSoldier->inv[ HEAD1POS ].usItem == GASMASK && pSoldier->inv[ HEAD1POS ].bStatus[0] >= USABLE )
	 {
			bPosOfMask = HEAD1POS;
	 }
	 else if ( pSoldier->inv[ HEAD2POS ].usItem == GASMASK && pSoldier->inv[ HEAD2POS ].bStatus[0] >= USABLE )
	 {
			bPosOfMask = HEAD2POS;
	 }

	 if ( bPosOfMask != NO_SLOT  )
	 {
		 if ( pSoldier->inv[ bPosOfMask ].bStatus[0] < GASMASK_MIN_STATUS )
		 {
			 // GAS MASK reduces breath loss by its work% (it leaks if not at least 70%)
			 sBreathAmt = ( sBreathAmt * ( 100 - pSoldier->inv[ bPosOfMask ].bStatus[0] ) ) / 100;
			 if ( sBreathAmt > 500 )
			 {
					// if at least 500 of breath damage got through
					// the soldier within the blast radius is gassed for at least one
					// turn, possibly more if it's tear gas (which hangs around a while)
					pSoldier->uiStatusFlags |= SOLDIER_GASSED;
			 }

			 if ( pSoldier->uiStatusFlags & SOLDIER_PC )
			 {

				 if ( sWoundAmt > 1 )
				 {
					pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 4 );
					sWoundAmt = ( sWoundAmt * ( 100 -  pSoldier->inv[ bPosOfMask ].bStatus[0] ) ) / 100;
				 }
				 else if ( sWoundAmt == 1 )
				 {
					pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 2 );
				 }
			 }
		 }
		 else
		 {
			sBreathAmt = 0;
			if ( sWoundAmt > 0 )
			{
			 if ( sWoundAmt == 1 )
			 {
				pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 2 );
			 }
			 else
			 {
				// use up gas mask
				pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 4 );
			 }
			}
			sWoundAmt = 0;					
		 }

	 }
	}

	if ( sWoundAmt != 0 || sBreathAmt != 0 )
	{
		switch( pExplosive->ubType )
		{
			case EXPLOSV_CREATUREGAS:
				pSoldier->fHitByGasFlags |= HIT_BY_CREATUREGAS;
				break;
			case EXPLOSV_TEARGAS:
				pSoldier->fHitByGasFlags |= HIT_BY_TEARGAS;
				break;
			case EXPLOSV_MUSTGAS:
				pSoldier->fHitByGasFlags |= HIT_BY_MUSTARDGAS;
				break;
			default:
				break;
		}
		// a gas effect, take damage directly...
		SoldierTakeDamage( pSoldier, ANIM_STAND, sWoundAmt, sBreathAmt, TAKE_DAMAGE_GAS, NOBODY, NOWHERE, 0, TRUE );
		if ( pSoldier->bLife >= CONSCIOUSNESS )
		{
			DoMercBattleSound( pSoldier, (INT8)( BATTLE_SOUND_HIT1 + Random( 2 ) ) );
		}

	  if ( ubOwner != NOBODY && MercPtrs[ ubOwner ]->bTeam == gbPlayerNum && pSoldier->bTeam != gbPlayerNum )
	  {
		  ProcessImplicationsOfPCAttack( MercPtrs[ ubOwner ], &pSoldier, REASON_EXPLOSION );
	  }
	}
	return( fRecompileMovementCosts );
}

BOOLEAN ExpAffect( INT16 sBombGridNo, INT16 sGridNo, UINT32 uiDist, UINT16 usItem, UINT8 ubOwner,  INT16 sSubsequent, BOOLEAN *pfMercHit, INT8 bLevel, INT32 iSmokeEffectID )
{
	INT16 sWoundAmt = 0,sBreathAmt = 0, sNewWoundAmt = 0, sNewBreathAmt = 0, sStructDmgAmt;
	UINT8 ubPerson;
	SOLDIERTYPE *pSoldier;
	EXPLOSIVETYPE *pExplosive;
	INT16 sX, sY;
	BOOLEAN fRecompileMovementCosts = FALSE;
	BOOLEAN fSmokeEffect=FALSE;
	BOOLEAN fStunEffect = FALSE;
	INT8		bSmokeEffectType = 0;
	BOOLEAN	fBlastEffect = TRUE;
	INT16		sNewGridNo;
	BOOLEAN	fBloodEffect = FALSE;
	ITEM_POOL * pItemPool, * pItemPoolNext;
	UINT32	uiRoll;

	//Init the variables
	sX = sY = -1;

	if ( sSubsequent == BLOOD_SPREAD_EFFECT )
	{
		fSmokeEffect			= FALSE;
		fBlastEffect			= FALSE;
		fBloodEffect			= TRUE;
	}
	else
	{
		 // Turn off blast effect if some types of items...
		 switch( usItem )
		 {
				case MUSTARD_GRENADE:

					fSmokeEffect			= TRUE;
					bSmokeEffectType	=	MUSTARDGAS_SMOKE_EFFECT; 
					fBlastEffect			= FALSE;
					break;

				case TEARGAS_GRENADE:
				case GL_TEARGAS_GRENADE:
				case BIG_TEAR_GAS:

					fSmokeEffect			= TRUE;
					bSmokeEffectType	=	TEARGAS_SMOKE_EFFECT; 
					fBlastEffect			= FALSE;
					break;

				case SMOKE_GRENADE:
				case GL_SMOKE_GRENADE:

					fSmokeEffect			= TRUE;
					bSmokeEffectType	=	NORMAL_SMOKE_EFFECT; 
					fBlastEffect			= FALSE;
					break;

				case STUN_GRENADE:
				case GL_STUN_GRENADE:
					fStunEffect				= TRUE;
					break;

        case SMALL_CREATURE_GAS:
        case LARGE_CREATURE_GAS:
        case VERY_SMALL_CREATURE_GAS:

					fSmokeEffect			= TRUE;
					bSmokeEffectType	=	CREATURE_SMOKE_EFFECT; 
					fBlastEffect			= FALSE;
			    break;
		 }
	}


	// OK, here we:
	// Get explosive data from table
	pExplosive = &(Explosive[ Item[ usItem ].ubClassIndex ] );

	uiRoll = PreRandom( 100 );

	// Calculate wound amount
	sWoundAmt = pExplosive->ubDamage + (INT16) ( (pExplosive->ubDamage * uiRoll) / 100 );

	// Calculate breath amount ( if stun damage applicable )
	sBreathAmt = ( pExplosive->ubStunDamage * 100 ) + (INT16) ( ( ( pExplosive->ubStunDamage / 2 ) * 100 * uiRoll ) / 100 ) ;

  // ATE: Make sure guys get pissed at us!
  HandleBuldingDestruction( sGridNo ,ubOwner );


	if ( fBlastEffect )
	{
		// lower effects for distance away from center of explosion
		// If radius is 3, damage % is (100)/66/33/17
		// If radius is 5, damage % is (100)/80/60/40/20/10
		// If radius is 8, damage % is (100)/88/75/63/50/37/25/13/6

		if ( pExplosive->ubRadius == 0 )
		{
			// leave as is, has to be at range 0 here
		}
		else if (uiDist < pExplosive->ubRadius)
		{
			// if radius is 5, go down by 5ths ~ 20%
			sWoundAmt -= (INT16)  (sWoundAmt * uiDist / pExplosive->ubRadius );
			sBreathAmt -= (INT16) (sBreathAmt * uiDist / pExplosive->ubRadius );
		}
		else
		{
			// at the edge of the explosion, do half the previous damage
			sWoundAmt = (INT16) ( (sWoundAmt / pExplosive->ubRadius) / 2);
			sBreathAmt = (INT16) ( (sBreathAmt / pExplosive->ubRadius) / 2);
		}

		if (sWoundAmt < 0)
			sWoundAmt = 0;

		if (sBreathAmt < 0)
			sBreathAmt = 0;

		// damage structures
		if ( uiDist <= __max( 1, (UINT32) (pExplosive->ubDamage / 30) ) )
		{
			if ( Item[ usItem ].usItemClass & IC_GRENADE )
			{
				sStructDmgAmt = sWoundAmt / 3;				
			}
			else // most explosives
			{
				sStructDmgAmt = sWoundAmt;
			}
			
			ExplosiveDamageGridNo( sGridNo, sStructDmgAmt, uiDist, &fRecompileMovementCosts, FALSE, -1, 0 , ubOwner, bLevel );

			// ATE: Look for damage to walls ONLY for next two gridnos
			sNewGridNo = NewGridNo( sGridNo, DirectionInc( NORTH ) );

			if ( GridNoOnVisibleWorldTile( sNewGridNo ) )
			{
				ExplosiveDamageGridNo( sNewGridNo, sStructDmgAmt, uiDist, &fRecompileMovementCosts, TRUE, -1, 0, ubOwner, bLevel );
			}

			// ATE: Look for damage to walls ONLY for next two gridnos
			sNewGridNo = NewGridNo( sGridNo, DirectionInc( WEST ) );

			if ( GridNoOnVisibleWorldTile( sNewGridNo ) )
			{
				ExplosiveDamageGridNo( sNewGridNo, sStructDmgAmt, uiDist, &fRecompileMovementCosts, TRUE, -1, 0, ubOwner, bLevel );
			}

		}

		// Add burn marks to ground randomly....
		if ( Random( 50 ) < 15 && uiDist == 1 )
		{
			//if ( !TypeRangeExistsInObjectLayer( sGridNo, FIRSTEXPLDEBRIS, SECONDEXPLDEBRIS, &usObjectIndex ) )
			//{
			//	GetTileIndexFromTypeSubIndex( SECONDEXPLDEBRIS, (UINT16)( Random( 10 ) + 1 ), &usTileIndex );
			//	AddObjectToHead( sGridNo, usTileIndex );

			//	SetRenderFlags(RENDER_FLAG_FULL);

			//}
		}

		// NB radius can be 0 so cannot divide it by 2 here
		if (!fStunEffect && (uiDist * 2 <= pExplosive->ubRadius)  )
		{
			GetItemPool( sGridNo, &pItemPool, bLevel );

			while( pItemPool )
			{
				pItemPoolNext = pItemPool->pNext;

				if ( DamageItemOnGround( &(gWorldItems[ pItemPool->iItemIndex ].o), sGridNo, bLevel, (INT32) (sWoundAmt * 2), ubOwner ) )
				{
					// item was destroyed
					RemoveItemFromPool( sGridNo, pItemPool->iItemIndex, bLevel );
				}
				pItemPool = pItemPoolNext;
			}

			/*
			// Search for an explosive item in item pool
			while ( ( iWorldItem = GetItemOfClassTypeInPool( sGridNo, IC_EXPLOSV, bLevel ) ) != -1 )
			{
				// Get usItem
				usItem = gWorldItems[ iWorldItem ].o.usItem;

				DamageItem

				if ( CheckForChainReaction( usItem, gWorldItems[ iWorldItem ].o.bStatus[0], sWoundAmt, TRUE ) )
				{
					RemoveItemFromPool( sGridNo, iWorldItem, bLevel );

					// OK, Ignite this explosion!
					IgniteExplosion( NOBODY, sX, sY, 0, sGridNo, usItem, bLevel );
				}
				else
				{
					RemoveItemFromPool( sGridNo, iWorldItem, bLevel );
				}

			}

			// Remove any unburied items here!
			RemoveAllUnburiedItems( sGridNo, bLevel );
			*/
		}
 }
 else if ( fSmokeEffect )
 {
		// If tear gar, determine turns to spread.....
		if ( sSubsequent == ERASE_SPREAD_EFFECT )
		{
			RemoveSmokeEffectFromTile( sGridNo, bLevel );
		}
		else if ( sSubsequent != REDO_SPREAD_EFFECT )
		{
			AddSmokeEffectToTile( iSmokeEffectID, bSmokeEffectType, sGridNo, bLevel );
		}
 }
 else
 {
		// Drop blood ....
	  // Get blood quantity....
		InternalDropBlood( sGridNo, 0, 0, (UINT8)(__max( ( MAXBLOODQUANTITY - ( uiDist * 2 ) ), 0 ) ), 1 );
 }

 if ( sSubsequent != ERASE_SPREAD_EFFECT && sSubsequent != BLOOD_SPREAD_EFFECT )
 {
	 // if an explosion effect....
	 if ( fBlastEffect )
	 {
		 // don't hurt anyone who is already dead & waiting to be removed
		 if ( ( ubPerson = WhoIsThere2( sGridNo, bLevel ) ) != NOBODY )
		 {
			 DamageSoldierFromBlast( ubPerson, ubOwner, sBombGridNo, sWoundAmt, sBreathAmt, uiDist, usItem, sSubsequent );
		 }

		 if ( bLevel == 1 )
		 {
			 if ( ( ubPerson = WhoIsThere2(sGridNo, 0 ) ) != NOBODY )
			 {
				 if ( (sWoundAmt / 2) > 20 )
				 {
					 // debris damage!
					 if ( (sBreathAmt / 2) > 20 )
					 {
						 DamageSoldierFromBlast( ubPerson, ubOwner, sBombGridNo, (INT16) Random( (sWoundAmt / 2) - 20 ), (INT16) Random( (sBreathAmt / 2) - 20 ), uiDist, usItem, sSubsequent );
					 }
					 else
					 {
						 DamageSoldierFromBlast( ubPerson, ubOwner, sBombGridNo, (INT16) Random( (sWoundAmt / 2) - 20 ), 1, uiDist, usItem, sSubsequent );
					 }
				
				 }

			 }
		 }
	 }
	 else
	 {
		 if ( ( ubPerson = WhoIsThere2(sGridNo, bLevel ) ) >= NOBODY )
		 {
			 return( fRecompileMovementCosts );
		 }

		 pSoldier = MercPtrs[ ubPerson ];   // someone is here, and they're gonna get hurt

		 fRecompileMovementCosts = DishOutGasDamage( pSoldier, pExplosive, sSubsequent, fRecompileMovementCosts, sWoundAmt, sBreathAmt, ubOwner );
/*
		 if (!pSoldier->bActive || !pSoldier->bInSector || !pSoldier->bLife || AM_A_ROBOT( pSoldier ) )
		 {
			 return( fRecompileMovementCosts );
		 }

		 if ( pExplosive->ubType == EXPLOSV_CREATUREGAS )
		 {
			 if ( pSoldier->uiStatusFlags & SOLDIER_MONSTER )
			 {
				// unaffected by own gas effects
				return( fRecompileMovementCosts );
			 }
			 if ( sSubsequent && pSoldier->fHitByGasFlags & HIT_BY_CREATUREGAS )
			 {
				// already affected by creature gas this turn
				return( fRecompileMovementCosts );				
			 }
		 }
		 else // no gas mask help from creature attacks
			// ATE/CJC: gas stuff
			{
			 INT8 bPosOfMask = NO_SLOT;


			 if ( pExplosive->ubType == EXPLOSV_TEARGAS )
			 {
				// ignore whether subsequent or not if hit this turn 
				 if ( pSoldier->fHitByGasFlags & HIT_BY_TEARGAS )
				 {
					// already affected by creature gas this turn
					return( fRecompileMovementCosts );				
				 }
			 }
			 else if ( pExplosive->ubType == EXPLOSV_MUSTGAS )
			 {
				 if ( sSubsequent && pSoldier->fHitByGasFlags & HIT_BY_MUSTARDGAS )
				 {
					// already affected by creature gas this turn
					return( fRecompileMovementCosts );				
				 }

			 }
			
			 if ( sSubsequent && pSoldier->fHitByGasFlags & HIT_BY_CREATUREGAS )
			 {
				// already affected by creature gas this turn
				return( fRecompileMovementCosts );				
			 }


			 if ( pSoldier->inv[ HEAD1POS ].usItem == GASMASK && pSoldier->inv[ HEAD1POS ].bStatus[0] >= USABLE )
			 {
					bPosOfMask = HEAD1POS;
			 }
			 else if ( pSoldier->inv[ HEAD2POS ].usItem == GASMASK && pSoldier->inv[ HEAD2POS ].bStatus[0] >= USABLE )
			 {
					bPosOfMask = HEAD2POS;
			 }

			 if ( bPosOfMask != NO_SLOT  )
			 {
				 if ( pSoldier->inv[ bPosOfMask ].bStatus[0] < GASMASK_MIN_STATUS )
				 {
					 // GAS MASK reduces breath loss by its work% (it leaks if not at least 70%)
					 sBreathAmt = ( sBreathAmt * ( 100 - pSoldier->inv[ bPosOfMask ].bStatus[0] ) ) / 100;
					 if ( sBreathAmt > 500 )
					 {
							// if at least 500 of breath damage got through
							// the soldier within the blast radius is gassed for at least one
							// turn, possibly more if it's tear gas (which hangs around a while)
							pSoldier->uiStatusFlags |= SOLDIER_GASSED;
					 }

					 if ( sWoundAmt > 1 )
					 {
					  pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 4 );
						sWoundAmt = ( sWoundAmt * ( 100 -  pSoldier->inv[ bPosOfMask ].bStatus[0] ) ) / 100;
					 }
					 else if ( sWoundAmt == 1 )
					 {
						pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 2 );
					 }
				 }
				 else
				 {
					sBreathAmt = 0;
					if ( sWoundAmt > 0 )
					{
					 if ( sWoundAmt == 1 )
					 {
						pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 2 );
					 }
					 else
					 {
						// use up gas mask
						pSoldier->inv[ bPosOfMask ].bStatus[0] -= (INT8) Random( 4 );
					 }
					}
					sWoundAmt = 0;					
				 }

			 }
			}

			if ( sWoundAmt != 0 || sBreathAmt != 0 )
			{
				switch( pExplosive->ubType )
				{
					case EXPLOSV_CREATUREGAS:
						pSoldier->fHitByGasFlags |= HIT_BY_CREATUREGAS;
						break;
					case EXPLOSV_TEARGAS:
						pSoldier->fHitByGasFlags |= HIT_BY_TEARGAS;
						break;
					case EXPLOSV_MUSTGAS:
						pSoldier->fHitByGasFlags |= HIT_BY_MUSTARDGAS;
						break;
					default:
						break;
				}
				// a gas effect, take damage directly...
				SoldierTakeDamage( pSoldier, ANIM_STAND, sWoundAmt, sBreathAmt, TAKE_DAMAGE_GAS, NOBODY, NOWHERE, 0, TRUE );
				if ( pSoldier->bLife >= CONSCIOUSNESS )
				{
					DoMercBattleSound( pSoldier, (INT8)( BATTLE_SOUND_HIT1 + Random( 2 ) ) );
				}
			}
			*/
	 }

	 (*pfMercHit) = TRUE;	
 }

 return( fRecompileMovementCosts );

}

void GetRayStopInfo( UINT32 uiNewSpot, UINT8 ubDir, INT8 bLevel, BOOLEAN fSmokeEffect, INT32 uiCurRange, INT32 *piMaxRange, UINT8 *pubKeepGoing )
{
   INT8         bStructHeight;
   UINT8				ubMovementCost;
   INT8					Blocking, BlockingTemp;
	 BOOLEAN      fTravelCostObs = FALSE;
   UINT32       uiRangeReduce;
   INT16        sNewGridNo;
	 STRUCTURE *	pBlockingStructure;
   BOOLEAN      fBlowWindowSouth = FALSE;
	 BOOLEAN			fReduceRay = TRUE;

	 ubMovementCost = gubWorldMovementCosts[ uiNewSpot ][ ubDir ][ bLevel ];

	 if ( IS_TRAVELCOST_DOOR( ubMovementCost ) )
	 {
		 ubMovementCost = DoorTravelCost( NULL, uiNewSpot, ubMovementCost, FALSE, NULL );
		 // If we have hit a wall, STOP HERE
		 if (ubMovementCost >= TRAVELCOST_BLOCKED)
		 {
			 fTravelCostObs  = TRUE;
		 }
	 }
	 else
	 {
		 // If we have hit a wall, STOP HERE
		 if ( ubMovementCost == TRAVELCOST_WALL )
		 {
				// We have an obstacle here..
				fTravelCostObs = TRUE;
		 }
	 }

  
	 Blocking = GetBlockingStructureInfo( (INT16)uiNewSpot, ubDir, 0, bLevel, &bStructHeight, &pBlockingStructure, TRUE );

	 if ( pBlockingStructure )
	 {
		 if ( pBlockingStructure->fFlags & STRUCTURE_CAVEWALL )
		 {
				// block completely!
				fTravelCostObs = TRUE;
		 }
		 else if ( pBlockingStructure->pDBStructureRef->pDBStructure->ubDensity <= 15 )
		 {
			// not stopped
			fTravelCostObs = FALSE;
			fReduceRay = FALSE;
		 }
	 }

	 if ( fTravelCostObs )
	 {

		 if ( fSmokeEffect )
		 {
			 if ( Blocking == BLOCKING_TOPRIGHT_OPEN_WINDOW || Blocking == BLOCKING_TOPLEFT_OPEN_WINDOW )
			 {
					// If open, fTravelCostObs set to false and reduce range....
					fTravelCostObs = FALSE;
					// Range will be reduced below...
			 }

			 if ( fTravelCostObs )
			 {
				 // ATE: For windows, check to the west and north for a broken window, as movement costs
				 // will override there...
				 sNewGridNo = NewGridNo( (INT16)uiNewSpot, DirectionInc( WEST ) );

				 BlockingTemp = GetBlockingStructureInfo( (INT16)sNewGridNo, ubDir, 0, bLevel, &bStructHeight, &pBlockingStructure, TRUE );
				 if ( BlockingTemp == BLOCKING_TOPRIGHT_OPEN_WINDOW || BlockingTemp == BLOCKING_TOPLEFT_OPEN_WINDOW )
				 {
						// If open, fTravelCostObs set to false and reduce range....
						fTravelCostObs = FALSE;
						// Range will be reduced below...
				 }
				 if ( pBlockingStructure && pBlockingStructure->pDBStructureRef->pDBStructure->ubDensity <= 15 )
				 {
					fTravelCostObs = FALSE;
					fReduceRay = FALSE;
				 }
			 }

			 if ( fTravelCostObs )
			 {
				 sNewGridNo = NewGridNo( (INT16)uiNewSpot, DirectionInc( NORTH ) );

				 BlockingTemp = GetBlockingStructureInfo( (INT16)sNewGridNo, ubDir, 0, bLevel, &bStructHeight, &pBlockingStructure, TRUE );
				 if ( BlockingTemp == BLOCKING_TOPRIGHT_OPEN_WINDOW || BlockingTemp == BLOCKING_TOPLEFT_OPEN_WINDOW )
				 {
						// If open, fTravelCostObs set to false and reduce range....
						fTravelCostObs = FALSE;
						// Range will be reduced below...
				 }
				 if ( pBlockingStructure && pBlockingStructure->pDBStructureRef->pDBStructure->ubDensity <= 15 )
				 {
					fTravelCostObs = FALSE;
					fReduceRay = FALSE;
				 }
			 }

		 }
		 else
		 {
			 // We are a blast effect....

			 // ATE: explode windows!!!!
			 if ( Blocking == BLOCKING_TOPLEFT_WINDOW || Blocking == BLOCKING_TOPRIGHT_WINDOW )
			 {
				 // Explode!
				 if ( ubDir == SOUTH || ubDir == SOUTHEAST || ubDir == SOUTHWEST )
				 {
					 fBlowWindowSouth = TRUE;
				 }

				 if ( pBlockingStructure != NULL )
				 {
       			WindowHit( (INT16)uiNewSpot, pBlockingStructure->usStructureID, fBlowWindowSouth, TRUE );
				 }
			 }

			 // ATE: For windows, check to the west and north for a broken window, as movement costs
			 // will override there...
			 sNewGridNo = NewGridNo( (INT16)uiNewSpot, DirectionInc( WEST ) );

			 BlockingTemp = GetBlockingStructureInfo( (INT16)sNewGridNo, ubDir, 0, bLevel, &bStructHeight, &pBlockingStructure , TRUE );
			 if ( pBlockingStructure && pBlockingStructure->pDBStructureRef->pDBStructure->ubDensity <= 15 )
			 {
				fTravelCostObs = FALSE;
				fReduceRay = FALSE;
			 }
			 if ( BlockingTemp == BLOCKING_TOPRIGHT_WINDOW || BlockingTemp == BLOCKING_TOPLEFT_WINDOW )
			 {
				 if ( pBlockingStructure != NULL )
				 {
       			WindowHit( sNewGridNo, pBlockingStructure->usStructureID, FALSE, TRUE );
				 }
			 }

			 sNewGridNo = NewGridNo( (INT16)uiNewSpot, DirectionInc( NORTH ) );
			 BlockingTemp = GetBlockingStructureInfo( (INT16)sNewGridNo, ubDir, 0, bLevel, &bStructHeight, &pBlockingStructure, TRUE );

			 if ( pBlockingStructure && pBlockingStructure->pDBStructureRef->pDBStructure->ubDensity <= 15 )
			 {
				  fTravelCostObs = FALSE;
				  fReduceRay = FALSE;
			 }
			 if ( BlockingTemp == BLOCKING_TOPRIGHT_WINDOW || BlockingTemp == BLOCKING_TOPLEFT_WINDOW )
			 {
				 if ( pBlockingStructure != NULL )
				 {
       			WindowHit( sNewGridNo, pBlockingStructure->usStructureID, FALSE, TRUE );
				 }
			 }
		 }
	 }

   // Have we hit things like furniture, etc?
	 if ( Blocking != NOTHING_BLOCKING && !fTravelCostObs )
	 {
      // ATE: Tall things should blaock all
      if ( bStructHeight == 4 )
      {
				 (*pubKeepGoing) = FALSE;
      }
      else
      {
        // If we are smoke, reduce range variably....
			  if ( fReduceRay )
			  {
				  if ( fSmokeEffect )
				  {
					  switch( bStructHeight )
					  {
						  case 3:
							  uiRangeReduce = 2;
							  break;

						  case 2:

							  uiRangeReduce = 1;
							  break;

						  default:

							  uiRangeReduce = 0;
							  break;
					  }        
				  }
				  else
				  {
					  uiRangeReduce = 2;
				  }

	  		  ( *piMaxRange ) -= uiRangeReduce;
			  }

			  if ( uiCurRange <= (*piMaxRange) )
			  {
				   (*pubKeepGoing) = TRUE;
			  }
			  else
			  {
				   (*pubKeepGoing) = FALSE;
			  }
      }
	 }
	 else
	 {
      if ( fTravelCostObs )
      {
			  ( *pubKeepGoing ) = FALSE;
      }
      else
      {
			  ( *pubKeepGoing ) = TRUE;
      }
	 }
}



void SpreadEffect( INT16 sGridNo, UINT8 ubRadius, UINT16 usItem, UINT8 ubOwner, BOOLEAN fSubsequent, INT8 bLevel, INT32 iSmokeEffectID  )
{
 INT32 uiNewSpot, uiTempSpot, uiBranchSpot, cnt, branchCnt;
 INT32  uiTempRange, ubBranchRange;
 UINT8  ubDir,ubBranchDir, ubKeepGoing;
 INT16 sRange;
 BOOLEAN		  fRecompileMovement = FALSE;
 BOOLEAN			fAnyMercHit = FALSE;
 BOOLEAN      fSmokeEffect = FALSE;

 switch( usItem )
 {
		case MUSTARD_GRENADE:
		case TEARGAS_GRENADE:
		case GL_TEARGAS_GRENADE:
		case BIG_TEAR_GAS:
		case SMOKE_GRENADE:
		case GL_SMOKE_GRENADE:
    case SMALL_CREATURE_GAS:
    case LARGE_CREATURE_GAS:
    case VERY_SMALL_CREATURE_GAS:

      fSmokeEffect = TRUE;
      break;
 }
 
	// Set values for recompile region to optimize area we need to recompile for MPs
	gsRecompileAreaTop = sGridNo / WORLD_COLS;
	gsRecompileAreaLeft = sGridNo % WORLD_COLS;
	gsRecompileAreaRight = gsRecompileAreaLeft;
	gsRecompileAreaBottom = gsRecompileAreaTop;

 // multiply range by 2 so we can correctly calculate approximately round explosion regions
 sRange = ubRadius * 2;

 // first, affect main spot
 if ( ExpAffect( sGridNo, sGridNo, 0, usItem, ubOwner, fSubsequent, &fAnyMercHit, bLevel, iSmokeEffectID ) )
 {
		fRecompileMovement = TRUE;
 }

 
 for (ubDir = NORTH; ubDir <= NORTHWEST; ubDir++ )
 {
   uiTempSpot = sGridNo;

   uiTempRange = sRange;

	 if (ubDir & 1)
	 {
		cnt = 3;
	 }
	 else
	 {
		cnt = 2;
	 }
	 while( cnt <= uiTempRange) // end of range loop
   {
     // move one tile in direction
     uiNewSpot = NewGridNo( (INT16)uiTempSpot, DirectionInc( ubDir ) );

     // see if this was a different spot & if we should be able to reach
     // this spot
     if (uiNewSpot == uiTempSpot)
		 {
			 ubKeepGoing = FALSE;
		 }
     else
     {
			 // Check if struct is a tree, etc and reduce range...
       GetRayStopInfo( uiNewSpot, ubDir, bLevel, fSmokeEffect, cnt, &uiTempRange, &ubKeepGoing );
     }

     if (ubKeepGoing)
     {
       uiTempSpot = uiNewSpot;

			 //DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Explosion affects %d", uiNewSpot) );
       // ok, do what we do here...
       if ( ExpAffect( sGridNo, (INT16)uiNewSpot, cnt / 2, usItem, ubOwner, fSubsequent, &fAnyMercHit, bLevel, iSmokeEffectID ) )
			 {
					fRecompileMovement = TRUE;
			 }

       // how far should we branch out here?
       ubBranchRange = (UINT8)( sRange - cnt );

       if ( ubBranchRange )
			 {
				 // ok, there's a branch here. Mark where we start this branch.
				 uiBranchSpot = uiNewSpot;

				 // figure the branch direction - which is one dir clockwise
				 ubBranchDir = (ubDir + 1) % 8;

				 if (ubBranchDir & 1)
				 {
					branchCnt = 3;
				 }
				 else
				 {
					branchCnt = 2;
				 }

				 while( branchCnt <= ubBranchRange) // end of range loop
				 {
						ubKeepGoing		 = TRUE;
					  uiNewSpot = NewGridNo( (INT16)uiBranchSpot, DirectionInc(ubBranchDir));

						if (uiNewSpot != uiBranchSpot)
						{
			        // Check if struct is a tree, etc and reduce range...
              GetRayStopInfo( uiNewSpot, ubBranchDir, bLevel, fSmokeEffect, branchCnt, &ubBranchRange, &ubKeepGoing );

						  if ( ubKeepGoing )
							{
								// ok, do what we do here
								//DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Explosion affects %d", uiNewSpot) );
								if ( ExpAffect( sGridNo, (INT16)uiNewSpot, (INT16)((cnt + branchCnt) / 2), usItem, ubOwner, fSubsequent, &fAnyMercHit, bLevel, iSmokeEffectID ) )
								{
									fRecompileMovement = TRUE;
								}
								uiBranchSpot = uiNewSpot;
							}
							//else
							{
								 // check if it's ANY door, and if so, affect that spot so it's damaged
              //   if (RealDoorAt(uiNewSpot))
							//	 {
              //      ExpAffect(sGridNo,uiNewSpot,cnt,ubReason,fSubsequent);
							//	 }
								 // blocked, break out of the the sub-branch loop
							//	 break;
							}
						}

					 if (ubBranchDir & 1)
					 {
						branchCnt += 3;
					 }
					 else
					 {
						branchCnt += 2;
					 }

				}
			} // end of if a branch to do

     }
     else    	// at edge, or tile blocks further spread in that direction
		 {
       break;
		 }

		 if (ubDir & 1)
		 {
			cnt += 3;
		 }
		 else
		 {
			cnt += 2;
		 }
	 }

  }	// end of dir loop

	// Recompile movement costs...
	if ( fRecompileMovement )
	{
		INT16 sX, sY;

		// DO wireframes as well
		ConvertGridNoToXY( (INT16)sGridNo, &sX, &sY );
		SetRecalculateWireFrameFlagRadius( sX, sY, ubRadius );
		CalculateWorldWireFrameTiles( FALSE );
	
		RecompileLocalMovementCostsInAreaWithFlags();
		RecompileLocalMovementCostsFromRadius( sGridNo, MAX_DISTANCE_EXPLOSIVE_CAN_DESTROY_STRUCTURES );

		// if anything has been done to change movement costs and this is a potential POW situation, check
		// paths for POWs
		if ( gWorldSectorX == 13 && gWorldSectorY == MAP_ROW_I )
		{
			DoPOWPathChecks();
		}

	}

	// do sight checks if something damaged or smoke stuff involved
	if ( fRecompileMovement || fSmokeEffect )
	{
		if ( gubElementsOnExplosionQueue )
		{
			gfExplosionQueueMayHaveChangedSight	= TRUE;
		}
	}

	gsRecompileAreaTop = 0;
	gsRecompileAreaLeft = 0;
	gsRecompileAreaRight = 0;
	gsRecompileAreaBottom = 0;

	if (fAnyMercHit)
	{
		// reset explosion hit flag so we can damage mercs again
		for ( cnt = 0; cnt < (INT32)guiNumMercSlots; cnt++ )
		{
			if ( MercSlots[ cnt ] )
			{
				MercSlots[ cnt ]->ubMiscSoldierFlags &= ~SOLDIER_MISC_HURT_BY_EXPLOSION;
			}
		}	
	}

	if ( fSubsequent != BLOOD_SPREAD_EFFECT )
	{
		MakeNoise( NOBODY, sGridNo, bLevel, gpWorldLevelData[sGridNo].ubTerrainID, Explosive[ Item [ usItem ].ubClassIndex ].ubVolume, NOISE_EXPLOSION );

	}
}

void ToggleActionItemsByFrequency( INT8 bFrequency )
{
	UINT32				uiWorldBombIndex;
	OBJECTTYPE *	pObj;

	// Go through all the bombs in the world, and look for remote ones
	for (uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
	{
		if (gWorldBombs[uiWorldBombIndex].fExists)
		{
			pObj = &( gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].o );
			if ( pObj->bDetonatorType == BOMB_REMOTE )
			{
				// Found a remote bomb, so check to see if it has the same frequency
				if (pObj->bFrequency == bFrequency)
				{
					// toggle its active flag
					if (pObj->fFlags & OBJECT_DISABLED_BOMB)
					{
						pObj->fFlags &= (~OBJECT_DISABLED_BOMB);
					}
					else
					{
						pObj->fFlags |= OBJECT_DISABLED_BOMB;
					}
				}
			}
		}
	}
}

void TogglePressureActionItemsInGridNo( INT16 sGridNo )
{
	UINT32				uiWorldBombIndex;
	OBJECTTYPE *	pObj;

	// Go through all the bombs in the world, and look for remote ones
	for (uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
	{
		if ( gWorldBombs[uiWorldBombIndex].fExists && gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].sGridNo == sGridNo )
		{
			pObj = &( gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].o );
			if ( pObj->bDetonatorType == BOMB_PRESSURE )
			{
				// Found a pressure item
				// toggle its active flag
				if (pObj->fFlags & OBJECT_DISABLED_BOMB)
				{
					pObj->fFlags &= (~OBJECT_DISABLED_BOMB);
				}
				else
				{
					pObj->fFlags |= OBJECT_DISABLED_BOMB;
				}
			}
		}
	}
}


void DelayedBillyTriggerToBlockOnExit( void )
{
	if ( WhoIsThere2( gsTempActionGridNo, 0 ) == NOBODY )
	{
		TriggerNPCRecord( BILLY, 6 );
	}
	else
	{
		// delay further!
		SetCustomizableTimerCallbackAndDelay( 1000, DelayedBillyTriggerToBlockOnExit, TRUE );
	}
}

void BillyBlocksDoorCallback( void )
{
	TriggerNPCRecord( BILLY, 6 );
}

BOOLEAN HookerInRoom( UINT8 ubRoom )
{
	UINT8						ubLoop, ubTempRoom;
	SOLDIERTYPE *		pSoldier;

	for ( ubLoop = gTacticalStatus.Team[ CIV_TEAM ].bFirstID; ubLoop <= gTacticalStatus.Team[ CIV_TEAM ].bLastID; ubLoop++ )
	{
		pSoldier = MercPtrs[ ubLoop ];

		if ( pSoldier->bActive && pSoldier->bInSector && pSoldier->bLife >= OKLIFE && pSoldier->bNeutral && pSoldier->ubBodyType == MINICIV )
		{
			if ( InARoom( pSoldier->sGridNo, &ubTempRoom ) && ubTempRoom == ubRoom )
			{
				return( TRUE );
			}
		}
	}

	return( FALSE );
}

void PerformItemAction( INT16 sGridNo, OBJECTTYPE * pObj )
{
	STRUCTURE * pStructure;

	switch( pObj->bActionValue )
	{
		case ACTION_ITEM_OPEN_DOOR:
			pStructure = FindStructure( sGridNo, STRUCTURE_ANYDOOR );
			if (pStructure)
			{
				if (pStructure->fFlags & STRUCTURE_OPEN)
				{
					// it's already open - this MIGHT be an error but probably not
					// because we are basically just ensuring that the door is open
				}
				else
				{
					if (pStructure->fFlags & STRUCTURE_BASE_TILE)
					{
						HandleDoorChangeFromGridNo( NULL, sGridNo, FALSE );
					}
					else
					{
						HandleDoorChangeFromGridNo( NULL, pStructure->sBaseGridNo, FALSE );
					}
					gfExplosionQueueMayHaveChangedSight = TRUE;
				}
			}
			else
			{
				// error message here
				#ifdef JA2BETAVERSION
					ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Action item to open door in gridno %d but there is none!", sGridNo );
				#endif
			}
			break;
		case ACTION_ITEM_CLOSE_DOOR:
			pStructure = FindStructure( sGridNo, STRUCTURE_ANYDOOR );
			if (pStructure)
			{
				if (pStructure->fFlags & STRUCTURE_OPEN)
				{
					if (pStructure->fFlags & STRUCTURE_BASE_TILE)
					{
						HandleDoorChangeFromGridNo( NULL, sGridNo , FALSE );
					}
					else
					{
						HandleDoorChangeFromGridNo( NULL, pStructure->sBaseGridNo, FALSE );
					}
					gfExplosionQueueMayHaveChangedSight = TRUE;
				}
				else
				{
					// it's already closed - this MIGHT be an error but probably not
					// because we are basically just ensuring that the door is closed
				}
			}
			else
			{
				// error message here
				#ifdef JA2BETAVERSION
					ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Action item to close door in gridno %d but there is none!", sGridNo );
				#endif
			}
			break;
		case ACTION_ITEM_TOGGLE_DOOR:
			pStructure = FindStructure( sGridNo, STRUCTURE_ANYDOOR );
			if (pStructure)
			{
				if (pStructure->fFlags & STRUCTURE_BASE_TILE)
				{
					HandleDoorChangeFromGridNo( NULL, sGridNo, FALSE );
				}
				else
				{
					HandleDoorChangeFromGridNo( NULL, pStructure->sBaseGridNo , FALSE );
				}
				gfExplosionQueueMayHaveChangedSight = TRUE;
			}
			else
			{
				// error message here
				#ifdef JA2BETAVERSION
					ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_TESTVERSION, L"Action item to toggle door in gridno %d but there is none!", sGridNo );
				#endif
			}
			break;
		case ACTION_ITEM_UNLOCK_DOOR:
			{
				DOOR * pDoor;
				
				pDoor = FindDoorInfoAtGridNo( sGridNo );
				if ( pDoor )
				{
					pDoor->fLocked = FALSE;
				}
			}
			break;
		case ACTION_ITEM_TOGGLE_LOCK:
			{
				DOOR * pDoor;
				
				pDoor = FindDoorInfoAtGridNo( sGridNo );
				if ( pDoor )
				{
					if ( pDoor->fLocked )
					{
						pDoor->fLocked = FALSE;
					}
					else
					{
						pDoor->fLocked = TRUE;
					}
				}
			}
			break;
		case ACTION_ITEM_UNTRAP_DOOR:
			{
				DOOR * pDoor;
				
				pDoor = FindDoorInfoAtGridNo( sGridNo );
				if ( pDoor )
				{
					pDoor->ubTrapLevel = 0;
					pDoor->ubTrapID = NO_TRAP;
				}
			}
			break;
		case ACTION_ITEM_SMALL_PIT:
			Add3X3Pit( sGridNo );
			SearchForOtherMembersWithinPitRadiusAndMakeThemFall( sGridNo, 1 );
			break;
		case ACTION_ITEM_LARGE_PIT:
			Add5X5Pit( sGridNo );
			SearchForOtherMembersWithinPitRadiusAndMakeThemFall( sGridNo, 2 );
			break;
		case ACTION_ITEM_TOGGLE_ACTION1:
			ToggleActionItemsByFrequency( FIRST_MAP_PLACED_FREQUENCY + 1 );
			break;
		case ACTION_ITEM_TOGGLE_ACTION2:
			ToggleActionItemsByFrequency( FIRST_MAP_PLACED_FREQUENCY + 2 );
			break;
		case ACTION_ITEM_TOGGLE_ACTION3:
			ToggleActionItemsByFrequency( FIRST_MAP_PLACED_FREQUENCY + 3 );
			break;
		case ACTION_ITEM_TOGGLE_ACTION4:
			ToggleActionItemsByFrequency( FIRST_MAP_PLACED_FREQUENCY + 4 );
			break;
		case ACTION_ITEM_TOGGLE_PRESSURE_ITEMS:
			TogglePressureActionItemsInGridNo( sGridNo );
			break;
		case ACTION_ITEM_ENTER_BROTHEL:
			// JA2Gold: Disable brothel tracking
			/*
			if ( ! (gTacticalStatus.uiFlags & INCOMBAT) )
			{
				UINT8		ubID;

				ubID = WhoIsThere2( sGridNo, 0 );
				if ( (ubID != NOBODY) && (MercPtrs[ ubID ]->bTeam == gbPlayerNum) )
				{
					if ( MercPtrs[ ubID ]->sOldGridNo == sGridNo + DirectionInc( SOUTH ) )
					{
						gMercProfiles[ MADAME ].bNPCData2++;

						SetFactTrue( FACT_PLAYER_USED_BROTHEL );
						SetFactTrue( FACT_PLAYER_PASSED_GOON );

						// If we for any reason trigger Madame's record 34 then we don't bother to do
						// anything else

						// Billy always moves back on a timer so that the player has a chance to sneak
						// someone else through

						// Madame's quote about female mercs should therefore not be made on a timer

						if ( gMercProfiles[ MADAME ].bNPCData2 > 2 )
						{
							// more than 2 entering brothel
							TriggerNPCRecord( MADAME, 35 );
							return;
						}

						if ( gMercProfiles[ MADAME ].bNPCData2 == gMercProfiles[ MADAME ].bNPCData )
						{
							// full # of mercs who paid have entered brothel
							// have Billy block the way again
							SetCustomizableTimerCallbackAndDelay( 2000, BillyBlocksDoorCallback, FALSE );
							//TriggerNPCRecord( BILLY, 6 );
						}
						else if ( gMercProfiles[ MADAME ].bNPCData2 > gMercProfiles[ MADAME ].bNPCData )
						{
							// more than full # of mercs who paid have entered brothel
							// have Billy block the way again?
							if ( CheckFact( FACT_PLAYER_FORCED_WAY_INTO_BROTHEL, 0 ) )
							{
								// player already did this once!
								TriggerNPCRecord( MADAME, 35 );
								return;
							}
							else
							{
								SetCustomizableTimerCallbackAndDelay( 2000, BillyBlocksDoorCallback, FALSE );
								SetFactTrue( FACT_PLAYER_FORCED_WAY_INTO_BROTHEL );
								TriggerNPCRecord( MADAME, 34 );
							}
						}

						if ( gMercProfiles[ MercPtrs[ ubID ]->ubProfile ].bSex == FEMALE )
						{
							// woman walking into brothel
							TriggerNPCRecordImmediately( MADAME, 33 );
						}

					}
					else
					{
						// someone wants to leave the brothel
						TriggerNPCRecord( BILLY, 5 );	
					}

				}

			}
			*/
			break;
		case ACTION_ITEM_EXIT_BROTHEL:
			// JA2Gold: Disable brothel tracking
			/*
			if ( ! (gTacticalStatus.uiFlags & INCOMBAT) )
			{
				UINT8		ubID;

				ubID = WhoIsThere2( sGridNo, 0 );
				if ( (ubID != NOBODY) && (MercPtrs[ ubID ]->bTeam == gbPlayerNum) && MercPtrs[ ubID ]->sOldGridNo == sGridNo + DirectionInc( NORTH ) )
				{
					gMercProfiles[ MADAME ].bNPCData2--;
					if ( gMercProfiles[ MADAME ].bNPCData2 == 0 )
					{
						// reset paid #
						gMercProfiles[ MADAME ].bNPCData = 0;
					}
					// Billy should move back to block the door again
					gsTempActionGridNo = sGridNo;
					SetCustomizableTimerCallbackAndDelay( 1000, DelayedBillyTriggerToBlockOnExit, TRUE );
				}
			}
			*/
			break;
		case ACTION_ITEM_KINGPIN_ALARM:
			PlayJA2Sample( KLAXON_ALARM, RATE_11025, SoundVolume( MIDVOLUME, sGridNo ), 5, SoundDir( sGridNo ) );
			CallAvailableKingpinMenTo( sGridNo );

			gTacticalStatus.fCivGroupHostile[ KINGPIN_CIV_GROUP ] = CIV_GROUP_HOSTILE;

			{
				UINT8		ubID, ubID2;
				BOOLEAN	fEnterCombat = FALSE;

				for ( ubID = gTacticalStatus.Team[ CIV_TEAM ].bFirstID; ubID <= gTacticalStatus.Team[ CIV_TEAM ].bLastID; ubID++ )
				{
					if ( MercPtrs[ ubID ]->bActive && MercPtrs[ ubID ]->bInSector && MercPtrs[ ubID ]->ubCivilianGroup == KINGPIN_CIV_GROUP )
					{
						for ( ubID2 = gTacticalStatus.Team[ gbPlayerNum ].bFirstID; ubID2 <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubID2++ )
						{
							if ( MercPtrs[ ubID ]->bOppList[ ubID2 ] == SEEN_CURRENTLY )
							{
								MakeCivHostile( MercPtrs[ ubID ], 2 );
								fEnterCombat = TRUE;
							}
						}
					}
				}

				if ( ! (gTacticalStatus.uiFlags & INCOMBAT) )
				{
					EnterCombatMode( CIV_TEAM );
				}
			}

			// now zap this object so it won't activate again
			pObj->fFlags &= (~OBJECT_DISABLED_BOMB);
			break;
		case ACTION_ITEM_SEX:
			// JA2Gold: Disable brothel sex
			/*
			if ( ! (gTacticalStatus.uiFlags & INCOMBAT) )
			{
				UINT8		ubID;
				OBJECTTYPE DoorCloser;
				INT16		sTeleportSpot;
				INT16		sDoorSpot;
				UINT8		ubDirection;
				UINT8		ubRoom, ubOldRoom;

				ubID = WhoIsThere2( sGridNo, 0 );
				if ( (ubID != NOBODY) && (MercPtrs[ ubID ]->bTeam == gbPlayerNum) )
				{
					if ( InARoom( sGridNo, &ubRoom ) && InARoom( MercPtrs[ ubID ]->sOldGridNo, &ubOldRoom ) && ubOldRoom != ubRoom )
					{
						// also require there to be a miniskirt civ in the room
						if ( HookerInRoom( ubRoom ) )
						{

							// stop the merc...
							EVENT_StopMerc( MercPtrs[ ubID ], MercPtrs[ ubID ]->sGridNo, MercPtrs[ ubID ]->bDirection );

							switch( sGridNo )
							{
								case 13414:
									sDoorSpot = 13413;
									sTeleportSpot = 13413;
									break;
								case 11174:
									sDoorSpot = 11173;
									sTeleportSpot = 11173;
									break;
								case 12290:
									sDoorSpot = 12290;
									sTeleportSpot = 12291;
									break;

								default:

									sDoorSpot = NOWHERE;
									sTeleportSpot = NOWHERE;


							}

							if ( sDoorSpot != NOWHERE && sTeleportSpot != NOWHERE )
							{
								// close the door... 
								DoorCloser.bActionValue = ACTION_ITEM_CLOSE_DOOR;
								PerformItemAction( sDoorSpot, &DoorCloser );

								// have sex
								HandleNPCDoAction( 0, NPC_ACTION_SEX, 0 );	
								
								// move the merc outside of the room again
								sTeleportSpot = FindGridNoFromSweetSpotWithStructData( MercPtrs[ ubID ], STANDING, sTeleportSpot, 2, &ubDirection, FALSE );
								ChangeSoldierState( MercPtrs[ ubID ], STANDING, 0, TRUE );
								TeleportSoldier( MercPtrs[ ubID ], sTeleportSpot, FALSE );

								HandleMoraleEvent( MercPtrs[ ubID ], MORALE_SEX, gWorldSectorX, gWorldSectorY, gbWorldSectorZ );
								FatigueCharacter( MercPtrs[ ubID ] );
								FatigueCharacter( MercPtrs[ ubID ] );
								FatigueCharacter( MercPtrs[ ubID ] );
								FatigueCharacter( MercPtrs[ ubID ] );
								DirtyMercPanelInterface( MercPtrs[ ubID ], DIRTYLEVEL1 ); 
							}
						}

					}
					break;

				}
			}
			*/
			break;
		case ACTION_ITEM_REVEAL_ROOM:
			{
				UINT8 ubRoom;
				if ( InAHiddenRoom( sGridNo, &ubRoom ) )
				{
					RemoveRoomRoof( sGridNo, ubRoom, NULL );
				}
			}
			break;
		case ACTION_ITEM_LOCAL_ALARM:			
			MakeNoise( NOBODY, sGridNo, 0, gpWorldLevelData[sGridNo].ubTerrainID, 30, NOISE_SILENT_ALARM );
			break;
		case ACTION_ITEM_GLOBAL_ALARM:
			CallAvailableEnemiesTo( sGridNo );
			break;
		case ACTION_ITEM_BLOODCAT_ALARM:
			CallAvailableTeamEnemiesTo( sGridNo, CREATURE_TEAM );
			break;
		case ACTION_ITEM_KLAXON:
			PlayJA2Sample( KLAXON_ALARM, RATE_11025, SoundVolume( MIDVOLUME, sGridNo ), 5, SoundDir( sGridNo ) );
			break;
		case ACTION_ITEM_MUSEUM_ALARM:
			PlayJA2Sample( KLAXON_ALARM, RATE_11025, SoundVolume( MIDVOLUME, sGridNo ), 5, SoundDir( sGridNo ) );
			CallEldinTo( sGridNo );
			break;
		default:
			// error message here
			#ifdef JA2BETAVERSION
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Action item with invalid action in gridno %d!", sGridNo );
			#endif
			break;
	}
}

void AddBombToQueue( UINT32 uiWorldBombIndex, UINT32 uiTimeStamp )
{
	if (gubElementsOnExplosionQueue == MAX_BOMB_QUEUE)
	{
		return;
	}
	
	gExplosionQueue[gubElementsOnExplosionQueue].uiWorldBombIndex = uiWorldBombIndex;
	gExplosionQueue[gubElementsOnExplosionQueue].uiTimeStamp = uiTimeStamp;
	gExplosionQueue[gubElementsOnExplosionQueue].fExists = TRUE;
	if (!gfExplosionQueueActive)
	{
		// lock UI
		guiPendingOverrideEvent = LU_BEGINUILOCK;
		// disable sight
		gTacticalStatus.uiFlags |= DISALLOW_SIGHT;
	}
	gubElementsOnExplosionQueue++;
	gfExplosionQueueActive = TRUE;
}

void HandleExplosionQueue( void )
{
	UINT32				uiIndex;
	UINT32				uiWorldBombIndex;
	UINT32				uiCurrentTime;
	INT16					sGridNo;
	OBJECTTYPE *	pObj;
  UINT8         ubLevel;

	if ( !gfExplosionQueueActive )
	{
		return;
	}

	uiCurrentTime = GetJA2Clock();
	for ( uiIndex = 0; uiIndex < gubElementsOnExplosionQueue; uiIndex++ )
	{
		if ( gExplosionQueue[ uiIndex ].fExists && uiCurrentTime >= gExplosionQueue[ uiIndex ].uiTimeStamp )
		{
			// Set off this bomb now!
	
			// Preliminary assignments:
			uiWorldBombIndex = gExplosionQueue[ uiIndex ].uiWorldBombIndex;
			pObj = &( gWorldItems[ gWorldBombs[ uiWorldBombIndex ].iItemIndex ].o );
			sGridNo = gWorldItems[ gWorldBombs[ uiWorldBombIndex ].iItemIndex ].sGridNo;
			ubLevel = gWorldItems[ gWorldBombs[ uiWorldBombIndex ].iItemIndex ].ubLevel;

			if (pObj->usItem == ACTION_ITEM && pObj->bActionValue != ACTION_ITEM_BLOW_UP)
			{
				PerformItemAction( sGridNo, pObj );
			}
			else if ( pObj->usBombItem == TRIP_KLAXON )
			{
				PlayJA2Sample( KLAXON_ALARM, RATE_11025, SoundVolume( MIDVOLUME, sGridNo ), 5, SoundDir( sGridNo ) );
				CallAvailableEnemiesTo( sGridNo );
				//RemoveItemFromPool( sGridNo, gWorldBombs[ uiWorldBombIndex ].iItemIndex, 0 );
			}
			else if ( pObj->usBombItem == TRIP_FLARE )
			{
				NewLightEffect( sGridNo, LIGHT_FLARE_MARK_1 );
				RemoveItemFromPool( sGridNo, gWorldBombs[ uiWorldBombIndex ].iItemIndex, ubLevel );
			}
			else
			{
				gfExplosionQueueMayHaveChangedSight = TRUE;

				// We have to remove the item first to prevent the explosion from detonating it
				// a second time :-)
				RemoveItemFromPool( sGridNo, gWorldBombs[ uiWorldBombIndex ].iItemIndex, ubLevel );

				// make sure no one thinks there is a bomb here any more!
				if ( gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_PLAYER_MINE_PRESENT )
				{
					RemoveBlueFlag( sGridNo, ubLevel );
				}
				gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_ENEMY_MINE_PRESENT);

				// BOOM!

				// bomb objects only store the SIDE who placed the bomb! :-(
				if ( pObj->ubBombOwner > 1 )
				{
					IgniteExplosion( (UINT8) (pObj->ubBombOwner - 2), CenterX( sGridNo ), CenterY( sGridNo ), 0, sGridNo, pObj->usBombItem, ubLevel );
				}
				else
				{
					// pre-placed
					IgniteExplosion( NOBODY, CenterX( sGridNo ), CenterY( sGridNo ), 0, sGridNo, pObj->usBombItem, ubLevel );
				}
			}

			// Bye bye bomb
			gExplosionQueue[ uiIndex ].fExists = FALSE;
		}
	}

	// See if we can reduce the # of elements on the queue that we have recorded
	// Easier to do it at this time rather than in the loop above
	while ( gubElementsOnExplosionQueue > 0 && gExplosionQueue[ gubElementsOnExplosionQueue - 1 ].fExists == FALSE )
	{
		gubElementsOnExplosionQueue--;
	}

	if ( gubElementsOnExplosionQueue == 0 && (gubPersonToSetOffExplosions == NOBODY || gTacticalStatus.ubAttackBusyCount == 0) )
	{
		// turn off explosion queue 

		// re-enable sight
		gTacticalStatus.uiFlags &= (~DISALLOW_SIGHT);

		if ( gubPersonToSetOffExplosions != NOBODY && !(MercPtrs[ gubPersonToSetOffExplosions ]->uiStatusFlags & SOLDIER_PC) )
		{
			FreeUpNPCFromPendingAction( MercPtrs[ gubPersonToSetOffExplosions ] );
		}

		if (gfExplosionQueueMayHaveChangedSight)
		{
			UINT8 ubLoop;
			SOLDIERTYPE * pTeamSoldier;

			// set variable so we may at least have someone to resolve interrupts vs
			gubInterruptProvoker = gubPersonToSetOffExplosions;
			AllTeamsLookForAll( TRUE );

			// call fov code
			ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
			for ( pTeamSoldier = MercPtrs[ ubLoop ]; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++, pTeamSoldier++ )
			{
				if ( pTeamSoldier->bActive && pTeamSoldier->bInSector )
				{
					RevealRoofsAndItems( pTeamSoldier, TRUE, FALSE, pTeamSoldier->bLevel, FALSE );
				}
			}

			gfExplosionQueueMayHaveChangedSight = FALSE;
			gubPersonToSetOffExplosions = NOBODY;
		}

		// unlock UI
		//UnSetUIBusy( (UINT8)gusSelectedSoldier );
		if ( !(gTacticalStatus.uiFlags & INCOMBAT) || gTacticalStatus.ubCurrentTeam == gbPlayerNum )
		{
			// don't end UI lock when it's a computer turn
			guiPendingOverrideEvent = LU_ENDUILOCK;
		}

		gfExplosionQueueActive = FALSE;
	}

}

void DecayBombTimers( void )
{
	UINT32				uiWorldBombIndex;
	UINT32				uiTimeStamp;
	OBJECTTYPE *	pObj;

	uiTimeStamp = GetJA2Clock();

	// Go through all the bombs in the world, and look for timed ones
	for (uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
	{
		if (gWorldBombs[uiWorldBombIndex].fExists)
		{
			pObj = &( gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].o );
			if ( pObj->bDetonatorType == BOMB_TIMED && !(pObj->fFlags & OBJECT_DISABLED_BOMB) )
			{
				// Found a timed bomb, so decay its delay value and see if it goes off
				pObj->bDelay--;
				if (pObj->bDelay == 0)
				{
					// put this bomb on the queue
					AddBombToQueue( uiWorldBombIndex, uiTimeStamp );
          // ATE: CC black magic....
			    if ( pObj->ubBombOwner > 1 )
          {
            gubPersonToSetOffExplosions = (UINT8) (pObj->ubBombOwner - 2);
          }
          else
          {
            gubPersonToSetOffExplosions = NOBODY;
          }

					if (pObj->usItem != ACTION_ITEM || pObj->bActionValue == ACTION_ITEM_BLOW_UP)
					{
						uiTimeStamp += BOMB_QUEUE_DELAY;
					}
				}
			}
		}
	}
}

void SetOffBombsByFrequency( UINT8 ubID, INT8 bFrequency )
{
	UINT32				uiWorldBombIndex;
	UINT32				uiTimeStamp;
	OBJECTTYPE *	pObj;

	uiTimeStamp = GetJA2Clock();

	// Go through all the bombs in the world, and look for remote ones
	for (uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
	{
		if (gWorldBombs[uiWorldBombIndex].fExists)
		{
			pObj = &( gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].o );
			if ( pObj->bDetonatorType == BOMB_REMOTE && !(pObj->fFlags & OBJECT_DISABLED_BOMB) )
			{
				// Found a remote bomb, so check to see if it has the same frequency
				if (pObj->bFrequency == bFrequency)
				{

					gubPersonToSetOffExplosions = ubID;

					// put this bomb on the queue
					AddBombToQueue( uiWorldBombIndex, uiTimeStamp );
					if (pObj->usItem != ACTION_ITEM || pObj->bActionValue == ACTION_ITEM_BLOW_UP)
					{
						uiTimeStamp += BOMB_QUEUE_DELAY;
					}
				}
			}
		}
	}
}

void SetOffPanicBombs( UINT8 ubID, INT8 bPanicTrigger )
{
	// need to turn off gridnos & flags in gTacticalStatus
	gTacticalStatus.sPanicTriggerGridNo[ bPanicTrigger ] = NOWHERE;
	if ( (gTacticalStatus.sPanicTriggerGridNo[0] == NOWHERE) && 
				(gTacticalStatus.sPanicTriggerGridNo[1] == NOWHERE) && 
				(gTacticalStatus.sPanicTriggerGridNo[2] == NOWHERE) )
	{
		gTacticalStatus.fPanicFlags &= ~(PANIC_TRIGGERS_HERE);
	}

	switch( bPanicTrigger )
	{
		case 0:												
			SetOffBombsByFrequency( ubID, PANIC_FREQUENCY );
			gTacticalStatus.fPanicFlags &= ~(PANIC_BOMBS_HERE);
			break;

		case 1:
			SetOffBombsByFrequency( ubID, PANIC_FREQUENCY_2 );
			break;

		case 2:
			SetOffBombsByFrequency( ubID, PANIC_FREQUENCY_3 );
			break;

		default:
			break;

	}

	if ( gTacticalStatus.fPanicFlags )
	{
		// find a new "closest one"
		MakeClosestEnemyChosenOne();
	}
}

BOOLEAN SetOffBombsInGridNo( UINT8 ubID, INT16 sGridNo, BOOLEAN fAllBombs, INT8 bLevel )
{
	UINT32				uiWorldBombIndex;
	UINT32				uiTimeStamp;
	OBJECTTYPE *	pObj;
	BOOLEAN				fFoundMine = FALSE;

	uiTimeStamp = GetJA2Clock();

	// Go through all the bombs in the world, and look for mines at this location
	for (uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
	{
		if (gWorldBombs[uiWorldBombIndex].fExists && gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].sGridNo == sGridNo && gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].ubLevel == bLevel )
		{
			pObj = &( gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].o );
			if (!(pObj->fFlags & OBJECT_DISABLED_BOMB))
			{
				if (fAllBombs || pObj->bDetonatorType == BOMB_PRESSURE)
				{
					if (!fAllBombs && MercPtrs[ ubID ]->bTeam != gbPlayerNum )
					{
						// ignore this unless it is a mine, etc which would have to have been placed by the
						// player, seeing as how the others are all marked as known to the AI.
						if ( !(pObj->usItem == MINE || pObj->usItem == TRIP_FLARE || pObj->usItem == TRIP_KLAXON ) )
						{
							continue;
						}
					}

					// player and militia ignore bombs set by player
					if ( pObj->ubBombOwner > 1 && (MercPtrs[ ubID ]->bTeam == gbPlayerNum || MercPtrs[ ubID ]->bTeam == MILITIA_TEAM) )
					{
						continue;
					
					}

					if (pObj->usItem == SWITCH)
					{
						// send out a signal to detonate other bombs, rather than this which
						// isn't a bomb but a trigger
						SetOffBombsByFrequency( ubID, pObj->bFrequency );
					}
					else
					{
						gubPersonToSetOffExplosions = ubID;

						// put this bomb on the queue
						AddBombToQueue( uiWorldBombIndex, uiTimeStamp );
						if (pObj->usItem != ACTION_ITEM || pObj->bActionValue == ACTION_ITEM_BLOW_UP)
						{
							uiTimeStamp += BOMB_QUEUE_DELAY;
						}

						if ( pObj->usBombItem != NOTHING && Item[ pObj->usBombItem ].usItemClass & IC_EXPLOSV )
						{
							fFoundMine = TRUE;
						}

					}
				}
			}
		}
	}
	return( fFoundMine );
}

void ActivateSwitchInGridNo( UINT8 ubID, INT16 sGridNo )
{
	UINT32				uiWorldBombIndex;
	OBJECTTYPE *	pObj;

	// Go through all the bombs in the world, and look for mines at this location
	for (uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
	{
		if (gWorldBombs[uiWorldBombIndex].fExists && gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].sGridNo == sGridNo)
		{
			pObj = &( gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].o );

			if ( pObj->usItem == SWITCH && ( !(pObj->fFlags & OBJECT_DISABLED_BOMB) ) && pObj->bDetonatorType == BOMB_SWITCH)
			{
				// send out a signal to detonate other bombs, rather than this which
				// isn't a bomb but a trigger

				// first set attack busy count to 0 in case of a lingering a.b.c. problem...
				gTacticalStatus.ubAttackBusyCount = 0;

				SetOffBombsByFrequency( ubID, pObj->bFrequency );
			}
		}
	}
}

BOOLEAN SaveExplosionTableToSaveGameFile( HWFILE hFile )
{
	UINT32	uiNumBytesWritten;
	UINT32	uiExplosionCount=0;
	UINT32	uiCnt;


	//
	//	Explosion queue Info
	//


	//Write the number of explosion queues
	FileWrite( hFile, &gubElementsOnExplosionQueue, sizeof( UINT32 ), &uiNumBytesWritten );
	if( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		FileClose( hFile );
		return( FALSE );
	}


	//loop through and add all the explosions
	for( uiCnt=0; uiCnt< MAX_BOMB_QUEUE; uiCnt++)
	{
		FileWrite( hFile, &gExplosionQueue[ uiCnt ], sizeof( ExplosionQueueElement ), &uiNumBytesWritten );
		if( uiNumBytesWritten != sizeof( ExplosionQueueElement ) )
		{
			FileClose( hFile );
			return( FALSE );
		}
	}


	//
	//	Explosion Data
	//

	//loop through and count all the active explosions
	uiExplosionCount = 0;
	for( uiCnt=0; uiCnt< NUM_EXPLOSION_SLOTS; uiCnt++)
	{
		if( gExplosionData[ uiCnt ].fAllocated )
		{
			uiExplosionCount++;
		}
	}

	//Save the number of explosions
	FileWrite( hFile, &uiExplosionCount, sizeof( UINT32 ), &uiNumBytesWritten );
	if( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		FileClose( hFile );
		return( FALSE );
	}



	//loop through and count all the active explosions
	for( uiCnt=0; uiCnt< NUM_EXPLOSION_SLOTS; uiCnt++)
	{
		if( gExplosionData[ uiCnt ].fAllocated )
		{			
			FileWrite( hFile, &gExplosionData[ uiCnt ], sizeof( EXPLOSIONTYPE ), &uiNumBytesWritten );
			if( uiNumBytesWritten != sizeof( EXPLOSIONTYPE ) )
			{
				FileClose( hFile );
				return( FALSE );
			}
		}
	}




	return( TRUE );
}



BOOLEAN LoadExplosionTableFromSavedGameFile( HWFILE hFile )
{
	UINT32	uiNumBytesRead;
	UINT32	uiExplosionCount=0;
	UINT32	uiCnt;


	//
	//	Explosion Queue
	//

	//Clear the Explosion queue
	memset( gExplosionQueue, 0, sizeof( ExplosionQueueElement ) * MAX_BOMB_QUEUE );

	//Read the number of explosions queue's
	FileRead( hFile, &gubElementsOnExplosionQueue, sizeof( UINT32 ), &uiNumBytesRead );
	if( uiNumBytesRead != sizeof( UINT32 ) )
	{
		return( FALSE );
	}


	//loop through read all the active explosions fro the file
	for( uiCnt=0; uiCnt<MAX_BOMB_QUEUE; uiCnt++)
	{
		FileRead( hFile, &gExplosionQueue[ uiCnt ], sizeof( ExplosionQueueElement ), &uiNumBytesRead );
		if( uiNumBytesRead != sizeof( ExplosionQueueElement ) )
		{
			return( FALSE );
		}
	}



	//
	//	Explosion Data
	//
	
	//Load the number of explosions
	FileRead( hFile, &guiNumExplosions, sizeof( UINT32 ), &uiNumBytesRead );
	if( uiNumBytesRead != sizeof( UINT32 ) )
	{
		return( FALSE );
	}


	//loop through and load all the active explosions
	for( uiCnt=0; uiCnt< guiNumExplosions; uiCnt++)
	{
		FileRead( hFile, &gExplosionData[ uiCnt ], sizeof( EXPLOSIONTYPE ), &uiNumBytesRead );
		if( uiNumBytesRead != sizeof( EXPLOSIONTYPE ) )
		{
			return( FALSE );
		}
		gExplosionData[ uiCnt ].iID = uiCnt;
		gExplosionData[ uiCnt ].iLightID = -1;

		GenerateExplosionFromExplosionPointer( &gExplosionData[ uiCnt ] );
	}

	return( TRUE );
}




BOOLEAN DoesSAMExistHere( INT16 sSectorX, INT16 sSectorY, INT16 sSectorZ, INT16 sGridNo )
{
	INT32 cnt;
	INT16	sSectorNo;

	// ATE: If we are belwo, return right away...
	if ( sSectorZ != 0 )
	{
		return( FALSE );
	}

	sSectorNo = SECTOR( sSectorX, sSectorY );

	for ( cnt = 0; cnt < NUMBER_OF_SAMS; cnt++ )
	{
		// Are we i nthe same sector...
		if ( pSamList[ cnt ] == sSectorNo )
		{
			// Are we in the same gridno?
			if ( pSamGridNoAList[ cnt ] == sGridNo || pSamGridNoBList[ cnt ] == sGridNo )
			{
				return( TRUE );
			}
		}
	}

	return( FALSE );
}


void UpdateAndDamageSAMIfFound( INT16 sSectorX, INT16 sSectorY, INT16 sSectorZ, INT16 sGridNo, UINT8 ubDamage )
{
	INT16 sSectorNo;

	// OK, First check if SAM exists, and if not, return
	if ( !DoesSAMExistHere( sSectorX, sSectorY, sSectorZ, sGridNo ) )
	{
		return;
	}

	// Damage.....
	sSectorNo = CALCULATE_STRATEGIC_INDEX( sSectorX, sSectorY );

	if ( StrategicMap[ sSectorNo ].bSAMCondition >= ubDamage )
	{
		StrategicMap[ sSectorNo ].bSAMCondition -= ubDamage;
	}
	else
	{
		StrategicMap[ sSectorNo ].bSAMCondition = 0;
	}

	// SAM site may have been put out of commission...
	UpdateAirspaceControl( );

	// ATE: GRAPHICS UPDATE WILL GET DONE VIA NORMAL EXPLOSION CODE.....
}


void UpdateSAMDoneRepair( INT16 sSectorX, INT16 sSectorY, INT16 sSectorZ  )
{
	INT32 cnt;
	INT16	sSectorNo;
	BOOLEAN		fInSector = FALSE;
	UINT16		usGoodGraphic, usDamagedGraphic;

	// ATE: If we are below, return right away...
	if ( sSectorZ != 0 )
	{
		return;
	}

	if ( sSectorX == gWorldSectorX && sSectorY == gWorldSectorY && sSectorZ == gbWorldSectorZ )
	{
		fInSector = TRUE;
	}


	sSectorNo = SECTOR( sSectorX, sSectorY );

	for ( cnt = 0; cnt < NUMBER_OF_SAMS; cnt++ )
	{
		// Are we i nthe same sector...
		if ( pSamList[ cnt ] == sSectorNo )
		{
			// get graphic.......
			GetTileIndexFromTypeSubIndex( EIGHTISTRUCT, (UINT16)( gbSAMGraphicList[ cnt ] ), &usGoodGraphic );					

			// Damaged one ( current ) is 2 less...
			usDamagedGraphic = usGoodGraphic - 2;

			// First gridno listed is base gridno....

			// if this is loaded....
			if ( fInSector )
			{
				// Update graphic.....
				// Remove old!
				ApplyMapChangesToMapTempFile( TRUE );

				RemoveStruct( pSamGridNoAList[ cnt ], usDamagedGraphic );
				AddStructToHead( pSamGridNoAList[ cnt ], usGoodGraphic );

				ApplyMapChangesToMapTempFile( FALSE );
			}
			else
			{
				// We add temp changes to map not loaded....
				// Remove old
				RemoveStructFromUnLoadedMapTempFile( pSamGridNoAList[ cnt ], usDamagedGraphic, sSectorX, sSectorY, (UINT8)sSectorZ );
				// Add new
				AddStructToUnLoadedMapTempFile( pSamGridNoAList[ cnt ], usGoodGraphic, sSectorX, sSectorY, (UINT8)sSectorZ );
			}
		}
	}

	// SAM site may have been put back into working order...
	UpdateAirspaceControl( );
}


// loop through civ team and find
// anybody who is an NPC and
// see if they get angry
void HandleBuldingDestruction( INT16 sGridNo, UINT8 ubOwner )
{
	SOLDIERTYPE *		pSoldier;
	UINT8						cnt;

  if ( ubOwner == NOBODY )
  {
    return;
  }

  if ( MercPtrs[ ubOwner ]->bTeam != gbPlayerNum )
  {
    return;
  }

	cnt = gTacticalStatus.Team[ CIV_TEAM ].bFirstID;
  for ( pSoldier = MercPtrs[ cnt ]; cnt <= gTacticalStatus.Team[ CIV_TEAM ].bLastID; cnt++ ,pSoldier++ )
	{
		if ( pSoldier->bActive && pSoldier->bInSector && pSoldier->bLife && pSoldier->bNeutral )
		{
      if ( pSoldier->ubProfile != NO_PROFILE )
      {
				// ignore if the player is fighting the enemy here and this is a good guy
				if ( gTacticalStatus.Team[ ENEMY_TEAM ].bMenInSector > 0 && (gMercProfiles[ pSoldier->ubProfile ].ubMiscFlags3 & PROFILE_MISC_FLAG3_GOODGUY) )
				{
					continue;
				}

        if ( DoesNPCOwnBuilding( pSoldier, sGridNo ) )
        {
					MakeNPCGrumpyForMinorOffense( pSoldier, MercPtrs[ ubOwner ] );
        }
      }
		}
	}
}

INT32 FindActiveTimedBomb( void )
{
	UINT32				uiWorldBombIndex;
	UINT32				uiTimeStamp;
	OBJECTTYPE *	pObj;

	uiTimeStamp = GetJA2Clock();

	// Go through all the bombs in the world, and look for timed ones
	for (uiWorldBombIndex = 0; uiWorldBombIndex < guiNumWorldBombs; uiWorldBombIndex++)
	{
		if (gWorldBombs[uiWorldBombIndex].fExists)
		{
			pObj = &( gWorldItems[ gWorldBombs[uiWorldBombIndex].iItemIndex ].o );
			if ( pObj->bDetonatorType == BOMB_TIMED && !(pObj->fFlags & OBJECT_DISABLED_BOMB) )
			{
				return( gWorldBombs[uiWorldBombIndex].iItemIndex );		
			}
		}
	}

	return( -1 );
}

BOOLEAN ActiveTimedBombExists( void )
{
	if ( gfWorldLoaded )
	{
		return( FindActiveTimedBomb() != -1 );
	}
	else
	{
		return( FALSE );
	}
}

void RemoveAllActiveTimedBombs( void )
{
	INT32	iItemIndex;

	do
	{
		iItemIndex = FindActiveTimedBomb();
		if (iItemIndex != -1 )
		{
			RemoveItemFromWorld( iItemIndex );
		}
	} while( iItemIndex != -1 );
	
}