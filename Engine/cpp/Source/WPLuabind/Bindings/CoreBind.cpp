#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/CoreBind.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    void include( const String &fileName )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = applicationManager->getScriptManagerPtr();
        if( scriptManager )
        {
            scriptManager->loadScript( fileName );
        }
        else
        {
            WP_LOG_ERROR( "Script manager is not available." );
        }
    }

    void setPropertyAsString( Properties *properties, const c8 *name, const c8 *value )
    {
        const auto nameStr = String( name );
        const auto valueStr = String( value );
        properties->setProperty( nameStr, valueStr );
    }

    String getPropertyAsString( Properties *properties, const c8 *name )
    {
        const auto propertyName = String( name );
        return properties->getProperty( propertyName );
    }

    void setPropertyAsInt( Properties *properties, const c8 *name, lua_Integer value )
    {
        const auto nameStr = String( name );
        properties->setProperty( nameStr, static_cast<s32>( value ) );
    }

    lua_Integer getPropertyAsInt( Properties *properties, const c8 *name )
    {
        const auto nameStr = String( name );
        return properties->getPropertyAsInt( nameStr );
    }

    void setPropertyAsBool( Properties *properties, const c8 *name, bool value )
    {
        const auto nameStr = String( name );
        properties->setProperty( nameStr, value );
    }

    bool getPropertyAsBool( Properties *properties, const c8 *name )
    {
        const auto nameStr = String( name );
        return properties->getPropertyAsBool( nameStr );
    }

    void setPropertyAsFloat( Properties *properties, const c8 *name, lua_Number value )
    {
        const auto nameStr = String( name );
        properties->setProperty( nameStr, value );
    }

    lua_Number getPropertyAsFloat( Properties *properties, const c8 *name )
    {
        const auto nameStr = String( name );
        return properties->getPropertyAsFloat( nameStr );
    }

    void bindCore( lua_State *L )
    {
        using namespace luabind;

        module( L )[def( "include", include )];

        module( L )[class_<ColourF>( "ColourF" )
                        .def( constructor<>() )
                        .def( constructor<const ColourF &>() )
                        .def( constructor<f32, f32, f32>() )
                        .def( constructor<f32, f32, f32, f32>() )
                        .def( "toSColor", &ColourF::toSColor )
                        .def( "set", static_cast<void ( ColourF::* )( f32, f32, f32 )>( &ColourF::set ) )
                        .def( "set",
                              static_cast<void ( ColourF::* )( f32, f32, f32, f32 )>( &ColourF::set ) )
                        .def( "getInterpolated", &ColourF::getInterpolated )
                        .def( "getInterpolated_quadratic", &ColourF::getInterpolated_quadratic )
                        .def( "setColorComponentValue", &ColourF::setColorComponentValue )
                        .def( "getAsRGBA", &ColourF::getAsRGBA )
                        .def( "getAsARGB", &ColourF::getAsARGB )
                        .def( "getAsBGRA", &ColourF::getAsBGRA )
                        .def( "getAsABGR", &ColourF::getAsABGR )
                        .def( "setAsRGBA", &ColourF::setAsRGBA )
                        .def( "setAsARGB", &ColourF::setAsARGB )
                        .def( "setAsBGRA", &ColourF::setAsBGRA )
                        .def( "setAsABGR", &ColourF::setAsABGR )
                        .def( "isValid", &ColourF::isValid )
                        //.def( self * f32() )
                        //.def( self *= f32() )
                        //.def( self == other<const ColourF &>() )
                        //.def( self != other<const ColourF &>() )
                        .def_readwrite( "r", &ColourF::r )
                        .def_readwrite( "g", &ColourF::g )
                        .def_readwrite( "b", &ColourF::b )
                        .def_readwrite( "a", &ColourF::a )];
        //.scope[def( "ZERO", &ColourF::ZERO ), def( "Black", &ColourF::Black ),
        //       def( "White", &ColourF::White ), def( "Red", &ColourF::Red ),
        //       def( "Green", &ColourF::Green ), def( "Blue", &ColourF::Blue )]];

        // module( L )[namespace_(
        //     "BitUtil" )[
        // def( "bit_and", &BitUtil::bit_and<lua_Integer> ),
        //         def( "bit_or", (lua_Integer ( * )( lua_Integer,
        //                                            lua_Integer ))&BitUtil::bit_or<lua_Integer> ),
        //         def( "bit_or", (lua_Integer ( * )( lua_Integer, lua_Integer,
        //                                            lua_Integer ))&BitUtil::bit_or<lua_Integer> ),
        //         def( "bit_or",
        //              (lua_Integer ( * )( lua_Integer, lua_Integer, lua_Integer,
        //                                 lua_Integer ))&BitUtil::bit_or<lua_Integer> )]
        //];

        module( L )[class_<Handle>( "Handle" )
                        .def( "getHash", &Handle::getHash )
                        .def( "setHash", &Handle::setHash )
                        //.def( "getUUID", &Handle::getUUID )
                        //.def( "setUUID", &Handle::setUUID )
                        .def( "getId", &Handle::getId )
                        .def( "setId", &Handle::setId )
                        .def( "getInstanceId", &Handle::getInstanceId )
                        .def( "setInstanceId", &Handle::setInstanceId )
                        .def( "operator==", &Handle::operator== )
                        .def( "operator=", &Handle::operator= )
                        .def( "toString", &Handle::toString )];

        module( L )
            [class_<Properties, ISharedObject, SmartPtr<ISharedObject>>( "Properties" )
                 .def( "setName", &Properties::setName )
                 .def( "getName", &Properties::getName )
                 .def( "clearAll", &Properties::clearAll )
                 .def( "addProperty", static_cast<void ( Properties::* )( const Property & )>(
                                          &Properties::addProperty ) )
                 .def( "addProperty", static_cast<void ( Properties::* )( const String &, const String &,
                                                                          const String &, bool )>(
                                          &Properties::addProperty ) )
                 .def( "setPropertyAsString", setPropertyAsString )
                 .def( "setProperty", setPropertyAsString )
                 .def( "setPropertyAsInt", setPropertyAsInt )
                 .def( "setPropertyAsBool", setPropertyAsBool )
                 .def( "setPropertyAsFloat", setPropertyAsFloat )
                 .def( "setProperty", static_cast<void ( Properties::* )( const String &, const String &,
                                                                          const String & )>(
                                          &Properties::setProperty ) )
                 .def( "setProperty", static_cast<void ( Properties::* )( const String &, const String &,
                                                                          const String &, bool )>(
                                          &Properties::setProperty ) )
                 .def( "setPropertyType", &Properties::setPropertyType )
                 .def( "removeProperty", &Properties::removeProperty )
                 //.def("getProperty", &Properties::getProperty)
                 .def( "hasProperty", &Properties::hasProperty )
                 .def( "getPropertyObject", static_cast<Property &(Properties::*)( const String & )>(
                                                &Properties::getPropertyObject ) )
                 .def( "getPropertyObject", static_cast<const Property &(Properties::*)( const String & )
                                                            const>( &Properties::getPropertyObject ) )
                 .def( "propertyValueEquals", &Properties::propertyValueEquals )
                 .def( "getProperty", static_cast<String ( Properties::* )( const String &, String )
                                                      const>( &Properties::getProperty ) )
                 .def( "getPropertyAsString", getPropertyAsString )
                 .def( "getProperty", getPropertyAsString )
                 .def( "getPropertyAsBool", &Properties::getPropertyAsBool )
                 .def( "getPropertyAsBool", &getPropertyAsBool )
                 .def( "getPropertyAsInt", &Properties::getPropertyAsInt )
                 .def( "getPropertyAsInt", &getPropertyAsInt )
                 .def( "getPropertyAsFloat", &Properties::getPropertyAsFloat )
                 .def( "getPropertyAsFloat", &getPropertyAsFloat )
                 .def( "getPropertyAsVector3", &Properties::getPropertyAsVector3 )
                 .def( "getPropertyAsVector3D", &Properties::getPropertyAsVector3D )
                 .def( "getPropertiesAsArray", &Properties::getPropertiesAsArray )
                 .def( "toData", &Properties::toData )
                 .def( "fromData", &Properties::fromData )
                 .def( "operator=", &Properties::operator= )
                 .def( "getChildren", &Properties::getChildren )
                 .def( "addChild", &Properties::addChild )
                 .def( "removeChild", &Properties::removeChild )
                 .def( "removeChild",
                       (void ( Properties::* )( const SmartPtr<Properties> & ))&Properties::removeChild )
                 .def( "removeChild", (void ( Properties::* )( u32 ))&Properties::removeChild )
                 .def( "getNumChildren", &Properties::getNumChildren )
                 .def( "hasChild", &Properties::hasChild )
                 .def( "getChild", static_cast<SmartPtr<Properties> ( Properties::* )( u32 ) const>(
                                       &Properties::getChild ) )
                 .def( "getChild", static_cast<SmartPtr<Properties> ( Properties::* )( const String & )
                                                   const>( &Properties::getChild ) )
                 .def( "getChildrenByName", &Properties::getChildrenByName )
                 .def( "setPropertiesAsArray", &Properties::setPropertiesAsArray )
                 .def( "setButtonPressed", &Properties::setButtonPressed )
                 .def( "isButtonPressed", &Properties::isButtonPressed )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const String &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Array<String> &,
                                                           bool )>( &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const char *const &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const bool &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const s32 &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const u32 &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const u64 &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const f32 &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const f64 &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Vector2I &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Vector2F &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Vector2D &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Vector3I &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Vector3F &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Vector3D &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const QuaternionF &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const QuaternionD &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Transform3F &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const Transform3D &, bool )>(
                           &Properties::setProperty ) )
                 .def( "setProperty",
                       static_cast<void ( Properties::* )( const String &, const ColourF &, bool )>(
                           &Properties::setProperty ) )
                 .scope[def( "typeInfo", Properties::typeInfo )]];
    }
} // namespace workphone
