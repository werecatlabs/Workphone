#ifndef __UIManager_h__
#define __UIManager_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UIManager
         * @brief Central manager for user interface objects and interactions.
         *
         * The UIManager coordinates creation, lifetime and high-level operations for UI
         * objects used by the application/editor. It acts as the entry point for
         * adding/removing UI elements and applications, dispatching UI messages, and
         * holding references to commonly used UI objects such as the main window and cursor.
         *
         * This class implements the IUIManager interface.
         */
        class WPCore_API UIManager : public IUIManager
        {
        public:
            /** @brief Construct a new UIManager. */
            UIManager();

            /** @brief Destroy the UIManager and release managed resources. */
            ~UIManager() override;

            /**
             * @brief Process queued UI messages or events.
             *
             * This method pumps the UI message queue, handling input and other queued
             * UI work. The function may be called regularly (for example once per frame)
             * to ensure UI remains responsive.
             *
             * @param data Optional shared object data passed into the pump (may be null).
             * @return The number of messages processed.
             */
            size_t messagePump( SmartPtr<ISharedObject> data ) override;

            void render() override;

            /**
             * @brief Create and register a new UI application instance.
             *
             * The created application is managed by the UIManager and can be retrieved
             * using getApplication().
             *
             * @return SmartPtr<IUIApplication> A smart pointer to the newly created application.
             */
            SmartPtr<IUIApplication> addApplication() override;

            /**
             * @brief Remove and destroy a previously added application.
             *
             * If the passed application is the current application managed by the UIManager,
             * it will be removed and its resources released.
             *
             * @param application Smart pointer to the application to remove.
             */
            void removeApplication( SmartPtr<IUIApplication> application ) override;

            /**
             * @brief Get the current UI application instance.
             *
             * @return SmartPtr<IUIApplication> Smart pointer to the managed application or null if none.
             */
            IUIApplication *getApplicationPtr() const override;

            /**
             * @brief Get the current UI application instance.
             *
             * @return SmartPtr<IUIApplication> Smart pointer to the managed application or null if none.
             */
            SmartPtr<IUIApplication> getApplication() const override;

            /**
             * @brief Set the active UI application instance.
             *
             * The UIManager will take ownership (via SmartPtr) of the provided application
             * reference for management.
             *
             * @param application Smart pointer to the application to set as active.
             */
            void setApplication( SmartPtr<IUIApplication> application ) override;

            /**
             * @brief Create and add a UI element of the specified type.
             *
             * The returned element is owned by the caller via SmartPtr and is registered
             * with the UIManager for lifecycle and lookup operations.
             *
             * @param type A hashed/type identifier for the element to create.
             * @return SmartPtr<IUIElement> Smart pointer to the created UI element or null on failure.
             */
            SmartPtr<IUIElement> addElement( hash64 type ) override;

            /**
             * @brief Remove and release a single UI element.
             *
             * The element will be unregistered from internal structures and its resources released.
             *
             * @param element Smart pointer to the element to remove.
             */
            void removeElement( SmartPtr<IUIElement> element ) override;

            /**
             * @brief Remove and release a collection of UI elements.
             *
             * Elements provided in @p elementsToRemove will be unregistered and released.
             *
             * @param elementsToRemove Array of smart pointers to the elements to remove.
             */
            void removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove ) override;

            Array<SmartPtr<IUIElement>> getElements() const;

            /**
             * @brief Remove all UI elements and reset the UIManager state.
             *
             * This will unregister and release all managed UI elements and return the
             * manager to an empty state.
             */
            void clear() override;

            /**
             * @brief Get the current UI cursor instance.
             *
             * @return SmartPtr<IUICursor> Smart pointer to the cursor or null if none.
             */
            SmartPtr<IUICursor> getCursor() const override;

            /**
             * @brief Find a UI element by its identifier.
             *
             * Performs a lookup of registered UI elements by their string identifier.
             *
             * @param id The identifier of the element to find.
             * @return SmartPtr<IUIElement> Smart pointer to the found element or null if not found.
             */
            SmartPtr<IUIElement> findElement( const String &id ) const override;

            /**
             * @brief Query whether a drag operation is currently active.
             *
             * @return true if an element is being dragged; false otherwise.
             */
            bool isDragging() const override;

            /**
             * @brief Set the dragging state.
             *
             * Used to indicate the start or end of a drag-and-drop operation within the UI.
             *
             * @param dragging true to mark dragging active; false to mark it inactive.
             */
            void setDragging( bool dragging ) override;

            /**
             * @brief Get the main UI window.
             *
             * @return SmartPtr<IUIWindow> Smart pointer to the main window or null if none set.
             */
            SmartPtr<IUIWindow> getMainWindow() const override;

            /**
             * @brief Set the main UI window.
             *
             * The main window is typically the primary container used by the application/editor.
             *
             * @param uiWindow Smart pointer to the window to set as main.
             */
            void setMainWindow( SmartPtr<IUIWindow> uiWindow ) override;

            /**
             * @brief Mark UI state as invalid and schedule a refresh.
             *
             * Call this when UI content needs to be redrawn or revalidated.
             */
            void invalidate() override;

            /**
             * @brief Internal helper to query the underlying native object pointer.
             *
             * Implements the ISharedObject-style interface for obtaining a raw object pointer.
             *
             * @param ppObject Pointer to a void* that will receive the raw object pointer.
             */
            void _getObject( void **ppObject ) override;

            /**
             * @brief Queue or immediately load a graphics/shared object required by the UI.
             *
             * If @p forceQueue is true the load will be forced into the load queue even if
             * immediate loading is possible.
             *
             * @param graphicsObject Shared graphics object to load.
             * @param forceQueue If true, force loading via the queue; otherwise allow immediate load.
             */
            void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            /**
             * @brief Queue or immediately unload a graphics/shared object used by the UI.
             *
             * If @p forceQueue is true the unload will be forced into the unload queue even if
             * immediate unloading is possible.
             *
             * @param graphicsObject Shared graphics object to unload.
             * @param forceQueue If true, force unloading via the queue; otherwise allow immediate
             * unload.
             */
            void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**< Managed UI cursor instance. */
            SmartPtr<IUICursor> m_cursor;

            /**< Main application window for the UI. */
            AtomicSmartPtr<IUIWindow> m_mainWindow;

            /**< True while a drag operation is active. */
            atomic_bool m_dragging = false;

            /**< Thread-safe container of UI elements managed by this class. */
            ConcurrentArray<SmartPtr<IUIElement>> m_elements;

            /**< Queue of resources to load on the UI thread. */
            ConcurrentQueue<SmartPtr<ISharedObject>> m_loadQueue;

            /**< Queue of resources to unload on the UI thread. */
            ConcurrentQueue<SmartPtr<ISharedObject>> m_unloadQueue;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // UIManager_h__
