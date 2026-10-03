// ---------------------------------------------------------------------------
//  JsonUtil.hpp
//
//  Replacement for the static helpers in
//  Tools/csharp/AssetDatabaseTool/JsonData.cs.  Provides conversion
//  utilities used by the C++ port and a minimal JSON parser sufficient for
//  the (small) data files the C# tool writes/reads.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseEditorJsonUtil_h__
#define AssetDatabaseEditorJsonUtil_h__

#include <AssetDatabaseDomain.hpp>
#include <Workphone/Workphone.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace workphone
{
    namespace adbeditor
    {
        /** Helper for the original @c vec4 type and the small JSON subset used
         *  by the C# tool (string, number, bool, object, array). */
        class JsonUtil
        {
        public:
            static Vector3F StringToVector3( const String &input, c8 splitChar = ',' );
            static String Vector3ToString( const Vector3F &value, c8 splitChar = ',' );

            /** Lightweight JSON pretty-printer used when writing aircraft /
             *  airfoil data.  Only the types used by the C# code are
             *  supported. */
            static String Serialize( const AircraftData &data );
            static String Serialize( const AircraftAirfoilData &data );

            /** Minimal JSON deserialiser for the TransmitterProfile file the C#
             *  code reads with @c JsonConvert.DeserializeObject. */
            static TransmitterProfile DeserializeTransmitterProfile( const String &text );

            /** Helpers for cloning the simple string format used by the C#
             *  code.  Returns a default constructed @c TransmitterProfile on
             *  failure. */
            static TransmitterProfile LoadTransmitterProfile( const String &filePath );
            static bool SaveAircraftData( const String &filePath, const AircraftData &data );
            static bool SaveAirfoil( const String &filePath, const AircraftAirfoilData &data );
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseEditorJsonUtil_h__
