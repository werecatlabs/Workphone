#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Procedural/Circuit.hpp>
#include <algorithm>
#include <optional>
#include <cmath>
#include <random>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <map>
#include <tuple>
#include <cstdio>

namespace workphone::scene::race
{

    void Circuit::swap( Circuit &other ) noexcept
    {
        samples.swap( other.samples );
        std::swap( length, other.length );
    }

    workphone::scene::race::Circuit &Circuit::operator=( const Circuit &other )
    {
        if( this != &other )
        {
            Circuit candidate( other );
            swap( candidate );
        }
        return *this;
    }

    Circuit::Circuit() = default;

    Circuit::Circuit( const Circuit & ) = default;

    Circuit::Circuit( Circuit && ) noexcept = default;

    Circuit &Circuit::operator=( Circuit && ) noexcept = default;

    size_t Circuit::nearest( const Vector3F &position ) const
    {
        size_t result = 0;
        float best = std::numeric_limits<float>::max();
        for( size_t i = 0; i < samples.size(); ++i )
        {
            auto d = ( samples[i].position - position ).length();
            if( d < best )
            {
                best = d;
                result = i;
            }
        }
        return result;
    }

    Circuit generateCircuit( u32 seed )
    {
        SourceBudgetVector<Vector3F> knots = { { 0, 0, 70 },    { 0, 0, 0 },     { 0, 0, -90 },
                                               { 20, 0, -165 }, { 80, 0, -180 }, { 140, 0, -150 },
                                               { 155, 0, -70 }, { 105, 0, -35 }, { 145, 0, 30 },
                                               { 135, 0, 105 }, { 50, 0, 130 },  { 0, 0, 95 } };
        // Seed changes the infield bends; the start straight and spawn remain fixed.
        std::mt19937 rng( seed );
        std::uniform_real_distribution<float> variation( -6, 6 );
        for( size_t i = 3; i < 11; ++i )
        {
            knots[i].x += variation( rng );
            knots[i].z += variation( rng );
        }
        SourceBudgetVector<Vector3F> dense;
        for( size_t i = 0; i < knots.size(); ++i )
        {
            for( int step = 0; step < 100; ++step )
            {
                auto a = knots[( i + knots.size() - 1 ) % knots.size()], b = knots[i],
                     c = knots[( i + 1 ) % knots.size()], d = knots[( i + 2 ) % knots.size()];
                float t = step / 100.f;
                dense.push_back( ( b * 2 + ( c - a ) * t + ( a * 2 - b * 5 + c * 4 - d ) * t * t +
                                   ( -a + b * 3 - c * 3 + d ) * t * t * t ) *
                                 .5f );
            }
        }
        SourceBudgetVector<float> distance( dense.size() + 1, 0 );
        for( size_t i = 0; i < dense.size(); ++i )
        {
            distance[i + 1] = distance[i] + ( dense[( i + 1 ) % dense.size()] - dense[i] ).length();
        }
        Circuit result;
        result.length = distance.back();
        auto count = static_cast<size_t>( std::ceil( result.length / 1.5f ) );
        size_t segment = 0;
        for( size_t i = 0; i < count; ++i )
        {
            float s = result.length * i / count;
            while( segment + 1 < dense.size() && distance[segment + 1] < s )
            {
                ++segment;
            }
            float t = ( s - distance[segment] ) / ( distance[segment + 1] - distance[segment] );
            result.samples.push_back(
                { dense[segment] + ( dense[( segment + 1 ) % dense.size()] - dense[segment] ) * t,
                  {},
                  s } );
        }
        // Make progress zero at the finish stripe, travelling in the car's -Z direction.
        auto start = result.nearest( { 0, 0, 0 } );
        std::rotate( result.samples.begin(), result.samples.begin() + start, result.samples.end() );
        for( size_t i = 0; i < count; ++i )
        {
            auto tangent = result.samples[( i + 1 ) % count].position -
                           result.samples[( i + count - 1 ) % count].position;
            tangent.normalise();
            result.samples[i].right = { -tangent.z, 0, tangent.x };
            result.samples[i].distance = result.length * i / count;
        }
        if( result.samples.size() < 3 || result.length < 500 )
        {
            throw std::runtime_error( "Invalid generated circuit." );
        }
        return result;
    }

    void validateCircuit()
    {
        for( u32 seed = 0; seed < 100; ++seed )
        {
            auto circuit = generateCircuit( seed ), repeat = generateCircuit( seed );
            if( circuit.length != repeat.length || circuit.samples.size() != repeat.samples.size() )
            {
                throw std::runtime_error( "Circuit generation is not deterministic." );
            }
            const auto spacing = circuit.length / circuit.samples.size();
            for( size_t i = 0; i < circuit.samples.size(); ++i )
            {
                const auto &a = circuit.samples[i],
                           &b = circuit.samples[( i + 1 ) % circuit.samples.size()];
                const auto chord = ( b.position - a.position ).length();
                if( a.position != repeat.samples[i].position || a.position.y != 0 ||
                    std::abs( a.right.length() - 1 ) > 1e-4f || chord < spacing * .95f ||
                    chord > spacing * 1.01f )
                {
                    throw std::runtime_error( "Circuit closure, spacing, or frame validation failed." );
                }
            }
            if( circuit.samples.front().position.length() > spacing ||
                circuit.samples.front().right.x < .99f )
            {
                throw std::runtime_error( "Circuit spawn does not align with the start straight." );
            }
        }
        if( generateCircuit( 7 ).samples[100].position == generateCircuit( 8 ).samples[100].position )
        {
            throw std::runtime_error( "Seed does not alter the circuit." );
        }
    }

}  // namespace workphone::scene::race
