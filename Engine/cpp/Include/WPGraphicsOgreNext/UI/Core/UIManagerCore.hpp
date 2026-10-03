#ifndef __UIManagerCore_h__
#define __UIManagerCore_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/UI/UIManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <OgreVector2.h>
#include <workphone_prerequisites.h>

namespace workphone
{
    namespace ui
    {

        /**
         * @brief UI manager implementation that drives the Core-based UI using the render system.
         *
         * RenderUI implements IUIManager and manages UI elements, applications, the Core
         * manager and layout window. It provides methods to load/unload UI resources, dispatch
         * input events to the UI, and perform per-frame updates.
         *
         * @details
         * This class serves as the primary interface between the engine's UI system and the Core
         * GUI library. It handles:
         * - Creation and lifecycle management of UI elements (buttons, text, images, etc.)
         * - Input event forwarding and handling
         * - Per-frame UI updates and rendering preparation
         * - Font loading and management
         * - Thread-safe resource loading/unloading through queuing mechanisms
         *
         * Thread-safe containers are used for element lists and load/unload queues so that resources
         * may be enqueued from other threads safely. However, the main UI operations (update, render
         * preparation) should be performed on the main/render thread.
         *
         * @note
         * This class requires a valid render::IGraphicsScene to be set before use. The Core manager
         * and layout window are created during the load() phase.
         *
         * @warning
         * UI elements should not be directly manipulated during rendering. Use the load/unload queuing
         * mechanisms for thread-safe resource management.
         *
         * @see UIManager
         * @see Core::CoreManager
         * @see render::IGraphicsScene
         */
        class UIManagerCore : public UIManager
        {
        public:
            /**
             * @brief Event listener used to forward engine events to the owner RenderUI.
             *
             * This listener holds a weak reference to its owner RenderUI and converts
             * incoming events into Parameter-based responses expected by the engine.
             *
             * @details
             * The EventListener acts as a bridge between the engine's event system and the UI manager.
             * It uses a weak pointer to avoid circular references and prevent memory leaks. When events
             * are received, they are validated and forwarded to the RenderUI instance if it still exists.
             *
             * @note
             * The weak reference pattern ensures that the listener does not prevent the RenderUI from
             * being destroyed when it's no longer needed.
             *
             * @see IEventListener
             * @see RenderUI::setInputListener
             */
            class EventListener : public IEventListener
            {
            public:
                /** @brief Default constructor. Initializes the event listener with no owner. */
                EventListener();

                /** @brief Virtual destructor. Cleans up listener resources. */
                ~EventListener() override;

                /**
                 * @brief Called when the listener is being unloaded.
                 * @param data Optional data provided by the caller.
                 *
                 * Implementers should release resources tied to this listener here.
                 * This is called during the cleanup phase to ensure proper resource disposal.
                 *
                 * @note This method is part of the ISharedObject lifecycle management.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an incoming event and optionally produce a Parameter response.
                 * @param eventType The event type identifier.
                 * @param eventValue Event-specific value (hashed identifier).
                 * @param arguments Additional event arguments as a parameter array.
                 * @param sender The event sender (optional, may be null).
                 * @param object Associated object (optional, may be null).
                 * @param event The original event object (optional, may be null).
                 * @return A Parameter containing result information for the event.
                 *
                 * This method converts engine events to an appropriate form for the UI system.
                 * It validates the owner reference before forwarding events and returns appropriate
                 * responses based on the event type.
                 *
                 * @note The event is only processed if the owner RenderUI still exists (weak pointer is valid).
                 * @see EventType
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Get the owner RenderUI instance (if still alive).
                 * @return SmartPtr to the owning RenderUI or null if it has expired.
                 *
                 * @note This method locks and resolves the weak pointer to check validity.
                 * @see setOwner
                 */
                SmartPtr<UIManagerCore> getOwner() const;

                /**
                 * @brief Set the owner RenderUI for this listener.
                 * @param owner SmartPtr to the RenderUI that owns this listener.
                 *
                 * Stores a weak reference to prevent circular dependencies.
                 * @see getOwner
                 */
                void setOwner( SmartPtr<UIManagerCore> owner );

            private:
                AtomicWeakPtr<UIManagerCore> m_owner; /**< Weak reference to the owner RenderUI. */
            };

            /** @brief Constructor. Initializes the RenderUI with default values. */
            UIManagerCore();

            /** @brief Destructor. Cleans up all UI resources and Core instances. */
            ~UIManagerCore() override;

