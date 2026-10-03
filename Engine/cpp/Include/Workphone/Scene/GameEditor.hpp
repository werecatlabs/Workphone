#ifndef Editor_h__
#define Editor_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Scene/IGameEditor.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class Editor
         * @brief Concrete implementation of the IEditor interface for editor windows.
         *
         * The Editor class provides the core functionality for editor windows in the user interface.
         * It manages parent/child window relationships, visibility, event handling, drag-and-drop,
         * and scripting integration. This class is intended to be used as a base for specialized
         * editor windows and can be extended to provide custom editor behaviors.
         *
         * Key features:
         * - Parent and debug window management
         * - Visibility control
         * - Event listener and application listener support
         * - Drag-and-drop source/target management
         * - Scripting integration (invoker, receiver, script class/data)
         * - Thread-safe data access via mutex
         */
        class WPCore_API GameEditor : public IGameEditor
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the editor instance and its internal state.
             */
            GameEditor();

            /**
             * @brief Destructor.
             *
             * Cleans up resources held by the editor instance.
             */
            ~GameEditor() override;

            /**
             * @brief Loads the editor with the specified data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the editor and releases associated resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the parent UI window of this editor window.
             * @return Smart pointer to the parent UI window, or nullptr if none.
             */
            SmartPtr<ui::IUIWindow> getParent() const override;

            /**
             * @brief Sets the parent UI window for this editor window.
             * @param parent Smart pointer to the new parent UI window.
             */
            void setParent( SmartPtr<ui::IUIWindow> parent ) override;

            /**
             * @brief Gets the parent window (may be different from the logical parent).
             * @return Smart pointer to the parent window, or nullptr if none.
             */
            SmartPtr<ui::IUIWindow> getParentWindow() const override;

            /**
             * @brief Sets the parent window for this editor window.
             * @param parentWindow Smart pointer to the new parent window.
             */
            void setParentWindow( SmartPtr<ui::IUIWindow> parentWindow ) override;

            /**
             * @brief Gets the debug window associated with this editor window.
             * @return Smart pointer to the debug window, or nullptr if none.
             */
            SmartPtr<ui::IUIWindow> getDebugWindow() const override;

            /**
             * @brief Sets the debug window for this editor window.
             * @param debugWindow Smart pointer to the debug window.
             */
            void setDebugWindow( SmartPtr<ui::IUIWindow> debugWindow ) override;

            /**
             * @brief Checks if the editor window is currently visible.
             * @return True if the window is visible, false otherwise.
             */
            bool isWindowVisible() const override;

            /**
             * @brief Sets the visibility of the editor window.
             * @param visible True to make the window visible, false to hide it.
             */
            void setWindowVisible( bool visible ) override;

            /**
             * @brief Updates the selection state of the editor window.
             *
             * This may trigger UI updates or selection change events.
             */
            void updateSelection() override;

            /**
             * @brief Gets the event listener associated with this editor window.
             * @return Smart pointer to the event listener, or nullptr if none.
             */
            SmartPtr<IEventListener> getEventListener() const override;

            /**
             * @brief Sets the event listener for this editor window.
             * @param eventListener Smart pointer to the event listener.
             */
            void setEventListener( SmartPtr<IEventListener> eventListener ) override;

            /**
             * @brief Gets the drag source for this editor window.
             * @return Smart pointer to the drag source, or nullptr if none.
             */
            SmartPtr<ui::IUIDragSource> getWindowDragSource() const override;

            /**
             * @brief Sets the drag source for this editor window.
             * @param windowDragSource Smart pointer to the drag source.
             */
            void setWindowDragSource( SmartPtr<ui::IUIDragSource> windowDragSource ) override;

            /**
             * @brief Gets the drop target for this editor window.
             * @return Smart pointer to the drop target, or nullptr if none.
             */
            SmartPtr<ui::IUIDropTarget> getWindowDropTarget() const override;

            /**
             * @brief Sets the drop target for this editor window.
             * @param windowDropTarget Smart pointer to the drop target.
             */
            void setWindowDropTarget( SmartPtr<ui::IUIDropTarget> windowDropTarget ) override;

            /**
             * @brief Sets whether a UI element is draggable.
             * @param element Smart pointer to the UI element.
             * @param draggable True to make the element draggable, false otherwise.
             */
            void setDraggable( SmartPtr<ui::IUIElement> element, bool draggable ) override;

            /**
             * @brief Checks if a UI element is draggable.
             * @param element Smart pointer to the UI element.
             * @return True if the element is draggable, false otherwise.
             */
            bool isDraggable( SmartPtr<ui::IUIElement> element ) const override;

            /**
             * @brief Sets whether a UI element is droppable.
             * @param element Smart pointer to the UI element.
             * @param droppable True to make the element droppable, false otherwise.
             */
            void setDroppable( SmartPtr<ui::IUIElement> element, bool droppable ) override;

            /**
             * @brief Checks if a UI element is droppable.
             * @param element Smart pointer to the UI element.
             * @return True if the element is droppable, false otherwise.
             */
            bool isDroppable( SmartPtr<ui::IUIElement> element ) const override;

            /**
             * @brief Sets whether a UI element handles events.
             * @param element Smart pointer to the UI element.
             * @param handleEvents True to enable event handling, false otherwise.
             */
            void setHandleEvents( SmartPtr<ui::IUIElement> element, bool handleEvents ) override;

            /**
             * @brief Checks if a UI element handles events.
             * @param element Smart pointer to the UI element.
             * @return True if the element handles events, false otherwise.
             */
            bool getHandleEvents( SmartPtr<ui::IUIElement> element ) const override;

            /**
             * @brief Handles an event dispatched to this editor window.
             * @param eventType The type of the event.
             * @param eventValue The value or identifier of the event.
             * @param arguments Additional arguments for the event.
             * @param sender The object that triggered the event (may be nullptr).
             * @param object The object associated with the event (may be nullptr).
             * @param event Smart pointer to the event data (may be nullptr).
             * @return A Parameter containing the result of the event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Handles a drag event on a UI element.
             * @param position The position of the drag event in window coordinates.
             * @param element Smart pointer to the UI element being dragged.
             * @return A string representing the data to be dropped.
             */
            String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element ) override;

            /**
             * @brief Handles a drop event on a UI element.
             * @param position The position of the drop event in window coordinates.
             * @param element Smart pointer to the UI element being dropped on.
             * @param data The data being dropped.
             */
            void handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> element,
                             const String &data ) override;

            /**
             * @brief Destroys the associated script object, if any.
             *
             * This is used to clean up script resources when the editor is unloaded or destroyed.
             */
            void destroyScriptObject();

            /**
             * @brief Gets the script invoker associated with this editor window.
             * @return Smart pointer to the script invoker, or nullptr if none.
             */
            SmartPtr<IScriptInvoker> getInvoker() const override;

            /**
             * @brief Sets the script invoker for this editor window.
             * @param invoker Smart pointer to the script invoker.
             */
            void setInvoker( SmartPtr<IScriptInvoker> invoker ) override;

            /**
             * @brief Gets the script receiver associated with this editor window.
             * @return Smart pointer to the script receiver, or nullptr if none.
             */
            SmartPtr<IScriptReceiver> getReceiver() const override;

            /**
             * @brief Sets the script receiver for this editor window.
             * @param receiver Smart pointer to the script receiver.
             */
            void setReceiver( SmartPtr<IScriptReceiver> receiver ) override;

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
            /** @brief The class name of the script class. */
            AtomicObject<String> m_className;

            /** @brief The script class used by the window. */
            AtomicSmartPtr<IScriptClass> m_scriptClass;

            /** @brief Used to call script functions. */
            AtomicSmartPtr<IScriptInvoker> m_invoker;

            /** @brief Used to receive script calls. */
            AtomicSmartPtr<IScriptReceiver> m_receiver;

            /** @brief The data used by the script system. */
            AtomicSmartPtr<IScriptData> m_scriptData;

            /** @brief The drag source for the window. */
            AtomicSmartPtr<ui::IUIDragSource> m_windowDragSource;

            /** @brief The drop target for the window. */
            AtomicSmartPtr<ui::IUIDropTarget> m_windowDropTarget;

            /** @brief The event listener for the editor window. */
            AtomicSmartPtr<IEventListener> m_eventListener;

            /** @brief The application-level event listener. */
            AtomicSmartPtr<IEventListener> m_applicationListener;

            /** @brief The parent of the window. */
            AtomicWeakPtr<ui::IUIWindow> m_parent;

            /** @brief The parent window of the window. */
            AtomicWeakPtr<ui::IUIWindow> m_parentWindow;

            /** @brief The debug window associated with the editor. */
            AtomicSmartPtr<ui::IUIWindow> m_debugWindow;

            /** @brief True if the window is visible, false otherwise. */
            atomic_bool m_windowVisible = true;

            /** @brief True if the editor should update in edit mode. */
            atomic_bool m_updateInEditMode = false;

            /** @brief The array of shared objects used by the editor. */
            ConcurrentArray<SmartPtr<ISharedObject>> m_dataArray;

            /** @brief The mutex used to protect the data array and internal state. */
            mutable RecursiveMutex m_mutex;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Editor_h__
