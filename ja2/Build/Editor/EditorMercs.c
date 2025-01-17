#ifdef PRECOMPILEDHEADERS
	#include "Editor All.h"
#else
	#include "builddefines.h"
#endif

#ifdef JA2EDITOR

#ifndef PRECOMPILEDHEADERS
	#include <windows.h>
	#include "tiledef.h"
	#include "edit_sys.h"
	#include "vsurface.h"
	#include "vobject.h"
	#include "mousesystem.h"
	#include "Button System.h"
	#include "wcheck.h"
	#include "vsurface.h"
	#include "line.h"
	#include "input.h"
	#include "sysutil.h"
	#include "font.h"
	#include "Font Control.h"
	#include "editscreen.h"
	#include "selectwin.h"
	#include "video.h"
	#include "vobject_blitters.h"
	#include "interface panels.h"
	#include "interface items.h"
	#include "text.h"
	#include "utilities.h"
	#include "World Items.h"
	#include "worldman.h"
	#include "overhead.h"	//GetSoldier
	#include "renderworld.h"
	#include "Animation Data.h"
	#include "Animation Control.h"
	#include "EditorDefines.h"
	#include "EditorMercs.h"
	#include "EditorTerrain.h" //for access to TerrainTileDrawMode
	#include "Soldier Create.h" //The stuff that connects the editor generated information
															//to the saved map and the game itself.
	#include "Inventory Choosing.h"
	#include "Soldier Init List.h"
	#include "strategicmap.h"
	#include "Soldier Add.h"
	#include "Soldier Control.h"
	#include "Soldier Profile Type.h"
	#include "Soldier Profile.h"
	#include "Text Input.h"
	#include "Random.h"
	#include "wordwrap.h"
	#include "EditorItems.h"
	#include "Editor Taskbar Utils.h"
	#include "Exit Grids.h"
	#include "Editor Undo.h"
	#include "Item Statistics.h"
	#include "Simple Render Utils.h"
	#include "Map Information.h"
	#include "Isometric Utils.h"
	#include "Render Dirty.h"
	#include "popupmenu.h"
	#include "Scheduling.h"
	#include "Timer Control.h"
	#include "message.h"
#endif

extern void GetSoldierAboveGuyPositions( SOLDIERTYPE *pSoldier, INT16 *psX, INT16 *psY, BOOLEAN fRadio );

//--------------------------------------------------
//	NON_CIV_GROUP,
//	REBEL_CIV_GROUP,
//	KINGPIN_CIV_GROUP,
//	SANMONA_ARMS_GROUP,
//	ANGELS_GROUP,
//	NUM_CIV_GROUPS
UINT16 gszCivGroupNames[ NUM_CIV_GROUPS ][ 20 ] = 
{
	L"NONE",
	L"REBEL",
	L"KINGPIN",
	L"SANMONA ARMS",
	L"ANGELS",

	L"BEGGARS",
	L"TOURISTS",
	L"ALMA MIL",
	L"DOCTORS",
	L"COUPLE1",

	L"HICKS",
	L"WARDEN",
	L"JUNKYARD",
	L"FACTORY KIDS",
	L"QUEENS",
	L"UNUSED15",
	L"UNUSED16",
	L"UNUSED17",
	L"UNUSED18",
	L"UNUSED19",
};

//--------------------------------------------------

//	SCHEDULE_ACTION_NONE,
//	SCHEDULE_ACTION_LOCKDOOR,
//	SCHEDULE_ACTION_UNLOCKDOOR,
//	SCHEDULE_ACTION_OPENDOOR,
//	SCHEDULE_ACTION_CLOSEDOOR,
//	SCHEDULE_ACTION_GRIDNO,
//	SCHEDULE_ACTION_LEAVESECTOR,
//	SCHEDULE_ACTION_ENTERSECTOR,
//	SCHEDULE_ACTION_STAYINSECTOR,
//  SCHEDULE_ACTION_SLEEP,
UINT16 gszScheduleActions[ NUM_SCHEDULE_ACTIONS ][20] = 
{
	L"No action",
	L"Lock door",
	L"Unlock door",
	L"Open door",
	L"Close door",
	L"Move to gridno",
	L"Leave sector",
	L"Enter sector",
	L"Stay in sector",
	L"Sleep",
	L"Ignore this!"
};


enum
{
	SCHEDULE_INSTRUCTIONS_NONE,
	SCHEDULE_INSTRUCTIONS_DOOR1,
	SCHEDULE_INSTRUCTIONS_DOOR2,
	SCHEDULE_INSTRUCTIONS_GRIDNO,
	SCHEDULE_INSTRUCTIONS_SLEEP,
	NUM_SCHEDULE_INSTRUCTIONS
};

extern BOOLEAN gfSingleAction = FALSE;
BOOLEAN gfUseScheduleData2 = FALSE;
UINT8 gubCurrentScheduleActionIndex = 0;
SCHEDULENODE gCurrSchedule;
UINT8 gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_NONE;
void DetermineScheduleEditability();
void RenderCurrentSchedule();
void UpdateScheduleInfo();

void ShowEditMercPalettes( SOLDIERTYPE *pSoldier );
void ShowEditMercColorSet( UINT8 ubPaletteRep, INT16 sSet );

void ChangeBaseSoldierStats( SOLDIERTYPE *pSoldier );
void AskDefaultDifficulty( void );

//internal merc inventory functions
void AddNewItemToSelectedMercsInventory( BOOLEAN fCreate );
void RenderMercInventoryPanel();
void SetDroppableCheckboxesBasedOnMercsInventory();

extern BOOLEAN InternalAddSoldierToSector( UINT8 ubID, BOOLEAN fCalculateDirection, BOOLEAN fUseAnimation, UINT16 usAnimState, UINT16 usAnimCode );

//array which keeps track of which item is in which slot.  This is dependant on the selected merc, so
//these temp values must be updated when different mercs are selected, and reset when a merc detailed 
//placement is created.  
OBJECTTYPE *gpMercSlotItem[9] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
//Because we only support these nine slots, they aren't continuous values, so this array helps
//processing functions by referring to this array to get the appropriate slot.
INT8 gbMercSlotTypes[9] = { HELMETPOS, VESTPOS, LEGPOS, HANDPOS, SECONDHANDPOS, 
														BIGPOCK1POS, BIGPOCK2POS, BIGPOCK3POS, BIGPOCK4POS };
//returns the usItem index of specified slot in the currently selected merc.
#define GetSelectedMercSlotItemIndex( x )  (gpSelected->pDetailedPlacement->Inv[gbMercSlotTypes[x]].usItem)
#define GetSelectedMercSlot( x )  (&gpSelected->pDetailedPlacement->Inv[gbMercSlotTypes[x]])
//values indicating which merc inventory slot is hilited and which slot is selected.
INT8 gbCurrHilite = -1;
INT8 gbCurrSelect = -1;
//when a new merc is selected, this function sets up all of the information for the slots, 
//selections, and hilites.
void UpdateMercItemSlots();

//internal merc utility functions
void SetupTextInputForMercAttributes();
void SetupTextInputForMercProfile();
void SetupTextInputForMercSchedule();
void ExtractAndUpdateMercAttributes();
void ExtractAndUpdateMercProfile();
void ExtractAndUpdateMercSchedule();
void CalcStringForValue( UINT16 *str, INT32 iValue, UINT32 uiMax );
void ChangeBodyType( INT8 bOffset );  //+1 or -1 only

//internal merc variables
BASIC_SOLDIERCREATE_STRUCT gTempBasicPlacement;
SOLDIERCREATE_STRUCT gTempDetailedPlacement;

INT16						gsSelectedMercID;
INT16						gsSelectedMercGridNo;
SOLDIERINITNODE *gpSelected;

UINT8						gubCurrMercMode									= MERC_TEAMMODE;
UINT8						gubPrevMercMode									= MERC_NOMODE;
UINT8						gubLastDetailedMercMode					= MERC_GENERALMODE;
INT8						gbDefaultOrders									= STATIONARY;
INT8						gbDefaultAttitude								= DEFENSIVE;
INT8						gbDefaultRelativeEquipmentLevel = 2;
INT8						gbDefaultRelativeAttributeLevel = 2;
INT8						gbDefaultDirection							= NORTHWEST;
INT8						gubSoldierClass							= SOLDIER_CLASS_ARMY;
UINT8						gubCivGroup											= NON_CIV_GROUP;

SOLDIERTYPE			*pTempSoldier;
BOOLEAN					gfRoofPlacement;

//Below are all flags that have to do with editing detailed placement mercs:

//Determines if the user is allowed to edit merc colors.  User must specifically
//click on the checkbox by the colors to confirm that colors will be specified.  If
//not, the colors will be randomly generated upon creation in the game.
BOOLEAN					gfCanEditMercColors							= FALSE;
//A rendering flag that is set whenever a full update of the merc editing information
//needs to be done.
BOOLEAN					gfRenderMercInfo								= FALSE;
//When the user specifies a profile index for the merc, all editing is disabled.  This is
//because the profile contains all of the information.  When this happens, all of the editing
//buttons are disabled, but you are allowed to view stats, inventory, etc., as well as specify
//orders, attitude, and direction.
BOOLEAN					gfCanEditMercs									= TRUE;
//When in inventory mode, this flag is set when the user wishes to get an item, which requires hooking
//into the item editing features.  This is processed during the editor update, which in turn, sets the
//new mode.
BOOLEAN					gfMercGetItem										= FALSE;
//As soon as an item is selected, the items index is stored here, so the item can be copied into the
//slot for editing and rendering purposes.  This is a temp store value only when leaving the editor items
//mode.
UINT16 gusMercsNewItemIndex												= 0xffff;

//old and probably obsolete
//BOOLEAN	fMercEdUseLeftSide = FALSE;
//BOOLEAN fEditingMerc = FALSE;
//BOOLEAN fKeepWindowHidden = FALSE;
INT32 iEditMercPage = 1;
INT32 iEditMercEnd = -1;
INT32 iEditMercBkgrndArea = -1;
INT32 iEditMercLocation;
INT32 iEditStatTimer = 0;
INT32 iEditWhichStat = -1;
INT32 iEditMercMode = EDIT_MERC_NONE;
INT32 iEditMercColorPage = -1;
INT32 iEditMercStatPage = -1;
INT32 iEditMercFindButton = -1;
INT32 iEditMercSlotNumber;
INT32	iEditColorStart[EDIT_NUM_COLORS];

BOOLEAN gfShowPlayers = TRUE;
BOOLEAN gfShowEnemies = TRUE;
BOOLEAN gfShowCreatures = TRUE;
BOOLEAN gfShowRebels = TRUE;
BOOLEAN gfShowCivilians = TRUE;

#define BASE_STAT_DEVIATION			7
#define BASE_EXPLVL_DEVIATION		1
#define BASE_PROTLVL_DEVIATION	0
#define BASE_GUNTYPE_DEVIATION	4
#define DEFAULT_DIFF						2

INT16 sCurBaseDiff = DEFAULT_DIFF;
BOOLEAN fAskForBaseDifficulty = TRUE;
UINT16 *zDiffNames[NUM_DIFF_LVLS] = { L"Wimp", L"Easy", L"Average", L"Tough", L"Steroid Users Only" };
INT16 sBaseStat[NUM_DIFF_LVLS] = { 50, 60, 70, 80, 90 };
INT16 sBaseExpLvl[NUM_DIFF_LVLS] = { 1, 3, 5, 7, 9 };

UINT16 *EditMercStat[12] = { L"Max Health",L"Cur Health",L"Strength",
														 L"Agility",L"Dexterity",L"Charisma",
														 L"Wisdom",L"Marksmanship",L"Explosives",
														 L"Medical",L"Scientific",L"Exp Level" };

#define NUM_MERC_ORDERS 8
UINT16 *EditMercOrders[8] = { L"Stationary",L"On Guard",L"Close Patrol",
															L"Far Patrol",L"Point Patrol",L"On Call",
															L"Seek Enemy", L"Random Point Patrol"};

UINT16 *EditMercAttitudes[6] = { L"Defensive",L"Brave Loner",L"Brave Buddy",
																	L"Cunning Loner",L"Cunning Buddy",
																	L"Aggressive" };

//information for bodytypes.
#ifdef RANDOM
  #undef RANDOM
#endif
#define RANDOM	-1
#define MAX_ENEMYTYPES				7
//#define MAX_ENEMYRANDOMTYPES	5
#define MAX_CREATURETYPES			8
#define MAX_REBELTYPES				7
#define MAX_CIVTYPES					18
//#define MAX_CIVRANDOMTYPES		11
INT8 bEnemyArray[MAX_ENEMYTYPES]={ RANDOM, REGMALE, BIGMALE, STOCKYMALE, REGFEMALE, TANK_NW, TANK_NE };
INT8 bCreatureArray[MAX_CREATURETYPES]={ BLOODCAT, LARVAE_MONSTER, INFANT_MONSTER, YAF_MONSTER, YAM_MONSTER, ADULTFEMALEMONSTER, AM_MONSTER, QUEENMONSTER };
INT8 bRebelArray[MAX_REBELTYPES]={ RANDOM, FATCIV, MANCIV, REGMALE, BIGMALE, STOCKYMALE, REGFEMALE };
INT8 bCivArray[MAX_CIVTYPES]={ RANDOM, FATCIV, MANCIV, MINICIV, DRESSCIV, HATKIDCIV, KIDCIV, REGMALE, BIGMALE, STOCKYMALE, REGFEMALE, 
															 HUMVEE, ELDORADO, ICECREAMTRUCK, JEEP, CRIPPLECIV, ROBOTNOWEAPON, COW };
INT8 gbCurrCreature = BLOODCAT;



BOOLEAN gfSaveBuffer = FALSE;
BASIC_SOLDIERCREATE_STRUCT gSaveBufferBasicPlacement;
SOLDIERCREATE_STRUCT gSaveBufferDetailedPlacement;
void CopyMercPlacement();
void PasteMercPlacement();

void GameInitEditorMercsInfo()
{
	INT32 i;
	//Initialize the placement list
	InitSoldierInitList();
	gMapInformation.ubNumIndividuals = 0;
	memset( &gCurrSchedule, 0, sizeof( SCHEDULENODE ) );
	for( i = 0; i < 4; i++ )
	{
		gCurrSchedule.usTime[i] = 0xffff;
		gCurrSchedule.usData1[i] = 0xffff;
		gCurrSchedule.usData2[i] = 0xffff;
	}
}

void GameShutdownEditorMercsInfo()
{
	UseEditorAlternateList();
	KillSoldierInitList();
	UseEditorOriginalList();
	KillSoldierInitList();
}

void EntryInitEditorMercsInfo()
{
	INT32 x, iCurStart = 0;
	iEditColorStart[0] = 0;
	for ( x = 1; x < EDIT_NUM_COLORS; x++ )
	{
		iCurStart += gubpNumReplacementsPerRange[ x - 1 ];
		iEditColorStart[x] = iCurStart;
	}
	gsSelectedMercID = -1; 
	gsSelectedMercGridNo = 0;
	
	gfCanEditMercs = TRUE;
}

enum
{
	HAIR_PAL,
	SKIN_PAL,
	VEST_PAL,
	PANTS_PAL
};

void ProcessMercEditing()
{
	UINT8 ubType, ubPaletteRep;
	SOLDIERTYPE *pSoldier;
	if ( iEditMercMode == EDIT_MERC_NONE )
	{
		return;
	}
	GetSoldier( &pSoldier, gsSelectedMercID );

	switch ( iEditMercMode )
	{
		case EDIT_MERC_PREV_COLOR:
		case EDIT_MERC_NEXT_COLOR:
			// Handle changes to the merc colors
			switch ( iEditWhichStat )
			{
				case 0:
					ubType = EDIT_COLOR_HEAD;
					GetPaletteRepIndexFromID( pSoldier->HeadPal, &ubPaletteRep );

					ubPaletteRep--;
					if ( (ubPaletteRep < (UINT8)iEditColorStart[ubType]) || (ubPaletteRep > ((UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType])) )
						ubPaletteRep = (UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType] - 1;

					SET_PALETTEREP_ID( pSoldier->HeadPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->HeadPal, pSoldier->HeadPal );
					CreateSoldierPalettes( pSoldier );
					break;
				case 1:
					ubType = EDIT_COLOR_HEAD;
					GetPaletteRepIndexFromID( pSoldier->HeadPal, &ubPaletteRep );

					ubPaletteRep++;
					if ( ubPaletteRep >= ((UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType]) )
						ubPaletteRep = (UINT8)iEditColorStart[ubType];

					SET_PALETTEREP_ID( pSoldier->HeadPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->HeadPal, pSoldier->HeadPal );
					CreateSoldierPalettes( pSoldier );
					break;

				case 2:
					ubType = EDIT_COLOR_SKIN;
					GetPaletteRepIndexFromID( pSoldier->SkinPal, &ubPaletteRep );

					ubPaletteRep--;
					if ( ubPaletteRep < (UINT8)iEditColorStart[ubType] )
						ubPaletteRep = (UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType] - 1;

					SET_PALETTEREP_ID( pSoldier->SkinPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->SkinPal, pSoldier->SkinPal );
					CreateSoldierPalettes( pSoldier );
					break;
				case 3:
					ubType = EDIT_COLOR_SKIN;
					GetPaletteRepIndexFromID( pSoldier->SkinPal, &ubPaletteRep );

					ubPaletteRep++;
					if ( ubPaletteRep >= ((UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType]) )
						ubPaletteRep = (UINT8)iEditColorStart[ubType];

					SET_PALETTEREP_ID( pSoldier->SkinPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->SkinPal, pSoldier->SkinPal );
					CreateSoldierPalettes( pSoldier );
					break;

				case 4:
					ubType = EDIT_COLOR_VEST;
					GetPaletteRepIndexFromID( pSoldier->VestPal, &ubPaletteRep );

					ubPaletteRep--;
					if ( ubPaletteRep < (UINT8)iEditColorStart[ubType] )
						ubPaletteRep = (UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType] - 1;

					SET_PALETTEREP_ID( pSoldier->VestPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->VestPal, pSoldier->VestPal );
					CreateSoldierPalettes( pSoldier );
					break;
				case 5:
					ubType = EDIT_COLOR_VEST;
					GetPaletteRepIndexFromID( pSoldier->VestPal, &ubPaletteRep );

					ubPaletteRep++;
					if ( ubPaletteRep >= ((UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType]) )
						ubPaletteRep = (UINT8)iEditColorStart[ubType];

					SET_PALETTEREP_ID( pSoldier->VestPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->VestPal, pSoldier->VestPal );
					CreateSoldierPalettes( pSoldier );
					break;

				case 6:
					ubType = EDIT_COLOR_PANTS;
					GetPaletteRepIndexFromID( pSoldier->PantsPal, &ubPaletteRep );

					ubPaletteRep--;
					if ( ubPaletteRep < (UINT8)iEditColorStart[ubType] )
						ubPaletteRep = (UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType] - 1;

					SET_PALETTEREP_ID( pSoldier->PantsPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->PantsPal, pSoldier->PantsPal );
					CreateSoldierPalettes( pSoldier );
					break;
				case 7:
					ubType = EDIT_COLOR_PANTS;
					GetPaletteRepIndexFromID( pSoldier->PantsPal, &ubPaletteRep );

					ubPaletteRep++;
					if ( ubPaletteRep >= ((UINT8)iEditColorStart[ubType] + gubpNumReplacementsPerRange[ubType]) )
						ubPaletteRep = (UINT8)iEditColorStart[ubType];

					SET_PALETTEREP_ID( pSoldier->PantsPal, gpPalRep[ ubPaletteRep ].ID );
					sprintf( gpSelected->pDetailedPlacement->PantsPal, pSoldier->PantsPal );
					CreateSoldierPalettes( pSoldier );
					break;
			}
			iEditMercMode = EDIT_MERC_NONE;
			break;
	}
}

