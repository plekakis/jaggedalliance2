#ifdef PRECOMPILEDHEADERS
	#include "TileEngine All.h"
#else
	#include <stdio.h>
	#include "FileMan.h"
	#include "debug.h"
	#include "types.h"
	#include "worlddef.h"
	#include "worldman.h"
	#include "smooth.h"
	#include "Exit Grids.h"
	#include "Editor Undo.h"
	#include "StrategicMap.h"
	#include "Strategic Movement.h"
	#include "message.h"
	#include "Font Control.h"
	#include "pathai.h"
	#include "overhead.h"
	#include "Animation Control.h"
	#include "Sys Globals.h"
	#include "quests.h"
	#include "SaveLoadMap.h"
	#include "Text.h"
#endif

BOOLEAN gfLoadingExitGrids = FALSE;

//used by editor.
EXITGRID		gExitGrid	= {0,1,1,0};

BOOLEAN gfOverrideInsertionWithExitGrid = FALSE;

INT32 ConvertExitGridToINT32( EXITGRID *pExitGrid )
{
	INT32 iExitGridInfo;
	iExitGridInfo  = (pExitGrid->ubGotoSectorX-1)<< 28;
	iExitGridInfo += (pExitGrid->ubGotoSectorY-1)<< 24;
	iExitGridInfo += pExitGrid->ubGotoSectorZ    << 20;
	iExitGridInfo += pExitGrid->usGridNo & 0x0000ffff;
	return iExitGridInfo;
}

void ConvertINT32ToExitGrid( INT32 iExitGridInfo, EXITGRID *pExitGrid )
{
	//convert the int into 4 unsigned bytes.
	pExitGrid->ubGotoSectorX		= (UINT8)(((iExitGridInfo & 0xf0000000)>>28)+1);
	pExitGrid->ubGotoSectorY		= (UINT8)(((iExitGridInfo & 0x0f000000)>>24)+1);
	pExitGrid->ubGotoSectorZ		= (UINT8)((iExitGridInfo & 0x00f00000)>>20);
	pExitGrid->usGridNo					= (UINT16)(iExitGridInfo & 0x0000ffff);
}

BOOLEAN	GetExitGrid( UINT16 usMapIndex, EXITGRID *pExitGrid )
{
	LEVELNODE *pShadow;
	pShadow = gpWorldLevelData[ usMapIndex ].pShadowHead;
	//Search through object layer for an exitgrid
	while( pShadow )
	{
		if ( pShadow->uiFlags & LEVELNODE_EXITGRID )
		{
			ConvertINT32ToExitGrid( pShadow->iExitGridInfo, pExitGrid );
			return TRUE;
		}
		pShadow = pShadow->pNext;
	}
	pExitGrid->ubGotoSectorX = 0;
	pExitGrid->ubGotoSectorY = 0;
	pExitGrid->ubGotoSectorZ = 0;
	pExitGrid->usGridNo = 0;
	return FALSE;	
}

BOOLEAN	ExitGridAtGridNo( UINT16 usMapIndex )
{
	LEVELNODE *pShadow;
	pShadow = gpWorldLevelData[ usMapIndex ].pShadowHead;
	//Search through object layer for an exitgrid
	while( pShadow )
	{
		if ( pShadow->uiFlags & LEVELNODE_EXITGRID )
		{
			return TRUE;
		}
		pShadow = pShadow->pNext;
	}
	return FALSE;	
}

BOOLEAN	GetExitGridLevelNode( UINT16 usMapIndex, LEVELNODE **ppLevelNode )
{
	LEVELNODE *pShadow;
	pShadow = gpWorldLevelData[ usMapIndex ].pShadowHead;
	//Search through object layer for an exitgrid
	while( pShadow )
	{
		if ( pShadow->uiFlags & LEVELNODE_EXITGRID )
		{
			*ppLevelNode = pShadow;
			return TRUE;
		}
		pShadow = pShadow->pNext;
	}
	return FALSE;	
}


