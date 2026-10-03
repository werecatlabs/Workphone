#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/ParameterUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{

    auto ParameterUtil::setVector2( const Vector2F &vec ) -> Parameters
    {
        Parameters params;
        params[0].data.fData = vec.X();
        params[1].data.fData = vec.Y();
        return params;
    }

    auto ParameterUtil::getParamTypeAsString( u32 paramType ) -> String
    {
        return getParamTypeAsString( static_cast<ParameterType>( paramType ) );
    }

    auto ParameterUtil::getVector2( const Parameters &params ) -> Vector2F
    {
        return { params[0].data.fData, params[1].data.fData };
    }

    auto ParameterUtil::getVector3( const Parameters &params ) -> Vector3<real_Num>
    {
        return { params[0].data.fData, params[1].data.fData, params[2].data.fData };
    }

    auto ParameterUtil::getAABB2( const Parameters &params ) -> AABB2F
    {
        return { params[0].data.fData, params[1].data.fData, params[2].data.fData,
                 params[3].data.fData };
    }

    auto ParameterUtil::getAABB3( const Parameters &params ) -> AABB3F
    {
        return { params[0].data.fData, params[1].data.fData, params[2].data.fData,
                 params[3].data.fData, params[4].data.fData, params[5].data.fData };
    }

    auto ParameterUtil::getParamTypeAsString( ParameterType paramType ) -> String
    {
        switch( paramType )
        {
        case ParameterType::PARAM_TYPE_NULL:
            return "null";
        case ParameterType::PARAM_TYPE_BOOL:
            return "bool";
        case ParameterType::PARAM_TYPE_U8:
            return "u8";
        case ParameterType::PARAM_TYPE_U16:
            return "u16";
        case ParameterType::PARAM_TYPE_U32:
            return "u32";
        case ParameterType::PARAM_TYPE_S8:
            return "s8";
        case ParameterType::PARAM_TYPE_S16:
            return "s16";
        case ParameterType::PARAM_TYPE_S32:
            return "s32";
        case ParameterType::PARAM_TYPE_F32:
            return "f32";
        case ParameterType::PARAM_TYPE_S64:
            return "s64";
        case ParameterType::PARAM_TYPE_F64:
            return "f64";
        case ParameterType::PARAM_TYPE_CHAR_PTR:
            return "charptr";
        case ParameterType::PARAM_TYPE_PTR:
            return "ptr";
        case ParameterType::PARAM_TYPE_STR:
            return "str";
        case ParameterType::PARAM_TYPE_VEC2I:
            return "vec2i";
        case ParameterType::PARAM_TYPE_VEC2F:
            return "vec2f";
        case ParameterType::PARAM_TYPE_VEC3I:
            return "vec3i";
        case ParameterType::PARAM_TYPE_VEC3F:
            return "vec3f";
        default:
        {
        }
        }

        return {};
    }

    auto ParameterUtil::getParamTypeFromString( const String &paramTypeStr ) -> ParameterType
    {
        if( paramTypeStr == String( "null" ) )
        {
            return ParameterType::PARAM_TYPE_NULL;
        }

        return ParameterType::PARAM_TYPE_NULL;
    }
}  // namespace workphone
