#ifndef WPSQLiteManager_h__
#define WPSQLiteManager_h__

#include <WPSQLite/WPSQLitePrerequisites.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Memory/SharedPtr.hpp>

namespace workphone
{

    /**
     * @class SQLiteDatabase
     * @brief Implementation of IDatabase interface for SQLite database operations
     *
     * This class provides a wrapper around SQLite database functionality, implementing
     * the IDatabase interface. It handles database connections, queries, and DML operations
     * for SQLite databases.
     */
    class SQLiteDatabase : public IDatabase
    {
    public:
        /**
         * @brief Default constructor
         */
        SQLiteDatabase();

        /**
         * @brief Destructor
         * Ensures proper cleanup of database resources
         */
        ~SQLiteDatabase() override;

        /**
         * @brief Unloads database resources
         * @param data Smart pointer to shared object containing data to unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Loads database from a file
         * @param filePath Path to the SQLite database file
         */
        void loadFromFile( const String &filePath ) override;

        /**
         * @brief Loads database from a file with encryption key
         * @param filePath Path to the SQLite database file
         * @param key Encryption key for the database
         */
        void loadFromFile( const String &filePath, const String &key ) override;

        /**
         * @brief Sets the encryption key for the database
         * @param key The encryption key to use
         */
        void setKey( const String &key ) override;

        /**
         * @brief Executes a query and returns a query result object
         * @param queryStr The SQL query string to execute
         * @return SmartPtr<IDatabaseQuery> containing the query results
         */
        SmartPtr<IDatabaseQuery> query( const String &queryStr ) override;

        /**
         * @brief Executes a wide-character query and returns a query result object
         * @param queryStr The SQL query string in wide-character format
         * @return SmartPtr<IDatabaseQuery> containing the query results
         */
        SmartPtr<IDatabaseQuery> query( const StringW &queryStr ) override;

        /**
         * @brief Executes a Data Manipulation Language (DML) query
         * @param queryStr The DML query string to execute
         */
        void queryDML( const String &queryStr ) override;

        /**
         * @brief Executes a wide-character Data Manipulation Language (DML) query
         * @param queryStr The DML query string in wide-character format
         */
        void queryDML( const StringW &queryStr ) override;

        /**
         * @brief Closes the database connection
         * Ensures proper cleanup of database resources and connection
         */
        void close() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Internal SQLite database handle
        SharedPtr<CppSQLite3DB> m_database;
    };
}  // namespace workphone

#endif  // WPSQLiteManager_h__
