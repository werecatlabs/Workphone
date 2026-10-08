#pragma once

#include <algorithm>
#include <cmath>

namespace workphone::handling
{
    // Limit digital/analogue steering in road-wheel degrees, with a lateral
    // acceleration ceiling. Zero rate leaves existing controllers unchanged.
    inline double steering( double requested, double previous, double speed, double wheelbase,
                            double acceleration, double rate, double dt )
    {
        if( rate <= 0 || dt <= 0 )
            return requested;
        if( wheelbase > 0 && acceleration > 0 )
        {
            const auto limit = std::atan2( wheelbase * acceleration, std::max( speed * speed, 1.0 ) ) *
                               180.0 / 3.141592653589793;
            requested = std::clamp( requested, -limit, limit );
        }
        return previous + std::clamp( requested - previous, -rate * dt, rate * dt );
    }

    inline double tractionTorque( double requested, double normalForce, double friction, double grip,
                                  double lateralForce, double radius )
    {
        const auto limit = std::max( 0.0, normalForce * friction * grip );
        // Leave a margin for transient load transfer and lateral tyre response.
        const auto available =
            .85 * std::sqrt( std::max( 0.0, limit * limit - lateralForce * lateralForce ) );
        return std::clamp( requested, -available * radius, available * radius );
    }

    inline double rollingLimit( double omega, double speed, double radius, double slip )
    {
        if( radius <= 0 )
            return omega;
        const auto allowance = slip * std::max( std::abs( speed ), 3.0 ) / radius;
        const auto rolling = speed / radius;
        return std::clamp( omega, rolling - allowance, rolling + allowance );
    }

    inline double brushMagnitude( double demand, double peak, double sliding )
    {
        if( peak <= 0 || demand <= 0 )
            return 0;
        const auto ratio = demand / peak;
        if( ratio < 3 )
            return demand * ( 1 - ratio / 3 + ratio * ratio / 27 );
        // Match the brush polynomial at saturation, then progressively approach
        // sliding friction instead of losing 20 percent of grip in one tick.
        return sliding + ( peak - sliding ) * std::exp( -( ratio - 3 ) * .5 );
    }
}  // namespace workphone::handling
