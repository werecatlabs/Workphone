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

        bool getRecord( const String &resourceId, const CompilationVariant &variant,
                        CompiledResourceRecord &record ) const override;
        bool commitCompilation( const CompiledResourceRecord &record, const CompilationVariant &variant,
                                const Array<CompileDependencyRecord> &dependencies ) override;
        bool removeRecord( const String &resourceId, const CompilationVariant &variant ) override;
        bool getDependencies( const String &resourceId, const CompilationVariant &variant,
                              Array<CompileDependencyRecord> &dependencies ) const override;
        bool getDependents( const String &path, const CompilationVariant &variant,
                            Array<String> &resourceIds ) const override;

        bool commitCompilations( const Array<CompilationCommit> &compilations ) override;

    private:
        bool writeCompilation( const CompiledResourceRecord &record, const CompilationVariant &variant,
                               const Array<CompileDependencyRecord> &dependencies );
        bool queryRecord( const String &resourceId, const CompilationVariant *variant,
                          CompiledResourceRecord &record ) const;
        bool eraseRecord( const String &resourceId, const CompilationVariant *variant );
        bool queryDependencies( const String &resourceId, const CompilationVariant *variant,
                                Array<CompileDependencyRecord> &dependencies ) const;
        bool queryDependents( const String &path, const CompilationVariant *variant,
                              Array<String> &resourceIds ) const;

        class Impl;
        Impl *m_impl = nullptr;
    };
}  // namespace workphone::resource

#endif  // WPResourceCompilationDatabase_h__
