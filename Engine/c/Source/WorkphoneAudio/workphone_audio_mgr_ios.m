/**
 * @file workphone_audio_mgr_ios.m
 * @brief iOS audio backend using AVAudioEngine.
 */

#import <AVFoundation/AVFoundation.h>
#include "workphone_audio_mgr.h"

#if WP_AUDIO_PLATFORM_IOS

/* -------------------------------------------------------------------------
 * Internal types
 * ---------------------------------------------------------------------- */

@interface WPAudioIOSSound : NSObject
{
@public
	AVAudioEngine *engine;
	AVAudioPlayerNode *playerNode;
	AVAudioFile *audioFile;
	AVAudioPCMBuffer *buffer;
	wp_s32 looping;
	wp_f32 volume;
	wp_f32 pitch;
	wp_s32 isPlaying;
}

- (instancetype)initWithFilepath:(const wp_c8 *)filepath loop:(wp_s32)loop;
- (void)play;
- (void)stop;
- (void)pause;
- (void)resume;
- (void)setVolume:(wp_f32)volume;
- (wp_f32)getVolume;
- (void)setPitch:(wp_f32)pitch;
- (wp_f32)getPitch;
- (void)setPosition:(wp_f32)x y:(wp_f32)y z:(wp_f32)z;
- (wp_s32)isPlaying;

@end

@implementation WPAudioIOSSound

- (instancetype)initWithFilepath:(const wp_c8 *)filepath loop:(wp_s32)loop
{
	self = [super init];
	if( self )
	{
		NSError *error = nil;
		NSString *path = [NSString stringWithUTF8String:filepath];
		NSURL *fileURL = [NSURL fileURLWithPath:path];

		engine = [[AVAudioEngine alloc] init];
		playerNode = [[AVAudioPlayerNode alloc] init];
		looping = loop;
		volume = 1.0f;
		pitch = 1.0f;
		isPlaying = 0;

		/* Configure audio session for playback */
		AVAudioSession *session = [AVAudioSession sharedInstance];
		[session setCategory:AVAudioSessionCategoryPlayback error:&error];
		[session setActive:YES error:&error];

		/* Load audio file */
		audioFile = [[AVAudioFile alloc] initForReading:fileURL error:&error];
		if( error || !audioFile )
		{
			return nil;
		}

		/* Create buffer */
		AVAudioFrameCount frameCount = (AVAudioFrameCount)audioFile.length;
		buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:audioFile.processingFormat
												frameCapacity:frameCount];
		[audioFile readIntoBuffer:buffer error:&error];
		if( error || !buffer )
		{
			return nil;
		}

		/* Attach and connect player node */
		[engine attachNode:playerNode];
		[engine connect:playerNode to:engine.mainMixerNode format:audioFile.processingFormat];

		/* Start engine */
		[engine prepare];
		[engine startAndReturnError:&error];
		if( error )
		{
			return nil;
		}
	}
	return self;
}

- (void)dealloc
{
	[playerNode stop];
	[engine stop];
}

- (void)play
{
	if( !buffer || !playerNode )
		return;

	if( looping )
	{
		[playerNode scheduleBuffer:buffer completionHandler:^{
			if( self->looping && self->isPlaying )
			{
				[self->playerNode scheduleBuffer:self->buffer atTime:nil options:0 completionHandler:^{
					if( self->looping && self->isPlaying )
					{
						/* Continue looping */
					}
				}];
			}
		}];
	}
	else
	{
		[playerNode scheduleBuffer:buffer atTime:nil options:0 completionHandler:nil];
	}

	[playerNode play];
	isPlaying = 1;
}

- (void)stop
{
	if( playerNode )
	{
		[playerNode stop];
		isPlaying = 0;
	}
}

- (void)pause
{
	[self stop];
}

- (void)resume
{
	[self play];
}

- (void)setVolume:(wp_f32)newVolume
{
	volume = newVolume;
	playerNode.volume = newVolume;
}

- (wp_f32)getVolume
{
	return volume;
}

- (void)setPitch:(wp_f32)newPitch
{
	pitch = newPitch;
	/* Note: AVAudioEngine doesn't directly support pitch without a time pitch unit.
	 * For production, attach an AVAudioUnitTimePitch to the chain. */
}

