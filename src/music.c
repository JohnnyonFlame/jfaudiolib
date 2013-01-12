/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/**********************************************************************
   module: MUSIC.C

   author: James R. Dose
   date:   March 25, 1994

   Device independant music playback routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "sndcards.h"
#include "drivers.h"
#include "music.h"
#include "midi.h"
#include "ll_man.h"

#ifdef USE_SDLMIXER
#include <SDL/SDL_mixer.h>
#include "errno.h"
#include <unistd.h>
#include <fcntl.h>
#endif

#ifdef USE_WILDMIDI
#include <wildmidi_lib.h>

int WILD_Initialized=0;
unsigned char *MUS_Buffer;
int MUS_Handle = 0;

void *midi_ptr = NULL;
struct _WM_Info * wm_info = NULL;

#define WMSamples 4096
#define WMSampleSize sizeof(MUS_Buffer)
#endif

#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

int MUSIC_ErrorCode = MUSIC_Ok;

#ifdef USE_SDLMIXER
unsigned char *song_rw = NULL;
Mix_Music *music;
int p_music = 0;
int volume_m = 128;
#endif

static midifuncs MUSIC_MidiFunctions;

//static int       MUSIC_FadeLength;
//static int       MUSIC_FadeRate;
//static unsigned  MUSIC_CurrentFadeVolume;
//static unsigned  MUSIC_LastFadeVolume;
//static int       MUSIC_EndingFadeVolume;
//static task     *MUSIC_FadeTask = NULL;

#define MUSIC_SetErrorCode( status ) \
   MUSIC_ErrorCode = ( status );

/*---------------------------------------------------------------------
   Function: MUSIC_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

const char *MUSIC_ErrorString
   (
   int ErrorNumber
   )

   {
   const char *ErrorString;

   switch( ErrorNumber )
      {
      case MUSIC_Warning :
      case MUSIC_Error :
         ErrorString = MUSIC_ErrorString( MUSIC_ErrorCode );
         break;

      case MUSIC_Ok :
         ErrorString = "Music ok.";
         break;

      case MUSIC_DriverError :
         ErrorString = SoundDriver_MIDI_ErrorString(SoundDriver_MIDI_GetError());
         break;

      case MUSIC_InvalidCard :
         ErrorString = "Invalid Music device.";
         break;

      case MUSIC_MidiError :
         ErrorString = "Error playing MIDI file.";
         break;

      default :
         ErrorString = "Unknown Music error code.";
         break;
      }

   return( ErrorString );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int MUSIC_Init
   (
   int SoundCard,
   int Address
   )

   {
   int i;
   int status;

   for( i = 0; i < 128; i++ )
      {
      MIDI_PatchMap[ i ] = i;
      }

	if (SoundCard == ASS_AutoDetect) {
#if 0 //defined __APPLE__
		SoundCard = ASS_CoreAudio;
#elif defined _WIN32
		SoundCard = ASS_WinMM;
#elif defined HAVE_ALSA
        SoundCard = ASS_ALSA;
#elif defined HAVE_FLUIDSYNTH
		SoundCard = ASS_FluidSynth;
#else
		SoundCard = ASS_NoSound;
#endif
	}

	if (SoundCard < 0 || SoundCard >= ASS_NumSoundCards) {
		MUSIC_ErrorCode = MUSIC_InvalidCard;
		return MUSIC_Error;
	}

    if (!SoundDriver_IsMIDISupported(SoundCard))
      {
      MUSIC_ErrorCode = MUSIC_InvalidCard;
      return MUSIC_Error;
      }

   ASS_MIDISoundDriver = SoundCard;
   
   status = SoundDriver_MIDI_Init(&MUSIC_MidiFunctions);
   if (status != MUSIC_Ok)
      {
      MUSIC_ErrorCode = MUSIC_DriverError;
      return MUSIC_Error;
      }

   MIDI_SetMidiFuncs( &MUSIC_MidiFunctions );

   return MUSIC_Ok;
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int MUSIC_Shutdown
   (
   void
   )

   {
   MIDI_StopSong();

   /*if ( MUSIC_FadeTask != NULL )
      {
      MUSIC_StopFade();
      }*/

   SoundDriver_MIDI_Shutdown();

   return MUSIC_Ok;
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetMaxFMMidiChannel

   Sets the maximum MIDI channel that FM cards respond to.
