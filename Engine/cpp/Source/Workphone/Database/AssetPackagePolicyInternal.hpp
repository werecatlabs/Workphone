#ifndef WP_ASSET_PACKAGE_POLICY_INTERNAL_HPP
#define WP_ASSET_PACKAGE_POLICY_INTERNAL_HPP
#include <Workphone/Database/AssetPackagePolicy.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Thread/ScopedLock.hpp>

namespace workphone::asset_package_detail
{
    bool copyPolicy( SmartPtr<AssetDatabaseManager>, const String &, const String &, String & );
    inline SmartPtr<IDatabaseQuery> query( SmartPtr<AssetDatabaseManager> catalog, const String &sql,
                                           const Array<String> &values = {} )
    {
        auto db = catalog ? catalog->getDatabase() : nullptr;
        auto bound = db ? dynamic_cast<IParameterizedDatabase *>( db.get() ) : nullptr;
        return bound && db->isLoaded() ? bound->queryBound( sql, values ) : nullptr;
    }

    /** Acquire only after the resource operation fence when used for export.
     * BEGIN IMMEDIATE admits this reader while excluding other SQLite writers.
     * A read admission always rolls back; it never commits authored mutations.
     */
    struct Admission final
    {
        SmartPtr<AssetDatabaseManager> catalog;
        ScopedLock<AssetDatabaseManager> owner;
        SmartPtr<IDatabase> database;
        DatabaseConnectionLock connection;
        bool active = false;

        Admission( SmartPtr<AssetDatabaseManager> value, String &error ) :
            catalog( value ), owner( value ), database( value ? value->getDatabase() : nullptr ),
            connection( database ? dynamic_cast<ISerializedDatabase *>( database.get() ) : nullptr )
        {
            if( !catalog || catalog->isReadOnly() || !database || !database->isLoaded() ||
                !dynamic_cast<ISerializedDatabase *>( database.get() ) ||
                !query( catalog, "BEGIN IMMEDIATE" ) )
            {
                error = "Package mutation admission requires an open writable project catalog and an available SQLite writer transaction";
                return;
            }
            active = true;
        }
        ~Admission()
        {
            if( active ) query( catalog, "ROLLBACK" );
        }
        bool commit( String &error )
        {
            if( !active || !query( catalog, "COMMIT" ) )
            {
                error = "Cannot commit asset package policy";
                return false;
            }
            active = false;
            return true;
        }
    };
}
#endif
