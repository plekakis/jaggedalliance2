#ifdef PRECOMPILEDHEADERS
	#include "TileEngine All.h"
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
	#include "animation control.h"
	#include "tile animation.h"
	#include "handle items.h"
	#include "lighteffects.h"
	#include "message.h"
	#include "isometric utils.h"
	#include "renderworld.h"
	#include "explosion control.h"
	#include "Random.h"
	#include "lighting.h"
	#include "Campaign Types.h"
#endif

#include "SaveLoadGame.h"


#define		NUM_LIGHT_EFFECT_SLOTS					25


// GLOBAL FOR LIGHT LISTING
LIGHTEFFECT				gLightEffectData[ NUM_LIGHT_EFFECT_SLOTS ];
UINT32						guiNumLightEffects = 0;


INT32 GetFreeLightEffect( void );
void RecountLightEffects( void );


INT32 GetFreeLightEffect( void )
{
	UINT32 uiCount;

	for(uiCount=0; uiCount < guiNumLightEffects; uiCount++)
	{
		if(( gLightEffectData[uiCount].fAllocated==FALSE ) )
			return( (INT32)uiCount );
	}

	if( guiNumLightEffects < NUM_LIGHT_EFFECT_SLOTS )
		return( (INT32) guiNumLightEffects++ );

	return( -1 );
}

void RecountLightEffects( void )
{
	INT32 uiCount;

	for(uiCount=guiNumLightEffects-1; (uiCount >=0) ; uiCount--)
	{
		if( ( gLightEffectData[uiCount].fAllocated ) )
		{
			guiNumLightEffects=(UINT32)(uiCount+1);
			break;
		}
	}
}


void UpdateLightingSprite( LIGHTEFFECT *pLight )
{
	CHAR8 LightName[20];
	// Build light....

	sprintf( LightName, "Light%d", pLight->bRadius );

	// Delete old one if one exists...
	if( pLight->iLight!=(-1) )
	{
		LightSpriteDestroy( pLight->iLight );	
		pLight->iLight = -1;
	}

	// Effect light.....
	if( ( pLight->iLight = LightSpriteCreate( LightName, 0 ) )==(-1))
	{
		// Could not light!
		return;
	}

	LightSpritePower( pLight->iLight, TRUE );
//	LightSpriteFake( pLight->iLight );
	LightSpritePosition( pLight->iLight, (INT16)( CenterX( pLight->sGridNo ) / CELL_X_SIZE ), (INT16)( CenterY( pLight->sGridNo ) / CELL_Y_SIZE ) );
}


INT32 NewLightEffect( INT16 sGridNo, INT8 bType )
{
	LIGHTEFFECT *pLight;
	INT32				iLightIndex;
	UINT8				ubDuration=0;
	UINT8				ubStartRadius=0;

	if( ( iLightIndex = GetFreeLightEffect() )==(-1) )
		return(-1);

	memset( &gLightEffectData[ iLightIndex ], 0, sizeof( LIGHTEFFECT ) );

	pLight = &gLightEffectData[ iLightIndex ];

	// Set some values...
	pLight->sGridNo									= sGridNo;
	pLight->bType										= bType;
	pLight->iLight									= -1;
	pLight->uiTimeOfLastUpdate			= GetWorldTotalSeconds( );

  switch( bType )
  {
		case LIGHT_FLARE_MARK_1:

			ubDuration				= 6;
			ubStartRadius			= 6;
			break;

  }

	pLight->ubDuration	= ubDuration;
	pLight->bRadius     = ubStartRadius;
	pLight->bAge				= 0;
	pLight->fAllocated  = TRUE;

	UpdateLightingSprite( pLight );

  // Handle sight here....
	AllTeamsLookForAll( FALSE );

	return( iLightIndex );
}



void RemoveLightEffectFromTile( INT16 sGridNo )
{
	LIGHTEFFECT *pLight;
	UINT32 cnt;

	// Set to unallocated....
  for ( cnt = 0; cnt < guiNumLightEffects; cnt++ )
  {
		pLight = &gLightEffectData[ cnt ];
		
		if ( pLight->fAllocated )
		{
			if ( pLight->sGridNo == sGridNo )
			{
				pLight->fAllocated = FALSE;

				// Remove light....
				if( pLight->iLight != (-1) )
				{
					LightSpriteDestroy( pLight->iLight );	
				}
				break;
			}
		}
	}

}


