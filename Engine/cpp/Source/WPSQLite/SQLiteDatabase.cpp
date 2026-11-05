#include <WPSQLite/SQLiteDatabase.hpp>
#include <WPSQLite/extern/CppSQLite3.hpp>
#include <WPSQLite/SQLiteQuery.hpp>
#include <Workphone/Workphone.hpp>
#include <filesystem>
#include <memory>
#include <string_view>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SQLiteDatabase, IDatabase );

    namespace
    {
        bool globComparableUtf8( std::string_view text ) noexcept
        {
            for( size_t index = 0; index < text.size(); )
            {
                const auto first = static_cast<unsigned char>( text[index++] );
                if( first < 0x80 )
                {
                    if( first == 0 ) return false;
                    continue;
                }
                const size_t count = first >= 0xc2 && first <= 0xdf   ? 1
                                   : first >= 0xe0 && first <= 0xef ? 2
                                   : first >= 0xf0 && first <= 0xf4 ? 3
                                                                    : 0;
                if( !count || text.size() - index < count ) return false;
                unsigned codepoint = first & ( 0x7f >> count );
                for( size_t byte = 0; byte < count; ++byte )
                {
                    const auto next = static_cast<unsigned char>( text[index++] );
                    if( ( next & 0xc0 ) != 0x80 ) return false;
                    codepoint = ( codepoint << 6 ) | ( next & 0x3f );
                }
                const unsigned minimum = count == 1 ? 0x80 : count == 2 ? 0x800 : 0x10000;
                // The bundled SQLite GLOB decoder replaces U+FFFE and U+FFFF.
                if( codepoint < minimum || codepoint > 0x10ffff ||
                    ( codepoint >= 0xd800 && codepoint <= 0xdfff ) ||
                    codepoint == 0xfffe || codepoint == 0xffff )
                {
                    return false;
                }
            }
            return true;
        }

        bool catalogSearchArgument( sqlite3_value *argument, int limit,
                                    std::string_view &text ) noexcept
        {
            if( sqlite3_value_type( argument ) != SQLITE_TEXT ) return false;
            const auto *data = reinterpret_cast<const char *>( sqlite3_value_text( argument ) );
            const auto size = sqlite3_value_bytes( argument );
            if( !data || size < 0 || size > limit ) return false;
            text = std::string_view( data, static_cast<size_t>( size ) );
            return globComparableUtf8( text );
        }

        void catalogLiteralContains( sqlite3_context *context, int count,
                                     sqlite3_value **arguments ) noexcept
        {
            std::string_view path, uuid, term;
            if( count != 3 || !catalogSearchArgument( arguments[2], 1024, term ) ||
                !catalogSearchArgument( arguments[0], 1024, path ) ||
                !catalogSearchArgument( arguments[1], 256, uuid ) )
            {
                sqlite3_result_null( context );
                return;
            }
            // A literal substring of valid UTF-8 matches the escaped GLOB on
            // the same codepoint boundaries. Views borrow the SQLite buffers.
            sqlite3_result_int( context, path.find( term ) != std::string_view::npos ||
                                         uuid.find( term ) != std::string_view::npos );
        }

        void registerCatalogSearch( sqlite3 *database ) noexcept
        {
            // Optional acceleration. The catalog can still query with GLOB if
            // the backend cannot register this function.
            (void)sqlite3_create_function( database, "wp_catalog_contains", 3, SQLITE_UTF8, nullptr,
                                           catalogLiteralContains, nullptr, nullptr );
        }
    }

    bool SQLiteDatabase::rollbackTransaction() noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(m_boundQueryMutex);
        return m_database && m_database->mpDB &&
            sqlite3_exec(m_database->mpDB, "ROLLBACK", nullptr, nullptr, nullptr) == SQLITE_OK;
    }

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

    bool SQLiteDatabase::openReadOnly( const String &path, String &error )
    {
        std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
        error.clear();
        close();
        if( path.empty() || path.find( '\0' ) != String::npos )
        {
            error = "Invalid read-only database path.";
            return false;
        }
        m_database = workphone::make_shared<CppSQLite3DB>();
        auto status = sqlite3_open_v2( path.c_str(), &m_database->mpDB,
                                      SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, nullptr );
        if( status != SQLITE_OK )
        {
            error = m_database->mpDB ? sqlite3_errmsg( m_database->mpDB ) : "Cannot open read-only database.";
            close();
            return false;
        }
        sqlite3_busy_timeout( m_database->mpDB, 5000 );
        registerCatalogSearch( m_database->mpDB );
        setLoadingState( LoadingState::Loaded );
        return true;
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
                    m_database->setBusyTimeout( 1000 );
                    registerCatalogSearch( m_database->mpDB );
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
            m_database->setBusyTimeout( 1000 );

            if( !StringUtil::isNullOrEmpty( key ) )
            {
                sqlite3_key( m_database->mpDB, key.c_str(), (int)key.size() );
            }

            registerCatalogSearch( m_database->mpDB );
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
                auto pQuery = workphone::make_ptr<SQLiteQuery>();
                auto query = m_database->execQuery( queryStr.c_str() );
                pQuery->setQuery( query );
                return pQuery;
            }
        }
        catch( CppSQLite3Exception &e )
        {
            String message = e.errorMessage();
            WP_LOG_ERROR( message );
        }

        catch( const std::bad_alloc & ) {} // Denial needs no diagnostic allocation.
        catch( const std::exception &e ) { WP_LOG_EXCEPTION(e); }
        return nullptr;
    }

    SmartPtr<IDatabaseQuery> SQLiteDatabase::queryBound( const String &sql,
                                                       const Array<String> &values )
    {
        try
        {
            std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
            if( !m_database || !isLoaded() || sql.empty() ) return nullptr;
            // Admit generic ownership metadata before any SQL side effects.
            // A successful mutation must never lose its acknowledgement because
            // creating the empty result wrapper afterwards exceeded SourceCpu.
            auto result = workphone::make_ptr<SQLiteQuery>();
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
                        if( executed == SQLITE_BUSY || executed == SQLITE_LOCKED )
                        {
                            WP_LOG_ERROR(
                                "SQLite writer is busy; retry after the other transaction completes "
                                "(1000 ms timeout)." );
                        }
                        else
                        {
                            WP_LOG_ERROR( sqlite3_errmsg( m_database->mpDB ) );
                        }
                        return nullptr;
                    }
                    return result;
                }
            }
            auto statement = m_database->compileStatement( sql.c_str() );
            for( size_t i = 0; i < values.size(); ++i )
            {
                if( values[i].find( '\0' ) != String::npos ) return nullptr;
                statement.bind( static_cast<int>(i + 1), values[i].c_str() );
            }
            auto query = statement.execQuery();
            result->setQuery( query );
            return result;
        }
        catch( CppSQLite3Exception &e )
        {
            WP_LOG_ERROR( e.errorMessage() );
        }
        catch( const std::bad_alloc & ) {}
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
                auto pQuery = workphone::make_ptr<SQLiteQuery>();
                auto query = m_database->execQuery( str.c_str() );
                pQuery->setQuery( query );
                return pQuery;
            }
        }
        catch( CppSQLite3Exception &e )
        {
            String message = e.errorMessage();
            WP_LOG_ERROR( message );
        }
        catch( const std::bad_alloc & ) {}
        catch( const std::exception &e ) { WP_LOG_EXCEPTION(e); }
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
