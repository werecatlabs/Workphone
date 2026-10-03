#ifndef WPAssimp_h__
#define WPAssimp_h__

#include <WPAssimp/WPAssimpPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    class WPAssimp_API WPAssimp : public ISharedObject
    {
    public:
        WPAssimp();
        ~WPAssimp() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        static SmartPtr<WPAssimp> instance();
        static void setInstance( SmartPtr<WPAssimp> plugin );

    protected:
        /// Pointer to the log stream for import messages.
        LogStream *m_logStream = nullptr;

        static SmartPtr<WPAssimp> m_sPlugin;
    };
}  // namespace workphone

#endif  // WPAssimp_h__
