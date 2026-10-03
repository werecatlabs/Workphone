#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <WPSQLite/extern/sqlite3.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <mutex>

namespace workphone::resource
{
    namespace
    {
        constexpr int schemaVersion = 2;

        class Statement
        {
        public:
            Statement( sqlite3 *database, const char *sql )
            {
                m_result = sqlite3_prepare_v2( database, sql, -1, &m_statement, nullptr );
            }

            ~Statement()
            {
                if( m_statement )
                {
                    sqlite3_finalize( m_statement );
                }
            }

            Statement( const Statement & ) = delete;
            Statement &operator=( const Statement & ) = delete;

            bool isValid() const
            {
                return m_result == SQLITE_OK && m_statement != nullptr;
            }

            int result() const
            {
                return m_result;
            }

            sqlite3_stmt *get() const
            {
                return m_statement;
            }

        private:
            sqlite3_stmt *m_statement = nullptr;
            int m_result = SQLITE_ERROR;
        };

        bool bindText( sqlite3_stmt *statement, int index, const String &value )
        {
            return sqlite3_bind_text( statement, index, value.c_str(), static_cast<int>( value.size() ),
                                      SQLITE_TRANSIENT ) == SQLITE_OK;
        }

        bool bindU64( sqlite3_stmt *statement, int index, u64 value )
        {
            return sqlite3_bind_blob( statement, index, &value, sizeof( value ), SQLITE_TRANSIENT ) ==
                   SQLITE_OK;
        }

        u64 readU64( sqlite3_stmt *statement, int column )
        {
            u64 value = 0;
            if( sqlite3_column_type( statement, column ) == SQLITE_BLOB &&
                sqlite3_column_bytes( statement, column ) == sizeof( value ) )
            {
                std::memcpy( &value, sqlite3_column_blob( statement, column ), sizeof( value ) );
            }
            return value;
        }

        String readText( sqlite3_stmt *statement, int column )
        {
            const auto *text = sqlite3_column_text( statement, column );
            return text ? reinterpret_cast<const char *>( text ) : "";
        }
    }  // namespace

    class ResourceCompilationDatabase::Impl
    {
    public:
        bool execute( const char *sql )
        {
            char *message = nullptr;
            const int result = sqlite3_exec( database, sql, nullptr, nullptr, &message );
            if( result != SQLITE_OK )
            {
                setError( message ? message : sqlite3_errmsg( database ) );
                sqlite3_free( message );
                return false;
            }
            return true;
        }

        void setError( const char *message ) const
        {
            lastError = message ? message : "Unknown SQLite error";
        }

        bool createSchema()
        {
            static const char *sql =
                "CREATE TABLE IF NOT EXISTS compiled_resources("
                "resource_id TEXT PRIMARY KEY NOT NULL,"
                "resource_type TEXT NOT NULL,"
                "compiler_version BLOB NOT NULL,"
                "source_hash BLOB NOT NULL,"
                "output_hash BLOB NOT NULL,"
                "output_path TEXT NOT NULL,"
                "updated_at INTEGER NOT NULL);"
                "CREATE TABLE IF NOT EXISTS compile_dependencies("
                "resource_id TEXT NOT NULL,"
                "dependency_path TEXT NOT NULL,"
                "is_resource INTEGER NOT NULL,"
                "PRIMARY KEY(resource_id, dependency_path),"
                "FOREIGN KEY(resource_id) REFERENCES compiled_resources(resource_id) ON DELETE CASCADE);"
                "CREATE INDEX IF NOT EXISTS idx_compile_dependencies_path "
                "ON compile_dependencies(dependency_path);";
            return execute( sql );
        }