void AddExitGridToWorld( INT32 iMapIndex, EXITGRID *pExitGrid )
{
	LEVELNODE *pShadow, *tail;
	pShadow = gpWorldLevelData[ iMapIndex ].pShadowHead;

	//Search through object layer for an exitgrid
	while( pShadow )
	{
		tail = pShadow;
		if( pShadow->uiFlags & LEVELNODE_EXITGRID )
		{ //we have found an existing exitgrid in this node, so replace it with the new information.
			pShadow->iExitGridInfo = ConvertExitGridToINT32( pExitGrid );
			//SmoothExitGridRadius( (UINT16)iMapIndex, 0 );
			return;
		}
		pShadow = pShadow->pNext;
	}

	// Add levelnode
	AddShadowToHead( iMapIndex, MOCKFLOOR1 );
	// Get new node
	pShadow = gpWorldLevelData[ iMapIndex ].pShadowHead;

	//fill in the information for the new exitgrid levelnode.
	pShadow->iExitGridInfo = ConvertExitGridToINT32( pExitGrid );
	pShadow->uiFlags |= ( LEVELNODE_EXITGRID | LEVELNODE_HIDDEN );

	//Add the exit grid to the sector, only if we call ApplyMapChangesToMapTempFile() first.
	if( !gfEditMode && !gfLoadingExitGrids )
	{
		AddExitGridToMapTempFile( (UINT16)iMapIndex, pExitGrid, gWorldSectorX, gWorldSectorY, gbWorldSectorZ );
	}
}

void RemoveExitGridFromWorld( INT32 iMapIndex )
{
	UINT16 usDummy;
	if( TypeExistsInShadowLayer( iMapIndex, MOCKFLOOR, &usDummy ) )
	{
		RemoveAllShadowsOfTypeRange( iMapIndex, MOCKFLOOR, MOCKFLOOR );
	}
}

void SaveExitGrids( HWFILE fp, UINT16 usNumExitGrids )
{
	EXITGRID exitGrid;
	UINT16 usNumSaved = 0;
	UINT16 x;
	UINT32	uiBytesWritten;
	FileWrite( fp, &usNumExitGrids, 2, &uiBytesWritten );
	for( x = 0; x < WORLD_MAX; x++ )
	{
		if( GetExitGrid( x, &exitGrid ) )
		{
			FileWrite( fp, &x, 2, &uiBytesWritten );
			FileWrite( fp, &exitGrid, 5, &uiBytesWritten );
			usNumSaved++;
		}
	}
	//If these numbers aren't equal, something is wrong!
	Assert( usNumExitGrids == usNumSaved );
}

void LoadExitGrids( INT8 **hBuffer )
{
	EXITGRID exitGrid;
	UINT16 x;
	UINT16 usNumSaved;
	UINT16 usMapIndex;
	gfLoadingExitGrids = TRUE;
	LOADDATA( &usNumSaved, *hBuffer, 2 );
	//FileRead( hfile, &usNumSaved, 2, NULL);
	for( x = 0; x < usNumSaved; x++ )
	{
		LOADDATA( &usMapIndex, *hBuffer, 2 );
		//FileRead( hfile, &usMapIndex, 2, NULL);
		LOADDATA( &exitGrid, *hBuffer, 5 );
		//FileRead( hfile, &exitGrid, 5, NULL);
		AddExitGridToWorld( usMapIndex, &exitGrid );
	}
	gfLoadingExitGrids = FALSE;
}

