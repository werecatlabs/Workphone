#ifndef SkyboxComponent_h__
#define SkyboxComponent_h__

#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Skybox component.
         *
         * This class represents a skybox component that can be attached to a game object in the scene.
         * A skybox is used to represent the background/environment surrounding the scene and is often
         * used in 3D rendering. The skybox can be loaded with multiple textures representing different
         * faces of the environment.
         */
        class WPCore_API Skybox : public Component
        {
        public:
            /** @name Property Keys
             *  Strings used as keys to identify properties of the Skybox component.
             */
            ///@{
            static const String frontPropertyKey;
            static const String backPropertyKey;
            static const String upPropertyKey;
            static const String downPropertyKey;
            static const String rightPropertyKey;
            static const String leftPropertyKey;
            static const String swapLeftRightPropertyKey;
            static const String swapUpDownPropertyKey;
            static const String swapFrontBackPropertyKey;
            ///@}

            /**
             * @brief Listener for material-related shared events.
             *
             * This internal listener monitors events associated with the Skybox's material,
             * such as loading state transitions, to ensure the component remains synchronized
             * with the material's lifecycle.
             */
            class MaterialSharedListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor for MaterialSharedListener.
                 */
                MaterialSharedListener();

                /**
                 * @brief Destructor for MaterialSharedListener.
                 */
                ~MaterialSharedListener() override;

                /**
                 * @brief Handles an event received by the listener.
                 * @param eventType The type of the event received.
                 * @param eventValue The value associated with the event.
                 * @param arguments An array of parameters associated with the event.
                 * @param sender The shared object that sent the event.
                 * @param object The shared object associated with the event.
                 * @param event The event object itself.
                 * @return The result of handling the event.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Handles the loading state change event for the shared object.
                 * @param sharedObject The shared object whose loading state changed.
                 * @param oldState The previous loading state.
                 * @param newState The new loading state.
                 */
                void loadingStateChanged( ISharedObject *sharedObject, LoadingState oldState,
                                          LoadingState newState );

                /**
                 * @brief Destroys the object pointed to by the given pointer.
                 * @param ptr A pointer to the object to be destroyed.
                 * @return True if the object was successfully destroyed, false otherwise.
                 */
                bool destroy( void *ptr );

                /**
                 * @brief Get the owner of the MaterialSharedListener.
                 * @return A pointer to the owner Skybox component.
                 */
                Skybox *getOwnerPtr() const;

                /**
                 * @brief Get the owner of the MaterialSharedListener.
                 * @return A smart pointer to the owner Skybox component.
                 */
                SmartPtr<Skybox> getOwner() const;

                /**
                 * @brief Set the owner of the MaterialSharedListener.
                 * @param owner A smart pointer to the Skybox component that owns this listener.
                 */
                void setOwner( SmartPtr<Skybox> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<Skybox> m_owner;
            };

            /**
             * @brief Default constructor for Skybox.
             */
            Skybox();

            /**
             * @brief Destructor for Skybox.
             */
            ~Skybox() override;

            /**
             * @brief Loads the shared object data for the Skybox component.
             * @param data The shared object data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the shared object data for the Skybox component.
             * @param data The shared object data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the component based on dirty flags.
             * @param flags The flags indicating the changes that occurred.
             * @param oldFlags The previous state's flags.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Get the properties associated with the Skybox component.
             * @return A smart pointer to the properties of the Skybox component.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set the properties for the Skybox component.
             * @param properties A smart pointer to the properties to be set for the Skybox component.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the material used by the Skybox component.
             * @return A smart pointer to the material used by the Skybox component.
             */
            SmartPtr<render::IMaterial> getMaterial() const;

            /**
             * @brief Get the distance of the Skybox from the camera.
             * @return The distance of the Skybox from the camera in floating-point value.
             */
            f32 getDistance() const;

            /**
             * @brief Set the distance of the Skybox from the camera.
             * @param distance The distance of the Skybox from the camera in floating-point value.
             */
            void setDistance( f32 distance );

            /**
             * @brief Get the array of textures used by the Skybox.
             * @return An array of smart pointers to textures used by the Skybox.
             */
            Array<SmartPtr<render::ITexture>> getTextures() const;

            /**
             * @brief Set the array of textures used by the Skybox.
             * @param textures An array of smart pointers to textures to be used by the Skybox.
             */
            void setTextures( const Array<SmartPtr<render::ITexture>> &textures );

            /**
             * @brief Get the texture at a specific index in the Skybox.
             * @param index The index of the texture to retrieve.
             * @return A smart pointer to the texture at the specified index.
             */
            SmartPtr<render::ITexture> getTexture( u8 index ) const;

            /**
             * @brief Set the texture at a specific index in the Skybox.
             * @param index The index of the texture to set.
             * @param texture A smart pointer to the texture to be set at the specified index.
             */
            void setTexture( SmartPtr<render::ITexture> texture, u8 index );

            /**
             * @brief Set the texture at a specific index in the Skybox.
             */
            void setTextureByName( const String &textureName, u8 index );

            /** @copydoc Component::updateMaterials */
            void updateMaterials() override;

            /**
             * @brief Checks if the left and right textures are swapped.
             * @return True if swapped, false otherwise.
             */
            bool getSwapLeftRight() const;

            /**
             * @brief Sets whether the left and right textures are swapped.
             * @param swapLeftRight The swap state to set.
             */
            void setSwapLeftRight( bool swapLeftRight );

            /**
             * @brief Checks if the up and down textures are swapped.
             * @return True if swapped, false otherwise.
             */
            bool getSwapUpDown() const;

            /**
             * @brief Sets whether the up and down textures are swapped.
             * @param swapUpDown The swap state to set.
             */
            void setSwapUpDown( bool swapUpDown );

            /**
             * @brief Checks if the front and back textures are swapped.
             * @return True if swapped, false otherwise.
             */
            bool getSwapFrontBack() const;

            /**
             * @brief Sets whether the front and back textures are swapped.
             * @param swapFrontBack The swap state to set.
             */
            void setSwapFrontBack( bool swapFrontBack );

            /**
             * @brief Sets up the material used by the Skybox component.
             */
            void setupMaterial();

            /**
             * @brief Get the skybox interface used by the Skybox component.
             * @return A pointer to the skybox interface.
             */
            render::ISkybox *getSkyboxPtr() const;

            /**
             * @brief Get the skybox interface used by the Skybox component.
             * @return A smart pointer to the skybox interface.
             */
            SmartPtr<render::ISkybox> getSkybox() const;

            /**
             * @brief Set the skybox interface used by the Skybox component.
             * @param skybox A smart pointer to the skybox interface to be set.
             */
            void setSkybox( SmartPtr<render::ISkybox> skybox );

            /**
             * @copydoc IComponent::updateVisibility
             * @brief Updates the component's visibility state.
             */
            void updateVisibility() override;

            /**
             * @brief Synchronizes the render skybox with the component's owner.
             * @param owner The owner Skybox component.
             */
            void syncRenderSkybox( Skybox *owner );

            /**
             * @brief Declaration of class registration for the Skybox component.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handles component events for the Skybox component.
             *
             * This function is part of the Component class and is called when an event specific to the
             * component is triggered. It handles the events based on the current state of the
             * component's finite state machine.
             *
             * @param state The current state of the finite state machine for the component
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief A smart pointer to the Skybox interface.
             */
            SmartPtr<render::ISkybox> m_skybox;

            /**
             * @brief An array of smart pointers to textures used by the Skybox.
             *
             * This array contains smart pointers to render::ITexture objects representing
             * the textures used by the Skybox component. The textures are used to render
             * the different faces of the skybox that surround the scene.
             */
            FixedArray<SmartPtr<render::ITexture>, 6> m_textures;

            /**
             * @brief A smart pointer to the MaterialSharedListener for the Skybox component.
             *
             * This smart pointer points to the MaterialSharedListener object associated with
             * the Skybox component. The MaterialSharedListener is responsible for handling events
             * related to the material used by the Skybox, such as loading state changes.
             */
            SmartPtr<MaterialSharedListener> m_materialSharedListener;

            /**
             * @brief A smart pointer to the material used by the Skybox component.
             *
             * This smart pointer points to the render::IMaterial object representing the material
             * used by the Skybox component. The material defines the visual appearance and properties
             * of the Skybox, including how the textures are blended and rendered in the scene.
             */
            SmartPtr<render::IMaterial> m_material;

            /**
             * @brief The distance of the Skybox from the camera in the scene.
             *
             * This floating-point variable represents the distance between the Skybox and the camera
             * in the scene. It is typically used to control the rendering order and depth of the Skybox,
             * ensuring that it appears in the background of the scene and does not intersect with other
             * objects at close distances.
             * The default value is 50000.0 units.
             */
            f32 m_distance = 50000.0f;

            /** @brief Whether to swap left and right textures. */
            bool m_swapLeftRight = false;

            /** @brief Whether to swap up and down textures. */
            bool m_swapUpDown = false;

            /** @brief Whether to swap front and back textures. */
            bool m_swapFrontBack = false;
        };

        inline render::ISkybox *Skybox::getSkyboxPtr() const
        {
            return m_skybox.get();
        }

        inline Skybox *Skybox::MaterialSharedListener::getOwnerPtr() const
        {
            return m_owner.get();
        }

    }  // namespace scene
}  // namespace workphone

#endif  // SkyboxComponent_h__
