#ifndef IDatabase_H
#define IDatabase_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for a database.
     *
     * Provides a common interface for working with databases.
     *
     * To load a database, use `loadFromFile()` and pass the file path. You can also specify an
     * encryption key to use while opening the database by passing the key string as a second parameter
     * to `loadFromFile()`.
     *
     * After opening a database, you can perform queries using `query()` or execute DML commands using
     * `queryDML()`.
     *
     * To close the database and release any resources, call `close()`.
     *
     */
    class WPCore_API IDatabase : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IDatabase() override;

        /**
         * Load a database file and open it for querying.
         * @param filePath The path to the database file to load.
         */
        virtual void loadFromFile( const String &filePath ) = 0;

        /**
         * Load a database file and open it for querying with the specified encryption key.
         *
         * @param filePath The path to the database file to load.
         * @param key The encryption key to use to open the database file.
         */
        virtual void loadFromFile( const String &filePath, const String &key ) = 0;

        /**
         * Close the database file and release any resources.
         */
        virtual void close() = 0;

        /**
         * Set the encryption key for the loaded database.
         *
         * @param key The encryption key to set.
         */
        virtual void setKey( const String &key ) = 0;

        /**
         * Query the database using the specified SQL query.
         *
         * @param queryStr The SQL query to execute.
         * @return A shared pointer to the resulting query object.
         */
        virtual SmartPtr<IDatabaseQuery> query( const String &queryStr ) = 0;

        /**
         * Query the database using the specified wide character SQL query.
         *
         * @param queryStr The SQL query to execute.
         * @return A shared pointer to the resulting query object.
         */
        virtual SmartPtr<IDatabaseQuery> query( const StringW &queryStr ) = 0;

        /**
         * Execute the specified DML (Data Manipulation Language) command on the database.
         *
         * @param queryStr The DML command to execute.
         */
        virtual void queryDML( const String &queryStr ) = 0;

        /**
         * Execute the specified wide character DML (Data Manipulation Language) command on the database.
         *
         * @param queryStr The DML command to execute.
         */
        virtual void queryDML( const StringW &queryStr ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IDatabase_h__
