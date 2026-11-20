#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace workphone::scene::cityEntranceSteps
{
    constexpr int maximumRisers = 9;
    constexpr float rimGap = .015f;

    struct FootwayFit
    {
        float firstRiser, tread, base;
        int risers;
    };

    // Each triplet samples the left edge, centre and right edge of the entrance.
    // The flight joins the level foundation to the edge of the fitted walk.
    inline std::optional<FootwayFit> fitToFootway( float floor, float foundationDepth,
                                                   const std::array<float, 3> &reach,
                                                   const std::array<float, 3> &height )
    {
        if( !std::isfinite( floor ) || !std::isfinite( foundationDepth ) || foundationDepth < 0 )
        {
            return {};
        }
        for( std::size_t i = 0; i < reach.size(); ++i )
        {
            if( !std::isfinite( reach[i] ) || reach[i] <= 0 || !std::isfinite( height[i] ) )
            {
                return {};
            }
        }
        const auto [shortest, longest] = std::minmax_element( reach.begin(), reach.end() );
        const auto [low, high] = std::minmax_element( height.begin(), height.end() );
        if( *longest - *shortest > .18f || *high - *low > .10f ||
            std::abs( reach[1] - ( reach[0] + reach[2] ) * .5f ) > .025f ||
            std::abs( height[1] - ( height[0] + height[2] ) * .5f ) > .025f )
        {
            return {};
        }
        const float base = ( height[0] + height[2] ) * .5f;
        const float rise = floor - base;
        if( rise < .06f || rise > maximumRisers * .18f )
        {
            return {};
        }
        const int risers = int( std::ceil( rise / .18f ) );
        const float firstRiser = std::max( .85f, foundationDepth + .02f );
        const float run = *shortest - rimGap - firstRiser;
        // Blend a level bottom tread into the measured crossfall without a lip.
        // Leave at least 12 units of run for each unit of vertical change.
        const float skewRun = 12.f * std::max( base - *low, *high - base );
        const float tread = std::clamp( ( run - std::max( .36f, skewRun ) ) / risers, .28f, .34f );
        const float apron = run - risers * tread;
        if( apron + .0001f < std::max( .14f, skewRun ) || apron + ( *longest - *shortest ) > 1.15f )
        {
            return {};
        }
        return FootwayFit{ firstRiser, tread, base, risers };
    }
}
