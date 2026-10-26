#include <Workphone/WorkphonePCH.hpp>
#include "AssetPackagePolicyInternal.hpp"

namespace workphone
{
    namespace
    {
        using namespace asset_package_detail;
        const String schemaSQL =
            "CREATE TABLE wp_asset_package_schema(id INTEGER PRIMARY KEY CHECK(id=1),version INTEGER "
            "NOT NULL)";
        // UUID policy survives quarantine/removal so restore cannot silently make
        // an Editor-only asset shippable. Retention follows operation history.
        const String usageSQL =
            "CREATE TABLE wp_asset_package_usage(uuid TEXT NOT NULL PRIMARY KEY,usage TEXT NOT NULL "
            "CHECK(usage IN ('runtime','editor-only','excluded')))";

        String triggerSQL( const String &table, const String &event )
        {
            return "CREATE TRIGGER " + table + "_" + event + " AFTER " + event + " ON " + table +
                   " BEGIN UPDATE wp_asset_catalog_changes SET revision=revision+1 WHERE id=1; END";
        }

        bool exists( SmartPtr<AssetDatabaseManager> catalog, const String &name, bool &present )
        {
            auto row =
                query( catalog, "SELECT name FROM sqlite_master WHERE name=? COLLATE NOCASE", { name } );
            if( !row )
            {
                return false;
            }
            present = !row->eof();
            return true;
        }

        bool schema( SmartPtr<AssetDatabaseManager> catalog, bool create, bool &present, String &error )
        {
            bool marker = false, usage = false;
            if( !exists( catalog, "wp_asset_package_schema", marker ) ||
                !exists( catalog, "wp_asset_package_usage", usage ) )
            {
                error = "Cannot inspect asset package policy schema";
                return false;
            }
            present = marker || usage;
            if( !present && !create )
            {
                return true;
            }
            if( !present )
            {
                if( !query( catalog, schemaSQL ) || !query( catalog, usageSQL ) )
                {
                    error = "Cannot create asset package policy schema";
                    return false;
                }
                for( const String table :
                     { String( "wp_asset_package_schema" ), String( "wp_asset_package_usage" ) } )
                {
                    for( const String event :
                         { String( "INSERT" ), String( "UPDATE" ), String( "DELETE" ) } )
                    {
                        if( !query( catalog, triggerSQL( table, event ) ) )
                        {
                            error = "Cannot install package policy revision tracking";
                            return false;
                        }
                    }
                }
                if( !query( catalog, "INSERT INTO wp_asset_package_schema(id,version) VALUES(1,1)" ) )
                {
                    error = "Cannot initialize asset package policy version";
                    return false;
                }
                present = true;
            }
            else if( !marker || !usage )
            {
                error = "Incomplete asset package policy schema; no automatic repair was applied";
                return false;
            }

            auto exact = [&]( const String &name, const String &type, const String &sql ) {
                auto row = query( catalog, "SELECT sql FROM sqlite_master WHERE name=? AND type=?",
                                  { name, type } );
                if( !row || row->eof() || row->getFieldValue( "sql" ) != sql )
                {
                    return false;
                }
                row->nextRow();
                return row->eof();
            };
            if( !exact( "wp_asset_package_schema", "table", schemaSQL ) ||
                !exact( "wp_asset_package_usage", "table", usageSQL ) )
            {
                error = "Unsupported asset package policy table layout";
                return false;
            }
            auto version = query( catalog, "SELECT id,version FROM wp_asset_package_schema" );
            if( !version || version->eof() || version->getFieldValue( "id" ) != "1" ||
                version->getFieldValue( "version" ) != "1" )
            {
                error = "Unsupported asset package policy version";
                return false;
            }
            version->nextRow();
            if( !version->eof() )
            {
                error = "Ambiguous asset package policy version";
                return false;
            }
            for( const String table :
                 { String( "wp_asset_package_schema" ), String( "wp_asset_package_usage" ) } )
            {
                for( const String event :
                     { String( "INSERT" ), String( "UPDATE" ), String( "DELETE" ) } )
                {
                    if( !exact( table + "_" + event, "trigger", triggerSQL( table, event ) ) )
                    {
                        error = "Missing or unsupported package policy revision tracking";
                        return false;
                    }
                }
            }
            return true;
        }

        bool decode( const String &value, AssetPackageUsage &usage )
        {
            if( value == "runtime" )
            {
                usage = AssetPackageUsage::Runtime;
            }
            else if( value == "editor-only" )
            {
                usage = AssetPackageUsage::EditorOnly;
            }
            else if( value == "excluded" )
            {
                usage = AssetPackageUsage::Excluded;
            }
            else
            {
                return false;
            }
            return true;
        }
    }  // namespace

