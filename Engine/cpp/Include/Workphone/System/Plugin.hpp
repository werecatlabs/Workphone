#ifndef __Plugin_h__
#define __Plugin_h__

#include <Workphone/Interface/System/IPlugin.hpp>

namespace workphone
{
    namespace core
    {

        /** Implementation of IPlugin interface. */
        class WPCore_API Plugin : public IPlugin
        {
        public:
            /** Constructor. */
            Plugin();

            /** Destructor. */
            ~Plugin() override;

            /** @copydoc IPlugin::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IPlugin::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IPlugin::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IPlugin::getLibraryHandle */
            LibraryHandle getLibraryHandle() const override;

            /** @copydoc IPlugin::setLibraryHandle */
            void setLibraryHandle( LibraryHandle handle ) override;

            /** @copydoc IPlugin::getFunction */
            LibraryFunction getFunction( const String &name ) const override;

            /** @copydoc IPlugin::getFilePath */
            StringW getFilePath() const override;

            /** @copydoc IPlugin::setFilePath */
            void setFilePath( const StringW &fileName ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** The filename. */
            StringW m_filename;

            /** The library handle. */
            LibraryHandle m_hMod = nullptr;
        };

    }  // namespace core
}  // namespace workphone

#endif  // Plugin_h__
