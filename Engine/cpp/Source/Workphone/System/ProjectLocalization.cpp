#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ProjectLocalization.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>

namespace workphone
{
    namespace
    {
        struct Dictionary
        {
            std::filesystem::path path;
            std::filesystem::file_time_type timestamp{};
            std::uintmax_t bytes = 0;
            std::map<std::string, std::string> entries;
        };
        std::mutex dictionaryMutex;
        std::map<std::string, Dictionary> dictionaries;

        const Dictionary &dictionary( const std::filesystem::path &root, const String &tag )
        {
            const auto path = root / "Localization" / ( std::string( tag.c_str() ) + ".json" );
            auto &cached = dictionaries[tag.c_str()];
            std::error_code error;
            const auto timestamp = std::filesystem::last_write_time( path, error );
            const auto bytes = !error ? std::filesystem::file_size( path, error ) : 0;
            if( cached.path == path && cached.timestamp == timestamp && cached.bytes == bytes )
                return cached;
            cached = Dictionary{};
            cached.path = path;
            cached.timestamp = timestamp;
            cached.bytes = bytes;
            if( error || bytes > 1024 * 1024 ) return cached;
            std::ifstream stream( path, std::ios::binary );
            std::ostringstream contents;
            contents << stream.rdbuf();
            if( !stream || stream.bad() ) return cached;
            const auto source = contents.str();
            rapidjson::Document document;
            document.Parse<rapidjson::kParseValidateEncodingFlag>( source.data(), source.size() );
            if( document.HasParseError() || !document.IsObject() ) return cached;
            for( auto it = document.MemberBegin(); it != document.MemberEnd(); ++it )
            {
                if( !it->value.IsString() ) continue;
                cached.entries.emplace(
                    std::string( it->name.GetString(), it->name.GetStringLength() ),
                    std::string( it->value.GetString(), it->value.GetStringLength() ) );
            }
            return cached;
        }
    }

    String getProjectLanguageTag()
    {
        static constexpr const char *tags[] = { "en", "fr", "de", "ja", "es", "it", "pt" };
        auto app = core::IApplicationManager::instancePtr();
        auto settings = app ? app->getPlayerSettings() : nullptr;
        auto language = settings ? settings->getPropertyAsInt( "project.defaultLanguage", 0 ) : 0;
        return tags[language >= 0 && language < 7 ? language : 0];
    }

    String localizeProjectString( const String &key, const String &fallback )
    {
        auto app = core::IApplicationManager::instancePtr();
        if( app && !key.empty() && !app->getProjectPath().empty() )
        {
            const auto root = std::filesystem::u8path( app->getProjectPath().c_str() );
            const auto language = getProjectLanguageTag();
            std::lock_guard<std::mutex> lock( dictionaryMutex );
            const auto lookup = [&]( const String &tag, String &value ) {
                const auto &entries = dictionary( root, tag ).entries;
                const auto it = entries.find( std::string( key.c_str(), key.size() ) );
                if( it == entries.end() ) return false;
                value = String( it->second.c_str(), it->second.size() );
                return true;
            };
            String value;
            if( lookup( language, value ) || ( language != "en" && lookup( "en", value ) ) )
                return value;
        }
        return fallback.empty() ? key : fallback;
    }

    String localizeProjectText( const String &text )
    {
        if( text.size() >= 4 && text[0] == '@' && text[1] == '{' && text.back() == '}' )
            return localizeProjectString( text.substr( 2, text.size() - 3 ), {} );
        return text;
    }

    void releaseProjectLocalization()
    {
        std::lock_guard<std::mutex> lock( dictionaryMutex );
        dictionaries.clear();
    }
}
