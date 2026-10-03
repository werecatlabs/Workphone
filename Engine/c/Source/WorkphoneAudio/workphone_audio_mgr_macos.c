/**
 * @file workphone_audio_mgr_macos.c
 * @brief macOS audio backend using Core Audio / Audio Queue Services.
 */

#include "workphone_audio_mgr.h"

#if WP_AUDIO_PLATFORM_MACOS

#    include <AudioToolbox/AudioToolbox.h>
#    include <CoreAudio/CoreAudio.h>
#    include <AudioUnit/AudioUnit.h>

/* -------------------------------------------------------------------------
 * Internal types
 * ---------------------------------------------------------------------- */

struct wp_audio_macos_sound
{
	AudioQueueRef       queue;
	AudioQueueBufferRef *buffers;
	wp_u32              buffer_count;
	wp_s32              looping;
	wp_f32              volume;
	wp_f32              pitch;
	wp_u8               *audio_data;
	wp_u32              audio_data_size;
	wp_s32              is_playing;
};

static AudioQueueRef g_default_queue = NULL;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

static wp_s32  macos_init( void );
static void    macos_shutdown( void );
static void    macos_update( void *mgr );
static void   *macos_sound_load( const wp_c8 *filepath, wp_s32 loop );
static void    macos_sound_unload( void *handle );
static void    macos_sound_play( void *handle );
static void    macos_sound_stop( void *handle );
static void    macos_sound_pause( void *handle );
static void    macos_sound_resume( void *handle );
static void    macos_sound_set_volume( void *handle, wp_f32 volume );
static wp_f32  macos_sound_get_volume( void *handle );
static void    macos_sound_set_pitch( void *handle, wp_f32 pitch );
static wp_f32  macos_sound_get_pitch( void *handle );
static void    macos_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z );
static wp_s32  macos_sound_is_playing( void *handle );

/* -------------------------------------------------------------------------
 * Backend interface
 * ---------------------------------------------------------------------- */

static const wp_audio_mgr_backend g_backend = { macos_init,
												   macos_shutdown,
												   macos_update,
												   macos_sound_load,
												   macos_sound_unload,
												   macos_sound_play,
												   macos_sound_stop,
												   macos_sound_pause,
												   macos_sound_resume,
												   macos_sound_set_volume,
												   macos_sound_get_volume,
												   macos_sound_set_pitch,
												   macos_sound_get_pitch,
												   macos_sound_set_position,
												   macos_sound_is_playing };

const wp_audio_mgr_backend *wp_audio_mgr_get_backend( void )
{
	return &g_backend;
}

/* -------------------------------------------------------------------------
 * Audio Queue callback
 * ---------------------------------------------------------------------- */

static void macos_audio_queue_callback( void *userdata, AudioQueueRef queue,
										 AudioQueueBufferRef buffer )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)userdata;
	(void)queue;

	if( !sound || !buffer )
		return;

	if( sound->looping && sound->is_playing )
	{
		/* Re-queue the buffer for looping */
		AudioQueueEnqueueBuffer( sound->queue, buffer, 0, NULL );
	}
	else
	{
		sound->is_playing = 0;
	}
}

/* -------------------------------------------------------------------------
 * Backend implementations
 * ---------------------------------------------------------------------- */

static wp_s32 macos_init( void )
{
	if( g_default_queue )
		return 1;  /* Already initialised */

	/* Audio Queue is lazily initialised per-sound on macOS */
	return 1;
}

static void macos_shutdown( void )
{
	if( g_default_queue )
	{
		AudioQueueStop( g_default_queue, 1 );
		AudioQueueDispose( g_default_queue, 1 );
		g_default_queue = NULL;
	}
}

static void macos_update( void *mgr )
{
	(void)mgr;
	/* Audio Queue handles updates internally */
}

