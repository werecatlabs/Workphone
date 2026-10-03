#ifndef IConfigFile_h__
#define IConfigFile_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * @brief Abstract interface representing a configuration file.
     *
     * Implementations of this interface provide functionality to load
     * configuration data from external sources and to query settings by key
     * and, optionally, by section. The interface returns settings as
     * `String` or as an `Array<String>` when multiple values are present for
     * a single key.
     */
    class WPCore_API IConfigFile : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived class destructors are called correctly when an
         * object is deleted through a pointer to this interface.
         */
        ~IConfigFile() override;

        /**
         * @brief Load configuration data from a file path.
         *
         * Implementations should read and parse the file located at
         * `filePath`. The format (INI, JSON, XML, etc.) is implementation
         * specific and should be documented by the concrete class.
         *
         * @param filePath Path to the configuration file to load.
         */
        virtual void loadFromFilePath( const String &filePath ) = 0;

        /**
         * @brief Load configuration data from a stream.
         *
         * Allows loading configuration data from any `IStream` source
         * (memory stream, file stream, network stream, etc.). Ownership
         * semantics of the provided `stream` follow project conventions for
         * `SmartPtr`.
         *
         * @param stream Smart pointer to an input stream containing
         * configuration data.
         */
        virtual void loadFromStream( SmartPtr<IStream> stream ) = 0;

        /**
         * @brief Retrieve a single configuration setting.
         *
         * Searches for `key` inside the optional `section`. If the key is
         * not found, `defaultValue` is returned.
         *
         * @param key The name of the setting to retrieve.
         * @param section Optional section/group name to search within.
         * @param defaultValue Value to return when the key is not present.
         * @return The setting value as a `String` or `defaultValue` if not found.
         */
        virtual String getSetting( const String &key, const String &section = String(),
                                   const String &defaultValue = String() ) const = 0;

        /**
         * @brief Retrieve multiple values for a given key.
         *
         * Some configuration formats support multiple values for the same
         * key. This method returns all values found for `key` within the
         * optional `section`.
         *
         * @param key The name of the setting whose values to retrieve.
         * @param section Optional section/group name to search within.
         * @return An array of values for the given key. If no values are
         *         found an empty array is returned.
         */
        virtual Array<String> getSettings( const String &key,
                                           const String &section = String() ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IConfigFile_h__
