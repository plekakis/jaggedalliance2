#ifndef __WORLDMAN_H_
#define __WORLDMAN_H_

#include "Types.h"

// memory-accounting function
void CountLevelNodes( void );


// Object manipulation functions
BOOLEAN RemoveObject( UINT32 iMapIndex, UINT16 usIndex );
LEVELNODE *AddObjectToTail( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN AddObjectToHead( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN TypeExistsInObjectLayer( UINT32 iMapIndex, UINT32 fType, UINT16 *pusObjectIndex );
BOOLEAN RemoveAllObjectsOfTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType );
void SetAllObjectShadeLevels( UINT32 iMapIndex, UINT8 ubShadeLevel );
void AdjustAllObjectShadeLevels( UINT32 iMapIndex, INT8 bShadeDiff );
BOOLEAN TypeRangeExistsInObjectLayer( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT16 *pusObjectIndex );


// Roof manipulation functions
BOOLEAN RemoveRoof( UINT32 iMapIndex, UINT16 usIndex );
LEVELNODE  *AddRoofToTail( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN AddRoofToHead( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN TypeExistsInRoofLayer( UINT32 iMapIndex, UINT32 fType, UINT16 *pusRoofIndex );
BOOLEAN RemoveAllRoofsOfTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType );
void SetAllRoofShadeLevels( UINT32 iMapIndex, UINT8 ubShadeLevel );
void AdjustAllRoofShadeLevels( UINT32 iMapIndex, INT8 bShadeDiff );
void RemoveRoofIndexFlagsFromTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT32 uiFlags  );
void SetRoofIndexFlagsFromTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT32 uiFlags  );
BOOLEAN TypeRangeExistsInRoofLayer( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT16 *pusRoofIndex );
void SetWallLevelnodeFlags( UINT16 sGridNo, UINT32 uiFlags );
void RemoveWallLevelnodeFlags( UINT16 sGridNo, UINT32 uiFlags );
BOOLEAN IndexExistsInRoofLayer( INT16 sGridNo, UINT16 usIndex );


// OnRoof manipulation functions
BOOLEAN RemoveOnRoof( UINT32 iMapIndex, UINT16 usIndex );
LEVELNODE *AddOnRoofToTail( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN AddOnRoofToHead( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN TypeExistsInOnRoofLayer( UINT32 iMapIndex, UINT32 fType, UINT16 *pusOnRoofIndex );
BOOLEAN RemoveAllOnRoofsOfTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType );
void SetAllOnRoofShadeLevels( UINT32 iMapIndex, UINT8 ubShadeLevel );
void AdjustAllOnRoofShadeLevels( UINT32 iMapIndex, INT8 bShadeDiff );
BOOLEAN RemoveOnRoofFromLevelNode( UINT32 iMapIndex, LEVELNODE *pNode );


// Land manipulation functions
BOOLEAN RemoveLand( UINT32 iMapIndex, UINT16 usIndex );
LEVELNODE *AddLandToTail( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN AddLandToHead( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN TypeExistsInLandLayer( UINT32 iMapIndex, UINT32 fType, UINT16 *pusLandIndex );
BOOLEAN RemoveAllLandsOfTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType );
BOOLEAN TypeRangeExistsInLandLayer( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT16 *pusLandIndex );
BOOLEAN TypeRangeExistsInLandHead( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT16 *pusLandIndex );
BOOLEAN ReplaceLandIndex( UINT32 iMapIndex, UINT16 usOldIndex, UINT16 usNewIndex );
BOOLEAN DeleteAllLandLayers( UINT32 iMapIndex );
BOOLEAN InsertLandIndexAtLevel( UINT32 iMapIndex, UINT16 usIndex, UINT8 ubLevel );
BOOLEAN RemoveHigherLandLevels( UINT32 iMapIndex, UINT32 fSrcType, UINT32 **puiHigherTypes, UINT8 *pubNumHigherTypes );
BOOLEAN SetLowerLandLevels( UINT32 iMapIndex, UINT32 fSrcType, UINT16 usIndex  );
BOOLEAN AdjustForFullTile( UINT32 iMapIndex );
void SetAllLandShadeLevels( UINT32 iMapIndex, UINT8 ubShadeLevel );
void AdjustAllLandShadeLevels( UINT32 iMapIndex, INT8 bShadeDiff );
void AdjustAllLandDirtyCount( UINT32 iMapIndex, INT8 bDirtyDiff );
BOOLEAN Water( INT16 sGridNo );
BOOLEAN DeepWater( INT16 sGridNo );
BOOLEAN WaterTooDeepForAttacks( INT16 sGridNo );

// Structure manipulation routines
BOOLEAN RemoveStruct( UINT32 iMapIndex, UINT16 usIndex );
LEVELNODE *AddStructToTail( UINT32 iMapIndex, UINT16 usIndex );
LEVELNODE *AddStructToTailCommon( UINT32 iMapIndex, UINT16 usIndex, BOOLEAN fAddStructDBInfo );
LEVELNODE *ForceStructToTail( UINT32 iMapIndex, UINT16 usIndex );

BOOLEAN AddStructToHead( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN TypeExistsInStructLayer( UINT32 iMapIndex, UINT32 fType, UINT16 *pusStructIndex );
BOOLEAN RemoveAllStructsOfTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType );
BOOLEAN AddWallToStructLayer( INT32 iMapIndex, UINT16 usIndex, BOOLEAN fReplace );
BOOLEAN ReplaceStructIndex( UINT32 iMapIndex, UINT16 usOldIndex, UINT16 usNewIndex );
BOOLEAN HideStructOfGivenType( UINT32 iMapIndex, UINT32 fType, BOOLEAN fHide );
BOOLEAN InsertStructIndex( UINT32 iMapIndex, UINT16 usIndex, UINT8 ubLevel );
void SetAllStructShadeLevels( UINT32 iMapIndex, UINT8 ubShadeLevel );
void AdjustAllStructShadeLevels( UINT32 iMapIndex, INT8 bShadeDiff );
void SetStructIndexFlagsFromTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT32 uiFlags  );
void RemoveStructIndexFlagsFromTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT32 uiFlags  );
void SetStructAframeFlags( UINT32 iMapIndex, UINT32 uiFlags  );
void RemoveStructAframeFlags( UINT32 iMapIndex, UINT32 uiFlags  );
BOOLEAN RemoveStructFromLevelNode( UINT32 iMapIndex, LEVELNODE *pNode );