void AddMercToWorld( INT32 iMapIndex )
{
	SOLDIERTYPE *pSoldier;
	INT32 i;

	memset( &gTempBasicPlacement, 0, sizeof( BASIC_SOLDIERCREATE_STRUCT ) );

	gTempBasicPlacement.bBodyType = -1;

	//calculate specific information based on the team.
	switch( iDrawMode )
	{
		case DRAW_MODE_ENEMY:
			gTempBasicPlacement.bTeam = ENEMY_TEAM;
			gTempBasicPlacement.bBodyType = RANDOM;
			gTempBasicPlacement.ubSoldierClass = gubSoldierClass;
			break;
		case DRAW_MODE_CREATURE:
			gTempBasicPlacement.bTeam = CREATURE_TEAM;
			gTempBasicPlacement.bBodyType = gbCurrCreature;
			break;
		case DRAW_MODE_REBEL:
			gTempBasicPlacement.bTeam = MILITIA_TEAM; 
			gTempBasicPlacement.bBodyType = RANDOM;
			break;  
		case DRAW_MODE_CIVILIAN:
			gTempBasicPlacement.bTeam = CIV_TEAM;
			gTempBasicPlacement.bBodyType = RANDOM;
			gTempBasicPlacement.ubCivilianGroup = gubCivGroup;
			if( giCurrentTilesetID == 1 ) //caves
			{
				gTempBasicPlacement.ubSoldierClass = SOLDIER_CLASS_MINER;
			}
			break;
	}

	if( IsLocationSittable( iMapIndex, gfRoofPlacement ) )
	{
		UINT8	ubID;
		INT16 sSectorX, sSectorY;
		SOLDIERINITNODE *pNode;
		
 		GetCurrentWorldSector( &sSectorX, &sSectorY );

		//Set up some general information.
		gTempBasicPlacement.fDetailedPlacement = FALSE;
		gTempBasicPlacement.fPriorityExistance = FALSE;
		gTempBasicPlacement.usStartingGridNo = (UINT16)iMapIndex;
		gTempBasicPlacement.bOrders = gbDefaultOrders;	
		gTempBasicPlacement.bAttitude = gbDefaultAttitude;
		gTempBasicPlacement.bRelativeAttributeLevel = gbDefaultRelativeAttributeLevel; 
		gTempBasicPlacement.bRelativeEquipmentLevel = gbDefaultRelativeEquipmentLevel; 
		gTempBasicPlacement.bDirection = gbDefaultDirection;

		//Generate detailed placement information given the temp placement information.
		CreateDetailedPlacementGivenBasicPlacementInfo( &gTempDetailedPlacement, &gTempBasicPlacement );

		//Set the sector information -- probably unnecessary.
		gTempDetailedPlacement.sSectorX = sSectorX;
		gTempDetailedPlacement.sSectorY = sSectorY;

		//Create the soldier, but don't place it yet.
		if ( pSoldier = TacticalCreateSoldier( &gTempDetailedPlacement, &ubID ) )
		{
			pSoldier->bVisible = 1;
			pSoldier->bLastRenderVisibleValue = 1;
			//Set up the soldier in the list, so we can track the soldier in the 
			//future (saving, loading, strategic AI)
			pNode = AddBasicPlacementToSoldierInitList( &gTempBasicPlacement );
			Assert( pNode );
			pNode->pSoldier = pSoldier;
			
			//Add the soldier to physically appear on the map now.
			InternalAddSoldierToSector( ubID, FALSE, FALSE, 0, 0 );
			IndicateSelectedMerc( ubID );

			//Move him to the roof if intended and possible.
			if( gfRoofPlacement && FlatRoofAboveGridNo( iMapIndex ) )
			{
				gpSelected->pBasicPlacement->fOnRoof = TRUE;
				if( gpSelected->pDetailedPlacement )
					gpSelected->pDetailedPlacement->fOnRoof = TRUE;
				SetSoldierHeight( gpSelected->pSoldier, 58.0 );
			}
			UnclickEditorButtons( FIRST_MERCS_INVENTORY_BUTTON, LAST_MERCS_INVENTORY_BUTTON );
			for( i = FIRST_MERCS_INVENTORY_BUTTON; i <= LAST_MERCS_INVENTORY_BUTTON; i++ )
			{
				SetEnemyDroppableStatus( gbMercSlotTypes[i-FIRST_MERCS_INVENTORY_BUTTON], FALSE );
			}
		}
	}
}

void HandleRightClickOnMerc( INT32 iMapIndex )
{
	SOLDIERINITNODE *pNode;
	INT16 sThisMercID;
	INT16 sCellX, sCellY;

	ConvertGridNoToCellXY( (INT16)iMapIndex, &sCellX, &sCellY );

	sThisMercID = (INT16)IsMercHere( iMapIndex );

	if ( sThisMercID != -1) 
	{ 
		if ( gsSelectedMercID != sThisMercID )
		{ // We want to edit a new merc (or different merc)
			//We need to avoid the editing of player mercs.
			pNode = FindSoldierInitNodeWithID( (UINT8)sThisMercID );
			if( !pNode )
				return;		//this is a player merc (which isn't in the list), or an error in logic.
			IndicateSelectedMerc( sThisMercID );
		}
	}
	else if( gsSelectedMercID != -1 && IsLocationSittable( iMapIndex, gfRoofPlacement ) )// We want to move the selected merc to this new location.
	{							
		RemoveAllObjectsOfTypeRange( gsSelectedMercGridNo, CONFIRMMOVE, CONFIRMMOVE );
		EVENT_SetSoldierPosition( gpSelected->pSoldier, (FLOAT)(sCellX + 5), (FLOAT)(sCellY + 5) );
		if( gfRoofPlacement && FlatRoofAboveGridNo( iMapIndex ) )
		{
			gpSelected->pBasicPlacement->fOnRoof = TRUE;
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->fOnRoof = TRUE;
			SetSoldierHeight( gpSelected->pSoldier, 58.0 );
		}
		else
		{
			gpSelected->pBasicPlacement->fOnRoof = FALSE;
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->fOnRoof = FALSE;
			SetSoldierHeight( gpSelected->pSoldier, 0.0 );
		}
		gsSelectedMercGridNo = (INT16)iMapIndex;
		gpSelected->pBasicPlacement->usStartingGridNo = gsSelectedMercGridNo;
		if( gpSelected->pDetailedPlacement )
			gpSelected->pDetailedPlacement->sInsertionGridNo = gsSelectedMercGridNo;
		AddObjectToHead( gsSelectedMercGridNo, CONFIRMMOVE1 );
	}
}

void ResetAllMercPositions()
{
	SOLDIERINITNODE *curr;
	//Remove all of the alternate placements (editor takes precedence)
	UseEditorAlternateList();
	curr = gSoldierInitHead;
	while( curr )
	{
		gpSelected = curr;
		curr = curr->next;
		RemoveSoldierNodeFromInitList( gpSelected );
	}
	//Now, remove any existing original list mercs, then readd them.
	UseEditorOriginalList();
	curr = gSoldierInitHead;
	while( curr )
	{
		if( curr->pSoldier )
		{
			TacticalRemoveSoldier( curr->pSoldier->ubID );
			curr->pSoldier = NULL;
		}
		//usMapIndex = gpSelected->pBasicPlacement->usStartingGridNo;
		//ConvertGridNoToCellXY( usMapIndex, &sCellX, &sCellY );
		//if( gpSelected->pSoldier )
		//{
		//	EVENT_SetSoldierPosition( gpSelected->pSoldier, (FLOAT)(sCellX + 5), (FLOAT)(sCellY + 5) );
		//	if( gpSelected->pBasicPlacement->fOnRoof )
		//		SetSoldierHeight( gpSelected->pSoldier, 58.0 );
		//	SetMercDirection( gpSelected->pBasicPlacement->bDirection );
		//}
		curr = curr->next;
	}
	AddSoldierInitListTeamToWorld( ENEMY_TEAM,		255 );
	AddSoldierInitListTeamToWorld( CREATURE_TEAM, 255 );
	AddSoldierInitListTeamToWorld( MILITIA_TEAM,		255 );
	AddSoldierInitListTeamToWorld( CIV_TEAM,			255 );
	gpSelected = NULL;
	gsSelectedMercID = -1;
}

void AddMercWaypoint( UINT32 iMapIndex )
{
	INT32 iNum;
	// index 0 isn't used
	if ( iActionParam == 0 )
		return;

	if ( gsSelectedMercID == -1 || (gsSelectedMercID <= (INT32)gTacticalStatus.Team[ OUR_TEAM ].bLastID) || gsSelectedMercID >= MAXMERCS )
		return;
	
	if ( iActionParam > gpSelected->pSoldier->bPatrolCnt )
	{
		// Fill up missing waypoints with same value as new one
		for(iNum = gpSelected->pSoldier->bPatrolCnt + 1; iNum <= iActionParam; iNum++)
		{
			gpSelected->pBasicPlacement->sPatrolGrid[iNum] = (INT16)iMapIndex;
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->sPatrolGrid[iNum] = (INT16)iMapIndex;
			gpSelected->pSoldier->usPatrolGrid[iNum] = (UINT16)iMapIndex;
		}
		
		gpSelected->pBasicPlacement->bPatrolCnt = (INT8)iActionParam;
		if( gpSelected->pDetailedPlacement )
			gpSelected->pDetailedPlacement->bPatrolCnt = (INT8)iActionParam;
		gpSelected->pSoldier->bPatrolCnt = (INT8) iActionParam;
		gpSelected->pSoldier->bNextPatrolPnt = 1;	
	}
	else
	{
		// Set this way point
		gpSelected->pBasicPlacement->sPatrolGrid[iActionParam] = (INT16)iMapIndex;
		if( gpSelected->pDetailedPlacement )
			gpSelected->pDetailedPlacement->sPatrolGrid[iActionParam] = (INT16)iMapIndex;
		gpSelected->pSoldier->usPatrolGrid[iActionParam] = (UINT16)iMapIndex;
	}
	gfRenderWorld = TRUE;
}

void EraseMercWaypoint()
{
	INT32 iNum;
	// index 0 isn't used
	if ( iActionParam == 0 )
		return;

	if ( gsSelectedMercID == -1 || (gsSelectedMercID <= (INT32)gTacticalStatus.Team[ OUR_TEAM ].bLastID) || gsSelectedMercID >= MAXMERCS )
		return;

	// Fill up missing areas
	if ( iActionParam > gpSelected->pSoldier->bPatrolCnt )
		return;

	for(iNum = iActionParam; iNum < gpSelected->pSoldier->bPatrolCnt; iNum++)
	{
		gpSelected->pBasicPlacement->sPatrolGrid[iNum] = gpSelected->pBasicPlacement->sPatrolGrid[iNum+1];
		if( gpSelected->pDetailedPlacement )
			gpSelected->pDetailedPlacement->sPatrolGrid[iNum] = gpSelected->pDetailedPlacement->sPatrolGrid[iNum+1];
		gpSelected->pSoldier->usPatrolGrid[iNum] = gpSelected->pSoldier->usPatrolGrid[iNum+1];
	}

	gpSelected->pBasicPlacement->bPatrolCnt--;
	if( gpSelected->pDetailedPlacement )
		gpSelected->pDetailedPlacement->bPatrolCnt--;
	gpSelected->pSoldier->bPatrolCnt--;
	gfRenderWorld = TRUE;
}


//----------------------------------------------------------------------------------------------
//	ChangeBaseSoldierStats
//
//	This functions changes the stats of a given merc (PC or NPC, though should only be used 
//	for NPC mercs) to reflect the base difficulty level selected.
//
void ChangeBaseSoldierStats( SOLDIERTYPE *pSoldier )
{
	if ( pSoldier == NULL )
		return;

	pSoldier->bLifeMax = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bLife	= pSoldier->bLifeMax;

	pSoldier->bBleeding	= 0;
	pSoldier->bBreath	= 100;

	pSoldier->bMarksmanship	= (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bMedical = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bMechanical = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bExplosive = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bAgility = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bDexterity = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));

	pSoldier->bStrength = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bLeadership = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bWisdom = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));
	pSoldier->bScientific = (UINT8)(sBaseStat[sCurBaseDiff] + (UINT16)(Random(BASE_STAT_DEVIATION * 2)-BASE_STAT_DEVIATION));

	pSoldier->bExpLevel = (UINT8)sBaseExpLvl[sCurBaseDiff];
	pSoldier->bGunType = (INT8)Random(BASE_GUNTYPE_DEVIATION);

	pSoldier->bActionPoints = CalcActionPoints( pSoldier );
}


//----------------------------------------------------------------------------------------------
//	DisplayEditMercWindow
//
//	Displays the edit merc stat page while editing mercs. If the merc color editing page is 
//	to be displayed, this function will dispatch it instead.
//
void DisplayEditMercWindow( void )
{
	INT32 iXPos, iYPos, iHeight, iWidth;
	UINT16 usFillColorBack, usFillColorDark, usFillColorLight, usFillColorTextBk;
	INT32 x, iXOff;
	INT16 TempString[30];
	SOLDIERTYPE *pSoldier;
	INT8 iEditStat[12];

	usFillColorBack = 0;

	if ( gsSelectedMercID == -1 )
	{
//		fEditingMerc = FALSE;
//		DestroyEditMercWindow();
		return;
	}


	GetSoldier( &pSoldier, (INT16)gsSelectedMercID );

//	usFillColorBack = GenericButtonFillColors[0];
	usFillColorDark = Get16BPPColor(FROMRGB(24, 61, 81));
	usFillColorLight = Get16BPPColor(FROMRGB(136, 138, 135));
	usFillColorTextBk = Get16BPPColor(FROMRGB(250, 240, 188));

	iWidth = 266;
	iHeight = 360;
	iYPos = 0;
	iXPos = 0;


	// Main window
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos, iYPos, iXPos + iWidth, iYPos + iHeight, usFillColorLight );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 1, iYPos + 1, iXPos + iWidth, iYPos + iHeight, usFillColorDark );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 1, iYPos + 1, iXPos + iWidth - 1, iYPos + iHeight - 1, usFillColorBack );
	
	SetFont( FONT12POINT1 );

	// Name window
	gprintf( iXPos + 128, iYPos + 3, L"Merc Name:" );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 128, iYPos + 16, iXPos + 128 + 104, iYPos + 16 + 19, usFillColorDark );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 129, iYPos + 17, iXPos + 128 + 104, iYPos + 17 + 19, usFillColorLight );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 129, iYPos + 17, iXPos + 128 + 103, iYPos + 17 + 18, usFillColorTextBk );
	iXOff = (105 - StringPixLength( pSoldier->name, FONT12POINT1 )) / 2;
	gprintf( iXPos + 130 + iXOff, iYPos + 20, L"%s", pSoldier->name );
		
	// Orders window
	gprintf( iXPos + 128, iYPos + 38, L"Orders:" );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 128, iYPos + 51, iXPos + 128 + 104, iYPos + 51 + 19, usFillColorDark );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 129, iYPos + 52, iXPos + 128 + 104, iYPos + 52 + 19, usFillColorLight );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 129, iYPos + 52, iXPos + 128 + 103, iYPos + 52 + 18, usFillColorTextBk );
	iXOff = (105 - StringPixLength( EditMercOrders[pSoldier->bOrders], FONT12POINT1 )) / 2;
	gprintf( iXPos + 130 + iXOff, iYPos + 55, L"%s", EditMercOrders[pSoldier->bOrders] );

	// Combat window
	gprintf( iXPos + 128, iYPos + 73, L"Combat Attitude:" );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 128, iYPos + 86, iXPos + 128 + 104, iYPos + 86 + 19, usFillColorDark );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 129, iYPos + 87, iXPos + 128 + 104, iYPos + 87 + 19, usFillColorLight );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 129, iYPos + 87, iXPos + 128 + 103, iYPos + 87 + 18, usFillColorTextBk );
	iXOff = (105 - StringPixLength( EditMercAttitudes[pSoldier->bAttitude], FONT12POINT1 )) / 2;
	gprintf( iXPos + 130 + iXOff, iYPos + 90, L"%s", EditMercAttitudes[pSoldier->bAttitude] );

	// Get stats
	iEditStat[0] = pSoldier->bLifeMax;			  // 12 13
	iEditStat[1] = pSoldier->bLife;						// 14 15
	iEditStat[2] = pSoldier->bStrength;       // 16 17
	iEditStat[3] = pSoldier->bAgility;        // 18 19
	iEditStat[4] = pSoldier->bDexterity;      // 20 21
	iEditStat[5] = pSoldier->bLeadership;     // 22 23
	iEditStat[6] = pSoldier->bWisdom;         // 24 25
	iEditStat[7] = pSoldier->bMarksmanship;   // 26 27
	iEditStat[8] = pSoldier->bExplosive;      // 28 29
	iEditStat[9] = pSoldier->bMedical;        // 30 31
	iEditStat[10] = pSoldier->bScientific;    // 32 33
	iEditStat[11] = pSoldier->bExpLevel;      // 34 35

	// Stat value windows
	for ( x = 0; x < 12; x++ )
	{
		gprintf( iXPos + 6, iYPos + 114 + (20 * x), L"%s", EditMercStat[x] );
		ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 116, iYPos + 110 + (20 * x), iXPos + 116 + 30, iYPos + 110 + (20 * x) + 19, usFillColorDark );
		ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 117, iYPos + 111 + (20 * x), iXPos + 116 + 30, iYPos + 111 + (20 * x) + 19, usFillColorLight );
		ColorFillVideoSurfaceArea(FRAME_BUFFER, iXPos + 117, iYPos + 111 + (20 * x), iXPos + 116 + 29, iYPos + 111 + (20 * x) + 18, usFillColorTextBk );
		
		swprintf( TempString, L"%d", iEditStat[x] );
		iXOff = (30 - StringPixLength( TempString, FONT12POINT1 )) / 2;
		gprintf( iXPos + 118 + iXOff, iYPos + 114 + (20 * x), L"%s", TempString );
	}

}



//----------------------------------------------------------------------------------------------
//	IsMercHere
//
//	Checks for a soldier at the given map coordinates. If there is one, it returns it's ID number,
//	otherwise it returns -1.
//
INT32 IsMercHere( INT32 iMapIndex )
{
	INT32 IDNumber;
	INT32 RetIDNumber;
	SOLDIERTYPE *pSoldier;
	BOOLEAN fSoldierFound;

	RetIDNumber = -1;
	fSoldierFound = FALSE;
	for ( IDNumber = 0; IDNumber < MAX_NUM_SOLDIERS && !fSoldierFound; IDNumber++ )
	{
		if ( GetSoldier( &pSoldier, (INT16)IDNumber ) )
		{
			if ( pSoldier->sGridNo == (INT16)iMapIndex )
			{
				fSoldierFound = TRUE;
				RetIDNumber = IDNumber;
			}
		}
	}

	return( RetIDNumber );
}


//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//	The following are the button callback functions for the merc editing pages
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

void EditMercChangeToStatsPageCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_TO_STATS;
	}
}


void EditMercChangeToColorPageCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_TO_COLOR;
	}
}

void EditMercDoneEditCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_DONE;
	}
}


void EditMercBkgrndCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN)
	{
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_DONE;
	}
}


void EditMercPrevOrderCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_PREV_ORDER;
	}
}


void EditMercNextOrderCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_NEXT_ORDER;
	}
}

void EditMercPrevAttCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_PREV_ATT;
	}
}

void EditMercNextAttCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_NEXT_ATT;
	}
}

void EditMercStatUpCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT32 iBtn;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{	
		iEditWhichStat = -1;
		for ( iBtn = 0; iBtn < 36 && iEditWhichStat == -1; iBtn++ )
		{
			if ( btn->IDNum == iEditorButton[iBtn] )
				iEditWhichStat = iBtn;
		}

		if ( iEditWhichStat != -1 )
		{
			iEditStatTimer = 0;
			iEditMercMode = EDIT_MERC_INC_STAT;
		}

		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_NONE;
		btn->uiFlags &= (~BUTTON_CLICKED_ON);
	}
}

void EditMercStatDwnCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT32 iBtn;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{	
		iEditWhichStat = -1;
		for ( iBtn = 0; iBtn < 36 && iEditWhichStat == -1; iBtn++ )
		{
			if ( btn->IDNum == iEditorButton[iBtn] )
				iEditWhichStat = iBtn;
		}

		if ( iEditWhichStat != -1 )
		{
			iEditStatTimer = 0;
			iEditMercMode = EDIT_MERC_DEC_STAT;
		}

		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
	else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_NONE;
		btn->uiFlags &= (~BUTTON_CLICKED_ON);
	}
}


void EditMercSetDirCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT32 iBtn;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{	
		iEditWhichStat = -1;
		for ( iBtn = 0; iBtn < 36 && iEditWhichStat == -1; iBtn++ )
		{
			if ( btn->IDNum == iEditorButton[iBtn] )
				iEditWhichStat = iBtn;
		}

		if ( iEditWhichStat != -1 )
		{
			iEditStatTimer = 0;
			iEditMercMode = EDIT_MERC_SET_DIR;
		}

		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
}


void EditMercCenterCallback(GUI_BUTTON *btn,INT32 reason)
{
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{	
		iEditStatTimer = 0;
		iEditMercMode = EDIT_MERC_FIND;

		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
}


void EditMercColorDwnCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT32 iBtn;

	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{	
		iEditWhichStat = -1;
		for ( iBtn = 0; iBtn < 8 && iEditWhichStat == -1; iBtn++ )
		{
			if ( btn->IDNum == iEditorButton[iBtn] )
				iEditWhichStat = iBtn;
		}

		if ( iEditWhichStat != -1 )
		{
			iEditStatTimer = 0;
			iEditMercMode = EDIT_MERC_PREV_COLOR;
		}

		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
}

void MercsToggleColorModeCallback( GUI_BUTTON *btn, INT32 reason )
{
	if( reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		if( btn->uiFlags & BUTTON_CLICKED_ON ) //button is checked
		{
			EnableEditorButtons( FIRST_MERCS_COLOR_BUTTON, LAST_MERCS_COLOR_BUTTON );
			gpSelected->pDetailedPlacement->fVisible = TRUE;
			sprintf( gpSelected->pDetailedPlacement->HeadPal, gpSelected->pSoldier->HeadPal );
			sprintf( gpSelected->pDetailedPlacement->SkinPal, gpSelected->pSoldier->SkinPal );
			sprintf( gpSelected->pDetailedPlacement->VestPal, gpSelected->pSoldier->VestPal );
			sprintf( gpSelected->pDetailedPlacement->PantsPal, gpSelected->pSoldier->PantsPal );
		}
		else //button is unchecked.
		{
			DisableEditorButtons( FIRST_MERCS_COLOR_BUTTON, LAST_MERCS_COLOR_BUTTON );
			gpSelected->pDetailedPlacement->fVisible = FALSE;
		}
		gfRenderMercInfo = TRUE;
		gfRenderTaskbar = TRUE;
	}
}

void MercsSetColorsCallback( GUI_BUTTON *btn, INT32 reason )
{
	INT32 iBtn;
	if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{	
		iEditWhichStat = -1;
		for ( iBtn = FIRST_MERCS_COLOR_BUTTON; iBtn <= LAST_MERCS_COLOR_BUTTON; iBtn++ )
		{
			if ( btn->IDNum == iEditorButton[iBtn] )
			{
				iEditWhichStat = iBtn - FIRST_MERCS_COLOR_BUTTON;
				iEditStatTimer = 0;
				iEditMercMode = EDIT_MERC_NEXT_COLOR;
				gfRenderMercInfo = TRUE;
				return;
			}
		}
	}
}

void MercsSetBodyTypeCallback( GUI_BUTTON *btn, INT32 reason )
{
	if( reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		gfRenderMercInfo = TRUE;
		if( btn->IDNum == iEditorButton[ MERCS_BODYTYPE_DOWN ] )
			ChangeBodyType( 1 );	//next
		else
      ChangeBodyType( -1 ); //previous
	}
}

void EditMercDecDifficultyCallback(GUI_BUTTON *btn,INT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	
		iEditorToolbarState = TBAR_MODE_DEC_DIFF;
	}
}


void EditMercIncDifficultyCallback(GUI_BUTTON *btn,INT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		btn->uiFlags |= BUTTON_CLICKED_ON;
	
		iEditorToolbarState = TBAR_MODE_INC_DIFF;
	}
}





//----------------------------------------------------------------------------------------------
//	ShowEditMercPalettes
//
//	Displays the palette of the given merc (used by the edit merc color page)
//
void ShowEditMercPalettes( SOLDIERTYPE *pSoldier )
{
	UINT8  ubPaletteRep;
	if( !pSoldier )
		ubPaletteRep = 0xff;

	if( pSoldier )
	{
		if( !strlen( pSoldier->HeadPal ) )
			ubPaletteRep = 0xff;
		else		
			GetPaletteRepIndexFromID( pSoldier->HeadPal, &ubPaletteRep );
	}
	ShowEditMercColorSet( ubPaletteRep, 0 );
	
	if( pSoldier )
	{
		if( !strlen( pSoldier->SkinPal ) )
			ubPaletteRep = 0xff;
		else
			GetPaletteRepIndexFromID( pSoldier->SkinPal, &ubPaletteRep );
	}
	ShowEditMercColorSet( ubPaletteRep, 1 );
	
	if( pSoldier )
	{
		if( !strlen( pSoldier->VestPal ) )
			ubPaletteRep = 0xff;
		else
			GetPaletteRepIndexFromID( pSoldier->VestPal, &ubPaletteRep );
	}
	ShowEditMercColorSet( ubPaletteRep, 2 );

	if( pSoldier )
	{
		if( !strlen( pSoldier->VestPal ) )
			ubPaletteRep = 0xff;
		else
			GetPaletteRepIndexFromID( pSoldier->PantsPal, &ubPaletteRep );
	}
	ShowEditMercColorSet( ubPaletteRep, 3 );
}


//----------------------------------------------------------------------------------------------
//	ShowEditMercColorSet
//
//	Displays a single palette set. (used by ShowEditMercPalettes)
//
void ShowEditMercColorSet( UINT8 ubPaletteRep, INT16 sSet )
{
	UINT16 us16BPPColor, usFillColorDark, usFillColorLight;
	UINT8 cnt1;
	UINT8  ubSize;
	INT16 sUnitSize;
	INT16  sLeft, sTop, sRight, sBottom;

	if( ubPaletteRep == 0xff )
		ubSize = 16;
	else
		ubSize = gpPalRep[ ubPaletteRep ].ubPaletteSize;

	sUnitSize = 128 / (INT16)(ubSize);

	sTop = 364 + (sSet * 24);
	sBottom = sTop + 20;
	sLeft = 230;
	sRight = 359; 

	usFillColorDark = Get16BPPColor(FROMRGB(24, 61, 81));
	usFillColorLight = Get16BPPColor(FROMRGB(136, 138, 135));

	// Draw color bar window area
	ColorFillVideoSurfaceArea(FRAME_BUFFER, sLeft,		 sTop,		 sRight, sBottom, usFillColorDark );
	ColorFillVideoSurfaceArea(FRAME_BUFFER, sLeft + 1, sTop + 1, sRight, sBottom, usFillColorLight );
	InvalidateRegion( sLeft, sTop, sRight, sBottom );

	sTop++;
	sBottom--;
	sLeft++;
	sRight = sLeft + sUnitSize;

	// Draw the color bar
	for ( cnt1 = 0; cnt1 < ubSize; cnt1++ )
	{
		if (cnt1 == (ubSize - 1) )
			sRight = 358;
		if( ubPaletteRep == 0xff )
			us16BPPColor = Get16BPPColor( FROMRGB( (16 - cnt1)*10, (16 - cnt1)*10, (16 - cnt1)*10 ) );
		else
			us16BPPColor = Get16BPPColor( FROMRGB( gpPalRep[ ubPaletteRep ].r[ cnt1 ], gpPalRep[ ubPaletteRep ].g[ cnt1 ], gpPalRep[ ubPaletteRep ].b[ cnt1 ] ) );
		ColorFillVideoSurfaceArea( FRAME_BUFFER, sLeft, sTop, sRight, sBottom, us16BPPColor );

		sLeft += sUnitSize;
		sRight += sUnitSize;
	}
}


//----------------------------------------------------------------------------------------------
//	DisplayWayPoints
//
//	Displays the way points of the currently selected merc.
//
void DisplayWayPoints(void)
{
	INT16 sX,sY;
	INT16	sXMapPos,sYMapPos;
	INT16 sScreenX,sScreenY;
	FLOAT ScrnX,ScrnY,dOffsetX,dOffsetY;
	INT8	bPoint;
	SOLDIERTYPE *pSoldier;
	INT16 sGridNo;
	

	if ( gsSelectedMercID == -1 || (gsSelectedMercID <= (INT32)gTacticalStatus.Team[ OUR_TEAM ].bLastID) || gsSelectedMercID >= MAXMERCS )
		return;

	GetSoldier( &pSoldier, (UINT16)gsSelectedMercID );
	if ( pSoldier == NULL || !pSoldier->bActive)
		return;

	// point 0 is not used!
	for ( bPoint = 1; bPoint <= pSoldier->bPatrolCnt; bPoint++ )
	{
		// Get the next point
		sGridNo = (INT16)pSoldier->usPatrolGrid[bPoint];

		// Can we see it?
		if ( !GridNoOnVisibleWorldTile( sGridNo ) )
			continue;

		if((sGridNo < 0) || (sGridNo > WORLD_MAX))
			continue;

		// Convert it's location to screen coordinates
		ConvertGridNoToXY( sGridNo, &sXMapPos, &sYMapPos );

		dOffsetX = (FLOAT)(sXMapPos * CELL_X_SIZE) - gsRenderCenterX;
		dOffsetY = (FLOAT)(sYMapPos * CELL_Y_SIZE) - gsRenderCenterY;

		FloatFromCellToScreenCoordinates( dOffsetX, dOffsetY, &ScrnX, &ScrnY);
		
		sScreenX = ( ( gsVIEWPORT_END_X - gsVIEWPORT_START_X ) /2 ) + (INT16)ScrnX;
		sScreenY = ( ( gsVIEWPORT_END_Y - gsVIEWPORT_START_Y ) /2 ) + (INT16)ScrnY;

		// Adjust for tiles height factor!
		sScreenY -= gpWorldLevelData[sGridNo].sHeight;
		// Bring it down a touch
		sScreenY += 5;

		if( sScreenY <= 355 )
		{
			// Shown it on screen!
			SetFont(TINYFONT1);
			if( pSoldier->bLevel == 1 )
			{
				SetFontBackground( FONT_LTBLUE );
				sScreenY -= 68;
			}
			else
				SetFontBackground( FONT_LTRED );
			SetFontForeground( FONT_WHITE );
			VarFindFontCenterCoordinates( sScreenX, sScreenY, 1, 1, TINYFONT1, &sX, &sY, L"%d", bPoint );
			mprintf(sX,sY,L"%d",bPoint);
		}
	}
}


void CreateEditMercWindow( void )
{
	INT32 iXPos, iYPos, iHeight, iWidth;
	INT32 x;
	SOLDIERTYPE *pSoldier;

	iWidth = 266;
	iHeight = 360;
	iYPos = 0;
	iXPos = 0;


	GetSoldier( &pSoldier, (INT16)gsSelectedMercID );
	iEditMercLocation = (INT32)pSoldier->sGridNo;
	gpWorldLevelData[ iEditMercLocation ].pObjectHead->ubShadeLevel = DEFAULT_SHADE_LEVEL;

	iEditMercBkgrndArea = CreateHotSpot((INT16)iXPos, (INT16)iYPos, (INT16)iWidth, (INT16)iHeight, MSYS_PRIORITY_NORMAL, DEFAULT_MOVE_CALLBACK, EditMercBkgrndCallback );

	iEditMercColorPage = CreateTextButton( L"Merc Colors", (INT16)FONT12POINT1, FONT_BLACK, FONT_BLACK,
		BUTTON_USE_DEFAULT, (INT16)(iXPos + 183), (INT16)(iYPos + 315), 80, 20, BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercChangeToColorPageCallback );
	iEditMercEnd = CreateTextButton( L"Done", (INT16)FONT12POINT1, FONT_MCOLOR_BLACK, FONT_BLACK,
		BUTTON_USE_DEFAULT, (INT16)(iXPos + 183), (INT16)(iYPos + 337), 80, 20, BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercDoneEditCallback );

	// Disable color editing for PC Mercs
	if ( (UINT16)gsSelectedMercID >= gTacticalStatus.Team[ OUR_TEAM ].bFirstID && (UINT16)gsSelectedMercID <=  gTacticalStatus.Team[ OUR_TEAM ].bLastID )
		DisableButton( iEditMercColorPage );

	iEditorButton[8] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 98), (INT16)(iYPos + 51), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercPrevOrderCallback );
	iEditorButton[9] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 233), (INT16)(iYPos + 51), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercNextOrderCallback );
	SetButtonFastHelpText( iEditorButton[8], L"Previous merc standing orders" );
	SetButtonFastHelpText( iEditorButton[9], L"Next merc standing orders" );

	iEditorButton[10] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 98), (INT16)(iYPos + 86), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercPrevAttCallback );
	iEditorButton[11] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 233), (INT16)(iYPos + 86), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercNextAttCallback );
	SetButtonFastHelpText( iEditorButton[10], L"Previous merc combat attitude" );
	SetButtonFastHelpText( iEditorButton[11], L"Next merc combat attitude" );

	iEditorButton[12] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 110), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[13] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 110), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	iEditorButton[14] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 130), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[15] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 130), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );
	
	iEditorButton[16] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 150), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[17] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 150), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );
	
	iEditorButton[18] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 170), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[19] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 170), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );
	
	iEditorButton[20] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 190), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[21] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 190), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );
	
	iEditorButton[22] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 210), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[23] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 210), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	iEditorButton[24] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 230), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[25] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 230), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	iEditorButton[26] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 250), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[27] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 250), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	iEditorButton[28] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 270), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[29] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 270), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	iEditorButton[30] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 290), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[31] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 290), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	iEditorButton[32] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 310), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[33] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 310), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	iEditorButton[34] = QuickCreateButton( giEditMercImage[0], (INT16)(iXPos + 86), (INT16)(iYPos + 330), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatDwnCallback );
	iEditorButton[35] = QuickCreateButton( giEditMercImage[1], (INT16)(iXPos + 146), (INT16)(iYPos + 330), BUTTON_NO_TOGGLE, MSYS_PRIORITY_NORMAL + 1, DEFAULT_MOVE_CALLBACK, EditMercStatUpCallback );

	for ( x = 12; x < 36; x += 2 )
	{
		SetButtonFastHelpText( iEditorButton[x], L"Decrease merc stat" );
		SetButtonFastHelpText( iEditorButton[x + 1], L"Increase merc stat" );
	}
}
void SetMercOrders( INT8 bOrders )
{
	gpSelected->pSoldier->bOrders = bOrders;
	gpSelected->pBasicPlacement->bOrders = bOrders;
	UnclickEditorButtons( FIRST_MERCS_ORDERS_BUTTON, LAST_MERCS_ORDERS_BUTTON );
	ClickEditorButton( FIRST_MERCS_ORDERS_BUTTON + bOrders );
	gbDefaultOrders = bOrders;
}

void SetMercAttitude( INT8 bAttitude )
{
	gpSelected->pSoldier->bAttitude = bAttitude;
	gpSelected->pBasicPlacement->bAttitude = bAttitude;
	UnclickEditorButtons( FIRST_MERCS_ATTITUDE_BUTTON, LAST_MERCS_ATTITUDE_BUTTON );
	ClickEditorButton( FIRST_MERCS_ATTITUDE_BUTTON + bAttitude );
	gbDefaultAttitude = bAttitude;
}

void SetMercDirection( INT8 bDirection )
{
	UnclickEditorButtons( FIRST_MERCS_DIRECTION_BUTTON, LAST_MERCS_DIRECTION_BUTTON );
	ClickEditorButton( FIRST_MERCS_DIRECTION_BUTTON + bDirection );

	gbDefaultDirection = bDirection;
	gpSelected->pBasicPlacement->bDirection = bDirection;

	// ATE: Changed these to call functions....
	EVENT_SetSoldierDirection( gpSelected->pSoldier, bDirection );
	EVENT_SetSoldierDesiredDirection( gpSelected->pSoldier, bDirection );

	ConvertAniCodeToAniFrame( gpSelected->pSoldier, 0 );
}

void SetMercRelativeEquipment( INT8 bLevel )
{
	gpSelected->pBasicPlacement->bRelativeEquipmentLevel = bLevel;

	UnclickEditorButtons( FIRST_MERCS_REL_EQUIPMENT_BUTTON, LAST_MERCS_REL_EQUIPMENT_BUTTON );
	ClickEditorButton( FIRST_MERCS_REL_EQUIPMENT_BUTTON + bLevel );
	gbDefaultRelativeEquipmentLevel = bLevel;
}

void SetMercRelativeAttributes( INT8 bLevel )
{
	gpSelected->pBasicPlacement->bRelativeAttributeLevel = bLevel;
	//We also have to modify the existing soldier incase the user wishes to enter game.
	ModifySoldierAttributesWithNewRelativeLevel( gpSelected->pSoldier, bLevel );

	UnclickEditorButtons( FIRST_MERCS_REL_ATTRIBUTE_BUTTON, LAST_MERCS_REL_ATTRIBUTE_BUTTON );
	ClickEditorButton( FIRST_MERCS_REL_ATTRIBUTE_BUTTON + bLevel );
	gbDefaultRelativeAttributeLevel = bLevel;
}

