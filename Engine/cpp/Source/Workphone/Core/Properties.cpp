#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/VectorUtil.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Util.hpp>
#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/System/DebugUtil.hpp>
#include <Workphone/System/Rttr.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, Properties, ISharedObject );

    namespace { thread_local Properties::ObjectResolver loadObjectResolver; }
    Properties::ObjectResolver Properties::exchangeObjectResolver( ObjectResolver resolver )
    {
        auto previous = std::move( loadObjectResolver );
        loadObjectResolver = std::move( resolver );
        return previous;
    }

    String Properties::getResourceUUID( const ISharedObject *object )
    {
        return object->getHandle()->getUUIDAsString();
    }

    bool Properties::getPropertyResources( const String &name,
                                          Array<SmartPtr<ISharedObject>> &value ) const
    {
        if( !hasProperty( name ) )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto resourceDatabase = applicationManager->getResourceDatabase();
        const auto &property = getPropertyObject( name );

        Array<String> uuids;
        StringUtil::parseArray( property.getValue(), uuids );
        for( const auto &uuid : uuids )
        {
            if( auto resource = resourceDatabase->getObject( StringUtil::parseUUID( uuid ) ) )
            {
                value.push_back( resource );
            }
        }
        return true;
    }

    void Properties::setPropertyAsTypeImpl( const String &name, ISharedObject *value, u32 type )
    {
        auto typeManager = TypeManager::instance();
        if( value )
        {
            auto typeName = typeManager->getName( type );
            setProperty( name, getResourceUUID( value ), resourceStr, false );
            getPropertyObject( name ).setAttribute( resourceTypeStr, typeName );
        }
        else
        {
            setProperty( name, "", resourceStr, false );
            auto typeName = typeManager->getName( type );
            getPropertyObject( name ).setAttribute( resourceTypeStr, typeName );
        }
    }

    bool Properties::getPropertyAsTypeImpl( const String &name, u32 type,
                                          SmartPtr<ISharedObject> &value, bool &assignValue ) const
    {
        assignValue = false;
        if( !hasProperty( name ) )
        {
            return false;
        }

        const auto sUUID = getPropertyObject( name ).getValue();
        if( StringUtil::isNullOrEmpty( sUUID ) )
        {
            return false;
        }

        const auto uuid = StringUtil::parseUUID( sUUID );
        auto applicationManager = core::IApplicationManager::instance();
        auto resourceDatabase = applicationManager->getResourceDatabase();
        auto resource = loadObjectResolver ? loadObjectResolver( sUUID ) : nullptr;
        if( !resource ) resource = resourceDatabase->getObject( uuid );
        if( resource )
        {
            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );
            const auto resourceType = resource->getTypeInfo();
            if( resourceType != 0 && typeManager->isDerived( resourceType, type ) )
            {
                value = resource;
                assignValue = true;
            }
            else if( resource->isDerived<scene::IGameActor>() )
            {
                auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( resource );
                value = nullptr;
                for( const auto &component : actor->getComponents() )
                {
                    const auto componentType = component->getTypeInfo();
                    if( componentType != 0 && typeManager->isDerived( componentType, type ) )
                    {
                        value = component;
                        break;
                    }
                }
                assignValue = true;
            }
        }
        return true;
    }

    static u32 NumPropertyGroups = 0;

    const String Properties::emptyStr = String( "" );
    const String Properties::resourceStr = String( "resource" );
    const String Properties::resourceTypeStr = String( "resourceType" );
    const String Properties::materialStr = String( "Material" );
    const String Properties::textureStr = String( "Texture" );
    const String Properties::componentStr = String( "Component" );
    const String Properties::noneStr = String( "None" );
    const String Properties::attributeNameStr = String( "click" );
    const String Properties::defaultValue = String( "true" );
    const String Properties::buttonTypeStr = String( "button" );
    const String Properties::enumTypeStr = String( "enum" );

    const String Properties::stringTypeStr = String( "string" );
    const String Properties::intTypeStr = String( "int" );
    const String Properties::doubleTypeStr = String( "double" );
    const String Properties::boolTypeStr = String( "bool" );

    const String Properties::colourStr = String( "colour" );
    const String Properties::colourfStr = String( "colourf" );

    const String Properties::aabbStr = String( "aabb" );
    const String Properties::aabbdStr = String( "aabbd" );

    const String Properties::stringArrayStr = String( "string_array" );
    const String Properties::u32Str = String( "u32" );
    const String Properties::unsignedLongLongStr = String( "unsigned long long" );
    const String Properties::unsignedLongStr = String( "unsigned long" );
    const String Properties::floatStr = String( "float" );
    const String Properties::doubleStr = String( "double" );
    const String Properties::vector2iStr = String( "Vector2i" );
    const String Properties::vector2Str = String( "Vector2" );
    const String Properties::vector2dStr = String( "Vector2d" );
    const String Properties::vector3iStr = String( "vector3i" );
    const String Properties::vector3Str = String( "vector3" );
    const String Properties::vector3dStr = String( "vector3d" );
    const String Properties::quaternionStr = String( "quaternion" );
    const String Properties::quaterniondStr = String( "quaterniond" );
    const String Properties::transformStr = String( "transform" );
    const String Properties::transformdStr = String( "transformd" );

    Properties::Properties() : ISharedObject( Properties::typeInfo() )
    {
        m_properties.reserve( 12 );
        m_children.reserve( 4 );
    }

    Properties::Properties( const Properties &other )
    {
        *this = other;

        m_properties.reserve( 12 );
        m_children.reserve( 4 );
    }

    Properties::~Properties()
    {
        m_properties.clear();
        m_children.clear();

#ifdef DEBUG_PROPGRP
        NumPropertyGroups--;
        WP_LOG_MESSAGE( "Properties", "Properties destroyed name: %s Num groups: %i", m_name.c_str(),
                        NumPropertyGroups );
#endif
    }

    void Properties::clearAll( bool cascade )
    {
        m_properties.clear();

        if( cascade )
        {
            for( auto &i : m_children )
            {
                i->clearAll( cascade );
            }
        }
    }

    void Properties::addProperty( const Property &property )
    {
        auto name = property.getName();
        if( hasProperty( name ) )
        {
            for( auto &currentProperty : m_properties )
            {
                if( currentProperty.getName() == name )
                {
                    currentProperty = property;
                }
            }
        }
        else
        {
            m_properties.push_back( property );
        }
    }

    auto Properties::hasProperty( const String &name ) const -> bool
    {
        if( !m_properties.empty() )
        {
            for( auto &currentProperty : m_properties )
            {
                if( currentProperty.getName() == name )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void Properties::addProperty( const String &name, const String &value, const String &type,
                                  bool readOnly )
    {
        auto iType = Util::getParameterTypeFromString( type );

        // check if the property exists
        if( !hasProperty( name ) )
        {
            Property property;
            property.setType( iType );
            property.setName( name );
            property.setValue( value );
            property.setReadOnly( readOnly );

            m_properties.push_back( property );
        }
        else
        {
            auto &property = getPropertyObject( name );
            property.setValue( value );
        }
    }

    void Properties::setProperty( const String &name, const String &value, const String &type )
    {
        auto iType = Util::getParameterTypeFromString( type );

        // check if the property exists
        if( !hasProperty( name ) )
        {
            Property property( name, value, type );
            m_properties.push_back( property );
        }
        else
        {
            auto &property = getPropertyObject( name );
            property.setValue( value );

            if( type.length() > 0 )
            {
                property.setType( iType );
            }
        }
    }

    void Properties::setProperty( const String &name, const String &value, const String &type,
                                  bool readOnly )
    {
        auto iType = Util::getParameterTypeFromString( type );

        // check if the property exists
        if( !hasProperty( name ) )
        {
            Property property;
            property.setType( iType );
            property.setName( name );
            property.setValue( value );
            property.setReadOnly( readOnly );

            m_properties.push_back( property );
        }
        else
        {
            auto &property = getPropertyObject( name );
            property.setValue( value );
        }
    }

    auto Properties::setPropertyType( const String &name, const String &type ) -> bool
    {
        if( !hasProperty( name ) )
        {
            return false;
        }

        auto iType = Util::getParameterTypeFromString( type );

        auto &property = getPropertyObject( name );
        property.setType( iType );
        return true;
    }

    auto Properties::removeProperty( const String &name ) -> bool
    {
        auto count = static_cast<size_t>( 0 );
        for( auto &property : m_properties )
        {
            if( property.getName() == name )
            {
                break;
            }

            ++count;
        }

        auto it = m_properties.begin();
        std::advance( it, count );
        if( it != m_properties.end() )
        {
            m_properties.erase( it );
            return true;
        }

        return false;
    }

    auto Properties::propertyValueEquals( const String &name, const String &value ) const -> bool
    {
        for( auto &property : m_properties )
        {
            if( property.getName() == name )
            {
                return property.getValue() == value;
            }
        }

        return false;
    }

    auto Properties::operator=( const Properties &other ) -> Properties &
    {
        if( this != &other )
        {
            m_properties = other.m_properties;
            m_children = other.m_children;
        }

        return *this;
    }

    u32 Properties::getNumProperties() const
    {
        return static_cast<u32>( m_properties.size() );
    }

    auto Properties::getChildren() const -> Array<SmartPtr<Properties>>
    {
        return m_children;
    }

    void Properties::addChild( const SmartPtr<Properties> &properties )
    {
        WP_ASSERT( properties != nullptr );
        WP_ASSERT( properties != this );

        // check if the child already exists
        WP_ASSERT( std::find( m_children.begin(), m_children.end(), properties ) == m_children.end() );

        m_children.push_back( properties );

        // Check if objects are unique
        WP_ASSERT( std::count( m_children.begin(), m_children.end(), properties ) == 1 );
    }

    void Properties::removeChild( const String &name )
    {
        for( u32 i = 0; i < m_children.size(); ++i )
        {
            auto &properties = m_children[i];
            if( properties->getName() == name )
            {
                auto it = m_children.begin();
                std::advance( it, i );
                m_children.erase( it );
                return;
            }
        }
    }

    auto Properties::getNumChildren() const -> u32
    {
        return static_cast<u32>( m_children.size() );
    }

    auto Properties::getChild( u32 index ) const -> SmartPtr<Properties>
    {
        WP_ASSERT( index < m_children.size() );
        return m_children[index];
    }

    auto Properties::getChild( const String &name ) const -> SmartPtr<Properties>
    {
        for( auto &child : m_children )
        {
            const auto childName = child->getNamePtr();
            if( name == childName )
            {
                return child;
            }
        }

        return nullptr;
    }

    auto Properties::getChildrenByName( const String &name ) const -> Array<SmartPtr<Properties>>
    {
        Array<SmartPtr<Properties>> children;
        children.reserve( m_children.size() );

        for( auto &child : m_children )
        {
            if( child )
            {
                const auto childName = child->getNamePtr();
                if( name == childName )
                {
                    children.push_back( child );
                }
            }
        }

        return children;
    }

    auto Properties::hasChild( const String &name ) const -> bool
    {
        for( const auto &child : m_children )
        {
            const auto childName = child->getNamePtr();
            if( name == childName )
            {
                return true;
            }
        }

        return false;
    }

    auto Properties::getProperty( const String &name, Property &property ) const -> bool
    {
        for( auto &currentProperty : m_properties )
        {
            if( currentProperty.getName() == name )
            {
                property = currentProperty;
                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyObject( const String &name ) -> Property &
    {
        for( auto &currentProperty : m_properties )
        {
            if( currentProperty.getName() == name )
            {
                return currentProperty;
            }
        }

        static Property emptyProperty;
        return emptyProperty;
    }

    auto Properties::getPropertyObject( const String &name ) const -> const Property &
    {
        for( auto &currentProperty : m_properties )
        {
            if( currentProperty.getName() == name )
            {
                return currentProperty;
            }
        }

        static Property emptyProperty;
        return emptyProperty;
    }

    void Properties::setPropertiesAsArray( const Array<Property> &array )
    {
        m_properties = array;
    }

    auto Properties::getPropertiesAsArray() const -> Array<Property>
    {
        return m_properties;
    }

    auto Properties::getProperty( const String &name, String defaultValue ) const -> String
    {
        String value;
        if( getPropertyValue( name, value ) )
        {
            return value;
        }

        return defaultValue;
    }

    auto Properties::getPropertyAsBool( const String &name, bool defaultValue /*= false*/ ) const -> bool
    {
        auto value = getProperty( name, emptyStr );
        return StringUtil::parseBool( value, defaultValue );
    }

    auto Properties::getPropertyAsInt( const String &name, s32 defaultValue /*= 0*/ ) const -> s32
    {
        auto value = getProperty( name, emptyStr );
        return StringUtil::parseInt( value, defaultValue );
    }

    auto Properties::getPropertyAsFloat( const String &name, f32 defaultValue /*= 0.0f*/ ) const -> f32
    {
        auto value = getProperty( name, emptyStr );
        return StringUtil::parseFloat( value, defaultValue );
    }

    auto Properties::getPropertyAsVector3( const String &name,
                                           Vector3F defaultValue /*= Vector3<real_Num>::ZERO*/ ) const
        -> Vector3F
    {
        auto value = getProperty( name, emptyStr );
        return StringUtil::parseVector3( value, defaultValue );
    }

    auto Properties::getPropertyAsVector3D( const String &name,
                                            Vector3D defaultValue /*= Vector3<real_Num>::ZERO*/ ) const
        -> Vector3D
    {
        auto value = getProperty( name, emptyStr );
        return StringUtil::parseVector3<f64>( value, defaultValue );
    }

    void Properties::setPropertyInChildren( const String &name, const String &value, bool cascade,
                                            bool checkingExisting )
    {
        auto children = getChildren();
        for( auto child : children )
        {
            if( checkingExisting )
            {
                if( child->hasProperty( name ) )
                {
                    child->setProperty( name, value );
                }
            }
            else
            {
                child->setProperty( name, value );
            }

            if( cascade )
            {
                child->setPropertyInChildren( name, value, cascade );
            }
        }
    }

    void Properties::setProperty( const String &name, const String &value, bool readOnly )
    {
        setProperty( name, value, stringTypeStr, readOnly );
    }

    void Properties::setProperty( const String &name, const Array<String> &value, bool readOnly )
    {
        auto arrayStr = StringUtil::toString( value );
        setProperty( name, arrayStr, stringArrayStr, readOnly );
    }

    void Properties::setProperty( const String &name, const char *const &value, bool readOnly )
    {
        String valueStr = value ? value : String();
        setProperty( name, valueStr, stringTypeStr, readOnly );
    }

    void Properties::setProperty( const String &name, const bool &value, bool readOnly )
    {
        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValueAsBool( value );
        }
        else
        {
            Property property;
            property.setName( name );
            property.setValueAsBool( value );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const s32 &value, bool readOnly )
    {
        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValueAsInt( value );
        }
        else
        {
            Property property;
            property.setName( name );
            property.setValueAsInt( value );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const u32 &value, bool readOnly )
    {
        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValueAsUInt( value );
        }
        else
        {
            Property property;
            property.setName( name );
            property.setValueAsUInt( value );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

#if defined WP_PLATFORM_WIN32

    void Properties::setProperty( const String &name, const unsigned long &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        setProperty( name, valueStr, unsignedLongLongStr, readOnly );
    }

    void Properties::setProperty( const String &name, const unsigned long long &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        setProperty( name, valueStr, unsignedLongLongStr, readOnly );
    }
#elif defined WP_PLATFORM_APPLE

    void Properties::setProperty( const String &name, const unsigned long &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( (u32)value );
        setProperty( name, valueStr, unsignedLongStr, readOnly );
    }

    void Properties::setProperty( const String &name, const unsigned long long &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( (u32)value );
        setProperty( name, valueStr, unsignedLongLongStr, readOnly );
    }
#elif defined WP_PLATFORM_LINUX

    void Properties::setProperty( const String &name, const u64 &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );
        setProperty( name, valueStr, unsignedLongLongStr, readOnly );
    }
#endif

    void Properties::setProperty( const String &name, const f32 &value, bool readOnly )
    {
        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValueAsFloat( value );
        }
        else
        {
            Property property;
            property.setName( name );
            property.setValueAsFloat( value );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const f64 &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_F64 );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setName( name );
            property.setType( ParameterType::PARAM_TYPE_F64 );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const Vector2I &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_VEC2I );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_VEC2I );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const Vector2F &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_VEC2F );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_VEC2F );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const Vector2D &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_VEC2D );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_VEC2D );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const Vector3I &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_VEC3I );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_VEC3I );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const Vector3F &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_VEC3F );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_VEC3F );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const Vector3D &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_VEC3D );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_VEC3D );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const QuaternionF &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_QUATF );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_QUATF );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const QuaternionD &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setType( ParameterType::PARAM_TYPE_QUATD );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_QUATD );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const Transform3F &value, bool readOnly )
    {
        auto p = value.getPosition();
        auto r = value.getRotation();
        auto s = value.getScale();

        auto valueStr = StringUtil::toString( p ) + ";" + StringUtil::toString( r ) + ";" +
                        StringUtil::toString( s );

        setProperty( name, valueStr, transformStr, readOnly );
    }

    void Properties::setProperty( const String &name, const Transform3D &value, bool readOnly )
    {
        auto p = value.getPosition();
        auto r = value.getRotation();
        auto s = value.getScale();

        auto valueStr = StringUtil::toString( p ) + ";" + StringUtil::toString( r ) + ";" +
                        StringUtil::toString( s );

        setProperty( name, valueStr, transformdStr, readOnly );
    }

    void Properties::setProperty( const String &name, const AABB3F &value, bool readOnly )
    {
        auto minValueStr = StringUtil::toString( value.getMinimum() );
        auto maxValueStr = StringUtil::toString( value.getMaximum() );
        auto valueStr = minValueStr + ";" + maxValueStr;

        setProperty( name, valueStr, aabbStr, readOnly );
    }

    void Properties::setProperty( const String &name, const AABB3D &value, bool readOnly )
    {
        auto minValueStr = StringUtil::toString( value.getMinimum() );
        auto maxValueStr = StringUtil::toString( value.getMaximum() );
        auto valueStr = minValueStr + ";" + maxValueStr;

        setProperty( name, valueStr, aabbdStr, readOnly );
    }

    void Properties::setProperty( const String &name, const ColourI &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_COLOURI );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setProperty( const String &name, const ColourF &value, bool readOnly )
    {
        auto valueStr = StringUtil::toString( value );

        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValue( valueStr );
        }
        else
        {
            Property property;
            property.setType( ParameterType::PARAM_TYPE_COLOUR );
            property.setName( name );
            property.setValue( valueStr );
            property.setReadOnly( readOnly );
            m_properties.push_back( property );
        }
    }

    void Properties::setPropertyAsEnum( const String &name, s32 value, const Array<String> &values,
                                        bool readOnly )
    {
        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValueAsEnum( value );
        }
        else
        {
            Property property;

            auto iType = Util::getParameterTypeFromString( enumTypeStr );
            property.setType( iType );

            property.setName( name );
            property.setValueAsEnum( value );
            property.setReadOnly( readOnly );

            auto valueStr = StringUtil::toString( values );
            property.setAttribute( enumTypeStr, valueStr );

            m_properties.push_back( property );
        }
    }

    void Properties::setPropertyAsEnum( const String &name, const String &value,
                                        const Array<String> &values, bool readOnly )
    {
        auto iValueStr = StringUtil::parseInt( value );
        setPropertyAsEnum( name, iValueStr, values, readOnly );
    }

    void Properties::setButtonPressed( const String &name, bool value )
    {
        if( hasProperty( name ) )
        {
            auto &property = getPropertyObject( name );
            property.setValueAsButton( value );
        }
        else
        {
            Property property;
            property.setName( name );
            property.setValueAsButton( value );
            property.setReadOnly( false );
            m_properties.push_back( property );
        }
    }

    auto Properties::isButtonPressed( const String &name ) const -> bool
    {
        if( hasProperty( name ) )
        {
            auto &buttonProperty = getPropertyObject( name );
            return buttonProperty.getValueAsButton();
        }

        return false;
    }

    void Properties::setProperty( const String &name, SmartPtr<ISharedObject> value, bool readOnly )
    {
        if( value )
        {
            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();

            setProperty( name, uuid, resourceStr, false );

            if( value->isDerived<IResource>() )
            {
                auto resource = workphone::static_pointer_cast<IResource>( value );
                auto resourceTypeName = ApplicationUtil::getObjectTypeName( value );

                auto &property = getPropertyObject( name );

                property.setAttribute( resourceTypeStr, resourceTypeName );
            }
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, noneStr );
        }
    }

    void Properties::setProperty( const String &name, SmartPtr<render::IMaterial> value, bool readOnly )
    {
        if( value )
        {
            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();
            setProperty( name, uuid, resourceStr, false );

            auto resourceTypeName = ApplicationUtil::getObjectTypeName( value );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, resourceTypeName );
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, materialStr );
        }
    }

    void Properties::setProperty( const String &name, SmartPtr<render::ITexture> value, bool readOnly )
    {
        if( value )
        {
            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();
            setProperty( name, uuid, resourceStr, false );

            auto resourceTypeName = ApplicationUtil::getObjectTypeName( value );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, resourceTypeName );
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, textureStr );
        }
    }

    void Properties::setProperty( const String &name, SmartPtr<scene::IGameActor> value, bool readOnly )
    {
        if( value )
        {
            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();
            setProperty( name, uuid, resourceStr, false );

            auto resourceTypeName = ApplicationUtil::getObjectTypeName( value );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, resourceTypeName );
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, componentStr );
        }
    }

    void Properties::setProperty( const String &name, SmartPtr<scene::IComponent> value, bool readOnly )
    {
        if( value )
        {
            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();
            setProperty( name, uuid, resourceStr, false );

            auto resourceTypeName = ApplicationUtil::getObjectTypeName( value );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, resourceTypeName );
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, componentStr );
        }
    }

    void Properties::setProperty( const String &name, SmartPtr<IMeshResource> value, bool readOnly )
    {
        if( value )
        {
            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();
            setProperty( name, uuid, resourceStr, false );

            auto resourceTypeName = ApplicationUtil::getObjectTypeName( value );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, resourceTypeName );
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, componentStr );
        }
    }

    void Properties::setProperty( const String &name, SmartPtr<ISound> value, bool readOnly )
    {
        if( value )
        {
            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();
            setProperty( name, uuid, resourceStr, false );

            auto resourceTypeName = ApplicationUtil::getObjectTypeName( value );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, resourceTypeName );
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, componentStr );
        }
    }

    void Properties::setProperty( const Property &property )
    {
        for( auto &currentProperty : m_properties )
        {
            if( currentProperty.getName() == property.getName() )
            {
                currentProperty = property;
                return;
            }
        }

        m_properties.push_back( property );
    }

    void Properties::setProperty( const String &name, Array<SmartPtr<scene::IComponent>> value,
                                  bool readOnly )
    {
        if( value.size() > 0 )
        {
            Array<String> uuids;
            for( auto &component : value )
            {
                auto handle = component->getHandle();
                auto uuid = handle->getUUIDAsString();
                uuids.push_back( uuid );
            }

            auto valueStr = StringUtil::toString( uuids );
            setProperty( name, valueStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, componentStr );
        }
        else
        {
            setProperty( name, emptyStr, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, componentStr );
        }
    }

    auto Properties::getPropertyValue( const String &name, String &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            value = property.getValue();
            return true;
        }

        for( auto &child : m_children )
        {
            if( child->getPropertyValue( name, value ) )
            {
                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, bool &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            value = property.getValueAsBool();
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, s32 &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            value = property.getValueAsInt();
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, u32 &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            value = static_cast<u32>( property.getValueAsInt() );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, f32 &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            value = property.getValueAsFloat();
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, f64 &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            value = StringUtil::parseDouble( property.getValue() );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Vector2I &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseVector2<s32>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Vector2F &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseVector2<f32>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Vector2D &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseVector2<f64>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Vector3I &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseVector3<s32>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Vector3F &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseVector3<f32>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Vector3D &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseVector3<f64>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, QuaternionF &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseQuaternion<f32>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, QuaternionD &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseQuaternion<f64>( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Transform3F &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            auto splitStr = StringUtil::split( str, ";", 3, false );

            if( splitStr.size() >= 3 )
            {
                auto p = StringUtil::parseVector3<f32>( splitStr[0] );
                auto r = StringUtil::parseVector3<f32>( splitStr[1] );
                auto s = StringUtil::parseVector3<f32>( splitStr[2] );

                value.setPosition( p );
                value.setRotation( r );
                value.setScale( s );
            }

            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Transform3D &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            auto splitStr = StringUtil::split( str, ";", 3, false );

            if( splitStr.size() >= 3 )
            {
                auto p = StringUtil::parseVector3<f64>( splitStr[0] );
                auto r = StringUtil::parseVector3<f64>( splitStr[1] );
                auto s = StringUtil::parseVector3<f64>( splitStr[2] );

                value.setPosition( p );
                value.setRotation( r );
                value.setScale( s );
            }

            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, ColourI &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseColour( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, ColourF &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            value = StringUtil::parseColourf( str );
            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, AABB3F &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            auto splitStr = StringUtil::split( str, ";", 2, false );
            if( splitStr.size() >= 2 )
            {
                auto min = StringUtil::parseVector3<f32>( splitStr[0] );
                auto max = StringUtil::parseVector3<f32>( splitStr[1] );
                value.setMinimum( min );
                value.setMaximum( max );
            }

            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, AABB3D &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            auto splitStr = StringUtil::split( str, ";", 2, false );
            if( splitStr.size() >= 2 )
            {
                auto min = StringUtil::parseVector3<f64>( splitStr[0] );
                auto max = StringUtil::parseVector3<f64>( splitStr[1] );
                value.setMinimum( min );
                value.setMaximum( max );
            }

            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, SmartPtr<ISharedObject> &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto sUUID = property.getValue();

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabasePtr();
            WP_ASSERT( resourceDatabase );

            auto uuid = StringUtil::parseUUID( sUUID );
            value = resourceDatabase->getObject( uuid );

            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, SmartPtr<render::IMaterial> &value ) const
        -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto sUUID = property.getValue();
            if( !StringUtil::isNullOrEmpty( sUUID ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto resourceDatabase = applicationManager->getResourceDatabasePtr();
                WP_ASSERT( resourceDatabase );

                auto uuid = StringUtil::parseUUID( sUUID );
                auto resource = resourceDatabase->getObject( uuid );
                if( resource )
                {
                    if( resource->isDerived<render::IMaterial>() )
                    {
                        value = workphone::static_pointer_cast<render::IMaterial>( resource );
                    }
                }

                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, SmartPtr<render::ITexture> &value ) const
        -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto sUUID = property.getValue();
            if( !StringUtil::isNullOrEmpty( sUUID ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto resourceDatabase = applicationManager->getResourceDatabasePtr();
                WP_ASSERT( resourceDatabase );

                auto uuid = StringUtil::parseUUID( sUUID );
                auto resource = resourceDatabase->getObject( uuid );
                if( resource )
                {
                    if( resource->isDerived<render::ITexture>() )
                    {
                        value = workphone::static_pointer_cast<render::ITexture>( resource );
                    }
                }

                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, SmartPtr<scene::IGameActor> &value ) const
        -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto sUUID = property.getValue();
            if( !StringUtil::isNullOrEmpty( sUUID ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto resourceDatabase = applicationManager->getResourceDatabasePtr();
                WP_ASSERT( resourceDatabase );

                auto uuid = StringUtil::parseUUID( sUUID );
                auto resource = resourceDatabase->getObject( uuid );
                if( resource )
                {
                    if( resource->isDerived<scene::IGameActor>() )
                    {
                        value = workphone::static_pointer_cast<scene::IGameActor>( resource );
                    }
                }

                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, SmartPtr<scene::IComponent> &value ) const
        -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto sUUID = property.getValue();
            if( !StringUtil::isNullOrEmpty( sUUID ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto resourceDatabase = applicationManager->getResourceDatabasePtr();
                WP_ASSERT( resourceDatabase );

                auto uuid = StringUtil::parseUUID( sUUID );
                auto resource = resourceDatabase->getObject( uuid );
                if( resource )
                {
                    if( resource->isDerived<scene::IComponent>() )
                    {
                        value = workphone::static_pointer_cast<scene::IComponent>( resource );
                    }
                }

                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, SmartPtr<IMeshResource> &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto sUUID = property.getValue();
            if( !StringUtil::isNullOrEmpty( sUUID ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto resourceDatabase = applicationManager->getResourceDatabasePtr();
                WP_ASSERT( resourceDatabase );

                auto uuid = StringUtil::parseUUID( sUUID );
                auto resource = resourceDatabase->getObject( uuid );
                if( resource )
                {
                    if( resource->isDerived<IMeshResource>() )
                    {
                        value = workphone::static_pointer_cast<IMeshResource>( resource );
                    }
                }

                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, SmartPtr<ISound> &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto sUUID = property.getValue();
            if( !StringUtil::isNullOrEmpty( sUUID ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto resourceDatabase = applicationManager->getResourceDatabasePtr();
                WP_ASSERT( resourceDatabase );

                auto uuid = StringUtil::parseUUID( sUUID );
                auto resource = resourceDatabase->getObject( uuid );
                if( resource )
                {
                    if( resource->isDerived<ISound>() )
                    {
                        value = workphone::static_pointer_cast<ISound>( resource );
                    }
                }

                return true;
            }
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name,
                                       Array<SmartPtr<scene::IComponent>> &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto valueStr = property.getValue();
            auto uuids = Array<String>();
            StringUtil::parseArray( valueStr, uuids );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabasePtr();
            WP_ASSERT( resourceDatabase );

            for( auto &sUUID : uuids )
            {
                auto uuid = StringUtil::parseUUID( sUUID );
                auto resource = resourceDatabase->getObject( uuid );
                if( resource )
                {
                    if( resource->isDerived<scene::IComponent>() )
                    {
                        value.push_back( workphone::static_pointer_cast<scene::IComponent>( resource ) );
                    }
                }
            }

            return true;
        }

        return false;
    }

    auto Properties::getPropertyValue( const String &name, Array<String> &value ) const -> bool
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            auto str = property.getValue();
            StringUtil::parseArray( str, value );
            return true;
        }

        return false;
    }

#if WP_ENABLE_TRACE
    s32 Properties::addReference()
    {
#    if !WP_FINAL
        auto debugStr = getDebugStr() + DebugUtil::getStackTrace() + "\n";
        setDebugStr( debugStr );
#    endif

        return ISharedObject::addReference();
    }

    bool Properties::removeReference()
    {
        return ISharedObject::removeReference();
    }
#endif

}  // namespace workphone
