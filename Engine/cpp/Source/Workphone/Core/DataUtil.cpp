#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/JsonParser.hpp>
#include <Workphone/Core/JsonWriter.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/XmlUtil.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <tinyxml.h>
#include <cJSON.h>
#include <vector>
#include <string>
#include <iostream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace workphone;

namespace
{
    JsonValue parseJsonValue( const String &jsonDataStr )
    {
        auto parserJson = JsonParser( jsonDataStr.c_str(), static_cast<u32>( jsonDataStr.size() ) );
        return parserJson.parse();
    }

    bool isJsonNumberInt32( f64 value )
    {
        if( !std::isfinite( value ) )
        {
            return false;
        }

        if( value < static_cast<f64>( std::numeric_limits<s32>::min() ) ||
            value > static_cast<f64>( std::numeric_limits<s32>::max() ) )
        {
            return false;
        }

        f64 integerPart = 0.0;
        return std::modf( value, &integerPart ) == 0.0;
    }

    String jsonNumberToString( f64 value )
    {
        std::ostringstream stream;
        stream.imbue( std::locale::classic() );
        stream << std::setprecision( std::numeric_limits<f64>::max_digits10 ) << value;
        return stream.str().c_str();
    }

    bool isScalarJsonValue( const JsonValue &value )
    {
        return std::holds_alternative<std::nullptr_t>( value ) ||
               std::holds_alternative<bool>( value ) || std::holds_alternative<f64>( value ) ||
               std::holds_alternative<String>( value );
    }

    String scalarJsonValueToString( const JsonValue &value )
    {
        if( std::holds_alternative<std::nullptr_t>( value ) )
        {
            return "null";
        }

        if( auto boolValue = std::get_if<bool>( &value ) )
        {
            return StringUtil::toString( *boolValue );
        }

        if( auto numberValue = std::get_if<f64>( &value ) )
        {
            return jsonNumberToString( *numberValue );
        }

        if( auto stringValue = std::get_if<String>( &value ) )
        {
            return *stringValue;
        }

        return {};
    }

    void setScalarProperty( Properties *properties, const String &key, const JsonValue &value )
    {
        if( auto stringValue = std::get_if<String>( &value ) )
        {
            properties->setProperty( key, *stringValue );
        }
        else if( auto numberValue = std::get_if<f64>( &value ) )
        {
            if( isJsonNumberInt32( *numberValue ) )
            {
                properties->setProperty( key, static_cast<s32>( *numberValue ) );
            }
            else
            {
                properties->setProperty( key, *numberValue );
            }
        }
        else if( auto boolValue = std::get_if<bool>( &value ) )
        {
            properties->setProperty( key, *boolValue );
        }
    }

    bool setScalarArrayProperty( Properties *properties, const String &key, const JsonArray &array )
    {
        Array<String> values;
        values.reserve( array.size() );

        for( const auto &element : array )
        {
            if( !isScalarJsonValue( element ) )
            {
                return false;
            }

            values.push_back( scalarJsonValueToString( element ) );
        }

        properties->setProperty( key, values );
        return true;
    }

    bool getJsonNumber( const JsonObject &obj, const String &key, f64 &value )
    {
        auto it = obj.find( key );
        if( it == obj.end() )
        {
            return false;
        }

        if( auto numberValue = std::get_if<f64>( &it->second ) )
        {
            value = *numberValue;
            return true;
        }

        if( auto stringValue = std::get_if<String>( &it->second ) )
        {
            value = StringUtil::parseDouble( *stringValue );
            return true;
        }

        return false;
    }

    template <class T>
    void parseVector2( const JsonObject *obj, Vector2<T> *value )
    {
        if( !obj || !value )
        {
            return;
        }

        f64 component = 0.0;
        if( getJsonNumber( *obj, "x", component ) )
        {
            value->x = static_cast<T>( component );
        }

        if( getJsonNumber( *obj, "y", component ) )
        {
            value->y = static_cast<T>( component );
        }
    }

