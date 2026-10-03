#ifndef FBGUI_ClawUIMANAGER_H
#define FBGUI_ClawUIMANAGER_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/UI/UIManager.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneRenderer.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

struct wp_context;

namespace workphone
{
    namespace ui
    {

        /**
         * @class ClawUIManager
         * @brief Implementation of a UI system that utilizes the render system.
         */
        class WPGraphics_API ClawUIManager : public UIManager
        {
        public:
            class InputListener : public IEventListener
            {
            public:
                InputListener( ClawUIManager *mgr );

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                ClawUIManager *m_mgr = nullptr;
            };

            /**
             * @brief Constructs a new {@link ClawUIManager} instance.
             */
            ClawUIManager();

            /** Destructor. */
            ~ClawUIManager() override;

            /** @copydoc IUIManager::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IUIManager::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Load a font for use by ImGui.
             * @param fontPath Path to the font file.
             * @param type Optional font type identifier (default empty).
             * @return true if the font was loaded successfully.
             */
            bool loadFont( const String &fontPath,
                           const String &type = StringUtil::EmptyString ) override;

            /**
             * @brief Unload a previously loaded font.
             * @param fontPath Optional path of the font to unload. If empty, the default font for the given type is removed.
             * @param type Optional font type identifier (default empty).
             */
            void unloadFont( const String &fontPath = StringUtil::EmptyString,
                             const String &type = StringUtil::EmptyString ) override;

            /** @copydoc IUIManager::messagePump */
            size_t messagePump( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handles input events.
             * @param event The input event to process.
             * @return true if the event was handled.
             */
            bool OnEvent( const SmartPtr<IInputEvent> &event );

            /**
             * @brief Updates the UI state.
             */
            void update() override;

            /** @copydoc IUIManager::render */
            void render() override;

            /** @copydoc IUIManager::addApplication */
            SmartPtr<IUIApplication> addApplication() override;

            /** @copydoc IUIManager::removeApplication */
            void removeApplication( SmartPtr<IUIApplication> application ) override;

            /** @copydoc IUIManager::addElement */
            SmartPtr<IUIElement> addElement( hash64 type ) override;

            void removeElement( SmartPtr<IUIElement> element ) override;
            void removeElements( const Array<SmartPtr<IUIElement>> &elements ) override;

            /**
             * @brief Adds a UI element as a child of the specified parent.
             * @param parent The parent UI element.
             * @param type The element type identifier.
             * @return Smart pointer to the newly created UI element.
             */
            SmartPtr<IUIElement> addElement( SmartPtr<IUIElement> parent, u8 type );

            /** @copydoc IUIManager::clear */
            void clear() override;

            /**
             * @brief Gets the root UI element.
             * @return The root element.
             */
            SmartPtr<IUIElement> getRoot() const;

            /**
             * @brief Reloads the current UI layout.
             */
            void reloadCurrentLayout();

            /**
             * @brief Retrieves the UI cursor.
             * @return Smart pointer to the current {@link IUICursor} instance.
             */
            SmartPtr<IUICursor> getCursor() const override;

            /**
             * @brief Finds a UI element by its identifier.
             * @param id The unique identifier of the UI element.
             * @return Smart pointer to the found UI element, or null if not found.
             */
            SmartPtr<IUIElement> findElement( const String &id ) const override;

            /**
             * @brief Retrieves the current UI application.
             * @return Smart pointer to the {@link IUIApplication} instance.
             */
            SmartPtr<IUIApplication> getApplication() const override;

            /**
             * @brief Sets the current UI application.
             * @param application Smart pointer to the {@link IUIApplication} to set.
             */
            void setApplication( SmartPtr<IUIApplication> application ) override;

            /**
             * @brief Retrieves the collection of UI windows.
             * @return Array of smart pointers to {@link IUIWindow} instances.
             */
            Array<SmartPtr<IUIWindow>> getWindows() const;

            /**
             * @brief Sets the collection of UI windows.
             * @param windows Array of UI windows to manage.
             */
            void setWindows( Array<SmartPtr<IUIWindow>> windows );

            /**
             * @brief Retrieves render windows.
             * @return Array of {@link IUIRenderWindow} smart pointers.
             */
            Array<SmartPtr<IUIRenderWindow>> getRenderWindows() const;
            /**
             * @brief Sets the render windows collection.
             * @param renderWindows Array of render windows.
             */
            void setRenderWindows( Array<SmartPtr<IUIRenderWindow>> renderWindows );

            /**
             * @brief Retrieves file browsers.
             * @return Array of {@link IUIFileBrowser} smart pointers.
             */
            Array<SmartPtr<IUIFileBrowser>> getFileBrowsers() const;
            /**
             * @brief Sets the file browsers collection.
             * @param fileBrowsers Array of file browsers.
             */
            void setFileBrowsers( Array<SmartPtr<IUIFileBrowser>> fileBrowsers );

            /**
             * @brief Checks whether a UI drag operation is in progress.
             * @return true if dragging, false otherwise.
             */
            bool isDragging() const override;

            /**
             * @brief Sets the dragging state.
             * @param dragging True to enable dragging, false to disable.
             */
            void setDragging( bool dragging ) override;

            /**
             * @brief Retrieves the main UI window.
             * @return Smart pointer to the main {@link IUIWindow}.
             */
            SmartPtr<IUIWindow> getMainWindow() const override;

            /**
             * @brief Sets the main UI window.
             * @param uiWindow Smart pointer to the UI window to set as main.
             */
            void setMainWindow( SmartPtr<IUIWindow> uiWindow ) override;

            /**
             * @brief Invalidates the UI, forcing a redraw on the next frame.
             */
            void invalidate() override;

            /**
             * @brief Retrieves the overlay shared object.
             * @return Smart pointer to the overlay.
             */
            SmartPtr<ISharedObject> getOverlay() const override;

            /**
             * @brief Sets the overlay shared object.
             * @param overlay Smart pointer to the overlay to set.
             */
            void setOverlay( SmartPtr<ISharedObject> overlay ) override;

            /**
             * @brief Loads a graphics object into the UI system.
             * @param graphicsObject The graphics object to load.
             * @param forceQueue If true, forces the operation to be queued.
             */
            void loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue = false ) override;

