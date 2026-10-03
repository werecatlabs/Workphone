#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/ResourceReference.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

#include <algorithm>

namespace workphone
{
    namespace
    {
        const String OwnerUUIDProperty = "ownerUUID";
        const String ResourceUUIDProperty = "resourceUUID";
        const String OwnerPathProperty = "ownerPath";
        const String ResourcePathProperty = "resourcePath";
        const String ResourceTypeProperty = "resourceType";
        const String PropertyNameProperty = "propertyName";
        const String DependencyModeProperty = "dependencyMode";
        const String RequiredProperty = "required";
        const String PriorityProperty = "priority";
        const String PlatformsProperty = "platforms";
        const String TagsProperty = "tags";
        const String PackageAllPlatform = "all";
        const String PackageAnyPlatform = "*";
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, ResourceReference, IResourceReference );

    ResourceReference::ResourceReference() = default;

    ResourceReference::ResourceReference( const String &ownerUUID, const String &resourceUUID )
    {
        setReference( ownerUUID, resourceUUID );
    }

    ResourceReference::~ResourceReference() = default;

    void ResourceReference::setOwnerUUID( const String &ownerUUID )
    {
        m_ownerUUID = normaliseToken( ownerUUID );
    }

    auto ResourceReference::getResourceUUID() const -> String
    {
        return m_resourceUUID;
    }

    void ResourceReference::setResourceUUID( const String &resourceUUID )
    {
        m_resourceUUID = normaliseToken( resourceUUID );
    }

    auto ResourceReference::getOwnerUUID() const -> String
    {
        return m_ownerUUID;
    }

    void ResourceReference::setReference( const String &ownerUUID, const String &resourceUUID )
    {
        setOwnerUUID( ownerUUID );
        setResourceUUID( resourceUUID );
    }

    String ResourceReference::getOwnerPath() const
    {
        return m_ownerPath;
    }

    void ResourceReference::setOwnerPath( const String &ownerPath )
    {
        m_ownerPath = normalisePath( ownerPath );
    }

    String ResourceReference::getResourcePath() const
    {
        return m_resourcePath;
    }

    void ResourceReference::setResourcePath( const String &resourcePath )
    {
        m_resourcePath = normalisePath( resourcePath );
    }

    String ResourceReference::getResourceType() const
    {
        return m_resourceType;
    }

    void ResourceReference::setResourceType( const String &resourceType )
    {
        m_resourceType = normaliseToken( resourceType );
    }

    String ResourceReference::getPropertyName() const
    {
        return m_propertyName;
    }

    void ResourceReference::setPropertyName( const String &propertyName )
    {
        m_propertyName = normaliseToken( propertyName );
    }

    ResourceReference::DependencyMode ResourceReference::getDependencyMode() const
    {
        return m_dependencyMode;
    }

    void ResourceReference::setDependencyMode( DependencyMode mode )
    {
        m_dependencyMode = mode;
    }

    String ResourceReference::getDependencyModeAsString() const
    {
        return dependencyModeToString( m_dependencyMode );
    }

    void ResourceReference::setDependencyModeFromString( const String &mode )
    {
        m_dependencyMode = dependencyModeFromString( mode );
    }

    bool ResourceReference::isRequired() const
    {
        return m_dependencyMode == DependencyMode::Required;
    }

    void ResourceReference::setRequired( bool required )
    {
        m_dependencyMode = required ? DependencyMode::Required : DependencyMode::Optional;
    }

    bool ResourceReference::isOptional() const
    {
        return m_dependencyMode == DependencyMode::Optional;
    }

    bool ResourceReference::isSoft() const
    {
        return m_dependencyMode == DependencyMode::Soft;
    }

    bool ResourceReference::isExcluded() const
    {
        return m_dependencyMode == DependencyMode::Excluded;
    }

    s32 ResourceReference::getPriority() const
    {
        return m_priority;
    }

    void ResourceReference::setPriority( s32 priority )
    {
        m_priority = priority;
    }

    Array<String> ResourceReference::getPlatforms() const
    {
        return m_platforms;
    }

    void ResourceReference::setPlatforms( const Array<String> &platforms )
    {
        assignUniqueTokens( m_platforms, platforms, true );
    }

    void ResourceReference::addPlatform( const String &platform )
    {
        Array<String> values;
        values.push_back( platform );
        auto merged = m_platforms;
        merged.insert( merged.end(), values.begin(), values.end() );
        assignUniqueTokens( m_platforms, merged, true );
    }

