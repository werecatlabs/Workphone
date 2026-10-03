#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/DatabaseManager.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, DatabaseManager, IDatabaseManager );
    WP_CLASS_REGISTER_DERIVED( workphone, DatabaseManager::QueryJob, Job );
    WP_CLASS_REGISTER_DERIVED( workphone, DatabaseManager::DMLQueryJob, Job );

    DatabaseManager::DatabaseManager()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            m_database = factoryManager->make_object<IDatabase>();
            if( !m_database )
            {
                WP_LOG_ERROR( "DatabaseManager::DatabaseManager: Failed to create database" );
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    DatabaseManager::~DatabaseManager() = default;

    void DatabaseManager::create()
    {
    }

    void DatabaseManager::destroy()
    {
    }

    void DatabaseManager::open()
    {
        try
        {
            if( auto database = getDatabase() )
            {
                database->loadFromFile( m_databasePath );
            }

            for( auto db : m_attached )
            {
                attach( db );
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::close()
    {
        try
        {
            if( auto database = getDatabase() )
            {
                database->close();
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::loadFromHandle( size_t handle )
    {
        // if( auto database = getDatabase() )
        // {
        //     database->mpDB = reinterpret_cast<sqlite3*>(handle);
        //     database->setBusyTimeout(60000);
        // }
    }

    void DatabaseManager::loadFromFile( const String &filePath )
    {
        try
        {
            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            auto database = getDatabase();
            if( database )
            {
                if( !StringUtil::isNullOrEmpty( m_databasePath ) )
                {
                    if( m_databasePath != filePath )
                    {
                        database->close();
                        database->unload( nullptr );
                        setDatabase( nullptr );
                        database = nullptr;
                    }
                }
            }

            if( !database )
            {
                database = factoryManager->make_object<IDatabase>();
                setDatabase( database );
            }

            if( database )
            {
                if( !database->isLoaded() )
                {
                    if( StringUtil::isNullOrEmpty( filePath ) )
                    {
                        WP_LOG( "Cannot load empty path." );
                        return;
                    }

                    WP_LOG( "Database::load: " + filePath );

                    m_databasePath = filePath;

                    auto path = Path::getFilePath( m_databasePath );

                    if( !StringUtil::isNullOrEmpty( path ) )
                    {
                        if( !Path::isExistingFolder( path ) )
                        {
                            Path::createDirectories( path );
                        }
                    }

                    auto databasePath = getDatabasePath();
                    if( applicationManager->isEditor() )
                    {
                        auto projectPath = applicationManager->getProjectPath();
                        if( StringUtil::isNullOrEmpty( projectPath ) )
                        {
                            WP_LOG_ERROR( "DatabaseManager::loadFromFile: Project path is empty" );
                            return;
                        }

                        if( !Path::isPathAbsolute( databasePath ) )
                        {
                            databasePath = Path::getAbsolutePath( projectPath, databasePath );
                        }
                    }

                    if( Path::isExistingFile( databasePath ) )
                    {
                        WP_LOG( "Database::load: database exists at " + databasePath );
                    }

                    database->loadFromFile( databasePath );
                }
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::loadFromFile( const StringW &filePath )
    {
        try
        {
            if( auto database = getDatabase() )
            {
                if( !database->isLoaded() )
                {
                    ScopedLock lock( this );

                    WP_ASSERT( !StringUtilW::isNullOrEmpty( filePath ) );
                    WP_LOG( L"Database::load: " + filePath );

                    if( PathW::isExistingFile( filePath ) )
                    {
                        WP_LOG( L"Database::load: database exists at " + filePath );
                    }

                    m_databasePath = StringUtil::toStringC( filePath );

                    database->loadFromFile( m_databasePath );
                    setDatabase( database );
                }
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this );
            auto databaseFilePath = getDatabasePath();
            loadFromFile( databaseFilePath );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this );

            if( auto database = getDatabase() )
            {
                database->close();
                setDatabase( nullptr );
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::attach( const String &filePath )
    {
        try
        {
            ScopedLock lock( this );

            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

            WP_LOG( "Database::load: " + filePath );

            if( Path::isExistingFile( filePath ) )
            {
                WP_LOG( "Database::load: database exists at " + filePath );
            }

            executeDML( "attach database '" + filePath + "' as refdb; " );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::attach( const StringW &filePath )
    {
        try
        {
            ScopedLock lock( this );

            WP_ASSERT( !StringUtilW::isNullOrEmpty( filePath ) );
            WP_LOG( L"Database::load: " + filePath );

            if( PathW::isExistingFile( filePath ) )
            {
                WP_LOG( L"Database::load: database exists at " + filePath );
            }

            executeDML( L"attach database '" + filePath + L"' as 'refdb'; " );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IDatabaseQuery> DatabaseManager::executeQuery( const String &queryStr )
    {
        try
        {
            if( auto db = getDatabase() )
            {
                return db->query( queryStr );
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IDatabaseQuery> DatabaseManager::executeQueryAsync( const String &queryStr )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto jobQueue = applicationManager->getJobQueue();
            WP_ASSERT( jobQueue );

            auto job = workphone::make_ptr<QueryJob>( this, queryStr );
            jobQueue->addJob( job );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    int DatabaseManager::executeDML( const String &dml )
    {
        try
        {
            ScopedLock lock( this );
            WP_LOG( "SQL: " + dml );
            if( auto database = getDatabase() )
            {
                database->queryDML( dml );
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return 0;
    }

    int DatabaseManager::executeDML( const StringW &dml )
    {
        try
        {
            ScopedLock lock( this );
            WP_LOG( "SQL: " + StringUtil::toStringC( dml ) );
            if( auto database = getDatabase() )
            {
                database->queryDML( dml );
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return 0;
    }

    int DatabaseManager::executeAsyncDML( const String &tag, const String &statement )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto jobQueue = applicationManager->getJobQueue();

            SmartPtr<DMLQueryJob> job( new DMLQueryJob( this, tag, statement ) );
            jobQueue->addJob( job );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return 0;
    }

    void DatabaseManager::runScript( const String &filePath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();
            auto stream = fileSystem->open( filePath, true, false, false, true, true );
            if( stream )
            {
                std::stringstream sqlBuffer;

                while( !stream->eof() )
                {
                    auto line = StringUtil::trim( stream->getLine() );
                    if( line.empty() || line.substr( 0, 2 ) == "--" )
                    {
                        continue;
                    }

                    // Append the line to the buffer
                    sqlBuffer << line << " ";

                    if( line.back() == ';' )
                    {
                        auto sql = sqlBuffer.str();
                        executeDML( sql.c_str() );

                        sqlBuffer.str( "" );
                        sqlBuffer.clear();
                    }
                }
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DatabaseManager::setSetting( const String &name, const String &value )
    {
        ScopedLock lock( this );

        auto sql = String( "select * from settings where param = '" ) + name + "'";
        auto queryResult = executeQuery( sql );
        auto hasSetting = !queryResult->eof();
        if( hasSetting )
        {
            auto insertSql = "update settings set value='" + value + "' where param = '" + name + "'";
            executeQuery( insertSql );
        }
        else
        {
            auto insertSql =
                "insert into settings(param, value) VALUES('" + name + "', '" + value + "')";
            executeQuery( insertSql );
        }
    }

    void DatabaseManager::setSettingAsBool( const String &name, bool bValue )
    {
        auto value = bValue ? "true" : "false";
        setSetting( name, value );
    }

    void DatabaseManager::setSettingAsInt( const String &name, s32 iValue )
    {
        auto value = StringUtil::toString( iValue );
        setSetting( name, value );
    }

    void DatabaseManager::setSettingAsFloat( const String &name, f32 fValue )
    {
        auto value = StringUtil::toString( fValue );
        setSetting( name, value );
    }

    String DatabaseManager::getSetting( const String &name, String value /*= ""*/ )
    {
        auto sql = "select * from settings where param = '" + name + "'";
        auto queryResult = executeQuery( sql );
        while( !queryResult->eof() )
        {
            value = queryResult->getFieldValue( "value" );
            queryResult->nextRow();
        }

        return value;
    }

    String DatabaseManager::getSettingRef( const String &name, String value /*= ""*/ )
    {
        auto sql = "select * from refdb.settings where param = '" + name + "'";
        auto queryResult = executeQuery( sql );
        while( !queryResult->eof() )
        {
            value = queryResult->getFieldValue( "value" );
            queryResult->nextRow();
        }

        return value;
    }

    s32 DatabaseManager::getSettingAsInt( const String &name, s32 defaultValue /*= 0*/ )
    {
        auto val = getSetting( name );
        if( !StringUtil::isNullOrEmpty( val ) )
        {
            return StringUtil::parseInt( val );
        }

        return defaultValue;
    }

    bool DatabaseManager::getSettingAsBool( const String &name, bool defaultValue /*= false*/ )
    {
        auto val = getSetting( name );
        if( !StringUtil::isNullOrEmpty( val ) )
        {
            return StringUtil::parseBool( val );
        }

        return defaultValue;
    }

    f32 DatabaseManager::getSettingAsFloat( const String &name, f32 defaultValue /*= 0.0f*/ )
    {
        auto val = getSetting( name );
        if( !StringUtil::isNullOrEmpty( val ) )
        {
            return StringUtil::parseFloat( val );
        }

        return defaultValue;
    }

    bool DatabaseManager::hasStateValue( const String &name )
    {
        auto sql = "select * from sim_states where param = '" + name + "';";
        auto query = executeQuery( sql );
        if( query && !query->eof() )
        {
            return true;
        }

        return false;
    }

    void DatabaseManager::setStateValue( const String &name, const String &value )
    {
        if( hasStateValue( name ) )
        {
            auto sql = "update sim_states set value = '" + value + "' where param = '" + name + "';";
            executeQuery( sql );
        }
        else
        {
            auto sql = "INSERT INTO sim_states(param, value) VALUES('" + name + "', '" + value + "');";
            executeQuery( sql );
        }
    }

    void DatabaseManager::setStateValueAsBool( const String &name, bool value )
    {
        setStateValue( name, StringUtil::toString( value ) );
    }

    void DatabaseManager::setStateValueAsInt( const String &name, s32 value )
    {
        setStateValue( name, StringUtil::toString( value ) );
    }

    void DatabaseManager::setStateValueAsFloat( const String &name, f32 value )
    {
        setStateValue( name, StringUtil::toString( value ) );
    }

    String DatabaseManager::getStateValue( const String &name )
    {
        auto sql = "select * from sim_states where param = '" + name + "';";
        auto query = executeQuery( sql );
        if( query && !query->eof() )
        {
            return query->getFieldValue( "value" );
        }

        return {};
    }

    bool DatabaseManager::getStateValueAsBool( const String &name )
    {
        auto sql = "select * from sim_states where param = '" + name + "';";
        auto query = executeQuery( sql );
        if( query && !query->eof() )
        {
            return StringUtil::parseBool( query->getFieldValue( "value" ), false );
        }

        return false;
    }

    s32 DatabaseManager::getStateValueAsInt( const String &name )
    {
        auto sql = "select * from sim_states where param = '" + name + "';";
        auto query = executeQuery( sql );
        if( query && !query->eof() )
        {
            return StringUtil::parseInt( query->getFieldValue( "value" ), 0 );
        }

        return 0;
    }

    f32 DatabaseManager::getStateValueAsFloat( const String &name )
    {
        auto sql = "select * from sim_states where param = '" + name + "';";
        auto query = executeQuery( sql );
        if( query && !query->eof() )
        {
            return StringUtil::parseFloat( query->getFieldValue( "value" ), 0.0f );
        }

        return 0.0f;
    }

    void DatabaseManager::optimise()
    {
        ScopedLock lock( this );

        executeQuery( "PRAGMA foreign_keys = ON;" );
        executeQuery( "PRAGMA count_changes = OFF;" );
        executeQuery( "PRAGMA synchronous=OFF;" );
        // executeQuery("PRAGMA page_size = 8192;");
        executeQuery( "PRAGMA cache_size = 10000;" );
        executeQuery( "PRAGMA journal_mode=memory;" );
        // executeQuery("PRAGMA locking_mode=EXCLUSIVE;");
        executeQuery( "PRAGMA temp_store=MEMORY;" );
        // executeQuery("PRAGMA threads = 4;");
    }

    DatabaseManager::QueryJob::QueryJob( SmartPtr<DatabaseManager> database, const String &query ) :
        m_query( query ),
        m_owner( std::move( database ) )
    {
    }

    DatabaseManager::QueryJob::QueryJob() = default;

    DatabaseManager::QueryJob::~QueryJob() = default;

    void DatabaseManager::QueryJob::execute()
    {
        try
        {
            // if( auto database = getOwner() )
            // {
            //     database->executeQuery(m_query);
            // }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<DatabaseManager> DatabaseManager::QueryJob::getOwner() const
    {
        return m_owner;
    }

    void DatabaseManager::QueryJob::setOwner( SmartPtr<DatabaseManager> owner )
    {
        m_owner = owner;
    }

    String DatabaseManager::QueryJob::getQuery() const
    {
        return m_query;
    }

    void DatabaseManager::QueryJob::setQuery( const String &query )
    {
        m_query = query;
    }

    DatabaseManager::DMLQueryJob::DMLQueryJob( SmartPtr<DatabaseManager> database, const String &tag,
                                               const String &query ) :
        m_tag( tag ),
        m_query( query ),
        m_owner( std::move( database ) )
    {
    }

    DatabaseManager::DMLQueryJob::DMLQueryJob() = default;

    DatabaseManager::DMLQueryJob::~DMLQueryJob() = default;

    void DatabaseManager::DMLQueryJob::execute()
    {
        // try
        //{
        //	if( auto database = getOwner() )
        //	{
        //		database->executeDML(m_query);
        //	}

        //	auto applicationManager = core::IApplicationManager::instance();
        //	auto pluginInterface = applicationManager->getPluginInterface();

        //	auto pluginEvent = fb::make_ptr<core::PluginEvent>();
        //	pluginEvent->setArg1(m_tag);

        //	static const String sQueryResult = "queryResult";
        //	pluginInterface->sendEvent(sQueryResult, pluginEvent);
        //}
        // catch (Exception& e)
        //{
        //	WP_LOG_EXCEPTION(e);
        //}
    }

    SmartPtr<DatabaseManager> DatabaseManager::DMLQueryJob::getOwner() const
    {
        return m_owner;
    }

    void DatabaseManager::DMLQueryJob::setOwner( SmartPtr<DatabaseManager> owner )
    {
        m_owner = owner;
    }

    void DatabaseManager::dropAllTables()
    {
        auto cursor = executeQuery( "SELECT name FROM sqlite_master WHERE type='table'" );
        while( !cursor->eof() )
        {
            auto name = cursor->getFieldValue( "name" );
            executeQuery( "DROP TABLE IF EXISTS " + name );
            cursor->nextRow();
        }
    }

    String DatabaseManager::getResourceValue( s32 id )
    {
        auto parent_id = -1;

#if WP_BUILD_EDITOR_PLUGIN
        auto modelSql = "Select * From configured_actors where id = " + StringUtil::toString( id );
#else
        auto modelSql = "Select * From refdb.configured_actors where id = " + StringUtil::toString( id );
#endif

        auto modelResult = executeQuery( modelSql );
        if( modelResult )
        {
            if( !modelResult->eof() )
            {
                return modelResult->getFieldValue( "resource_name" );
            }
        }

        return "";
    }

    String DatabaseManager::getResourceValue( const String &name )
    {
#if WP_BUILD_EDITOR_PLUGIN
        auto modelSql = "select * from resourcemap where name = '" + name + "'";
#else
        auto modelSql = "select * from refdb.resourcemap where name = '" + name + "'";
#endif

        auto modelResult = executeQuery( modelSql );
        if( modelResult )
        {
            if( !modelResult->eof() )
            {
                return modelResult->getFieldValue( "name" );
            }
        }

        return name;
    }

    SmartPtr<IDatabase> DatabaseManager::getDatabase() const
    {
        return m_database;
    }

    void DatabaseManager::setDatabase( SmartPtr<IDatabase> database )
    {
        m_database = database;
    }

    void DatabaseManager::setDatabasePath( const String &databasePath )
    {
        m_databasePath = databasePath;
    }

    Array<String> DatabaseManager::getAttached() const
    {
        return m_attached.snapshot();
    }

    void DatabaseManager::setAttached( const Array<String> &attached )
    {
        m_attached = { attached.begin(), attached.end() };
    }

    void DatabaseManager::addAttached( const String &attached )
    {
        m_attached.push_back( attached );
    }

    void DatabaseManager::lock()
    {
        m_mutex.lock();
    }

    bool DatabaseManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void DatabaseManager::unlock()
    {
        m_mutex.unlock();
    }

    String DatabaseManager::getDatabasePath() const
    {
        return m_databasePath;
    }
}  // namespace workphone