void IndicateSelectedMerc( INT16 sID )
{
	SOLDIERINITNODE *prev;
	INT8 bTeam; 

	//If we are trying to select a merc that is already selected, ignore.
	if( sID >= 0 && sID == gsSelectedMercGridNo )
		return;

	//first remove the cursor of the previous merc.
	//NOTE:  It doesn't matter what the value is, even if a merc isn't selected.
	//There is no need to validate the gridNo value, because it is always valid.
	RemoveAllObjectsOfTypeRange( gsSelectedMercGridNo, CONFIRMMOVE, CONFIRMMOVE );

	//This is very important, because clearing the merc editing mode actually,
	//updates the edited merc.  If this call isn't here, it is possible to update the
	//newly selected merc with the wrong information.
	SetMercEditingMode( MERC_NOMODE );
	
	bTeam = -1;

	//determine selection method
	switch( sID )
	{
		case SELECT_NEXT_MERC:
			prev = gpSelected;
			if( gsSelectedMercID == -1 || !gpSelected )
			{ //if no merc selected, then select the first one in list.
				gpSelected = gSoldierInitHead;
			}
			else
			{ //validate this merc in the list.
				if( gpSelected->next )
				{ //select the next merc in the list
					gpSelected = gpSelected->next;
				}
				else
				{ //we are at the end of the list, so select the first merc in the list.
					gpSelected = gSoldierInitHead; 
				}
			}
			if( !gpSelected ) //list is empty
			{
				SetMercEditability( TRUE );
				SetMercEditingMode( MERC_TEAMMODE );
				return;
			}
			while( gpSelected != prev )
			{
				if( !gpSelected )
				{
					gpSelected = gSoldierInitHead;
					continue;
				}
				if( gpSelected->pSoldier && gpSelected->pSoldier->bVisible == 1 )
				{ //we have found a visible soldier, so select him.
					break;
				}
				gpSelected = gpSelected->next;
			}
			//we have a valid merc now.
			break;
		case SELECT_NO_MERC:
			SetMercEditability( TRUE );
			gpSelected = NULL;
			gsSelectedMercID = -1;
			gsSelectedGridNo = 0;
			SetMercEditingMode( MERC_TEAMMODE );
			return; //we already deselected the previous merc.
		case SELECT_NEXT_ENEMY:
			bTeam = ENEMY_TEAM;
			break;
		case SELECT_NEXT_CREATURE:
			bTeam = CREATURE_TEAM;
			break;
		case SELECT_NEXT_REBEL:
			bTeam = MILITIA_TEAM;
			break;
		case SELECT_NEXT_CIV:
			bTeam = CIV_TEAM;
			break;
		default:
			//search for the merc with the specific ID.
			gpSelected = FindSoldierInitNodeWithID( (UINT8)sID );
			if( !gpSelected )
			{
				gsSelectedMercID = -1;
				gsSelectedGridNo = 0;
				SetMercEditability( TRUE );
				SetMercEditingMode( MERC_TEAMMODE );
				return; //Invalid merc ID.
			}
			break;
	}
	if( bTeam != -1 )
	{ //We are searching for the next occurence of a particular team.
		prev = gpSelected;
		if( gsSelectedMercID == -1 || !gpSelected )
		{ //if no merc selected, then select the first one in list.
			gpSelected = gSoldierInitHead;
		}
		else
		{ //validate this merc in the list.
			if( gpSelected->next )
			{ //select the next merc in the list
				gpSelected = gpSelected->next;
			}
			else
			{ //we are at the end of the list, so select the first merc in the list.
				gpSelected = gSoldierInitHead; 
			}
		}
		if( !gpSelected ) //list is empty
		{
			SetMercEditability( TRUE );
			SetMercEditingMode( MERC_TEAMMODE );
			return;
		}
		while( gpSelected != prev )
		{
			if( !gpSelected )
			{
				gpSelected = gSoldierInitHead;
				continue;
			}
			if( gpSelected->pSoldier && gpSelected->pSoldier->bVisible == 1 && gpSelected->pSoldier->bTeam == bTeam )
			{ //we have found a visible soldier on the desired team, so select him.
				break;
			}
			gpSelected = gpSelected->next;
		}
		if( !gpSelected )
			return;
		if( gpSelected == prev  )
		{ //we have cycled through the list already, so choose the same guy (if he is on the desired team)...
			if( !gpSelected->pSoldier || gpSelected->pSoldier->bVisible != 1 || gpSelected->pSoldier->bTeam != bTeam )
			{
				SetMercEditability( TRUE );
				SetMercEditingMode( MERC_TEAMMODE );
				return;
			}
		}
	}
	//if we made it this far, then we have a new merc cursor indicator to draw.
	if( gpSelected->pSoldier )
		gsSelectedMercGridNo = gpSelected->pSoldier->sGridNo;
	else
	{
		SetMercEditability( TRUE );
		SetMercEditingMode( MERC_TEAMMODE );
		return;
	}
	gsSelectedMercID = gpSelected->pSoldier->ubID;
	AddObjectToHead( gsSelectedMercGridNo, CONFIRMMOVE1 );

	//If the merc has a valid profile, then turn off editability
	if( gpSelected->pDetailedPlacement )
		SetMercEditability( (BOOLEAN)(gpSelected->pDetailedPlacement->ubProfile == NO_PROFILE) );
	else
		SetMercEditability( TRUE );

	if( sID < 0 )
	{	//We want to center the screen on the next merc, and update the interface.
		gsRenderCenterX = (INT16)gpSelected->pSoldier->dXPos;
		gsRenderCenterY = (INT16)gpSelected->pSoldier->dYPos;
		gfRenderWorld = TRUE;
	}

	//update the merc item slots to reflect what the merc currently has.
	UpdateMercItemSlots();
	
	//Whatever the case, we want to update the gui to press the appropriate buttons 
	//depending on the merc's attributes.
	//Click the appropriate team button
	UnclickEditorButton( MERCS_ENEMY );
	UnclickEditorButton( MERCS_CREATURE );
	UnclickEditorButton( MERCS_REBEL );
	UnclickEditorButton( MERCS_CIVILIAN );
	switch( gpSelected->pSoldier->bTeam )
	{
		case ENEMY_TEAM:		ClickEditorButton( MERCS_ENEMY );			iDrawMode = DRAW_MODE_ENEMY;		break;
		case CREATURE_TEAM:	ClickEditorButton( MERCS_CREATURE );	iDrawMode = DRAW_MODE_CREATURE;	break;
		case MILITIA_TEAM:	ClickEditorButton( MERCS_REBEL );			iDrawMode = DRAW_MODE_REBEL;		break;
		case CIV_TEAM:			ClickEditorButton( MERCS_CIVILIAN );	iDrawMode = DRAW_MODE_CIVILIAN;	break;
	}
	//Update the editing mode
	if( gpSelected->pDetailedPlacement )
		SetMercEditingMode( gubLastDetailedMercMode );
	else
		SetMercEditingMode( MERC_BASICMODE );
	//Determine which team button to press.
	gfRenderMercInfo = TRUE;
	//These calls will set the proper button states, even though it redundantly
	//assigns the soldier with the same orders/attitude.
	SetMercOrders( gpSelected->pSoldier->bOrders );
	SetMercAttitude( gpSelected->pSoldier->bAttitude );
	SetMercDirection( gpSelected->pSoldier->bDirection );
	if( gpSelected->pBasicPlacement->fPriorityExistance )
		ClickEditorButton( MERCS_PRIORITYEXISTANCE_CHECKBOX );
	else
		UnclickEditorButton( MERCS_PRIORITYEXISTANCE_CHECKBOX );
	if( gpSelected->pBasicPlacement->fHasKeys )
		ClickEditorButton( MERCS_HASKEYS_CHECKBOX );
	else
		UnclickEditorButton( MERCS_HASKEYS_CHECKBOX );
	if( gpSelected->pSoldier->ubProfile == NO_PROFILE )
	{
		SetMercRelativeEquipment( gpSelected->pBasicPlacement->bRelativeEquipmentLevel );
		SetMercRelativeAttributes( gpSelected->pBasicPlacement->bRelativeAttributeLevel );
		SetEnemyColorCode( gpSelected->pBasicPlacement->ubSoldierClass );
	}
	if( iDrawMode == DRAW_MODE_CIVILIAN )
	{
		ChangeCivGroup( gpSelected->pSoldier->ubCivilianGroup );
	}
}

void DeleteSelectedMerc()
{
	if( gsSelectedMercID != -1 )
	{
		RemoveSoldierNodeFromInitList( gpSelected );
		gpSelected = NULL;
		gsSelectedMercID = -1;
		gfRenderWorld = TRUE;
		if( TextInputMode() )
			KillTextInputMode();
		IndicateSelectedMerc( SELECT_NO_MERC );
	}
}

void SetupTextInputForMercProfile()
{
	UINT16 str[4];
	INT16 sNum;

	InitTextInputModeWithScheme( DEFAULT_SCHEME );

	sNum = gpSelected->pDetailedPlacement->ubProfile;
	if( sNum == NO_PROFILE )
		str[0] = '\0';
	else
		CalcStringForValue( str, gpSelected->pDetailedPlacement->ubProfile, NUM_PROFILES );
	AddTextInputField( 200, 430, 30, 20, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );

}

void SetupTextInputForMercAttributes()
{
	UINT16 str[4];

	InitTextInputModeWithScheme( DEFAULT_SCHEME );

	CalcStringForValue( str, gpSelected->pDetailedPlacement->bExpLevel, 100 );
	AddTextInputField( 200, 365, 20, 15, MSYS_PRIORITY_NORMAL, str, 1, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bLife, 100 );
	AddTextInputField( 200, 390, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bLifeMax, 100 );
	AddTextInputField( 200, 415, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bMarksmanship, 100 );
	AddTextInputField( 200, 440, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bStrength, 100 );
	AddTextInputField( 300, 365, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bAgility, 100 );
	AddTextInputField( 300, 390, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bDexterity, 100 );
	AddTextInputField( 300, 415, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bWisdom, 100 );
	AddTextInputField( 300, 440, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bLeadership, 100 );
	AddTextInputField( 400, 365, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bExplosive, 100 );
	AddTextInputField( 400, 390, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bMedical, 100 );
	AddTextInputField( 400, 415, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bMechanical, 100 );
	AddTextInputField( 400, 440, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );
	CalcStringForValue( str, gpSelected->pDetailedPlacement->bMorale, 100 );
	AddTextInputField( 500, 365, 20, 15, MSYS_PRIORITY_NORMAL, str, 3, INPUTTYPE_NUMERICSTRICT );

	if( !gfCanEditMercs )
		DisableAllTextFields();
}

//In the merc editing, all detailed placement values for generated attributes are set to -1.  
//When making a generated attribute static, we then set the value to its applicable value.
//This function is similar to the itoa function except that -1 is converted to a null string.
void CalcStringForValue( UINT16 *str, INT32 iValue, UINT32 uiMax )
{
	if( iValue < 0 )			//a blank string is determined by a negative value.
		str[0] = '\0';
	else if( (UINT32)iValue > uiMax )	//higher than max attribute value, so convert it to the max.
		swprintf( str, L"%d", uiMax );
	else										//this is a valid static value, so convert it to a string.
		swprintf( str, L"%d", iValue );
}

void ExtractAndUpdateMercAttributes()
{
	//If we have just deleted the merc's detailed placement in the editor, we don't
	//need to extract the information
	if( !gpSelected->pDetailedPlacement )
		return;

	//It just so happens that GetNumericStrict...() will return -1 for any blank fields.
	//-1 values in the detailed placement work nicely, because that signifies that specific
	//field isn't static.  Any other value becomes static, and static values override any
	//generated values.
	gpSelected->pDetailedPlacement->bExpLevel			= (INT8)min( GetNumericStrictValueFromField( 0 ), 100 );
	gpSelected->pDetailedPlacement->bLife					= (INT8)min( GetNumericStrictValueFromField( 1 ), 100 );
	gpSelected->pDetailedPlacement->bLifeMax			= (INT8)min( GetNumericStrictValueFromField( 2 ), 100 );
	gpSelected->pDetailedPlacement->bMarksmanship	= (INT8)min( GetNumericStrictValueFromField( 3 ), 100 );
	gpSelected->pDetailedPlacement->bStrength			= (INT8)min( GetNumericStrictValueFromField( 4 ), 100 );
	gpSelected->pDetailedPlacement->bAgility			= (INT8)min( GetNumericStrictValueFromField( 5 ), 100 );
	gpSelected->pDetailedPlacement->bDexterity		= (INT8)min( GetNumericStrictValueFromField( 6 ), 100 );
	gpSelected->pDetailedPlacement->bWisdom				= (INT8)min( GetNumericStrictValueFromField( 7 ), 100 );
	gpSelected->pDetailedPlacement->bLeadership		= (INT8)min( GetNumericStrictValueFromField( 8 ), 100 );
	gpSelected->pDetailedPlacement->bExplosive		= (INT8)min( GetNumericStrictValueFromField( 9 ), 100 );
	gpSelected->pDetailedPlacement->bMedical			= (INT8)min( GetNumericStrictValueFromField( 10 ), 100 );
	gpSelected->pDetailedPlacement->bMechanical		= (INT8)min( GetNumericStrictValueFromField( 11 ), 100 );
	gpSelected->pDetailedPlacement->bMorale				= (INT8)min( GetNumericStrictValueFromField( 11 ), 100 );

	//make sure that experience level ranges between 1 and 9
	if( gpSelected->pDetailedPlacement->bExpLevel != -1 )
		gpSelected->pDetailedPlacement->bExpLevel = max( min( gpSelected->pDetailedPlacement->bExpLevel , 9 ), 1 );

	//no such thing as a life max of 0
	if( !gpSelected->pDetailedPlacement->bLifeMax )
		gpSelected->pDetailedPlacement->bLifeMax = 1;

	//make sure the life doesn't exceed the maxlife...
	if( gpSelected->pDetailedPlacement->bLifeMax != -1 && gpSelected->pDetailedPlacement->bLife != -1 &&
			gpSelected->pDetailedPlacement->bLife > gpSelected->pDetailedPlacement->bLifeMax )
	  gpSelected->pDetailedPlacement->bLife = gpSelected->pDetailedPlacement->bLifeMax;

	//update the soldier
	UpdateSoldierWithStaticDetailedInformation( gpSelected->pSoldier, gpSelected->pDetailedPlacement );
}

void ExtractAndUpdateMercProfile()
{
	INT16 sNum;
	static INT16 sPrev = NO_PROFILE;

	//If we have just deleted the merc's detailed placement in the editor, we don't
	//need to extract the information
	if( !gpSelected->pDetailedPlacement )
		return;

	//if the string is blank, returning -1, then set the value to NO_PROFILE
	//because ubProfile is unsigned.
	sNum = (INT16)min( GetNumericStrictValueFromField( 0 ), NUM_PROFILES );
	if( sNum == -1 )
	{
		gpSelected->pDetailedPlacement->ubProfile = NO_PROFILE;
		gpSelected->pDetailedPlacement->fCopyProfileItemsOver = FALSE;
		SetMercEditability( TRUE );
	}
	else if( sPrev != sNum )
	{
		gpSelected->pDetailedPlacement->ubProfile = (UINT8)sNum;
		gpSelected->pDetailedPlacement->fCopyProfileItemsOver = TRUE;
		gpSelected->pBasicPlacement->fPriorityExistance = TRUE;
		ClickEditorButton( MERCS_PRIORITYEXISTANCE_CHECKBOX );
		SetMercEditability( FALSE );
	}
	else
		return;

	UpdateSoldierWithStaticDetailedInformation( gpSelected->pSoldier, gpSelected->pDetailedPlacement );
	if( gpSelected->pSoldier->bTeam == CIV_TEAM )
	{
		ChangeCivGroup( gpSelected->pSoldier->ubCivilianGroup );
	}
}

void SetupTextInputForMercSchedule()
{
	InitTextInputModeWithScheme( DEFAULT_SCHEME );
	AddUserInputField( NULL );
	AddTextInputField( 268, 373, 36, 16, MSYS_PRIORITY_NORMAL, L"", 6, INPUTTYPE_EXCLUSIVE_24HOURCLOCK );
	SetExclusive24HourTimeValue( 1, gCurrSchedule.usTime[0] );
	AddTextInputField( 268, 394, 36, 16, MSYS_PRIORITY_NORMAL, L"", 6, INPUTTYPE_EXCLUSIVE_24HOURCLOCK );
	SetExclusive24HourTimeValue( 2, gCurrSchedule.usTime[1] );
	AddTextInputField( 268, 415, 36, 16, MSYS_PRIORITY_NORMAL, L"", 6, INPUTTYPE_EXCLUSIVE_24HOURCLOCK );
	SetExclusive24HourTimeValue( 3, gCurrSchedule.usTime[2] );
	AddTextInputField( 268, 436, 36, 16, MSYS_PRIORITY_NORMAL, L"", 6, INPUTTYPE_EXCLUSIVE_24HOURCLOCK );
	SetExclusive24HourTimeValue( 4, gCurrSchedule.usTime[3] );
}

void ExtractAndUpdateMercSchedule()
{
	INT32 i;
	BOOLEAN fValidSchedule = FALSE;
	BOOLEAN fScheduleNeedsUpdate = FALSE;
	SCHEDULENODE *pNext = NULL;
  if( !gpSelected )
		return;
	//extract all of the fields into a temp schedulenode.
	//memset( &gScheduleNode, 0, sizeof( SCHEDULENODE ) );
	for( i = 0; i < 4; i++ )
	{
		gCurrSchedule.usTime[i]	= GetExclusive24HourTimeValueFromField( (UINT8)(i+1) );
		gCurrSchedule.ubAction[i] = (UINT8)MSYS_GetBtnUserData( ButtonList[ iEditorButton[ MERCS_SCHEDULE_ACTION1 + i ]], 0 );
		if( gCurrSchedule.ubAction[i] )
			fValidSchedule = TRUE;
	}

	if( !gpSelected->pSoldier->ubScheduleID )
	{ //The soldier doesn't actually have a schedule yet, so create one if necessary (not blank)
		if( fValidSchedule )
		{ //create a new schedule
			if( SortSchedule( &gCurrSchedule ) )
				fScheduleNeedsUpdate = TRUE;
			CopyScheduleToList( &gCurrSchedule, gpSelected );
			ShowEditorButton( MERCS_GLOWSCHEDULE );
			HideEditorButton( MERCS_SCHEDULE );
		}
	}
	else
	{
		SCHEDULENODE *pSchedule;
		pSchedule = GetSchedule( gpSelected->pSoldier->ubScheduleID );
		if( !pSchedule )
		{
			gpSelected->pSoldier->ubScheduleID = 0;
			gpSelected->pDetailedPlacement->ubScheduleID = 0;
			HideEditorButton( MERCS_GLOWSCHEDULE );
			ShowEditorButton( MERCS_SCHEDULE );
			return;
		}
		if( fValidSchedule )
		{ //overwrite the existing schedule with the new one.
			gCurrSchedule.ubScheduleID = gpSelected->pSoldier->ubScheduleID;
			if( SortSchedule( &gCurrSchedule ) )
				fScheduleNeedsUpdate = TRUE;
			pNext = pSchedule->next;
			memcpy( pSchedule, &gCurrSchedule, sizeof( SCHEDULENODE ) );
			pSchedule->next = pNext;
		}
		else
		{ //remove the existing schedule, as the new one is blank.
			DeleteSchedule( pSchedule->ubScheduleID );
			gpSelected->pSoldier->ubScheduleID = 0;
			gpSelected->pDetailedPlacement->ubScheduleID = 0;
			HideEditorButton( MERCS_GLOWSCHEDULE );
			ShowEditorButton( MERCS_SCHEDULE );
		}
	}
	if( fScheduleNeedsUpdate )
	{ //The schedule was sorted, so update the gui.
		UpdateScheduleInfo();
	}
	SetActiveField( 0 );
}

void ExtractCurrentMercModeInfo( BOOLEAN fKillTextInputMode )
{
	//This happens if we deleted a merc
	if( gsSelectedMercID == -1 )
		return;
	//Extract and update mercs via text fields if applicable
	switch( gubCurrMercMode )
	{
		case MERC_ATTRIBUTEMODE:
			ExtractAndUpdateMercAttributes();
			break;
		case MERC_PROFILEMODE:
			ExtractAndUpdateMercProfile();
			break;
		case MERC_SCHEDULEMODE:
			ExtractAndUpdateMercSchedule();
			break;
		default: 
			fKillTextInputMode = FALSE;
			break;
	}
	if( fKillTextInputMode )
		KillTextInputMode();
}

void InitDetailedPlacementForMerc()
{
	Assert( !gpSelected->pDetailedPlacement );

	gpSelected->pDetailedPlacement = (SOLDIERCREATE_STRUCT*)MemAlloc(sizeof(SOLDIERCREATE_STRUCT));

	Assert( gpSelected->pDetailedPlacement );
	
	gpSelected->pBasicPlacement->fDetailedPlacement = TRUE;
	gpSelected->pBasicPlacement->fPriorityExistance = FALSE;
	CreateStaticDetailedPlacementGivenBasicPlacementInfo( gpSelected->pDetailedPlacement, gpSelected->pBasicPlacement );
	
	ClearCurrentSchedule();

	//update the soldier
	UpdateSoldierWithStaticDetailedInformation ( gpSelected->pSoldier, gpSelected->pDetailedPlacement );
}

void KillDetailedPlacementForMerc()
{
	Assert( gpSelected->pDetailedPlacement );
	MemFree( gpSelected->pDetailedPlacement );
	gpSelected->pDetailedPlacement = NULL;
	gpSelected->pBasicPlacement->fDetailedPlacement = FALSE;
	SetMercEditability( TRUE );
}


