#ifndef __ImGuiManager_h__
#define __ImGuiManager_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Core/ConcurrentHashMap.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>
#include <Workphone/UI/UIManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

/**
 * @file ImGuiManager.hpp
 * @brief Manager for ImGui-based UI within the engine.
 *
 * This header declares the `ImGuiManager` class which implements `IUIManager`
 * and provides facilities to manage ImGui applications, UI elements, fonts,
 * and integration with the windowing/input subsystems.
 */

namespace workphone
{
    namespace ui
    {
        /**
         * @brief Implementation of the UI manager using ImGui.
         *
         * `ImGuiManager` is responsible for loading/unloading UI resources,
         * registering applications and UI elements, handling input and window
         * events, and managing font resources. Thread-safety is provided by an
         * internal recursive mutex and atomic smart pointers for key objects.
         */
        class ImGuiManager : public UIManager
        {
        public:
            /**
             * @brief Construct a new ImGuiManager instance.
             */
            ImGuiManager();

            /**
             * @brief Destroy the ImGuiManager and free resources.
             */
            ~ImGuiManager() override;

            /**
             * @copydoc IObject::load
             *
             * Loads UI-related data or objects. The argument may be a layout or
             * other shared object that the manager uses to initialize UI state.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IObject::unload
             *
             * Unloads UI-related data and releases resources associated with the
             * provided shared object.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Load a font for use by ImGui.
             *
             * @param fontPath Path to the font file.
             * @param type Optional font type identifier.
             * @return true if the font was loaded successfully.
             */
            bool loadFont( const String &fontPath,
                           const String &type = StringUtil::EmptyString ) override;

            /**
             * @brief Unload a previously loaded font.
             *
             * If no path is provided, this will attempt to unload the default
             * font for the provided type.
             *
             * @param fontPath Optional path of the font to unload.
             * @param type Optional font type identifier.
             */
            void unloadFont( const String &fontPath = StringUtil::EmptyString,
                             const String &type = StringUtil::EmptyString ) override;

            /**
             * @brief Process pending UI messages or queued work.
             *
             * This function is expected to be called regularly from the main
             * loop to allow the manager to process queued load/unload
             * operations and other UI tasks.
             *
             * @param data Optional shared object passed to the pump.
             * @return Number of processed items.
             */
            size_t messagePump( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handle a window event propagated from the renderer.
             *
             * This integrates window events (resize, focus, etc.) with the UI
             * subsystem so ImGui can update its state accordingly.
             */
            void handleWindowEvent( SmartPtr<render::IGraphicsWindowEvent> event );

            /**
             * @brief Create and register a new ImGui application object.
             *
             * @return A smart pointer to the created application instance.
             */
            SmartPtr<IUIApplication> addApplication() override;

            /**
             * @brief Remove an application previously added to the manager.
             *
             * @param application Application instance to remove.
             */
            void removeApplication( SmartPtr<IUIApplication> application ) override;

            /**
             * @brief Create and register a UI element of the given type.
             *
             * @param type Element type hash.
             * @return The created element.
             */
            SmartPtr<IUIElement> addElement( hash64 type ) override;

            /**
             * @brief Remove a UI element instance from the manager.
             *
             * @param element Element to remove.
             */
            void removeElement( SmartPtr<IUIElement> element ) override;

            /**
             * @brief Remove multiple elements in a single operation.
             *
             * @param elementsToRemove Array of elements to remove.
             */
            void removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove ) override;

            /**
             * @brief Clear all registered UI state managed by this instance.
             */
            void clear() override;

            /**
             * @brief Reload the currently active layout.
             *
             * Useful during development or when the layout resources are
             * updated on disk and need to be refreshed.
             */
            void reloadCurrentLayout();

            /**
             * @copydoc IUIManager::getCursor
             */
            SmartPtr<IUICursor> getCursor() const override;

