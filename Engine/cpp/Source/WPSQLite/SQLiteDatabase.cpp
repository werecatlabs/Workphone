#include <WPSQLite/SQLiteDatabase.hpp>
#include <WPSQLite/extern/CppSQLite3.hpp>
#include <WPSQLite/SQLiteQuery.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <new>
#include <string_view>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SQLiteDatabase, IDatabase );

    namespace
    {
        constexpr int maximumCatalogSearchBytes = 1024;

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
            return true;
        }

        struct CatalogNeedle
        {
            std::string_view text;
            size_t asciiPrefix = 0;
            bool valid = false;
        };

        struct CachedCatalogNeedle
        {
            char bytes[maximumCatalogSearchBytes];
            CatalogNeedle needle;

            explicit CachedCatalogNeedle( const CatalogNeedle &source ) noexcept : needle( source )
            {
                if( !source.text.empty() )
                {
                    std::memcpy( bytes, source.text.data(), source.text.size() );
                }
                needle.text = std::string_view( bytes, source.text.size() );
            }
        };

        enum class CatalogMatch { No, Yes, UseGlob };

        CatalogMatch matchCatalogField( sqlite3_value *argument, int limit,
                                        const CatalogNeedle &needle ) noexcept
        {
            std::string_view text;
            if( !catalogSearchArgument( argument, limit, text ) )
            {
                return CatalogMatch::UseGlob;
            }
            // SQLite's decoder can replace malformed sequences with U+FFFD,
            // but it cannot create ASCII. A missing literal ASCII prefix
            // rules out a match even before checking the field's encoding.
            if( needle.asciiPrefix &&
                text.find( needle.text.substr( 0, needle.asciiPrefix ) ) == std::string_view::npos )
            {
                return CatalogMatch::No;
            }
            if( !globComparableUtf8( text ) )
            {
                return CatalogMatch::UseGlob;
            }
            return text.find( needle.text ) == std::string_view::npos ? CatalogMatch::No : CatalogMatch::Yes;
        }

        void matchCatalogRow( sqlite3_context *context, sqlite3_value **arguments,
                              const CatalogNeedle &needle ) noexcept
        {
            if( !needle.valid )
            {
                sqlite3_result_null( context );
                return;
            }
            const auto path = matchCatalogField( arguments[0], maximumCatalogSearchBytes, needle );
            if( path == CatalogMatch::Yes )
            {
                sqlite3_result_int( context, 1 );
                return;
            }
            const auto uuid = matchCatalogField( arguments[1], 256, needle );
            if( uuid == CatalogMatch::Yes )
            {
                sqlite3_result_int( context, 1 );
            }
            else if( path == CatalogMatch::UseGlob || uuid == CatalogMatch::UseGlob )
            {
                sqlite3_result_null( context );
            }
            else
            {
                sqlite3_result_int( context, 0 );
            }
        }

        void catalogLiteralContains( sqlite3_context *context, int count,
                                     sqlite3_value **arguments ) noexcept
        {
            if( count != 3 )
            {
                sqlite3_result_null( context );
                return;
            }
            if( auto *cached = static_cast<CachedCatalogNeedle *>( sqlite3_get_auxdata( context, 2 ) ) )
            {
                matchCatalogRow( context, arguments, cached->needle );
                return;
            }

            CatalogNeedle needle;
            needle.valid = catalogSearchArgument( arguments[2], maximumCatalogSearchBytes, needle.text ) &&
                           globComparableUtf8( needle.text );
            if( needle.valid )
            {
                while( needle.asciiPrefix < needle.text.size() &&
                       static_cast<unsigned char>( needle.text[needle.asciiPrefix] ) < 0x80 )
                {
                    ++needle.asciiPrefix;
                }
            }
            matchCatalogRow( context, arguments, needle );

            // The statement owns its bound term. SQLite can drop auxiliary
            // data immediately or when an argument changes, so use it only on
            // subsequent callbacks that obtain it from the statement again.
            if( auto *cached = new( std::nothrow ) CachedCatalogNeedle( needle ) )
            {
                sqlite3_set_auxdata( context, 2, cached,
                    []( void *data ) { delete static_cast<CachedCatalogNeedle *>( data ); } );
            }
        }

        void registerCatalogSearch( sqlite3 *database ) noexcept
        {
            // Optional acceleration. The catalog can still query with GLOB if
            // the backend cannot register this function.
            (void)sqlite3_create_function( database, "wp_catalog_contains", 3, SQLITE_UTF8, nullptr,
                                           catalogLiteralContains, nullptr, nullptr );
        }

        sqlite3_int64 readCachePragma( sqlite3 *database, const char *sql ) noexcept
        {
            sqlite3_stmt *statement = nullptr;
            const auto prepared = sqlite3_prepare_v2( database, sql, -1, &statement, nullptr );
            const auto value = prepared == SQLITE_OK && sqlite3_step( statement ) == SQLITE_ROW
                                   ? sqlite3_column_int64( statement, 0 ) : -1;
            sqlite3_finalize( statement );
            return value;
        }
    }

    void SQLiteDatabase::prepareCatalogBrowseCache() noexcept
    {
        try
        {
            std::lock_guard<std::recursive_mutex> lock( m_boundQueryMutex );
            if( m_catalogBrowseCache || !isLoaded() || !m_database || !m_database->mpDB )
                return;

            auto *database = m_database->mpDB;
            const auto pageSize = readCachePragma( database, "PRAGMA page_size" );
            if( pageSize < 512 || pageSize > 65536 )
                return;
            const auto pageCount = readCachePragma( database, "PRAGMA page_count" );
            constexpr sqlite3_int64 minimumDatabaseBytes = 64ll << 20;
            if( pageCount < minimumDatabaseBytes / pageSize )
                return;

            constexpr sqlite3_int64 allowance = 24ll << 20;
            // Allow space for SQLite's page headers, cache bookkeeping and allocator overhead.
            const auto bytesPerPage = pageSize + 512;
            const auto pages = static_cast<int>( allowance / bytesPerPage );
            const auto current = readCachePragma( database, "PRAGMA cache_size" );
            if( current <= 0 || current >= pages )
                return;

            auto budget = WorldStreamingScheduler::reserveSharedSource( pages * bytesPerPage );
            if( !budget )
                return;

            // This setting applies only to this open connection.
            char sql[48];
            std::snprintf( sql, sizeof( sql ), "PRAGMA cache_size=%d", pages );
            if( sqlite3_exec( database, sql, nullptr, nullptr, nullptr ) == SQLITE_OK )
                m_catalogBrowseCache = std::move( budget );
        }
        catch( ... )
        {
            // Cache tuning is optional. The current connection remains usable.
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
                    m_database->close();
                    m_catalogBrowseCache.reset();
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
            m_database->close();
            m_catalogBrowseCache.reset();
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
        m_catalogBrowseCache.reset();
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
