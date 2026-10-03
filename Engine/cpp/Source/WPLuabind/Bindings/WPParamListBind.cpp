#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/WPParamListBind.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include "WPLuabind/SmartPtrConverter.hpp"
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    namespace
    {
        using StringVector = Array<String>;

        const Parameter *findParameter( const Parameters &params, lua_Integer index )
        {
            if( index < 0 || static_cast<size_t>( index ) >= params.size() )
            {
                return nullptr;
            }

            return &params[static_cast<size_t>( index )];
        }

        void addParameter( Parameters &params, const Parameter &value )
        {
            params.push_back( value );
        }

        void addParamAsBool( Parameters &params, bool value )
        {
            params.emplace_back( value );
        }

        void addParamAsInt( Parameters &params, lua_Integer value )
        {
            params.emplace_back( static_cast<s64>( value ) );
        }

        void addParamAsNumber( Parameters &params, lua_Number value )
        {
            params.emplace_back( static_cast<f64>( value ) );
        }

        void addParamAsString( Parameters &params, const String &value )
        {
            params.emplace_back( value );
        }

        void addParamAsObject( Parameters &params, SmartPtr<ISharedObject> value )
        {
            params.emplace_back( value );
        }

        Parameter getParam( const Parameters &params, lua_Integer index )
        {
            if( const auto *parameter = findParameter( params, index ) )
            {
                return *parameter;
            }

            return {};
        }

        bool getParamAsBool( const Parameters &params, lua_Integer index )
        {
            if( const auto *param = findParameter( params, index ) )
            {
                return param->getBool();
            }

            return false;
        }

        lua_Integer getParamAsInt( const Parameters &params, lua_Integer index )
        {
            const auto *param = findParameter( params, index );
            if( !param )
            {
                return 0;
            }

            if( param->type == ParameterType::PARAM_TYPE_S64 )
            {
                return static_cast<lua_Integer>( param->getS64() );
            }

            return static_cast<lua_Integer>( param->getS32() );
        }

        lua_Number getParamAsNumber( const Parameters &params, lua_Integer index )
        {
            const auto *param = findParameter( params, index );
            if( !param )
            {
                return 0.0;
            }

            if( param->type == ParameterType::PARAM_TYPE_F64 )
            {
                return static_cast<lua_Number>( param->getF64() );
            }

            return static_cast<lua_Number>( param->getF32() );
        }

        String getParamAsString( const Parameters &params, lua_Integer index )
        {
            if( const auto *param = findParameter( params, index ) )
            {
                return param->getStr();
            }

            return {};
        }

        SmartPtr<ISharedObject> getParamAsObject( const Parameters &params, lua_Integer index )
        {
            const auto *param = findParameter( params, index );
            if( !param )
            {
                return nullptr;
            }

            if( param->object )
            {
                return param->getObject();
            }

            return static_cast<ISharedObject *>( param->getPtr() );
        }

        StringVector getParamAsStringArray( const Parameters &params )
        {
            StringVector values;
            values.reserve( params.size() );

            for( const auto &param : params )
            {
                values.push_back( param.getStr() );
            }

            return values;
        }

        void popParameter( Parameters &params )
        {
            if( !params.empty() )
            {
                params.pop_back();
            }
        }

        void eraseParameter( Parameters &params, lua_Integer index )
        {
            if( index >= 0 && static_cast<size_t>( index ) < params.size() )
            {
                params.erase( params.begin() + static_cast<size_t>( index ) );
            }
        }

        lua_Integer getListSize( const Parameters &params )
        {
            return static_cast<lua_Integer>( params.size() );
        }
    } // namespace

    void bindParamList( lua_State *L )
    {
        using namespace luabind;

        module(
            L )[class_<StringVector>( "StringVector" )
                    .def( constructor<>() )
                    .def( "push_back", static_cast<void ( StringVector::* )( const String & )>(
                                           &StringVector::push_back ) )
                    .def( "pop_back", &StringVector::pop_back )
                    .def( "clear", &StringVector::clear )
                    .def( "reserve", &StringVector::reserve )
                    .def( "size", &StringVector::size )
                    .def( "empty", &StringVector::empty )
                    .def( "at", static_cast<String &(StringVector::*)( size_t )>( &StringVector::at ) )];

        module( L )[class_<Parameters>( "Parameters" )
                        .def( constructor<>() )
                        .def( "add", addParameter )
                        .def( "push_back", addParameter )
                        .def( "addAsBool", addParamAsBool )
                        .def( "addAsInt", addParamAsInt )
                        .def( "addAsNumber", addParamAsNumber )
                        .def( "addAsString", addParamAsString )
                        .def( "addAsObject", addParamAsObject )
                        .def( "get", getParam )
                        .def( "at", getParam )
                        .def( "getAsBool", getParamAsBool )
                        .def( "getAsInt", getParamAsInt )
                        .def( "getAsNumber", getParamAsNumber )
                        .def( "getAsString", getParamAsString )
                        .def( "getAsObject", getParamAsObject )
                        .def( "getAsStringArray", getParamAsStringArray )
                        .def( "pop_back", popParameter )
                        .def( "erase", eraseParameter )
                        .def( "clear", &Parameters::clear )
                        .def( "reserve", &Parameters::reserve )
                        .def( "empty", &Parameters::empty )
                        .def( "size", getListSize )];
    }
} // namespace workphone
