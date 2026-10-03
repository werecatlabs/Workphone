#ifndef __WP_Settings_h__
#define __WP_Settings_h__

#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/WorkphoneTypes.hpp>

#include <map>

namespace workphone
{
    class ConfigFile;

    /**
     * @brief Stores and validates engine settings.
     *
     * Settings are stored as strings so the class can consume the existing
     * ConfigFile format without coupling the engine to a particular file
     * format. Typed accessors are provided for the values commonly used by
     * engine systems, while validation rules keep invalid values from being
     * silently accepted.
     *
     * Config-file sections are flattened into dotted keys. For example,
     * `[render]` followed by `width = 1920` is exposed as `render.width`.
     */
    class WPCore_API Settings
    {
    public:
        enum class ValueType
        {
            String,
            Boolean,
            Integer,
            Real
        };

        enum class Severity
        {
            Warning,
            Error
        };

        struct ValidationIssue
        {
            String key;
            String message;
            Severity severity = Severity::Error;
        };

        struct Rule
        {
            String key;
            ValueType type = ValueType::String;
            bool required = false;
            bool hasMinimum = false;
            bool hasMaximum = false;
            f64 minimum = 0.0;
            f64 maximum = 0.0;
        };

        Settings();
        ~Settings();

        /** Restores the built-in defaults and built-in validation rules. */
        void reset();

        /**
         * Loads settings from an existing ConfigFile.
         *
         * Values are reset to the engine defaults before the file values are
         * applied. Custom rules added with addRule() are retained.
         */
        bool load( const ConfigFile &configFile );

        /** Loads settings from a ConfigFile-compatible file path. */
        bool loadFromFilePath( const String &filePath );

        /** Sets or replaces a setting value. */
        void setValue( const String &key, const String &value );

        /** Returns true when a setting has been explicitly stored. */
        bool hasValue( const String &key ) const;

        /** Gets a setting as text, returning defaultValue when it is absent. */
        String getString( const String &key, const String &defaultValue = String() ) const;

        /** Gets a setting as a boolean, returning defaultValue on conversion failure. */
        bool getBool( const String &key, bool defaultValue = false ) const;

        /** Gets a setting as a signed integer, returning defaultValue on conversion failure. */
        s32 getInt( const String &key, s32 defaultValue = 0 ) const;

        /** Gets a setting as a floating-point value, returning defaultValue on conversion failure. */
        f64 getReal( const String &key, f64 defaultValue = 0.0 ) const;

        /** Adds a custom validation rule. Existing rules for the same key are replaced. */
        void addRule( const Rule &rule );

        /** Adds a required-value rule. */
        void addRequiredRule( const String &key, ValueType type );

        /** Adds an inclusive numeric range rule. */
        void addRangeRule( const String &key, ValueType type, f64 minimum, f64 maximum );

        /** Removes all validation rules, including the built-in rules. */
        void clearRules();

        /** Checks all configured values against the current validation rules. */
        bool verifySettings();

        /** Returns the result of the last verifySettings() call. */
        bool isValid() const;

        /** Returns the issues found by the last verifySettings() call. */
        const Array<ValidationIssue> &getValidationIssues() const;

        /** Returns the default engine settings as a new object. */
        static Settings createDefault();

    protected:
        void setDefaultValues();
        void addDefaultRules();
        bool parseBool( const String &value, bool &result ) const;
        bool parseInt( const String &value, s32 &result ) const;
        bool parseReal( const String &value, f64 &result ) const;
        void addIssue( const String &key, const String &message, Severity severity );

        std::map<String, String> m_values;
        Array<Rule> m_rules;
        Array<ValidationIssue> m_validationIssues;
        bool m_isValid = false;
    };
}  // namespace workphone

#include <Workphone/System/Settings.inl>

#endif  // __WP_Settings_h__