    void ResourceReference::removePlatform( const String &platform )
    {
        removeToken( m_platforms, platform, true );
    }

    void ResourceReference::clearPlatforms()
    {
        m_platforms.clear();
    }

    bool ResourceReference::supportsPlatform( const String &platform ) const
    {
        if( m_platforms.empty() )
        {
            return true;
        }

        const auto normalisedPlatform = StringUtil::make_lower( normaliseToken( platform ) );
        if( StringUtil::isNullOrEmpty( normalisedPlatform ) )
        {
            return containsToken( m_platforms, PackageAllPlatform, true ) ||
                   containsToken( m_platforms, PackageAnyPlatform, true );
        }

        return containsToken( m_platforms, PackageAllPlatform, true ) ||
               containsToken( m_platforms, PackageAnyPlatform, true ) ||
               containsToken( m_platforms, normalisedPlatform, true );
    }

    Array<String> ResourceReference::getTags() const
    {
        return m_tags;
    }

    void ResourceReference::setTags( const Array<String> &tags )
    {
        assignUniqueTokens( m_tags, tags, false );
    }

    void ResourceReference::addTag( const String &tag )
    {
        Array<String> values;
        values.push_back( tag );
        auto merged = m_tags;
        merged.insert( merged.end(), values.begin(), values.end() );
        assignUniqueTokens( m_tags, merged, false );
    }

    void ResourceReference::removeTag( const String &tag )
    {
        removeToken( m_tags, tag, false );
    }

    bool ResourceReference::hasTag( const String &tag ) const
    {
        return containsToken( m_tags, tag, false );
    }

    void ResourceReference::clearTags()
    {
        m_tags.clear();
    }

    bool ResourceReference::isValid() const
    {
        return !StringUtil::isNullOrEmpty( m_ownerUUID ) &&
               !StringUtil::isNullOrEmpty( m_resourceUUID ) && !isSelfReference();
    }

    bool ResourceReference::isSelfReference() const
    {
        return !StringUtil::isNullOrEmpty( m_ownerUUID ) && m_ownerUUID == m_resourceUUID;
    }

    bool ResourceReference::matches( const String &ownerUUID, const String &resourceUUID ) const
    {
        return m_ownerUUID == normaliseToken( ownerUUID ) &&
               m_resourceUUID == normaliseToken( resourceUUID );
    }

    String ResourceReference::getKey() const
    {
        return m_ownerUUID + "->" + m_resourceUUID + ":" + getDependencyModeAsString();
    }

    String ResourceReference::getDiagnosticName() const
    {
        auto owner = StringUtil::isNullOrEmpty( m_ownerPath ) ? m_ownerUUID : m_ownerPath;
        auto resource = StringUtil::isNullOrEmpty( m_resourcePath ) ? m_resourceUUID : m_resourcePath;
        auto property = StringUtil::isNullOrEmpty( m_propertyName )
                            ? String( "" )
                            : String( " [" ) + m_propertyName + "]";

        return owner + " -> " + resource + property;
    }

