#ifndef WPResourceCompilationDatabase_h__
#define WPResourceCompilationDatabase_h__

#include <WPSQLite/WPSQLitePrerequisites.hpp>
#include <Workphone/Interface/Database/IResourceCompilationDatabase.hpp>

namespace workphone::resource
{
    /** SQLite implementation of the resource-compilation metadata database. */
    class WPSQLite_API ResourceCompilationDatabase final :
        public IResourceCompilationDatabase
    {
    public:
        ResourceCompilationDatabase();
        ~ResourceCompilationDatabase() override;

        ResourceCompilationDatabase( const ResourceCompilationDatabase & ) = delete;
        ResourceCompilationDatabase &operator=( const ResourceCompilationDatabase & ) = delete;

        bool connect( const String &databasePath ) override;
        void disconnect() override;
        bool isConnected() const override;

        String getLastError() const override;
        bool reset() override;

        bool getRecord( const String &resourceId,
                        CompiledResourceRecord &record ) const override;
        bool commitCompilation(
            const CompiledResourceRecord &record,
            const Array<CompileDependencyRecord> &dependencies ) override;
        bool removeRecord( const String &resourceId ) override;

        bool getDependencies(
            const String &resourceId,
            Array<CompileDependencyRecord> &dependencies ) const override;
        bool getDependents( const String &dependencyPath,
                            Array<String> &resourceIds ) const override;

    private:
        class Impl;
        Impl *m_impl = nullptr;
    };
}  // namespace workphone::resource

#endif  // WPResourceCompilationDatabase_h__
