#ifdef PRECOMPILEDHEADERS
	#include "Laptop All.h"
#else
	#include "laptop.h"
	#include "AimFacialIndex.h"
	#include "WordWrap.h"
	#include "Utilities.h"
	#include "WCheck.h"
	#include "stdio.h"
	#include "Aim.h"
	#include "Soldier Profile.h"
	#include "email.h"
	#include "Text.h"
	#include "AimSort.h"
	#include "Assignments.h"
#endif


UINT8			gubCurrentSortMode;
UINT8			gubCurrentListMode;
extern UINT8			gbCurrentIndex;


UINT32		guiMugShotBorder;
UINT32		guiAimFiFace[ MAX_NUMBER_MERCS ];



#define		AIM_FI_NUM_MUHSHOTS_X		8
#define		AIM_FI_NUM_MUHSHOTS_Y		5

#define		AIM_FI_PORTRAIT_WIDTH		52
#define		AIM_FI_PORTRAIT_HEIGHT	48

#define		AIM_FI_FIRST_MUGSHOT_X	IMAGE_OFFSET_X + 6
#define		AIM_FI_FIRST_MUGSHOT_Y	IMAGE_OFFSET_Y + 69//67//70 //68 //65
#define		AIM_FI_MUGSHOT_GAP_X		10
#define		AIM_FI_MUGSHOT_GAP_Y		13
#define		AIM_FI_FACE_OFFSET			2

#define		AIM_FI_NNAME_OFFSET_X		2
#define		AIM_FI_NNAME_OFFSET_Y		AIM_FI_PORTRAIT_HEIGHT+1
#define		AIM_FI_NNAME_WIDTH			AIM_FI_PORTRAIT_WIDTH+4

#define		AIM_FI_MEMBER_TEXT_X		IMAGE_OFFSET_X + 155
#define		AIM_FI_MEMBER_TEXT_Y		AIM_SYMBOL_Y + AIM_SYMBOL_SIZE_Y + 1
#define		AIM_FI_MEMBER_TEXT_WIDTH	190

#define		AIM_FI_AWAY_TEXT_OFFSET_X			3
#define		AIM_FI_AWAY_TEXT_OFFSET_Y			23//3//36
#define		AIM_FI_AWAY_TEXT_OFFSET_WIDTH	48


//Mouse Regions

//Face regions
MOUSE_REGION		gMercFaceMouseRegions[ MAX_NUMBER_MERCS ];
void SelectMercFaceRegionCallBack(MOUSE_REGION * pRegion, INT32 iReason );
void SelectMercFaceMoveRegionCallBack(MOUSE_REGION * pRegion, INT32 iReason );

//Screen region, used to right click to go back to previous page
MOUSE_REGION		gScreenMouseRegions;
void SelectScreenRegionCallBack(MOUSE_REGION * pRegion, INT32 iReason );


BOOLEAN DrawMercsFaceToScreen(UINT8 ubMercID, UINT16 usPosX, UINT16 usPosY, UINT8 ubImage);


void GameInitAimFacialIndex()
{

}

BOOLEAN EnterAimFacialIndex()
{
  VOBJECT_DESC    VObjectDesc;
	UINT8	i;
	UINT16		usPosX, usPosY, x,y;
	STR				sFaceLoc = "FACES\\";
	char			sTemp[100];

	// load the Portait graphic and add it
	VObjectDesc.fCreateFlags=VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("LAPTOP\\MugShotBorder3.sti", VObjectDesc.ImageFile);
	CHECKF(AddVideoObject(&VObjectDesc, &guiMugShotBorder));

	usPosX = AIM_FI_FIRST_MUGSHOT_X;
	usPosY = AIM_FI_FIRST_MUGSHOT_Y;
	i=0;
	for(y=0; y<AIM_FI_NUM_MUHSHOTS_Y; y++)
	{
		for(x=0; x<AIM_FI_NUM_MUHSHOTS_X; x++)
		{

			MSYS_DefineRegion( &gMercFaceMouseRegions[ i ], usPosX, usPosY, (INT16)(usPosX + AIM_FI_PORTRAIT_WIDTH), (INT16)(usPosY + AIM_FI_PORTRAIT_HEIGHT), MSYS_PRIORITY_HIGH,
								 CURSOR_WWW, SelectMercFaceMoveRegionCallBack, SelectMercFaceRegionCallBack); 
			// Add region
			MSYS_AddRegion( &gMercFaceMouseRegions[ i ] );
			MSYS_SetRegionUserData( &gMercFaceMouseRegions[ i ], 0, i);

			sprintf(sTemp, "%s%02d.sti", sFaceLoc, AimMercArray[i]);
			VObjectDesc.fCreateFlags=VOBJECT_CREATE_FROMFILE;
			FilenameForBPP(sTemp, VObjectDesc.ImageFile);
			if( !AddVideoObject(&VObjectDesc, &guiAimFiFace[i]) )
				return( FALSE );

			
			usPosX += AIM_FI_PORTRAIT_WIDTH + AIM_FI_MUGSHOT_GAP_X;
			i++;
		}
		usPosX = AIM_FI_FIRST_MUGSHOT_X;
		usPosY += AIM_FI_PORTRAIT_HEIGHT + AIM_FI_MUGSHOT_GAP_Y;
	}

	MSYS_DefineRegion( &gScreenMouseRegions, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X, LAPTOP_SCREEN_WEB_LR_Y, MSYS_PRIORITY_HIGH-1,
						 CURSOR_LAPTOP_SCREEN, MSYS_NO_CALLBACK, SelectScreenRegionCallBack); 
	// Add region
	MSYS_AddRegion( &gScreenMouseRegions );


	InitAimMenuBar();
	InitAimDefaults();

	RenderAimFacialIndex();

	return( TRUE );
}

