#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Property.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/Util.hpp>

namespace workphone
{

    const String Property::defaultType = String( "string" );

    Property::Property()
    {
        clearData();

        constexpr u32 size = sizeof( Property );
        constexpr u32 align = alignof( Property );
        constexpr u32 paddedSize = ( size + align - 1 ) & ~( align - 1 );

        constexpr u32 dataSize = sizeof( Data );
    }

    Property::Property( const Property &other )
    {
        *this = other;
    }

    Property::Property( const String &name, const String &value )
    {
        clearData();
        m_type = Util::getParameterTypeFromString( defaultType );
        m_name = name;
        setValue( value );
    }

    Property::Property( const String &name, const String &value, const String &type )
    {
        m_attributes.reserve( 4 );

        clearData();
        m_type = Util::getParameterTypeFromString( type );
        m_name = name;
        setValue( value );
    }

    Property::Property( const String &name, const String &value, const String &label,
                        const String &type )
    {
        clearData();
        m_type = Util::getParameterTypeFromString( type );
        m_name = name;
        setValue( value );
    }

    Property::Property( const String &name, const String &value, const String &label, const String &type,
                        bool readyOnly ) :
        m_readOnly( readyOnly )
    {
        clearData();
        m_type = Util::getParameterTypeFromString( type );
        m_name = name;
        setValue( value );
    }

    Property::~Property()
    {
    }

    void Property::setName( const String &name )
    {
        m_name = name;
    }

    String Property::getName() const
    {
        return m_name;
    }

    String Property::getValue() const
    {
        return m_value;
    }

    void Property::setValue( const String &value )
    {
        m_value = value;
        setDataFromValue( m_value );
    }

    void Property::setTypeName( const String &type )
    {
        m_type = Util::getParameterTypeFromString( type );
        setDataFromValue( m_value );
    }

    String Property::getTypeName() const
    {
        return Util::getStringFromParameterType( m_type );
    }

    void Property::setType( ParameterType type )
    {
        m_type = type;
        setDataFromValue( m_value );
    }

    ParameterType Property::getType() const
    {
        return m_type;
    }

    void Property::setReadOnly( bool readOnly )
    {
        m_readOnly = readOnly == 1;
    }

    auto Property::isReadOnly() const -> bool
    {
        return m_readOnly == 1;
    }

    void Property::setAttribute( const String &name, const String &value )
    {
        for( auto &attrib : m_attributes )
        {
            if( StringUtil::isEqual( name, attrib.first ) )
            {
                attrib.second = value;
                return;
            }
        }

        m_attributes.emplace_back( name, value );
    }

    auto Property::getAttribute( const String &name ) const -> String
    {
        for( auto &attribute : m_attributes )
        {
            if( StringUtil::isEqual( name, attribute.first ) )
            {
                return attribute.second;
            }
        }

        return {};
    }

    auto Property::getAttributes() const -> Array<Pair<String, String>>
    {
        Array<Pair<String, String>> attributes;
        attributes.reserve( m_attributes.size() );

        for( const auto &attrib : m_attributes )
        {
            attributes.emplace_back( String( attrib.first.c_str(), attrib.first.length() ),
                                     String( attrib.second.c_str(), attrib.second.length() ) );
        }

        return attributes;
    }

    auto Property::getAttributeByIndex( u32 index ) -> Pair<String, String>
    {
        auto it = m_attributes.begin();
        std::advance( it, index );
        return { it->first, it->second };
    }

    auto Property::getNumAttributes() const -> u32
    {
        return static_cast<u32>( m_attributes.size() );
    }

    auto Property::operator=( const Property &other ) -> Property &
    {
        m_name = other.m_name;
        m_value = other.m_value;

        m_data = other.m_data;

        m_type = other.m_type;
        m_readOnly = other.m_readOnly;

        m_attributes = other.m_attributes;

        return *this;
    }

    auto Property::getValueAsBool() const -> bool
    {
        if( m_type == ParameterType::PARAM_TYPE_BOOL || m_type == ParameterType::PARAM_TYPE_BUTTON )
        {
            return m_data.bData;
        }

        return StringUtil::parseBool( m_value );
    }

    bool Property::getValueAsButton() const
    {
        if( m_type == ParameterType::PARAM_TYPE_BUTTON )
        {
            return m_data.bData;
        }

        return StringUtil::parseBool( m_value );
    }

