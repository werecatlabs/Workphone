#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Settings.hpp>

#include <Workphone/System/ConfigFile.hpp>
#include <Workphone/Core/StringUtil.hpp>

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>

namespace workphone
{
    namespace
    {
        String makeSectionKey( const String &section, const String &key )
        {
            if( section.empty() )
            {
                return key;
            }

            return section + "." + key;
        }

        String valueTypeName( Settings::ValueType type )
        {
            switch( type )
            {
            case Settings::ValueType::Boolean:
                return "boolean";
            case Settings::ValueType::Integer:
                return "integer";
            case Settings::ValueType::Real:
                return "real";
            case Settings::ValueType::String:
            default:
                return "string";
            }
        }
    }  // namespace

    Settings::Settings()
    {
        reset();
    }

    Settings::~Settings() = default;

    void Settings::reset()
    {
        setDefaultValues();
        m_rules.clear();
        addDefaultRules();
        m_validationIssues.clear();
        verifySettings();
    }

    Settings Settings::createDefault()
    {
        return Settings();
    }

    void Settings::setDefaultValues()
    {
        m_values.clear();

        // Application/runtime defaults.
        setValue( "engine.fixed_update_rate", "60" );
        setValue( "engine.max_fps", "0" );  // Zero means uncapped.

        // Renderer defaults mirror the resolution used by the legacy
        // SystemSettings implementation while remaining renderer-agnostic.
        setValue( "render.width", "1280" );
        setValue( "render.height", "720" );
        setValue( "render.color_depth", "32" );
        setValue( "render.fullscreen", "false" );
        setValue( "render.vsync", "true" );

        // Audio values are normalized to [0, 1].
        setValue( "audio.master_volume", "1.0" );
        setValue( "audio.ambient_volume", "1.0" );

        // Paths are intentionally relative by default; applications can
        // replace them with absolute paths during startup.
        setValue( "paths.project", "." );
        setValue( "paths.media", "Media" );
        setValue( "paths.cache", "Cache" );
        setValue( "paths.settings", "Settings" );
    }

    void Settings::addDefaultRules()
    {
        addRangeRule( "engine.fixed_update_rate", ValueType::Integer, 1.0, 1000.0 );
        addRangeRule( "engine.max_fps", ValueType::Integer, 0.0, 1000.0 );

        addRangeRule( "render.width", ValueType::Integer, 1.0, 16384.0 );
        addRangeRule( "render.height", ValueType::Integer, 1.0, 16384.0 );
        addRangeRule( "render.color_depth", ValueType::Integer, 16.0, 64.0 );
        addRule( { "render.fullscreen", ValueType::Boolean } );
        addRule( { "render.vsync", ValueType::Boolean } );

        addRangeRule( "audio.master_volume", ValueType::Real, 0.0, 1.0 );
        addRangeRule( "audio.ambient_volume", ValueType::Real, 0.0, 1.0 );

        addRequiredRule( "paths.project", ValueType::String );
        addRequiredRule( "paths.media", ValueType::String );
        addRequiredRule( "paths.cache", ValueType::String );
        addRequiredRule( "paths.settings", ValueType::String );
    }

    bool Settings::load( const ConfigFile &configFile )
    {
        setDefaultValues();
        m_validationIssues.clear();
        m_isValid = false;

        for( const auto &section : configFile.getSettingsBySection() )
        {
            for( const auto &setting : section.second )
            {
                // A repeated key is legal in ConfigFile. The final value is
                // used here, matching the usual scalar-settings expectation.
                setValue( makeSectionKey( section.first, setting.first ), setting.second );
            }
        }

        return verifySettings();
    }

    bool Settings::loadFromFilePath( const String &filePath )
    {
        std::ifstream file( filePath.c_str(), std::ios::in );
        if( !file.is_open() )
        {
            m_validationIssues.clear();
            addIssue( filePath, "Unable to open settings file.", Severity::Error );
            m_isValid = false;
            return false;
        }

        file.close();

        ConfigFile configFile;
        configFile.loadFromFilePath( filePath );
        return load( configFile );
    }

    void Settings::setValue( const String &key, const String &value )
    {
        if( key.empty() )
        {
            return;
        }

        m_values[key] = value;
    }

    String Settings::getString( const String &key, const String &defaultValue ) const
    {
        auto iter = m_values.find( key );
        return iter != m_values.end() ? iter->second : defaultValue;
    }

    bool Settings::getBool( const String &key, bool defaultValue ) const
    {
        bool result = defaultValue;
        parseBool( getString( key ), result );
        return result;
    }