BOOLEAN RemoveStructFromTail( UINT32 iMapIndex );
BOOLEAN RemoveStructFromTailCommon( UINT32 iMapIndex, BOOLEAN fRemoveStructDBInfo );
BOOLEAN ForceRemoveStructFromTail( UINT32 iMapIndex );

BOOLEAN TypeRangeExistsInStructLayer( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType, UINT16 *pusStructIndex );


// Shadow manipulation routines
BOOLEAN RemoveShadow( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN AddShadowToTail( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN AddShadowToHead( UINT32 iMapIndex, UINT16 usIndex );
void AddExclusiveShadow( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN TypeExistsInShadowLayer( UINT32 iMapIndex, UINT32 fType, UINT16 *pusShadowIndex );
BOOLEAN RemoveAllShadowsOfTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType );
BOOLEAN	RemoveAllShadows( UINT32 iMapIndex );
BOOLEAN RemoveShadowFromLevelNode( UINT32 iMapIndex, LEVELNODE *pNode );


// Merc manipulation routines
// #################################################################

BOOLEAN AddMercToHead( UINT32 iMapIndex, SOLDIERTYPE *pSoldier, BOOLEAN fAddStructInfo );
BOOLEAN RemoveMerc( UINT32 iMapIndex, SOLDIERTYPE *pSoldier, BOOLEAN fPlaceHolder  );
UINT8 WhoIsThere2( INT16 sGridNo, INT8 bLevel );
BOOLEAN AddMercStructureInfo( INT16 sGridNo, SOLDIERTYPE *pSoldier );
BOOLEAN AddMercStructureInfoFromAnimSurface( INT16 sGridNo, SOLDIERTYPE *pSoldier, UINT16 usAnimSurface, UINT16 usAnimState );
BOOLEAN UpdateMercStructureInfo( SOLDIERTYPE *pSoldier );
BOOLEAN OKToAddMercToWorld( SOLDIERTYPE *pSoldier, INT8 bDirection );
 

// TOPMOST manipulation functions
LEVELNODE  *AddTopmostToTail( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN AddTopmostToHead( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN RemoveTopmost( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN TypeExistsInTopmostLayer( UINT32 iMapIndex, UINT32 fType, UINT16 *pusTopmostIndex );
BOOLEAN RemoveAllTopmostsOfTypeRange( UINT32 iMapIndex, UINT32 fStartType, UINT32 fEndType );
BOOLEAN SetMapElementShadeLevel( UINT32 uiMapIndex, UINT8 ubShadeLevel );
void SetTopmostFlags( UINT32 iMapIndex, UINT32 uiFlags, UINT16 usIndex );
void RemoveTopmostFlags( UINT32 iMapIndex, UINT32 uiFlags, UINT16 usIndex );
BOOLEAN AddUIElem( UINT32 iMapIndex, UINT16 usIndex, INT8	sRelativeX, INT8 sRelativeY, LEVELNODE **ppNewNode );
void RemoveUIElem( UINT32 iMapIndex, UINT16 usIndex );
BOOLEAN RemoveTopmostFromLevelNode( UINT32 iMapIndex, LEVELNODE *pNode );


BOOLEAN IsLowerLevel( INT16 sGridNo );
BOOLEAN IsHeigherLevel( INT16 sGridNo );
BOOLEAN IsRoofVisible( INT16 sMapPos );
BOOLEAN IsRoofVisible2( INT16 sMapPos );


LEVELNODE * FindLevelNodeBasedOnStructure( INT16 sGridNo, STRUCTURE * pStructure );
LEVELNODE * FindShadow( INT16 sGridNo, UINT16 usStructIndex );

void WorldHideTrees( );
void WorldShowTrees( );


BOOLEAN IsTileRedundent( UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );

//this is found in editscreen.c
//Andrew, you had worldman.c checked out at the time, so I stuck it here.
//The best thing to do is toast it here, and include editscreen.h in worldman.c.
extern UINT32	gCurrentBackground;

void		SetTreeTopStateForMap();

#endif