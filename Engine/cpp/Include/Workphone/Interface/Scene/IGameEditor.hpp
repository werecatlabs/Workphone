#ifndef IEditor_h__
#define IEditor_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Interface/System/IEvent.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class IEditor
         * @brief Interface for an editor window object in the user interface.
         *
         * This interface defines the contract for editor windows, including parent/child relationships,
         * visibility, event handling, drag-and-drop, and scripting support. Implementations should
         * provide the logic for managing UI window hierarchies, user interactions, and integration with
         * the event system.
         */
        class WPCore_API IGameEditor : public ISharedObject
        {
        public:
            /** @brief String constant for the 'load' script command. */
            static const String loadStr;

            /** @brief String constant for the 'unload' script command. */
            static const String unloadStr;

            /** @brief String constant for the 'show' script command. */
            static const String showStr;

            /** @brief String constant for the 'hide' script command. */
            static const String hideStr;

            /**
             * @brief Virtual destructor.
             */
            ~IGameEditor() override;

            /**
             * @brief Gets the parent UI window of this editor window.
             * @return Smart pointer to the parent UI window, or nullptr if none.
             */
            virtual SmartPtr<ui::IUIWindow> getParent() const = 0;

            /**
             * @brief Sets the parent UI window for this editor window.
             * @param parent Smart pointer to the new parent UI window.
             */
            virtual void setParent( SmartPtr<ui::IUIWindow> parent ) = 0;

            /**
             * @brief Gets the parent window (may be different from the logical parent).
             * @return Smart pointer to the parent window, or nullptr if none.
             */
            virtual SmartPtr<ui::IUIWindow> getParentWindow() const = 0;

            /**
             * @brief Sets the parent window for this editor window.
             * @param parentWindow Smart pointer to the new parent window.
             */
            virtual void setParentWindow( SmartPtr<ui::IUIWindow> parentWindow ) = 0;

            /**
             * @brief Gets the debug window associated with this editor window.
             * @return Smart pointer to the debug window, or nullptr if none.
             */
            virtual SmartPtr<ui::IUIWindow> getDebugWindow() const = 0;

            /**
             * @brief Sets the debug window for this editor window.
             * @param debugWindow Smart pointer to the debug window.
             */
            virtual void setDebugWindow( SmartPtr<ui::IUIWindow> debugWindow ) = 0;

            /**
             * @brief Checks if the editor window is currently visible.
             * @return True if the window is visible, false otherwise.
             */
            virtual bool isWindowVisible() const = 0;

            /**
             * @brief Sets the visibility of the editor window.
             * @param visible True to make the window visible, false to hide it.
             */
            virtual void setWindowVisible( bool visible ) = 0;

            /**
             * @brief Updates the selection state of the editor window.
             *
             * This may trigger UI updates or selection change events.
             */
            virtual void updateSelection() = 0;

            /**
             * @brief Gets the event listener associated with this editor window.
             * @return Smart pointer to the event listener, or nullptr if none.
             */
            virtual SmartPtr<IEventListener> getEventListener() const = 0;

            /**
             * @brief Sets the event listener for this editor window.
             * @param eventListener Smart pointer to the event listener.
             */
            virtual void setEventListener( SmartPtr<IEventListener> eventListener ) = 0;

            /**
             * @brief Gets the drag source for this editor window.
             * @return Smart pointer to the drag source, or nullptr if none.
             */
            virtual SmartPtr<ui::IUIDragSource> getWindowDragSource() const = 0;

            /**
             * @brief Sets the drag source for this editor window.
             * @param windowDragSource Smart pointer to the drag source.
             */
            virtual void setWindowDragSource( SmartPtr<ui::IUIDragSource> windowDragSource ) = 0;

            /**
             * @brief Gets the drop target for this editor window.
             * @return Smart pointer to the drop target, or nullptr if none.
             */
            virtual SmartPtr<ui::IUIDropTarget> getWindowDropTarget() const = 0;

            /**
             * @brief Sets the drop target for this editor window.
             * @param windowDropTarget Smart pointer to the drop target.
             */
            virtual void setWindowDropTarget( SmartPtr<ui::IUIDropTarget> windowDropTarget ) = 0;

            /**
             * @brief Sets whether a UI element is draggable.
             * @param element Smart pointer to the UI element.
             * @param draggable True to make the element draggable, false otherwise.
             */
            virtual void setDraggable( SmartPtr<ui::IUIElement> element, bool draggable ) = 0;

            /**
             * @brief Checks if a UI element is draggable.
             * @param element Smart pointer to the UI element.
             * @return True if the element is draggable, false otherwise.
             */
            virtual bool isDraggable( SmartPtr<ui::IUIElement> element ) const = 0;

            /**
             * @brief Sets whether a UI element is droppable.
             * @param element Smart pointer to the UI element.
             * @param droppable True to make the element droppable, false otherwise.
             */
            virtual void setDroppable( SmartPtr<ui::IUIElement> element, bool droppable ) = 0;

            /**
             * @brief Checks if a UI element is droppable.
             * @param element Smart pointer to the UI element.
             * @return True if the element is droppable, false otherwise.
             */
            virtual bool isDroppable( SmartPtr<ui::IUIElement> element ) const = 0;

            /**
             * @brief Sets whether a UI element handles events.
             * @param element Smart pointer to the UI element.
             * @param handleEvents True to enable event handling, false otherwise.
             */
            virtual void setHandleEvents( SmartPtr<ui::IUIElement> element, bool handleEvents ) = 0;

            /**
             * @brief Checks if a UI element handles events.
             * @param element Smart pointer to the UI element.
             * @return True if the element handles events, false otherwise.
             */
            virtual bool getHandleEvents( SmartPtr<ui::IUIElement> element ) const = 0;

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
            virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                                           const Array<Parameter> &arguments,
                                           SmartPtr<ISharedObject> sender,
                                           SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) = 0;

            /**
             * @brief Handles a drag event on a UI element.
             * @param position The position of the drag event in window coordinates.
             * @param element Smart pointer to the UI element being dragged.
             * @return A string representing the data to be dropped.
             */
            virtual String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element ) = 0;

            /**
             * @brief Handles a drop event on a UI element.
             * @param position The position of the drop event in window coordinates.
             * @param element Smart pointer to the UI element being dropped on.
             * @param data The data being dropped.
             */
            virtual void handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> element,
                                     const String &data ) = 0;

            /**
             * @brief Gets the script invoker associated with this editor window.
             * @return Smart pointer to the script invoker, or nullptr if none.
             */
            virtual SmartPtr<IScriptInvoker> getInvoker() const = 0;

            /**
             * @brief Sets the script invoker for this editor window.
             * @param invoker Smart pointer to the script invoker.
             */
            virtual void setInvoker( SmartPtr<IScriptInvoker> invoker ) = 0;

            /**
             * @brief Gets the script receiver associated with this editor window.
             * @return Smart pointer to the script receiver, or nullptr if none.
             */
            virtual SmartPtr<IScriptReceiver> getReceiver() const = 0;

            /**
             * @brief Sets the script receiver for this editor window.
             * @param receiver Smart pointer to the script receiver.
             */
            virtual void setReceiver( SmartPtr<IScriptReceiver> receiver ) = 0;

            /**
             * @brief Registers the class for reflection or serialization.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // IEditor_h__
