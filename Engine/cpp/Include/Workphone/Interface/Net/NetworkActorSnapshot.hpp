#ifndef WORKPHONE_NETWORK_ACTOR_SNAPSHOT_HPP
#define WORKPHONE_NETWORK_ACTOR_SNAPSHOT_HPP
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace workphone::network
{
    // Remain correct under the engine's /fp:fast build flags: floating-point
    // comparisons/classification may otherwise assume inputs are finite.
    inline bool isFiniteFloat( float value )
    {
        static_assert( sizeof( float ) == 4 && std::numeric_limits<float>::is_iec559 );
        std::uint32_t bits;
        std::memcpy( &bits, &value, sizeof( bits ) );
        return ( bits & 0x7f800000u ) != 0x7f800000u;
    }

    struct ActorSnapshot
    {
        std::uint32_t sequence = 0;
        std::uint8_t channels = 0;
        std::array<float, 3> position{}, rotation{}, scale{};
    };

    // Cursor follows the 10-byte NetworkMessageHeader. Validate the complete
    // payload before the caller mutates the actor or advances its sequence.
    template <class Packet>
    ActorSnapshot readActorSnapshot( Packet &packet )
    {
        ActorSnapshot state;
        packet.read( state.sequence );
        packet.read( state.channels );
        if( ( state.channels & ~7u ) != 0 )
            throw std::invalid_argument( "Network snapshot: unknown channel" );
        const auto count = ( ( state.channels & 1u ) != 0 ) + ( ( state.channels & 2u ) != 0 ) +
                           ( ( state.channels & 4u ) != 0 );
        if( packet.getDataLength() != 15u + 12u * count )
            throw std::invalid_argument( "Network snapshot: invalid message size" );
        auto readVector = [&]( std::array<float, 3> &values ) {
            for( auto &value : values )
            {
                packet.read( value );
                if( !isFiniteFloat( value ) )
                    throw std::invalid_argument( "Network snapshot: non-finite transform" );
            }
        };
        if( state.channels & 1u )
            readVector( state.position );
        if( state.channels & 2u )
            readVector( state.rotation );
        if( state.channels & 4u )
            readVector( state.scale );
        return state;
    }

    inline bool isNewerSequence( std::uint32_t candidate, std::uint32_t previous )
    {
        const auto delta = candidate - previous;
        return delta != 0 && delta < 0x80000000u;
    }
}  // namespace workphone::network
#endif