void ChangeBodyType( INT8 bOffset )  //+1 or -1 only
{
	INT8 *pbArray;
	INT32	iMax, x;
	INT32 iIndex;

	gfRenderTaskbar = TRUE;
	gfRenderMercInfo = TRUE;
	//verify that we have a proper offset ( only +-1 allowed )
	Assert( bOffset == -1 || bOffset == 1 );
	//get access to information depending on the team
	switch( gpSelected->pBasicPlacement->bTeam )
	{
		case ENEMY_TEAM:
			pbArray = bEnemyArray;
			iMax = MAX_ENEMYTYPES;
			break;
		case CREATURE_TEAM:
			pbArray = bCreatureArray;
			iMax = MAX_CREATURETYPES;
			break;
		case MILITIA_TEAM:
			pbArray = bRebelArray;
			iMax = MAX_REBELTYPES;
			break;
		case CIV_TEAM:
			pbArray = bCivArray;
			iMax = MAX_CIVTYPES;
			break;
	}
	//find the matching bodytype index within the array.
	for( x = 0; x < iMax; x++ )
	{
		iIndex = pbArray[ x ];
		if( iIndex == gpSelected->pBasicPlacement->bBodyType )
			break;
	}
	Assert( iIndex == gpSelected->pBasicPlacement->bBodyType );
	//now we have a match, so go to the next element (depending on offset value)
	x += bOffset;
	if( x >= iMax )
		x = 0;
	else if( x < 0 )
		x = iMax-1;
	iIndex = pbArray[ x ];
	//Set the new bodytype into the and update the soldier info
	if( iIndex != -1 )
	{
		gpSelected->pSoldier->ubBodyType = (UINT8)iIndex;
		//Set the flags based on the bodytype
		gpSelected->pSoldier->uiStatusFlags &= ~(SOLDIER_VEHICLE | SOLDIER_ROBOT | SOLDIER_ANIMAL | SOLDIER_MONSTER);
		switch( gpSelected->pSoldier->ubBodyType )
		{
			case ADULTFEMALEMONSTER:
			case AM_MONSTER:
			case YAF_MONSTER:
			case YAM_MONSTER:
			case LARVAE_MONSTER:
			case INFANT_MONSTER:
			case QUEENMONSTER:
				gpSelected->pSoldier->uiStatusFlags |= SOLDIER_MONSTER;
				break;
			case BLOODCAT:
			case COW:
			case CROW:
				gpSelected->pSoldier->uiStatusFlags |= SOLDIER_ANIMAL;
				break;
			case ROBOTNOWEAPON:
				gpSelected->pSoldier->uiStatusFlags |= SOLDIER_ROBOT;
				break;
			case HUMVEE:
			case ELDORADO:
			case ICECREAMTRUCK:
			case JEEP:
			case TANK_NW:
			case TANK_NE:
				gpSelected->pSoldier->uiStatusFlags |= SOLDIER_VEHICLE;
				break;
		}
		SetSoldierAnimationSurface( gpSelected->pSoldier, gpSelected->pSoldier->usAnimState );
	}
	//Update the placement's info as well.
	gpSelected->pBasicPlacement->bBodyType = (INT8)iIndex;
	if( gpSelected->pDetailedPlacement )
	{
		gpSelected->pDetailedPlacement->bBodyType = (INT8)iIndex;
	}
	if( gpSelected->pSoldier->bTeam == CREATURE_TEAM )
	{
		gbCurrCreature = (INT8)iIndex;
		AssignCreatureInventory( gpSelected->pSoldier );
	}
	CreateSoldierPalettes( gpSelected->pSoldier );
}

void SetMercEditability( BOOLEAN fEditable )
{
	gfRenderMercInfo = TRUE;
	if( fEditable == gfCanEditMercs )
		return;
	gfCanEditMercs = fEditable;
	if( gfCanEditMercs )
	{ //enable buttons to allow editing
		EnableEditorButtons( MERCS_EQUIPMENT_BAD, MERCS_ATTRIBUTES_GREAT );
		EnableEditorButtons( FIRST_MERCS_COLORMODE_BUTTON, LAST_MERCS_COLORMODE_BUTTON );
		if( gpSelected && gpSelected->pDetailedPlacement && !gpSelected->pDetailedPlacement->fVisible )
			UnclickEditorButton( MERCS_TOGGLECOLOR_BUTTON );
		EnableEditorButton( MERCS_PRIORITYEXISTANCE_CHECKBOX );
		EnableEditorButton( MERCS_CIVILIAN_GROUP );
	}
	else
	{ //disable buttons to prevent editing
		DisableEditorButtons( MERCS_EQUIPMENT_BAD, MERCS_ATTRIBUTES_GREAT );
		DisableEditorButtons( FIRST_MERCS_COLORMODE_BUTTON, LAST_MERCS_COLORMODE_BUTTON );
		ClickEditorButton( MERCS_TOGGLECOLOR_BUTTON );
		DisableEditorButton( MERCS_PRIORITYEXISTANCE_CHECKBOX );
		DisableEditorButton( MERCS_CIVILIAN_GROUP );
	}	
}


//There are 4 exclusive entry points in a map.  Only one of each type can exist on a 
//map, and these points are used to validate the map by attempting to connect the four
//points together.  If one of the points is isolated, then the map will be rejected.  It
//isn't necessary to specify all four points.  You wouldn't want to specify a north point if
//there isn't going to be any traversing to adjacent maps from that side.
void SpecifyEntryPoint( UINT32 iMapIndex )
{
	INT16 *psEntryGridNo;
	BOOLEAN fErasing = FALSE;
	if( iDrawMode >= DRAW_MODE_ERASE )
	{
		iDrawMode -= DRAW_MODE_ERASE;
		fErasing = TRUE;
	}
	switch( iDrawMode )
	{
		case DRAW_MODE_NORTHPOINT:		psEntryGridNo = &gMapInformation.sNorthGridNo;		break;
		case DRAW_MODE_WESTPOINT:			psEntryGridNo = &gMapInformation.sWestGridNo;			break;
		case DRAW_MODE_EASTPOINT:			psEntryGridNo = &gMapInformation.sEastGridNo;			break;
		case DRAW_MODE_SOUTHPOINT:		psEntryGridNo = &gMapInformation.sSouthGridNo;		break;
		case DRAW_MODE_CENTERPOINT:		psEntryGridNo = &gMapInformation.sCenterGridNo;		break;
		case DRAW_MODE_ISOLATEDPOINT:	psEntryGridNo = &gMapInformation.sIsolatedGridNo;	break;
		default:																																				return;
	}
	if( !fErasing )
	{
		if( *psEntryGridNo >= 0 )
		{
			AddToUndoList( *psEntryGridNo );
			RemoveAllTopmostsOfTypeRange( *psEntryGridNo, FIRSTPOINTERS, FIRSTPOINTERS );
		}
		*psEntryGridNo = (UINT16)iMapIndex;
		ValidateEntryPointGridNo( psEntryGridNo );
		AddToUndoList( *psEntryGridNo );
		AddTopmostToTail( *psEntryGridNo, FIRSTPOINTERS2 );
	}
	else
	{
		UINT16 usDummy;
		if( TypeExistsInTopmostLayer( iMapIndex, FIRSTPOINTERS, &usDummy ) )
		{
			AddToUndoList( iMapIndex );
			RemoveAllTopmostsOfTypeRange( iMapIndex, FIRSTPOINTERS, FIRSTPOINTERS );
			*psEntryGridNo = -1;
		}
		//restore the drawmode
		iDrawMode += DRAW_MODE_ERASE;
	}
}

void SetMercEditingMode( UINT8 ubNewMode )
{
	//We need to update the taskbar for the buttons that were erased.
	gfRenderTaskbar = TRUE;

	//set up the new mode values.
	if( gubCurrMercMode >= MERC_GENERALMODE )
		gubLastDetailedMercMode = gubCurrMercMode;

	//Depending on the mode we were just in, we may want to extract and update the
	//merc first.  Then we change modes...
	ExtractCurrentMercModeInfo( TRUE );
	
	//Change modes now.
	gubPrevMercMode = gubCurrMercMode;
	gubCurrMercMode = ubNewMode;

	//Hide all of the merc buttons except the team buttons which are static.
	HideEditorButtons( FIRST_MERCS_BASICMODE_BUTTON, LAST_MERCS_BUTTON );

	switch( gubPrevMercMode )
	{
		case MERC_GETITEMMODE:
			EnableEditorButtons( TAB_TERRAIN, TAB_OPTIONS );
			HideEditorButtons( FIRST_MERCS_GETITEM_BUTTON, LAST_MERCS_GETITEM_BUTTON );
			AddNewItemToSelectedMercsInventory( TRUE );
			break;
		case MERC_INVENTORYMODE:
			HideItemStatsPanel();
			DisableEditorRegion( MERC_REGION_ID );
			break;
		case MERC_GENERALMODE:
			EnableEditorButton( MERCS_APPEARANCE );
			break;
		case MERC_SCHEDULEMODE:
			//ClearCurrentSchedule();
			break;
	}
	
	//If we leave the merc tab, then we want to update editable fields such as
	//attributes, which was just handled above, then turn everything off, and exit.
	if( ubNewMode == MERC_NOMODE )
	{
		HideEditorButtons( FIRST_MERCS_BUTTON, LAST_MERCS_TEAMMODE_BUTTON );
		HideEditorButtons( MERCS_SCHEDULE, MERCS_GLOWSCHEDULE );
		return;
	}
	if( gubPrevMercMode == MERC_NOMODE || gubPrevMercMode == MERC_GETITEMMODE )
	{
		ShowEditorButtons( FIRST_MERCS_BUTTON, LAST_MERCS_TEAMMODE_BUTTON );
	}
	
	//Release the currently selected merc if you just selected a new team.
	if( gsSelectedMercID != -1 && ubNewMode == MERC_TEAMMODE )
	{
		//attempt to weed out conditions where we select a team that matches the currently
		//selected merc.  We don't want to deselect him in this case.
		if( gpSelected->pSoldier->bTeam == ENEMY_TEAM && iDrawMode == DRAW_MODE_ENEMY ||
				gpSelected->pSoldier->bTeam == CREATURE_TEAM && iDrawMode == DRAW_MODE_CREATURE ||
				gpSelected->pSoldier->bTeam == MILITIA_TEAM && iDrawMode == DRAW_MODE_REBEL ||
				gpSelected->pSoldier->bTeam == CIV_TEAM && iDrawMode == DRAW_MODE_CIVILIAN )
		{	//Same team, so don't deselect merc.  Instead, keep the previous editing mode
			//because we are still editing this merc.
			gubCurrMercMode = gubPrevMercMode;
			//if we don't have a detailed placement, auto set to basic mode.
			if( !gpSelected->pDetailedPlacement )
				gubCurrMercMode = MERC_BASICMODE;
		}
		else
		{
			//Different teams, so deselect the current merc and the detailed checkbox if applicable.
			IndicateSelectedMerc( SELECT_NO_MERC );
			ShowEditorButtons( FIRST_MERCS_BUTTON, LAST_MERCS_TEAMMODE_BUTTON );
			UnclickEditorButton( MERCS_DETAILEDCHECKBOX );
		}
	}

	ShowButton( iEditorButton[ MERCS_NEXT ] );
	if( gsSelectedMercID != -1 )
		ShowButton( iEditorButton[ MERCS_DELETE ] );

	if( gubCurrMercMode > MERC_TEAMMODE )
	{	//Add the basic buttons if applicable.
		ShowEditorButtons( FIRST_MERCS_BASICMODE_BUTTON, LAST_MERCS_BASICMODE_BUTTON );
	}
	if( gubCurrMercMode > MERC_BASICMODE )
	{ //Add the detailed buttons if applicable.
		ClickEditorButton( MERCS_DETAILEDCHECKBOX );
		ShowEditorButtons( FIRST_MERCS_PRIORITYMODE_BUTTON, LAST_MERCS_PRIORITYMODE_BUTTON );
	}
	else
		UnclickEditorButton( MERCS_DETAILEDCHECKBOX );
	//Now we are setting up the button states for the new mode, as well as show the 
	//applicable buttons for the detailed placement modes.
	if( gubCurrMercMode == MERC_APPEARANCEMODE && iDrawMode == DRAW_MODE_CREATURE || 
			gubCurrMercMode == MERC_SCHEDULEMODE && iDrawMode != DRAW_MODE_CIVILIAN )
	{
		gubCurrMercMode = MERC_GENERALMODE;
	}
	switch( gubCurrMercMode )
	{
		case MERC_GETITEMMODE:
			DisableEditorButtons( TAB_TERRAIN, TAB_OPTIONS );
			EnableEditorButton( TAB_MERCS );
			HideEditorButtons( FIRST_MERCS_BUTTON, LAST_MERCS_TEAMMODE_BUTTON );
			HideEditorButtons( MERCS_SCHEDULE, MERCS_GLOWSCHEDULE );
			ShowEditorButtons( FIRST_MERCS_GETITEM_BUTTON, LAST_MERCS_GETITEM_BUTTON );
			InitEditorItemsInfo( eInfo.uiItemType );
			ClickEditorButton( ITEMS_WEAPONS + eInfo.uiItemType - TBAR_MODE_ITEM_WEAPONS );
			break;
		case MERC_INVENTORYMODE:
			UpdateMercItemSlots();
			ShowItemStatsPanel();
			if( gbCurrSelect == -1 )
				SpecifyItemToEdit( NULL, -1 );
			else
				SpecifyItemToEdit( gpMercSlotItem[ gbCurrSelect ], -1 );
			HideEditorButtons( MERCS_DELETE, MERCS_NEXT );
			ShowEditorButtons( FIRST_MERCS_INVENTORY_BUTTON, LAST_MERCS_INVENTORY_BUTTON );
			EnableEditorRegion( MERC_REGION_ID );
			UnclickEditorButtons( FIRST_MERCS_PRIORITYMODE_BUTTON, LAST_MERCS_PRIORITYMODE_BUTTON );
			ClickEditorButton( MERCS_INVENTORY );
			break;
		case MERC_BASICMODE:
			ShowEditorButtons( FIRST_MERCS_GENERAL_BUTTON, LAST_MERCS_GENERAL_BUTTON );
			if( iDrawMode == DRAW_MODE_CREATURE )
			{ //Set up alternate general mode.  This one doesn't allow you to specify relative attributes
				//but requires you to specify a body type.
				HideEditorButtons( FIRST_MERCS_REL_EQUIPMENT_BUTTON, LAST_MERCS_REL_EQUIPMENT_BUTTON );
				ShowEditorButtons( FIRST_MERCS_BODYTYPE_BUTTON, LAST_MERCS_BODYTYPE_BUTTON );
			}
			if( iDrawMode != DRAW_MODE_ENEMY )
				HideEditorButtons( FIRST_MERCS_COLORCODE_BUTTON, LAST_MERCS_COLORCODE_BUTTON );
			if( iDrawMode == DRAW_MODE_CIVILIAN )
				ShowEditorButton( MERCS_CIVILIAN_GROUP );
			break;
		case MERC_GENERALMODE:
			ShowEditorButtons( FIRST_MERCS_GENERAL_BUTTON, LAST_MERCS_GENERAL_BUTTON );
			UnclickEditorButtons( FIRST_MERCS_PRIORITYMODE_BUTTON, LAST_MERCS_PRIORITYMODE_BUTTON );
			ClickEditorButton( MERCS_GENERAL );
			if( iDrawMode == DRAW_MODE_CREATURE )
			{ //Set up alternate general mode.  This one doesn't allow you to specify relative equipment
				//but requires you to specify a body type.
				HideEditorButtons( FIRST_MERCS_REL_EQUIPMENT_BUTTON, LAST_MERCS_REL_EQUIPMENT_BUTTON );
				ShowEditorButtons( FIRST_MERCS_BODYTYPE_BUTTON, LAST_MERCS_BODYTYPE_BUTTON );
				DisableEditorButton( MERCS_APPEARANCE );
			}
			if( iDrawMode != DRAW_MODE_ENEMY )
				HideEditorButtons( FIRST_MERCS_COLORCODE_BUTTON, LAST_MERCS_COLORCODE_BUTTON );
			if( iDrawMode == DRAW_MODE_CIVILIAN )
				ShowEditorButton( MERCS_CIVILIAN_GROUP );
			break;
		case MERC_ATTRIBUTEMODE:
			UnclickEditorButtons( FIRST_MERCS_PRIORITYMODE_BUTTON, LAST_MERCS_PRIORITYMODE_BUTTON );
			ClickEditorButton( MERCS_ATTRIBUTES );
			SetupTextInputForMercAttributes();
			break;
		case MERC_APPEARANCEMODE:
			ShowEditorButtons( FIRST_MERCS_COLORMODE_BUTTON, LAST_MERCS_COLORMODE_BUTTON );
			UnclickEditorButtons( FIRST_MERCS_PRIORITYMODE_BUTTON, LAST_MERCS_PRIORITYMODE_BUTTON );
			ClickEditorButton( MERCS_APPEARANCE );
			if( gfCanEditMercs && gpSelected && gpSelected->pDetailedPlacement )
			{
				if( !gpSelected->pDetailedPlacement->fVisible )
				{
					UnclickEditorButton( MERCS_TOGGLECOLOR_BUTTON );
					DisableEditorButtons( FIRST_MERCS_COLOR_BUTTON, LAST_MERCS_COLOR_BUTTON );
				}
				else
				{
					ClickEditorButton( MERCS_TOGGLECOLOR_BUTTON );
					EnableEditorButtons( FIRST_MERCS_COLOR_BUTTON, LAST_MERCS_COLOR_BUTTON );
				}
			}
			break;
		case MERC_PROFILEMODE:
			UnclickEditorButtons( FIRST_MERCS_PRIORITYMODE_BUTTON, LAST_MERCS_PRIORITYMODE_BUTTON );
			ClickEditorButton( MERCS_PROFILE );
			SetupTextInputForMercProfile();
			break;
		case MERC_SCHEDULEMODE:
			ShowEditorButtons( MERCS_SCHEDULE_ACTION1, MERCS_SCHEDULE_VARIANCE4 );
			ShowEditorButton( MERCS_SCHEDULE_CLEAR );
			UnclickEditorButtons( FIRST_MERCS_PRIORITYMODE_BUTTON, LAST_MERCS_PRIORITYMODE_BUTTON );
			ClickEditorButton( MERCS_SCHEDULE );
			SetupTextInputForMercSchedule();
			UpdateScheduleInfo();			
			DetermineScheduleEditability();
	}
	//Show or hide the schedule buttons
	if( gpSelected && gubCurrMercMode != MERC_GETITEMMODE )
	{
		if( gpSelected->pDetailedPlacement && gpSelected->pDetailedPlacement->ubScheduleID )
		{
			HideEditorButton( MERCS_SCHEDULE );
			ShowEditorButton( MERCS_GLOWSCHEDULE );
		}
		else
		{
			HideEditorButton( MERCS_GLOWSCHEDULE );
			if( gpSelected->pDetailedPlacement )
			{
				ShowEditorButton( MERCS_SCHEDULE );
				if( gpSelected->pSoldier->bTeam == CIV_TEAM )
					EnableEditorButton( MERCS_SCHEDULE );
				else
					DisableEditorButton( MERCS_SCHEDULE );
			}
			else
			{
				HideEditorButton( MERCS_SCHEDULE );
			}
		}
	}
	else
	{
		HideEditorButtons( MERCS_SCHEDULE, MERCS_GLOWSCHEDULE );
	}
}

void DisplayBodyTypeInfo()
{
	UINT16 str[20];
	switch( gpSelected->pBasicPlacement->bBodyType )
	{
		case RANDOM:							swprintf( str, L"Random" );					break;
		case REGMALE:							swprintf( str, L"Reg Male" );				break;
		case BIGMALE:							swprintf( str, L"Big Male" );				break;
		case STOCKYMALE:					swprintf( str, L"Stocky Male" );		break;
		case REGFEMALE:						swprintf( str, L"Reg Female" );			break;
		case TANK_NE:							swprintf( str, L"NE Tank" );				break;
		case TANK_NW:							swprintf( str, L"NW Tank" );				break;
		case FATCIV:							swprintf( str, L"Fat Civilian" );		break;
		case MANCIV:							swprintf( str, L"M Civilian" );			break;
		case MINICIV:							swprintf( str, L"Miniskirt" );			break;
		case DRESSCIV:						swprintf( str, L"F Civilian" );			break;
		case HATKIDCIV:						swprintf( str, L"Kid w/ Hat" );			break;
		case HUMVEE:							swprintf( str, L"Humvee" );					break;
		case ELDORADO:						swprintf( str, L"Eldorado" );				break;
		case ICECREAMTRUCK:				swprintf( str, L"Icecream Truck" );	break;
		case JEEP:								swprintf( str, L"Jeep" );						break;
		case KIDCIV:							swprintf( str, L"Kid Civilian" );		break;
		case COW:									swprintf( str, L"Domestic Cow" );		break;
		case CRIPPLECIV:					swprintf( str, L"Cripple" );				break;
		case ROBOTNOWEAPON:				swprintf( str, L"Unarmed Robot" );	break;
		case LARVAE_MONSTER:			swprintf( str, L"Larvae" );					break;
		case INFANT_MONSTER:			swprintf( str, L"Infant" );					break;
		case YAF_MONSTER:					swprintf( str, L"Yng F Monster" );	break;
		case YAM_MONSTER:					swprintf( str, L"Yng M Monster" );	break;
		case ADULTFEMALEMONSTER:	swprintf( str, L"Adt F Monster" );	break;
		case AM_MONSTER:					swprintf( str, L"Adt M Monster" );	break;
		case QUEENMONSTER:				swprintf( str, L"Queen Monster" );	break;
		case BLOODCAT:						swprintf( str, L"Bloodcat" );				break;
	}
	DrawEditorInfoBox( str, FONT10ARIAL, 490, 364, 70, 20 );
}

