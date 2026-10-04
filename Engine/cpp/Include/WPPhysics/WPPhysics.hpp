#ifndef WPPHYSICS_HPP
#define WPPHYSICS_HPP

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @class WPPhysics
         * @brief Main physics engine controller class.
         *
         * Handles the lifecycle and global access to the physics system within the workphone namespace.
         */
        class WPPhysics_API WPPhysics : public ISharedObject
        {
        public:
            /**
             * @brief Constructs a new WPPhysics instance.
             */
            WPPhysics();

            /**
             * @brief Destroys the WPPhysics instance.
             */
            ~WPPhysics() override;

            /**
             * @brief Loads the physics engine data.
             * @param data Pointer to the shared object containing the data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the physics engine data.
             * @param data Pointer to the shared object containing the data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the singleton instance of the physics engine.
             * @return A SmartPtr to the current WPPhysics instance.
             */
            static SmartPtr<WPPhysics> instance();

            /**
             * @brief Sets the singleton instance of the physics engine.
             * @param plugin The SmartPtr to the physics plugin instance.
             */
            static void setInstance( SmartPtr<WPPhysics> plugin );

        protected:
            static SmartPtr<WPPhysics>
                m_sPlugin;  ///< The global singleton instance of the physics engine.
        };

    }  // namespace physics
}  // namespace workphone

#endif
