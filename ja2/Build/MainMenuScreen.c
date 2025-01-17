#ifdef PRECOMPILEDHEADERS
	#include "JA2 All.h"
	#include "Encrypted File.h"
#else
	#include "sgp.h"
	#include "screenids.h"
	#include "Timer Control.h"
	#include "sys globals.h"
	#include "fade screen.h"
	#include "sysutil.h"
	#include "vobject_blitters.h"
	#include "MercTextBox.h"
	#include "wcheck.h"
	#include "cursors.h"
	#include "font control.h"
	#include "mainmenuscreen.h"
	#include "cursor control.h"
	#include "render dirty.h"
	#include "music control.h"
	#include "GameSettings.h"
	#include "GameScreen.h"
	#include "SaveLoadScreen.h"
	#include "SaveLoadGame.h"
	#include "Options Screen.h"
	#include	"English.h"
	#include	"Gameloop.h"
	#include	"Game Init.h"
	#include	"Utilities.h"
	#include	"WordWrap.h"
	#include "Font Control.h"
	#include "text.h"
	#include "Multi Language Graphic Utils.h"
	#include "Encrypted File.h"
	#include "JA2 Splash.h"
	#include "GameVersion.h"
#endif

#define	MAINMENU_TEXT_FILE						"LoadScreens\\MainMenu.edt"
#define MAINMENU_RECORD_SIZE					80 * 2

//#define TESTFOREIGNFONTS

// MENU ITEMS
enum
{
//	TITLE,
	NEW_GAME,
	LOAD_GAME,
	PREFERENCES,
	CREDITS,
	QUIT,
	NUM_MENU_ITEMS
};

#define		MAINMENU_X					( (	640 - 214 ) / 2 )
#define		MAINMENU_TITLE_Y		75
#define		MAINMENU_Y					277//200
#define		MAINMENU_Y_SPACE		37

	
INT32							iMenuImages[ NUM_MENU_ITEMS ];
INT32							iMenuButtons[ NUM_MENU_ITEMS ]; 

UINT16						gusMainMenuButtonWidths[ NUM_MENU_ITEMS ];

UINT32						guiMainMenuBackGroundImage;
UINT32						guiJa2LogoImage;

MOUSE_REGION			gBackRegion;
INT8							gbHandledMainMenu = 0;
BOOLEAN						fInitialRender = FALSE;
//BOOLEAN						gfDoHelpScreen = 0;

BOOLEAN						gfMainMenuScreenEntry = FALSE;
BOOLEAN						gfMainMenuScreenExit = FALSE;

UINT32						guiMainMenuExitScreen = MAINMENU_SCREEN;


extern	BOOLEAN		gfLoadGameUponEntry;


void ExitMainMenu( );
void MenuButtonCallback(GUI_BUTTON *btn, INT32 reason);
void HandleMainMenuInput();
void HandleMainMenuScreen();
void DisplayAssignmentText();
void ClearMainMenu();
void HandleHelpScreenInput();
void SelectMainMenuBackGroundRegionCallBack(MOUSE_REGION * pRegion, INT32 iReason );
void SetMainMenuExitScreen( UINT32 uiNewScreen );
void CreateDestroyBackGroundMouseMask( BOOLEAN fCreate );
BOOLEAN CreateDestroyMainMenuButtons( BOOLEAN fCreate );
void RenderMainMenu();
void RestoreButtonBackGrounds();




UINT32	MainMenuScreenInit( )
{
	DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Version Label: %S", zVersionLabel ));
	DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Version #:     %s", czVersionNumber ));
	DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Tracking #:    %S", zTrackingNumber ));

	return( TRUE );
}