void UpdateMercsInfo()
{
	if( !gfRenderMercInfo )
		return;

	//We are rendering it now, so signify that it has been done, so
	//it doesn't get rendered every frame.
	gfRenderMercInfo = FALSE;

	switch( gubCurrMercMode )
	{
		case MERC_GETITEMMODE:
			RenderEditorItemsInfo();
			break;
		case MERC_INVENTORYMODE:
			if( gfMercGetItem )
				SetMercEditingMode( MERC_GETITEMMODE );
			else
				RenderMercInventoryPanel();
			break;
		case MERC_BASICMODE:
		case MERC_GENERALMODE:
			BltVideoObjectFromIndex( FRAME_BUFFER, guiExclamation, 0, 188, 362, VO_BLT_SRCTRANSPARENCY, NULL );
			BltVideoObjectFromIndex( FRAME_BUFFER, guiKeyImage, 0, 186, 387, VO_BLT_SRCTRANSPARENCY, NULL );
			SetFont( SMALLCOMPFONT );
			SetFontForeground( FONT_YELLOW );
			SetFontShadow( FONT_NEARBLACK );
			mprintf( 240, 363, L" --=ORDERS=-- ");
			mprintf( 240, 419, L"--=ATTITUDE=--");
			if( iDrawMode == DRAW_MODE_CREATURE )
			{
				DisplayBodyTypeInfo();
				SetFont( SMALLCOMPFONT );
				SetFontForeground( FONT_LTBLUE );
				mprintf( 493, 416, L"RELATIVE"); 
				mprintf( 480, 422, L"ATTRIBUTES"); 
			}
			else
			{
				SetFontForeground( FONT_LTGREEN );
				mprintf( 480, 363, L"RELATIVE"); 
				mprintf( 480, 371, L"EQUIPMENT"); 
				SetFontForeground( FONT_LTBLUE );
				mprintf( 530, 363, L"RELATIVE"); 
				mprintf( 530, 371, L"ATTRIBUTES"); 
			}
			if( iDrawMode == DRAW_MODE_ENEMY )
			{
				SetFont( FONT10ARIAL );
				SetFontForeground( FONT_YELLOW );
				mprintf( 590, 411, L"Army" );
				mprintf( 590, 425, L"Admin" );
				mprintf( 590, 439, L"Elite" );
			}
			break;
		case MERC_ATTRIBUTEMODE:
			SetFont( FONT10ARIAL );
			SetFontForeground( FONT_YELLOW );
			SetFontShadow( FONT_NEARBLACK );
			mprintf( 225, 370, L"Exp. Level");
			mprintf( 225, 395, L"Life");
			mprintf( 225, 420, L"LifeMax");
			mprintf( 225, 445, L"Marksmanship");
			mprintf( 325, 370, L"Strength");
			mprintf( 325, 395, L"Agility");
			mprintf( 325, 420, L"Dexterity");
			mprintf( 325, 445, L"Wisdom");
			mprintf( 425, 370, L"Leadership");
			mprintf( 425, 395, L"Explosives");
			mprintf( 425, 420, L"Medical");
			mprintf( 425, 445, L"Mechanical");
			mprintf( 525, 370, L"Morale");
			break;
		case MERC_APPEARANCEMODE:
			SetFont( FONT10ARIAL );
			if( gpSelected->pDetailedPlacement->fVisible || gpSelected->pDetailedPlacement->ubProfile != NO_PROFILE )
				SetFontForeground( FONT_YELLOW );
			else
				SetFontForeground( FONT_DKYELLOW );
			SetFontShadow( FONT_NEARBLACK );

			mprintf( 396, 364, L"Hair color:");
			mprintf( 396, 388, L"Skin color:");
			mprintf( 396, 412, L"Vest color:"); 
			mprintf( 396, 436, L"Pant color:");

			SetFont( SMALLCOMPFONT );
			SetFontForeground( FONT_BLACK );
			if( gpSelected->pDetailedPlacement->fVisible || gpSelected->pDetailedPlacement->ubProfile != NO_PROFILE )
			{
				mprintfEditor( 396, 374, L"%S    ", gpSelected->pSoldier->HeadPal );
				mprintfEditor( 396, 398, L"%S    ", gpSelected->pSoldier->SkinPal );
				mprintfEditor( 396, 422, L"%S    ", gpSelected->pSoldier->VestPal );
				mprintfEditor( 396, 446, L"%S    ", gpSelected->pSoldier->PantsPal );
				ShowEditMercPalettes( gpSelected->pSoldier );
			}
			else
			{
				mprintf( 396, 374, L"RANDOM");
				mprintf( 396, 398, L"RANDOM");
				mprintf( 396, 422, L"RANDOM");
				mprintf( 396, 446, L"RANDOM");
				ShowEditMercPalettes( NULL ); //will display grey scale to signify random
			}
			DisplayBodyTypeInfo();
			break;
		case MERC_PROFILEMODE:
			SetFont( FONT10ARIAL );
			SetFontForeground( FONT_YELLOW );
			SetFontShadow( FONT_NEARBLACK );
			{ //scope trick
				UINT16 tempStr[500];
				swprintf( tempStr, L"%s%s%s%s%s%d.",
					L"By specifying a profile index, all of the information will be extracted from the profile ",
					L"and override any values that you have edited.  It will also disable the editing features ", 
					L"though, you will still be able to view stats, etc.  Pressing ENTER will automatically ",
					L"extract the number you have typed.  A blank field will clear the profile.  The current ",
					L"number of profiles range from 0 to ", NUM_PROFILES );
				DisplayWrappedString(180, 370, 400, 2, FONT10ARIAL, 146, tempStr,	FONT_BLACK, FALSE, LEFT_JUSTIFIED );
				SetFont( FONT12POINT1 );
				if( gpSelected->pDetailedPlacement->ubProfile == NO_PROFILE )
				{
					SetFontForeground( FONT_GRAY3 );
					mprintfEditor( 240, 435, L"Current Profile:  n/a                            ");
				}
				else
				{
					SetFontForeground( FONT_WHITE );
					ClearTaskbarRegion( 240, 435, 580, 445 );
					mprintf( 240, 435, L"Current Profile:  %s", gMercProfiles[ gpSelected->pDetailedPlacement->ubProfile ].zName );
				}
			}
			break;
		case MERC_SCHEDULEMODE:
			SetFont( FONT10ARIAL );
			SetFontForeground( FONT_WHITE );
			SetFontShadow( FONT_NEARBLACK );
			switch( gpSelected->pSoldier->bOrders )
			{
				case STATIONARY:	mprintf( 430, 363, L"STATIONARY" );			break;
				case ONCALL:			mprintf( 430, 363, L"ON CALL" );				break;
				case ONGUARD:			mprintf( 430, 363, L"ON GUARD" );				break;
				case SEEKENEMY:		mprintf( 430, 363, L"SEEK ENEMY" );			break;
				case CLOSEPATROL:	mprintf( 430, 363, L"CLOSE PATROL" );		break;
				case FARPATROL:		mprintf( 430, 363, L"FAR PATROL" );			break;
				case POINTPATROL:	mprintf( 430, 363, L"POINT PATROL" );		break;
				case RNDPTPATROL:	mprintf( 430, 363, L"RND PT PATROL" );	break;
			}
			SetFontForeground( FONT_YELLOW );
			mprintf( 186, 363, L"Action" );
			mprintf( 268, 363, L"Time" );
			mprintf( 309, 363, L"V" );
			mprintf( 331, 363, L"GridNo 1" );
			mprintf( 381, 363, L"GridNo 2" );
			mprintf( 172, 376, L"1)" );
			mprintf( 172, 397, L"2)" );
			mprintf( 172, 418, L"3)" );
			mprintf( 172, 439, L"4)" );
			if( gubScheduleInstructions )
			{
				UINT16 str[255];
				UINT16 keyword[10] = L"";
				ColorFillVideoSurfaceArea( FRAME_BUFFER, 431, 388, 590, 450, Get16BPPColor( FROMRGB( 32, 45, 72 ) ) );
				switch( gCurrSchedule.ubAction[ gubCurrentScheduleActionIndex ] )
				{
					case SCHEDULE_ACTION_LOCKDOOR:			swprintf( keyword, L"lock" );			break;
					case SCHEDULE_ACTION_UNLOCKDOOR:		swprintf( keyword, L"unlock" );		break;
					case SCHEDULE_ACTION_OPENDOOR:			swprintf( keyword, L"open" );			break;
					case SCHEDULE_ACTION_CLOSEDOOR:			swprintf( keyword, L"close" );		break;
				}
				switch( gubScheduleInstructions )
				{
					case SCHEDULE_INSTRUCTIONS_DOOR1:
						swprintf( str, L"Click on the gridno adjacent to the door that you wish to %s.", keyword );
						break;
					case SCHEDULE_INSTRUCTIONS_DOOR2:
						swprintf( str, L"Click on the gridno where you wish to move after you %s the door.", keyword );
						break;
					case SCHEDULE_INSTRUCTIONS_GRIDNO:
						swprintf( str, L"Click on the gridno where you wish to move to." );
						break;
					case SCHEDULE_INSTRUCTIONS_SLEEP:
						swprintf( str, L"Click on the gridno where you wish to sleep at.  Person will automatically return to original position after waking up." );
					default:
						return;
				}
				wcscat( str, L"  Hit ESC to abort entering this line in the schedule." );
				DisplayWrappedString( 436, 392, 149, 2, FONT10ARIAL, FONT_YELLOW, str, FONT_BLACK, FALSE, LEFT_JUSTIFIED );
			}
			break;
	}
}

//When a detailed placement merc is in the inventory panel, there is a overall region
//blanketing this panel.  As the user moves the mouse around and clicks, etc., this function 
//is called by the region callback functions to handle these cases.  The event types are defined
//in Editor Taskbar Utils.h.  Here are the internal functions...

SGPRect mercRects[9] = 
{
	{  75,  0, 104, 19 }, //head
	{  75, 22, 104, 41 }, //body
	{  76, 73, 105, 92 }, //legs
	{  26, 43,  78, 62 }, //left hand
	{ 104, 42, 156, 61 }, //right hand
	{ 180,  6, 232, 25 }, //pack 1
	{ 180, 29, 232, 48 }, //pack 2
	{ 180, 52, 232, 71 }, //pack 3
	{ 180, 75, 232, 94 }  //pack 4
};


BOOLEAN PointInRect( SGPRect *pRect, INT32 x, INT32 y )
{
	return( x >= pRect->iLeft && x <= pRect->iRight && y >= pRect->iTop && y <= pRect->iBottom );
}

void DrawRect( SGPRect *pRect, INT16 color )
{
	UINT32	uiDestPitchBYTES;
	UINT8		*pDestBuf;
	pDestBuf = LockVideoSurface( FRAME_BUFFER, &uiDestPitchBYTES );
	SetClippingRegionAndImageWidth( uiDestPitchBYTES, 0, 0, 640, 480 );
	RectangleDraw( TRUE, pRect->iLeft+MERCPANEL_X, pRect->iTop+MERCPANEL_Y, pRect->iRight+MERCPANEL_X, pRect->iBottom+MERCPANEL_Y, color, pDestBuf );
	UnLockVideoSurface( FRAME_BUFFER );
	//InvalidateRegion( pRect->iLeft+175, pRect->iTop+361, pRect->iRight+176, pRect->iBottom+362 );
}

void RenderSelectedMercsInventory()
{
	INT32 i;
	UINT8 *pSrc, *pDst;
	INT32 xp, yp;
	UINT32 uiSrcPitchBYTES, uiDstPitchBYTES;
	UINT16 pItemName[100];
	UINT8 ubFontColor;
	if( gsSelectedMercID == -1 )
		return;
	for( i = 0; i < 9; i++ )
	{
		if( gpMercSlotItem[i] )
		{ //Render the current image.
			xp = mercRects[ i ].iLeft + 4 + MERCPANEL_X;
			yp = mercRects[ i ].iTop + MERCPANEL_Y;
			pDst = LockVideoSurface( FRAME_BUFFER,							&uiDstPitchBYTES );
			pSrc = LockVideoSurface( guiMercInvPanelBuffers[i], &uiSrcPitchBYTES );
			Blt16BPPTo16BPPTrans( (UINT16*)pDst, uiDstPitchBYTES, 
				(UINT16 *)pSrc, uiSrcPitchBYTES, xp, yp, 0, 0, i<3 ? 22 : 44, 15, 0 );
			UnLockVideoSurface( FRAME_BUFFER );
			UnLockVideoSurface( guiMercInvPanelBuffers[i] );
			LoadItemInfo( gpMercSlotItem[i]->usItem, pItemName, NULL );
			//Render the text
			switch( i )
			{
				case 2: //legs (to the right of the box, but move it down to make room for right hand text)
					xp = mercRects[i].iRight + 2;
					yp = mercRects[i].iTop + 8;
					break;
				case 3: //left hand (underneath box and to the left -- obscurred by checkbox)
					xp = mercRects[i].iLeft - 20;
					yp = mercRects[i].iBottom + 2;
					break;
				case 4: //right hand (underneath box)
					xp = mercRects[i].iLeft;
					yp = mercRects[i].iBottom + 2;
					break;
				default: //normal cases (to the right of the box)
					xp = mercRects[i].iRight + 2;
					yp = mercRects[i].iTop;
					break;
			}
			xp += MERCPANEL_X;
			yp += MERCPANEL_Y;
			SetFont( SMALLCOMPFONT );
			if( i == gbCurrSelect )
				ubFontColor = FONT_LTRED;
			else if( i == gbCurrHilite )
				ubFontColor = FONT_YELLOW;
			else
				ubFontColor = FONT_WHITE;
			DisplayWrappedString( (UINT16)xp, (UINT16)yp, 60, 2, SMALLCOMPFONT, ubFontColor, pItemName, 0, FALSE, LEFT_JUSTIFIED );
		}
	}
}

void DeleteSelectedMercsItem()
{
	if( gbCurrSelect != -1 )
	{
		gusMercsNewItemIndex = 0;
		AddNewItemToSelectedMercsInventory( TRUE );
	}
}

//This function does two main things:
// 1)  Allows a new item to be created via usItem and assigned to the currently selected merc.
// 2)  Converts the image from interface size to the smaller panel used by the editor.  The slots
//		 in the editor are approximately 80% of that size.  This involves scaling calculations.  These
//		 images are saved in individual slots are are blitted to the screen during rendering, not here.
// NOTE:  Step one can be skipped (when selecting an existing merc).  By setting the 
void AddNewItemToSelectedMercsInventory( BOOLEAN fCreate )
{
	UINT32 uiVideoObjectIndex;
	UINT32 uiSrcID, uiDstID;
	HVOBJECT hVObject;
	ETRLEObject *pObject;
	INVTYPE *item;
	SGPRect	SrcRect, DstRect;
	INT32 iSrcWidth, iSrcHeight;
	INT32 iDstWidth, iDstHeight;
	float rScalar, rWidthScalar, rHeightScalar;
	BOOLEAN fUnDroppable;

	if( fCreate )
	{
		/*
		if( gpMercSlotItem[ gbCurrSelect ] && gpMercSlotItem[ gbCurrSelect ]->usItem == gusMercsNewItemIndex )
		{ //User selected same item, so ignore.
			gusMercsNewItemIndex = 0xffff;
			return;
		}
		*/
		if( gusMercsNewItemIndex == 0xffff )
		{ //User selected no item, so ignore.
			return;
		}
		//Create the item, and set up the slot.
		fUnDroppable = gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ].fFlags & OBJECT_UNDROPPABLE ? TRUE : FALSE;

		if ( Item[ gusMercsNewItemIndex ].usItemClass == IC_KEY )
		{
			CreateKeyObject( &gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ], 1, (UINT8) eInfo.sSelItemIndex );
		}
		else
		{
			CreateItem( gusMercsNewItemIndex, 100, &gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ] );
		}
		if( fUnDroppable )
		{
			gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ].fFlags |= OBJECT_UNDROPPABLE;
		}

		//randomize the status on non-ammo items.
		if( !(Item[ gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ].usItem ].usItemClass & IC_AMMO) )
			gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ].bStatus[0] = (INT8)(80 + Random( 21 ));

		if( gusMercsNewItemIndex )
		{
			gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ].fFlags |= OBJECT_NO_OVERWRITE;
		}
	}
	//allow the slot to point to the selected merc's inventory for editing/rendering purposes.
	gpMercSlotItem[ gbCurrSelect ] = &gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ gbCurrSelect ] ];

	if( !fCreate )
	{ //it is possible to have a null item which we don't want to blit!  Also, we need to set the
		//new item index, so that it can extract the item's image using that.
		gusMercsNewItemIndex = gpMercSlotItem[ gbCurrSelect ]->usItem;
		if( !gpMercSlotItem[ gbCurrSelect ] )
			return;
	}
	//GOAL:
	//From here on, we are going to first render the new item into a temp buffer, then crop the image in
	//the buffer and scale it down to fit into it's associated slot in the panel (which on average will
	//require scaling the item by 80%).  We have to do a bunch of calculations to override the offsets, etc.
	//Each slot has it's own smaller version buffer, and this is what gets drawn when the rendering happens.
	
	//assign the buffers
	uiSrcID = guiMercTempBuffer;
	uiDstID = guiMercInvPanelBuffers[ gbCurrSelect ];

	//build the rects
	iDstWidth = gbCurrSelect < 3 ? MERCINV_SMSLOT_WIDTH : MERCINV_LGSLOT_WIDTH;
	iDstHeight = MERCINV_SLOT_HEIGHT;
	SrcRect.iLeft = 0;
	SrcRect.iTop = 0;
	SrcRect.iRight = 60;
	SrcRect.iBottom = 25;
	DstRect.iLeft = 0;
	DstRect.iTop = 0;
	DstRect.iRight = iDstWidth;
	DstRect.iBottom = iDstHeight;

	//clear both buffers (fill with black to erase previous graphic)
	ColorFillVideoSurfaceArea( uiSrcID, SrcRect.iLeft, SrcRect.iTop, SrcRect.iRight, SrcRect.iBottom, 0 );
	ColorFillVideoSurfaceArea( uiDstID, DstRect.iLeft, DstRect.iTop, DstRect.iRight, DstRect.iBottom, 0 );

	//if the index is 0, then there is no item.
	if( !gusMercsNewItemIndex )
		return;

	//now draw the fullsize item into the temp buffer
	item = &Item[ gusMercsNewItemIndex ];
	uiVideoObjectIndex = GetInterfaceGraphicForItem( item );
	GetVideoObject( &hVObject, uiVideoObjectIndex );
	BltVideoObjectOutlineFromIndex( uiSrcID, uiVideoObjectIndex, item->ubGraphicNum, 0, 0, 0, FALSE );

	//crop the source image
	pObject = &hVObject->pETRLEObject[ item->ubGraphicNum ];
	iSrcWidth				=		pObject->usWidth;
	iSrcHeight			=		pObject->usHeight;
	SrcRect.iLeft   +=	pObject->sOffsetX;
	SrcRect.iRight  =		SrcRect.iLeft + iSrcWidth;
	SrcRect.iTop    +=	pObject->sOffsetY;
	SrcRect.iBottom =		SrcRect.iTop + iSrcHeight;

	//if the source image width is less than 30 (small slot), then modify the DstRect.
	if( iSrcWidth < 30 )
		iDstWidth = MERCINV_SMSLOT_WIDTH;
	else
		iDstWidth = MERCINV_LGSLOT_WIDTH;

	//compare the sizes of the cropped image to the destination buffer size, and calculate the
	//scalar value.  It is possible to have scalars > 1.0, in which case, we change it to 1.0 and
	//use the other value.  
	rWidthScalar =  (float)iDstWidth/(float)iSrcWidth;
	if( rWidthScalar > 1.0 )
		rWidthScalar = 1.0;
	rHeightScalar = (float)iDstHeight/(float)iSrcHeight;
	if( rHeightScalar > 1.0 )
		rHeightScalar = 1.0;

	//determine which scalar to use.
	if( rWidthScalar == 1.0 )
		rScalar = rHeightScalar;
	else if( rHeightScalar == 1.0 )
		rScalar = rWidthScalar ;
	else
		rScalar = max( rWidthScalar, rHeightScalar );
	
	//apply the scalar to the destination width and height
	iDstWidth = (INT32)( iSrcWidth * rScalar );
	iDstHeight = (INT32)( iSrcHeight * rScalar );

	//sometimes it is possible to scale too big, so clip if applicable
	if( iDstWidth > MERCINV_LGSLOT_WIDTH )
		iDstWidth = MERCINV_LGSLOT_WIDTH;
	else if( gbCurrSelect < 3 && iDstWidth > MERCINV_SMSLOT_WIDTH )
		iDstWidth = MERCINV_SMSLOT_WIDTH;
	if( iDstHeight > MERCINV_SLOT_HEIGHT )
		iDstHeight = MERCINV_SLOT_HEIGHT;
	
	//use the new width and height values to calculate the new dest rect (center the item)
	DstRect.iLeft		= (DstRect.iRight - DstRect.iLeft - iDstWidth) / 2;
	DstRect.iRight	= DstRect.iLeft + iDstWidth;
	DstRect.iTop		= (DstRect.iBottom - DstRect.iTop - iDstHeight) / 2;
	DstRect.iBottom = DstRect.iTop + iDstHeight;

	//scale the item down to the smaller buffer.
	BltStretchVideoSurface( uiDstID, uiSrcID, 0, 0, VO_BLT_SRCTRANSPARENCY, &SrcRect, &DstRect );

	//invalidate the mercs new item index
	gusMercsNewItemIndex = 0xffff;
}

