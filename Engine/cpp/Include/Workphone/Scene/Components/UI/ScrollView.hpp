#ifndef ScrollView_h__
#define ScrollView_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /** Component for a scroll view. */
        class WPCore_API ScrollView : public UIComponent
        {
        public:
            static const String scrollBarStr;
            static const String contentPanelStr;
            static const String lastDragPositionStr;
            static const String scrollSpeedStr;
            static const String inertiaStr;
            static const String velocityStr;
            static const String isDraggingStr;

            /** Constructor. */
            ScrollView();

            /** Destructor. */
            ~ScrollView() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::isValid */
            bool isValid() const override;

            /** @copydoc UIComponent::handleEvent */
            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            /** Get the content panel.
             * @return The content panel.
             */
            SmartPtr<LayoutTransform> getContentPanel() const;

            /** Set the content panel.
             * @param contentPanel The content panel to set.
             */
            void setContentPanel( SmartPtr<LayoutTransform> contentPanel );

            /** Get the scroll speed.
             * @return The scroll speed value.
             */
            f32 getScrollSpeed() const;

            /** Set the scroll speed.
             * @param scrollSpeed The scroll speed value to set.
             */
            void setScrollSpeed( f32 scrollSpeed );

            /** Get the inertia.
             * @return The inertia value.
             */
            f32 getInertia() const;

            /** Set the inertia.
             * @param inertia The inertia value to set.
             */
            void setInertia( f32 inertia );

            /** Get the last drag position.
             * @return The last drag position value.
             */
            Vector2<real_Num> getLastDragPosition() const;

            /** Set the last drag position.
             * @param lastDragPosition The last drag position value to set.
             */
            void setLastDragPosition( const Vector2<real_Num> &lastDragPosition );

            /** Get the current velocity.
             * @return The current velocity value.
             */
            f32 getVelocity() const;

            /** Set the current velocity.
             * @param velocity The velocity value to set.
             */
            void setVelocity( f32 velocity );

            /** Check if the scroll view is currently dragging.
             * @return True if dragging, false otherwise.
             */
            bool isDragging() const;

            /** Set the dragging state.
             * @param dragging The dragging state to set.
             */
            void setDragging( bool dragging );

            /** Get the scroll bar.
             * @return The scroll bar.
             */
            SmartPtr<ScrollBar> getScrollBar() const;

            /** Set the scroll bar.
             * @param scrollBar The scroll bar to set.
             */
            void setScrollBar( SmartPtr<ScrollBar> scrollBar );

            /** Update the scroll view. */
            void updateScrollBar();

            /** Update the scroll view. */
            void syncWithScrollBar();

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Handle the begin drag event. */
            void handleBeginDrag( SmartPtr<IInputEvent> inputEvent );

            /** Handle the drag event. */
            void handleDrag( SmartPtr<IInputEvent> inputEvent );

            /** Handle the end drag event. */
            void handleEndDrag( SmartPtr<IInputEvent> inputEvent );

            /** Handle the scroll event. */
            void handleScroll( SmartPtr<IInputEvent> inputEvent );

            /** Clamp the position to the bounds of the scroll view. */
            Vector2<real_Num> clampToBounds( const Vector2<real_Num> &pos );

            // The content panel that will be scrolled
            SmartPtr<ScrollBar> m_scrollBar;

            // Assign the content panel
            SmartPtr<LayoutTransform> m_contentPanel;

            // The last drag position
            Vector2<real_Num> m_lastDragPosition;

            // Drag speed
            f32 m_scrollSpeed = 10.0f;

            // Smooth scrolling effect
            f32 m_inertia = 0.95f;

            // The current velocity of the scroll view
            f32 m_velocity = 0.0f;

            // The current position of the scroll view
            bool m_isDragging = false;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ScrollView_h__