        bool migrateSchema()
        {
            Statement versionStatement( database, "PRAGMA user_version;" );
            if( !versionStatement.isValid() )
            {
                setError( sqlite3_errmsg( database ) );
                return false;
            }

            int currentVersion = 0;
            if( sqlite3_step( versionStatement.get() ) == SQLITE_ROW )
            {
                currentVersion = sqlite3_column_int( versionStatement.get(), 0 );
            }

            if( currentVersion == schemaVersion )
            {
                return createSchema();
            }
            if( currentVersion > schemaVersion )
            {
                setError( "Compilation database schema is newer than this engine supports" );
                return false;
            }

            if( !execute( "BEGIN IMMEDIATE;" ) )
            {
                return false;
            }

            const bool succeeded = execute(
                                       "DROP TABLE IF EXISTS compile_dependencies;"
                                       "DROP TABLE IF EXISTS compiled_resources;" ) &&
                                   createSchema() && execute( "PRAGMA user_version=2;" ) &&
                                   execute( "COMMIT;" );

            if( !succeeded )
            {
                sqlite3_exec( database, "ROLLBACK;", nullptr, nullptr, nullptr );
            }
            return succeeded;
        }

        mutable std::mutex mutex;
        sqlite3 *database = nullptr;
        mutable String lastError;
    };

    ResourceCompilationDatabase::ResourceCompilationDatabase() : m_impl( new Impl() )
    {
    }

    ResourceCompilationDatabase::~ResourceCompilationDatabase()
    {
        disconnect();
        delete m_impl;
        m_impl = nullptr;
    }

    bool ResourceCompilationDatabase::connect( const String &databasePath )
    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        if( m_impl->database )
        {
            m_impl->setError( "Compilation database is already connected" );
            return false;
        }
        if( databasePath.empty() )
        {
            m_impl->setError( "Compilation database path is empty" );
            return false;
        }

        std::error_code error;
        const std::filesystem::path path( databasePath.c_str() );
        if( path.has_parent_path() )
        {
            std::filesystem::create_directories( path.parent_path(), error );
            if( error )
            {
                m_impl->lastError = String( "Failed to create compilation database directory: " ) +
                                    error.message().c_str();
                return false;
            }
        }

        const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
        const int result = sqlite3_open_v2( databasePath.c_str(), &m_impl->database, flags, nullptr );
        if( result != SQLITE_OK )
        {
            m_impl->setError( m_impl->database ? sqlite3_errmsg( m_impl->database )
                                               : "Failed to allocate SQLite connection" );
            if( m_impl->database )
            {
                sqlite3_close( m_impl->database );
                m_impl->database = nullptr;
            }
            return false;
        }

        sqlite3_extended_result_codes( m_impl->database, 1 );
        sqlite3_busy_timeout( m_impl->database, 5000 );
        m_impl->lastError.clear();