UINT32	MainMenuScreenHandle( )
{
	UINT32 cnt;
	UINT32 uiTime;

	if( guiSplashStartTime + 4000 > GetJA2Clock() )
	{
		SetCurrentCursorFromDatabase( VIDEO_NO_CURSOR );
		SetMusicMode( MUSIC_NONE );
		return MAINMENU_SCREEN;  //The splash screen hasn't been up long enough yet.
	}
	if( guiSplashFrameFade )
	{ //Fade the splash screen.
		uiTime = GetJA2Clock();
		if( guiSplashFrameFade > 2 )
			ShadowVideoSurfaceRectUsingLowPercentTable( FRAME_BUFFER, 0, 0, 640, 480 );		
		else if( guiSplashFrameFade > 1 )
			ColorFillVideoSurfaceArea( FRAME_BUFFER, 0, 0, 640, 480, 0 );
		else
		{
			uiTime = GetJA2Clock();
			//while( GetJA2Clock() < uiTime + 375 );
			SetMusicMode( MUSIC_MAIN_MENU );
		}

		//while( uiTime + 100 > GetJA2Clock() );

		guiSplashFrameFade--;

		InvalidateScreen();
		EndFrameBufferRender();

		SetCurrentCursorFromDatabase( VIDEO_NO_CURSOR );
		
		return MAINMENU_SCREEN;
	}

	SetCurrentCursorFromDatabase( CURSOR_NORMAL );

	if( gfMainMenuScreenEntry )
	{
		InitMainMenu( );
		gfMainMenuScreenEntry = FALSE;
		gfMainMenuScreenExit = FALSE;
		guiMainMenuExitScreen = MAINMENU_SCREEN;
		SetMusicMode( MUSIC_MAIN_MENU );
	}


	if ( fInitialRender )
	{
		ClearMainMenu();
		RenderMainMenu();

		fInitialRender = FALSE;
	}

	RestoreButtonBackGrounds();

	// Render buttons
	for ( cnt = 0; cnt < NUM_MENU_ITEMS; cnt++ )
	{
		MarkAButtonDirty( iMenuButtons[ cnt ] );
	}

	RenderButtons( );

	EndFrameBufferRender( );

	
//	if ( gfDoHelpScreen )
//		HandleHelpScreenInput();
//	else
		HandleMainMenuInput();

	HandleMainMenuScreen();

	if( gfMainMenuScreenExit )
	{
		ExitMainMenu( );
		gfMainMenuScreenExit = FALSE;
		gfMainMenuScreenEntry = TRUE;
	}

	if( guiMainMenuExitScreen != MAINMENU_SCREEN )
		gfMainMenuScreenEntry = TRUE;

	return( guiMainMenuExitScreen );
}


UINT32	MainMenuScreenShutdown(  )
{
	return( FALSE );
}




void HandleMainMenuScreen()
{
	if ( gbHandledMainMenu != 0 )
	{
		// Exit according to handled value!
		switch( gbHandledMainMenu )
		{
			case QUIT:
				gfMainMenuScreenExit = TRUE;

#ifdef JA2DEMO
				// Goto ad pages				
				SetPendingNewScreen( DEMO_EXIT_SCREEN );
				SetMusicMode( MUSIC_MAIN_MENU );
				FadeOutNextFrame( );
#else
				gfProgramIsRunning = FALSE;
#endif
				break;

			case NEW_GAME:

//					gfDoHelpScreen = 1;
//				gfMainMenuScreenExit = TRUE;
//				if( !gfDoHelpScreen )
//					SetMainMenuExitScreen( INIT_SCREEN );
				break;

			case LOAD_GAME:
				// Select the game which is to be restored
				guiPreviousOptionScreen = guiCurrentScreen;
				guiMainMenuExitScreen = SAVE_LOAD_SCREEN;
				gbHandledMainMenu = 0;
				gfSaveGame = FALSE;
				gfMainMenuScreenExit = TRUE;

				break;

			case PREFERENCES:
				guiPreviousOptionScreen = guiCurrentScreen;
				guiMainMenuExitScreen = OPTIONS_SCREEN;
				gbHandledMainMenu = 0;
				gfMainMenuScreenExit = TRUE;
				break;

			case CREDITS:
				guiMainMenuExitScreen = CREDIT_SCREEN;
				gbHandledMainMenu = 0;
				gfMainMenuScreenExit = TRUE;
				break;
		}
	}
}



