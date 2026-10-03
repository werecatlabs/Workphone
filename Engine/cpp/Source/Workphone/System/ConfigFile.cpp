#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ConfigFile.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <fstream>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ConfigFile, IConfigFile );

    ConfigFile::ConfigFile()
    {
        m_separators = "=";
    }

    ConfigFile::~ConfigFile() = default;

    void ConfigFile::loadFromFilePath( const String &filePath )
    {
        s32 flags = std::fstream::in;
        auto file = new std::fstream( filePath.c_str(), flags );
        if( file->is_open() )
        {
            auto stream = workphone::make_ptr<FileDataStream>( file, true );
            loadFromStream( stream );
        }
    }

    void ConfigFile::loadFromStream( SmartPtr<IStream> stream )
    {
        clear();

        String currentSection = "";
        SettingsMultiMap *currentSettings = &m_settings[currentSection];

        String line, optName, optVal;
        while( !stream->eof() )
        {
            line = stream->getLine();

            if( line.length() > 0 && line.at( 0 ) != '#' && line.at( 0 ) != '@' )
            {
                if( line.at( 0 ) == '[' && line.at( line.length() - 1 ) == ']' )
                {
                    currentSection = line.substr( 1, line.length() - 2 );
                    currentSettings = &m_settings[currentSection];
                }
                else
                {
                    auto values = StringUtil::split( line, "=" );

                    auto settingValue =
                        std::make_pair( StringUtil::trim( values[0] ), StringUtil::trim( values[1] ) );
                    currentSettings->insert( settingValue );
                }
            }
        }
    }

    String ConfigFile::getSetting( const String &key, const String &section,
                                   const String &defaultValue ) const
    {
        auto seci = m_settings.find( section );
        bool found = ( seci != m_settings.end() );
        if( found )
        {
            auto i = seci->second.find( key );
            bool foundKey = ( i != seci->second.end() );
            {
                return i->second;
            }
        }
        return defaultValue;
    }

    Array<String> ConfigFile::getSettings( const String &key, const String &section ) const
    {
        Array<String> ret;
        ret.reserve( 32 );

        auto seci = m_settings.find( section );
        if( seci != m_settings.end() )
        {
            SettingsMultiMap::const_iterator i;

            i = seci->second.find( key );

            // Iterate over matches
            while( i != seci->second.end() && i->first == key )
            {
                ret.push_back( i->second );
                ++i;
            }
        }

        return ret;
    }

    const ConfigFile::SettingsMultiMap &ConfigFile::getSettingsMap( const String &section ) const
    {
        auto seci = m_settings.find( section );
        bool found = ( seci != m_settings.end() );
        if( found )
        {
            // Handle not found
        }
        return seci->second;
    }

    const ConfigFile::SettingsBySection &ConfigFile::getSettingsBySection() const
    {
        return m_settings;
    }

    void ConfigFile::clear()
    {
        m_settings.clear();
    }

    bool ConfigFile::getTrim() const
    {
        return m_trim;
    }

    void ConfigFile::setTrim( bool trim )
    {
        m_trim = trim;
    }

}  // namespace workphone