void ExitAimFacialIndex()
{
	UINT8	i;

	RemoveAimDefaults();

	DeleteVideoObjectFromIndex(guiMugShotBorder);


	for(i=0; i<MAX_NUMBER_MERCS; i++)
	{
		DeleteVideoObjectFromIndex( guiAimFiFace[i]);
	  MSYS_RemoveRegion( &gMercFaceMouseRegions[ i ]);
	}
	ExitAimMenuBar();

  MSYS_RemoveRegion( &gScreenMouseRegions);
}

void HandleAimFacialIndex()
{
//	if( fShowBookmarkInfo )
//		fPausedReDrawScreenFlag = TRUE;

}

BOOLEAN RenderAimFacialIndex()
{
	UINT16		usPosX, usPosY, x,y;
	wchar_t		sString[150];
	UINT8			i;

	DrawAimDefaults();

	//Display the 'A.I.M. Members Sorted Ascending By Price' type string
	if( gubCurrentListMode == AIM_ASCEND )
		swprintf(sString, AimFiText[ AIM_FI_AIM_MEMBERS_SORTED_ASCENDING ], AimFiText[gubCurrentSortMode] );
	else
		swprintf(sString, AimFiText[ AIM_FI_AIM_MEMBERS_SORTED_DESCENDING ], AimFiText[gubCurrentSortMode] );

	DrawTextToScreen(sString, AIM_FI_MEMBER_TEXT_X, AIM_FI_MEMBER_TEXT_Y, AIM_FI_MEMBER_TEXT_WIDTH, AIM_MAINTITLE_FONT, AIM_MAINTITLE_COLOR, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);			


	//Draw the mug shot border and face
	usPosX = AIM_FI_FIRST_MUGSHOT_X;
	usPosY = AIM_FI_FIRST_MUGSHOT_Y;

	i=0;
	for(y=0; y<AIM_FI_NUM_MUHSHOTS_Y; y++)
	{
		for(x=0; x<AIM_FI_NUM_MUHSHOTS_X; x++)
		{
			DrawMercsFaceToScreen(i, usPosX, usPosY, 1);
			DrawTextToScreen(gMercProfiles[AimMercArray[i]].zNickname, (UINT16)(usPosX - AIM_FI_NNAME_OFFSET_X), (UINT16)(usPosY + AIM_FI_NNAME_OFFSET_Y), AIM_FI_NNAME_WIDTH, AIM_FONT12ARIAL, AIM_FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);			
		
			usPosX += AIM_FI_PORTRAIT_WIDTH + AIM_FI_MUGSHOT_GAP_X;
			i++;
		}
		usPosX = AIM_FI_FIRST_MUGSHOT_X;
		usPosY += AIM_FI_PORTRAIT_HEIGHT + AIM_FI_MUGSHOT_GAP_Y;
	}

	DisableAimButton();

	//display the 'left and right click' onscreen help msg
	DrawTextToScreen(AimFiText[AIM_FI_LEFT_CLICK], AIM_FI_LEFT_CLICK_TEXT_X, AIM_FI_LEFT_CLICK_TEXT_Y, AIM_FI_CLICK_TEXT_WIDTH, AIM_FI_HELP_TITLE_FONT, AIM_FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);			
	DrawTextToScreen(AimFiText[AIM_FI_TO_SELECT], AIM_FI_LEFT_CLICK_TEXT_X, AIM_FI_LEFT_CLICK_TEXT_Y+AIM_FI_CLICK_DESC_TEXT_Y_OFFSET, AIM_FI_CLICK_TEXT_WIDTH, AIM_FI_HELP_FONT, AIM_FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);			

	DrawTextToScreen(AimFiText[AIM_FI_RIGHT_CLICK], AIM_FI_RIGHT_CLICK_TEXT_X, AIM_FI_LEFT_CLICK_TEXT_Y, AIM_FI_CLICK_TEXT_WIDTH, AIM_FI_HELP_TITLE_FONT, AIM_FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);			
	DrawTextToScreen(AimFiText[AIM_FI_TO_ENTER_SORT_PAGE], AIM_FI_RIGHT_CLICK_TEXT_X, AIM_FI_LEFT_CLICK_TEXT_Y+AIM_FI_CLICK_DESC_TEXT_Y_OFFSET, AIM_FI_CLICK_TEXT_WIDTH, AIM_FI_HELP_FONT, AIM_FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);			

  MarkButtonsDirty( );

  RenderWWWProgramTitleBar( );

	InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
	return(TRUE);
}