BOOLEAN InitMainMenu( )
{
  VOBJECT_DESC    VObjectDesc;

//	gfDoHelpScreen = 0;

	//Check to see whatr saved game files exist
	InitSaveGameArray();

	//Create the background mouse mask
	CreateDestroyBackGroundMouseMask( TRUE );

	CreateDestroyMainMenuButtons( TRUE );

	// load background graphic and add it
	VObjectDesc.fCreateFlags=VOBJECT_CREATE_FROMFILE;
	FilenameForBPP("LOADSCREENS\\MainMenuBackGround.sti", VObjectDesc.ImageFile);
	CHECKF(AddVideoObject(&VObjectDesc, &guiMainMenuBackGroundImage ));

	// load ja2 logo graphic and add it
	VObjectDesc.fCreateFlags=VOBJECT_CREATE_FROMFILE;
//	FilenameForBPP("INTERFACE\\Ja2_2.sti", VObjectDesc.ImageFile);
	FilenameForBPP("LOADSCREENS\\Ja2Logo.sti", VObjectDesc.ImageFile);
	CHECKF(AddVideoObject(&VObjectDesc, &guiJa2LogoImage ));

/*
	// Gray out some buttons based on status of game!
	if( gGameSettings.bLastSavedGameSlot < 0 || gGameSettings.bLastSavedGameSlot >= NUM_SAVE_GAMES )
	{
		DisableButton( iMenuButtons[ LOAD_GAME ] );
	}
	//The ini file said we have a saved game, but there is no saved game
	else if( gbSaveGameArray[ gGameSettings.bLastSavedGameSlot ] == FALSE )
		DisableButton( iMenuButtons[ LOAD_GAME ] );
*/

	//if there are no saved games, disable the button
	if( !IsThereAnySavedGameFiles() )
		DisableButton( iMenuButtons[ LOAD_GAME ] );


//	DisableButton( iMenuButtons[ CREDITS ] );
//	DisableButton( iMenuButtons[ TITLE ] );

	gbHandledMainMenu = 0;
	fInitialRender		= TRUE;

	SetPendingNewScreen( MAINMENU_SCREEN);
	guiMainMenuExitScreen = MAINMENU_SCREEN;

	InitGameOptions();

	DequeueAllKeyBoardEvents();

	return( TRUE );
}


void ExitMainMenu( )
{
//	UINT32										 uiDestPitchBYTES;
//	UINT8											 *pDestBuf;

//	if( !gfDoHelpScreen )
	{
		CreateDestroyBackGroundMouseMask( FALSE );
	}


	CreateDestroyMainMenuButtons( FALSE );
	
	DeleteVideoObjectFromIndex( guiMainMenuBackGroundImage );
	DeleteVideoObjectFromIndex( guiJa2LogoImage );

	gMsgBox.uiExitScreen = MAINMENU_SCREEN;
/*
	// CLEAR THE FRAME BUFFER
	pDestBuf = LockVideoSurface( FRAME_BUFFER, &uiDestPitchBYTES );
	memset(pDestBuf, 0, SCREEN_HEIGHT * uiDestPitchBYTES );
	UnLockVideoSurface( FRAME_BUFFER );
	InvalidateScreen( );
*/
}


