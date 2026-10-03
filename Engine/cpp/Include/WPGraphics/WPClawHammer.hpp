#ifndef WP_ClawHammer_h__
#define WP_ClawHammer_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class ClawHammer
         * @brief Singleton plugin that provides the "ClawHammer" rendering functionality.
         *
         * This class implements the ISharedObject interface and follows the
         * standard singleton pattern used throughout the workphone rendering
         * subsystem. It can be dynamically loaded and unloaded by the engine.
         */
        class WPGraphics_API ClawHammer : public ISharedObject
        {
        public:
            /**
             * @brief Default constructor.
             */
            ClawHammer();

            /**
             * @brief Destructor. Releases any resources held by the plugin.
             */
            ~ClawHammer() override;

            /**
             * @brief Load plugin-specific data.
             * @param data Shared pointer to the data object required for loading.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload plugin-specific data.
             * @param data Shared pointer to the data object to be unloaded.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Retrieve the singleton instance of the ClawHammer plugin.
             * @return SmartPtr to the shared ClawHammer instance.
             */
            static SmartPtr<ClawHammer> instance();

            /**
             * @brief Set the singleton instance manually.
             * @param plugin SmartPtr to a ClawHammer object that will become the global instance.
             */
            static void setInstance( SmartPtr<ClawHammer> plugin );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Singleton instance of the ClawHammer plugin. */
            static SmartPtr<ClawHammer> m_sPlugin;
        };
    }  // namespace render
}  // namespace workphone

#endif  // WPGraphics_h__
