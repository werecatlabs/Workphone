#ifndef __DatabaseManager_h__
#define __DatabaseManager_h__

#include <Workphone/Interface/Database/IDatabaseManager.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{

    /** @class DatabaseManager
     * @brief Implementation of the IDatabaseManager interface for managing database operations.
     *
     * This class provides functionality for managing database connections, executing queries,
     * and handling database operations in both synchronous and asynchronous manners.
     * It supports various database operations including query execution, DML operations,
     * and database optimization.
     */
    class WPCore_API DatabaseManager : public IDatabaseManager
    {
    public:
        /** @class QueryJob
         * @brief A job class for executing database queries asynchronously.
         *
         * This class represents a job that can be executed in a separate thread
         * to perform database queries without blocking the main thread.
         */
        class QueryJob : public Job
        {
        public:
            /** @brief Default constructor */
            QueryJob();

            /** @brief Constructor with database and query parameters
             * @param database The database manager instance
             * @param query The SQL query to execute
             */
            QueryJob( SmartPtr<DatabaseManager> database, const String &query );

            /** @brief Destructor */
            ~QueryJob() override;

            /** @brief Executes the database query */
            void execute() override;

            /** @brief Gets the owner database manager
             * @return Smart pointer to the database manager
             */
            SmartPtr<DatabaseManager> getOwner() const;

            /** @brief Sets the owner database manager
             * @param owner The database manager instance
             */
            void setOwner( SmartPtr<DatabaseManager> owner );

            /** @brief Gets the query string
             * @return The SQL query string
             */
            String getQuery() const;

            /** @brief Sets the query string
             * @param query The SQL query to execute
             */
            void setQuery( const String &query );

            WP_CLASS_REGISTER_DECL;

        private:
            String m_query;                     ///< The SQL query to execute
            SmartPtr<DatabaseManager> m_owner;  ///< The owner database manager
        };

        /** @class DMLQueryJob
         * @brief A job class for executing Data Manipulation Language (DML) queries asynchronously.
         *
         * This class handles asynchronous execution of DML operations like INSERT, UPDATE, and DELETE.
         */
        class DMLQueryJob : public Job
        {
        public:
            /** @brief Default constructor */
            DMLQueryJob();

            /** @brief Constructor with database, tag, and query parameters
             * @param database The database manager instance
             * @param tag The operation tag for identification
             * @param query The DML query to execute
             */
            DMLQueryJob( SmartPtr<DatabaseManager> database, const String &tag, const String &query );

            /** @brief Destructor */
            ~DMLQueryJob() override;

            /** @brief Executes the DML query */
            void execute() override;

            /** @brief Gets the owner database manager
             * @return Smart pointer to the database manager
             */
            SmartPtr<DatabaseManager> getOwner() const;

            /** @brief Sets the owner database manager
             * @param owner The database manager instance
             */
            void setOwner( SmartPtr<DatabaseManager> owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_tag;                       ///< The operation tag
            String m_query;                     ///< The DML query to execute
            SmartPtr<DatabaseManager> m_owner;  ///< The owner database manager
        };

        /** @brief Default constructor */
        DatabaseManager();

        /** @brief Destructor */
        ~DatabaseManager() override;

        /** @brief Loads the database manager with the specified data
         * @param data The shared object containing initialization data
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @brief Unloads the database manager and releases resources
         * @param data The shared object containing cleanup data
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @brief Creates and initializes the database connection */
        void create() override;

        /** @brief Destroys the database connection and releases resources */
        void destroy() override;

        /** @brief Opens the database connection */
        void open() override;

        /** @brief Closes the database connection */
        void close() override;

        /** @brief Loads database from a handle
         * @param handle The database handle
         */
        void loadFromHandle( size_t handle ) override;

        /** @brief Loads database from a file
         * @param filePath The path to the database file
         */
        void loadFromFile( const String &filePath ) override;

        /** @brief Loads database from a wide string file path
         * @param filePath The wide string path to the database file
         */
        void loadFromFile( const StringW &filePath ) override;

        /** @brief Optimizes the database for better performance */
        void optimise() override;

        /** @brief Attaches an additional database file
         * @param filePath The path to the database file to attach
         */
        void attach( const String &filePath ) override;

        /** @brief Attaches an additional database file using wide string path
         * @param filePath The wide string path to the database file to attach
         */
        void attach( const StringW &filePath ) override;

        /** @brief Executes a database query synchronously
         * @param query The SQL query to execute
         * @return Smart pointer to the query result
         */
        SmartPtr<IDatabaseQuery> executeQuery( const String &query ) override;

        /** @brief Executes a database query asynchronously
         * @param query The SQL query to execute
         * @return Smart pointer to the query result
         */
        SmartPtr<IDatabaseQuery> executeQueryAsync( const String &query );

        /** @brief Executes a DML query synchronously
         * @param dml The DML query to execute
         * @return Status code indicating success or failure
         */
        s32 executeDML( const String &dml ) override;

        /** @brief Executes a DML query synchronously using wide string
         * @param dml The DML query to execute as wide string
         * @return Status code indicating success or failure
         */
        s32 executeDML( const StringW &dml ) override;

        /** @brief Executes a DML query asynchronously
         * @param tag The operation tag for identification
         * @param value The DML query to execute
         * @return Status code indicating success or failure
         */
        s32 executeAsyncDML( const String &tag, const String &statement ) override;

        /** @brief Executes a SQL script from a file
         * @param filePath The path to the SQL script file
         */
        void runScript( const String &filePath ) override;

        /** @brief Sets a database setting
         * @param name The setting name
         * @param valueue The setting value
         */
        void setSetting( const String &name, const String &value );

        /** @brief Sets a boolean database setting
         * @param name The setting name
         * @param bValue The boolean setting value
         */
        void setSettingAsBool( const String &name, bool bValue );

        /** @brief Sets an integer database setting
         * @param name The setting name
         * @param iValue The integer setting value
         */
        void setSettingAsInt( const String &name, s32 iValue );

        /** @brief Sets a float database setting
         * @param name The setting name
         * @param fValue The float setting value
         */
        void setSettingAsFloat( const String &name, f32 fValue );

        /** @brief Gets a database setting
         * @param name The setting name
         * @param valueue The default value if setting not found
         * @return The setting value
         */
        String getSetting( const String &name, String value = "" );

        /** @brief Gets a database setting by reference
         * @param name The setting name
         * @param valueue The default value if setting not found
         * @return The setting value
         */
        String getSettingRef( const String &name, String value = "" );

        /** @brief Gets a boolean database setting
         * @param name The setting name
         * @param defaultValue The default value if setting not found
         * @return The boolean setting value
         */
        bool getSettingAsBool( const String &name, bool defaultValue = false );

        /** @brief Gets an integer database setting
         * @param name The setting name
         * @param defaultValue The default value if setting not found
         * @return The integer setting value
         */
        s32 getSettingAsInt( const String &name, s32 defaultValue = 0 );

        /** @brief Gets a float database setting
         * @param name The setting name
         * @param defaultValue The default value if setting not found
         * @return The float setting value
         */
        f32 getSettingAsFloat( const String &name, f32 defaultValue = 0.0f );

        /** @brief Checks if a state value exists
         * @param name The state value name
         * @return True if the state value exists
         */
        bool hasStateValue( const String &name );

        /** @brief Sets a state value
         * @param name The state value name
         * @param valueue The state value
         */
        void setStateValue( const String &name, const String &value );

        /** @brief Sets a boolean state value
         * @param name The state value name
         * @param value The boolean state value
         */
        void setStateValueAsBool( const String &name, bool value );

        /** @brief Sets an integer state value
         * @param name The state value name
         * @param value The integer state value
         */
        void setStateValueAsInt( const String &name, s32 value );

        /** @brief Sets a float state value
         * @param name The state value name
         * @param value The float state value
         */
        void setStateValueAsFloat( const String &name, f32 value );

        /** @brief Gets a state value
         * @param name The state value name
         * @return The state value
         */
        String getStateValue( const String &name );

        /** @brief Gets a boolean state value
         * @param name The state value name
         * @return The boolean state value
         */
        bool getStateValueAsBool( const String &name );

        /** @brief Gets an integer state value
         * @param name The state value name
         * @return The integer state value
         */
        s32 getStateValueAsInt( const String &name );

        /** @brief Gets a float state value
         * @param name The state value name
         * @return The float state value
         */
        f32 getStateValueAsFloat( const String &name );

        /** @brief Drops all tables from the database */
        void dropAllTables();

        /** @brief Gets a resource value by name
         * @param name The resource name
         * @return The resource value
         */
        String getResourceValue( const String &name );

        /** @brief Gets a resource value by ID
         * @param id The resource ID
         * @return The resource value
         */
        String getResourceValue( s32 id );

        /** @brief Gets the database instance
         * @return Smart pointer to the database
         */
        SmartPtr<IDatabase> getDatabase() const override;

        /** @brief Sets the database instance
         * @param database The database instance
         */
        void setDatabase( SmartPtr<IDatabase> database ) override;

        /** @brief Gets the database file path
         * @return The database file path
         */
        String getDatabasePath() const override;

        /** @brief Sets the database file path
         * @param databasePath The database file path
         */
        void setDatabasePath( const String &databasePath ) override;

        /** @brief Gets the list of attached databases
         * @return Array of attached database paths
         */
        Array<String> getAttached() const override;

        /** @brief Sets the list of attached databases
         * @param attached Array of database paths to attach
         */
        void setAttached( const Array<String> &attached ) override;

        /** @brief Adds an attached database
         * @param attached The database path to attach
         */
        void addAttached( const String &attached ) override;

        void lock() override;

        bool try_lock() override;

        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        AtomicSmartPtr<IDatabase> m_database;  ///< The database instance
        AtomicObject<String> m_databasePath;   ///< The database file path
        ConcurrentArray<String> m_attached;    ///< List of attached databases
        mutable RecursiveSpinMutex m_mutex;    ///< Mutex for thread synchronization
    };
}  // namespace workphone

#endif  // Database_h__