void MenuButtonCallback(GUI_BUTTON *btn,INT32 reason)
{
	INT8	bID;

	bID = (UINT8)btn->UserData[0];

	if (!(btn->uiFlags & BUTTON_ENABLED))
		return;

	if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
	{
		// handle menu
		gbHandledMainMenu = bID;
		RenderMainMenu();

		if( gbHandledMainMenu == NEW_GAME )
		{
			SetMainMenuExitScreen( GAME_INIT_OPTIONS_SCREEN );
		}
		else if( gbHandledMainMenu == LOAD_GAME )
		{
			if( gfKeyState[ ALT ] )
				gfLoadGameUponEntry = TRUE;
		}

		btn->uiFlags &= (~BUTTON_CLICKED_ON );
	}
	if( reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
	{
		RenderMainMenu();
		btn->uiFlags |= BUTTON_CLICKED_ON;
	}
}

void MenuButtonMoveCallback(GUI_BUTTON *btn,INT32 reason)
{
	if(reason & MSYS_CALLBACK_REASON_LOST_MOUSE)
	{
//		btn->uiFlags &= (~BUTTON_CLICKED_ON );
		RenderMainMenu();
		InvalidateRegion(btn->Area.RegionTopLeftX, btn->Area.RegionTopLeftY, btn->Area.RegionBottomRightX, btn->Area.RegionBottomRightY);
	}
	if(reason & MSYS_CALLBACK_REASON_GAIN_MOUSE)
	{
//		btn->uiFlags &= (~BUTTON_CLICKED_ON );
		RenderMainMenu();
		InvalidateRegion(btn->Area.RegionTopLeftX, btn->Area.RegionTopLeftY, btn->Area.RegionBottomRightX, btn->Area.RegionBottomRightY);
	}
}



void HandleMainMenuInput()
{
	InputAtom  InputEvent;

	// Check for esc 
	while (DequeueEvent(&InputEvent) == TRUE)
  {
    if( InputEvent.usEvent == KEY_UP )
		{
			switch( InputEvent.usParam )
			{
/*
				case ESC:
					gbHandledMainMenu = QUIT;
					break;
*/

#ifdef JA2TESTVERSION
				case 'q':
					gbHandledMainMenu = NEW_GAME;
					gfMainMenuScreenExit = TRUE;
					SetMainMenuExitScreen( INIT_SCREEN );

//						gfDoHelpScreen = 1;
					break;

				case 'i':
					SetPendingNewScreen( INTRO_SCREEN );
					gfMainMenuScreenExit = TRUE;
					break;
#endif

				case 'c':
					gbHandledMainMenu = LOAD_GAME;

					if( gfKeyState[ ALT ] )
						gfLoadGameUponEntry = TRUE;

					break;

				case 'o':
					gbHandledMainMenu = PREFERENCES;
					break;						

				case 's':
					gbHandledMainMenu = CREDITS;
					break;						
			}
		}
	}
}

void HandleHelpScreenInput()
{
	InputAtom									InputEvent;

	// Check for key
	while (DequeueEvent(&InputEvent) == TRUE)
	{
		switch( InputEvent.usEvent )
		{
			case KEY_UP:
				SetMainMenuExitScreen( INIT_SCREEN );
			break;
		}
	}
}


void ClearMainMenu()
{
	UINT32										 uiDestPitchBYTES;
	UINT8											 *pDestBuf;

	// CLEAR THE FRAME BUFFER
	pDestBuf = LockVideoSurface( FRAME_BUFFER, &uiDestPitchBYTES );
	memset(pDestBuf, 0, SCREEN_HEIGHT * uiDestPitchBYTES );
	UnLockVideoSurface( FRAME_BUFFER );
	InvalidateScreen( );
}


void SelectMainMenuBackGroundRegionCallBack(MOUSE_REGION * pRegion, INT32 iReason )
{
	if (iReason & MSYS_CALLBACK_REASON_INIT)
	{
	}
	else if(iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)
	{
//		if( gfDoHelpScreen )
//		{
//			SetMainMenuExitScreen( INIT_SCREEN );
//			gfDoHelpScreen = FALSE;
//		}
	}
	else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)
	{
/*
		if( gfDoHelpScreen )
		{
			SetMainMenuExitScreen( INIT_SCREEN );
			gfDoHelpScreen = FALSE;
		}
*/
	} 
}

void SetMainMenuExitScreen( UINT32 uiNewScreen )
{
	guiMainMenuExitScreen = uiNewScreen;

	//REmove the background region
	CreateDestroyBackGroundMouseMask( FALSE );

	gfMainMenuScreenExit = TRUE;
}


void CreateDestroyBackGroundMouseMask( BOOLEAN fCreate )
{
	static fRegionCreated = FALSE;

	if( fCreate )
	{
		if( fRegionCreated )
			return;

		// Make a mouse region
		MSYS_DefineRegion( &(gBackRegion), 0, 0, 640, 480, MSYS_PRIORITY_HIGHEST,
							 CURSOR_NORMAL, MSYS_NO_CALLBACK, SelectMainMenuBackGroundRegionCallBack ); 
		// Add region
		MSYS_AddRegion(&(gBackRegion) );

		fRegionCreated = TRUE;
	}
	else
	{
		if( !fRegionCreated )
			return;

		MSYS_RemoveRegion( &gBackRegion );
		fRegionCreated = FALSE;
	}
}


