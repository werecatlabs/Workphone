#ifndef IDatabaseManager_h__
#define IDatabaseManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Interface for managing database operations and connections.
     *
     * The IDatabaseManager interface provides a comprehensive set of methods for managing database
     * operations, including creation, destruction, connection management, query execution, and database
     * optimization. It serves as a high-level abstraction for database interactions, supporting both
     * synchronous and asynchronous operations.
     *
     * @note This interface inherits from ISharedObject for memory management.
     * @see ISharedObject
     */
    class WPCore_API IDatabaseManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of database resources when the manager is destroyed.
         */
        ~IDatabaseManager() override;

        /**
         * @brief Creates a new database instance.
         *
         * Initializes a new database with default settings. This method should be called
         * before any other database operations.
         */
        virtual void create() = 0;

        /**
         * @brief Destroys the current database instance.
         *
         * Performs cleanup operations and releases all database resources.
         * Should be called when the database is no longer needed.
         */
        virtual void destroy() = 0;

        /**
         * @brief Opens a connection to the database.
         *
         * Establishes a connection to the database for subsequent operations.
         * Must be called before executing queries or other database operations.
         */
        virtual void open() = 0;

        /**
         * @brief Closes the current database connection.
         *
         * Safely terminates the connection to the database and releases associated resources.
         */
        virtual void close() = 0;

        /**
         * @brief Loads a database from a handle.
         * @param handle The handle to load the database from.
         * @throw std::runtime_error if the handle is invalid or loading fails.
         */
        virtual void loadFromHandle( size_t handle ) = 0;

        /**
         * @brief Loads a database from a file path.
         * @param filePath The file path to load the database from.
         * @throw std::runtime_error if the file doesn't exist or is invalid.
         */
        virtual void loadFromFile( const String &filePath ) = 0;

        /**
         * @brief Loads a database from a file path.
         * @param filePath The file path to load the database from.
         */
        virtual void loadFromFile( const StringW &filePath ) = 0;

        /**
         * @brief Attaches an additional database file to the current connection.
         * @param filePath The file path of the database to attach.
         * @throw std::runtime_error if the attachment fails.
         */
        virtual void attach( const String &filePath ) = 0;

        /**
         * @brief Attaches an additional database file using a wide string path.
         * @param filePath The wide string file path of the database to attach.
         * @throw std::runtime_error if the attachment fails.
         */
        virtual void attach( const StringW &filePath ) = 0;

        /**
         * @brief Executes a SQL query on the database.
         * @param query The SQL query string to execute.
         * @return SmartPtr<IDatabaseQuery> A smart pointer to the query result.
         * @throw std::runtime_error if the query execution fails.
         */
        virtual SmartPtr<IDatabaseQuery> executeQuery( const String &query ) = 0;

        /**
         * @brief Executes a Data Manipulation Language (DML) query.
         * @param dml The DML query string to execute.
         * @return s32 The number of affected rows or -1 if the operation fails.
         */
        virtual s32 executeDML( const String &dml ) = 0;

        /**
         * @brief Executes a DML query using a wide string.
         * @param dml The wide string DML query to execute.
         * @return s32 The number of affected rows or -1 if the operation fails.
         */
        virtual s32 executeDML( const StringW &dml ) = 0;

        /**
         * @brief Executes a DML query asynchronously.
         * @param tag A unique identifier for the async operation.
         * @param dml The DML query to execute asynchronously.
         * @return s32 A status code indicating the success or failure of the operation.
         */
        virtual s32 executeAsyncDML( const String &tag, const String &dml ) = 0;

        /**
         * @brief Executes a SQL script from a file.
         * @param filePath The path to the SQL script file.
         * @throw std::runtime_error if the script execution fails.
         */
        virtual void runScript( const String &filePath ) = 0;

        /**
         * @brief Gets the current database instance.
         * @return SmartPtr<IDatabase> A smart pointer to the current database.
         */
        virtual SmartPtr<IDatabase> getDatabase() const = 0;

        /**
         * @brief Sets the current database instance.
         * @param database The database instance to set as current.
         */
        virtual void setDatabase( SmartPtr<IDatabase> database ) = 0;

        /**
         * @brief Optimizes the database for better performance.
         *
         * Performs maintenance operations such as index rebuilding, statistics updates,
         * and space reclamation.
         */
        virtual void optimise() = 0;

        /**
         * @brief Gets the list of attached databases.
         * @return Array<String> An array of file paths for attached databases.
         */
        virtual Array<String> getAttached() const = 0;

        /**
         * @brief Sets the list of attached databases.
         * @param attached An array of database file paths to attach.
         */
        virtual void setAttached( const Array<String> &attached ) = 0;

        /**
         * @brief Adds a new database to the list of attached databases.
         * @param attached The file path of the database to attach.
         */
        virtual void addAttached( const String &attached ) = 0;

        /**
         * @brief Gets the path of the current database.
         * @return String The file path of the current database.
         */
        virtual String getDatabasePath() const = 0;

        /**
         * @brief Sets the path of the current database.
         * @param databasePath The new file path for the database.
         */
        virtual void setDatabasePath( const String &databasePath ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IDatabaseManager_h__
