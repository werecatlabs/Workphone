#ifndef __Prefab_h__
#define __Prefab_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/System/Resource.hpp>

namespace workphone
{
    namespace scene
    {

        /** Prefab class. */
        class GamePrefab : public Resource<IGamePrefab>
        {
        public:
            /** Constructor. */
            GamePrefab();

            /** Destructor. */
            ~GamePrefab() override;

            /** @copydoc Resource<IPrefab>::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource<IPrefab>::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource<IPrefab>::createActor */
            SmartPtr<IGameActor> createActor() override;

            /** @copydoc Resource<IPrefab>::save */
            void save() override;

            /** @copydoc Resource<IPrefab>::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Resource<IPrefab>::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc Resource<IPrefab>::_getObject */
            void _getObject( void **ppObject ) const override;

            /** @copydoc Resource<IPrefab>::getData */
            SmartPtr<ISharedObject> getData() const override;

            /** @copydoc Resource<IPrefab>::setData */
            void setData( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IGamePrefabManager::lock
             * @brief Locks the game prefab manager for thread-safe operations.
             */
            void lock() override;

            /**
             * @copydoc IGamePrefabManager::try_lock
             * @brief Attempts to lock the game prefab manager for thread-safe operations.
             * @return True if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @copydoc IGamePrefabManager::unlock
             * @brief Unlocks the game prefab manager.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Data. */
            SmartPtr<ISharedObject> m_data;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // IResource_h__