        if( !m_impl->execute( "PRAGMA foreign_keys=ON;"
                              "PRAGMA journal_mode=WAL;"
                              "PRAGMA synchronous=NORMAL;"
                              "PRAGMA temp_store=MEMORY;" ) ||
            !m_impl->migrateSchema() )
        {
            sqlite3_close( m_impl->database );
            m_impl->database = nullptr;
            return false;
        }
        return true;
    }

    void ResourceCompilationDatabase::disconnect()
    {
        if( !m_impl )
        {
            return;
        }

        std::lock_guard<std::mutex> lock( m_impl->mutex );
        if( m_impl->database )
        {
            sqlite3_wal_checkpoint_v2( m_impl->database, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr,
                                       nullptr );
            const int result = sqlite3_close( m_impl->database );
            if( result != SQLITE_OK )
            {
                m_impl->setError( "Failed to close the SQLite compilation database" );
            }
            m_impl->database = nullptr;
        }
    }

    bool ResourceCompilationDatabase::isConnected() const
    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        return m_impl->database != nullptr;
    }

    String ResourceCompilationDatabase::getLastError() const
    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        return m_impl->lastError;
    }

    bool ResourceCompilationDatabase::reset()
    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->lastError.clear();
        if( !m_impl->database )
        {
            m_impl->setError( "Compilation database is not connected" );
            return false;
        }

        if( !m_impl->execute( "BEGIN IMMEDIATE;" ) )
        {
            return false;
        }
        const bool succeeded = m_impl->execute(
                                   "DELETE FROM compile_dependencies;"
                                   "DELETE FROM compiled_resources;" ) &&
                               m_impl->execute( "COMMIT;" );
        if( !succeeded )
        {
            sqlite3_exec( m_impl->database, "ROLLBACK;", nullptr, nullptr, nullptr );
        }
        return succeeded;
    }

    bool ResourceCompilationDatabase::getRecord( const String &resourceId,
                                                 CompiledResourceRecord &record ) const
    {
        record.clear();
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->lastError.clear();
        if( !m_impl->database )
        {
            m_impl->setError( "Compilation database is not connected" );
            return false;
        }

        Statement statement(
            m_impl->database,
            "SELECT resource_id,resource_type,compiler_version,source_hash,output_hash,output_path "
            "FROM compiled_resources WHERE resource_id=?1;" );
        if( !statement.isValid() || !bindText( statement.get(), 1, resourceId ) )
        {
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            return false;
        }

        const int result = sqlite3_step( statement.get() );
        if( result == SQLITE_DONE )
        {
            return true;
        }
        if( result != SQLITE_ROW )
        {
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            return false;
        }

        record.resourceId = readText( statement.get(), 0 );
        record.resourceType = readText( statement.get(), 1 );
        record.compilerVersion = readU64( statement.get(), 2 );
        record.sourceHash = readU64( statement.get(), 3 );
        record.outputHash = readU64( statement.get(), 4 );
        record.outputPath = readText( statement.get(), 5 );
        return true;
    }

    bool ResourceCompilationDatabase::commitCompilation(
        const CompiledResourceRecord &record, const Array<CompileDependencyRecord> &dependencies )
    {
        if( !record.isValid() )
        {
            std::lock_guard<std::mutex> lock( m_impl->mutex );
            m_impl->setError( "Cannot commit an invalid compiled resource record" );
            return false;
        }

        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->lastError.clear();
        if( !m_impl->database )
        {
            m_impl->setError( "Compilation database is not connected" );
            return false;
        }
        if( !m_impl->execute( "BEGIN IMMEDIATE;" ) )
        {
            return false;
        }

        bool succeeded = true;
        Statement recordStatement(
            m_impl->database,
            "INSERT OR REPLACE INTO "
            "compiled_resources(resource_id,resource_type,compiler_version,source_hash,"
            "output_hash,output_path,updated_at) VALUES(?1,?2,?3,?4,?5,?6,?7) "
            ";" );

        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                             std::chrono::system_clock::now().time_since_epoch() )
                             .count();
        if( !recordStatement.isValid() || !bindText( recordStatement.get(), 1, record.resourceId ) ||
            !bindText( recordStatement.get(), 2, record.resourceType ) ||
            !bindU64( recordStatement.get(), 3, record.compilerVersion ) ||
            !bindU64( recordStatement.get(), 4, record.sourceHash ) ||
            !bindU64( recordStatement.get(), 5, record.outputHash ) ||
            !bindText( recordStatement.get(), 6, record.outputPath ) ||
            sqlite3_bind_int64( recordStatement.get(), 7, now ) != SQLITE_OK ||
            sqlite3_step( recordStatement.get() ) != SQLITE_DONE )
        {
            succeeded = false;
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
        }

        if( succeeded )
        {
            Statement deleteStatement( m_impl->database,
                                       "DELETE FROM compile_dependencies WHERE resource_id=?1;" );
            if( !deleteStatement.isValid() || !bindText( deleteStatement.get(), 1, record.resourceId ) ||
                sqlite3_step( deleteStatement.get() ) != SQLITE_DONE )
            {
                succeeded = false;
                m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            }
        }

        if( succeeded )
        {
            Statement dependencyStatement(
                m_impl->database,
                "INSERT INTO compile_dependencies(resource_id,dependency_path,is_resource) "
                "VALUES(?1,?2,?3);" );
            if( !dependencyStatement.isValid() )
            {
                succeeded = false;
                m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            }
            else
            {
                for( const auto &dependency : dependencies )
                {
                    if( dependency.path.empty() ||
                        !bindText( dependencyStatement.get(), 1, record.resourceId ) ||
                        !bindText( dependencyStatement.get(), 2, dependency.path ) ||
                        sqlite3_bind_int( dependencyStatement.get(), 3,
                                          dependency.isResource ? 1 : 0 ) != SQLITE_OK ||
                        sqlite3_step( dependencyStatement.get() ) != SQLITE_DONE )
                    {
                        succeeded = false;
                        m_impl->setError( dependency.path.empty()
                                              ? "Cannot commit an empty dependency path"
                                              : sqlite3_errmsg( m_impl->database ) );
                        break;
                    }
                    sqlite3_reset( dependencyStatement.get() );
                    sqlite3_clear_bindings( dependencyStatement.get() );
                }
            }
        }

        if( succeeded )
        {
            succeeded = m_impl->execute( "COMMIT;" );
        }
        if( !succeeded )
        {
            sqlite3_exec( m_impl->database, "ROLLBACK;", nullptr, nullptr, nullptr );
        }
        return succeeded;
    }

    bool ResourceCompilationDatabase::removeRecord( const String &resourceId )
    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->lastError.clear();
        if( !m_impl->database )
        {
            m_impl->setError( "Compilation database is not connected" );
            return false;
        }

        Statement statement( m_impl->database, "DELETE FROM compiled_resources WHERE resource_id=?1;" );
        if( !statement.isValid() || !bindText( statement.get(), 1, resourceId ) ||
            sqlite3_step( statement.get() ) != SQLITE_DONE )
        {
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            return false;
        }
        return true;
    }

    bool ResourceCompilationDatabase::getDependencies(
        const String &resourceId, Array<CompileDependencyRecord> &dependencies ) const
    {
        dependencies.clear();
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->lastError.clear();
        if( !m_impl->database )
        {
            m_impl->setError( "Compilation database is not connected" );
            return false;
        }

        Statement statement( m_impl->database,
                             "SELECT dependency_path,is_resource FROM compile_dependencies "
                             "WHERE resource_id=?1 ORDER BY dependency_path;" );
        if( !statement.isValid() || !bindText( statement.get(), 1, resourceId ) )
        {
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            return false;
        }

        int result = SQLITE_ROW;
        while( ( result = sqlite3_step( statement.get() ) ) == SQLITE_ROW )
        {
            CompileDependencyRecord dependency;
            dependency.path = readText( statement.get(), 0 );
            dependency.isResource = sqlite3_column_int( statement.get(), 1 ) != 0;
            dependencies.push_back( dependency );
        }
        if( result != SQLITE_DONE )
        {
            dependencies.clear();
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            return false;
        }
        return true;
    }

    bool ResourceCompilationDatabase::getDependents( const String &dependencyPath,
                                                     Array<String> &resourceIds ) const
    {
        resourceIds.clear();
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->lastError.clear();
        if( !m_impl->database )
        {
            m_impl->setError( "Compilation database is not connected" );
            return false;
        }

        Statement statement( m_impl->database,
                             "SELECT resource_id FROM compile_dependencies WHERE dependency_path=?1 "
                             "ORDER BY resource_id;" );
        if( !statement.isValid() || !bindText( statement.get(), 1, dependencyPath ) )
        {
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            return false;
        }

        int result = SQLITE_ROW;
        while( ( result = sqlite3_step( statement.get() ) ) == SQLITE_ROW )
        {
            resourceIds.push_back( readText( statement.get(), 0 ) );
        }
        if( result != SQLITE_DONE )
        {
            resourceIds.clear();
            m_impl->setError( sqlite3_errmsg( m_impl->database ) );
            return false;
        }
        return true;
    }
}  // namespace workphone::resource
