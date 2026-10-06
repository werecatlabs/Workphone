#include <WPSQLite/SQLiteDatabase.hpp>
#include <WPSQLite/extern/CppSQLite3.hpp>
#include <WPSQLite/SQLiteQuery.hpp>
#include <Workphone/Workphone.hpp>

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
        try
        {
            setLoadingState( LoadingState::Loading );

            WP_LOG( "Load database: " + filePath );

            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

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
        if( !StringUtil::isNullOrEmpty( key ) )
        {
            sqlite3_rekey( m_database->mpDB, key.c_str(), (int)key.size() );
        }
    }

    SmartPtr<IDatabaseQuery> SQLiteDatabase::query( const String &queryStr )
    {
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
            std::lock_guard<std::mutex> lock( m_boundQueryMutex );
            if( !m_database || !isLoaded() || sql.empty() ) return nullptr;
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
        std::lock_guard<std::mutex> lock( m_boundQueryMutex );
        if( m_database )
        {
            m_database->close();
        }
        setLoadingState( LoadingState::Unloaded );
    }
}  // namespace workphone
