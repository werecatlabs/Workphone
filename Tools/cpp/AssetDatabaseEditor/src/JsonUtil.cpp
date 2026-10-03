// ---------------------------------------------------------------------------
//  JsonUtil.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <JsonUtil.hpp>
#include <Workphone/Workphone.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace workphone::adbeditor
{
    namespace
    {
        String Trim( const String &input )
        {
            auto start = input.find_first_not_of( " \t\r\n" );
            if( start == String::npos )
            {
                return String();
            }
            auto end = input.find_last_not_of( " \t\r\n" );
            return input.substr( start, end - start + 1 );
        }

        std::vector<String> Split( const String &input, c8 splitChar )
        {
            std::vector<String> result;
            String current;
            for( auto ch : input )
            {
                if( ch == splitChar )
                {
                    result.push_back( Trim( current ) );
                    current.clear();
                }
                else
                {
                    current.push_back( ch );
                }
            }
            result.push_back( Trim( current ) );
            return result;
        }

        String Escape( const String &input )
        {
            String out;
            out.reserve( input.size() );
            for( auto ch : input )
            {
                switch( ch )
                {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out.push_back( ch ); break;
                }
            }
            return out;
        }

        // Very small JSON reader sufficient for the TransmitterProfile file
        // emitted by the C# tool.  It supports only the subset used by the
        // editor (string/number/bool/object/array).
        class JsonReader
        {
        public:
            explicit JsonReader( const String &text ) : m_text( text ) {}

            void skipWhitespace()
            {
                while( m_pos < m_text.size() &&
                       ( m_text[m_pos] == ' ' || m_text[m_pos] == '\t' ||
                         m_text[m_pos] == '\n' || m_text[m_pos] == '\r' ) )
                {
                    ++m_pos;
                }
            }

            bool consume( c8 expected )
            {
                skipWhitespace();
                if( m_pos < m_text.size() && m_text[m_pos] == expected )
                {
                    ++m_pos;
                    return true;
                }
                return false;
            }

            bool tryMatch( const char *token )
            {
                skipWhitespace();
                const auto len = std::strlen( token );
                if( m_pos + len > m_text.size() )
                {
                    return false;
                }
                if( std::strncmp( m_text.c_str() + m_pos, token, len ) != 0 )
                {
                    return false;
                }
                m_pos += len;
                return true;
            }

            String readString()
            {
                skipWhitespace();
                if( m_pos >= m_text.size() || m_text[m_pos] != '"' )
                {
                    return String();
                }
                ++m_pos;  // skip opening quote
                String out;
                while( m_pos < m_text.size() && m_text[m_pos] != '"' )
                {
                    if( m_text[m_pos] == '\\' && m_pos + 1 < m_text.size() )
                    {
                        out.push_back( m_text[m_pos + 1] );
                        m_pos += 2;
                    }
                    else
                    {
                        out.push_back( m_text[m_pos] );
                        ++m_pos;
                    }
                }
                if( m_pos < m_text.size() )
                {
                    ++m_pos;  // skip closing quote
                }
                return out;
            }

            String readValue()
            {
                skipWhitespace();
                if( m_pos >= m_text.size() )
                {
                    return String();
                }
                if( m_text[m_pos] == '"' )
                {
                    return readString();
                }
                String out;
                while( m_pos < m_text.size() && m_text[m_pos] != ',' &&
                       m_text[m_pos] != '}' && m_text[m_pos] != ']' &&
                       m_text[m_pos] != ' ' && m_text[m_pos] != '\n' &&
                       m_text[m_pos] != '\r' && m_text[m_pos] != '\t' )
                {
                    out.push_back( m_text[m_pos] );
                    ++m_pos;
                }
                return out;
            }

            size_t m_pos = 0;
            String m_text;
        };

        void WriteVector( std::ostream &out, const String &name, const Vector3F &value )
        {
            out << "    \"" << name << "\": { \"x\": " << value.x << ", \"y\": " << value.y
                << ", \"z\": " << value.z << " }";
        }

        void WriteTransform( std::ostream &out, const String &name, const TransformData &tr )
        {
            out << "    \"" << name << "\": {\n";
            WriteVector( out, "position", tr.position ); out << ",\n";
            WriteVector( out, "rotation", tr.rotation ); out << ",\n";
            WriteVector( out, "scale", tr.scale ); out << "\n";
            out << "    }";
        }

        void WriteCurve( std::ostream &out, const String &name, const std::vector<CurveValue> &curve )
        {
            out << "    \"" << name << "\": [\n";
            for( size_t i = 0; i < curve.size(); ++i )
            {
                out << "      { \"time\": " << curve[i].time << ", \"value\": " << curve[i].value << " }";
                if( i + 1 < curve.size() )
                {
                    out << ",";
                }
                out << "\n";
            }
            out << "    ]";
        }

        void WriteWing( std::ostream &out, const AircraftWingData &wing, bool last )
        {
            out << "  {\n";
            out << "    \"name\": \"" << Escape( wing.name ) << "\",\n";
            WriteTransform( out, "localTransform", wing.localTransform ); out << ",\n";
            out << "    \"sectionCount\": " << wing.sectionCount << ",\n";
            out << "    \"liftLineChordPosition\": " << wing.liftLineChordPosition << ",\n";
            out << "    \"wingTipWidthZeroToOne\": " << wing.wingTipWidthZeroToOne << ",\n";
            out << "    \"wingTipSweep\": " << wing.wingTipSweep << ",\n";
            out << "    \"wingTipAngle\": " << wing.wingTipAngle << ",\n";
            out << "    \"aoaMultiplier\": " << wing.aoaMultiplier << ",\n";
            out << "    \"cdMultiplier\": " << wing.cdMultiplier << ",\n";
            out << "    \"clMultiplier\": " << wing.clMultiplier << ",\n";
            out << "    \"cmMultiplier\": " << wing.cmMultiplier << ",\n";
            out << "    \"stallControlCL\": " << wing.stallControlCL << ",\n";
            out << "    \"stallControlCD\": " << wing.stallControlCD << ",\n";
            out << "    \"stallControlCM\": " << wing.stallControlCM << ",\n";
            out << "    \"stallThreshold\": " << wing.stallThreshold << ",\n";
            out << "    \"aerofoilName\": \"" << Escape( wing.aerofoilName ) << "\",\n";
            out << "    \"subDivision\": { \"x\": " << wing.subDivision.x
                << ", \"y\": " << wing.subDivision.y
                << ", \"z\": " << wing.subDivision.z << " }\n";
            out << "  }" << ( last ? "" : "," ) << "\n";
        }

        void WriteControlSurface( std::ostream &out,
                                 const AircraftControlSurfaceData &cs,
                                 bool last )
        {
            out << "  {\n";
            out << "    \"name\": \"" << Escape( cs.name ) << "\",\n";
            out << "    \"reverse\": " << ( cs.reverse ? "true" : "false" ) << ",\n";
            out << "    \"minDeflectionDegrees\": " << cs.minDeflectionDegrees << ",\n";
            out << "    \"maxDeflectionDegrees\": " << cs.maxDeflectionDegrees << ",\n";
            out << "    \"rootHingeDistanceFromTrailingEdge\": "
                << cs.rootHingeDistanceFromTrailingEdge << ",\n";
            out << "    \"tipHingeDistanceFromTrailingEdge\": "
                << cs.tipHingeDistanceFromTrailingEdge << ",\n";
            out << "    \"surfaceId\": " << cs.surfaceId << "\n";
            out << "  }" << ( last ? "" : "," ) << "\n";
        }
    }  // namespace

    // ---------------------------------------------------------------------
    //  Vector helpers
    // ---------------------------------------------------------------------

    Vector3F JsonUtil::StringToVector3( const String &input, c8 splitChar )
    {
        String text = input;
        if( !text.empty() && text.front() == '(' && text.back() == ')' )
        {
            text = text.substr( 1, text.size() - 2 );
        }
        auto parts = Split( text, splitChar );
        Vector3F result;
        if( parts.size() >= 3 )
        {
            result.x = StringUtil::parseFloat( parts[0] );
            result.y = StringUtil::parseFloat( parts[1] );
            result.z = StringUtil::parseFloat( parts[2] );
        }
        return result;
    }

    String JsonUtil::Vector3ToString( const Vector3F &value, c8 splitChar )
    {
        std::ostringstream stream;
        stream << value.x << splitChar << " " << value.y << splitChar << " " << value.z;
        return stream.str();
    }

    // ---------------------------------------------------------------------
    //  Aircraft serialisation
    // ---------------------------------------------------------------------

    String JsonUtil::Serialize( const AircraftData &data )
    {
        std::ostringstream out;
        out << "{\n";
        out << "  \"cgPosition\": { \"x\": " << data.cgPosition.x
            << ", \"y\": " << data.cgPosition.y
            << ", \"z\": " << data.cgPosition.z << " },\n";
        out << "  \"drag\": { \"x\": " << data.drag.x
            << ", \"y\": " << data.drag.y
            << ", \"z\": " << data.drag.z << " },\n";
        out << "  \"rollwiseDamping\": " << data.rollwiseDamping << ",\n";
        out << "  \"sectionMultiplier\": " << data.sectionMultiplier << ",\n";

        out << "  \"wingData\": [\n";
        for( size_t i = 0; i < data.wingData.size(); ++i )
        {
            WriteWing( out, data.wingData[i], i + 1 == data.wingData.size() );
        }
        out << "  ],\n";

        out << "  \"controlSurfaceData\": [\n";
        for( size_t i = 0; i < data.controlSurfaceData.size(); ++i )
        {
            WriteControlSurface( out, data.controlSurfaceData[i],
                                 i + 1 == data.controlSurfaceData.size() );
        }
        out << "  ],\n";

        out << "  \"engineData\": [],\n";
        out << "  \"wheelData\": [],\n";
        out << "  \"propWashData\": []\n";
        out << "}\n";
        return out.str();
    }

    String JsonUtil::Serialize( const AircraftAirfoilData &data )
    {
        std::ostringstream out;
        out << "{\n";
        WriteCurve( out, "cl", data.cl );  out << ",\n";
        WriteCurve( out, "cd", data.cd );  out << ",\n";
        WriteCurve( out, "cm", data.cm );  out << ",\n";
        WriteCurve( out, "clPlus10", data.clPlus10 );  out << ",\n";
        WriteCurve( out, "cdPlus10", data.cdPlus10 );  out << ",\n";
        WriteCurve( out, "cmPlus10", data.cmPlus10 );  out << ",\n";
        WriteCurve( out, "clMinus10", data.clMinus10 );  out << ",\n";
        WriteCurve( out, "cdMinus10", data.cdMinus10 );  out << ",\n";
        WriteCurve( out, "cmMinus10", data.cmMinus10 );  out << "\n";
        out << "}\n";
        return out.str();
    }

    bool JsonUtil::SaveAircraftData( const String &filePath, const AircraftData &data )
    {
        std::ofstream stream( filePath.c_str() );
        if( !stream )
        {
            return false;
        }
        stream << Serialize( data );
        return stream.good();
    }

    bool JsonUtil::SaveAirfoil( const String &filePath, const AircraftAirfoilData &data )
    {
        std::ofstream stream( filePath.c_str() );
        if( !stream )
        {
            return false;
        }
        stream << Serialize( data );
        return stream.good();
    }

    TransmitterProfile JsonUtil::DeserializeTransmitterProfile( const String &text )
    {
        TransmitterProfile profile;
        JsonReader reader( text );
        if( !reader.consume( '{' ) )
        {
            return profile;
        }
        while( true )
        {
            reader.skipWhitespace();
            if( !reader.consume( '"' ) )
            {
                break;
            }
            auto key = reader.readString();
            if( !reader.consume( ':' ) )
            {
                break;
            }
            if( key == "functions" )
            {
                if( !reader.consume( '[' ) )
                {
                    break;
                }
                while( true )
                {
                    reader.skipWhitespace();
                    if( reader.consume( ']' ) )
                    {
                        break;
                    }
                    if( !reader.consume( '{' ) )
                    {
                        break;
                    }
                    Attrib entry;
                    while( true )
                    {
                        reader.skipWhitespace();
                        if( reader.consume( '}' ) )
                        {
                            break;
                        }
                        if( !reader.consume( '"' ) )
                        {
                            break;
                        }
                        auto field = reader.readString();
                        if( !reader.consume( ':' ) )
                        {
                            break;
                        }
                        if( field == "name" ) entry.name = reader.readString();
                        else if( field == "value" ) entry.value = reader.readString();
                        else if( field == "type" ) entry.groupType = reader.readString();
                        else if( field == "channel" ) entry.objectId = StringUtil::parseInt( reader.readValue() );
                        else if( field == "cmap" ) entry.refComponentId = StringUtil::parseInt( reader.readValue() );
                        else if( field == "offset" ) {} else if( field == "multiplier" ) {} else if( field == "low_multiplier" ) {} else if( field == "high_multiplier" ) {} else if( field == "reverse" ) entry.applied = reader.tryMatch( "true" );
                        else { reader.readValue(); }
                        reader.skipWhitespace();
                        reader.consume( ',' );
                    }
                    profile.functions.push_back( entry );
                    reader.skipWhitespace();
                    if( !reader.consume( ',' ) )
                    {
                        // last entry
                        reader.skipWhitespace();
                        reader.consume( ']' );
                        break;
                    }
                }
            }
            else
            {
                reader.readValue();
            }
            reader.skipWhitespace();
            if( !reader.consume( ',' ) )
            {
                break;
            }
        }
        return profile;
    }

    TransmitterProfile JsonUtil::LoadTransmitterProfile( const String &filePath )
    {
        std::ifstream stream( filePath.c_str() );
        if( !stream )
        {
            return TransmitterProfile();
        }
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        return DeserializeTransmitterProfile( buffer.str() );
    }
}  // namespace workphone::adbeditor