BOOLEAN CreateDestroyMainMenuButtons( BOOLEAN fCreate )
{
	static BOOLEAN fButtonsCreated = FALSE;
	INT32 cnt;
	SGPFILENAME filename;
	INT16 sSlot;
	INT32	iStartLoc=0;
#ifndef _DEBUG
	CHAR16	zText[512];
#endif


	if( fCreate )
	{
		if( fButtonsCreated )
			return( TRUE );

		//reset the variable that allows the user to ALT click on the continue save btn to load the save instantly
		gfLoadGameUponEntry = FALSE;

		// Load button images
	GetMLGFilename( filename, MLG_TITLETEXT );

#ifdef JA2DEMO
		iMenuImages[ NEW_GAME ]	= LoadButtonImage( filename, 17,17, 18, 19 ,-1 );
		sSlot = 17;
#else
		iMenuImages[ NEW_GAME ]	= LoadButtonImage( filename, 0,0, 1, 2 ,-1 );
		sSlot = 0;
#endif
		iMenuImages[ LOAD_GAME ] = UseLoadedButtonImage( iMenuImages[ NEW_GAME ] ,6,3,4,5,-1 );
		iMenuImages[ PREFERENCES ] = UseLoadedButtonImage( iMenuImages[ NEW_GAME ] ,7,7,8,9,-1 );
		iMenuImages[ CREDITS ] = UseLoadedButtonImage( iMenuImages[ NEW_GAME ] ,13,10,11,12,-1 );
		iMenuImages[ QUIT ] = UseLoadedButtonImage( iMenuImages[ NEW_GAME ] ,14,14,15,16,-1 );

		for ( cnt = 0; cnt < NUM_MENU_ITEMS; cnt++ )
		{
			switch( cnt )
			{
				case NEW_GAME:		gusMainMenuButtonWidths[cnt] = GetWidthOfButtonPic( (UINT16)iMenuImages[cnt], sSlot );	break;
				case LOAD_GAME:		gusMainMenuButtonWidths[cnt] = GetWidthOfButtonPic( (UINT16)iMenuImages[cnt], 3 );			break;
				case PREFERENCES:	gusMainMenuButtonWidths[cnt] = GetWidthOfButtonPic( (UINT16)iMenuImages[cnt], 7 );			break;
				case CREDITS:			gusMainMenuButtonWidths[cnt] = GetWidthOfButtonPic( (UINT16)iMenuImages[cnt], 10 );			break;
				case QUIT:				gusMainMenuButtonWidths[cnt] = GetWidthOfButtonPic( (UINT16)iMenuImages[cnt], 15 );			break;
			}
#ifdef TESTFOREIGNFONTS
			iMenuButtons[ cnt ] = QuickCreateButton( iMenuImages[ cnt ], (INT16)(320 - gusMainMenuButtonWidths[cnt]/2), (INT16)( 0 + ( cnt * 18 ) ),
												BUTTON_TOGGLE, MSYS_PRIORITY_HIGHEST,
												DEFAULT_MOVE_CALLBACK, MenuButtonCallback );
#else
			iMenuButtons[ cnt ] = QuickCreateButton( iMenuImages[ cnt ], (INT16)(320 - gusMainMenuButtonWidths[cnt]/2), (INT16)( MAINMENU_Y + ( cnt * MAINMENU_Y_SPACE ) ),
												BUTTON_TOGGLE, MSYS_PRIORITY_HIGHEST,
												DEFAULT_MOVE_CALLBACK, MenuButtonCallback );
#endif
			if ( iMenuButtons[ cnt ] == -1 )
			{
				return( FALSE );
			}
			ButtonList[ iMenuButtons[ cnt ] ]->UserData[0] = cnt;

			#ifndef _DEBUG
				//load up some info from the 'mainmenu.edt' file.  This makes sure the file is present.  The file is
				// 'marked' with a code that identifies the testers
				iStartLoc = MAINMENU_RECORD_SIZE * cnt;
				if( !LoadEncryptedDataFromFile(MAINMENU_TEXT_FILE, zText, iStartLoc, MAINMENU_RECORD_SIZE ) )
				{
					//the file was not able to be loaded properly
					SOLDIERTYPE *pSoldier = NULL;

					if( pSoldier->bActive != TRUE )
					{
						//something is very wrong
						pSoldier->bActive = pSoldier->bLife;
					}
				}
			#endif

		}


		fButtonsCreated = TRUE;
	}
	else
	{
		if( !fButtonsCreated )
			return( TRUE );

		// Delete images/buttons
		for ( cnt = 0; cnt < NUM_MENU_ITEMS; cnt++ )
		{
			RemoveButton( iMenuButtons[ cnt ] );
			UnloadButtonImage( iMenuImages[ cnt ] );
		}
		fButtonsCreated = FALSE;
	}

	return( TRUE );
}


