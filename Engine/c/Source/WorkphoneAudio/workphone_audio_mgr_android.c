/**
 * @file workphone_audio_mgr_android.c
 * @brief Android audio backend using OpenSL ES or AAudio.
 */

#include "workphone_audio_mgr.h"

#if WP_AUDIO_PLATFORM_ANDROID

#    include <SLES/OpenSLES.h>
#    include <SLES/OpenSLES_Android.h>

/* -------------------------------------------------------------------------
 * Internal types
 * ---------------------------------------------------------------------- */

struct wp_audio_android_sound
{
	SLObjectItf               engine_object;
	SLObjectItf               output_mixer;
	SLObjectItf               player_object;
	SLPlayItf                 play_interface;
	SLVolumeItf              volume_interface;
	wp_u8                    *audio_data;
	wp_u32                    audio_data_size;
	wp_s32                    looping;
	wp_f32                    volume;
	wp_f32                    pitch;
	wp_s32                    is_playing;
};

static SLObjectItf g_sl_engine = NULL;
static SLEngineItf g_sl_engine_interface = NULL;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

static wp_s32  android_init( void );
static void    android_shutdown( void );
static void    android_update( void *mgr );
static void   *android_sound_load( const wp_c8 *filepath, wp_s32 loop );
static void    android_sound_unload( void *handle );
static void    android_sound_play( void *handle );
static void    android_sound_stop( void *handle );
static void    android_sound_pause( void *handle );
static void    android_sound_resume( void *handle );
static void    android_sound_set_volume( void *handle, wp_f32 volume );
static wp_f32  android_sound_get_volume( void *handle );
static void    android_sound_set_pitch( void *handle, wp_f32 pitch );
static wp_f32  android_sound_get_pitch( void *handle );
static void    android_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z );
static wp_s32  android_sound_is_playing( void *handle );

/* -------------------------------------------------------------------------
 * Backend interface
 * ---------------------------------------------------------------------- */

static const wp_audio_mgr_backend g_backend = { android_init,
												   android_shutdown,
												   android_update,
												   android_sound_load,
												   android_sound_unload,
												   android_sound_play,
												   android_sound_stop,
												   android_sound_pause,
												   android_sound_resume,
												   android_sound_set_volume,
												   android_sound_get_volume,
												   android_sound_set_pitch,
												   android_sound_get_pitch,
												   android_sound_set_position,
												   android_sound_is_playing };

const wp_audio_mgr_backend *wp_audio_mgr_get_backend( void )
{
	return &g_backend;
}

/* -------------------------------------------------------------------------
 * WAV loading helpers
 * ---------------------------------------------------------------------- */

#include <stdio.h>

