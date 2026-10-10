#include <WPNetwork/WPNetworkConfig.hpp>
#include <rapidjson/document.h>
#include <set>
#include <WorkphoneNetwork/workphone_network.h>
#include <fstream>
#include <stdexcept>

namespace workphone
{
    WPNetworkConfig WPNetworkConfig::load( const String &filePath )
    {
        WPNetworkConfig config;
        if( filePath.empty() )
            return config;

        std::ifstream input( filePath.c_str(), std::ios::binary );
        if( !input )
            throw std::runtime_error( "WPNetwork: cannot open network profile" );
        // Bound file reads before invoking the engine JSON parser.
        String text;
        char value;
        while( input.get( value ) )
        {
            if( text.size() >= 8192 )
                throw std::length_error( "WPNetwork: network profile exceeds 8192 bytes" );
            text.push_back( value );
        }
        if( input.bad() )
            throw std::runtime_error( "WPNetwork: cannot read network profile" );
        rapidjson::Document object;
        object.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseIterativeFlag>(
            text.data(), text.size() );
        if( object.HasParseError() || !object.IsObject() )
            throw std::invalid_argument( "WPNetwork: network profile must be a JSON object" );
        auto integer = []( const rapidjson::Value &item, u32 minimum, u32 maximum ) -> u32 {
            if( !item.IsUint() || item.GetUint() < minimum || item.GetUint() > maximum )
                throw std::invalid_argument( "WPNetwork: invalid profile integer" );
            return item.GetUint();
        };
        auto requireString = []( const rapidjson::Value &item, const char *expected ) {
            if( !item.IsString() || String( item.GetString(), item.GetStringLength() ) != expected )
                throw std::invalid_argument( "WPNetwork: unsupported backend or environment" );
        };
        std::set<String> fields;
        for( const auto &member : object.GetObject() )
        {
            const String name( member.name.GetString(), member.name.GetStringLength() );
            const auto &item = member.value;
            if( !fields.insert( name ).second )
                throw std::invalid_argument( "WPNetwork: duplicate network profile field" );
            if( name == "version" )
                integer( item, 1, 1 );
            else if( name == "backend" )
                requireString( item, "native_udp_development" );
            else if( name == "environment" )
                requireString( item, "development" );
            else if( name == "port" )
                config.port = integer( item, 0, 65535 );
            else if( name == "maxClients" )
                config.maxClients = integer( item, 1, NET_MAX_PEERS );
            else if( name == "eventsPerPoll" )
                config.eventsPerPoll = static_cast<u16>( integer( item, 1, NET_MAX_EVENTS ) );
            else if( name == "verbose" )
            {
                if( !item.IsBool() )
                    throw std::invalid_argument( "WPNetwork: verbose must be a Boolean" );
                config.verbose = item.GetBool();
            }
            else
                throw std::invalid_argument( "WPNetwork: unknown network profile field" );
        }
        if( fields.count( "version" ) != 1 || fields.count( "backend" ) != 1 ||
            fields.count( "environment" ) != 1 )
            throw std::invalid_argument(
                "WPNetwork: profile requires version, backend and environment" );
        return config;
    }
}  // namespace workphone
