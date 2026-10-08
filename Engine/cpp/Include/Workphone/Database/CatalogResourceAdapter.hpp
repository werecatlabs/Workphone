#ifndef WPCatalogResourceAdapter_h__
#define WPCatalogResourceAdapter_h__

#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Resource/IResourceSystem.hpp>

namespace workphone
{
    /**
     * Connects durable catalog identity to the existing resource compilation service.
     * Calls reject stale requests before and after work. They do not lock catalog
     * mutation across resource compilation or loading. A render-thread publisher
     * must guard the final generation check and GPU replacement separately.
     * Stale compilation may leave an unused cooked file at its original path.
     */
    class WPCore_API CatalogResourceAdapter final
    {
    public:
        struct TypeMapping
        {
            String catalogType;
            resource::ResourceTypeID resourceType;
        };

        struct Request
        {
            AssetDatabaseManager::EntrySnapshot entry;
            resource::ResourceID resourceId;
        };

        /**
         * sourceRoot must be the sourceRoot used to initialize resources. The resource
         * interface does not expose its configuration, so the caller owns that binding
         * and must rebuild this adapter if the service is reconfigured. It must match
         * the catalog project root. Mappings are explicit and never rewrite extensions
         * to a different resource type. ResourceID normalizes extension case only when
         * the catalog path policy still identifies the same source file.
         */
        CatalogResourceAdapter( SmartPtr<AssetDatabaseManager> catalog,
                                std::shared_ptr<resource::IResourceSystem> resources,
                                const String &sourceRoot, const Array<TypeMapping> &mappings );

        bool resolve( const String &uuid, Request &request, String &error ) const;
        bool isCurrent( const Request &request, String &error ) const;
        resource::CompilationReport compile( const Request &request,
                                             const resource::CompilationOptions &options = {} ) const;
        std::shared_ptr<const resource::RuntimeResource> load( const Request &request,
                                                               String &error ) const;

    private:
        bool validateBinding( String &error ) const;
        bool validateRequest( const Request &request, String &error ) const;

        SmartPtr<AssetDatabaseManager> m_catalog;
        std::shared_ptr<resource::IResourceSystem> m_resources;
        String m_sourceRoot;
        Array<TypeMapping> m_mappings;
    };
}  // namespace workphone

#endif  // WPCatalogResourceAdapter_h__
