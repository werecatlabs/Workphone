#ifndef __UIImageOgreNext_h__
#define __UIImageOgreNext_h__

#include <WPGraphicsOgreNext/UI/Colibri/UIElementColibri.hpp>
#include <Workphone/UI/UIImage.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @file UIImageOgreNext.hpp
         * @brief UI image element for the OgreNext/Colibri renderer.
         *
         * Provides an OgreNext/Colibri-backed implementation of the `UIImage` UI element.
         * The element can display textures or materials using an Ogre `HlmsColibriDatablock`
         * and a Colibri custom shape renderable. It manages lazy creation of the datablock,
         * material updates, tiling and exposes properties via the generic `Properties` API.
         *
         * @note This header only contains the interface and documentation. Implementation
         *       details are in the corresponding .cpp file.
         */
        /**
         * @class UIImageOgreNext
         * @brief OgreNext-specific implementation of a UI image element.
         *
         * `UIImageOgreNext` implements the `IUIImage` interface through the
         * `UIElementOgreNext<UIImage>` base. It is responsible for:
         * - creating and keeping a reference to a Colibri datablock used to render the image,
         * - updating the material and tiling parameters when properties change,
         * - responding to state changes and exposing element properties via `Properties`.
         *
         * The class holds a pointer to the Colibri datablock and the Colibri custom shape
         * renderable used for drawing. These pointers are non-owning raw pointers and their
         * lifetime is governed by the renderer/Hlms/Colibri systems; the class will create
         * or request them as needed and will ensure safe use while the element is active.
         */
        class UIImageColibri : public UIElementColibri<UIImage>
        {
        public:
            /**
             * @brief Construct a new UIImageOgreNext.
             *
             * Initializes a new UI image element with default state (no datablock or
             * renderable assigned). Member smart pointers are default-initialized.
             */
            UIImageColibri();

            /**
             * @brief Destroy the UIImageOgreNext.
             *
             * Releases any held resources and unregisters any listeners created by this
             * element. Does not forcibly delete externally-managed Ogre/Colibri objects;
             * those are managed by the engine's resource systems.
             */
            ~UIImageColibri() override;

            /**
             * @brief Load or (re)initialize image resources.
             *
             * Called when the element should allocate or prepare its rendering resources.
             * This may create a Colibri datablock, assign materials, start listeners or
             * otherwise prepare the element to be displayed.
             *
             * @param data Optional shared data object passed by the caller. The element
             *             may use data to parameterize loading; nullptr is allowed.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release image resources.
             *
             * Called when the element is being destroyed or when its resources should be
             * released (for example when the UI context is torn down). This should undo
             * work performed in `load`.
             *
             * @param data Optional shared data object passed by the caller. The element
             *             may use data to parameterize unloading; nullptr is allowed.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handle a state change affecting this element.
             *
             * This method is invoked when an element state object changes (for example,
             * material or property state). Implementations should update internal
             * representation to reflect the new state and return whether the change
             * was handled.
             *
             * @param state Reference to the new state object.
             * @return True if the state change was handled and no further processing is
             *         required; false to let other handlers attempt to process the change.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Retrieve this element's properties.
             *
             * Exposes the element's configurable properties (material name, datablock
             * name, tiling settings, etc.) as a `Properties` object for serialization
             * or editor UIs.
             *
             * @return Shared pointer to a `Properties` instance representing current state.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties to this element.
             *
             * Updates the element's internal settings from a `Properties` object.
             * Typical use cases include loading saved UI layouts or applying changes
             * from an editor.
             *
             * @param properties Shared pointer to the new properties; must not be null.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the Colibri datablock used to render this image.
             *
             * The returned pointer is a non-owning pointer to an Ogre `HlmsColibriDatablock`.
             * The lifetime of the datablock is managed by the Ogre/Hlms system.
             *
             * @return Pointer to the `Ogre::HlmsColibriDatablock`, or nullptr if none created.
             */
            Ogre::HlmsColibriDatablock *getDatablock() const;

            /**
             * @brief Set the Colibri datablock to use for rendering.
             *
             * Assigns a datablock to the image element. Passing nullptr clears the datablock.
             * The element will not assume ownership of the provided pointer.
             *
             * @param datablock Non-owning pointer to an `HlmsColibriDatablock` or nullptr.
             */
            void setDatablock( Ogre::HlmsColibriDatablock *datablock );

            /**
             * @brief Get the name of the datablock currently used by this image.
             *
             * The name can be empty if no datablock has been assigned or created.
             *
             * @return Datablock name as a `String`.
             */
            String getDatablockName() const;

            /**
             * @brief Set the name of the datablock to use or create.
             *
             * Setting the datablock name will influence the datablock that is created or
             * looked up by `createDatablock()`. Changing the name typically requires a
             * material/datablock update.
             *
             * @param datablockName New datablock name.
             */
            void setDatablockName( const String &datablockName );

            /**
             * @brief Create or refresh the element's state/context objects.
             *
             * Prepares any additional state context required by the UI system (for example,
             * registering state listeners). This is called internally when the element
             * transitions into a context that requires such setup.
             */
            void createStateContext() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Ensure a Colibri datablock exists for this image.
             *
             * Creates or acquires an `Ogre::HlmsColibriDatablock` using the current
             * datablock name or material settings. This method is idempotent and will
             * return early if a suitable datablock already exists.
             *
             * @note The created datablock is typically managed by the Hlms/Colibri system;
             *       the pointer stored by this class is non-owning.
             */
            void createDatablock();

            /**
             * @brief Update the material assigned to the image's renderable.
             *
             * Rebuilds or updates material parameters on the datablock / renderable based
             * on the current `Properties` and state (texture, tiling, sprite size, etc.).
             */
            void updateMaterial();

            /**
             * @brief Update tiling, UV or sprite settings for the renderable.
             *
             * Applies tiling and sprite-size changes to the Colibri custom shape so the
             * image is displayed with the correct UV mapping and repetition.
             */
            void updateTiling();

            /** Listener to react to material/state changes. Managed via SmartPtr. */
            SmartPtr<IStateListener> m_materialStateListener;

            /**
             * Non-owning pointer to the Colibri datablock used for rendering.
             * Lifetime is controlled by Ogre/Hlms systems.
             */
            Ogre::HlmsColibriDatablock *m_datablock = nullptr;

            /**
             * Non-owning pointer to the Colibri custom shape renderable used to draw
             * the image. Lifetime is managed by Colibri; this class only references it.
             */
            Colibri::CustomShape *m_renderable = nullptr;

            /** Name of the datablock to create or lookup. Empty if not specified. */
            String m_datablockName;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UIImage_h__