---------------------------------------------------------------------*/

void MUSIC_SetMaxFMMidiChannel
   (
   int channel
   )

   {
   //AL_SetMaxMidiChannel( channel );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetVolume

   Sets the volume of music playback.
---------------------------------------------------------------------*/

void MUSIC_SetVolume
   (
   int volume
   )

   {

   
   #ifdef USE_SDLMIXER
   volume = volume >> 1;
   
   volume = max( 0, volume );
   volume = min( volume, 128 );
   
   volume_m = volume;

 	 //Mix_VolumeMusic(volume);
   #elif USE_WILDMIDI
   volume = volume >> 1;

   volume = max( 0, volume );
   volume = min( volume, 127 );

   if (WILD_Initialized)
  	 WildMidi_MasterVolume(volume);


	 #else

   volume = max( 0, volume );
   volume = min( volume, 255 );
	#endif
   

   /*if ( MUSIC_SoundDevice != -1 )
      {
      MIDI_SetVolume( volume );
      }*/
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetMidiChannelVolume

   Sets the volume of music playback on the specified MIDI channel.
---------------------------------------------------------------------*/

void MUSIC_SetMidiChannelVolume
   (
   int channel,
   int volume
   )

   {
   MIDI_SetUserChannelVolume( channel, volume );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_ResetMidiChannelVolumes

   Sets the volume of music playback on all MIDI channels to full volume.
---------------------------------------------------------------------*/

void MUSIC_ResetMidiChannelVolumes
   (
   void
   )

   {
   MIDI_ResetUserChannelVolume();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetVolume

   Returns the volume of music playback.
---------------------------------------------------------------------*/

int MUSIC_GetVolume
   (
   void
   )

   {
   /*if ( MUSIC_SoundDevice == -1 )
      {
      return( 0 );
      }*/
   return( MIDI_GetVolume() );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetLoopFlag

   Set whether the music will loop or end when it reaches the end of
   the song.
---------------------------------------------------------------------*/

void MUSIC_SetLoopFlag
   (
   int loopflag
   )

   {
   MIDI_SetLoopFlag( loopflag );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SongPlaying

   Returns whether there is a song playing.
---------------------------------------------------------------------*/

int MUSIC_SongPlaying
   (
   void
   )

   {
   return( MIDI_SongPlaying() );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Continue

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MUSIC_Continue
   (
   void
   )

   {
   MIDI_ContinueSong();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_Pause

   Pauses playback of a song.
---------------------------------------------------------------------*/

void MUSIC_Pause
   (
   void
   )

   {
   MIDI_PauseSong();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_StopSong

   Stops playback of current song.
---------------------------------------------------------------------*/

#ifdef USE_SDLMIXER
void MIDI_StopSong
( 
void
)
{
	p_music = 0;
	Mix_HaltMusic();

	if (music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music   = NULL;
	}

	if (song_rw)
		{
		  free(song_rw);
		  song_rw = NULL;
		}
}
#endif

#ifdef USE_WILDMIDI
int WILDLOCK = 0;
#endif

void CALLBACK_DONONE(char **ptr, unsigned int *length)
{
#ifdef USE_WILDMIDI

	if (midi_ptr)
		{
			while (WILDLOCK)
				usleep(5000);

			wm_info  = WildMidi_GetInfo( midi_ptr );

			*ptr = MUS_Buffer;
			*length = WildMidi_GetOutput (midi_ptr, MUS_Buffer, WMSamples);

			if (*length == 0)
				{
					unsigned long int sample_seeker = wm_info->approx_total_samples-wm_info->current_sample;
					WildMidi_FastSeek (midi_ptr, &sample_seeker);
				}
		}
#endif
}

int MUSIC_StopSong
   (
   void
   )

   {
#ifdef USE_WILDMIDI
	WILDLOCK = 1;
	 if (MUS_Handle)
		 {
		 MV_Kill(MUS_Handle);
		 MUS_Handle = 0;
		 }

  if (midi_ptr)
 	 {
 	 WildMidi_Close (midi_ptr);
 	 midi_ptr = NULL;
 	 }
  WILDLOCK = 0;
#else
   MIDI_StopSong();
#endif
   MUSIC_SetErrorCode( MUSIC_Ok );
   return( MUSIC_Ok );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_PlaySong

   Begins playback of MIDI song.
---------------------------------------------------------------------*/

int MUSIC_PlaySong
   (
   unsigned char *song,
   unsigned int length,
   int loopflag
   )

   {
   int status;

   MIDI_StopSong();
#ifdef USE_SDLMIXER
   WILDLOCK = 1;
   song_rw = malloc(length);
   song_rw = memcpy(song_rw, song, length);
   
   SDL_RWops *rw = SDL_RWFromMem(song_rw ,length);
   if (rw == NULL)
   {
	   initprintf( "Error allocating RWops: %s\n", SDL_GetError() );  
   }
   initprintf("Allocated file into RAM, trying to load it...\n");
   music = Mix_LoadMUS_RW(rw);
   if (music==NULL)
   {
	   initprintf( "Error opening song: %s\n", Mix_GetError() );      
	   initprintf( "Error opening song: %s\n", SDL_GetError() );      
	   MUSIC_SetErrorCode( MUSIC_MidiError );
	   return( MUSIC_Warning );
   }
   Mix_VolumeMusic(volume_m);
   
   if(Mix_PlayMusic(music, -loopflag)==-1) {
       printf("ERR MUS: %s\n", Mix_GetError());
   }
   p_music = 1;
   //SDL_FreeRW(rw);
#elif USE_WILDMIDI
   if (memcmp(song, "MThd", 4))
  	 return (MUSIC_Warning);

   if (!WILD_Initialized)
  	 if (WildMidi_Init ("/etc/timidity.cfg", 44100, NULL) == -1)
  		 {
  			 initprintf("Failed initting WildMidi.\n");
  			 MUSIC_SetErrorCode(MUSIC_MidiError);

  			 return (MUSIC_Warning);
  		 }
  	 else
  		 WILD_Initialized = 1;

	 if (!MUS_Buffer)
		 MUS_Buffer = malloc(WMSamples * WMSampleSize);

	 if (MUS_Handle)
		 {
		 MV_Kill(MUS_Handle);
		 MUS_Handle = 0;
		 }

   if (midi_ptr)
  	 {
  	 WildMidi_Close (midi_ptr);
  	 midi_ptr = NULL;
  	 }

   midi_ptr = WildMidi_OpenBuffer( song, length );
   if (!midi_ptr)
		 {
			 initprintf("Failed opening buffer for stream.\n");
			 MUSIC_SetErrorCode(MUSIC_MidiError);

			 return (MUSIC_Warning);
		 }
   wm_info  = WildMidi_GetInfo( midi_ptr );
   if (!wm_info)
		 {
			 initprintf("Failed getting midi file info.\n");
			 MUSIC_SetErrorCode(MUSIC_MidiError);

			 return (MUSIC_Warning);
		 }

   WILDLOCK = 0;

   //SoundDriver_PCM_BeginPlayback(MUS_Buffer, output_result,	1, CALLBACK_DONONE );
   MUS_Handle = MV_StartDemandFeedPlayback16( CALLBACK_DONONE,
            44100, 0, 255, 255, 255,
            0x7fffffffl, 1 );

#else
   status = MIDI_PlaySong( song, loopflag );
   if ( status != MIDI_Ok )
      {
      MUSIC_SetErrorCode( MUSIC_MidiError );
      return( MUSIC_Warning );
      }
#endif
   return( MUSIC_Ok );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetContext

   Sets the song context.
---------------------------------------------------------------------*/

void MUSIC_SetContext
   (
   int context
   )

   {
   MIDI_SetContext( context );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetContext

   Returns the current song context.
---------------------------------------------------------------------*/

int MUSIC_GetContext
   (
   void
   )

   {
   return MIDI_GetContext();
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTick

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTick
   (
   unsigned long PositionInTicks
   )

   {
   MIDI_SetSongTick( PositionInTicks );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTime

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTime
   (
   unsigned long milliseconds
   )

   {
   MIDI_SetSongTime( milliseconds );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongPosition

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongPosition
   (
   int measure,
   int beat,
   int tick
   )

   {
   MIDI_SetSongPosition( measure, beat, tick );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongPosition

   Returns the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_GetSongPosition
   (
   songposition *pos
   )

   {
   MIDI_GetSongPosition( pos );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongLength

   Returns the length of the song.
---------------------------------------------------------------------*/

void MUSIC_GetSongLength
   (
   songposition *pos
   )

   {
   MIDI_GetSongLength( pos );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_FadeRoutine

   Fades music volume from current level to another over a specified
   period of time.
---------------------------------------------------------------------*/
/*
static void MUSIC_FadeRoutine
   (
   task *Task
   )

   {
   int volume;

   MUSIC_CurrentFadeVolume += MUSIC_FadeRate;
   if ( MUSIC_FadeLength == 0 )
      {
      MIDI_SetVolume( MUSIC_EndingFadeVolume );
      TS_Terminate( Task );
      MUSIC_FadeTask = NULL;
      }
   else
      {
      MUSIC_FadeLength--;
//      if ( ( MUSIC_SoundDevice == GenMidi ) &&
//         ( ( MUSIC_FadeLength % 12 ) != 0 ) )
//         {
//         return;
//         }

      volume = MUSIC_CurrentFadeVolume >> 7;
      if ( MUSIC_LastFadeVolume != volume )
         {
         MUSIC_LastFadeVolume = volume;
         MIDI_SetVolume( volume );
         }
      }
   }
*/

/*---------------------------------------------------------------------
   Function: MUSIC_FadeVolume

   Fades music volume from current level to another over a specified
   period of time.
---------------------------------------------------------------------*/

int MUSIC_FadeVolume
   (
   int tovolume,
   int milliseconds
   )

   {
   /*int fromvolume;

   if ( ( MUSIC_SoundDevice == ProAudioSpectrum ) ||
      ( MUSIC_SoundDevice == SoundMan16 ) ||
      ( MUSIC_SoundDevice == GenMidi ) ||
      ( MUSIC_SoundDevice == SoundScape ) ||
      ( MUSIC_SoundDevice == SoundCanvas ) )
      {
      MIDI_SetVolume( tovolume );
      return( MUSIC_Ok );
      }

   if ( MUSIC_FadeTask != NULL )
      {
      MUSIC_StopFade();
      }

   tovolume = max( 0, tovolume );
   tovolume = min( 255, tovolume );
   fromvolume = MUSIC_GetVolume();

   MUSIC_FadeLength = milliseconds / 25;
   MUSIC_FadeRate   = ( ( tovolume - fromvolume ) << 7 ) / MUSIC_FadeLength;
   MUSIC_LastFadeVolume = fromvolume;
   MUSIC_CurrentFadeVolume = fromvolume << 7;
   MUSIC_EndingFadeVolume = tovolume;

   MUSIC_FadeTask = TS_ScheduleTask( MUSIC_FadeRoutine, 40, 1, NULL );
   if ( MUSIC_FadeTask == NULL )
      {
      MUSIC_SetErrorCode( MUSIC_TaskManError );
      return( MUSIC_Warning );
      }

   TS_Dispatch();*/
   return( MUSIC_Ok );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_FadeActive

   Returns whether the fade routine is active.
---------------------------------------------------------------------*/

int MUSIC_FadeActive
   (
   void
   )

   {
   return 0;//( MUSIC_FadeTask != NULL );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_StopFade

   Stops fading the music.
---------------------------------------------------------------------*/

void MUSIC_StopFade
   (
   void
   )

   {
   /*if ( MUSIC_FadeTask != NULL )
      {
      TS_Terminate( MUSIC_FadeTask );
      MUSIC_FadeTask = NULL;
      }*/
   }


/*---------------------------------------------------------------------
   Function: MUSIC_RerouteMidiChannel

   Sets callback function to reroute MIDI commands from specified
   function.
---------------------------------------------------------------------*/

void MUSIC_RerouteMidiChannel
   (
   int channel,
   int ( *function )( int event, int c1, int c2 )
   )

   {
   MIDI_RerouteMidiChannel( channel, function );
   }


/*---------------------------------------------------------------------
   Function: MUSIC_RegisterTimbreBank

   Halts playback of all sounds.
---------------------------------------------------------------------*/

void MUSIC_RegisterTimbreBank
   (
   unsigned char *timbres
   )

   {
   //AL_RegisterTimbreBank( timbres );
   }
