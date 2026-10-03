/**
 * @file workphone_audio_mgr_win32.c
 * @brief Windows audio backend using XAudio2.
 */

#include "workphone_audio_mgr.h"

#if WP_AUDIO_PLATFORM_WINDOWS

#    define WIN32_LEAN_AND_MEAN
#    ifdef _WIN32_WINNT
#        undef _WIN32_WINNT
#    endif
#    define _WIN32_WINNT 0x0602
#    include <windows.h>
#    include <xaudio2.h>

/* -------------------------------------------------------------------------
 * Internal types
 * ---------------------------------------------------------------------- */

struct wp_audio_win32_sound
{
	IXAudio2SourceVoice *voice;
	XAUDIO2_BUFFER      *buffer;
	wp_u8               *audio_data;
	wp_u32               audio_data_size;
	wp_s32               looping;
	wp_f32               volume;
	wp_f32               pitch;
};

static IXAudio2 *g_xaudio2        = NULL;
static IXAudio2MasteringVoice *g_master_voice = NULL;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

static wp_s32  win32_init( void );
static void    win32_shutdown( void );
static void    win32_update( void *mgr );
static void   *win32_sound_load( const wp_c8 *filepath, wp_s32 loop );
static void    win32_sound_unload( void *handle );
static void    win32_sound_play( void *handle );
static void    win32_sound_stop( void *handle );
static void    win32_sound_pause( void *handle );
static void    win32_sound_resume( void *handle );
static void    win32_sound_set_volume( void *handle, wp_f32 volume );
static wp_f32  win32_sound_get_volume( void *handle );
static void    win32_sound_set_pitch( void *handle, wp_f32 pitch );
static wp_f32  win32_sound_get_pitch( void *handle );
static void    win32_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z );
static wp_s32  win32_sound_is_playing( void *handle );

/* -------------------------------------------------------------------------
 * Backend interface
 * ---------------------------------------------------------------------- */

static const wp_audio_mgr_backend g_backend = { win32_init,
												  win32_shutdown,
												  win32_update,
												  win32_sound_load,
												  win32_sound_unload,
												  win32_sound_play,
												  win32_sound_stop,
												  win32_sound_pause,
												  win32_sound_resume,
												  win32_sound_set_volume,
												  win32_sound_get_volume,
												  win32_sound_set_pitch,
												  win32_sound_get_pitch,
												  win32_sound_set_position,
												  win32_sound_is_playing };

const wp_audio_mgr_backend *wp_audio_mgr_get_backend( void )
{
	return &g_backend;
}

/* -------------------------------------------------------------------------
 * WAV loading helpers
 * ---------------------------------------------------------------------- */