            /**
             * @brief Unloads a graphics object from the UI system.
             * @param graphicsObject The graphics object to unload.
             * @param forceQueue If true, forces the operation to be queued.
             */
            void unloadObject( SmartPtr<ISharedObject> graphicsObject,
                               bool forceQueue = false ) override;

            /**
             * @brief Acquires the internal UI mutex.
             */
            void lock() override;

            /**
             * @brief Releases the internal UI mutex.
             */
            void unlock() override;

            /**
             * @brief Retrieves a raw pointer to the underlying UI object.
             * @param ppObject Output pointer to receive the object.
             */
            void _getObject( void **ppObject ) override;

            /**
             * @brief Retrieves the factory manager used by the UI.
             * @return Smart pointer to the {@link IFactoryManager}.
             */
            SmartPtr<IFactoryManager> getFactoryManager() const;

            /**
             * @brief Sets the factory manager for UI element creation.
             * @param factoryManager Smart pointer to the factory manager.
             */
            void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

            /**
             * @brief Get the Workphone context owned by this UI manager.
             *
             * Returns the raw `wp_context` pointer used for all Workphone draw calls.
             * Valid after load() and null before load() or after unload().
             *
             * @return Raw pointer to the `wp_context`, or nullptr if not initialised.
             */
            struct wp_context *getContext() const;

            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Sets the root UI element of the hierarchy.
             * @param root Smart pointer to the root element.
             */
            void setRoot( SmartPtr<IUIElement> root );

            SmartPtr<IUIWindow> m_uiWindow;  ///< The current active UI window.

            SmartPtr<IUIApplication> m_application;  ///< The current active UI application.

            AtomicSmartPtr<IUIElement> m_root;  ///< The root element of the UI hierarchy.

            SmartPtr<IUIElement> m_itemInFocus;  ///< The UI element currently in focus.

            SmartPtr<IEventListener> m_inputListener;  ///< Listener for handling input events.

            SmartPtr<IFactoryManager> m_factoryManager;

            /** WorkphoneCore immediate-mode UI context. */
            ClawUIWorkphoneContext *m_workphoneContext = nullptr;

            /** Renderer bridge that converts WorkphoneCore output into draw data. */
            ClawUIWorkphoneRenderer *m_workphoneRenderer = nullptr;

            bool m_dragging = false;

            bool m_inputFrameOpen = false;

            Array<SmartPtr<IUIWindow>> m_windows;  ///< List of UI windows managed by the manager.

            SmartPtr<IUICursor> m_cursor;  ///< Cursor used for UI interaction.

            mutable RecursiveSpinMutex m_mutex;  ///< Mutex for thread-safe access to UI state.
        };

    }  // end namespace ui
}  // namespace workphone

#endif
