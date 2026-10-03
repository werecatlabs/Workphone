#ifndef ScrollBar_h__
#define ScrollBar_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {
        /** ScrollBar component for UI. */
        class WPCore_API ScrollBar : public UIComponent
        {
        public:
            /** Constructor. */
            ScrollBar();

            /** Destructor. */
            ~ScrollBar() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::handleEvent */
            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            /** Gets handle actor. */
            SmartPtr<IGameActor> getHandleActor() const;

            /** Sets handle actor. */
            void setHandleActor( SmartPtr<IGameActor> handle );

            /** Gets the background. */
            SmartPtr<IGameActor> getBackground() const;

            /** Sets the background. */
            void setBackground( SmartPtr<IGameActor> background );

            /** Get the fill actor. */
            SmartPtr<IGameActor> getFill() const;

            /** Set the fill actor. */
            void setFill( SmartPtr<IGameActor> fill );

            /** Sets callback function. */
            void setCallbackFunction( std::function<void( hash_type )> callbackFunction );

            /** Gets the scroll value. */
            f32 getScrollValue() const;

            /** Sets the scroll value. */
            void setScrollValue( f32 value );

            /** Gets the size of the handle. */
            f32 getHandleSize() const;

            /** Sets the size of the handle. */
            void setHandleSize( f32 handleSize );

            /** Gets the direction of scroll bar. */
            Direction getDirection() const;

            /** Sets the direction of scroll bar. */
            void setDirection( Direction direction );

            /** Gets the scroll view. */
            ScrollView *getScrollView() const;

            /** Sets the scroll view. */
            void setScrollView( ScrollView *scrollView );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Updates handle position */
            void updateHandlePosition();

            /** Handles the scroll bar event. */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            // A pointer to the scrollable area.
            ScrollView *m_scrollView = nullptr;

            // The scroll bar direction.
            Direction m_direction = Direction::Horizontal;

            // The handle actor.
            SmartPtr<IGameActor> m_handleActor;

            // The background actor.
            SmartPtr<IGameActor> m_background;

            // The fill actor.
            SmartPtr<IGameActor> m_fill;

            // The size of the handle.
            f32 m_handleSize = 1.0f;

            // The scroll value.
            f32 m_scrollValue = 0.0f;

            // The callback function.
            std::function<void( hash_type )> m_callbackFunction;
        };

        inline f32 ScrollBar::getScrollValue() const
        {
            return m_scrollValue;
        }

        inline f32 ScrollBar::getHandleSize() const
        {
            return m_handleSize;
        }

        inline Direction ScrollBar::getDirection() const
        {
            return m_direction;
        }
    }  // namespace scene
}  // namespace workphone

#endif  // ScrollBar_h__