void RenderMercInventoryPanel()
{
	INT32 x;
	//Draw the graphical panel
	BltVideoObjectFromIndex( FRAME_BUFFER, guiMercInventoryPanel, 0, MERCPANEL_X, MERCPANEL_Y, VO_BLT_SRCTRANSPARENCY, NULL );
	//Mark the buttons dirty, so they don't disappear.
	for( x = FIRST_MERCS_INVENTORY_BUTTON; x <= LAST_MERCS_INVENTORY_BUTTON; x++ )
	{
		MarkAButtonDirty( iEditorButton[ x ] );
	}
	RenderButtons();
	if( gbCurrHilite != -1 )
		DrawRect( &mercRects[ gbCurrHilite ], Get16BPPColor( FROMRGB( 200, 200, 0 ) ) );
	if( gbCurrSelect != -1 )
		DrawRect( &mercRects[ gbCurrSelect ], Get16BPPColor( FROMRGB( 200,   0, 0 ) ) );
	RenderSelectedMercsInventory();
	InvalidateRegion( MERCPANEL_X, MERCPANEL_Y, 475, 460 );
	UpdateItemStatsPanel();
}


//This function is called by the move and click callback functions for the region blanketing the
//9 slots in the inventory panel.  It passes the event type as well as the relative x and y positions
//which are processed here.  This basically checks for new changes in hilighting and selections, which
//will set the rendering flag, and getitem flag if the user wishes to choose an item.
void HandleMercInventoryPanel( INT16 sX, INT16 sY, INT8 bEvent )
{
	INT8 x;
	if( !gfCanEditMercs && bEvent == GUI_RCLICK_EVENT )
	{ //if we are dealing with a profile merc, we can't allow editing
		//of items, but we can look at them.  So, treat all right clicks 
		//as if they were left clicks.
		bEvent = GUI_LCLICK_EVENT;
	}
	switch( bEvent )
	{
		case GUI_MOVE_EVENT:
			//user is moving the mouse around the panel, so determine which slot
			//needs to be hilighted yellow.
			for( x = 0; x < 9; x++ )
			{
				if( PointInRect( &mercRects[x], sX, sY ) )
				{
					if( gbCurrHilite != x ) //only render if the slot isn't the same one.
						gfRenderMercInfo = TRUE;
					gbCurrHilite = x;
					return;
				}
			}
			//if we don't find a valid slot, then we need to turn it off.
			if( gbCurrHilite != -1 ) 
			{ //only turn off if it isn't already off.  This avoids unecessary rendering.
				gbCurrHilite = -1;
				gfRenderMercInfo = TRUE;
			}
			break;
		case GUI_LCLICK_EVENT:
		case GUI_RCLICK_EVENT:
			//The user has clicked in the inventory panel.  Determine if he clicked in
			//a slot.  Left click selects the slot for editing, right clicking enables
			//the user to choose an item for that slot.
			for( x = 0; x < 9; x++ )
			{
				if( PointInRect( &mercRects[x], sX, sY ) )
				{
					if( gbCurrSelect != x ) //only if it isn't the same slot.
					{
						gfRenderMercInfo = TRUE;
						if( bEvent == GUI_LCLICK_EVENT )
							SpecifyItemToEdit( gpMercSlotItem[ x ], -1 );
					}
					if( bEvent == GUI_RCLICK_EVENT ) //user r-clicked, so enable item choosing
						gfMercGetItem = TRUE;
					gbCurrSelect = x;
					return;
				}
			}
			break;
	}
}

void UpdateMercItemSlots()
{
	INT8 x;
	if( !gpSelected->pDetailedPlacement )
	{
		for( x = 0; x < 9; x++ )
		{
			gpMercSlotItem[ x ] = NULL;
		}
	}
	else 
	{
		if( gpSelected->pDetailedPlacement->ubProfile != NO_PROFILE )
		{
			memcpy( gpSelected->pDetailedPlacement->Inv, gpSelected->pSoldier->inv, sizeof( OBJECTTYPE ) * NUM_INV_SLOTS );
		}
		for( x = 0; x < 9; x++ )
		{
			//Set the curr select and the addnewitem function will handle the rest, including rebuilding
			//the nine slot buffers, etc.
			gbCurrSelect = x; 
			AddNewItemToSelectedMercsInventory( FALSE );
		}
	}
	SetDroppableCheckboxesBasedOnMercsInventory();
	SpecifyItemToEdit( NULL, -1 );
	gbCurrSelect = -1;
	gbCurrHilite = -1;
}

void SetDroppableCheckboxesBasedOnMercsInventory()
{
	OBJECTTYPE *pItem;
	INT32 i;
	if( gpSelected && gpSelected->pDetailedPlacement )
	{
		for( i = 0; i < 9; i++ )
		{
			pItem = &gpSelected->pDetailedPlacement->Inv[ gbMercSlotTypes[ i ] ];
			if( pItem->fFlags & OBJECT_UNDROPPABLE )
			{	//check box is clear
				UnclickEditorButton( MERCS_HEAD_SLOT + i );
			}
			else
			{
				ClickEditorButton( MERCS_HEAD_SLOT + i );
			}
		}
	}
}

void SetEnemyColorCode( UINT8 ubColorCode )
{
	if( gpSelected->pDetailedPlacement && gpSelected->pDetailedPlacement->ubProfile != NO_PROFILE )
		return;

	UnclickEditorButtons( FIRST_MERCS_COLORCODE_BUTTON, LAST_MERCS_COLORCODE_BUTTON );
	switch( ubColorCode )
	{
		case SOLDIER_CLASS_ARMY:
			gpSelected->pBasicPlacement->ubSoldierClass = SOLDIER_CLASS_ARMY;
			gubSoldierClass = SOLDIER_CLASS_ARMY;
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->ubSoldierClass = SOLDIER_CLASS_ARMY;
			SET_PALETTEREP_ID( gpSelected->pSoldier->VestPal, "REDVEST"  );
			SET_PALETTEREP_ID( gpSelected->pSoldier->PantsPal, "GREENPANTS"   );
			ClickEditorButton( MERCS_ARMY_CODE );
			break;
		case SOLDIER_CLASS_ADMINISTRATOR:
			gpSelected->pBasicPlacement->ubSoldierClass = SOLDIER_CLASS_ADMINISTRATOR;
			gubSoldierClass = SOLDIER_CLASS_ADMINISTRATOR;
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->ubSoldierClass = SOLDIER_CLASS_ADMINISTRATOR;
			ClickEditorButton( MERCS_ADMIN_CODE );
			SET_PALETTEREP_ID( gpSelected->pSoldier->VestPal, "BLUEVEST"  );
			SET_PALETTEREP_ID( gpSelected->pSoldier->PantsPal, "BLUEPANTS"   );
			break;
		case SOLDIER_CLASS_ELITE:
			gpSelected->pBasicPlacement->ubSoldierClass = SOLDIER_CLASS_ELITE;
			gubSoldierClass = SOLDIER_CLASS_ELITE;
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->ubSoldierClass = SOLDIER_CLASS_ELITE;
			SET_PALETTEREP_ID( gpSelected->pSoldier->VestPal, "BLACKSHIRT"  );
			SET_PALETTEREP_ID( gpSelected->pSoldier->PantsPal, "BLACKPANTS"   );
			ClickEditorButton( MERCS_ELITE_CODE );
			break;
		case SOLDIER_CLASS_MINER:
			//will probably never get called
			gpSelected->pBasicPlacement->ubSoldierClass = SOLDIER_CLASS_ELITE;
			gubSoldierClass = SOLDIER_CLASS_MINER;
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->ubSoldierClass = SOLDIER_CLASS_ELITE;
			SET_PALETTEREP_ID( gpSelected->pSoldier->VestPal, "greyVEST"  );
			SET_PALETTEREP_ID( gpSelected->pSoldier->PantsPal, "BEIGEPANTS"   );
			break;

		default:
			return;
	}
	CreateSoldierPalettes( gpSelected->pSoldier );
}

void SetEnemyDroppableStatus( UINT32 uiSlot, BOOLEAN fDroppable )
{
	if( gpSelected ) 
	{
		if( fDroppable )
		{
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->Inv[ uiSlot ].fFlags &= (~OBJECT_UNDROPPABLE);
			if( gpSelected->pSoldier )
				gpSelected->pSoldier->inv[ uiSlot ].fFlags &= (~OBJECT_UNDROPPABLE);
		}
		else
		{
			if( gpSelected->pDetailedPlacement )
				gpSelected->pDetailedPlacement->Inv[ uiSlot ].fFlags |= OBJECT_UNDROPPABLE;
			if( gpSelected->pSoldier )
				gpSelected->pSoldier->inv[ uiSlot ].fFlags |= OBJECT_UNDROPPABLE;
		}
	}
	if( gbCurrSelect != -1 && uiSlot == (UINT32)gbMercSlotTypes[ gbCurrSelect ] )
	{
		if( gpMercSlotItem[ gbCurrSelect ]->usItem == NOTHING )
			SpecifyItemToEdit( gpMercSlotItem[ gbCurrSelect ], -1 );
	}
}

void ChangeCivGroup( UINT8 ubNewCivGroup )
{
	Assert( ubNewCivGroup < NUM_CIV_GROUPS );
	if( gubCivGroup == ubNewCivGroup )
		return;
	gubCivGroup = ubNewCivGroup;
	if( gpSelected && gpSelected->pSoldier )
	{
		gpSelected->pBasicPlacement->ubCivilianGroup = gubCivGroup;
		if( gpSelected->pDetailedPlacement )
			gpSelected->pDetailedPlacement->ubCivilianGroup = gubCivGroup;
		gpSelected->pSoldier->ubCivilianGroup = gubCivGroup;
	}
	//Adjust the text on the button
	SpecifyButtonText( iEditorButton[ MERCS_CIVILIAN_GROUP ], gszCivGroupNames[ gubCivGroup ] );
}

void RenderMercStrings()
{
	SOLDIERTYPE								 *pSoldier;
	INT16 sXPos, sYPos;
	INT16 sX, sY;
	UINT16 *pStr;
	SOLDIERINITNODE *curr;
	UINT16 str[50];

	curr = gSoldierInitHead;
	while( curr )
	{
		if( curr->pSoldier && curr->pSoldier->bVisible == 1 )
		{ //Render the health text
			pSoldier = curr->pSoldier;
			GetSoldierAboveGuyPositions( pSoldier, &sXPos, &sYPos, FALSE );
			// Display name
			SetFont( TINYFONT1 );
			SetFontBackground( FONT_BLACK );
			SetFontForeground( FONT_WHITE );
			if ( pSoldier->ubProfile != NO_PROFILE )
			{
				FindFontCenterCoordinates( sXPos, sYPos, (INT16)(80 ), 1, pSoldier->name, TINYFONT1, &sX, &sY );
				if( sY < 352 )
				{
					gprintfdirty( sX, sY, pSoldier->name );
					mprintf( sX, sY, pSoldier->name );
				}
				sYPos += 10;

				pStr = GetSoldierHealthString( pSoldier );

				SetFont( TINYFONT1 );
				SetFontBackground( FONT_BLACK  );
				SetFontForeground( FONT_RED );

				FindFontCenterCoordinates( sXPos, sYPos, 80, 1, pStr, TINYFONT1, &sX, &sY );
				if( sY < 352 )
				{
					gprintfdirty( sX, sY, pStr );
					mprintf( sX, sY, pStr );
				}
				sYPos += 10;

				SetFontForeground( FONT_GRAY2 );
				swprintf( str, L"Slot #%d", pSoldier->ubID );
				FindFontCenterCoordinates( sXPos, sYPos, 80, 1, str, TINYFONT1, &sX, &sY );
				if( sY < 352 )
				{
					gprintfdirty( sX, sY, str );
					mprintf( sX, sY, str );
				}
				sYPos += 10;
			}
			else
			{
				pStr = GetSoldierHealthString( pSoldier );

				SetFont( TINYFONT1 );
				SetFontBackground( FONT_BLACK );
				SetFontForeground( FONT_RED );

				FindFontCenterCoordinates( sXPos, sYPos, 80, 1, pStr, TINYFONT1, &sX, &sY );
				if( sY < 352 )
				{
					gprintfdirty( sX, sY, pStr );
					mprintf( sX, sY, pStr );
				}
				sYPos += 10;

				SetFontForeground( FONT_GRAY2 );
				swprintf( str, L"Slot #%d", pSoldier->ubID );
				FindFontCenterCoordinates( sXPos, sYPos, 80, 1, str, TINYFONT1, &sX, &sY );
				if( sY < 352 )
				{
					gprintfdirty( sX, sY, str );
					mprintf( sX, sY, str );
				}
				sYPos += 10;
			}
			if( curr->pBasicPlacement->bOrders == RNDPTPATROL || curr->pBasicPlacement->bOrders == POINTPATROL )
			{ //make sure the placement has at least one waypoint.
				if( !curr->pBasicPlacement->bPatrolCnt )
				{
					if( GetJA2Clock() % 1000 < 500 )
						SetFontForeground( FONT_DKRED );
					else 
						SetFontForeground( FONT_RED );
					swprintf( str, L"Patrol orders with no waypoints" );
					FindFontCenterCoordinates( sXPos, sYPos, 80, 1, str, TINYFONT1, &sX, &sY );
					if( sY < 352 )
					{
						gprintfdirty( sX, sY, str );
						mprintf( sX, sY, str );
					}
					sYPos += 10;
				}
			}
			else if( curr->pBasicPlacement->bPatrolCnt )
			{
				if( GetJA2Clock() % 1000 < 500 )
					SetFontForeground( FONT_DKRED );
				else 
					SetFontForeground( FONT_RED );
				swprintf( str, L"Waypoints with no patrol orders" );
				FindFontCenterCoordinates( sXPos, sYPos, 80, 1, str, TINYFONT1, &sX, &sY );
				if( sY < 352 )
				{
					gprintfdirty( sX, sY, str );
					mprintf( sX, sY, str );
				}
				sYPos += 10;
			}
		}
		curr = curr->next;
	}
	if( gubCurrMercMode == MERC_SCHEDULEMODE )
	{
		RenderCurrentSchedule();
	}
}

void SetMercTeamVisibility( INT8 bTeam, BOOLEAN fVisible )
{
	SOLDIERINITNODE *curr;
	INT8 bVisible;
	curr = gSoldierInitHead;
	bVisible = fVisible ? 1 : -1;
	while( curr ) 
	{
		if( curr->pBasicPlacement->bTeam == bTeam )
		{
			if( curr->pSoldier )
			{
				curr->pSoldier->bLastRenderVisibleValue = bVisible;
				curr->pSoldier->bVisible = bVisible;
			}
		}
		curr = curr->next;
	}
	if( gpSelected && gpSelected->pSoldier && gpSelected->pSoldier->bTeam == bTeam && !fVisible )
	{
		IndicateSelectedMerc( SELECT_NO_MERC );
	}
}

void DetermineScheduleEditability()
{
	INT32 i;
	EnableEditorButtons( MERCS_SCHEDULE_ACTION1, MERCS_SCHEDULE_DATA4B );
	EnableTextFields( 1, 4 );
	for( i = 0; i < 4; i++ )
	{
		switch( gCurrSchedule.ubAction[i] )
		{
			case SCHEDULE_ACTION_NONE:
			case SCHEDULE_ACTION_LEAVESECTOR:
				EnableEditorButton( MERCS_SCHEDULE_ACTION1 + i );
				EnableEditorButton( MERCS_SCHEDULE_VARIANCE1 + i );
				HideEditorButton( MERCS_SCHEDULE_DATA1A + i );
				HideEditorButton( MERCS_SCHEDULE_DATA1B + i );
				break;
			case SCHEDULE_ACTION_LOCKDOOR:
			case SCHEDULE_ACTION_UNLOCKDOOR:
			case SCHEDULE_ACTION_OPENDOOR:
			case SCHEDULE_ACTION_CLOSEDOOR:
				EnableEditorButton( MERCS_SCHEDULE_ACTION1 + i );
				EnableEditorButton( MERCS_SCHEDULE_VARIANCE1 + i );
				ShowEditorButton( MERCS_SCHEDULE_DATA1A + i );
				ShowEditorButton( MERCS_SCHEDULE_DATA1B + i );
				EnableEditorButton( MERCS_SCHEDULE_DATA1A + i );
				EnableEditorButton( MERCS_SCHEDULE_DATA1B + i );
				break;
			case SCHEDULE_ACTION_GRIDNO:
			case SCHEDULE_ACTION_ENTERSECTOR:
			case SCHEDULE_ACTION_SLEEP:
				EnableEditorButton( MERCS_SCHEDULE_ACTION1 + i );
				EnableEditorButton( MERCS_SCHEDULE_VARIANCE1 + i );
				ShowEditorButton( MERCS_SCHEDULE_DATA1A + i );
				HideEditorButton( MERCS_SCHEDULE_DATA1B + i );
				EnableEditorButton( MERCS_SCHEDULE_DATA1A + i );
				break;
			case SCHEDULE_ACTION_STAYINSECTOR:
				DisableTextField( (UINT8)(i+1) );
				EnableEditorButton( MERCS_SCHEDULE_ACTION1 + i );
				DisableEditorButton( MERCS_SCHEDULE_VARIANCE1 + i );
				HideEditorButton( MERCS_SCHEDULE_DATA1A + i );
				HideEditorButton( MERCS_SCHEDULE_DATA1B + i );
				break;
		}
	}
}

