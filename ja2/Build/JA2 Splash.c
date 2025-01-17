#ifdef PRECOMPILEDHEADERS
	#include "JA2 All.h"
#else
	//add includes here!
	#include "Types.h"
	#include "vsurface.h"
	#include "TopicIDs.h"
	#include "TopicOps.h"
	#include "LibraryDataBase.h"
#endif

UINT32 guiSplashFrameFade = 10;
UINT32 guiSplashStartTime = 0;
extern HVSURFACE ghFrameBuffer;

//Simply create videosurface, load image, and draw it to the screen.
void InitJA2SplashScreen()
{
	UINT32 uiLogoID = 0;
	STRING512			CurrentDir;
	STRING512			DataDir;
	HVSURFACE hVSurface;
	VSURFACE_DESC VSurfaceDesc;
	INT32 i = 0;

	InitializeJA2Clock();
	//InitializeJA2TimerID();
	// Get Executable Directory
	GetExecutableDirectory( CurrentDir );

	// Adjust Current Dir
	sprintf( DataDir, "%s\\Data", CurrentDir );
	if ( !SetFileManCurrentDirectory( DataDir ) )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Could not find data directory, shutting down");
		return;
	}

	//Initialize the file database
	InitializeFileDatabase( gGameLibaries, NUMBER_OF_LIBRARIES );

#if !defined( ENGLISH ) && defined( JA2TESTVERSION )
	memset( &VSurfaceDesc, 0, sizeof( VSURFACE_DESC ) );
	VSurfaceDesc.fCreateFlags = VSURFACE_CREATE_FROMFILE | VSURFACE_SYSTEM_MEM_USAGE;
	sprintf( VSurfaceDesc.ImageFile, "LOADSCREENS\\Notification.sti" );
	if( !AddVideoSurface( &VSurfaceDesc, &uiLogoID ) )
	{	
		AssertMsg( 0, String( "Failed to load %s", VSurfaceDesc.ImageFile ) );
		return;
	}
	GetVideoSurface(&hVSurface, uiLogoID );
			BltVideoSurfaceToVideoSurface( ghFrameBuffer, hVSurface, 0, 0, 0, 0, NULL );
	DeleteVideoSurfaceFromIndex( uiLogoID );


	InvalidateScreen();
	RefreshScreen( NULL );

	guiSplashStartTime = GetJA2Clock();
	while( i < 60 * 15 )//guiSplashStartTime + 15000 > GetJA2Clock() )
	{
		//Allow the user to pick his bum.
		InvalidateScreen();
		RefreshScreen( NULL );
		i++;
	}
#endif
	
	#ifdef ENGLISH
		ClearMainMenu();
	#else
		{

			memset( &VSurfaceDesc, 0, sizeof( VSURFACE_DESC ) );
			VSurfaceDesc.fCreateFlags = VSURFACE_CREATE_FROMFILE | VSURFACE_SYSTEM_MEM_USAGE;
			GetMLGFilename( VSurfaceDesc.ImageFile, MLG_SPLASH );
			if( !AddVideoSurface( &VSurfaceDesc, &uiLogoID ) )
			{	
				AssertMsg( 0, String( "Failed to load %s", VSurfaceDesc.ImageFile ) );
				return;
			}

			GetVideoSurface( &hVSurface, uiLogoID );
			BltVideoSurfaceToVideoSurface( ghFrameBuffer, hVSurface, 0, 0, 0, 0, NULL );
			DeleteVideoSurfaceFromIndex( uiLogoID );
		}
	#endif


	InvalidateScreen();
	RefreshScreen( NULL );

	guiSplashStartTime = GetJA2Clock();
}
