#ifndef __WPFMODSound__H
#define __WPFMODSound__H

#include <WPFMODStudio/WPFMODStudioPrerequisites.hpp>
#include <WPFMODStudio/WPFMODStudioAutolink.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @class WPFMODStudio
     * @brief This class is used to initialise the library and plugin.
     */
    class WPFMODStudio_API WPFMODStudio : public ISharedObject
    {
    public:
        /** Constructor. */
        WPFMODStudio();

        /** Destructor. */
        ~WPFMODStudio() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** Gets the plugin instance. */
        static SmartPtr<WPFMODStudio> instance();

        /** Sets the plugin instance. */
        static void setInstance( SmartPtr<WPFMODStudio> plugin );

        static SmartPtr<IFactoryManager> getFactoryManager();
        static void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

    protected:
        static SmartPtr<WPFMODStudio> m_sPlugin;
        static SmartPtr<IFactoryManager> m_factoryManager;
    };
}  // namespace workphone

#endif
