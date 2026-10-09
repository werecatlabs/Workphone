#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace workphone::advanced
{
struct VehicleAudioInput
{
    float rpm = 0, idleRpm = 1500, redlineRpm = 12000;
    float throttle = 0, speed = 0, slipSpeed = 0;
    bool grounded = false, playing = true;
};

struct VehicleAudioGains
{
    std::array<float, 6> engine{};
    float rolling = 0, squeal = 0;
};

inline VehicleAudioGains vehicleAudioTargets( const VehicleAudioInput& input )
{
    VehicleAudioGains gains;
    if ( !input.playing ) return gains;
    auto finite = []( float value ) { return std::isfinite( value ) ? value : 0.f; };
    const auto range = std::max( finite( input.redlineRpm - input.idleRpm ), 1.f );
    const auto level = std::clamp( finite( input.rpm - input.idleRpm ) / range, 0.f, 1.f ) * 5.f;
    const auto lower = size_t( level );
    const auto fraction = level - float( lower );
    const auto volume = .20f + .20f * std::clamp( finite( input.throttle ), 0.f, 1.f );
    // Equal-power crossfade between adjacent authored pitch loops.
    gains.engine[lower] = volume * std::sqrt( 1.f - fraction );
    if ( lower < 5 ) gains.engine[lower + 1] = volume * std::sqrt( fraction );
    if ( input.grounded )
    {
        gains.rolling = .08f * std::clamp( finite( input.speed ) / 55.f, 0.f, 1.f );
        // A dead zone excludes ordinary rolling slip and parked-wheel jitter.
        gains.squeal = .28f * std::clamp( ( finite( input.slipSpeed ) - 1.5f ) / 6.f, 0.f, 1.f );
    }
    return gains;
}

inline void smoothVehicleAudio( VehicleAudioGains& current, const VehicleAudioGains& target,
                                float dt )
{
    if ( !std::isfinite( dt ) || dt <= 0 ) return;
    const auto blend = 1.f - std::exp( -std::min( dt, .1f ) / .08f );
    for ( size_t i = 0; i < current.engine.size(); ++i )
        current.engine[i] += ( target.engine[i] - current.engine[i] ) * blend;
    current.rolling += ( target.rolling - current.rolling ) * blend;
    current.squeal += ( target.squeal - current.squeal ) * blend;
}
}  // namespace workphone::advanced