    template <class T>
    void parseVector3( const JsonObject *obj, Vector3<T> *value )
    {
        if( !obj || !value )
        {
            return;
        }

        f64 component = 0.0;
        if( getJsonNumber( *obj, "x", component ) )
        {
            value->x = static_cast<T>( component );
        }

        if( getJsonNumber( *obj, "y", component ) )
        {
            value->y = static_cast<T>( component );
        }

        if( getJsonNumber( *obj, "z", component ) )
        {
            value->z = static_cast<T>( component );
        }
    }

    void writeJsonValue( JsonWriter &writer, const JsonValue &value )
    {
        if( std::holds_alternative<std::nullptr_t>( value ) )
        {
            writer.null();
        }
        else if( auto boolValue = std::get_if<bool>( &value ) )
        {
            writer.boolean( *boolValue );
        }
        else if( auto numberValue = std::get_if<f64>( &value ) )
        {
            writer.number( *numberValue );
        }
        else if( auto stringValue = std::get_if<String>( &value ) )
        {
            writer.string( *stringValue );
        }
        else if( auto arrayValue = std::get_if<JsonArray>( &value ) )
        {
            writer.beginArray();
            for( const auto &element : *arrayValue )
            {
                writeJsonValue( writer, element );
            }
            writer.endArray();
        }
        else if( auto objectValue = std::get_if<JsonObject>( &value ) )
        {
            writer.beginObject();
            for( const auto &[key, val] : *objectValue )
            {
                writer.key( key );
                writeJsonValue( writer, val );
            }
            writer.endObject();
        }
    }

    String quoteJsonString( const String &value )
    {
        auto output = StringOutput();
        auto writer = JsonWriter( output );
        writer.writeString( value );
        return output.buffer;
    }

    void prettyPrintJsonValue( std::ostream &os, const JsonValue &jsonValue, String *indent = nullptr )
    {
        String indentValue;
        if( !indent )
        {
            indent = &indentValue;
        }

        if( auto objectValue = std::get_if<JsonObject>( &jsonValue ) )
        {
            os << "{";
            if( !objectValue->empty() )
            {
                os << "\n";
                indent->append( 4, ' ' );

                auto it = objectValue->begin();
                while( it != objectValue->end() )
                {
                    os << *indent << quoteJsonString( it->first ) << " : ";
                    prettyPrintJsonValue( os, it->second, indent );

                    ++it;
                    if( it != objectValue->end() )
                    {
                        os << ",\n";
                    }
                }

                os << "\n";
                indent->resize( indent->size() - 4 );
                os << *indent;
            }
            os << "}";
        }
        else if( auto arrayValue = std::get_if<JsonArray>( &jsonValue ) )
        {
            os << "[";
            if( !arrayValue->empty() )
            {
                os << "\n";
                indent->append( 4, ' ' );

                auto it = arrayValue->begin();
                while( it != arrayValue->end() )
                {
                    os << *indent;
                    prettyPrintJsonValue( os, *it, indent );

                    ++it;
                    if( it != arrayValue->end() )
                    {
                        os << ",\n";
                    }
                }

                os << "\n";
                indent->resize( indent->size() - 4 );
                os << *indent;
            }
            os << "]";
        }
        else if( auto stringValue = std::get_if<String>( &jsonValue ) )
        {
            os << quoteJsonString( *stringValue );
        }
        else if( auto numberValue = std::get_if<f64>( &jsonValue ) )
        {
            os << jsonNumberToString( *numberValue );
        }
        else if( auto boolValue = std::get_if<bool>( &jsonValue ) )
        {
            os << ( *boolValue ? "true" : "false" );
        }
        else
        {
            os << "null";
        }

        if( indent->empty() )
        {
            os << "\n";
        }
    }

    String serializeJsonValue( const JsonValue &jsonValue, bool formatted )
    {
        if( formatted )
        {
            std::ostringstream stream;
            prettyPrintJsonValue( stream, jsonValue );
            return stream.str().c_str();
        }

        auto output = StringOutput();
        auto writer = JsonWriter( output );
        writeJsonValue( writer, jsonValue );
        return output.buffer;
    }