            /**
             * @brief Find a UI element by its string identifier.
             *
             * @param id Identifier of the element to find.
             * @return Smart pointer to the element if found, otherwise null.
             */
            SmartPtr<IUIElement> findElement( const String &id ) const override;

            /**
             * @copydoc IUIManager::getApplicationPtr
             */
            IUIApplication *getApplicationPtr() const override;

            /**
             * @copydoc IUIManager::getApplication
             */
            SmartPtr<IUIApplication> getApplication() const override;

            /**
             * @copydoc IUIManager::setApplication
             */
            void setApplication( SmartPtr<IUIApplication> application ) override;

            /**
             * @brief Get windows managed by this UI manager.
             */
            Array<SmartPtr<IUIWindow>> getWindows() const;

            /**
             * @brief Replace the current windows list.
             */
            void setWindows( Array<SmartPtr<IUIWindow>> windows );

            /**
             * @brief Get registered file browser widgets.
             */
            Array<SmartPtr<IUIFileBrowser>> getFileBrowsers() const;

            /**
             * @brief Set the collection of file browser widgets.
             */
            void setFileBrowsers( Array<SmartPtr<IUIFileBrowser>> fileBrowsers );

            /**
             * @brief Get ImGui render windows managed by this manager.
             */
            Array<SmartPtr<IUIRenderWindow>> getRenderWindows() const;

            /**
             * @brief Set the collection of ImGui render windows.
             */
            void setRenderWindows( Array<SmartPtr<IUIRenderWindow>> renderWindows );

            /**
             * @copydoc IUIManager::isDragging
             */
            bool isDragging() const override;

            /**
             * @copydoc IUIManager::setDragging
             */
            void setDragging( bool dragging ) override;

            /**
             * @brief Get the main UI window used by ImGui.
             */
            SmartPtr<IUIWindow> getMainWindow() const override;

            /**
             * @brief Set the main UI window used for receiving input and focus.
             */
            void setMainWindow( SmartPtr<IUIWindow> uiWindow ) override;

            /**
             * @brief Mark the UI as invalid so it will be rebuilt or refreshed.
             */
            void invalidate() override;

            /**
             * @brief Retrieve raw pointer to this object for external APIs.
             *
             * @param ppObject Out parameter receiving the pointer to this instance.
             */
            void _getObject( void **ppObject ) override;

            IFactoryManager *getFactoryManagerPtr() const;

            /**
             * @brief Get the factory manager used to create UI objects.
             */
            SmartPtr<IFactoryManager> getFactoryManager() const;

            /**
             * @brief Set the factory manager used for UI element creation.
             */
            void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

            /**
             * @brief Get the overlay object used when rendering to an overlay.
             */
            SmartPtr<ISharedObject> getOverlay() const override;

            /**
             * @brief Set the overlay object used for overlay rendering.
             */
            void setOverlay( SmartPtr<ISharedObject> overlay ) override;

            /**
             * @brief Load a graphics/shared object, optionally queueing the load.
             *
             * @param graphicsObject Object to load.
             * @param forceQueue If true, forces the load to be queued.
             */
            void loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue = false ) override;

            /**
             * @brief Unload a graphics/shared object, optionally queueing the unload.
             */
            void unloadObject( SmartPtr<ISharedObject> graphicsObject,
                               bool forceQueue = false ) override;

            /**
             * @brief Acquire the manager's internal lock.
             */
            void lock() override;

            /**
             * @brief Release the manager's internal lock.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Small event listener that forwards input events to the manager.
             *
             * This nested class implements the engine's IEventListener interface
             * and acts as a thin adapter that calls into the owning
             * ImGuiManager instance.
             */
            class InputListener : public IEventListener
            {
            public:
                /**
                 * @brief Called when an input event has occurred.
                 *
                 * @param event The input event object.
                 * @return true if the event was consumed.
                 */
                bool inputEvent( SmartPtr<IInputEvent> event );