void CancelCurrentScheduleAction()
{
	UpdateScheduleAction( SCHEDULE_ACTION_NONE );
	DetermineScheduleEditability();
}

void RegisterCurrentScheduleAction( INT32 iMapIndex )
{
	UINT16 str[6];
	MarkWorldDirty();
	swprintf( str, L"%d", iMapIndex );
	if( gfUseScheduleData2 )
	{
		if( gfSingleAction )
			return;
		iDrawMode = DRAW_MODE_PLAYER + gpSelected->pBasicPlacement->bTeam;
		gCurrSchedule.usData2[ gubCurrentScheduleActionIndex ] = (UINT16)iMapIndex;
		SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_DATA1B + gubCurrentScheduleActionIndex ], str );
		DetermineScheduleEditability();
		gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_NONE;
		gfRenderTaskbar = TRUE;
		gfUseScheduleData2 = FALSE;
	}
	else
	{
		switch( gCurrSchedule.ubAction[ gubCurrentScheduleActionIndex ] )
		{
			case SCHEDULE_ACTION_LOCKDOOR:
			case SCHEDULE_ACTION_UNLOCKDOOR:
			case SCHEDULE_ACTION_OPENDOOR:
			case SCHEDULE_ACTION_CLOSEDOOR:
				if( gfSingleAction )
				{
					gfSingleAction = FALSE;
					gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_NONE;
					gfRenderTaskbar = TRUE;
					DetermineScheduleEditability();
					break;
				}
				DisableEditorButton( MERCS_SCHEDULE_DATA1A + gubCurrentScheduleActionIndex );
				EnableEditorButton( MERCS_SCHEDULE_DATA1B + gubCurrentScheduleActionIndex );
				gfUseScheduleData2 = TRUE;
				iDrawMode = DRAW_MODE_SCHEDULEACTION;
				gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_DOOR2;
				gfRenderTaskbar = TRUE;
				break;
			case SCHEDULE_ACTION_GRIDNO:
			case SCHEDULE_ACTION_ENTERSECTOR:
			case SCHEDULE_ACTION_SLEEP:
				iDrawMode = DRAW_MODE_PLAYER + gpSelected->pBasicPlacement->bTeam;
				DetermineScheduleEditability();
				gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_NONE;
				gfRenderTaskbar = TRUE;
				break;
			case SCHEDULE_ACTION_LEAVESECTOR:
			case SCHEDULE_ACTION_STAYINSECTOR:
			case SCHEDULE_ACTION_NONE:
				break;
		}
		gCurrSchedule.usData1[ gubCurrentScheduleActionIndex ] = (UINT16)iMapIndex;
		SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_DATA1A + gubCurrentScheduleActionIndex ], str );
	}
}

void StartScheduleAction()
{
	switch( gCurrSchedule.ubAction[ gubCurrentScheduleActionIndex ] )
	{
		case SCHEDULE_ACTION_NONE:
			EnableEditorButtons( MERCS_SCHEDULE_ACTION1, MERCS_SCHEDULE_DATA4B );
			EnableTextFields( 1, 4 );
			gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_NONE;
			gfRenderTaskbar = TRUE;
			gCurrSchedule.usData1[ gubCurrentScheduleActionIndex ] = 0xffff;
			gCurrSchedule.usData2[ gubCurrentScheduleActionIndex ] = 0xffff;
			break;
		case SCHEDULE_ACTION_LOCKDOOR:
		case SCHEDULE_ACTION_UNLOCKDOOR:
		case SCHEDULE_ACTION_OPENDOOR:
		case SCHEDULE_ACTION_CLOSEDOOR:
			//First disable everything -- its easier that way.
			ShowEditorButton( MERCS_SCHEDULE_DATA1A + gubCurrentScheduleActionIndex );
			ShowEditorButton( MERCS_SCHEDULE_DATA1B + gubCurrentScheduleActionIndex );
			DisableEditorButtons( MERCS_SCHEDULE_ACTION1, MERCS_SCHEDULE_DATA4B );
			DisableTextFields( 1, 4 );
			EnableEditorButton( MERCS_SCHEDULE_DATA1A + gubCurrentScheduleActionIndex );
			gfUseScheduleData2 = FALSE;
			iDrawMode = DRAW_MODE_SCHEDULEACTION;
			gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_DOOR1;
			gfRenderTaskbar = TRUE;
			break;
		case SCHEDULE_ACTION_GRIDNO:
		case SCHEDULE_ACTION_ENTERSECTOR:
		case SCHEDULE_ACTION_SLEEP:
				ShowEditorButton( MERCS_SCHEDULE_DATA1A + gubCurrentScheduleActionIndex );
			HideEditorButton( MERCS_SCHEDULE_DATA1B + gubCurrentScheduleActionIndex );
			DisableEditorButtons( MERCS_SCHEDULE_ACTION1, MERCS_SCHEDULE_DATA4B );
			DisableTextFields( 1, 4 );
			EnableEditorButton( MERCS_SCHEDULE_DATA1A + gubCurrentScheduleActionIndex );
			gfUseScheduleData2 = FALSE;
			iDrawMode = DRAW_MODE_SCHEDULEACTION;
			gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_GRIDNO;
			gfRenderTaskbar = TRUE;
			gCurrSchedule.usData2[ gubCurrentScheduleActionIndex ] = 0xffff;
			break;
		case SCHEDULE_ACTION_LEAVESECTOR:
		case SCHEDULE_ACTION_STAYINSECTOR:
			gubScheduleInstructions = SCHEDULE_INSTRUCTIONS_NONE;
			gfRenderTaskbar = TRUE;
			gCurrSchedule.usData1[ gubCurrentScheduleActionIndex ] = 0xffff;
			gCurrSchedule.usData2[ gubCurrentScheduleActionIndex ] = 0xffff;
			break;
	}
	MarkWorldDirty();
}

void UpdateScheduleAction( UINT8 ubNewAction )
{
	gCurrSchedule.ubAction[ gubCurrentScheduleActionIndex ] = ubNewAction;
	SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_ACTION1 + gubCurrentScheduleActionIndex ], 
		gszScheduleActions[ ubNewAction ] );
	MSYS_SetBtnUserData( iEditorButton[ MERCS_SCHEDULE_ACTION1 + gubCurrentScheduleActionIndex ], 0, ubNewAction );
	//Now, based on this action, disable the other buttons
	StartScheduleAction();
	gfSingleAction = FALSE;
}

// 0:1A, 1:1B, 2:2A, 3:2B, ...
void FindScheduleGridNo( UINT8 ubScheduleData )
{
	INT32 iMapIndex;
	switch( ubScheduleData )
	{
		case 0: //1a
			iMapIndex = gCurrSchedule.usData1[0];
			break;
		case 1:	//1b
			iMapIndex = gCurrSchedule.usData2[0];
			break;
		case 2:	//2a
			iMapIndex = gCurrSchedule.usData1[1];
			break;
		case 3: //2b
			iMapIndex = gCurrSchedule.usData2[1];
			break;
		case 4:	//3a
			iMapIndex = gCurrSchedule.usData1[2];
			break;
		case 5:	//3b
			iMapIndex = gCurrSchedule.usData2[2];
			break;
		case 6:	//4a
			iMapIndex = gCurrSchedule.usData1[3];
			break;
		case 7:	//4b
			iMapIndex = gCurrSchedule.usData2[3];
			break;
		default:
			AssertMsg( 0, "FindScheduleGridNo passed incorrect dataID." );
	}
	if( iMapIndex != 0xffff )
	{
		CenterScreenAtMapIndex( iMapIndex );
	}
}

void ClearCurrentSchedule()
{
	UINT8 i;
	memset( &gCurrSchedule, 0, sizeof( SCHEDULENODE ) );
	for( i = 0; i < 4; i++ )
	{
		MSYS_SetBtnUserData( iEditorButton[ MERCS_SCHEDULE_ACTION1 + i ], 0, 0 );
		SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_ACTION1 + i ], L"No action" );
		gCurrSchedule.usTime[i] = 0xffff;
		SetExclusive24HourTimeValue( (UINT8)(i+1), gCurrSchedule.usTime[ i ] ); //blanks the field
		gCurrSchedule.usData1[i] = 0xffff;
		SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_DATA1A + i ], L"" );
		gCurrSchedule.usData2[i] = 0xffff;
		SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_DATA1B + i ], L"" );
	}
	//Remove the variance stuff
	gCurrSchedule.usFlags = 0;
	UnclickEditorButtons( MERCS_SCHEDULE_VARIANCE1, MERCS_SCHEDULE_VARIANCE4 );

	gubCurrentScheduleActionIndex = 0;
	DetermineScheduleEditability();
	gfRenderTaskbar = TRUE;
	MarkWorldDirty();
}

void RenderCurrentSchedule()
{
	FLOAT dOffsetX, dOffsetY;
	FLOAT ScrnX, ScrnY;
	INT32 i;
	INT32 iMapIndex;
	INT16 sXMapPos, sYMapPos;
	INT16 sScreenX, sScreenY;
	INT16 sX, sY;
	UINT16 str[ 3 ];
	for( i = 0; i < 8; i++ )
	{
		if( i % 2 )
			iMapIndex = gCurrSchedule.usData2[ i / 2 ];
		else
			iMapIndex = gCurrSchedule.usData1[ i / 2 ];

		if( iMapIndex == 0xffff )
			continue;

		// Convert it's location to screen coordinates
		ConvertGridNoToXY( (INT16)iMapIndex, &sXMapPos, &sYMapPos );

		dOffsetX = (FLOAT)(sXMapPos * CELL_X_SIZE) - gsRenderCenterX;
		dOffsetY = (FLOAT)(sYMapPos * CELL_Y_SIZE) - gsRenderCenterY;

		FloatFromCellToScreenCoordinates( dOffsetX, dOffsetY, &ScrnX, &ScrnY);
		
		sScreenX = ( ( gsVIEWPORT_END_X - gsVIEWPORT_START_X ) /2 ) + (INT16)ScrnX;
		sScreenY = ( ( gsVIEWPORT_END_Y - gsVIEWPORT_START_Y ) /2 ) + (INT16)ScrnY;

		// Adjust for tiles height factor!
		sScreenY -= gpWorldLevelData[ iMapIndex ].sHeight;
		// Bring it down a touch
		sScreenY += 5;

		if( sScreenY <= 355 )
		{
			// Shown it on screen!
			SetFont(TINYFONT1);
			SetFontBackground( FONT_LTKHAKI );
			SetFontForeground( FONT_WHITE );
			swprintf( str, L"%d%c", i/2 + 1, 'A' + (i%2) );
			VarFindFontCenterCoordinates( sScreenX, sScreenY, 1, 1, TINYFONT1, &sX, &sY, str );
			mprintf( sX, sY, str );
		}
	}
}

void UpdateScheduleInfo()
{
	INT32 i;
	SCHEDULENODE *pSchedule;
	UINT16 str[6];
	if( gpSelected->pSoldier->ubScheduleID )
	{
		pSchedule = GetSchedule( gpSelected->pSoldier->ubScheduleID );
		if( !pSchedule )
		{
			return;
		}
		for( i = 0; i < 4; i++ )
		{ //Update the text and buttons
			MSYS_SetBtnUserData( iEditorButton[ MERCS_SCHEDULE_ACTION1 + i ], 0, pSchedule->ubAction[i] );
			SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_ACTION1 + i ], gszScheduleActions[ pSchedule->ubAction[i] ] );
			swprintf( str, L"" );
			if( pSchedule->usData1[i] != 0xffff )
				swprintf( str, L"%d", pSchedule->usData1[i] );
			SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_DATA1A + i ], str );
			swprintf( str, L"" );
			if( pSchedule->usData2[i] != 0xffff )
				swprintf( str, L"%d", pSchedule->usData2[i] );
			SpecifyButtonText( iEditorButton[ MERCS_SCHEDULE_DATA1B + i ], str );
			if( gubCurrMercMode == MERC_SCHEDULEMODE )
			{ //Update the text input fields too!
				SetExclusive24HourTimeValue( (UINT8)(i+1), pSchedule->usTime[i] );
			}
		}
		
		//Check or uncheck the checkbox buttons based on the schedule's status.
		UnclickEditorButtons( MERCS_SCHEDULE_VARIANCE1, MERCS_SCHEDULE_VARIANCE4 );
		if( pSchedule->usFlags & SCHEDULE_FLAGS_VARIANCE1 )
			ClickEditorButton( MERCS_SCHEDULE_VARIANCE1 );
		if( pSchedule->usFlags & SCHEDULE_FLAGS_VARIANCE2 )
			ClickEditorButton( MERCS_SCHEDULE_VARIANCE2 );
		if( pSchedule->usFlags & SCHEDULE_FLAGS_VARIANCE3 )
			ClickEditorButton( MERCS_SCHEDULE_VARIANCE3 );
		if( pSchedule->usFlags & SCHEDULE_FLAGS_VARIANCE4 )
			ClickEditorButton( MERCS_SCHEDULE_VARIANCE4 );

		//Copy the schedule over to the current global schedule used for editing purposes.
		memcpy( &gCurrSchedule, pSchedule, sizeof( SCHEDULENODE ) );
		DetermineScheduleEditability();
	}
	else
	{
		ClearCurrentSchedule();
	}
}

BASIC_SOLDIERCREATE_STRUCT gSaveBufferBasicPlacement;
SOLDIERCREATE_STRUCT gSaveBufferDetailedPlacement;

void CopyMercPlacement( INT32 iMapIndex )
{
	if( gsSelectedMercID == -1 )
	{
		ScreenMsg( FONT_MCOLOR_LTRED, MSG_INTERFACE, L"Placement not copied because no placement selected." );
		return;
	}
	gfSaveBuffer = TRUE;
	memcpy( &gSaveBufferBasicPlacement, gpSelected->pBasicPlacement, sizeof( BASIC_SOLDIERCREATE_STRUCT ) );
	if( gSaveBufferBasicPlacement.fDetailedPlacement )
	{
		memcpy( &gSaveBufferDetailedPlacement, gpSelected->pDetailedPlacement, sizeof( SOLDIERCREATE_STRUCT ) );
	}
	ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"Placement copied." );
}

void PasteMercPlacement( INT32 iMapIndex )
{
	SOLDIERTYPE *pSoldier;
	SOLDIERCREATE_STRUCT tempDetailedPlacement;
	INT32 i;

	if( !gfSaveBuffer )
	{
		ScreenMsg( FONT_MCOLOR_LTRED, MSG_INTERFACE, L"Placement not pasted as no placement is saved in buffer." );
		return;
	}

	memcpy( &gTempBasicPlacement, &gSaveBufferBasicPlacement, sizeof( BASIC_SOLDIERCREATE_STRUCT ) );

	gTempBasicPlacement.bBodyType = -1;

	//calculate specific information based on the team.
	switch( iDrawMode )
	{
		case DRAW_MODE_ENEMY:
			gTempBasicPlacement.bTeam = ENEMY_TEAM;
			gTempBasicPlacement.bBodyType = RANDOM;
			gTempBasicPlacement.ubSoldierClass = gubSoldierClass;
			break;
		case DRAW_MODE_CREATURE:
			gTempBasicPlacement.bTeam = CREATURE_TEAM;
			gTempBasicPlacement.bBodyType = gbCurrCreature;
			break;
		case DRAW_MODE_REBEL:
			gTempBasicPlacement.bTeam = MILITIA_TEAM; 
			gTempBasicPlacement.bBodyType = RANDOM;
			break;  
		case DRAW_MODE_CIVILIAN:
			gTempBasicPlacement.bTeam = CIV_TEAM;
			gTempBasicPlacement.bBodyType = RANDOM;
			gTempBasicPlacement.ubCivilianGroup = gubCivGroup;
			if( giCurrentTilesetID == 1 ) //caves
			{
				gTempBasicPlacement.ubSoldierClass = SOLDIER_CLASS_MINER;
			}
			break;
	}

	if( IsLocationSittable( iMapIndex, gfRoofPlacement ) )
	{
		UINT8	ubID;
		INT16 sSectorX, sSectorY;
		SOLDIERINITNODE *pNode;
		
 		GetCurrentWorldSector( &sSectorX, &sSectorY );

		//Set up some general information.
		//gTempBasicPlacement.fDetailedPlacement = TRUE;
		gTempBasicPlacement.usStartingGridNo = (UINT16)iMapIndex;

		//Generate detailed placement information given the temp placement information.
		if( gTempBasicPlacement.fDetailedPlacement )
		{
			memcpy( &gTempDetailedPlacement, &gSaveBufferDetailedPlacement, sizeof( SOLDIERCREATE_STRUCT ) );
		}
		else
		{
			CreateDetailedPlacementGivenBasicPlacementInfo( &gTempDetailedPlacement, &gTempBasicPlacement );
		}

		//Set the sector information -- probably unnecessary.
		gTempDetailedPlacement.sSectorX = sSectorX;
		gTempDetailedPlacement.sSectorY = sSectorY;

		if( gTempBasicPlacement.fDetailedPlacement )
		{
			CreateDetailedPlacementGivenStaticDetailedPlacementAndBasicPlacementInfo( &tempDetailedPlacement, &gTempDetailedPlacement, &gTempBasicPlacement );
		}
		else
		{
			memcpy( &tempDetailedPlacement, &gTempDetailedPlacement, sizeof( SOLDIERCREATE_STRUCT ) );
		}

		//Create the soldier, but don't place it yet.
		if ( pSoldier = TacticalCreateSoldier( &tempDetailedPlacement, &ubID ) )
		{
			pSoldier->bVisible = 1;
			pSoldier->bLastRenderVisibleValue = 1;
			//Set up the soldier in the list, so we can track the soldier in the 
			//future (saving, loading, strategic AI)
			pNode = AddBasicPlacementToSoldierInitList( &gTempBasicPlacement );
			Assert( pNode );
			pNode->pSoldier = pSoldier;
			if( gSaveBufferBasicPlacement.fDetailedPlacement )
			{ //Add the static detailed placement information in the same newly created node as the basic placement.
				//read static detailed placement from file
				//allocate memory for new static detailed placement
				gTempBasicPlacement.fDetailedPlacement = TRUE;
				gTempBasicPlacement.fPriorityExistance = gSaveBufferBasicPlacement.fPriorityExistance;
				pNode->pDetailedPlacement = (SOLDIERCREATE_STRUCT*)MemAlloc( sizeof( SOLDIERCREATE_STRUCT ) );
				if( !pNode->pDetailedPlacement )
				{
					AssertMsg( 0, "Failed to allocate memory for new detailed placement in PasteMercPlacement." );
					return;
				}
				//copy the file information from temp var to node in list.
				memcpy( pNode->pDetailedPlacement, &gSaveBufferDetailedPlacement, sizeof( SOLDIERCREATE_STRUCT ) );
			}
			
			//Add the soldier to physically appear on the map now.
			InternalAddSoldierToSector( ubID, FALSE, FALSE, 0, 0 );
			IndicateSelectedMerc( ubID );

			//Move him to the roof if intended and possible.
			if( gfRoofPlacement && FlatRoofAboveGridNo( iMapIndex ) )
			{
				gpSelected->pBasicPlacement->fOnRoof = TRUE;
				if( gpSelected->pDetailedPlacement )
					gpSelected->pDetailedPlacement->fOnRoof = TRUE;
				SetSoldierHeight( gpSelected->pSoldier, 58.0 );
			}
			UnclickEditorButtons( FIRST_MERCS_INVENTORY_BUTTON, LAST_MERCS_INVENTORY_BUTTON );
			for( i = FIRST_MERCS_INVENTORY_BUTTON; i <= LAST_MERCS_INVENTORY_BUTTON; i++ )
			{
				SetEnemyDroppableStatus( gbMercSlotTypes[i-FIRST_MERCS_INVENTORY_BUTTON], FALSE );
			}
			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"Placement pasted." );
		}
		else
		{
			ScreenMsg( FONT_MCOLOR_LTRED, MSG_INTERFACE, L"Placement not pasted as the maximum number of placements for this team is already used." );
		}
	}
}


#endif