    void setPropertyValueFromJson( Property &property, const JsonValue &value )
    {
        if( auto stringValue = std::get_if<String>( &value ) )
        {
            property.setValue( *stringValue );
            property.setTypeName( Properties::stringTypeStr );
        }
        else if( auto numberValue = std::get_if<f64>( &value ) )
        {
            if( isJsonNumberInt32( *numberValue ) )
            {
                property.setValue( StringUtil::toString( static_cast<s32>( *numberValue ) ) );
                property.setTypeName( Properties::intTypeStr );
            }
            else
            {
                property.setValue( jsonNumberToString( *numberValue ) );
                property.setTypeName( Properties::doubleTypeStr );
            }
        }
        else if( auto boolValue = std::get_if<bool>( &value ) )
        {
            property.setValue( StringUtil::toString( *boolValue ) );
            property.setTypeName( Properties::boolTypeStr );
        }
    }
}  // namespace

#define WP_DECLARE_DATA_CLASS( T )                                      \
    template <>                                                         \
    String DataUtil::toString( T *ptr, bool formatted, DataFormat fmt ) \
    {                                                                   \
        switch( fmt )                                                   \
        {                                                               \
        case DataFormat::JSON:                                          \
        {                                                               \
            return objectToJsonStr( *ptr, formatted );                  \
        }                                                               \
        default:                                                        \
        {                                                               \
            return "";                                                  \
        }                                                               \
        }                                                               \
                                                                        \
        return "";                                                      \
    }                                                                   \
                                                                        \
    template <>                                                         \
    void DataUtil::parse( const String &str, T *ptr, DataFormat fmt )   \
    {                                                                   \
        if( !StringUtil::isNullOrEmpty( str ) )                         \
        {                                                               \
            switch( fmt )                                               \
            {                                                           \
            case DataFormat::JSON:                                      \
            {                                                           \
                auto jsonData = parseJsonValue( str );                  \
                                                                        \
                if( auto obj = std::get_if<JsonObject>( &jsonData ) )   \
                {                                                       \
                    DataUtil::parseJson( obj, ptr );                    \
                }                                                       \
            }                                                           \
            break;                                                      \
            case DataFormat::YAML:                                      \
            {                                                           \
                parseYAML( str, ptr );                                  \
            }                                                           \
            break;                                                      \
            }                                                           \
        }                                                               \
    }

//WP_DECLARE_DATA_CLASS( Vector2F )
//WP_DECLARE_DATA_CLASS( Vector2D )
//WP_DECLARE_DATA_CLASS( Vector3F )
//WP_DECLARE_DATA_CLASS( Vector3D )
//
//WP_DECLARE_DATA_CLASS( Transform3F )
//WP_DECLARE_DATA_CLASS( Transform3D )
//
//WP_DECLARE_DATA_CLASS( Property )
//WP_DECLARE_DATA_CLASS( Properties )

auto fromObject( const JsonObject &obj ) -> SmartPtr<Properties>
{
    auto properties = workphone::make_ptr<Properties>();

    for( const auto &[key, value] : obj )
    {
        if( isScalarJsonValue( value ) )
        {
            setScalarProperty( properties.get(), key, value );
        }
        else if( auto objectValue = std::get_if<JsonObject>( &value ) )
        {
            auto child = fromObject( *objectValue );

            child->setName( key );
            properties->addChild( child );
        }
        else if( auto arrayValue = std::get_if<JsonArray>( &value ) )
        {
            if( setScalarArrayProperty( properties.get(), key, *arrayValue ) )
            {
                continue;
            }

            for( const auto &element : *arrayValue )
            {
                if( auto childObj = std::get_if<JsonObject>( &element ) )
                {
                    auto child = fromObject( *childObj );

                    child->setName( key );
                    properties->addChild( child );
                }
            }
        }
    }

    return properties;
}

