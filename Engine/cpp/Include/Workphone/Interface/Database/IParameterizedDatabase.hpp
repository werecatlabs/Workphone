#ifndef WORKPHONE_PARAMETERIZED_DATABASE_HPP
#define WORKPHONE_PARAMETERIZED_DATABASE_HPP
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>

namespace workphone
{
    /** Optional connection-wide transaction guard. Recursive on the owning thread.
     * Every query/open/close path of an implementing backend uses the same guard. */
    class ISerializedDatabase
    {
    public:
        virtual ~ISerializedDatabase() = default;
        virtual void lockConnection() = 0;
        virtual void unlockConnection() = 0;
    };

    class DatabaseConnectionLock final
    {
    public:
        explicit DatabaseConnectionLock( ISerializedDatabase *database ) : m_database(database)
        {
            if( m_database ) m_database->lockConnection();
        }
        ~DatabaseConnectionLock()
        {
            if( m_database ) m_database->unlockConnection();
        }
        DatabaseConnectionLock( const DatabaseConnectionLock & ) = delete;
        DatabaseConnectionLock &operator=( const DatabaseConnectionLock & ) = delete;
    private:
        ISerializedDatabase *m_database;
    };

    /** Consistent independent snapshot, including committed WAL content.
     * Never overwrites an existing destination. */
    class IBackupDatabase
    {
    public:
        virtual ~IBackupDatabase() = default;
        virtual bool backupTo( const String &path, String &error ) = 0;
    };

    /** Optional database extension. Leaves the existing IDatabase ABI unchanged.
     * SQL uses positional ? parameters. A null result denotes failure, including
     * for mutations; a successful mutation returns an empty result set. */
    class IParameterizedDatabase
    {
    public:
        virtual ~IParameterizedDatabase() = default;
        virtual SmartPtr<IDatabaseQuery> queryBound( const String &sql,
                                                     const Array<String> &values ) = 0;
    };
}  // namespace workphone
#endif