                /**
                 * @brief Called when input devices have been updated.
                 *
                 * @param event Latest input event snapshot.
                 * @return true if handled.
                 */
                bool updateEvent( const SmartPtr<IInputEvent> &event );

                /**
                 * @brief Set the listener priority used by the event system.
                 */
                void setPriority( s32 priority ) override;

                /**
                 * @brief Get the listener priority.
                 */
                s32 getPriority() const override;

                /**
                 * @brief Associate this listener with its owning ImGuiManager.
                 */
                void setOwner( ImGuiManager *owner );

                /**
                 * @brief Get the associated ImGuiManager instance.
                 */
                ImGuiManager *getOwner() const;

            protected:
                ImGuiManager *m_owner = nullptr; /**< Owner pointer; not owned. */
            };

            /**
             * @brief Window listener that forwards renderer window events.
             *
             * Implements `render::IGraphicsWindowListener` and forwards events like
             * resize/focus/close to the owning ImGuiManager instance.
             */
            class WindowListener : public render::IGraphicsWindowListener
            {
            public:
                WindowListener();
                ~WindowListener() override;

                /**
                 * @brief Handle a generic event from the renderer/window system.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Set the owning ImGuiManager for this listener.
                 */
                void setOwner( ImGuiManager *owner );

                /**
                 * @brief Get the owning ImGuiManager for this listener.
                 */
                ImGuiManager *getOwner() const;

            protected:
                ImGuiManager *m_owner = nullptr; /**< Owner pointer; not owned. */
            };

            /**
             * @brief Load the special FontAwesome font used by the UI.
             *
             * @param fontPath Path to the FontAwesome font file.
             * @return true if loaded successfully.
             */
            bool loadFontAwesomeFont( const String &fontPath );

            /**
             * @brief Unload the FontAwesome font from ImGui.
             */
            void unloadFontAwesomeFont();

            /**
             * @brief Internal helper to register an element under a numeric type.
             */
            void addElement( u32 type, SmartPtr<IUIElement> node );

            /**
             * @brief Internal helper to remove an element by type.
             */
            void removeElement( u32 type, SmartPtr<IUIElement> node );

            /**
             * @brief Get a pointer to the list of elements for a given type.
             */
            SharedPtr<Array<SmartPtr<IUIElement>>> getElementsPtr( u32 type ) const;

            /**
             * @brief Set the list pointer used for a given element type.
             */
            void setElementsPtr( u32 type, SharedPtr<Array<SmartPtr<IUIElement>>> p );

            /**
             * @brief Return all element lists indexed by type.
             */
            Array<SharedPtr<Array<SmartPtr<IUIElement>>>> getElementsByType() const;

            SmartPtr<ISharedObject> m_overlay; /**< Overlay object used for overlay rendering. */

            AtomicSmartPtr<IUIWindow> m_uiWindow; /**< Main UI window pointer (atomic). */

            AtomicSmartPtr<IUIApplication> m_application; /**< Current application instance (atomic). */

            AtomicSmartPtr<IFactoryManager> m_factoryManager;
            /**< Factory manager used to create UI objects. */

            atomic_bool m_dragging = false; /**< True when a drag operation is active. */

            ConcurrentHashMap<u32, AtomicSharedPtr<Array<SmartPtr<IUIElement>>>> m_elements;
            /**< Elements grouped by type. */

            ConcurrentQueue<SmartPtr<ISharedObject>> m_loadQueue;
            /**< Queue of objects to load on the main thread. */
            ConcurrentQueue<SmartPtr<ISharedObject>> m_unloadQueue;
            /**< Queue of objects to unload on the main thread. */

            mutable RecursiveSpinMutex m_mutex; /**< Protects internal state for thread-safety. */
        };

        inline IFactoryManager *ImGuiManager::getFactoryManagerPtr() const
        {
            return m_factoryManager.get();
        }

    }  // end namespace ui
}  // namespace workphone

#endif  // CEGUIManager_h__