void SelectMercFaceRegionCallBack(MOUSE_REGION * pRegion, INT32 iReason )
{ 
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
	}
	else if(iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS;
		gbCurrentIndex = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS_SORTED_FILES;
	} 
}


void SelectScreenRegionCallBack(MOUSE_REGION * pRegion, INT32 iReason )
{ 
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
		guiCurrentLaptopMode = LAPTOP_MODE_AIM_MEMBERS_SORTED_FILES;
	}
}


void SelectMercFaceMoveRegionCallBack(MOUSE_REGION * pRegion, INT32 reason )
{
	UINT8	ubMercNum;
	UINT16 usPosX, usPosY;
	UINT16 ty1, ty2, tx1, tx2;

	ubMercNum = (UINT8) MSYS_GetRegionUserData( pRegion, 0 );

	ty1 = AIM_FI_FIRST_MUGSHOT_Y;
	ty2 = (AIM_FI_PORTRAIT_HEIGHT + AIM_FI_MUGSHOT_GAP_Y);

	tx1 = AIM_FI_FIRST_MUGSHOT_X;
	tx2 = (AIM_FI_PORTRAIT_WIDTH + AIM_FI_MUGSHOT_GAP_X);

	usPosY = ubMercNum / AIM_FI_NUM_MUHSHOTS_X;
	usPosY = AIM_FI_FIRST_MUGSHOT_Y + (AIM_FI_PORTRAIT_HEIGHT + AIM_FI_MUGSHOT_GAP_Y) * usPosY;

	usPosX = ubMercNum % AIM_FI_NUM_MUHSHOTS_X;
	usPosX = AIM_FI_FIRST_MUGSHOT_X + (AIM_FI_PORTRAIT_WIDTH + AIM_FI_MUGSHOT_GAP_X) * usPosX;

//	fReDrawNewMailFlag = TRUE;

	if( reason & MSYS_CALLBACK_REASON_LOST_MOUSE )
	{
		pRegion->uiFlags &= (~BUTTON_CLICKED_ON );
		DrawMercsFaceToScreen(ubMercNum, usPosX, usPosY, 1);
		InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX, pRegion->RegionBottomRightY);
	}
	else if( reason & MSYS_CALLBACK_REASON_GAIN_MOUSE )
	{
		pRegion->uiFlags |= BUTTON_CLICKED_ON ;
		DrawMercsFaceToScreen(ubMercNum, usPosX, usPosY, 0);
		InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX, pRegion->RegionBottomRightY);
	}
} 