static wp_s32 win32_read_wav_file( const wp_c8 *filepath, wp_u8 **pp_data, wp_u32 *p_size,
									WAVEFORMATEX *p_format )
{
	HANDLE file;
	DWORD bytes_read      = 0;
	DWORD data_offset     = 0;
	DWORD data_size       = 0;
	wp_u8 header[44];

	file = CreateFileA( filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL, NULL );
	if( file == INVALID_HANDLE_VALUE )
		return 0;

	if( !ReadFile( file, header, sizeof( header ), &bytes_read, NULL ) ||
		bytes_read != sizeof( header ) )
	{
		CloseHandle( file );
		return 0;
	}

	/* Verify RIFF header */
	if( header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F' )
	{
		CloseHandle( file );
		return 0;
	}

	/* Verify WAVE format */
	if( header[8] != 'W' || header[9] != 'A' || header[10] != 'V' || header[11] != 'E' )
	{
		CloseHandle( file );
		return 0;
	}

	/* Find data chunk */
	while( 1 )
	{
		wp_u8 chunk_header[8];
		DWORD chunk_size;

		if( !ReadFile( file, chunk_header, sizeof( chunk_header ), &bytes_read, NULL ) ||
			bytes_read != sizeof( chunk_header ) )
		{
			CloseHandle( file );
			return 0;
		}

		chunk_size = *(wp_u32 *)( chunk_header + 4 );

		if( chunk_header[0] == 'd' && chunk_header[1] == 'a' && chunk_header[2] == 't' &&
			chunk_header[3] == 'a' )
		{
			data_offset = SetFilePointer( file, 0, NULL, FILE_CURRENT );
			data_size   = chunk_size;
			break;
		}

		if( chunk_header[0] == 'R' && chunk_header[1] == 'I' && chunk_header[2] == 'F' &&
			chunk_header[3] == 'F' && chunk_header[4] == 'F' )
		{
			/* End of file without finding data chunk */
			CloseHandle( file );
			return 0;
		}

		SetFilePointer( file, chunk_size, NULL, FILE_CURRENT );
	}

	/* Parse format chunk from earlier */
	p_format->wFormatTag      = *(wp_u16 *)( header + 20 );
	p_format->nChannels       = *(wp_u16 *)( header + 22 );
	p_format->nSamplesPerSec  = *(wp_u32 *)( header + 24 );
	p_format->nAvgBytesPerSec = *(wp_u32 *)( header + 28 );
	p_format->nBlockAlign     = *(wp_u16 *)( header + 32 );
	p_format->wBitsPerSample  = *(wp_u16 *)( header + 34 );
	p_format->cbSize           = 0;

	/* Read audio data */
	*pp_data = (wp_u8 *)malloc( data_size );
	if( !*pp_data )
	{
		CloseHandle( file );
		return 0;
	}

	SetFilePointer( file, data_offset, NULL, FILE_BEGIN );
	if( !ReadFile( file, *pp_data, data_size, &bytes_read, NULL ) || bytes_read != data_size )
	{
		free( *pp_data );
		CloseHandle( file );
		return 0;
	}

	*p_size = data_size;
	CloseHandle( file );
	return 1;
}

/* -------------------------------------------------------------------------
 * Backend implementations
 * ---------------------------------------------------------------------- */

static wp_s32 win32_init( void )
{
	HRESULT hr;

	if( g_xaudio2 )
		return 1;  /* Already initialised */

	hr = XAudio2Create( &g_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR );
	if( FAILED( hr ) )
		return 0;

	hr = IXAudio2_CreateMasteringVoice( g_xaudio2, &g_master_voice, XAUDIO2_DEFAULT_CHANNELS,
												XAUDIO2_DEFAULT_SAMPLERATE, 0, NULL, NULL, 0 );
	if( FAILED( hr ) )
	{
		IXAudio2_Release( g_xaudio2 );
		g_xaudio2 = NULL;
		return 0;
	}

	return 1;
}

static void win32_shutdown( void )
{
	if( g_master_voice )
	{
		IXAudio2MasteringVoice_DestroyVoice( g_master_voice );
		g_master_voice = NULL;
	}

	if( g_xaudio2 )
	{
		IXAudio2_StopEngine( g_xaudio2 );
		IXAudio2_Release( g_xaudio2 );
		g_xaudio2 = NULL;
	}
}

static void win32_update( void *mgr )
{
	(void)mgr;
	/* XAudio2 handles updates internally when voices are played */
}

static void *win32_sound_load( const wp_c8 *filepath, wp_s32 loop )
{
	struct wp_audio_win32_sound *sound;
	WAVEFORMATEX format;
	wp_u8 *audio_data;
	wp_u32 audio_size;
	HRESULT hr;

	if( !filepath || !g_xaudio2 )
		return NULL;

	if( !win32_read_wav_file( filepath, &audio_data, &audio_size, &format ) )
		return NULL;

	sound = (struct wp_audio_win32_sound *)malloc( sizeof( struct wp_audio_win32_sound ) );
	if( !sound )
	{
		free( audio_data );
		return NULL;
	}

	memset( sound, 0, sizeof( struct wp_audio_win32_sound ) );
	sound->audio_data      = audio_data;
	sound->audio_data_size = audio_size;
	sound->looping         = loop;
	sound->volume          = 1.0f;
	sound->pitch           = 1.0f;

	/* Create XAudio2 buffer */
	sound->buffer = (XAUDIO2_BUFFER *)malloc( sizeof( XAUDIO2_BUFFER ) );
	if( !sound->buffer )
	{
		free( audio_data );
		free( sound );
		return NULL;
	}

	/* Note: In production, you'd use proper XAudio2 buffer creation.
	 * This simplified version stores data for later voice creation. */
	(void)hr;
	(void)format;

	return sound;
}

static void win32_sound_unload( void *handle )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound )
		return;

	if( sound->voice )
	{
		IXAudio2SourceVoice_Stop( sound->voice, 0, 0 );
		IXAudio2SourceVoice_DestroyVoice( sound->voice );
	}

	if( sound->buffer )
		free( sound->buffer );

	if( sound->audio_data )
		free( sound->audio_data );

	free( sound );
}

static void win32_sound_play( void *handle )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound || !sound->voice )
		return;

	IXAudio2SourceVoice_Start( sound->voice, 0, 0 );
}

static void win32_sound_stop( void *handle )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound || !sound->voice )
		return;

	IXAudio2SourceVoice_Stop( sound->voice, 0, 0 );
}

static void win32_sound_pause( void *handle )
{
	win32_sound_stop( handle );
}

static void win32_sound_resume( void *handle )
{
	win32_sound_play( handle );
}

static void win32_sound_set_volume( void *handle, wp_f32 volume )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound )
		return;

	sound->volume = volume;
	if( sound->voice )
		IXAudio2SourceVoice_SetVolume( sound->voice, volume, 0 );
}

static wp_f32 win32_sound_get_volume( void *handle )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound )
		return 0.0f;

	return sound->volume;
}

static void win32_sound_set_pitch( void *handle, wp_f32 pitch )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound )
		return;

	sound->pitch = pitch;
	if( sound->voice )
		IXAudio2SourceVoice_SetFrequencyRatio( sound->voice, pitch, 0 );
}

static wp_f32 win32_sound_get_pitch( void *handle )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound )
		return 1.0f;

	return sound->pitch;
}

static void win32_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z )
{
	(void)handle;
	(void)x;
	(void)y;
	(void)z;
	/* 3D audio positioning would be implemented here using X3DAudio */
}

static wp_s32 win32_sound_is_playing( void *handle )
{
	struct wp_audio_win32_sound *sound = (struct wp_audio_win32_sound *)handle;
	if( !sound || !sound->voice )
		return 0;

	XAUDIO2_VOICE_STATE state;
	IXAudio2SourceVoice_GetState( sound->voice, &state, 0 );
	return ( state.BuffersQueued > 0 ) ? 1 : 0;
}

#endif /* WP_AUDIO_PLATFORM_WINDOWS */