void RenderMainMenu()
{
  HVOBJECT hPixHandle;

	//Get and display the background image
	GetVideoObject(&hPixHandle, guiMainMenuBackGroundImage );
  BltVideoObject( guiSAVEBUFFER, hPixHandle, 0, 0, 0, VO_BLT_SRCTRANSPARENCY,NULL);
  BltVideoObject( FRAME_BUFFER, hPixHandle, 0, 0, 0, VO_BLT_SRCTRANSPARENCY,NULL);

	GetVideoObject(&hPixHandle, guiJa2LogoImage );
  BltVideoObject( FRAME_BUFFER, hPixHandle, 0, 188, 15, VO_BLT_SRCTRANSPARENCY,NULL);
  BltVideoObject( guiSAVEBUFFER, hPixHandle, 0, 188, 15, VO_BLT_SRCTRANSPARENCY,NULL);


#ifdef TESTFOREIGNFONTS
	DrawTextToScreen( L"LARGEFONT1: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 105, 640, LARGEFONT1, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"SMALLFONT1: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 125, 640, SMALLFONT1, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"TINYFONT1: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 145, 640, TINYFONT1, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT12POINT1: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 165, 640, FONT12POINT1, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"COMPFONT: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 185, 640, COMPFONT, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"SMALLCOMPFONT: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 205, 640, SMALLCOMPFONT, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT10ROMAN: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 225, 640, FONT10ROMAN, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT12ROMAN: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 245, 640, FONT12ROMAN, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT14SANSERIF: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 255, 640, FONT14SANSERIF, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"MILITARYFONT: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 265, 640, MILITARYFONT1, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT10ARIAL: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 285, 640, FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT14ARIAL: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 305, 640, FONT14ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT12ARIAL: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 325, 640, FONT12ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT10ARIALBOLD: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 345, 640, FONT10ARIALBOLD, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"BLOCKFONT: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 365, 640, BLOCKFONT, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"BLOCKFONT2: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 385, 640, BLOCKFONT2, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT12ARIALFIXEDWIDTH: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 405, 640, FONT12ARIALFIXEDWIDTH, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT16ARIAL: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 425, 640, FONT16ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"BLOCKFONTNARROW: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 445, 640, BLOCKFONTNARROW, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
	DrawTextToScreen( L"FONT14HUMANIST: ����������������������������������������"/*gzCopyrightText[ 0 ]*/, 0, 465, 640, FONT14HUMANIST, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, LEFT_JUSTIFIED );	
#else
	DrawTextToScreen( gzCopyrightText[ 0 ], 0, 465, 640, FONT10ARIAL, FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, FALSE, CENTER_JUSTIFIED );	
#endif

	InvalidateRegion( 0, 0, 640, 480 );
}

void RestoreButtonBackGrounds()
{
	UINT8	cnt;

//	RestoreExternBackgroundRect( (UINT16)(320 - gusMainMenuButtonWidths[TITLE]/2), MAINMENU_TITLE_Y, gusMainMenuButtonWidths[TITLE], 23 );

#ifndef TESTFOREIGNFONTS
	for ( cnt = 0; cnt < NUM_MENU_ITEMS; cnt++ )
	{
		RestoreExternBackgroundRect( (UINT16)(320 - gusMainMenuButtonWidths[cnt]/2), (INT16)( MAINMENU_Y + ( cnt * MAINMENU_Y_SPACE )-1), (UINT16)(gusMainMenuButtonWidths[cnt]+1), 23 );
	}
#endif
}


