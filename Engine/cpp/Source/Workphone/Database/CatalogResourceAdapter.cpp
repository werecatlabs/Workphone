#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/CatalogResourceAdapter.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>

#include <filesystem>
#include <set>

namespace workphone
{
    CatalogResourceAdapter::CatalogResourceAdapter( SmartPtr<AssetDatabaseManager> catalog,
                                                    std::shared_ptr<resource::IResourceSystem> resources,
                                                    const String &sourceRoot,
                                                    const Array<TypeMapping> &mappings ) :
        m_catalog( catalog ),
        m_resources( std::move( resources ) ),
        m_mappings( mappings )
    {
        String error;
        canonicalAssetCatalogRoot( sourceRoot, m_sourceRoot, error );
    }

    bool CatalogResourceAdapter::validateBinding( String &error ) const
    {
        if( !m_catalog || !m_resources || !m_resources->isInitialized() )
        {
            error = "Catalog resource adapter requires a catalog and initialized resource system";
            return false;
        }
        auto catalog = m_catalog;
        const auto catalogRoot = catalog->getProjectRoot();
        if( m_sourceRoot.empty() || catalogRoot.empty() || m_sourceRoot.find( '\0' ) != String::npos )
        {
            error = "Catalog resource adapter requires an explicit source root binding";
            return false;
        }
        std::error_code filesystemError;
        const bool sameRoot = std::filesystem::equivalent(
            std::filesystem::u8path( m_sourceRoot.c_str() ),
            std::filesystem::u8path( catalogRoot.c_str() ), filesystemError );
        if( filesystemError || !sameRoot )
        {
            error = "Resource source root does not match the catalog project root";
            return false;
        }
        std::set<String> catalogTypes;
        for( const auto &mapping : m_mappings )
        {
            if( mapping.catalogType.empty() || mapping.catalogType.find( '\0' ) != String::npos ||
                !mapping.resourceType.isValid() || !catalogTypes.insert( mapping.catalogType ).second )
            {
                error = "Catalog resource type mappings must be valid and unique by catalog type";
                return false;
            }
        }
        return true;
    }

    bool CatalogResourceAdapter::validateRequest( const Request &request, String &error ) const
    {
        if( !validateBinding( error ) )
            return false;
        auto catalog = m_catalog;
        if( !catalog->isEntryCurrent( request.entry ) )
        {
            error = "Catalog resource request is stale or its entry is unavailable";
            return false;
        }
        if( request.entry.kind != AssetDatabaseManager::EntryKind::File )
        {
            error = "Scene catalog entries cannot be compiled as file resources";
            return false;
        }
        resource::ResourceID expected;
        if( !expected.set( request.entry.path, &error ) || expected.isSubResource() ||
            expected != request.resourceId )
        {
            error = "Catalog source path must identify the exact canonical resource source file";
            return false;
        }
        AssetCatalogPath catalogPath;
        AssetCatalogPath resourcePath;
        const auto catalogRoot = catalog->getProjectRoot();
        if( !canonicalAssetCatalogPath( catalogRoot, request.entry.path, catalogPath, error ) ||
            !canonicalAssetCatalogPath( catalogRoot, expected.sourceRelativePath(), resourcePath,
                                        error ) ||
            catalogPath.key != resourcePath.key )
        {
            error = "ResourceID normalization must preserve the catalog source file identity";
            return false;
        }
        const TypeMapping *selected = nullptr;
        for( const auto &mapping : m_mappings )
        {
            if( mapping.catalogType == request.entry.type )
            {
                selected = &mapping;
                break;
            }
        }
        if( !selected )
        {
            error = String( "No resource type mapping for catalog type '" ) + request.entry.type + "'";
            return false;
        }
        if( selected->resourceType != expected.type() )
        {
            error = "Catalog type mapping does not match the source file's resource extension";
            return false;
        }
        const auto registry = m_resources->registry();
        const auto compiler = registry ? registry->getCompiler( expected.type() ) : nullptr;
        if( !compiler || compiler->versionFor( expected.type() ) == 0 )
        {
            error = String( "No compiler registered for catalog resource type '" ) +
                    expected.type().str() + "'";
            return false;
        }
        return true;
    }

    bool CatalogResourceAdapter::resolve( const String &uuid, Request &request, String &error ) const
    {
        request = Request();
        error.clear();
        if( !validateBinding( error ) )
            return false;
        Request resolved;
        auto catalog = m_catalog;
        if( !catalog->tryGetEntry( uuid, resolved.entry ) )
        {
            error = "Catalog resource UUID is unavailable";
            return false;
        }
        resolved.resourceId.set( resolved.entry.path );
        if( !validateRequest( resolved, error ) )
            return false;
        request = std::move( resolved );
        return true;
    }

    bool CatalogResourceAdapter::isCurrent( const Request &request, String &error ) const
    {
        error.clear();
        return validateRequest( request, error );
    }

    resource::CompilationReport CatalogResourceAdapter::compile(
        const Request &request, const resource::CompilationOptions &options ) const
    {
        String error;
        if( !validateRequest( request, error ) )
        {
            resource::CompilationReport failure;
            failure.resourceId = request.resourceId;
            failure.messages.push_back( error );
            return failure;
        }
        auto report = m_resources->compile( request.resourceId, options );
        if( !validateRequest( request, error ) )
        {
            report.status = resource::CompilationStatus::Failure;
            report.messages.push_back( error );
        }
        return report;
    }

    std::shared_ptr<const resource::RuntimeResource> CatalogResourceAdapter::load(
        const Request &request, String &error ) const
    {
        error.clear();
        if( !validateRequest( request, error ) )
            return nullptr;
        auto loaded = m_resources->load( request.resourceId, error );
        if( !loaded || !validateRequest( request, error ) )
            return nullptr;
        const auto registry = m_resources->registry();
        const auto compiler = registry ? registry->getCompiler( request.resourceId.type() ) : nullptr;
        if( !compiler || loaded->header.resourceId != request.resourceId ||
            loaded->header.resourceType != request.resourceId.type() ||
            loaded->header.compilerVersion != compiler->versionFor( request.resourceId.type() ) )
        {
            error = "Compiled catalog resource identity, type or compiler version does not match";
            return nullptr;
        }
        return loaded;
    }
}  // namespace workphone