void AttemptToChangeFloorLevel( INT8 bRelativeZLevel )
{
	UINT8 ubLookForLevel=0;
	UINT16 i;
	if( bRelativeZLevel != 1 && bRelativeZLevel != -1 )
		return;
	//Check if on ground level -- if so, can't go up!
	if( bRelativeZLevel == -1 && !gbWorldSectorZ )
	{
		ScreenMsg( FONT_DKYELLOW, MSG_INTERFACE, pMessageStrings[ MSG_CANT_GO_UP ], ubLookForLevel );
		return;
	}
	//Check if on bottom level -- if so, can't go down!
	if( bRelativeZLevel == 1 && gbWorldSectorZ == 3 )
	{

		ScreenMsg( FONT_DKYELLOW, MSG_INTERFACE, pMessageStrings[ MSG_CANT_GO_DOWN ], ubLookForLevel );
		return;
	}
	ubLookForLevel = (UINT8)(gbWorldSectorZ + bRelativeZLevel);
	for( i = 0; i < WORLD_MAX; i++ )
	{
		if( GetExitGrid( i, &gExitGrid ) )
		{
			if( gExitGrid.ubGotoSectorZ == ubLookForLevel )
			{ //found an exit grid leading to the goal sector!
				gfOverrideInsertionWithExitGrid = TRUE;
				//change all current mercs in the loaded sector, and move them
				//to the new sector.
				MoveAllGroupsInCurrentSectorToSector( (UINT8)gWorldSectorX, (UINT8)gWorldSectorY, ubLookForLevel );
				if( ubLookForLevel )
					ScreenMsg( FONT_YELLOW, MSG_INTERFACE, pMessageStrings[ MSG_ENTERING_LEVEL ], ubLookForLevel );
				else
					ScreenMsg( FONT_YELLOW, MSG_INTERFACE, pMessageStrings[ MSG_LEAVING_BASEMENT ] );

				SetCurrentWorldSector( gWorldSectorX, gWorldSectorY, ubLookForLevel );
				gfOverrideInsertionWithExitGrid = FALSE;
			}
		}
	}
}


UINT16 FindGridNoFromSweetSpotCloseToExitGrid( SOLDIERTYPE *pSoldier, INT16 sSweetGridNo, INT8 ubRadius, UINT8 *pubDirection )
{
	INT16  sTop, sBottom;
	INT16  sLeft, sRight;
	INT16  cnt1, cnt2;
	INT16		sGridNo;
	INT32		uiRange, uiLowestRange = 999999;
	INT16		sLowestGridNo=0;
	INT32					leftmost;
	BOOLEAN	fFound = FALSE;
	SOLDIERTYPE soldier;
	UINT8 ubSaveNPCAPBudget;
	UINT8 ubSaveNPCDistLimit;
	EXITGRID	ExitGrid;
	UINT8	ubGotoSectorX, ubGotoSectorY, ubGotoSectorZ;

	// Turn off at end of function...
	gfPlotPathToExitGrid = TRUE;

	//Save AI pathing vars.  changing the distlimit restricts how 
	//far away the pathing will consider.
	ubSaveNPCAPBudget = gubNPCAPBudget;
	ubSaveNPCDistLimit = gubNPCDistLimit;
	gubNPCAPBudget = 0;
	gubNPCDistLimit = ubRadius;

	//create dummy soldier, and use the pathing to determine which nearby slots are
	//reachable.
	memset( &soldier, 0, sizeof( SOLDIERTYPE ) );
	soldier.bLevel = 0;
	soldier.bTeam = 1;
	soldier.sGridNo = pSoldier->sGridNo;

	// OK, Get an exit grid ( if possible )
  if ( !GetExitGrid( sSweetGridNo, &ExitGrid ) )
	{
		return( NOWHERE );
	}

	// Copy our dest values.....
	ubGotoSectorX = ExitGrid.ubGotoSectorX;
	ubGotoSectorY = ExitGrid.ubGotoSectorY;
	ubGotoSectorZ = ExitGrid.ubGotoSectorZ;


	sTop		= ubRadius;
	sBottom = -ubRadius;
	sLeft   = - ubRadius;
	sRight  = ubRadius;

	//clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
	//in the square region.
	for( cnt1 = sBottom; cnt1 <= sTop; cnt1++ )
	{
		for( cnt2 = sLeft; cnt2 <= sRight; cnt2++ )
		{
			sGridNo = pSoldier->sGridNo + (WORLD_COLS * cnt1) + cnt2;
			if ( sGridNo >= 0 && sGridNo < WORLD_MAX )
			{
				gpWorldLevelData[ sGridNo ].uiFlags &= (~MAPELEMENT_REACHABLE);
			}
		}
	}

	//Now, find out which of these gridnos are reachable 
	//(use the fake soldier and the pathing settings)
	FindBestPath( &soldier, NOWHERE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE );
	
	uiLowestRange = 999999;	

	for( cnt1 = sBottom; cnt1 <= sTop; cnt1++ )
	{
		leftmost = ( ( pSoldier->sGridNo + ( WORLD_COLS * cnt1 ) )/ WORLD_COLS ) * WORLD_COLS;

		for( cnt2 = sLeft; cnt2 <= sRight; cnt2++ )
		{
			sGridNo = pSoldier->sGridNo + ( WORLD_COLS * cnt1 ) + cnt2;
			if ( sGridNo >=0 && sGridNo < WORLD_MAX && sGridNo >= leftmost && sGridNo < ( leftmost + WORLD_COLS ) && 
				gpWorldLevelData[ sGridNo ].uiFlags & MAPELEMENT_REACHABLE )
			{
				// Go on sweet stop
				// ATE: Added this check because for all intensive purposes, cavewalls will be not an OKDEST
				// but we want thenm too...
				if ( NewOKDestination( pSoldier, sGridNo, TRUE, pSoldier->bLevel ) )
				{
					if ( GetExitGrid( sGridNo, &ExitGrid ) )
					{
						// Is it the same exitgrid?
						if ( ExitGrid.ubGotoSectorX == ubGotoSectorX && ExitGrid.ubGotoSectorY == ubGotoSectorY && ExitGrid.ubGotoSectorZ == ubGotoSectorZ )
						{
							uiRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

							if ( uiRange < uiLowestRange )
							{
								sLowestGridNo = sGridNo;
								uiLowestRange = uiRange;
								fFound = TRUE;
							}
						}
					}
				}
			}
		}
	}
	gubNPCAPBudget = ubSaveNPCAPBudget;
	gubNPCDistLimit = ubSaveNPCDistLimit;

	gfPlotPathToExitGrid = FALSE;

	if ( fFound )
	{
		// Set direction to center of map!
		*pubDirection =  (UINT8)GetDirectionToGridNoFromGridNo( sLowestGridNo, ( ( ( WORLD_ROWS / 2 ) * WORLD_COLS ) + ( WORLD_COLS / 2 ) ) );
		return( sLowestGridNo );
	}
	else
	{
		return( NOWHERE );
	}
}