static void *macos_sound_load( const wp_c8 *filepath, wp_s32 loop )
{
	struct wp_audio_macos_sound *sound;
	CFURLRef file_url;
	AudioFileID audio_file;
	OSStatus status;
	AudioStreamBasicDescription format;
	UInt32 property_size;

	if( !filepath )
		return NULL;

	sound = (struct wp_audio_macos_sound *)calloc( 1, sizeof( struct wp_audio_macos_sound ) );
	if( !sound )
		return NULL;

	sound->looping     = loop;
	sound->volume      = 1.0f;
	sound->pitch       = 1.0f;
	sound->is_playing  = 0;

	/* Create file URL */
	file_url = CFURLCreateFromFileSystemRepresentation( NULL, (const UInt8 *)filepath,
														 strlen( filepath ), 0 );
	if( !file_url )
	{
		free( sound );
		return NULL;
	}

	/* Open audio file */
	status = AudioFileOpenURL( file_url, kAudioFileReadPermission, 0, &audio_file );
	CFRelease( file_url );

	if( status != noErr )
	{
		free( sound );
		return NULL;
	}

	/* Get audio format */
	property_size = sizeof( format );
	status = AudioFileGetProperty( audio_file, kAudioFilePropertyDataFormat, &property_size,
									&format );
	if( status != noErr )
	{
		AudioFileClose( audio_file );
		free( sound );
		return NULL;
	}

	/* Read audio data */
	UInt64 data_size = 0;
	property_size    = sizeof( data_size );
	AudioFileGetProperty( audio_file, kAudioFilePropertyAudioDataByteSize, &property_size,
						   &data_size );

	sound->audio_data_size = (wp_u32)data_size;
	sound->audio_data      = (wp_u8 *)malloc( sound->audio_data_size );

	if( !sound->audio_data )
	{
		AudioFileClose( audio_file );
		free( sound );
		return NULL;
	}

	UInt32 bytes_read = sound->audio_data_size;
	status = AudioFileReadBytes( audio_file, 0, 0, &bytes_read, sound->audio_data );
	AudioFileClose( audio_file );

	if( status != noErr )
	{
		free( sound->audio_data );
		free( sound );
		return NULL;
	}

	/* Create Audio Queue */
	status = AudioQueueNewOutput( &format, macos_audio_queue_callback, sound, NULL, NULL, 0,
								   &sound->queue );
	if( status != noErr )
	{
		free( sound->audio_data );
		free( sound );
		return NULL;
	}

	/* Allocate and fill buffers */
	sound->buffer_count = 4;
	sound->buffers      = (AudioQueueBufferRef *)malloc(
		sound->buffer_count * sizeof( AudioQueueBufferRef ) );

	UInt32 buffer_size = sound->audio_data_size / sound->buffer_count;
	wp_u32 i;

	for( i = 0; i < sound->buffer_count; ++i )
	{
		AudioQueueAllocateBuffer( sound->queue, buffer_size, &sound->buffers[i] );
		memcpy( sound->buffers[i]->mAudioData, sound->audio_data + ( i * buffer_size ),
				buffer_size );
		sound->buffers[i]->mAudioDataByteSize = buffer_size;

		if( i == 0 )
		{
			AudioQueueEnqueueBuffer( sound->queue, sound->buffers[i], 0, NULL );
		}
	}

	/* Set initial volume */
	AudioQueueSetParameter( sound->queue, kAudioQueueParam_Volume, sound->volume );

	return sound;
}

static void macos_sound_unload( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound )
		return;

	if( sound->queue )
	{
		AudioQueueStop( sound->queue, 1 );
		AudioQueueDispose( sound->queue, 1 );
	}

	if( sound->buffers )
		free( sound->buffers );

	if( sound->audio_data )
		free( sound->audio_data );

	free( sound );
}

static void macos_sound_play( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound || !sound->queue )
		return;

	AudioQueueStart( sound->queue, NULL );
	sound->is_playing = 1;
}

static void macos_sound_stop( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound || !sound->queue )
		return;

	AudioQueueStop( sound->queue, 1 );
	sound->is_playing = 0;
}

static void macos_sound_pause( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound || !sound->queue )
		return;

	AudioQueuePause( sound->queue );
}

static void macos_sound_resume( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound || !sound->queue )
		return;

	AudioQueueStart( sound->queue, NULL );
}

static void macos_sound_set_volume( void *handle, wp_f32 volume )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound )
		return;

	sound->volume = volume;
	if( sound->queue )
		AudioQueueSetParameter( sound->queue, kAudioQueueParam_Volume, volume );
}

static wp_f32 macos_sound_get_volume( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound )
		return 0.0f;

	return sound->volume;
}

static void macos_sound_set_pitch( void *handle, wp_f32 pitch )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound )
		return;

	sound->pitch = pitch;
	/* Note: AudioQueue doesn't support pitch directly.
	 * Would need to use AudioUnit for pitch shifting. */
	(void)sound->pitch;
}

static wp_f32 macos_sound_get_pitch( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound )
		return 1.0f;

	return sound->pitch;
}

static void macos_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z )
{
	(void)handle;
	(void)x;
	(void)y;
	(void)z;
	/* 3D audio positioning would be implemented here using OpenAL or Core 3D Audio */
}

static wp_s32 macos_sound_is_playing( void *handle )
{
	struct wp_audio_macos_sound *sound = (struct wp_audio_macos_sound *)handle;
	if( !sound )
		return 0;

	return sound->is_playing;
}

#endif /* WP_AUDIO_PLATFORM_MACOS */
