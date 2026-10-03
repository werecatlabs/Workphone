#ifndef WPSQLite_h__
#define WPSQLite_h__

#include <WPSQLite/WPSQLitePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    class WPSQLite_API SQLitePlugin : public ISharedObject
    {
    public:
        SQLitePlugin();
        ~SQLitePlugin() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        static SmartPtr<SQLitePlugin> instance();
        static void setInstance( SmartPtr<SQLitePlugin> plugin );

    protected:
        static SmartPtr<SQLitePlugin> m_sPlugin;
    };
}  // namespace workphone

#endif  // WPSQLite_h__