    s32 Settings::getInt( const String &key, s32 defaultValue ) const
    {
        s32 result = defaultValue;
        parseInt( getString( key ), result );
        return result;
    }

    f64 Settings::getReal( const String &key, f64 defaultValue ) const
    {
        f64 result = defaultValue;
        parseReal( getString( key ), result );
        return result;
    }

    void Settings::addRule( const Rule &rule )
    {
        for( auto &existingRule : m_rules )
        {
            if( existingRule.key == rule.key )
            {
                existingRule = rule;
                return;
            }
        }

        m_rules.push_back( rule );
    }

    void Settings::addRequiredRule( const String &key, ValueType type )
    {
        Rule rule;
        rule.key = key;
        rule.type = type;
        rule.required = true;
        addRule( rule );
    }

    void Settings::addRangeRule( const String &key, ValueType type, f64 minimum, f64 maximum )
    {
        Rule rule;
        rule.key = key;
        rule.type = type;
        rule.hasMinimum = true;
        rule.hasMaximum = true;
        rule.minimum = minimum;
        rule.maximum = maximum;
        addRule( rule );
    }

    void Settings::clearRules()
    {
        m_rules.clear();
    }

    bool Settings::verifySettings()
    {
        m_validationIssues.clear();

        for( const auto &rule : m_rules )
        {
            auto valueIter = m_values.find( rule.key );
            if( valueIter == m_values.end() )
            {
                if( rule.required )
                {
                    addIssue( rule.key, "Required setting is missing.", Severity::Error );
                }
                continue;
            }

            const auto &value = valueIter->second;
            bool parsed = true;
            f64 numericValue = 0.0;

            switch( rule.type )
            {
            case ValueType::Boolean:
            {
                bool boolValue = false;
                parsed = parseBool( value, boolValue );
                break;
            }
            case ValueType::Integer:
            {
                s32 integerValue = 0;
                parsed = parseInt( value, integerValue );
                numericValue = static_cast<f64>( integerValue );
                break;
            }
            case ValueType::Real:
                parsed = parseReal( value, numericValue );
                break;
            case ValueType::String:
                parsed = !value.empty();
                break;
            }

            if( !parsed )
            {
                addIssue( rule.key, "Expected a " + valueTypeName( rule.type ) + " value.",
                          Severity::Error );
                continue;
            }

            if( rule.hasMinimum && numericValue < rule.minimum )
            {
                addIssue( rule.key, "Value is below the minimum allowed value.", Severity::Error );
            }
            else if( rule.hasMaximum && numericValue > rule.maximum )
            {
                addIssue( rule.key, "Value is above the maximum allowed value.", Severity::Error );
            }
        }

        m_isValid = true;
        for( const auto &issue : m_validationIssues )
        {
            if( issue.severity == Severity::Error )
            {
                m_isValid = false;
                break;
            }
        }

        return m_isValid;
    }

    bool Settings::parseBool( const String &value, bool &result ) const
    {
        String normalized = StringUtil::trim( value );
        for( auto &character : normalized )
        {
            character = static_cast<char>( std::tolower( static_cast<unsigned char>( character ) ) );
        }

        if( normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on" )
        {
            result = true;
            return true;
        }

        if( normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off" )
        {
            result = false;
            return true;
        }

        return false;
    }

    bool Settings::parseInt( const String &value, s32 &result ) const
    {
        const auto trimmed = StringUtil::trim( value );
        if( trimmed.empty() )
        {
            return false;
        }

        char *end = nullptr;
        errno = 0;
        const long parsed = std::strtol( trimmed.c_str(), &end, 10 );
        if( errno == ERANGE || end == trimmed.c_str() || *end != '\0' ||
            parsed < std::numeric_limits<s32>::min() || parsed > std::numeric_limits<s32>::max() )
        {
            return false;
        }

        result = static_cast<s32>( parsed );
        return true;
    }

    bool Settings::parseReal( const String &value, f64 &result ) const
    {
        const auto trimmed = StringUtil::trim( value );
        if( trimmed.empty() )
        {
            return false;
        }

        char *end = nullptr;
        errno = 0;
        const auto parsed = std::strtod( trimmed.c_str(), &end );
        if( errno == ERANGE || end == trimmed.c_str() || *end != '\0' || !std::isfinite( parsed ) )
        {
            return false;
        }

        result = parsed;
        return true;
    }

    void Settings::addIssue( const String &key, const String &message, Severity severity )
    {
        ValidationIssue issue;
        issue.key = key;
        issue.message = message;
        issue.severity = severity;
        m_validationIssues.push_back( issue );
    }
}  // namespace workphone