            /**
             * @copydoc IUIManager::load
             * @param data Optional data to use during load (may contain configuration parameters).
             *
             * Initialize Core manager, create the layout window and prepare UI resources.
             * This method must be called before the UI can be used.
             *
             * @note This should be called on the main/render thread.
             * @see unload
             * @see reload
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IUIManager::reload
             * @param data Optional data used for reloading resources.
             *
             * Reloads UI resources and refreshes layouts; typically called when assets change
             * or when the UI needs to be refreshed due to resolution or theme changes.
             *
             * @note This preserves existing UI state where possible.
             * @see load
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IUIManager::unload
             * @param data Optional data passed to unload.
             *
             * Cleanly shuts down the Core manager and releases UI resources.
             * All UI elements are destroyed and the layout window is released.
             *
             * @note This should be called on the same thread that called load().
             * @see load
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Load a font for use by the UI system.
             *
             * @param fontPath Path to the font file (typically .ttf or .otf).
             * @param type Optional font type identifier (e.g., "default", "header", "monospace").
             * @return true if the font was loaded successfully, false otherwise.
             *
             * The loaded font can be referenced by UI text elements. Font loading is
             * delegated to the Core rendering system.
             *
             * @note Font paths should be relative to the application's resource directory.
             * @see unloadFont
             */
            bool loadFont( const String &fontPath,
                           const String &type = StringUtil::EmptyString ) override;

            /**
             * @brief Unload a previously loaded font.
             *
             * If no path is provided, this will attempt to unload the default
             * font for the provided type.
             *
             * @param fontPath Optional path of the font to unload. If empty, unloads by type.
             * @param type Optional font type identifier.
             *
             * @note Unloading a font that is currently in use by UI elements may cause
             * those elements to fall back to a default font.
             * @see loadFont
             */
            void unloadFont( const String &fontPath = StringUtil::EmptyString,
                             const String &type = StringUtil::EmptyString ) override;

            /**
             * @brief Handle a platform/input event and forward it to the UI system.
             * @param event Input event to process (mouse, keyboard, touch, etc.).
             * @return True if the event was consumed by the UI, false if it should be passed to other systems.
             *
             * This method translates engine input events to Core input events and forwards them
             * to the UI system. Events consumed by the UI (e.g., clicking on a button) return true
             * to prevent further processing by game logic.
             *
             * @note This should be called before game input processing to allow UI priority.
             * @see IInputEvent
             */
            bool handleEvent( const SmartPtr<IInputEvent> &event );

            /**
             * @brief Get a raw pointer to the internal implementation object.
             * @param ppObject Out parameter receiving the raw pointer to this RenderUI instance.
             *
             * This is used for interop with code expecting a raw engine UI object pointer.
             * The pointer should not be deleted by the caller.
             *
             * @warning The returned pointer does not manage lifetime. Ensure the RenderUI
             * remains alive for the duration of use.
             */
            void _getObject( void **ppObject ) override;

            /**
             * @brief Per-frame update for the UI manager.
             *
             * Processes Core updates, pending load/unload queues, and performs
             * any per-frame widget updates required before rendering. This should be
             * called once per frame before the render pass.
             *
             * @details
             * During the update:
             * - Queued load/unload operations are processed
             * - Widget animations and state changes are updated
             * - Layout recalculations are performed if needed
             * - Input state is prepared for the next frame
             *
             * @note This must be called on the main/render thread.
             * @see handleEvent
             */
            void update() override;

            /** @copydoc UIManager::render */
            void render() override;

            /**
             * @brief Create and return a UI element of the specified type.
             * @param type Hash64 identifier representing the element type (factory key).
             * @return SmartPtr to the created IUIElement, or null if creation failed.
             *
             * The type parameter is matched against registered UI element factories.
             * Common types include buttons, labels, images, text fields, etc.
             *
             * @note The created element is automatically added to the internal element list.
             * @see removeElement
             * @see IFactoryManager
             */
            SmartPtr<IUIElement> addElement( hash64 type ) override;

            /**
             * @brief Remove the specified UI element.
             * @param element Element to remove from the UI system.
             *
             * The element is removed from the active element list and cleaned up.
             * Any visual representation is also destroyed.
             *
             * @note It is safe to call this with null or already-removed elements.
             * @see addElement
             * @see removeElements
             */
            void removeElement( SmartPtr<IUIElement> element ) override;

            /**
             * @brief Remove multiple UI elements in a batch.
             * @param elementsToRemove Array of elements to remove.
             *
             * This is more efficient than calling removeElement repeatedly as it
             * batches the cleanup operations.
             *
             * @see removeElement
             * @see clear
             */
            void removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove ) override;