static wp_s32 android_read_wav_file( const wp_c8 *filepath, wp_u8 **pp_data, wp_u32 *p_size,
									  SLDataFormat_PCM *p_format )
{
	FILE *file;
	wp_u8 header[44];
	wp_u32 data_size;

	if( !filepath )
		return 0;

	file = fopen( filepath, "rb" );
	if( !file )
		return 0;

	if( fread( header, 1, sizeof( header ), file ) != sizeof( header ) )
	{
		fclose( file );
		return 0;
	}

	/* Verify RIFF header */
	if( header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F' )
	{
		fclose( file );
		return 0;
	}

	/* Verify WAVE format */
	if( header[8] != 'W' || header[9] != 'A' || header[10] != 'V' || header[11] != 'E' )
	{
		fclose( file );
		return 0;
	}

	/* Find data chunk */
	while( 1 )
	{
		wp_u8 chunk_header[8];
		wp_u32 chunk_size;

		if( fread( chunk_header, 1, sizeof( chunk_header ), file ) != sizeof( chunk_header ) )
		{
			fclose( file );
			return 0;
		}

		chunk_size = *(wp_u32 *)( chunk_header + 4 );

		if( chunk_header[0] == 'd' && chunk_header[1] == 'a' && chunk_header[2] == 't' &&
			chunk_header[3] == 'a' )
		{
			data_size = chunk_size;
			break;
		}

		if( chunk_header[0] == 'R' && chunk_header[1] == 'I' && chunk_header[2] == 'F' &&
			chunk_header[3] == 'F' )
		{
			fclose( file );
			return 0;
		}

		fseek( file, chunk_size, SEEK_CUR );
	}

	/* Parse WAV format */
	p_format->formatType     = SL_DATAFORMAT_PCM;
	p_format->numChannels     = *(wp_u16 *)( header + 22 );
	p_format->samplesPerSec   = *(wp_u32 *)( header + 24 );
	p_format->bitsPerSample   = *(wp_u16 *)( header + 34 );
	p_format->containerSize   = p_format->bitsPerSample;
	p_format->channelMask     = ( p_format->numChannels == 1 ) ? SL_SPEAKER_FRONT_CENTER
																 : SL_SPEAKER_FRONT_LEFT
																   | SL_SPEAKER_FRONT_RIGHT;
	p_format->endianness      = SL_BYTEORDER_LITTLEENDIAN;

	/* Read audio data */
	*pp_data = (wp_u8 *)malloc( data_size );
	if( !*pp_data )
	{
		fclose( file );
		return 0;
	}

	if( fread( *pp_data, 1, data_size, file ) != data_size )
	{
		free( *pp_data );
		fclose( file );
		return 0;
	}

	*p_size = data_size;
	fclose( file );
	return 1;
}

/* -------------------------------------------------------------------------
 * Backend implementations
 * ---------------------------------------------------------------------- */

static wp_s32 android_init( void )
{
	SLresult result;

	if( g_sl_engine )
		return 1;  /* Already initialised */

	/* Create engine */
	result = slCreateEngine( &g_sl_engine, 0, NULL, 0, NULL, NULL );
	if( result != SL_RESULT_SUCCESS )
		return 0;

	/* Realise engine */
	result = (*g_sl_engine)->Realize( g_sl_engine, SL_BOOLEAN_FALSE );
	if( result != SL_RESULT_SUCCESS )
	{
		(*g_sl_engine)->Destroy( g_sl_engine );
		g_sl_engine = NULL;
		return 0;
	}

	/* Get engine interface */
	result = (*g_sl_engine)
				 ->GetInterface( g_sl_engine, SL_IID_ENGINE, (void *)&g_sl_engine_interface );
	if( result != SL_RESULT_SUCCESS )
	{
		(*g_sl_engine)->Destroy( g_sl_engine );
		g_sl_engine = NULL;
		return 0;
	}

	return 1;
}

static void android_shutdown( void )
{
	if( g_sl_engine )
	{
		(*g_sl_engine)->Destroy( g_sl_engine );
		g_sl_engine           = NULL;
		g_sl_engine_interface = NULL;
	}
}

static void android_update( void *mgr )
{
	(void)mgr;
	/* OpenSL ES handles updates internally */
}

static void *android_sound_load( const wp_c8 *filepath, wp_s32 loop )
{
	struct wp_audio_android_sound *sound;
	SLDataFormat_PCM format;
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq;
	SLDataSource audio_src;
	SLDataLocator_OutputMix loc_outmix;
	SLDataSink audio_sink;
	SLresult result;
	const SLInterfaceID ids[]      = { SL_IID_VOLUME };
	const SLboolean req[]          = { SL_BOOLEAN_FALSE };
	wp_u8 *audio_data;
	wp_u32 audio_data_size;

	if( !filepath || !g_sl_engine_interface )
		return NULL;

	/* Read WAV file first */
	if( !android_read_wav_file( filepath, &audio_data, &audio_data_size, &format ) )
		return NULL;

	sound = (struct wp_audio_android_sound *)calloc( 1, sizeof( struct wp_audio_android_sound ) );
	if( !sound )
	{
		free( audio_data );
		return NULL;
	}

	sound->audio_data       = audio_data;
	sound->audio_data_size  = audio_data_size;
	sound->looping          = loop;
	sound->volume           = 1.0f;
	sound->pitch            = 1.0f;
	sound->is_playing       = 0;

	/* Create output mixer */
	result = (*g_sl_engine_interface)
				 ->CreateOutputMix( g_sl_engine_interface, &sound->output_mixer, 0, NULL, NULL );
	if( result != SL_RESULT_SUCCESS )
	{
		free( sound->audio_data );
		free( sound );
		return NULL;
	}

	(*sound->output_mixer)->Realize( sound->output_mixer, SL_BOOLEAN_FALSE );

	/* Setup buffer queue */
	loc_bufq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	loc_bufq.numBuffers  = 2;

	audio_src.pFormat = &format;
	audio_src.pLocator = &loc_bufq;

	/* Setup output */
	loc_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	loc_outmix.outputMix   = sound->output_mixer;

	audio_sink.pLocator = &loc_outmix;
	audio_sink.pFormat  = NULL;

	/* Create audio player */
	result = (*g_sl_engine_interface)
				 ->CreateAudioPlayer( g_sl_engine_interface, &sound->player_object, &audio_src,
									  &audio_sink, 1, ids, req );
	if( result != SL_RESULT_SUCCESS )
	{
		(*sound->output_mixer)->Destroy( sound->output_mixer );
		free( sound->audio_data );
		free( sound );
		return NULL;
	}

	(*sound->player_object)->Realize( sound->player_object, SL_BOOLEAN_FALSE );

	/* Get play interface */
	result = (*sound->player_object)
				 ->GetInterface( sound->player_object, SL_IID_PLAY, (void *)&sound->play_interface );
	if( result != SL_RESULT_SUCCESS )
	{
		(*sound->player_object)->Destroy( sound->player_object );
		(*sound->output_mixer)->Destroy( sound->output_mixer );
		free( sound->audio_data );
		free( sound );
		return NULL;
	}

	/* Get volume interface */
	(*sound->player_object)
		->GetInterface( sound->player_object, SL_IID_VOLUME, (void *)&sound->volume_interface );

	return sound;
}

static void android_sound_unload( void *handle )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound )
		return;

	if( sound->player_object )
	{
		(*sound->player_object)->Destroy( sound->player_object );
	}

	if( sound->output_mixer )
	{
		(*sound->output_mixer)->Destroy( sound->output_mixer );
	}

	free( sound );
}