BOOLEAN DrawMercsFaceToScreen(UINT8 ubMercID, UINT16 usPosX, UINT16 usPosY, UINT8 ubImage)
{
  HVOBJECT	hMugShotBorderHandle;
	HVOBJECT	hFaceHandle;
	SOLDIERTYPE	*pSoldier=NULL;

	pSoldier = FindSoldierByProfileID( AimMercArray[ubMercID], TRUE );


	//Blt the portrait background
	GetVideoObject(&hMugShotBorderHandle, guiMugShotBorder);
	BltVideoObject(FRAME_BUFFER, hMugShotBorderHandle, ubImage,usPosX, usPosY, VO_BLT_SRCTRANSPARENCY,NULL);

	//Blt face to screen
	GetVideoObject(&hFaceHandle, guiAimFiFace[ubMercID]);
	BltVideoObject(FRAME_BUFFER, hFaceHandle, 0,usPosX+AIM_FI_FACE_OFFSET, usPosY+AIM_FI_FACE_OFFSET, VO_BLT_SRCTRANSPARENCY,NULL);

	if( IsMercDead( AimMercArray[ubMercID] ) )
	{
		//get the face object
		GetVideoObject(&hFaceHandle, guiAimFiFace[ubMercID]);

		//if the merc is dead
		//shade the face red, (to signif that he is dead)
		hFaceHandle->pShades[ 0 ]		= Create16BPPPaletteShaded( hFaceHandle->pPaletteEntry, DEAD_MERC_COLOR_RED, DEAD_MERC_COLOR_GREEN, DEAD_MERC_COLOR_BLUE, TRUE );

		//set the red pallete to the face
		SetObjectHandleShade( guiAimFiFace[ubMercID], 0 );

		//Blt face to screen 
		BltVideoObject(FRAME_BUFFER, hFaceHandle, 0,usPosX+AIM_FI_FACE_OFFSET, usPosY+AIM_FI_FACE_OFFSET, VO_BLT_SRCTRANSPARENCY,NULL);
	
		DrawTextToScreen(AimFiText[AIM_FI_DEAD], (UINT16)(usPosX+AIM_FI_AWAY_TEXT_OFFSET_X), (UINT16)(usPosY+AIM_FI_AWAY_TEXT_OFFSET_Y), AIM_FI_AWAY_TEXT_OFFSET_WIDTH, FONT10ARIAL, 145, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);
	}

	//else if the merc is currently a POW or, the merc was fired as a pow
	else if( gMercProfiles[ AimMercArray[ubMercID] ].bMercStatus == MERC_FIRED_AS_A_POW  || ( pSoldier &&  pSoldier->bAssignment == ASSIGNMENT_POW ) )
	{
		ShadowVideoSurfaceRect( FRAME_BUFFER, usPosX+AIM_FI_FACE_OFFSET, usPosY+AIM_FI_FACE_OFFSET, usPosX + 48+AIM_FI_FACE_OFFSET, usPosY + 43+AIM_FI_FACE_OFFSET);
		DrawTextToScreen( pPOWStrings[0], (UINT16)(usPosX+AIM_FI_AWAY_TEXT_OFFSET_X), (UINT16)(usPosY+AIM_FI_AWAY_TEXT_OFFSET_Y), AIM_FI_AWAY_TEXT_OFFSET_WIDTH, FONT10ARIAL, 145, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);
	}

	//if the merc is on our team
	else if( pSoldier != NULL )
	{
		ShadowVideoSurfaceRect( FRAME_BUFFER, usPosX+AIM_FI_FACE_OFFSET, usPosY+AIM_FI_FACE_OFFSET, usPosX + 48+AIM_FI_FACE_OFFSET, usPosY + 43+AIM_FI_FACE_OFFSET);
		DrawTextToScreen( MercInfo[MERC_FILES_ALREADY_HIRED], (UINT16)(usPosX+AIM_FI_AWAY_TEXT_OFFSET_X), (UINT16)(usPosY+AIM_FI_AWAY_TEXT_OFFSET_Y), AIM_FI_AWAY_TEXT_OFFSET_WIDTH, FONT10ARIAL, 145, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);
	}

	//if the merc is away, shadow his/her face and blit 'away' over top
	else if( !IsMercHireable( AimMercArray[ubMercID] ) )
	{
		ShadowVideoSurfaceRect( FRAME_BUFFER, usPosX+AIM_FI_FACE_OFFSET, usPosY+AIM_FI_FACE_OFFSET, usPosX + 48+AIM_FI_FACE_OFFSET, usPosY + 43+AIM_FI_FACE_OFFSET);
		DrawTextToScreen( AimFiText[AIM_FI_DEAD+1], (UINT16)(usPosX+AIM_FI_AWAY_TEXT_OFFSET_X), (UINT16)(usPosY+AIM_FI_AWAY_TEXT_OFFSET_Y), AIM_FI_AWAY_TEXT_OFFSET_WIDTH, FONT10ARIAL, 145, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED	);
		//if not enough room use this..
		//AimFiText[AIM_FI_AWAY]
	}

	return(TRUE);
}