void DecayLightEffects( UINT32 uiTime )
{
	LIGHTEFFECT *pLight;
	UINT32 cnt, cnt2;
	BOOLEAN	  fDelete = FALSE;
  UINT16    usNumUpdates = 1;

  // age all active tear gas clouds, deactivate those that are just dispersing
  for ( cnt = 0; cnt < guiNumLightEffects; cnt++ )
  {
		pLight = &gLightEffectData[ cnt ];
		
		fDelete = FALSE;

		if ( pLight->fAllocated )
		{
			// ATE: Do this every so ofte, to acheive the effect we want...
			if ( ( uiTime - pLight->uiTimeOfLastUpdate ) > 350 )
			{
        usNumUpdates = ( UINT16 ) ( ( uiTime - pLight->uiTimeOfLastUpdate ) / 350 );

				pLight->uiTimeOfLastUpdate = uiTime;

        for ( cnt2 = 0; cnt2 < usNumUpdates; cnt2++ )
        {
				  pLight->bAge++;

				  // if this cloud remains effective (duration not reached)
				  if ( pLight->bAge < pLight->ubDuration)
				  {
					  // calculate the new cloud radius
					  // cloud expands by 1 every turn outdoors, and every other turn indoors
					  if ( ( pLight->bAge % 2 ) )
					  {
						  pLight->bRadius--;
					  }

					  if ( pLight->bRadius == 0 )
					  {
						  // Delete...
						  fDelete = TRUE;
              break;
					  }
					  else
					  {
						  UpdateLightingSprite( pLight );
					  }
				  }
				  else
				  {
					  fDelete = TRUE;
            break;
				  }
        }

				if ( fDelete )
				{
					pLight->fAllocated = FALSE;				

					if( pLight->iLight != (-1) )
					{
						LightSpriteDestroy( pLight->iLight );	
					}
				}

        // Handle sight here....
		    AllTeamsLookForAll( FALSE );
			}
    }
  }
}



BOOLEAN SaveLightEffectsToSaveGameFile( HWFILE hFile )
{
	/*
	UINT32	uiNumBytesWritten;
	UINT32	uiNumberOfLights=0;
	UINT32	uiCnt;

	//loop through and count the number of active slots
	for( uiCnt=0; uiCnt<guiNumLightEffects; uiCnt++)
	{
		if( gLightEffectData[ uiCnt ].fAllocated )
		{
			uiNumberOfLights++;
		}
	}

	//Save the Number of Light Effects
	FileWrite( hFile, &uiNumberOfLights, sizeof( UINT32 ), &uiNumBytesWritten );
	if( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		return( FALSE );
	}


	//if there are lights to save
	if( uiNumberOfLights != 0 )
	{
		//loop through and save each active slot
		for( uiCnt=0; uiCnt < guiNumLightEffects; uiCnt++)
		{
			if( gLightEffectData[ uiCnt ].fAllocated )
			{
				//Save the Light effect Data
				FileWrite( hFile, &gLightEffectData[ uiCnt ], sizeof( LIGHTEFFECT ), &uiNumBytesWritten );
				if( uiNumBytesWritten != sizeof( LIGHTEFFECT ) )
				{
					return( FALSE );
				}
			}
		}
	}
*/
	return( TRUE );
}


BOOLEAN LoadLightEffectsFromLoadGameFile( HWFILE hFile )
{
	UINT32	uiNumBytesRead;
	UINT32	uiCount;

	//no longer need to load Light effects.  They are now in temp files
	if( guiSaveGameVersion < 76 )
	{
		memset( gLightEffectData, 0, sizeof( LIGHTEFFECT ) *  NUM_LIGHT_EFFECT_SLOTS );

		//Load the Number of Light Effects
		FileRead( hFile, &guiNumLightEffects, sizeof( UINT32 ), &uiNumBytesRead );
		if( uiNumBytesRead != sizeof( UINT32 ) )
		{
			return( FALSE );
		}

		//if there are lights saved.
		if( guiNumLightEffects != 0 )
		{
			//loop through and apply the light effects to the map
			for(uiCount=0; uiCount < guiNumLightEffects; uiCount++)
			{
				//Load the Light effect Data
				FileRead( hFile, &gLightEffectData[ uiCount ], sizeof( LIGHTEFFECT ), &uiNumBytesRead );
				if( uiNumBytesRead != sizeof( LIGHTEFFECT ) )
				{
					return( FALSE );
				}
			}
		}


		//loop through and apply the light effects to the map
		for(uiCount=0; uiCount < guiNumLightEffects; uiCount++)
		{
			if( gLightEffectData[uiCount].fAllocated )
				UpdateLightingSprite( &( gLightEffectData[uiCount] ) );			
		}
	}

	return( TRUE );
}



