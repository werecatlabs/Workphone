#ifndef UIComponent_h__
#define UIComponent_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Math/AABB2.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class UIComponent
         * @brief UIComponent is a user interface component that inherits from BaseComponent.
         *
         * This class provides functionality for UI elements and manages their properties,
         * state, visibility, input handling, and more.
         */
        class WPCore_API UIComponent : public Component
        {
        public:
            /**
             * @class UIElementListener
             * @brief A class that inherits from ui::CUIElementListener and listens for events on a
             * UIComponent.
             *
             * UIElementListener is designed to handle events and manage the relationship between the
             * listener and its owner (a UIComponent). The class provides methods to set and get the
             * owner, as well as handling events.
             */
            class WPCore_API UIElementListener : public IEventListener
            {
            public:
                /**
                 * @brief Default constructor.
                 */
                UIElementListener();

                /**
                 * @brief Default destructor.
                 */
                ~UIElementListener() override;

                /**
                 * @brief Handles the specified event type and updates the UIComponent.
                 * @param eventType The type of the event.
                 * @param eventValue The value associated with the event.
                 * @param arguments An array of parameters related to the event.
                 * @param sender A smart pointer to the object that sent the event.
                 * @param event A smart pointer to the event object.
                 * @return A Parameter object as a result of handling the event.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Retrieves the owner of this UIElementListener.
                 * @return A pointer to the UIComponent that owns this UIElementListener.
                 */
                SmartPtr<UIComponent> getOwner() const;

                /**
                 * @brief Sets the owner of this UIElementListener.
                 * @param owner A pointer to the UIComponent that will own this UIElementListener.
                 */
                void setOwner( SmartPtr<UIComponent> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /**
                 * @brief A pointer to the UIComponent that owns this UIElementListener.
                 */
                WeakPtr<UIComponent> m_owner;
            };

            static const String colourStr;
            static const String orderStr;
            static const String cascadeInputStr;
            static const String handleInputEventsStr;
            static const String resetStr;
            static const String updateTransformStr;
            static const String updateOrderStr;
            static const String updateVisibilityStr;
            static const String showLabelStr;
            static const String labelStr;
            static const String labelActorStr;
            static const String autoCalculateOrderStr;
            static const String layoutTransformStr;
            static const String editorWindowBorderSizeStr;
            static const String setupCanvasStr;
            static const String createUIStr;
            static const String updateElementStateStr;

            static const u8 cascadeInputFlag;
            static const u8 handleInputEventsFlag;
            static const u8 showLabelFlag;
            static const u8 autoCalculateOrderFlag;

            /**
             * @brief Default constructor.
             */
            UIComponent();

            /**
             * @brief Destructor.
             */
            ~UIComponent() override;

            /**
             * @brief Loads the UIComponent data.
             * @copydoc Component::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the UIComponent data.
             * @copydoc Component::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the UI element listener.
             * @return A smart pointer to the UI element listener.
             */
            SmartPtr<IEventListener> getElementListener() const;

            /**
             * @brief Sets the UI element listener.
             * @param elementListener A smart pointer to the new UI element listener.
             */
            void setElementListener( SmartPtr<IEventListener> elementListener );

            /**
             * @brief Gets the UI element.
             * @return A smart pointer to the UI element.
             */
            ui::IUIElement *getElementPtr() const;

            /**
             * @brief Gets the UI element.
             * @return A smart pointer to the UI element.
             */
            SmartPtr<ui::IUIElement> getElement() const;

            /**
             * @brief Sets the UI element.
             * @param element A smart pointer to the new UI element.
             */
            void setElement( SmartPtr<ui::IUIElement> element );

            /**
             * @brief Gets the canvas.
             * @return A pointer to the UIComponent canvas.
             */
            UIComponent *getCanvasPtr() const;

            /**
             * @brief Gets the canvas.
             * @return A smart pointer to the UIComponent canvas.
             */
            SmartPtr<UIComponent> getCanvas() const;

            /**
             * @brief Sets the canvas.
             * @param canvas A smart pointer to the new UIComponent canvas.
             */
            void setCanvas( SmartPtr<UIComponent> canvas );

            /**
             * @brief Updates the dimensions of the UI component.
             */
            virtual void updateDimensions();

            /**
             * @brief Updates the materials of the UI component.
             */
            void updateMaterials() override;

            /**
             * @brief Gets the child objects of the UI component.
             * @copydoc Component::getChildObjects
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the properties of the UI component.
             * @copydoc Component::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the UI component.
             * @copydoc Component::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Updates the dirty state of the UI component.
             * @copydoc Component::updateFlags
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Updates the transform of the UI component.
             * @copydoc Component::updateTransform
             */
            void updateTransform() override;

            /**
             * @brief Gets if to cascade the input.
             */
            bool getCascadeInput() const;

            /**
             * @brief Sets if to cascade the input.
             */
            void setCascadeInput( bool cascadeInput );

            /**
             * @brief Gets if the UI component should calculate its order from the layout hierarchy.
             */
            bool getAutoCalculateOrder() const;

            /**
             * @brief Sets if the UI component should calculate its order from the layout hierarchy.
             */
            void setAutoCalculateOrder( bool autoCalculateOrder );

            /**
             * @brief Updates the z order.
             */
            void updateOrder() override;

            /**
             * @brief Gets if the UI component is visible.
             * @return True if the UI component is visible, false otherwise.
             */
            Array<SmartPtr<IGameActor>> getActorListeners() const;

            /**
             * @brief Gets the component listeners.
             * @return An array of smart pointers to the component listeners.
             */
            Array<SmartPtr<IComponent>> getComponentListeners() const;

            /**
             * @brief Adds an actor listener to the UI component.
             * @param actor A smart pointer to the actor to be added as a listener.
             */
            void addActorListener( SmartPtr<IGameActor> actor );

            /**
             * @brief Removes an actor listener from the UI component.
             * @param actor A smart pointer to the actor to be removed as a listener.
             */
            void removeActorListener( SmartPtr<IGameActor> actor );

            /**
             * @brief Adds a component listener to the UI component.
             * @param component A smart pointer to the component to be added as a listener.
             */
            void addListener( SmartPtr<IComponent> component );

            /**
             * @brief Removes a component listener from the UI component.
             * @param component A smart pointer to the component to be removed as a listener.
             */
            void removeListener( SmartPtr<IComponent> component );

            /**
             * @brief Gets the UI component's colour.
             * @return The colour of the UI component.
             */
            ColourF getColour() const;

            /**
             * @brief Sets the UI component's colour.
             * @param colour The new colour to be set for the UI component.
             */
            void setColour( const ColourF &colour );

            /**
             * @brief Handles input events for the UI component.
             * @param event A smart pointer to the input event to be handled.
             * @return True if the event was handled, false otherwise.
             */
            virtual bool handleEvent( const SmartPtr<IInputEvent> &event );

            /**
             * @brief Handles input events for the UI component.
             * @param event A smart pointer to the input event to be handled.
             * @return True if the event was handled, false otherwise.
             */
            bool getHandleInputEvents() const;

            /**
             * @brief Sets if the UI component should handle input events.
             * @param handleInputEvents True to enable input event handling, false to disable.
             */
            void setHandleInputEvents( bool handleInputEvents );

            /**
             * @brief Gets if the label should be shown for the UI component.
             * @return True if the label should be shown, false otherwise.
             */
            bool getShowLabel() const;

            /**
             * @brief Sets if the label should be shown for the UI component.
             * @param showLabel True to show the label, false to hide it.
             */
            void setShowLabel( bool showLabel );

            /**
             * @brief Gets the label for the UI component.
             * @return The label of the UI component.
             */
            String getLabel() const;

            /**
             * @brief Sets the label for the UI component.
             * @param label The new label to be set for the UI component.
             */
            void setLabel( const String &label );

            /**
             * @brief Gets the label actor for the UI component.
             * @return A smart pointer to the label actor associated with the UI component.
             */
            SmartPtr<IGameActor> getLabelActor() const;

            /**
             * @brief Sets the label actor for the UI component.
             * @param labelActor A smart pointer to the new label actor to be set.
             */
            void setLabelActor( SmartPtr<IGameActor> labelActor );

            /**
             * @brief Gets the layout transform for the UI component.
             * @return A pointer to the LayoutTransform associated with the UI component.
             */
            LayoutTransform *getLayoutTransform() const;

            /**
             * @brief Sets the layout transform for the UI component.
             * @param layoutTransform A pointer to the new LayoutTransform to be set.
             */
            void setLayoutTransform( LayoutTransform *layoutTransform );

            /**
             * @brief Gets the editor window border size in pixels used when mapping mouse positions.
             * @return The editor window border size in pixels.
             */
            Vector2<real_Num> getEditorWindowBorderSize() const;

            /**
             * @brief Sets the editor window border size in pixels used when mapping mouse positions.
             * @param editorWindowBorderSize The editor window border size in pixels.
             */
            void setEditorWindowBorderSize( const Vector2<real_Num> &editorWindowBorderSize );

            /**
             * @brief Updates the visibility of the UIComponent based on its current state.
             *
             * This method is called to refresh the visibility of the UIComponent when its state changes.
             * The default implementation can be overridden by derived classes if custom behavior is
             * needed.
             */
            void updateVisibility() override;

            /**
             * @brief Updates the state of UI elements within the UIComponent.
             *
             * This method is called to refresh the state of UI elements based on the current state
             * of the UIComponent. It should be implemented by derived classes if additional state
             * management is needed.
             */
            virtual void updateElementState();

            /** @brief Updates the element colour. */
            virtual void updateColour();

            /**
             * Returns the component's final layout rectangle in canvas reference
             * coordinates.
             * This uses the absolute layout minimum and maximum and is
             * therefore suitable for
             * visualising nested UI hit regions.
             */
            AABB2<real_Num> getDebugBounds() const;

            /** Draws the component's final layout rectangle using a 2D debug-capable renderer. */
            void drawDebugBounds( render::IRenderer2 &renderer,
                                  const ColourF &colour = ColourF::Green ) const;

            /** Returns whether the UI component draws its layout rectangle every frame. */
            bool isDebugDrawEnabled() const;

            /** Enables automatic layout rectangle drawing when a 2D renderer is configured. */
            void setDebugDrawEnabled( bool enabled );

            /** @copydoc ISharedObject::postUpdate */
            void postUpdate() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handles the specified event type and updates the UIComponent.
             *
             * @param eventType The type of the event.
             * @param eventValue The value associated with the event.
             * @param arguments An array of parameters related to the event.
             * @param sender A smart pointer to the object that sent the event.
             * @param event A smart pointer to the event object.
             * @return A Parameter object as a result of handling the event.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /**
             * @brief Handles component events within the context of the finite state machine (FSM).
             *
             * @param state The current state of the FSM.
             * @param eventType The type of event to handle.
             * @return The return type of the FSM after handling the event.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Sets up the canvas for the UIComponent.
             *
             * This method is responsible for initializing the canvas for the UIComponent.
             * It should be implemented by derived classes if additional setup is needed.
             */
            virtual void setupCanvas();

            /**
             * @brief Creates the user interface for the UIComponent.
             *
             * This method is responsible for creating and initializing UI elements.
             * It should be implemented by derived classes to define the UI structure.
             */
            virtual void createUI();

            /** Gets the bounds. */
            AABB2<real_Num> getBounds() const;

            /** Gets the bounds. */
            AABB2<real_Num> getBounds( SmartPtr<IGameActor> actor ) const;

            /** Get the local mouse position. */
            bool getLocalMousePosition( const SmartPtr<IInputEvent> &event,
                                        Vector2<real_Num> &localPos ) const;

            /** Get the local mouse position. */
            bool getLocalMousePosition( const SmartPtr<IInputEvent> &event, SmartPtr<IGameActor> actor,
                                        Vector2<real_Num> &localPos ) const;

            // The element colour.
            AtomicObject<ColourF> m_colour = ColourF::White;

            // The layout transform.
            mutable AtomicRawPtr<LayoutTransform> m_layoutTransform;

            // An actor for the label object.
            AtomicSmartPtr<IGameActor> m_labelActor;

            /**
             * @brief A smart pointer to the UIElementListener object responsible for handling UI events.
             */
            AtomicSmartPtr<IEventListener> m_elementListener;

            /**
             * @brief A smart pointer to the IUIElement object representing a UI element in the
             * UIComponent.
             */
            AtomicSmartPtr<ui::IUIElement> m_element;

            /**
             * @brief A smart pointer to the UIComponent object representing the canvas on which the
             * UIComponent is drawn.
             */
            AtomicSmartPtr<UIComponent> m_canvas;

            // Flags for the UI component.
            AtomicObject<u8> m_uiComponentFlags;

            // The actor listeners.
            ConcurrentArray<SmartPtr<IGameActor>> m_actorListeners;

            // The component listeners.
            ConcurrentArray<SmartPtr<IComponent>> m_componentListeners;

            // The label.
            AtomicObject<FixedString<128>> m_label;

            // The editor window border size in pixels used by mouse hit testing.
            AtomicObject<Vector2<real_Num>> m_editorWindowBorderSize = Vector2<real_Num>( 0.0f, 20.0f );

            // Debug-only runtime state; intentionally not serialized with component properties.
            atomic_bool m_debugDrawEnabled = false;
        };

        inline ColourF UIComponent::getColour() const
        {
            return m_colour;
        }

        inline ui::IUIElement *UIComponent::getElementPtr() const
        {
            return m_element.get();
        }

        inline UIComponent *UIComponent::getCanvasPtr() const
        {
            return m_canvas.get();
        }

    }  // namespace scene
}  // namespace workphone

#endif  // UIComponent_h__
