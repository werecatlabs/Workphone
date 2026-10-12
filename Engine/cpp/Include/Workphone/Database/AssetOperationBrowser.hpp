#ifndef WP_ASSET_OPERATION_BROWSER_HPP
#define WP_ASSET_OPERATION_BROWSER_HPP
#include <Workphone/Database/AssetCatalogBrowser.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <stdexcept>

namespace workphone
{
    struct AssetOperationRow
    {
        String id, kind, source, destination, root, state;
    };

    /** Read-only journal inspection. Undo still uses the production service's
     * state, byte-digest, path and collision checks immediately before mutation. */
    struct AssetOperationPage
    {
        Array<AssetOperationRow> rows;
        AssetCatalogSession session;
        bool succeeded = false, hasMore = false;
        String error;

        bool canUndo( SmartPtr<AssetDatabaseManager> catalog, size_t index ) const
        {
            return succeeded && index < rows.size() && catalog && !catalog->isReadOnly() &&
                   session.isCurrent( catalog ) && rows[index].state == "committed" &&
                   rows[index].root == catalog->getProjectRoot();
        }
    };

    inline AssetOperationPage queryAssetOperations( SmartPtr<AssetDatabaseManager> catalog,
                                                    u32 limit = 100, const String &afterId = {} )
    {
        AssetOperationPage page;
        if( !limit || limit > 256 || afterId.size() > 128 || afterId.find( '\0' ) != String::npos ||
            !page.session.capture( catalog ) )
        {
            page.error = "Invalid or unavailable operation journal session.";
            return page;
        }
        auto database = catalog->getDatabase();
        auto bound = database ? dynamic_cast<IParameterizedDatabase *>( database.get() ) : nullptr;
        if( !bound )
        {
            page.error = "Operation journal requires bound database reads.";
            return page;
        }
        try
        {
            {
                DatabaseConnectionLock lock( dynamic_cast<ISerializedDatabase *>( database.get() ) );
                auto table = bound->queryBound(
                    "SELECT name FROM sqlite_master WHERE type='table' AND name='wp_asset_operations'",
                    {} );
                if( !table )
                {
                    throw std::runtime_error( "Cannot inspect operation journal schema." );
                }
                if( !table->eof() )
                {
                    auto rows = bound->queryBound(
                        "SELECT substr(id,1,129) AS id,substr(kind,1,16) AS kind,"
                        "substr(source,1,4097) AS source,substr(destination,1,4097) AS destination,"
                        "substr(root,1,4097) AS root,substr(state,1,32) AS state "
                        "FROM wp_asset_operations WHERE id>? ORDER BY id LIMIT ?",
                        { afterId, StringUtil::toString( limit + 1 ) } );
                    if( !rows )
                    {
                        throw std::runtime_error( "Cannot read operation journal." );
                    }
                    while( !rows->eof() )
                    {
                        if( page.rows.size() == limit )
                        {
                            page.hasMore = true;
                            break;
                        }
                        AssetOperationRow row{
                            rows->getFieldValue( "id" ),     rows->getFieldValue( "kind" ),
                            rows->getFieldValue( "source" ), rows->getFieldValue( "destination" ),
                            rows->getFieldValue( "root" ),   rows->getFieldValue( "state" )
                        };
                        if( row.id.empty() || row.id.size() > 128 || row.source.size() > 4096 ||
                            row.destination.size() > 4096 || row.root.size() > 4096 ||
                            row.id.find( '\0' ) != String::npos ||
                            row.source.find( '\0' ) != String::npos ||
                            row.destination.find( '\0' ) != String::npos ||
                            row.root.find( '\0' ) != String::npos ||
                            ( row.kind != "move" && row.kind != "copy" && row.kind != "delete" ) ||
                            ( row.state != "prepared" && row.state != "committed" &&
                              row.state != "undoing" && row.state != "undone" &&
                              row.state != "rolled-back" ) )
                        {
                            throw std::runtime_error(
                                "Operation journal contains malformed records; explicit repair "
                                "required." );
                        }
                        page.rows.push_back( std::move( row ) );
                        rows->nextRow();
                    }
                }
            }
            if( !page.session.isCurrent( catalog ) )
            {
                throw std::runtime_error( "Project changed while reading operation journal." );
            }
            page.succeeded = true;
        }
        catch( const std::exception &e )
        {
            page.rows.clear();
            page.hasMore = false;
            page.error = e.what();
        }
        return page;
    }
}  // namespace workphone
#endif