            /**
             * @brief Remove all UI elements and reset the UI state.
             *
             * Destroys all active UI elements and clears internal containers.
             * This is useful when transitioning between UI states or scenes.
             *
             * @note This does not unload the UI system itself; call unload() for that.
             * @see removeElement
             * @see unload
             */
            void clear() override;

            /**
             * @brief Mark the UI as needing a refresh before the next render.
             *
             * This forces a full UI layout recalculation and redraw. Typically called
             * after batch element modifications or when the viewport changes.
             *
             * @note The actual refresh occurs during the next update() call.
             * @see update
             */
            void invalidate() override;

            /**
             * @brief Get the Core layout window used for rendering UI.
             * @return Pointer to wp_window or nullptr if not available.
             *
             * The layout window represents the root UI container and canvas.
             * All UI elements are children of this window.
             *
             * @note The window is created during load() via createLayoutWindow().
             * @see setLayoutWindow
             * @see createLayoutWindow
             */
            SmartPtr<UILayoutCore> getLayoutWindow() const;

            /**
             * @brief Get the Workphone context owned by this UI manager.
             *
             * Returns the raw `wp_context` pointer used for all Workphone draw calls.
             * Valid after load() and null before load() or after unload().
             *
             * @return Raw pointer to the `wp_context`, or nullptr if not initialised.
             */
            struct wp_context *getContext() const;

            /**
             * @brief Set the Core layout window.
             * @param layoutWindow Pointer to the wp_window instance to use as the root container.
             *
             * @warning Setting this to null or changing it while elements are active may cause crashes.
             * @see getLayoutWindow
             */
            void setLayoutWindow( SmartPtr<UILayoutCore> layoutWindow );

            /**
             * @brief Get the factory manager used to create UI objects.
             * @return SmartPtr to IFactoryManager.
             *
             * The factory manager provides the mechanism to instantiate UI elements based on type hashes.
             * @see setFactoryManager
             * @see addElement
             */
            SmartPtr<IFactoryManager> getFactoryManager() const;

            /**
             * @brief Set the factory manager used to create UI objects.
             * @param factoryManager SmartPtr to an IFactoryManager instance.
             *
             * The factory manager must have UI element factories registered before elements can be created.
             * @see getFactoryManager
             */
            void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

            /**
             * @brief Get the current list of UI elements.
             * @return Array of SmartPtr<IUIElement> representing all active elements.
             *
             * This returns a snapshot copy of the element list for safe iteration.
             * @see setElements
             * @see addElement
             */
            Array<SmartPtr<IUIElement>> getElements() const;

            /**
             * @brief Replace the current list of UI elements.
             * @param elements Array of UI elements to set.
             *
             * This clears the existing element list and replaces it with the provided array.
             * @warning Use with caution as this bypasses normal element lifecycle management.
             * @see getElements
             */
            void setElements( Array<SmartPtr<IUIElement>> elements );

            /**
             * @brief Get the graphics scene used by the UI (render target / context).
             * @return Raw pointer to the render::IGraphicsScene, or nullptr if not set.
             *
             * Returns a raw pointer for performance-critical code paths.
             * @see getGraphicsScene
             * @see setGraphicsScene
             */
            render::IGraphicsScene *getGraphicsScenePtr() const;

            /**
             * @brief Get the graphics scene used by the UI (render target / context).
             * @return SmartPtr to the render::IGraphicsScene.
             *
             * The graphics scene provides the rendering context and resources for UI rendering.
             * @see getGraphicsScenePtr
             * @see setGraphicsScene
             */
            SmartPtr<render::IGraphicsScene> getGraphicsScene() const;

            /**
             * @brief Set the graphics scene used by the UI.
             * @param graphicsScene SmartPtr to a render::IGraphicsScene.
             *
             * This must be set before load() is called. The graphics scene provides the
             * rendering context required for UI operations.
             *
             * @note Changing the scene after load() may require a reload() call.
             * @see getGraphicsScene
             * @see setupSceneManager
             */
            void setGraphicsScene( SmartPtr<render::IGraphicsScene> graphicsScene );

            /**
             * @copydoc IUIManager::getOverlay
             * @return SmartPtr to the overlay object, or nullptr if not set.
             *
             * The overlay may represent additional rendering layers or debugging visualizations.
             */
            SmartPtr<ISharedObject> getOverlay() const override;

