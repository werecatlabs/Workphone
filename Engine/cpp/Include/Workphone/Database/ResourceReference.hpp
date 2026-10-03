#ifndef ResourceReference_h__
#define ResourceReference_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Database/IResourceReference.hpp>

namespace workphone
{
    class Properties;

    /**
     * @brief Describes a package-time dependency from one resource to another.
     *
     * ResourceReference keeps the lightweight UUID pair required by IResourceReference, then
     * layers on the metadata a build/package pipeline needs to make deterministic platform
     * bundles: paths, dependency mode, platform filters, tags and stable keys.
     */
    class ResourceReference : public IResourceReference
    {
    public:
        enum class DependencyMode
        {
            Required,
            Optional,
            Soft,
            Excluded
        };

        ResourceReference();
        ResourceReference( const String &ownerUUID, const String &resourceUUID );
        ~ResourceReference() override;

        String getOwnerUUID() const override;

        void setOwnerUUID( const String &ownerUUID ) override;

        String getResourceUUID() const override;

        void setResourceUUID( const String &resourceUUID ) override;

        void setReference( const String &ownerUUID, const String &resourceUUID );

        String getOwnerPath() const;
        void setOwnerPath( const String &ownerPath );

        String getResourcePath() const;
        void setResourcePath( const String &resourcePath );

        String getResourceType() const;
        void setResourceType( const String &resourceType );

        String getPropertyName() const;
        void setPropertyName( const String &propertyName );

        DependencyMode getDependencyMode() const;
        void setDependencyMode( DependencyMode mode );

        String getDependencyModeAsString() const;
        void setDependencyModeFromString( const String &mode );

        bool isRequired() const;
        void setRequired( bool required );

        bool isOptional() const;
        bool isSoft() const;
        bool isExcluded() const;

        s32 getPriority() const;
        void setPriority( s32 priority );

        Array<String> getPlatforms() const;
        void setPlatforms( const Array<String> &platforms );
        void addPlatform( const String &platform );
        void removePlatform( const String &platform );
        void clearPlatforms();
        bool supportsPlatform( const String &platform ) const;

        Array<String> getTags() const;
        void setTags( const Array<String> &tags );
        void addTag( const String &tag );
        void removeTag( const String &tag );
        bool hasTag( const String &tag ) const;
        void clearTags();

        bool isValid() const;
        bool isSelfReference() const;
        bool matches( const String &ownerUUID, const String &resourceUUID ) const;
        String getKey() const;
        String getDiagnosticName() const;

        SmartPtr<Properties> toProperties() const;
        void fromProperties( SmartPtr<Properties> properties );
        void clear();

        WP_CLASS_REGISTER_DECL;

    private:
        static String normaliseToken( const String &value );
        static String normalisePath( const String &value );
        static void assignUniqueTokens( Array<String> &target, const Array<String> &values,
                                        bool lowerCase );
        static bool containsToken( const Array<String> &tokens, const String &value, bool lowerCase );
        static void removeToken( Array<String> &tokens, const String &value, bool lowerCase );
        static String dependencyModeToString( DependencyMode mode );
        static DependencyMode dependencyModeFromString( const String &mode );

        String m_ownerUUID;
        String m_resourceUUID;
        String m_ownerPath;
        String m_resourcePath;
        String m_resourceType;
        String m_propertyName;
        DependencyMode m_dependencyMode = DependencyMode::Required;
        s32 m_priority = 0;
        Array<String> m_platforms;
        Array<String> m_tags;
    };
}  // namespace workphone

#endif  // ResourceReference_h__