UINT16 FindClosestExitGrid( SOLDIERTYPE *pSoldier, INT16 sSrcGridNo, INT8 ubRadius )
{
	INT16  sTop, sBottom;
	INT16  sLeft, sRight;
	INT16  cnt1, cnt2;
	INT16		sGridNo;
	INT32		uiRange, uiLowestRange = 999999;
	INT16		sLowestGridNo=0;
	INT32					leftmost;
	BOOLEAN	fFound = FALSE;
	EXITGRID	ExitGrid;


	sTop		= ubRadius;
	sBottom = -ubRadius;
	sLeft   = - ubRadius;
	sRight  = ubRadius;

	//clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
	uiLowestRange = 999999;	

	for( cnt1 = sBottom; cnt1 <= sTop; cnt1++ )
	{
		leftmost = ( ( sSrcGridNo + ( WORLD_COLS * cnt1 ) )/ WORLD_COLS ) * WORLD_COLS;

		for( cnt2 = sLeft; cnt2 <= sRight; cnt2++ )
		{
			sGridNo = sSrcGridNo + ( WORLD_COLS * cnt1 ) + cnt2;
			if( sGridNo >=0 && sGridNo < WORLD_MAX && sGridNo >= leftmost && sGridNo < ( leftmost + WORLD_COLS ) )
			{
				if ( GetExitGrid( sGridNo, &ExitGrid ) )
				{
					uiRange = GetRangeInCellCoordsFromGridNoDiff( sSrcGridNo, sGridNo );

					if ( uiRange < uiLowestRange )
					{
						sLowestGridNo = sGridNo;
						uiLowestRange = uiRange;
						fFound = TRUE;
					}
				}
			}
		}
	}

	if ( fFound )
	{
		return( sLowestGridNo );
	}
	else
	{
		return( NOWHERE );
	}
}


