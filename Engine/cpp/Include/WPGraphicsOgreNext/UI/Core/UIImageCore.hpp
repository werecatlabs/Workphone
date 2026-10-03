#ifndef __UIImageCore_h__
#define __UIImageCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/UI/UIImage.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @file UIImageOgreNext.hpp
         * @brief UI image element for the OgreNext/Core renderer.
         *
         * Provides an OgreNext/Core-backed implementation of the `UIImage` UI element.
         * The element can display textures or materials using an Ogre `HlmsCoreDatablock`
         * and a Core custom shape renderable. It manages lazy creation of the datablock,
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
         * - creating and keeping a reference to a Core datablock used to render the image,
         * - updating the material and tiling parameters when properties change,
         * - responding to state changes and exposing element properties via `Properties`.
         *
         * The class holds a pointer to the Core datablock and the Core custom shape
         * renderable used for drawing. These pointers are non-owning raw pointers and their
         * lifetime is governed by the renderer/Hlms/Core systems; the class will create
         * or request them as needed and will ensure safe use while the element is active.
         */
        class UIImageCore : public UIElementCore<UIImage>
        {
        public:
            /**
             * @brief Construct a new UIImageOgreNext.
             *
             * Initializes a new UI image element with default state (no datablock or
             * renderable assigned). Member smart pointers are default-initialized.
             */
            UIImageCore();

            /**
             * @brief Destroy the UIImageOgreNext.
             *
             * Releases any held resources and unregisters any listeners created by this
             * element. Does not forcibly delete externally-managed Ogre/Core objects;
             * those are managed by the engine's resource systems.
             */
            ~UIImageCore() override;

            /**
             * @brief Load or (re)initialize image resources.
             *
             * Called when the element should allocate or prepare its rendering resources.
             * This may create a Core datablock, assign materials, start listeners or
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
             * @brief Per-frame update — submits the image draw command to the Workphone context.
             *
             * Resolves the element's bounds to pixel space and emits a `wp_draw_image` call
             * into the window canvas so the OgreNext renderer picks it up this frame.
             */
            void update() override;

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
             * @brief Create or refresh the element's state/context objects.
             *
             * Prepares any additional state context required by the UI system (for example,
             * registering state listeners). This is called internally when the element
             * transitions into a context that requires such setup.
             */
            void createStateContext() override;

            /** Get the reference canvas width used for normalised-to-pixel conversion. */
            f32 getReferenceWidth() const;

            /** Set the reference canvas width (default 1920). */
            void setReferenceWidth( f32 width );

            /** Get the reference canvas height used for normalised-to-pixel conversion. */
            f32 getReferenceHeight() const;

            /** Set the reference canvas height (default 1080). */
            void setReferenceHeight( f32 height );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Width of the reference canvas in pixels (default 1920). */
            f32 m_referenceWidth = 1920.0f;

            /** Height of the reference canvas in pixels (default 1080). */
            f32 m_referenceHeight = 1080.0f;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIImage_h__
