#ifndef __WPGraphicsOgreNext__H
#define __WPGraphicsOgreNext__H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextConfig.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextAutoLink.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    namespace render
    {

        /** Manages the ogre-next graphics system. */
        class WPGraphicsOgreNext_API WPGraphicsOgreNext : public ISharedObject
        {
        public:
            /** Constructor. */
            WPGraphicsOgreNext();

            /** Destructor. */
            ~WPGraphicsOgreNext() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** Gets the instance of the plugin. */
            static SmartPtr<WPGraphicsOgreNext> instance();

            /** Sets the instance of the plugin. */
            static void setInstance( SmartPtr<WPGraphicsOgreNext> plugin );

            /** Gets the factory manager. */
            static SmartPtr<IFactoryManager> getFactoryManager();

            /** Sets the factory manager. */
            static void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

        protected:
            /** The instance of the plugin. */
            static SmartPtr<WPGraphicsOgreNext> m_sPlugin;

            /** The factory manager. */
            static SmartPtr<IFactoryManager> m_factoryManager;
        };

    }  // end namespace render
}  // namespace workphone

#endif