auto propertiesFromObject( const JsonObject &obj ) -> SmartPtr<Properties>
{
    try
    {
        auto properties = workphone::make_ptr<Properties>();

        bool isPropertyName = false;
        bool isPropertyValue = false;

        Property property;

        for( const auto &[key, value] : obj )
        {
            if( key == "handle_name" )
            {
                if( auto stringValue = std::get_if<String>( &value ) )
                {
                    properties->setName( *stringValue );
                }
            }
            else if( key == "name" )
            {
                isPropertyName = true;

                if( auto stringValue = std::get_if<String>( &value ) )
                {
                    property.setName( *stringValue );
                }
            }
            else if( key == "value" )
            {
                isPropertyValue = true;
                setPropertyValueFromJson( property, value );
            }
            else if( key == "children" )
            {
                if( auto arrayValue = std::get_if<JsonArray>( &value ) )
                {
                    for( const auto &element : *arrayValue )
                    {
                        if( auto childObj = std::get_if<JsonObject>( &element ) )
                        {
                            auto child = propertiesFromObject( *childObj );

                            properties->addChild( child );
                        }
                    }
                }
            }
            else if( isScalarJsonValue( value ) )
            {
                setScalarProperty( properties.get(), key, value );
            }
            else if( auto objectValue = std::get_if<JsonObject>( &value ) )
            {
                auto child = propertiesFromObject( *objectValue );

                child->setName( key );
                properties->addChild( child );
            }
            else if( auto arrayValue = std::get_if<JsonArray>( &value ) )
            {
                if( setScalarArrayProperty( properties.get(), key, *arrayValue ) )
                {
                    continue;
                }

                for( const auto &element : *arrayValue )
                {
                    if( auto childObj = std::get_if<JsonObject>( &element ) )
                    {
                        auto child = propertiesFromObject( *childObj );

                        auto childName = child->getName();
                        if( StringUtil::isNullOrEmpty( childName ) )
                        {
                            child->setName( key );
                        }

                        properties->addChild( child );
                    }
                }
            }
        }

        if( isPropertyName && isPropertyValue )
        {
            properties->addProperty( property );
        }
        else if( isPropertyName )
        {
            properties->setProperty( "name", property.getName() );
        }

        return properties;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    return nullptr;
}

auto DataUtil::parseJson( const String &jsonDataStr ) -> SmartPtr<Properties>
{
    auto jsonData = parseJsonValue( jsonDataStr );

    if( auto obj = std::get_if<JsonObject>( &jsonData ) )
    {
        return fromObject( *obj );
    }

    return nullptr;
}

auto DataUtil::parsePropertiesFromJson( const String &jsonDataStr ) -> SmartPtr<Properties>
{
    try
    {
        if( StringUtil::isNullOrEmpty( jsonDataStr ) )
        {
            return nullptr;
        }

        auto jsonData = parseJsonValue( jsonDataStr );

        if( auto obj = std::get_if<JsonObject>( &jsonData ) )
        {
            return propertiesFromObject( *obj );
        }

        return nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    return nullptr;
}

namespace workphone
{

    void propertiesToUSD( Properties *ptr )
    {
    }

    void DataUtil::parse( SmartPtr<Properties> properties, ColourF &value )
    {
        if( properties )
        {
            value.r = properties->getPropertyAsFloat( "x" );
            value.g = properties->getPropertyAsFloat( "y" );
            value.b = properties->getPropertyAsFloat( "z" );
            value.a = properties->getPropertyAsFloat( "w" );
        }
    }

    void DataUtil::parse( SmartPtr<Properties> properties, QuaternionF &value )
    {
        if( properties )
        {
            value.x = properties->getPropertyAsFloat( "x" );
            value.y = properties->getPropertyAsFloat( "y" );
            value.z = properties->getPropertyAsFloat( "z" );
            value.w = properties->getPropertyAsFloat( "w" );
        }
    }

    void DataUtil::parse( SmartPtr<Properties> properties, Vector4F &value )
    {
        if( properties )
        {
            value.x = properties->getPropertyAsFloat( "x" );
            value.y = properties->getPropertyAsFloat( "y" );
            value.z = properties->getPropertyAsFloat( "z" );
            value.w = properties->getPropertyAsFloat( "w" );
        }
    }

    void DataUtil::parse( SmartPtr<Properties> properties, Vector3F &value )
    {
        if( properties )
        {
            value.x = properties->getPropertyAsFloat( "x" );
            value.y = properties->getPropertyAsFloat( "y" );
            value.z = properties->getPropertyAsFloat( "z" );
        }
    }

    void DataUtil::parse( SmartPtr<Properties> properties, Vector2F &value )
    {
        if( properties )
        {
            value.x = properties->getPropertyAsFloat( "x" );
            value.y = properties->getPropertyAsFloat( "y" );
        }
    }

    auto DataUtil::formatJson( const String &jsonString ) -> String
    {
        auto jsonValue = parseJsonValue( jsonString );
        return serializeJsonValue( jsonValue, true );
    }

    void DataUtil::parseJson( const JsonObject *obj, Vector2I *value )
    {
        parseVector2( obj, value );
    }

    void DataUtil::parseJson( const JsonObject *obj, Vector2F *value )
    {
        parseVector2( obj, value );
    }

    void DataUtil::parseJson( const JsonObject *obj, Vector2D *value )
    {
        parseVector2( obj, value );
    }

    void DataUtil::parseJson( const JsonObject *obj, Vector3I *value )
    {
        parseVector3( obj, value );
    }

    void DataUtil::parseJson( const JsonObject *obj, Vector3F *value )
    {
        parseVector3( obj, value );
    }

    void DataUtil::parseJson( const JsonObject *obj, Vector3D *value )
    {
        parseVector3( obj, value );
    }

    void DataUtil::parseJson( const JsonObject *obj, Transform3F *ptr )
    {
    }

    void DataUtil::parseJson( const JsonObject *obj, Transform3D *ptr )
    {
    }

    void DataUtil::parseJson( const JsonObject *obj, Properties *ptr )
    {
        if( !obj || !ptr )
        {
            return;
        }

        if( auto properties = propertiesFromObject( *obj ) )
        {
            *ptr = *properties;
        }
    }

    void DataUtil::parseJson( const JsonObject *obj, Property *ptr )
    {
        if( !obj || !ptr )
        {
            return;
        }

        if( auto nameIt = obj->find( "name" ); nameIt != obj->end() )
        {
            if( auto name = std::get_if<String>( &nameIt->second ) )
            {
                ptr->setName( *name );
            }
        }

        if( auto valueIt = obj->find( "value" ); valueIt != obj->end() )
        {
            setPropertyValueFromJson( *ptr, valueIt->second );
        }

        if( auto typeIt = obj->find( "type" ); typeIt != obj->end() )
        {
            if( auto type = std::get_if<String>( &typeIt->second ) )
            {
                ptr->setTypeName( *type );
            }
        }

        if( auto readOnlyIt = obj->find( "readOnly" ); readOnlyIt != obj->end() )
        {
            if( auto readOnly = std::get_if<bool>( &readOnlyIt->second ) )
            {
                ptr->setReadOnly( *readOnly );
            }
            else if( auto readOnly = std::get_if<String>( &readOnlyIt->second ) )
            {
                ptr->setReadOnly( StringUtil::parseBool( *readOnly, false ) );
            }
        }
    }

    JsonObject DataUtil::convert( Properties *properties )
    {
        if( !properties )
        {
            return {};
        }

        JsonObject j;

        const auto propertiesName = properties->getName();
        static const String handleNameStr = "handle_name";
        j[handleNameStr] = JsonValue( propertiesName );

        auto propertiesArray = properties->getPropertiesAsArray();
        for( auto &prop : propertiesArray )
        {
            auto name = prop.getName();
            auto value = prop.getValue();
            j[name] = JsonValue( value );
        }

        auto children = properties->getChildren();

        JsonArray array;
        array.reserve( children.size() );

        for( auto child : children )
        {
            if( child )
            {
                auto childJson = convert( child.get() );
                array.push_back( JsonValue( childJson ) );
            }
        }

        static const String childrenStr = "children";
        j[childrenStr] = JsonValue( array );

        return j;
    }

    template <>
    auto WPCore_API DataUtil::toString( Properties *ptr, bool formatted, DataFormat fmt ) -> String
    {
        try
        {
            switch( fmt )
            {
            case DataFormat::JSON:
            {
                auto jsonValue = convert( ptr );
                return serializeJsonValue( JsonValue( jsonValue ), formatted );
            }
            break;
            case DataFormat::XML:
            {
                auto dataStr = XmlUtil::toString( *ptr );
                return XmlUtil::toString( *ptr );
            }
            break;
            case DataFormat::USD:
            {
                // Create a new USD stage (a .usd file)
                //UsdStageRefPtr stage = UsdStage::CreateNew( "test.usda" );

                // Define a transform node at path /World
                //UsdGeomXform xform = UsdGeomXform::Define( stage, SdfPath( "/World" ) );

                return String( "Scene" );
            }
            break;
            default:
            {
            }
            break;
            };
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    template <>
    void WPCore_API DataUtil::parse( const String &dataStr, Properties *ptr, DataFormat fmt )
    {
        if( !StringUtil::isNullOrEmpty( dataStr ) )
        {
            switch( fmt )
            {
            case DataFormat::JSON:
            {
                if( auto properties = parsePropertiesFromJson( dataStr ) )
                {
                    *ptr = *properties;
                }
            }
            break;
            case DataFormat::XML:
            {
                XmlUtil::parse( *ptr, dataStr );
            }
            break;
            case DataFormat::YAML:
            {
            }
            break;
            case DataFormat::USD:
            {
                // Create a new USD stage (a .usd file)
                //UsdStageRefPtr stage = UsdStage::CreateNew( "test.usda" );

                // Define a transform node at path /World
                //UsdGeomXform xform = UsdGeomXform::Define( stage, SdfPath( "/World" ) );
            }
            break;
            default:
            {
            }
            break;
            };
        }
    }

    template <>
    auto WPCore_API DataUtil::toString( ISharedObject *ptr, bool formatted, DataFormat fmt ) -> String
    {
        switch( fmt )
        {
        case DataFormat::JSON:
        {
            if( ptr->isDerived<Properties>() )
            {
                auto properties = (Properties *)ptr;
                return toString( properties, formatted, fmt );
            }
        }
        break;
        case DataFormat::XML:
        {
            if( ptr->isDerived<Properties>() )
            {
                auto properties = (Properties *)ptr;
                return toString( properties, formatted, fmt );
            }
        }
        break;
        case DataFormat::USD:
        {
        }
        break;
        default:
        {
        }
        break;
        };

        return {};
    }

    template <>
    void WPCore_API DataUtil::parse( const String &dataStr, ISharedObject *ptr, DataFormat fmt )
    {
        if( ptr->isDerived<Properties>() )
        {
            auto properties = (Properties *)ptr;
            if( fmt == DataFormat::JSON )
            {
                if( auto parsedProperties = parsePropertiesFromJson( dataStr ) )
                {
                    *properties = *parsedProperties;
                }
            }
        }
    }

    String DataUtil::formatText( const String &text, DataFormat format )
    {
        switch( format )
        {
        case DataFormat::JSON:
        {
            try
            {
                return formatJson( text );
            }
            catch( const std::exception &e )
            {
                std::cerr << "JSON parsing error: " << e.what() << "\n";
                return {};
            }
        }
        break;
        default:
        {
        }
        };

        return {};
    }

    bool DataUtil::isValidData( const String &jsonDataStr, DataFormat format )
    {
        switch( format )
        {
        case DataFormat::JSON:
        {
            try
            {
                auto jsonData = parseJsonValue( jsonDataStr );
                if( std::holds_alternative<JsonObject>( jsonData ) )
                {
                    return true;
                }
            }
            catch( const std::exception &e )
            {
                std::cerr << "JSON parsing error: " << e.what() << "\n";
            }
        }
        break;
        case DataFormat::USD:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        return false;
    }

    bool DataUtil::isFormat( const String &text, DataFormat fmt )
    {
        switch( fmt )
        {
        case DataFormat::JSON:
        {
            try
            {
                auto jsonData = parseJsonValue( text );
                return true;
            }
            catch( const std::exception & /*e*/ )
            {
                // Not valid JSON
                return false;
            }
        }
        break;
        case DataFormat::XML:
        {
            try
            {
                auto doc = XmlUtil::parseDocument( text );
                if( doc )
                {
                    return true;
                }
            }
            catch( const std::exception & /*e*/ )
            {
                // Not valid XML
                return false;
            }
        }
        break;
        default:
        {
        }
        break;
        }

        return false;
    }

}  // namespace workphone