- (wp_f32)getPitch
{
	return pitch;
}

- (void)setPosition:(wp_f32)x y:(wp_f32)y z:(wp_f32)z
{
	(void)x;
	(void)y;
	(void)z;
	/* 3D audio positioning would be implemented using AVAudio3DMixing */
}

- (wp_s32)isPlaying
{
	return isPlaying;
}

@end

/* -------------------------------------------------------------------------
 * C interface wrappers
 * ---------------------------------------------------------------------- */

static wp_s32  ios_init( void );
static void    ios_shutdown( void );
static void    ios_update( void *mgr );
static void   *ios_sound_load( const wp_c8 *filepath, wp_s32 loop );
static void    ios_sound_unload( void *handle );
static void    ios_sound_play( void *handle );
static void    ios_sound_stop( void *handle );
static void    ios_sound_pause( void *handle );
static void    ios_sound_resume( void *handle );
static void    ios_sound_set_volume( void *handle, wp_f32 volume );
static wp_f32  ios_sound_get_volume( void *handle );
static void    ios_sound_set_pitch( void *handle, wp_f32 pitch );
static wp_f32  ios_sound_get_pitch( void *handle );
static void    ios_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z );
static wp_s32  ios_sound_is_playing( void *handle );

/* -------------------------------------------------------------------------
 * Backend interface
 * ---------------------------------------------------------------------- */

static const wp_audio_mgr_backend g_backend = { ios_init,
												   ios_shutdown,
												   ios_update,
												   ios_sound_load,
												   ios_sound_unload,
												   ios_sound_play,
												   ios_sound_stop,
												   ios_sound_pause,
												   ios_sound_resume,
												   ios_sound_set_volume,
												   ios_sound_get_volume,
												   ios_sound_set_pitch,
												   ios_sound_get_pitch,
												   ios_sound_set_position,
												   ios_sound_is_playing };

const wp_audio_mgr_backend *wp_audio_mgr_get_backend( void )
{
	return &g_backend;
}

/* -------------------------------------------------------------------------
 * Backend implementations
 * ---------------------------------------------------------------------- */

static wp_s32 ios_init( void )
{
	/* Audio session is configured per-sound in the init method */
	return 1;
}

static void ios_shutdown( void )
{
	/* Release audio session */
	NSError *error = nil;
	[[AVAudioSession sharedInstance] setActive:NO error:&error];
}

static void ios_update( void *mgr )
{
	(void)mgr;
	/* AVAudioEngine handles updates internally */
}

static void *ios_sound_load( const wp_c8 *filepath, wp_s32 loop )
{
	if( !filepath )
		return NULL;

	WPAudioIOSSound *sound = [[WPAudioIOSSound alloc] initWithFilepath:filepath loop:loop];
	if( !sound )
		return NULL;

	return (__bridge void *)sound;
}

static void ios_sound_unload( void *handle )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound stop];
	sound = nil;  /* This will dealloc if retain count is 1 */
}

static void ios_sound_play( void *handle )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound play];
}

static void ios_sound_stop( void *handle )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound stop];
}

static void ios_sound_pause( void *handle )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound pause];
}

static void ios_sound_resume( void *handle )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound resume];
}

static void ios_sound_set_volume( void *handle, wp_f32 volume )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound setVolume:volume];
}

static wp_f32 ios_sound_get_volume( void *handle )
{
	if( !handle )
		return 0.0f;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	return [sound getVolume];
}

static void ios_sound_set_pitch( void *handle, wp_f32 pitch )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound setPitch:pitch];
}

static wp_f32 ios_sound_get_pitch( void *handle )
{
	if( !handle )
		return 1.0f;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	return [sound getPitch];
}

static void ios_sound_set_position( void *handle, wp_f32 x, wp_f32 y, wp_f32 z )
{
	if( !handle )
		return;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	[sound setPosition:x y:y z:z];
}

static wp_s32 ios_sound_is_playing( void *handle )
{
	if( !handle )
		return 0;

	WPAudioIOSSound *sound = (__bridge WPAudioIOSSound *)handle;
	return [sound isPlaying];
}

#endif /* WP_AUDIO_PLATFORM_IOS */
