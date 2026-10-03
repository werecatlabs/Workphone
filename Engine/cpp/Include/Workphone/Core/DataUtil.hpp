#ifndef DataUtil_h__
#define DataUtil_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/JsonValue.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    /** Utility class used to convert data. */
    class WPCore_API DataUtil
    {
    public:
        using Format = DataFormat;

        static bool isFormat( const String &text, DataFormat fmt = DataFormat::JSON );

        /** Supported formats as a data strings.
         * @note This is used for parsing data strings.
         * @param text The string to parse.
         * @param fmt The format of the data string.
         * @return The parsed properties object.
         */
        static String formatText( const String &text, DataFormat fmt = DataFormat::JSON );

        /** Converts data from a structure to a string.
         * @tparam T The type of the structure.
         * @param ptr The pointer to the structure.
         * @param formatted Whether to format the string.
         * @param fmt The format of the string.
         * @return The string representation of the structure.
         */
        template <class T>
        static String toString( T *ptr, bool formatted = false, DataFormat fmt = DataFormat::JSON );

        /** Converts data from a string to a structure.
         * @param dataStr The string to parse.
         * @param ptr The pointer to the structure.
         * @param fmt The format of the string.
         */
        template <class T>
        static void parse( const String &dataStr, T *ptr, DataFormat fmt = DataFormat::JSON );

        /** Converts properties data to a vector2 value. */
        static void parse( SmartPtr<Properties> properties, Vector2F &value );

        /** Converts properties data to a vector3 value. */
        static void parse( SmartPtr<Properties> properties, Vector3F &value );

        /** Converts properties data to a vector4 value. */
        static void parse( SmartPtr<Properties> properties, Vector4F &value );

        /** Converts properties data to a quaternion value. */
        static void parse( SmartPtr<Properties> properties, QuaternionF &value );

        /** Converts properties data to a colour value.
         * @param properties The properties object to parse.
         * @param value The colour value to set.
         */
        static void parse( SmartPtr<Properties> properties, ColourF &value );

        /** Checks if the given string is a valid data string.
         * @param jsonDataStr The string to check.
         * @param format The format of the string.
         */
        static bool isValidData( const String &jsonDataStr, DataFormat format = DataFormat::JSON );

    private:
        /** Parses a JSON string into a Properties object. */
        static SmartPtr<Properties> parseJson( const String &jsonDataStr );

        static JsonObject convert( Properties *properties );

        /** Parses a JSON string into a Properties object. Expects the JSON string to for a properties
         * object.
         */
        static SmartPtr<Properties> parsePropertiesFromJson( const String &jsonDataStr );

        /** Parses a JSON string into a Property object. Expects the JSON string to for a property
         * object.
         */
        static String formatJson( const String &jsonString );

        static void parseJson( const JsonObject *jsonObject, Vector2I *ptr );
        static void parseJson( const JsonObject *jsonObject, Vector2F *ptr );
        static void parseJson( const JsonObject *jsonObject, Vector2D *ptr );
        static void parseJson( const JsonObject *jsonObject, Vector3I *ptr );
        static void parseJson( const JsonObject *jsonObject, Vector3F *ptr );
        static void parseJson( const JsonObject *jsonObject, Vector3D *ptr );

        static void parseJson( const JsonObject *jsonObject, Transform3F *ptr );
        static void parseJson( const JsonObject *jsonObject, Transform3D *ptr );

        static void parseJson( const JsonObject *jsonObject, Properties *ptr );
        static void parseJson( const JsonObject *jsonObject, Property *ptr );
    };

}  // namespace workphone

#endif  // DataUtil_h__