            /**
             * @copydoc IUIManager::setOverlay
             * @param overlay SmartPtr to an overlay object.
             *
             * Sets an optional overlay for additional rendering functionality.
             */
            void setOverlay( SmartPtr<ISharedObject> overlay ) override;

            /**
             * @brief Enqueue or immediately load a graphics object required by the UI.
             * @param graphicsObject Object to load (textures, materials, etc.).
             * @param forceQueue If true, the object is added to the load queue even if it could be loaded immediately.
             *
             * Thread-safe method to request loading of graphics resources. If forceQueue is false
             * and the call is made from the render thread, the object may be loaded immediately.
             *
             * @note Queued loads are processed during update() on the render thread.
             * @see unloadObject
             */
            void loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue = false ) override;

            /**
             * @brief Enqueue or immediately unload a graphics object used by the UI.
             * @param graphicsObject Object to unload.
             * @param forceQueue If true, the object is added to the unload queue even if it could be unloaded immediately.
             *
             * Thread-safe method to request unloading of graphics resources. Queued operations
             * are processed during update().
             *
             * @note It is safe to unload objects that are no longer referenced by UI elements.
             * @see loadObject
             */
            void unloadObject( ISharedObject* object, bool forceQueue = false ) override;

            /**
             * @brief Acquire the UI manager lock for thread-safe operations.
             *
             * Must be paired with unlock(). Used to protect shared state during multi-threaded access.
             * @see unlock
             */
            void lock() override;

            /**
             * @brief Release the UI manager lock.
             *
             * Must be called after lock() to release the mutex.
             * @see lock
             */
            void unlock() override;

            /**
             * @brief Query whether this UI manager is in a valid (initialized) state.
             * @return True if valid and ready for use, false otherwise.
             *
             * A UI manager is valid after successful load() and before unload().
             * Invalid states may occur during construction or after cleanup.
             *
             * @see load
             * @see unload
             */
            bool isValid() const override;

            /** @brief Macro for class registration with the engine's factory system. */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Configure the scene manager for UI rendering integration.
             * @param sceneManager Graphics scene used to setup UI rendering state.
             *
             * Initializes the scene manager with necessary UI rendering parameters,
             * such as viewport configuration and compositor setup.
             *
             * @note Called during load() to prepare the rendering pipeline.
             * @see load
             */
            void setupSceneManager( SmartPtr<render::IGraphicsScene> sceneManager );

            /**
             * @brief Create the Core layout window used for UI placement and rendering.
             *
             * Instantiates the root Core window with appropriate canvas size and resolution.
             * All UI elements are children of this window.
             *
             * @note Called during load() after the Core manager is initialized.
             * @see load
             * @see getLayoutWindow
             */
            void createLayoutWindow();

            /** @brief Weak reference to the graphics scene. Thread-safe atomic wrapper. */
            AtomicWeakPtr<render::IGraphicsScene> m_graphicsScene;

            /** @brief Factory manager for creating UI elements. Thread-safe atomic wrapper. */
            AtomicSmartPtr<IFactoryManager> m_factoryManager;

            /** @brief Listener for input events forwarded to the UI. Thread-safe atomic wrapper. */
            AtomicSmartPtr<IEventListener> m_inputListener;

            /** @brief Raw pointer to the Core layout window. Thread-safe atomic wrapper. */
            AtomicSmartPtr<UILayoutCore> m_layoutWindow;

            /** @brief Raw pointer to the Core manager. Thread-safe atomic wrapper. */
            AtomicRawPtr<struct wp_context> m_ctx;

            /** @brief Logical canvas size used by Core for layout calculations (in virtual pixels). */
            Ogre::Vector2 m_canvasSize = Ogre::Vector2( 1920, 1080 );

            /** @brief Current window resolution in physical pixels. */
            Ogre::Vector2 m_windowResolution = Ogre::Vector2( 1920, 1080 );

            /** @brief Optional overlay object associated with this UI manager. */
            AtomicSmartPtr<ISharedObject> m_overlay;
        };

        /**
         * @brief Inline implementation of getGraphicsScenePtr for performance.
         * @return Raw pointer to the graphics scene.
         *
         * This inline accessor provides efficient access to the graphics scene pointer
         * for performance-critical rendering paths.
         */
        inline render::IGraphicsScene *UIManagerCore::getGraphicsScenePtr() const
        {
            return m_graphicsScene.get();
        }

        inline struct wp_context *UIManagerCore::getContext() const
        {
            return m_ctx.load();
        }

    }  // namespace ui
}  // namespace workphone

#endif  // UIManager_h__