    bool getAssetPackagePolicy( SmartPtr<AssetDatabaseManager> catalog, const String &uuid,
                                AssetPackagePolicy &policy, String &error )
    {
        policy = {};
        error.clear();
        if( !catalog )
        {
            error = "No asset catalog for package policy";
            return false;
        }
        ScopedLock owner( catalog );
        auto database = catalog->getDatabase();
        DatabaseConnectionLock guard( database ? dynamic_cast<ISerializedDatabase *>( database.get() )
                                               : nullptr );
        AssetPackagePolicy result;
        if( !catalog->tryGetEntry( uuid, result.entry ) ||
            result.entry.kind != AssetDatabaseManager::EntryKind::File )
        {
            error = "Package policy requires a current file asset identity";
            return false;
        }
        bool media = false, mediaEntries = false, present = false;
        if( !exists( catalog, "wp_media_catalog", media ) ||
            !exists( catalog, "wp_media_entries", mediaEntries ) || media != mediaEntries )
        {
            error = "Cannot inspect catalog ownership";
            return false;
        }
        String table = "wp_asset_package_usage";
        if( media )
        {
            auto marker = query( catalog, "SELECT id,namespace,version FROM wp_media_catalog" );
            if( !marker || marker->eof() || marker->getFieldValue( "id" ) != "1" ||
                marker->getFieldValue( "namespace" ) != "workphone.builtin.media.v1" ||
                marker->getFieldValue( "version" ) != "1" )
            {
                error = "Unsupported bundled media package policy";
                return false;
            }
            marker->nextRow();
            if( !marker->eof() )
            {
                error = "Ambiguous bundled media ownership";
                return false;
            }
            table = "wp_media_entries";
            present = true;
        }
        else if( !schema( catalog, false, present, error ) )
        {
            return false;
        }
        if( present )
        {
            auto row =
                query( catalog, "SELECT usage FROM " + table + " WHERE uuid=? LIMIT 2", { uuid } );
            if( !row || ( media && row->eof() ) )
            {
                error = "Missing or unreadable asset package usage";
                return false;
            }
            if( !row->eof() )
            {
                if( !decode( row->getFieldValue( "usage" ), result.usage ) )
                {
                    error = "Unsupported asset package usage";
                    return false;
                }
                result.explicitPolicy = true;
                row->nextRow();
                if( !row->eof() )
                {
                    error = "Ambiguous asset package usage";
                    return false;
                }
            }
        }
        if( !catalog->isEntryCurrent( result.entry ) )
        {
            error = "Catalog changed while inspecting package policy";
            return false;
        }
        policy = result;
        return true;
    }

    bool setAssetPackagePolicy( SmartPtr<AssetDatabaseManager> catalog,
                                const AssetDatabaseManager::EntrySnapshot &entry,
                                AssetPackageUsage usage, String &error )
    {
        error.clear();
        String encoded;
        switch( usage )
        {
        case AssetPackageUsage::Runtime:
            encoded = "runtime";
            break;
        case AssetPackageUsage::EditorOnly:
            encoded = "editor-only";
            break;
        case AssetPackageUsage::Excluded:
            encoded = "excluded";
            break;
        default:
            error = "Invalid asset package usage";
            return false;
        }
        Admission admission( catalog, error );
        if( !admission.active )
        {
            return false;
        }
        if( !catalog->isEntryCurrent( entry ) || entry.kind != AssetDatabaseManager::EntryKind::File )
        {
            error = "Asset identity or project session changed; refresh before changing shipping policy";
            return false;
        }
        bool media = false, mediaEntries = false, present = false;
        if( !exists( catalog, "wp_media_catalog", media ) ||
            !exists( catalog, "wp_media_entries", mediaEntries ) || media || mediaEntries )
        {
            error = "Bundled media shipping policy is owned by its inventory builder";
            return false;
        }
        if( !schema( catalog, true, present, error ) )
        {
            return false;
        }
        if( !query( catalog, "INSERT OR REPLACE INTO wp_asset_package_usage(uuid,usage) VALUES(?,?)",
                    { entry.uuid, encoded } ) )
        {
            error = "Cannot write asset package usage";
            return false;
        }
        return admission.commit( error );
    }

    bool asset_package_detail::copyPolicy( SmartPtr<AssetDatabaseManager> catalog, const String &source,
                                           const String &destination, String &error )
    {
        AssetPackagePolicy policy;
        if( !getAssetPackagePolicy( catalog, source, policy, error ) )
        {
            return false;
        }
        if( !policy.explicitPolicy )
        {
            return true;
        }
        if( !query( catalog,
                    "INSERT INTO wp_asset_package_usage(uuid,usage) SELECT ?,usage FROM "
                    "wp_asset_package_usage WHERE uuid=?",
                    { destination, source } ) )
        {
            error = "Cannot copy asset shipping policy";
            return false;
        }
        return true;
    }
}  // namespace workphone