static void android_sound_play( void *handle )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound || !sound->play_interface )
		return;

	(*sound->play_interface)->SetPlayState( sound->play_interface, SL_PLAYSTATE_PLAYING );
	sound->is_playing = 1;
}

static void android_sound_stop( void *handle )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound || !sound->play_interface )
		return;

	(*sound->play_interface)->SetPlayState( sound->play_interface, SL_PLAYSTATE_STOPPED );
	sound->is_playing = 0;
}

static void android_sound_pause( void *handle )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound || !sound->play_interface )
		return;

	(*sound->play_interface)->SetPlayState( sound->play_interface, SL_PLAYSTATE_PAUSED );
}

static void android_sound_resume( void *handle )
{
	android_sound_play( handle );
}

static void android_sound_set_volume( void *handle, wp_f32 volume )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound )
		return;

	sound->volume = volume;
	if( sound->volume_interface )
	{
		/* Convert 0.0-1.0 to millibels (-100dB to 0dB) */
		SLmillibel level = (SLmillibel)( -100.0f * ( 1.0f - volume ) );
		(*sound->volume_interface)->SetVolumeLevel( sound->volume_interface, level );
	}
}

static wp_f32 android_sound_get_volume( void *handle )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound )
		return 0.0f;

	return sound->volume;
}

static void android_sound_set_pitch( void *handle, wp_f32 pitch )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound )
		return;

	sound->pitch = pitch;
	/* Note: OpenSL ES doesn't support pitch without playback rate modification */
}

static wp_f32 android_sound_get_pitch( void *handle )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound )
		return 1.0f;

	return sound->pitch;
}

static void android_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z )
{
	(void)handle;
	(void)x;
	(void)y;
	(void)z;
	/* 3D audio positioning would be implemented using OpenSL ES 3D audio extensions */
}

static wp_s32 android_sound_is_playing( void *handle )
{
	struct wp_audio_android_sound *sound = (struct wp_audio_android_sound *)handle;
	if( !sound || !sound->play_interface )
		return 0;

	SLuint32 state;
	(*sound->play_interface)->GetPlayState( sound->play_interface, &state );
	return ( state == SL_PLAYSTATE_PLAYING ) ? 1 : 0;
}

#endif /* WP_AUDIO_PLATFORM_ANDROID */
