#ifndef BaseWindow_h__
#define BaseWindow_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Scene/GameEditor.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @brief Base class for editor windows in the user interface.
         * 
         * The EditorWindow class provides functionality for managing editor windows, including
         * drag and drop operations, event handling, and window visibility control. It serves as
         * a base class for specialized editor windows in the application.
         */
        class EditorWindow : public scene::GameEditor
        {
        public:
            /**
             * @brief Listener class for handling application-level events.
             * 
             * This class is responsible for processing application-wide events and forwarding
             * them to the owning EditorWindow instance.
             */
            class ApplicationListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                ApplicationListener();

                /**
                 * @brief Destructor.
                 */
                ~ApplicationListener() override;

                /**
                 * @brief Handles application events.
                 * @param eventType The type of event.
                 * @param eventValue The hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The object that sent the event.
                 * @param object The target object of the event.
                 * @param event The event object.
                 * @return Parameter containing the result of event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner window of this listener.
                 * @return SmartPtr to the owner EditorWindow.
                 */
                SmartPtr<EditorWindow> getOwner() const;

                /**
                 * @brief Sets the owner window for this listener.
                 * @param owner The EditorWindow instance to set as owner.
                 */
                void setOwner( SmartPtr<EditorWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<EditorWindow> m_owner;
            };

            /**
             * @brief Listener class for handling UI-specific events.
             * 
             * This class processes UI-related events and manages their propagation to the
             * owning EditorWindow instance.
             */
            class UIListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                UIListener();

                /**
                 * @brief Destructor.
                 */
                ~UIListener() override;

                /**
                 * @brief Handles UI events.
                 * @param eventType The type of event.
                 * @param eventValue The hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The object that sent the event.
                 * @param object The target object of the event.
                 * @param event The event object.
                 * @return Parameter containing the result of event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Gets the owner window of this listener.
                 * @return SmartPtr to the owner EditorWindow.
                 */
                SmartPtr<EditorWindow> getOwner() const;

                /**
                 * @brief Sets the owner window for this listener.
                 * @param owner The EditorWindow instance to set as owner.
                 */
                void setOwner( SmartPtr<EditorWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<EditorWindow> m_owner;
            };

            /**
             * @brief Class for handling drag operations in the editor window.
             * 
             * This class implements the IUIDragSource interface to provide drag functionality
             * for UI elements within the editor window.
             */
            class UIDragSource : public ui::IUIDragSource
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                UIDragSource();

                /**
                 * @brief Destructor.
                 */
                ~UIDragSource() override;

                /**
                 * @brief Handles drag events.
                 * @param eventType The type of event.
                 * @param eventValue The hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The object that sent the event.
                 * @param object The target object of the event.
                 * @param event The event object.
                 * @return Parameter containing the result of event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Handles the drag operation.
                 * @param position The current mouse position.
                 * @param element The UI element being dragged.
                 * @return String containing drag data.
                 */
                String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element );

                /**
                 * @brief Gets the owner window of this drag source.
                 * @return SmartPtr to the owner EditorWindow.
                 */
                SmartPtr<EditorWindow> getOwner() const;

                /**
                 * @brief Sets the owner window for this drag source.
                 * @param owner The EditorWindow instance to set as owner.
                 */
                void setOwner( SmartPtr<EditorWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<EditorWindow> m_owner;
            };

            /**
             * @brief Class for handling drop operations in the editor window.
             * 
             * This class implements the IUIDropTarget interface to provide drop functionality
             * for UI elements within the editor window.
             */
            class UIDropTarget : public ui::IUIDropTarget
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                UIDropTarget();

                /**
                 * @brief Destructor.
                 */
                ~UIDropTarget() override;

                /**
                 * @brief Handles drop events.
                 * @param eventType The type of event.
                 * @param eventValue The hash value of the event.
                 * @param arguments Array of event parameters.
                 * @param sender The object that sent the event.
                 * @param object The target object of the event.
                 * @param event The event object.
                 * @return Parameter containing the result of event handling.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Handles the drop operation.
                 * @param position The current mouse position.
                 * @param src The source UI element.
                 * @param dst The destination UI element.
                 * @param data The data being dropped.
                 * @return bool True if the drop was successful, false otherwise.
                 */
                bool handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> src,
                                 SmartPtr<ui::IUIElement> dst, const String &data );

                /**
                 * @brief Gets the owner window of this drop target.
                 * @return SmartPtr to the owner EditorWindow.
                 */
                SmartPtr<EditorWindow> getOwner() const;

                /**
                 * @brief Sets the owner window for this drop target.
                 * @param owner The EditorWindow instance to set as owner.
                 */
                void setOwner( SmartPtr<EditorWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<EditorWindow> m_owner;
            };

            /**
             * @brief Default constructor.
             */
            EditorWindow();

            /**
             * @brief Destructor.
             */
            ~EditorWindow() override;

            /**
             * @brief Loads the window with the specified data.
             * @param data The data to load into the window.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reloads the window with the specified data.
             * @param data The data to reload into the window.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the window data.
             * @param data The data to unload from the window.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the debug window associated with this editor window.
             * @return SmartPtr to the debug window.
             */
            SmartPtr<ui::IUIWindow> getDebugWindow() const override;

            /**
             * @brief Sets the debug window for this editor window.
             * @param debugWindow The debug window to set.
             */
            void setDebugWindow( SmartPtr<ui::IUIWindow> debugWindow ) override;

            /**
             * @brief Checks if the window is visible.
             * @return bool True if the window is visible, false otherwise.
             */
            bool isWindowVisible() const override;

            /**
             * @brief Sets the window visibility.
             * @param visible True to make the window visible, false to hide it.
             */
            void setWindowVisible( bool visible ) override;

            /**
             * @brief Updates the current selection in the window.
             */
            void updateSelection() override;

            /**
             * @brief Gets the event listener for this window.
             * @return SmartPtr to the event listener.
             */
            SmartPtr<IEventListener> getEventListener() const override;

            /**
             * @brief Sets the event listener for this window.
             * @param eventListener The event listener to set.
             */
            void setEventListener( SmartPtr<IEventListener> eventListener ) override;

            /**
             * @brief Gets the drag source for this window.
             * @return SmartPtr to the drag source.
             */
            SmartPtr<ui::IUIDragSource> getWindowDragSource() const override;

            /**
             * @brief Sets the drag source for this window.
             * @param windowDragSource The drag source to set.
             */
            void setWindowDragSource( SmartPtr<ui::IUIDragSource> windowDragSource ) override;

            /**
             * @brief Gets the drop target for this window.
             * @return SmartPtr to the drop target.
             */
            SmartPtr<ui::IUIDropTarget> getWindowDropTarget() const override;

            /**
             * @brief Sets the drop target for this window.
             * @param windowDropTarget The drop target to set.
             */
            void setWindowDropTarget( SmartPtr<ui::IUIDropTarget> windowDropTarget ) override;

            /**
             * @brief Sets whether an element is draggable.
             * @param element The UI element to configure.
             * @param draggable True to make the element draggable, false otherwise.
             */
            void setDraggable( SmartPtr<ui::IUIElement> element, bool draggable ) override;

            /**
             * @brief Checks if an element is draggable.
             * @param element The UI element to check.
             * @return bool True if the element is draggable, false otherwise.
             */
            bool isDraggable( SmartPtr<ui::IUIElement> element ) const override;

            /**
             * @brief Sets whether an element can receive drops.
             * @param element The UI element to configure.
             * @param droppable True to make the element droppable, false otherwise.
             */
            void setDroppable( SmartPtr<ui::IUIElement> element, bool droppable ) override;

            /**
             * @brief Checks if an element can receive drops.
             * @param element The UI element to check.
             * @return bool True if the element is droppable, false otherwise.
             */
            bool isDroppable( SmartPtr<ui::IUIElement> element ) const override;

            /**
             * @brief Sets whether an element should handle events.
             * @param element The UI element to configure.
             * @param handleEvents True to make the element handle events, false otherwise.
             */
            void setHandleEvents( SmartPtr<ui::IUIElement> element, bool handleEvents ) override;

            /**
             * @brief Checks if an element handles events.
             * @param element The UI element to check.
             * @return bool True if the element handles events, false otherwise.
             */
            bool getHandleEvents( SmartPtr<ui::IUIElement> element ) const override;

            /**
             * @brief Handles general events for this window.
             * @param eventType The type of event.
             * @param eventValue The hash value of the event.
             * @param arguments Array of event parameters.
             * @param sender The object that sent the event.
             * @param object The target object of the event.
             * @param event The event object.
             * @return Parameter containing the result of event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Handles application-specific events.
             * @param eventType The type of event.
             * @param eventValue The hash value of the event.
             * @param arguments Array of event parameters.
             * @param sender The object that sent the event.
             * @param object The target object of the event.
             * @param event The event object.
             * @return Parameter containing the result of event handling.
             */
            virtual Parameter handleApplicationEvent( EventType eventType, hash_type eventValue,
                                                      const Array<Parameter> &arguments,
                                                      SmartPtr<ISharedObject> sender,
                                                      SmartPtr<ISharedObject> object,
                                                      SmartPtr<IEvent> event );

            /**
             * @brief Handles drag operations.
             * @param position The current mouse position.
             * @param element The UI element being dragged.
             * @return String containing drag data.
             */
            String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element ) override;

            /**
             * @brief Handles drop operations.
             * @param position The current mouse position.
             * @param element The UI element receiving the drop.
             * @param data The data being dropped.
             */
            void handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> element,
                             const String &data ) override;

            /**
             * @brief Gets the class name of this window.
             * @return String containing the class name.
             */
            String getClassName() const;

            /**
             * @brief Sets the class name for this window.
             * @param className The class name to set.
             */
            void setClassName( const String &className );

            /**
             * @brief Gets the script invoker for this window.
             * @return SmartPtr to the script invoker.
             */
            SmartPtr<IScriptInvoker> getInvoker() const;

            /**
             * @brief Sets the script invoker for this window.
             * @param invoker The script invoker to set.
             */
            void setInvoker( SmartPtr<IScriptInvoker> invoker );

            /**
             * @brief Gets the script receiver for this window.
             * @return SmartPtr to the script receiver.
             */
            SmartPtr<IScriptReceiver> getReceiver() const;

            /**
             * @brief Sets the script receiver for this window.
             * @param receiver The script receiver to set.
             */
            void setReceiver( SmartPtr<IScriptReceiver> receiver );

            /**
             * @brief Gets the script class for this window.
             * @return SmartPtr to the script class.
             */
            SmartPtr<IScriptClass> getScriptClass() const;

            /**
             * @brief Sets the script class for this window.
             * @param scriptClass The script class to set.
             */
            void setScriptClass( SmartPtr<IScriptClass> scriptClass );

            /**
             * @brief Locks the window for thread-safe operations.
             */
            void lock() override;

            /**
             * @brief Unlocks the window after thread-safe operations.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Adds data to the window.
             * @param data The data to add.
             */
            void addData( SmartPtr<ISharedObject> data );

            /**
             * @brief Removes data from the window.
             * @param data The data to remove.
             */
            void removeData( SmartPtr<ISharedObject> data );

            /**
             * @brief Clears all data from the window.
             */
            void clearData();

            /**
             * @brief Sets up event listeners for the window.
             */
            void setupListeners();

            /**
             * @brief Sets up application-level event listeners.
             */
            void setupApplicationListeners();

            /**
             * @brief Gets all data associated with the window.
             * @return Array of shared objects containing the window data.
             */
            Array<SmartPtr<ISharedObject>> getData() const;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // BaseWindow_h__
