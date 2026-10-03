#ifndef Layout_h__
#define Layout_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class Layout
         * @brief Layout component for arranging UI components in a specific manner.
         *
         * The Layout class is responsible for managing the arrangement and reference sizing of UI
         * components. It provides methods to set and retrieve the layout object, reference size, and to
         * handle events and properties related to the layout. It also manages the order and visibility
         * of UI elements within the layout.
         */
        class WPCore_API Layout : public UIComponent
        {
        public:
            static const String referenceSizeStr;

            // Property key strings for each PanelFlag — exposed to the editor as booleans.
            static const String flagBorderStr;
            static const String flagMovableStr;
            static const String flagScalableStr;
            static const String flagClosableStr;
            static const String flagMinimizableStr;
            static const String flagNoScrollbarStr;
            static const String flagTitleStr;
            static const String flagScrollAutoHideStr;
            static const String flagBackgroundStr;
            static const String flagScaleLeftStr;
            static const String flagNoInputStr;

            /**
             * @brief Default constructor.
             */
            Layout();

            /**
             * @brief Destructor.
             */
            ~Layout() override;

            /**
             * @brief Loads the layout component with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the layout component and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the layout object used for arranging UI components.
             * @return Smart pointer to the IUILayoutWindow object.
             */
            SmartPtr<ui::IUILayoutWindow> getLayout() const;

            /**
             * @brief Sets the layout object used for arranging UI components.
             * @param layout Smart pointer to the IUILayoutWindow object.
             */
            void setLayout( SmartPtr<ui::IUILayoutWindow> layout );

            /**
             * @brief Gets the reference size for the layout.
             * @return The reference size as a Vector2I.
             */
            Vector2I getReferenceSize() const;

            /**
             * @brief Sets the reference size for the layout.
             * @param referenceSize The new reference size as a Vector2I.
             */
            void setReferenceSize( const Vector2I &referenceSize );

            /**
             * @brief Gets the child objects managed by this layout component.
             * @return Array of smart pointers to child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the properties of the layout component.
             * @return Smart pointer to the Properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the layout component.
             * @param properties Smart pointer to the Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Updates the component's flags.
             * @param flags New flags value.
             * @param oldFlags Previous flags value.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Gets the order of a UI component within the layout.
             * @param component Smart pointer to the UIComponent.
             * @return The order index of the component.
             */
            u32 getElementOrder( SmartPtr<UIComponent> component ) const;

            /**
             * @brief Gets the reversed order of a UI component within the layout.
             * @param component Smart pointer to the UIComponent.
             * @return The reversed order index of the component.
             */
            u32 getElementOrderReversed( SmartPtr<UIComponent> component ) const;

            /**
             * @brief Gets the Z-order of an actor in the layout.
             * @param obj Smart pointer to the IActor object.
             * @return The Z-order value.
             */
            s32 getZOrder( SmartPtr<IGameActor> obj );

            /**
             * @brief Handles a generic event for the layout component.
             * @param eventType The type of event.
             * @param eventValue The event value (hashed).
             * @param arguments Array of event parameters.
             * @param sender Smart pointer to the sender object.
             * @param object Smart pointer to the related object.
             * @param event Smart pointer to the event object.
             * @return Parameter result of the event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Handles an input event for the layout component.
             * @param event Smart pointer to the input event.
             * @return True if the event was handled, false otherwise.
             */
            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            /**
             * @brief Handles an input event for a specific actor.
             * @param actor Smart pointer to the actor.
             * @param event Smart pointer to the input event.
             * @return True if the event was handled, false otherwise.
             */
            bool handleEvent( SmartPtr<IGameActor> actor, const SmartPtr<IInputEvent> &event );

            /**
             * @brief Creates the UI elements managed by this layout.
             */
            void createUI() override;

            PanelFlags getPanelFlags() const;

            void setPanelFlags( PanelFlags panelFlags );

            /**
             * @brief Returns true if all bits in @p flag are set in m_panelFlags.
             */
            bool hasPanelFlag( PanelFlags flag ) const;

            /**
             * @brief Sets one or more panel flags without clearing existing ones.
             */
            void addPanelFlag( PanelFlags flag );

            /**
             * @brief Clears one or more panel flags.
             */
            void removePanelFlag( PanelFlags flag );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handles component-specific events for the layout.
             * @param state The current state.
             * @param eventType The event type.
             * @return The result of the finite state machine event handling.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Updates the visibility of the layout and its children.
             */
            void updateVisibility() override;

            /**
             * @brief Updates the order of UI elements in the layout.
             */
            void updateOrder() override;

            /**
             * @brief Pushes m_panelFlags to the underlying IUILayoutWindow as WindowFlags.
             *
             * Called whenever m_panelFlags changes so the renderer always reflects the
             * data-driven state.
             */
            void applyFlagsToLayout();

            /**
             * @brief The layout object responsible for arranging UI components.
             */
            SmartPtr<ui::IUILayoutWindow> m_layout;

            /**
             * @brief The reference size used for scaling and arranging UI components.
             *        Defaults to 1920x1080.
             */
            Vector2I m_referenceSize = Vector2I( 1920, 1080 );

            /**
             * @brief Flags controlling layout behavior.
             */
            PanelFlags m_panelFlags = (PanelFlags)0;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Layout_h__
