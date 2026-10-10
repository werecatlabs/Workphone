#include <WPSQLite/SQLiteDatabase.hpp>
#include <WPSQLite/extern/CppSQLite3.hpp>
#include <WPSQLite/SQLiteQuery.hpp>
#include <Workphone/Workphone.hpp>
#include <filesystem>
#include <memory>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SQLiteDatabase, IDatabase );

    SQLiteDatabase::SQLiteDatabase()
    {
        m_database = workphone::make_shared<CppSQLite3DB>();

        //if (sqlite3_threadsafe() == 0)
        //{
        //	WP_LOG_ERROR("Error: sqlite compiled without thread safety.");
        //}
    }

    SQLiteDatabase::~SQLiteDatabase()
    {
        unload( nullptr );
    }

    void SQLiteDatabase::unload( SmartPtr<ISharedObject> data )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        try
        {
            setLoadingState( LoadingState::Unloading );

            close();
            m_database = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SQLiteDatabase::loadFromFile( const String &filePath )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            if( !StringUtil::isNullOrEmpty( filePath ) )
            {
                if( !m_database )
                {
                    m_database = workphone::make_shared<CppSQLite3DB>();
                }

                if( m_database )
                {
                    m_database->open( filePath.c_str() );
                }
                else
                {
                    WP_LOG_ERROR( "SQLiteDatabase::loadFromFile: Failed to create database" );
                }
            }
            else
            {
                WP_LOG_ERROR( "SQLiteDatabase::loadFromFile: Invalid file path" );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( CppSQLite3Exception &e )
        {
            String message = e.errorMessage();
            WP_LOG_ERROR( message );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SQLiteDatabase::loadFromFile( const String &filePath, const String &key )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        try
        {
            setLoadingState( LoadingState::Loading );

            WP_LOG( "Load database: " + filePath );

            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            if( !m_database )
                m_database = workphone::make_shared<CppSQLite3DB>();
            m_database->open( filePath.c_str() );

            if( !StringUtil::isNullOrEmpty( key ) )
            {
                sqlite3_key( m_database->mpDB, key.c_str(), (int)key.size() );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( CppSQLite3Exception &e )
        {
            String message = e.errorMessage();
            WP_LOG_ERROR( message );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SQLiteDatabase::setKey( const String &key )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        if( m_database && isLoaded() && !StringUtil::isNullOrEmpty( key ) )
        {
            sqlite3_rekey( m_database->mpDB, key.c_str(), (int)key.size() );
        }
    }

    SmartPtr<IDatabaseQuery> SQLiteDatabase::query( const String &queryStr )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        if( !m_database || !isLoaded() )
            return nullptr;
        try
        {
            if( !StringUtil::isNullOrEmpty( queryStr ) )
            {
                auto query = m_database->execQuery( queryStr.c_str() );

                auto pQuery = workphone::make_ptr<SQLiteQuery>();
                pQuery->setQuery( query );
                return pQuery;
            }
        }
        catch( CppSQLite3Exception &e )
        {
            String message = e.errorMessage();
            WP_LOG_ERROR( message );
        }

        return nullptr;
    }

    SmartPtr<IDatabaseQuery> SQLiteDatabase::queryBound( const String &sql,
                                                       const Array<String> &values )
    {
        try
        {
            std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
            if( !m_database || !isLoaded() || sql.empty() ) return nullptr;
            if( values.empty() )
            {
                // Legacy runScript can submit several mutation statements at
                // once. Preparing just the first would silently drop the rest.
                sqlite3_stmt *probe = nullptr;
                const auto prepared = sqlite3_prepare_v2(m_database->mpDB, sql.c_str(), -1, &probe, nullptr);
                const bool mutation = prepared == SQLITE_OK && probe && sqlite3_column_count(probe) == 0;
                sqlite3_finalize(probe);
                if( mutation )
                {
                    const auto executed = sqlite3_exec(m_database->mpDB, sql.c_str(), nullptr, nullptr, nullptr);
                    if( executed != SQLITE_OK )
                    {
                        WP_LOG_ERROR(sqlite3_errmsg(m_database->mpDB));
                        return nullptr;
                    }
                    return workphone::make_ptr<SQLiteQuery>();
                }
            }
            auto statement = m_database->compileStatement( sql.c_str() );
            for( size_t i = 0; i < values.size(); ++i )
            {
                if( values[i].find( '\0' ) != String::npos ) return nullptr;
                statement.bind( static_cast<int>(i + 1), values[i].c_str() );
            }
            auto query = statement.execQuery();
            auto result = workphone::make_ptr<SQLiteQuery>();
            result->setQuery( query );
            return result;
        }
        catch( CppSQLite3Exception &e )
        {
            WP_LOG_ERROR( e.errorMessage() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        return nullptr;
    }

    SmartPtr<IDatabaseQuery> SQLiteDatabase::query( const StringW &queryStr )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        if( !m_database || !isLoaded() )
            return nullptr;
        try
        {
            if( !StringUtilW::isNullOrEmpty( queryStr ) )
            {
                auto str = StringUtilW::toUTF16to8( queryStr );
                auto query = m_database->execQuery( str.c_str() );

                auto pQuery = workphone::make_ptr<SQLiteQuery>();
                pQuery->setQuery( query );
                return pQuery;
            }
        }
        catch( CppSQLite3Exception &e )
        {
            String message = e.errorMessage();
            WP_LOG_ERROR( message );
        }

        return nullptr;
    }

    void SQLiteDatabase::queryDML( const StringW &queryStr )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        if( !m_database || !isLoaded() )
            return;
        try
        {
            m_database->execDML( queryStr.c_str() );
        }
        catch( CppSQLite3Exception &e )
        {
            WP_LOG_ERROR( e.errorMessage() );
        }
    }

    void SQLiteDatabase::queryDML( const String &queryStr )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        if( !m_database || !isLoaded() )
            return;
        try
        {
            m_database->execDML( queryStr.c_str() );
        }
        catch( CppSQLite3Exception &e )
        {
            WP_LOG_ERROR( e.errorMessage() );
        }
    }

    void SQLiteDatabase::close()
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        if( m_database )
        {
            m_database->close();
        }
        setLoadingState( LoadingState::Unloaded );
    }

    void SQLiteDatabase::lockConnection()
    {
        m_boundQueryMutex.lock();
    }

    void SQLiteDatabase::unlockConnection()
    {
        m_boundQueryMutex.unlock();
    }

    bool SQLiteDatabase::backupTo( const String &path, String &error )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        error.clear();
        try
        {
        if( !m_database || !isLoaded() || path.empty() || path.find('\0') != String::npos )
        {
            error = "Backup requires an open database and a valid destination";
            return false;
        }
        namespace fs = std::filesystem;
        std::error_code ec;
        auto destination = fs::absolute( fs::u8path(path.c_str()), ec );
        if( ec || fs::exists(destination, ec) || ec )
        {
            error = "Backup destination already exists or cannot be inspected";
            return false;
        }
        fs::create_directories(destination.parent_path(), ec);
        if( ec ) { error = ec.message().c_str(); return false; }
        auto temporary = destination;
        temporary += std::string(".") + StringUtil::getUUID().c_str() + ".tmp";
        struct Cleanup
        {
            fs::path path;
            ~Cleanup() { std::error_code ignored; fs::remove(path, ignored); }
        } cleanup{temporary};
        sqlite3 *handle = nullptr;
        const auto temporaryText = temporary.u8string();
        const auto opened = sqlite3_open_v2(temporaryText.c_str(), &handle,
                                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        std::unique_ptr<sqlite3, decltype(&sqlite3_close)> output(handle, &sqlite3_close);
        if( opened != SQLITE_OK )
        {
            error = handle ? sqlite3_errmsg(handle) : "Cannot open backup destination";
            return false;
        }
        auto backup = sqlite3_backup_init(handle, "main", m_database->mpDB, "main");
        if( !backup ) { error = sqlite3_errmsg(handle); return false; }
        const auto step = sqlite3_backup_step(backup, -1);
        const auto finished = sqlite3_backup_finish(backup);
        if( step != SQLITE_DONE || finished != SQLITE_OK )
        {
            error = sqlite3_errmsg(handle);
            return false;
        }
        output.reset();
        // A snapshot is published only after SQLite has completed and closed it.
        fs::create_hard_link(temporary, destination, ec);
        if( ec ) { error = ec.message().c_str(); return false; }
        return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }
}  // namespace workphone
