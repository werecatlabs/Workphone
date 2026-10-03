#ifndef __WP_ConfigFile_H__
#define __WP_ConfigFile_H__

#include <Workphone/Interface/System/IConfigFile.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <map>

namespace workphone
{
    /**
     * @file ConfigFile.hpp
     * @brief Lightweight configuration file parser and container.
     *
     * The `ConfigFile` class provides simple parsing and storage for
     * configuration files that use simple key/value pairs and optional
     * sections. Multiple values per key are supported. The parser uses
     * a configurable separator (default `"="`) and can trim whitespace
     * around keys and values.
     *
     * Example format:
     * ```
     * key = value
     *
     * [Section]
     * key = value1
     * key = value2
     * ```
     */
    class ConfigFile : public IConfigFile
    {
    public:
        /**
         * @brief Multi-map type that stores zero-or-more values for a single key.
         *
         * Keys and values use the project's `String` type.
         */
        using SettingsMultiMap = std::multimap<String, String>;

        /**
         * @brief Map of section name -> settings multimap.
         *
         * Each entry in the outer map identifies a section. The empty string
         * ("") is used for top-level keys that are not inside any explicit section.
         */
        using SettingsBySection = std::map<String, SettingsMultiMap>;

        /**
         * @brief Construct an empty ConfigFile object.
         *
         * Initializes internal storage and default parsing options:
         * - separator: "="
         * - trimWhitespace: true
         */
        ConfigFile();

        /**
         * @brief Destructor.
         *
         * Releases any resources held by the ConfigFile. No special behaviour.
         */
        ~ConfigFile() override;

        /**
         * @brief Load and parse configuration data from a file path.
         *
         * The implementation should read the file specified by `filePath`,
         * parse keys, values and optional sections, and populate the internal
         * settings storage.
         *
         * @param filePath Filesystem path to the configuration file to load.
         */
        void loadFromFilePath( const String &filePath ) override;

        /**
         * @brief Load and parse configuration data from a stream.
         *
         * Allows callers to provide a stream (file, memory, embedded resource)
         * containing configuration text. Parsed settings are stored internally.
         *
         * @param stream Smart pointer to an `IStream` providing the configuration content.
         */
        void loadFromStream( SmartPtr<IStream> stream ) override;

        /**
         * @brief Retrieve a single setting value.
         *
         * Returns the first value found for the given `key` within the specified
         * `section`. If the key is not present, `defaultValue` is returned.
         *
         * @param key The configuration key to look up.
         * @param section Optional section name. Use empty string ("") for top-level keys.
         * @param defaultValue Value to return if the key is not found.
         * @return The found value, or `defaultValue` if missing.
         */
        String getSetting( const String &key, const String &section = String(),
                           const String &defaultValue = String() ) const override;

        /**
         * @brief Retrieve all values for a key (supports duplicate keys).
         *
         * If multiple values were defined for the same `key` (e.g. repeated lines),
         * all values are returned in the order they were parsed.
         *
         * @param key The configuration key to look up.
         * @param section Optional section name. Use empty string ("") for top-level keys.
         * @return An `Array<String>` containing zero or more values for the key.
         */
        Array<String> getSettings( const String &key, const String &section = String() ) const override;

        /**
         * @brief Access the entire settings container grouped by section.
         *
         * Provides read-only access to the internal `SettingsBySection` map.
         *
         * @return Const reference to the section -> settings map.
         */
        const SettingsBySection &getSettingsBySection() const;

        /**
         * @brief Get the settings multi-map for a specific section.
         *
         * Returns the multimap storing keys and their (possibly multiple) values
         * for the given `section`. The default section is the empty string ("").
         *
         * @param section Section name to query (default = top-level / empty).
         * @return Const reference to the settings multimap for the section.
         */
        const SettingsMultiMap &getSettingsMap( const String &section = "" ) const;

        /**
         * @brief Clear all loaded settings.
         *
         * Empties the internal settings storage so the object can be reused.
         */
        void clear();

        bool getTrim() const;

        void setTrim( bool trim );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Internal storage of parsed settings organized by section.
         *
         * Key: section name (empty string for top-level).
         * Value: multimap of key -> value(s).
         */
        SettingsBySection m_settings;

        /**
         * @brief Characters used to separate a key from its value when parsing.
         *
         * Default value is `"="`. This can contain multiple separator characters
         * if the parser implementation treats any of them as valid separators.
         */
        String m_separators;

        /**
         * @brief Whether the parser trims leading/trailing whitespace from keys and values.
         *
         * Default is true.
         */
        bool m_trim = true;
    };
}  // namespace workphone

#endif
