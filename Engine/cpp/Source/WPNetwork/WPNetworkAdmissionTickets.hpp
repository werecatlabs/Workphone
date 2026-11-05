#pragma once

#include <openssl/rand.h>
#include <openssl/sha.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace workphone::network
{
    // One store per listening session. The transport serializes access and supplies
    // the canonical certificate identity for issuance and consumption.
    class WPNetworkAdmissionTickets
    {
    public:
        using Token = std::array<unsigned char, 32>;

        WPNetworkAdmissionTickets() = default;
        WPNetworkAdmissionTickets( const WPNetworkAdmissionTickets & ) = delete;
        WPNetworkAdmissionTickets &operator=( const WPNetworkAdmissionTickets & ) = delete;

        static Token randomToken()
        {
            Token token{};
            if( RAND_bytes( token.data(), static_cast<int>( token.size() ) ) != 1 )
            {
                throw std::runtime_error( "Could not generate secure admission bytes" );
            }
            return token;
        }

        Token issue( std::string_view identity, std::chrono::seconds lifetime )
        {
            if( identity.empty() || identity.size() > MaxIdentityBytes ||
                lifetime < std::chrono::seconds{ 1 } || lifetime > std::chrono::seconds{ 120 } )
            {
                throw std::invalid_argument( "Invalid admission ticket identity or lifetime" );
            }

            const auto now = Clock::now();
            eraseExpired( now );
            if( m_pending.size() >= MaxPending )
            {
                throw std::runtime_error( "Admission ticket capacity exhausted" );
            }

            for( int attempt = 0; attempt < 4; ++attempt )
            {
                const auto token = randomToken();
                Digest digest{};
                if( !SHA256( token.data(), token.size(), digest.data() ) )
                {
                    throw std::runtime_error( "Could not hash admission ticket" );
                }
                if( m_pending.try_emplace( digest, Pending{ std::string( identity ), now + lifetime } )
                        .second )
                {
                    return token;
                }
            }
            throw std::runtime_error( "Could not allocate a unique admission ticket" );
        }

        bool consume( const Token &token, std::string_view identity )
        {
            eraseExpired( Clock::now() );

            Digest digest{};
            if( !SHA256( token.data(), token.size(), digest.data() ) )
            {
                return false;
            }

            const auto found = m_pending.find( digest );
            if( found == m_pending.end() || std::string_view( found->second.identity ) != identity )
            {
                return false;
            }

            m_pending.erase( found );
            return true;
        }

        void clear() noexcept
        {
            m_pending.clear();
        }

    private:
        using Clock = std::chrono::steady_clock;
        using Digest = std::array<unsigned char, SHA256_DIGEST_LENGTH>;
        static constexpr std::size_t MaxIdentityBytes = 128;
        static constexpr std::size_t MaxPending = 256;

        struct Pending
        {
            std::string identity;
            Clock::time_point deadline;
        };

        void eraseExpired( Clock::time_point now )
        {
            for( auto entry = m_pending.begin(); entry != m_pending.end(); )
            {
                if( entry->second.deadline <= now )
                {
                    entry = m_pending.erase( entry );
                }
                else
                {
                    ++entry;
                }
            }
        }

        // Store a digest so the server keeps no plaintext bearer credential.
        std::map<Digest, Pending> m_pending;
    };
}  // namespace workphone::network
