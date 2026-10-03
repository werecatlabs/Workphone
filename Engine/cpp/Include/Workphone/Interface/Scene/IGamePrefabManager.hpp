#ifndef _IPrefabManager_H
#define _IPrefabManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * Interface for managing prefabs.
         * Extends the IResourceManager interface.
         */
        class WPCore_API IGamePrefabManager : public IResourceManager
        {
        public:
            /** Virtual destructor. */
            ~IGamePrefabManager() override;

            /**
             * Creates an instance of an Actor prefab.
             * @param prefab The prefab to create an instance of.
             * @return A smart pointer to the instantiated Actor object.
             */
            virtual SmartPtr<IGameActor> createInstance( SmartPtr<IGameActor> prefab ) = 0;

            /**
             * Loads an Actor object from data.
             * @param data The data to load the Actor from.
             * @param parent A smart pointer to the parent Actor of the loaded Actor.
             * @param cascade Whether to cascade the load to child objects.
             * @return A smart pointer to the loaded Actor object.
             */
            virtual SmartPtr<IGameActor> loadActor( SmartPtr<Properties> data,
                                                    SmartPtr<IGameActor> parent,
                                                    bool cascade = true ) = 0;

            /**
             * Loads a prefab from a file path.
             * @param filePath The file path of the prefab to load.
             * @return A smart pointer to the loaded IPrefab object.
             */
            virtual SmartPtr<IGamePrefab> loadPrefab( const String &filePath ) = 0;

            /**
             * Saves a prefab to a file.
             * @param filePath The file path to save the prefab to.
             * @param prefab A smart pointer to the prefab to save.
             */
            virtual void savePrefab( const String &filePath, SmartPtr<IGameActor> prefab ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace scene
}  // namespace workphone

#endif
