#include "VehicleAudioMix.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

using namespace workphone::advanced;

void check( bool passed, const char *message )
{
    if( !passed )
    {
        std::fprintf( stderr, "%s\n", message );
        std::exit( 1 );
    }
}

int main()
{
    VehicleAudioInput input;
    input.rpm = input.idleRpm;
    auto idle = vehicleAudioTargets( input );
    check( idle.engine[0] == .2f && idle.squeal == 0 && idle.rolling == 0,
           "Parked car must idle without tyre noise" );
    for( int i = 0; i <= 1000; ++i )
    {
        input.rpm = input.idleRpm + ( input.redlineRpm - input.idleRpm ) * i / 1000.f;
        auto gains = vehicleAudioTargets( input );
        float power = 0;
        size_t active = 0;
        for( auto gain : gains.engine )
        {
            power += gain * gain;
            active += gain > 0;
        }
        check( std::abs( power - .04f ) < .00001f && active <= 2,
               "RPM crossfade must retain level and use at most two adjacent loops" );
    }
    input.throttle = 1;
    input.grounded = true;
    input.speed = 40;
    input.slipSpeed = 8;
    auto skid = vehicleAudioTargets( input );
    check( skid.engine[5] == .4f && skid.squeal > .27f && skid.rolling > .05f,
           "Redline, road speed and slip should produce their corresponding sounds" );
    input.grounded = false;
    auto airborne = vehicleAudioTargets( input );
    check( airborne.rolling == 0 && airborne.squeal == 0 && airborne.engine[5] > 0,
           "Airborne wheels must retain engine sound and silence tyre sound" );
    input.playing = false;
    auto paused = vehicleAudioTargets( input );
    VehicleAudioGains fading = skid;
    smoothVehicleAudio( fading, paused, .01f );
    check( fading.squeal > 0 && fading.squeal < skid.squeal,
           "Pausing should fade instead of cutting the waveform" );
    for( int i = 0; i < 120; ++i )
        smoothVehicleAudio( fading, paused, 1.f / 60.f );
    check( fading.engine[5] < .001f && fading.squeal < .001f && fading.rolling < .001f,
           "Fade must eventually silence every loop" );
    input.playing = true;
    input.rpm = input.slipSpeed = std::numeric_limits<float>::quiet_NaN();
    input.grounded = true;
    auto invalid = vehicleAudioTargets( input );
    for( auto gain : invalid.engine )
        check( std::isfinite( gain ) && gain >= 0 && gain <= 1, "Invalid telemetry reached mixer" );
    check( std::isfinite( invalid.squeal ), "Invalid slip reached mixer" );
    std::puts( "Vehicle audio RPM blending, contact gating and fades: passed." );
}
