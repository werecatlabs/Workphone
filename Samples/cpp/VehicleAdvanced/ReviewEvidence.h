#pragma once

#include <cmath>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

namespace workphone::advanced
{
    inline bool reviewPathsAlias( const std::string &a, const std::string &b )
    {
        if( a.empty() || b.empty() ) return false;
        auto left = std::filesystem::weakly_canonical( std::filesystem::absolute( a ) ).generic_string();
        auto right = std::filesystem::weakly_canonical( std::filesystem::absolute( b ) ).generic_string();
#ifdef _WIN32
        std::transform( left.begin(), left.end(), left.begin(), []( unsigned char c ) { return char( std::tolower( c ) ); } );
        std::transform( right.begin(), right.end(), right.begin(), []( unsigned char c ) { return char( std::tolower( c ) ); } );
#endif
        if( left == right ) return true;
        std::error_code error;
        return std::filesystem::equivalent( a, b, error ) && !error;
    }

    inline std::string jsonString( const std::string &value )
    {
        std::ostringstream out;
        out << '"';
        for( unsigned char c : value )
        {
            switch( c )
            {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if( c < 32 ) out << "\\u00" << std::hex << std::setw( 2 ) << std::setfill( '0' ) << unsigned( c ) << std::dec;
                else out << char( c );
            }
        }
        out << '"';
        return out.str();
    }

    struct ReviewControl
    {
        float throttle = 0, brake = 0, steering = 0;
    };

    // One input per physics update at 120 Hz. This replays controls, not a claim
    // of cross-platform deterministic physics or identical rendered frame timing.
    inline std::vector<ReviewControl> loadReviewReplay( const std::string &path )
    {
        std::ifstream input( path );
        input.imbue( std::locale::classic() );
        if( !input ) throw std::runtime_error( "Cannot open review replay: " + path );
        std::string line;
        if( !std::getline( input, line ) ) throw std::runtime_error( "Empty review replay." );
        if( !line.empty() && line.back() == '\r' ) line.pop_back();
        if( line != "tick,throttle,brake,steering" )
            throw std::runtime_error( "Replay header must be tick,throttle,brake,steering (120 Hz)." );
        std::vector<ReviewControl> controls;
        while( std::getline( input, line ) )
        {
            if( !line.empty() && line.back() == '\r' ) line.pop_back();
            std::istringstream row( line );
            row.imbue( std::locale::classic() );
            std::uint64_t tick;
            char comma1, comma2, comma3;
            ReviewControl control;
            if( !( row >> tick >> comma1 >> control.throttle >> comma2 >> control.brake >> comma3 >> control.steering ) ||
                comma1 != ',' || comma2 != ',' || comma3 != ',' || tick != controls.size() ||
                !std::isfinite( control.throttle ) || !std::isfinite( control.brake ) ||
                !std::isfinite( control.steering ) || control.throttle < 0 || control.throttle > 1 ||
                control.brake < 0 || control.brake > 1 || control.steering < -1 || control.steering > 1 )
                throw std::runtime_error( "Invalid replay control at tick " + std::to_string( controls.size() ) );
            row >> std::ws;
            if( !row.eof() ) throw std::runtime_error( "Trailing replay control data." );
            if( controls.size() >= 120 * 3600 ) throw std::runtime_error( "Replay exceeds one hour." );
            controls.push_back( control );
        }
        if( !input.eof() || controls.empty() ) throw std::runtime_error( "Replay is empty or unreadable." );
        return controls;
    }

    inline bool writeReviewEvidence( const std::string &path, const std::string &json )
    {
        std::ofstream out( path, std::ios::binary | std::ios::trunc );
        if( !out ) return false;
        out.write( json.data(), std::streamsize( json.size() ) );
        out.flush();
        const bool written = out.good();
        out.close();
        return written && !out.fail();
    }
}