    SmartPtr<Properties> ResourceReference::toProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( OwnerUUIDProperty, m_ownerUUID );
        properties->setProperty( ResourceUUIDProperty, m_resourceUUID );
        properties->setProperty( OwnerPathProperty, m_ownerPath );
        properties->setProperty( ResourcePathProperty, m_resourcePath );
        properties->setProperty( ResourceTypeProperty, m_resourceType );
        properties->setProperty( PropertyNameProperty, m_propertyName );
        properties->setProperty( DependencyModeProperty, getDependencyModeAsString() );
        properties->setProperty( RequiredProperty, isRequired() );
        properties->setProperty( PriorityProperty, m_priority );
        properties->setProperty( PlatformsProperty, StringUtil::toString( m_platforms ) );
        properties->setProperty( TagsProperty, StringUtil::toString( m_tags ) );
        return properties;
    }

    void ResourceReference::fromProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            clear();
            return;
        }

        String value;
        if( properties->getPropertyValue( OwnerUUIDProperty, value ) )
        {
            setOwnerUUID( value );
        }

        if( properties->getPropertyValue( ResourceUUIDProperty, value ) )
        {
            setResourceUUID( value );
        }

        if( properties->getPropertyValue( OwnerPathProperty, value ) )
        {
            setOwnerPath( value );
        }

        if( properties->getPropertyValue( ResourcePathProperty, value ) )
        {
            setResourcePath( value );
        }

        if( properties->getPropertyValue( ResourceTypeProperty, value ) )
        {
            setResourceType( value );
        }

        if( properties->getPropertyValue( PropertyNameProperty, value ) )
        {
            setPropertyName( value );
        }

        if( properties->getPropertyValue( DependencyModeProperty, value ) )
        {
            setDependencyModeFromString( value );
        }
        else
        {
            bool required = true;
            if( properties->getPropertyValue( RequiredProperty, required ) )
            {
                setRequired( required );
            }
        }

        s32 priority = 0;
        if( properties->getPropertyValue( PriorityProperty, priority ) )
        {
            setPriority( priority );
        }

        if( properties->getPropertyValue( PlatformsProperty, value ) )
        {
            Array<String> platforms;
            StringUtil::parseArray( value, platforms );
            setPlatforms( platforms );
        }

        if( properties->getPropertyValue( TagsProperty, value ) )
        {
            Array<String> tags;
            StringUtil::parseArray( value, tags );
            setTags( tags );
        }
    }

    void ResourceReference::clear()
    {
        m_ownerUUID.clear();
        m_resourceUUID.clear();
        m_ownerPath.clear();
        m_resourcePath.clear();
        m_resourceType.clear();
        m_propertyName.clear();
        m_dependencyMode = DependencyMode::Required;
        m_priority = 0;
        m_platforms.clear();
        m_tags.clear();
    }

    String ResourceReference::normaliseToken( const String &value )
    {
        return StringUtil::trim( value );
    }

    String ResourceReference::normalisePath( const String &value )
    {
        auto path = StringUtil::trim( value );
        if( StringUtil::isNullOrEmpty( path ) )
        {
            return String();
        }

        return StringUtil::cleanupPath( path );
    }

    void ResourceReference::assignUniqueTokens( Array<String> &target, const Array<String> &values,
                                                bool lowerCase )
    {
        target.clear();

        for( auto value : values )
        {
            value = normaliseToken( value );
            if( lowerCase )
            {
                value = StringUtil::make_lower( value );
            }

            if( StringUtil::isNullOrEmpty( value ) || containsToken( target, value, false ) )
            {
                continue;
            }

            target.push_back( value );
        }
    }

    bool ResourceReference::containsToken( const Array<String> &tokens, const String &value,
                                           bool lowerCase )
    {
        auto token = normaliseToken( value );
        if( lowerCase )
        {
            token = StringUtil::make_lower( token );
        }

        for( auto existing : tokens )
        {
            existing = normaliseToken( existing );
            if( lowerCase )
            {
                existing = StringUtil::make_lower( existing );
            }

            if( existing == token )
            {
                return true;
            }
        }

        return false;
    }

    void ResourceReference::removeToken( Array<String> &tokens, const String &value, bool lowerCase )
    {
        auto token = normaliseToken( value );
        if( lowerCase )
        {
            token = StringUtil::make_lower( token );
        }

        tokens.erase( std::remove_if( tokens.begin(), tokens.end(),
                                      [&]( const String &existing ) {
                                          auto compare = normaliseToken( existing );
                                          if( lowerCase )
                                          {
                                              compare = StringUtil::make_lower( compare );
                                          }

                                          return compare == token;
                                      } ),
                      tokens.end() );
    }

    String ResourceReference::dependencyModeToString( DependencyMode mode )
    {
        switch( mode )
        {
        case DependencyMode::Required:
            return "required";
        case DependencyMode::Optional:
            return "optional";
        case DependencyMode::Soft:
            return "soft";
        case DependencyMode::Excluded:
            return "excluded";
        default:
            return "required";
        }
    }

    ResourceReference::DependencyMode ResourceReference::dependencyModeFromString( const String &mode )
    {
        auto value = StringUtil::make_lower( normaliseToken( mode ) );
        if( value == "optional" )
        {
            return DependencyMode::Optional;
        }

        if( value == "soft" || value == "weak" )
        {
            return DependencyMode::Soft;
        }

        if( value == "excluded" || value == "exclude" || value == "ignore" )
        {
            return DependencyMode::Excluded;
        }

        return DependencyMode::Required;
    }

}  // namespace workphone