BOOLEAN SaveLightEffectsToMapTempFile( INT16 sMapX, INT16 sMapY, INT8 bMapZ )
{
	UINT32	uiNumLightEffects=0;
	HWFILE	hFile;
	UINT32	uiNumBytesWritten=0;
	CHAR8		zMapName[ 128 ];
	UINT32	uiCnt;

	//get the name of the map
	GetMapTempFileName( SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS, zMapName, sMapX, sMapY, bMapZ );

	//delete file the file.
	FileDelete( zMapName );

	//loop through and count the number of Light effects
	for( uiCnt=0; uiCnt<guiNumLightEffects; uiCnt++)
	{
		if( gLightEffectData[ uiCnt ].fAllocated )
			uiNumLightEffects++;
	}

	//if there are no Light effects
	if( uiNumLightEffects == 0 )
	{
		//set the fact that there are no Light effects for this sector
		ReSetSectorFlag( sMapX, sMapY, bMapZ, SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS );

		return( TRUE );
	}

	//Open the file for writing
	hFile = FileOpen( zMapName, FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS, FALSE );
	if( hFile == 0 )
	{
		//Error opening map modification file
		return( FALSE );
	}


	//Save the Number of Light Effects
	FileWrite( hFile, &uiNumLightEffects, sizeof( UINT32 ), &uiNumBytesWritten );
	if( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		//Close the file
		FileClose( hFile );

		return( FALSE );
	}


	//loop through and save the number of Light effects
	for( uiCnt=0; uiCnt < guiNumLightEffects; uiCnt++)
	{
		//if the Light is active
		if( gLightEffectData[ uiCnt ].fAllocated )
		{
			//Save the Light effect Data
			FileWrite( hFile, &gLightEffectData[ uiCnt ], sizeof( LIGHTEFFECT ), &uiNumBytesWritten );
			if( uiNumBytesWritten != sizeof( LIGHTEFFECT ) )
			{
				//Close the file
				FileClose( hFile );

				return( FALSE );
			}
		}
	}

	//Close the file
	FileClose( hFile );

	SetSectorFlag( sMapX, sMapY, bMapZ, SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS );

	return( TRUE );
}



BOOLEAN LoadLightEffectsFromMapTempFile( INT16 sMapX, INT16 sMapY, INT8 bMapZ )
{
	UINT32	uiNumBytesRead;
	UINT32	uiCount;
	UINT32	uiCnt=0;
	HWFILE	hFile;
	UINT32	uiNumBytesWritten=0;
	CHAR8		zMapName[ 128 ];

	GetMapTempFileName( SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS, zMapName, sMapX, sMapY, bMapZ );

	//Open the file for reading, Create it if it doesnt exist
	hFile = FileOpen( zMapName, FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE );
	if( hFile == 0 )
	{
		//Error opening file
		return( FALSE );
	}

	//Clear out the old list
	ResetLightEffects();


	//Load the Number of Light Effects
	FileRead( hFile, &guiNumLightEffects, sizeof( UINT32 ), &uiNumBytesRead );
	if( uiNumBytesRead != sizeof( UINT32 ) )
	{
		FileClose( hFile );
		return( FALSE );
	}

	//loop through and load the list
	for( uiCnt=0; uiCnt<guiNumLightEffects;uiCnt++)
	{
		//Load the Light effect Data
		FileRead( hFile, &gLightEffectData[ uiCnt ], sizeof( LIGHTEFFECT ), &uiNumBytesRead );
		if( uiNumBytesRead != sizeof( LIGHTEFFECT ) )
		{
			FileClose( hFile );
			return( FALSE );
		}
	}


	//loop through and apply the light effects to the map
	for(uiCount=0; uiCount < guiNumLightEffects; uiCount++)
	{
		if( gLightEffectData[uiCount].fAllocated )
			UpdateLightingSprite( &( gLightEffectData[uiCount] ) );			
	}

	FileClose( hFile );

	return( TRUE );
}


void ResetLightEffects()
{
	//Clear out the old list
	memset( gLightEffectData, 0, sizeof( LIGHTEFFECT ) * NUM_LIGHT_EFFECT_SLOTS );
	guiNumLightEffects = 0;
}
