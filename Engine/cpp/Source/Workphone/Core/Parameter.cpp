#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone
{

    Parameter::Parameter() : type( ParameterType::PARAM_TYPE_NULL )
    {
        data.pData = nullptr;

#if !WP_FINAL
        constexpr auto ptrSize = sizeof( void * );
        constexpr auto smartPtrSize = sizeof( SmartPtr<ISharedObject> );
#endif
    }

    Parameter::~Parameter() = default;

    void Parameter::setBool( bool value )
    {
        type = ParameterType::PARAM_TYPE_BOOL;
        data.bData = value;
    }

    void Parameter::setCharPtr( const c8 *value )
    {
        type = ParameterType::PARAM_TYPE_CHAR_PTR;
        data.pData = (void *)value;
    }

    void Parameter::setU8( u8 value )
    {
        type = ParameterType::PARAM_TYPE_U8;
        data.iData = value;
    }

    void Parameter::setU16( u16 value )
    {
        type = ParameterType::PARAM_TYPE_U16;
        data.iData = value;
    }

    void Parameter::setS32( s32 value )
    {
        type = ParameterType::PARAM_TYPE_S32;
        data.iData = value;
    }

    void Parameter::setU32( u32 value )
    {
        type = ParameterType::PARAM_TYPE_U32;
        data.iData = static_cast<s32>( value );
    }

    void Parameter::setF32( f32 value )
    {
        type = ParameterType::PARAM_TYPE_F32;
        data.fData = value;
    }

    void Parameter::setS64( s64 value )
    {
        type = ParameterType::PARAM_TYPE_S64;
        data.i64Data = value;
    }

    void Parameter::setF64( f64 value )
    {
        type = ParameterType::PARAM_TYPE_F64;
        data.f64Data = value;
    }

    void Parameter::setPtr( void *value )
    {
        type = ParameterType::PARAM_TYPE_PTR;
        data.pData = value;
    }

    bool Parameter::getBool() const
    {
        //WP_ASSERT(type == PARAM_TYPE_BOOL );
        return data.bData == 1;
    }

    c8 *Parameter::getCharPtr() const
    {
        return static_cast<c8 *>( data.pData );
    }

    u8 Parameter::getU8() const
    {
        return static_cast<u8>( data.iData );
    }

    u16 Parameter::getU16() const
    {
        return static_cast<u16>( data.iData );
    }

    s32 Parameter::getS32() const
    {
        return data.iData;
    }

    u32 Parameter::getU32() const
    {
        return static_cast<u32>( data.iData );
    }

    f32 Parameter::getF32() const
    {
        return data.fData;
    }

    s64 Parameter::getS64() const
    {
        return data.i64Data;
    }

    f64 Parameter::getF64() const
    {
        return data.f64Data;
    }

    void *Parameter::getPtr() const
    {
        //WP_ASSERT(type == PARAM_TYPE_PTR);
        return data.pData;
    }

    void Parameter::setObject( SmartPtr<ISharedObject> object )
    {
        type = ParameterType::PARAM_TYPE_OBJECT;
        data.pData = nullptr;
        this->object = object;
    }

    void Parameter::setArray( const Array<Parameter> &data )
    {
        type = ParameterType::PARAM_TYPE_ARRAY;
        this->data.pData = nullptr;
        array = data;
    }

    void Parameter::setStr( const String &data )
    {
        type = ParameterType::PARAM_TYPE_STR;
        this->data.pData = nullptr;
        str = data;
    }

    auto Parameter::getVector2() const -> Vector2F
    {
        WP_ASSERT( array.size() >= 2 );
        return { array[0].data.fData, array[1].data.fData };
    }

    void Parameter::setVector2( const Vector2F &data )
    {
        type = ParameterType::PARAM_TYPE_VEC2F;
        this->data.pData = nullptr;
        array.resize( 2 );
        array[0].setF32( data.x );
        array[1].setF32( data.y );
    }

    auto Parameter::getVector3() const -> Vector3<real_Num>
    {
        WP_ASSERT( array.size() >= 3 );
        return { array[0].data.fData, array[1].data.fData, array[2].data.fData };
    }

    void Parameter::setVector3( const Vector3<real_Num> &data )
    {
        type = ParameterType::PARAM_TYPE_VEC3F;
        this->data.pData = nullptr;
        array.resize( 3 );
        array[0].setF32( data.x );
        array[1].setF32( data.y );
        array[2].setF32( data.z );
    }

    auto Parameter::getQuaternion() const -> Quaternion<real_Num>
    {
        WP_ASSERT( array.size() >= 4 );
        return { array[0].data.fData, array[1].data.fData, array[2].data.fData, array[3].data.fData };
    }

    void Parameter::setQuaternion( const Quaternion<real_Num> &data )
    {
        type = ParameterType::PARAM_TYPE_QUATF;
        this->data.pData = nullptr;
        array.resize( 4 );

        array[0].setF32( data[0] );
        array[1].setF32( data[1] );
        array[2].setF32( data[2] );
        array[3].setF32( data[3] );
    }

    auto Parameter::getObject() const -> SmartPtr<ISharedObject>
    {
        return object;
    }

    auto Parameter::getArray() const -> const Array<Parameter> &
    {
        return array;
    }

    auto Parameter::getStr() const -> String
    {
        return str;
    }

    const Parameter Parameter::VOID_PARAM = Parameter();

    Parameter::Parameter( void *data )
    {
        setPtr( data );
    }

    Parameter::Parameter( f64 data )
    {
        setF64( data );
    }

    Parameter::Parameter( s64 data )
    {
        setS64( data );
    }

    Parameter::Parameter( f32 data )
    {
        setF32( data );
    }

    Parameter::Parameter( u32 data )
    {
        setU32( data );
    }

    Parameter::Parameter( s32 data )
    {
        setS32( data );
    }

    Parameter::Parameter( u16 data )
    {
        setU16( data );
    }

    Parameter::Parameter( u8 data )
    {
        setU8( data );
    }

    Parameter::Parameter( const c8 *data )
    {
        setCharPtr( const_cast<c8 *>( data ) );
    }

    Parameter::Parameter( bool data )
    {
        setBool( data );
    }

    Parameter::Parameter( const Array<Parameter> &data )
    {
        setArray( data );
    }

    Parameter::Parameter( SmartPtr<ISharedObject> data )
    {
        setObject( data );
    }

    Parameter::Parameter( const String &data )
    {
        setStr( data );
    }

    auto Parameter::operator==( const Parameter &other ) const -> bool
    {
        if( type != other.type )
        {
            return false;
        }

        switch( type )
        {
        case ParameterType::PARAM_TYPE_VOID:
        case ParameterType::PARAM_TYPE_NULL:
            return true;
        case ParameterType::PARAM_TYPE_BOOL:
            return getBool() == other.getBool();
        case ParameterType::PARAM_TYPE_U8:
        case ParameterType::PARAM_TYPE_U16:
        case ParameterType::PARAM_TYPE_U32:
        case ParameterType::PARAM_TYPE_S8:
        case ParameterType::PARAM_TYPE_S16:
        case ParameterType::PARAM_TYPE_S32:
        case ParameterType::PARAM_TYPE_BUTTON:
        case ParameterType::PARAM_TYPE_ENUM:
            return data.iData == other.data.iData;
        case ParameterType::PARAM_TYPE_S64:
            return data.i64Data == other.data.i64Data;
        case ParameterType::PARAM_TYPE_F32:
            return data.fData == other.data.fData;
        case ParameterType::PARAM_TYPE_F64:
            return data.f64Data == other.data.f64Data;
        case ParameterType::PARAM_TYPE_CHAR_PTR:
        case ParameterType::PARAM_TYPE_PTR:
            return data.pData == other.data.pData;
        case ParameterType::PARAM_TYPE_OBJECT:
            return object == other.object;
        case ParameterType::PARAM_TYPE_COMPONENT:
        case ParameterType::PARAM_TYPE_TEXTURE:
        case ParameterType::PARAM_TYPE_RESOURCE:
            if( object || other.object )
            {
                return object == other.object;
            }

            if( !str.empty() || !other.str.empty() )
            {
                return str == other.str;
            }

            if( !array.empty() || !other.array.empty() )
            {
                return array == other.array;
            }

            return data.pData == other.data.pData;
        case ParameterType::PARAM_TYPE_STR:
            return str == other.str;
        case ParameterType::PARAM_TYPE_ARRAY:
        case ParameterType::PARAM_TYPE_VEC2I:
        case ParameterType::PARAM_TYPE_VEC2F:
        case ParameterType::PARAM_TYPE_VEC2D:
        case ParameterType::PARAM_TYPE_VEC3I:
        case ParameterType::PARAM_TYPE_VEC3F:
        case ParameterType::PARAM_TYPE_VEC3D:
        case ParameterType::PARAM_TYPE_QUATF:
        case ParameterType::PARAM_TYPE_QUATD:
        case ParameterType::PARAM_TYPE_COLOUR:
        case ParameterType::PARAM_TYPE_AABB3:
        case ParameterType::PARAM_TYPE_AABB3D:
        case ParameterType::PARAM_TYPE_TRANSFORM3:
        case ParameterType::PARAM_TYPE_TRANSFORM3D:
            return array == other.array;
        case ParameterType::PARAM_TYPE_COLOURI:
            if( !array.empty() || !other.array.empty() )
            {
                return array == other.array;
            }

            return data.iData == other.data.iData;
        case ParameterType::PARAM_TYPE_COUNT:
            break;
        }

        return data.iData == other.data.iData;
    }
}  // namespace workphone