    s32 Property::getValueAsEnum() const
    {
        if( m_type == ParameterType::PARAM_TYPE_U8 || m_type == ParameterType::PARAM_TYPE_U16 ||
            m_type == ParameterType::PARAM_TYPE_U32 )
        {
            return static_cast<s32>( m_data.uData );
        }

        if( m_type == ParameterType::PARAM_TYPE_S8 || m_type == ParameterType::PARAM_TYPE_S16 ||
            m_type == ParameterType::PARAM_TYPE_S32 || m_type == ParameterType::PARAM_TYPE_S64 ||
            m_type == ParameterType::PARAM_TYPE_ENUM )
        {
            return m_data.iData;
        }

        return StringUtil::parseInt( m_value );
    }

    void Property::setValueAsBool( bool value )
    {
        m_type = ParameterType::PARAM_TYPE_BOOL;
        m_data.bData = value;
        m_value = StringUtil::toString( value );
    }

    void Property::setValueAsButton( bool value )
    {
        m_type = ParameterType::PARAM_TYPE_BUTTON;
        m_data.bData = value;
        m_value = StringUtil::toString( value );
    }

    auto Property::getValueAsInt() const -> s32
    {
        if( m_type == ParameterType::PARAM_TYPE_U8 || m_type == ParameterType::PARAM_TYPE_U16 ||
            m_type == ParameterType::PARAM_TYPE_U32 )
        {
            return static_cast<s32>( m_data.uData );
        }

        if( m_type == ParameterType::PARAM_TYPE_S8 || m_type == ParameterType::PARAM_TYPE_S16 ||
            m_type == ParameterType::PARAM_TYPE_S32 || m_type == ParameterType::PARAM_TYPE_S64 ||
            m_type == ParameterType::PARAM_TYPE_ENUM )
        {
            return m_data.iData;
        }

        return StringUtil::parseInt( m_value );
    }

    void Property::setValueAsEnum( s32 value )
    {
        m_type = ParameterType::PARAM_TYPE_ENUM;
        m_data.iData = value;
        m_value = StringUtil::toString( value );
    }

    void Property::setValueAsInt( s32 value )
    {
        m_type = ParameterType::PARAM_TYPE_S32;
        m_data.iData = value;
        m_value = StringUtil::toString( value );
    }

    u32 Property::getValueAsUInt() const
    {
        if( m_type == ParameterType::PARAM_TYPE_U8 || m_type == ParameterType::PARAM_TYPE_U16 ||
            m_type == ParameterType::PARAM_TYPE_U32 )
        {
            return m_data.uData;
        }

        return StringUtil::parseUInt( m_value );
    }

    void Property::setValueAsUInt( u32 value )
    {
        m_type = ParameterType::PARAM_TYPE_U32;
        m_data.uData = value;
        m_value = StringUtil::toString( value );
    }

    auto Property::getValueAsFloat() const -> f32
    {
        if( m_type == ParameterType::PARAM_TYPE_F32 || m_type == ParameterType::PARAM_TYPE_F64 )
        {
            return m_data.fData;
        }

        return StringUtil::parseFloat( m_value );
    }

    void Property::setValueAsFloat( f32 value )
    {
        m_type = ParameterType::PARAM_TYPE_F32;
        m_data.fData = value;
        m_value = StringUtil::toString( value );
    }

    auto Property::getValueAsVector3f() const -> Vector3F
    {
        return StringUtil::parseVector3<f32>( m_value );
    }

    auto Property::getValueAsVector4f() const -> Vector4F
    {
        return StringUtil::parseVector4<f32>( m_value );
    }

    void Property::clearData()
    {
        m_data.uData = 0;
    }

    void Property::setDataFromValue( const String &value )
    {
        clearData();

        if( StringUtil::isNullOrEmpty( value ) )
        {
            return;
        }

        switch( m_type )
        {
        case ParameterType::PARAM_TYPE_BOOL:
        case ParameterType::PARAM_TYPE_BUTTON:
            m_data.bData = StringUtil::parseBool( value, false );
            break;
        case ParameterType::PARAM_TYPE_U8:
        case ParameterType::PARAM_TYPE_U16:
        case ParameterType::PARAM_TYPE_U32:
            m_data.uData = StringUtil::parseUInt( value );
            break;
        case ParameterType::PARAM_TYPE_S8:
        case ParameterType::PARAM_TYPE_S16:
        case ParameterType::PARAM_TYPE_S32:
        case ParameterType::PARAM_TYPE_S64:
        case ParameterType::PARAM_TYPE_ENUM:
            m_data.iData = StringUtil::parseInt( value );
            break;
        case ParameterType::PARAM_TYPE_F32:
        case ParameterType::PARAM_TYPE_F64:
            m_data.fData = StringUtil::parseFloat( value );
            break;
        default:
            break;
        }
    }

}  // namespace workphone
