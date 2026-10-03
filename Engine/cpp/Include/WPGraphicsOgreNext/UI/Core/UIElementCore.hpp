#ifndef __UIElementCore_h__
#define __UIElementCore_h__

/**
 * @file UIElementOgreNext.hpp
 * @brief Core/Ogre-backed UI element base template used by the render UI.
 *
 * This header declares `UIElementOgreNext<T>`, a CRTP template providing
 * common functionality required by UI elements implemented with the
 * Core GUI library and the Ogre render system. It integrates with the
 * engine's state system, exposes hooks for child/parent relationships,
 * and manages an optional Core widget attachment.
 *
 * The template parameter `T` is expected to be the concrete derived type
 * (CRTP) and to provide several methods used by this base, such as
 * `getObjectListeners()`, `getLayout()`, `getLoadingState()` and
 * other prototype-ish behavior.
 */

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/State/Messages/StateMessageObject.hpp>
#include <Workphone/State/Messages/StateMessageMaterial.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/UIElementStateData.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>
#include <Workphone/System/Prototype.hpp>
#include <workphone_prerequisites.h>

struct wp_widget
{
    /* This is a placeholder for the actual wp_widget structure defined in workphone_widget.h.
     * The real structure would include fields relevant to the Core widget representation.
     */
    int dummy; /* Placeholder field */
};

namespace workphone
{
    namespace ui
    {

        /**
         * @brief CRTP base class for UI elements backed by Core + Ogre.
         *
         * This template implements common UI element behavior needed by the engine's
         * RenderUI. It handles parent/child management, visibility/enabled state
         * notifications, focus/select/activation propagation, state context creation
         * and handling, and convenient integration with a wp_widget when one
         * exists.
         *
         * The concrete element type should be provided as the template parameter `T`.
         * `T` is expected to derive from engine prototype infrastructure and to
         * implement or expose certain helper methods (for example layout and
         * listener accessors) used by this base class.
         *
         * @tparam T Concrete derived UI element type (CRTP).
         */
        template <class T>
        class UIElementCore : public T
        {
        public:
            /**
             * @brief Construct a UI element.
             *
             * Generates a unique default name for the element and initializes
             * internal pointers to null. Concrete derived classes may set up
             * a Core widget or state listeners later during load.
             */
            UIElementCore();

            /**
             * @brief Virtual destructor.
             *
             * Ensures attached state context (if present) is destroyed and that
             * any loaded resources are unloaded.
             */
            ~UIElementCore() override;

            /**
             * @brief Unload and release renderer/UI resources owned by this element.
             *
             * This removes any Core widget listeners, destroys the associated
             * Core widget using the Core manager (if present), clears the
             * container link and removes all children. The operation is guarded by
             * the graphics system lock since it manipulates render/UI objects.
             *
             * @param data Optional user data passed by the caller (unused).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Per-frame update for the element.
             *
             * The base implementation early-exits if the element is not visible
             * or not enabled. When executed on the application task, it will
             * recursively call `update()` on children. Derived classes should
             * call the base implementation and then perform element-specific
             * per-frame logic.
             */
            void update() override;

            /**
             * @brief Handle a platform or user input event.
             *
             * Default implementation does not consume the event (returns false).
             * Derived components override to handle mouse, keyboard or other
             * input events targeted at this element.
             *
             * @param event Input event object.
             * @return true if the event was handled and should not be propagated.
             */
            virtual bool handleEvent( const SmartPtr<IInputEvent> &event );

            /**
             * @brief Add a child element to this element.
             *
             * If called on a task that is not the render/state task the operation
             * will be queued as a state message and applied on the render/state task.
             *
             * @param child Smart pointer to the child element to add.
             */
            void addChild( SmartPtr<IUIElement> child ) override;

            /**
             * @brief Add an animator to the element.
             *
             * Base class is a no-op; concrete element types may store and use
             * animators to drive timed changes.
             *
             * @param animator Reference to an animator smart pointer.
             */
            void addAnimator( SmartPtr<IAnimator> &animator );

            /**
             * @brief Remove an animator from the element.
             *
             * Base class returns false by default. Derived classes should implement
             * removal and return true when an animator was removed.
             *
             * @param animator Animator instance to remove.
             * @return true if removed; otherwise false.
             */
            bool removeAnimator( SmartPtr<IAnimator> &animator );

            /**
             * @brief Get the container that owns this element.
             * @return Smart pointer to the IUIContainer or nullptr if none.
             */
            SmartPtr<IUILayoutContainer> getContainer() const;

            /**
             * @brief Set the container that owns this element.
             * @param container Smart pointer to the IUIContainer.
             */
            void setContainer( SmartPtr<IUILayoutContainer> container );

            /**
             * @brief Retrieve a native (raw) object pointer associated with this element.
             *
             * Default implementation sets `*ppObject` to nullptr. Derived implementations
             * may provide a platform-specific pointer for interop.
             *
             * @param ppObject Out parameter receiving the raw pointer.
             */
            void _getObject( void **ppObject ) const;

            /** @copydoc IComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            Vector2F getReferenceSize() const;
            void setReferenceSize( const Vector2F &referenceSize );

            /**
             * @brief Return child objects associated with this element for
             * serialization/inspection and ownership traversal.
             *
             * The base implementation returns commonly exposed child objects like
             * drag/drop sources and targets. Derived classes can extend this list.
             *
             * @return Array of shared object smart pointers.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Handle notification that the associated state object changed.
             *
             * Derived classes should override to apply state data (for transform,
             * visibility, material, etc.) to renderer-specific widgets or objects.
             *
             * @param state Reference to the changed state object.
             * @return true if the change was applied and consumed; otherwise false.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Query whether it is safe to operate on this element in the current task.
             *
             * This checks the element's loading state and compares the current thread/task
             * against the graphics system's render task.
             *
             * @return true if thread/task context is appropriate for render operations.
             */
            bool isThreadSafe() const;

            /**
             * @brief Queue a state message for this element using the graphics system's
             * state task identifier.
             *
             * If the element has a state context, the message is enqueued on the
             * graphics system's state task so it will be processed on the correct thread.
             *
             * @param message State message to enqueue.
             */
            void addMessage( SmartPtr<IStateMessage> message );

            /**
             * @brief Configure or update a render material used by this element.
             *
             * Default implementation is a no-op; renderer-specific derived elements
             * should override to apply the material to their underlying primitives.
             *
             * @param material Material object to apply.
             */
            virtual void setupMaterial( SmartPtr<render::IMaterial> material );

            /**
             * @brief Get the associated Core widget pointer.
             * @return Raw pointer to the wp_widget or nullptr if not set.
             */
            struct wp_widget *getWidget() const;

            /**
             * @brief Associate a Core widget with this element.
             *
             * When a widget is assigned the internal widget listener (if present)
             * is automatically registered/unregistered with the widget.
             *
             * @param widget Raw wp_widget pointer to associate, or nullptr to clear.
             */
            void setWidget( struct wp_widget *widget );

            /** @brief Mark this element as requiring revalidation / layout (no-op in base). */
            void invalidate();

            /** @brief Acquire the UI/render lock via RenderUI for thread-safe operations. */
            void lock();

            /** @brief Release the UI/render lock via RenderUI. */
            void unlock();

            /**
             * @brief Returns the parent prototype for prototype chaining.
             *
             * The base implementation returns nullptr.
             */
            SmartPtr<core::IPrototype> getParentPrototype() const;

            /**
             * @brief Sets the parent prototype for prototype chaining.
             *
             * The base implementation does nothing. Concrete prototype classes may
             * override to maintain prototype relationships.
             *
             * @param prototype Smart pointer to the parent prototype to set.
             */
            virtual void setParentPrototype( SmartPtr<core::IPrototype> prototype );

            WP_CLASS_REGISTER_TEMPLATE_DECL( Prototype, T );

        protected:
            /**
             * @brief Listener that forwards state messages and change notifications
             *        to its owning UI element instance.
             *
             * This inner class implements IStateListener and holds a weak reference
             * to the owner element. When it receives state messages it will acquire
             * the graphics lock as necessary and call into the owner to perform the
             * real work on the render/state thread.
             */
            class ElementStateListener : public IStateListener
            {
            public:
                ElementStateListener() = default;
                ~ElementStateListener() override = default;

                /**
                 * @brief Forward an incoming state message to the owner element.
                 *
                 * The listener will handle standard messages such as add/remove child
                 * or material updates and call the corresponding owner methods.
                 *
                 * @param message The incoming state message.
                 * @return true if the message was handled and consumed.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Receive notification that an IState object changed and forward
                 *        the change to the owner.
                 *
                 * @param state The changed state object.
                 * @return true if the owner applied the state change.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /** @brief Get the owner element (strong pointer). */
                SmartPtr<IUIElement> getOwner() const;

                /** @brief Set the owner element. */
                void setOwner( SmartPtr<IUIElement> owner );

            protected:
                AtomicWeakPtr<IUIElement> m_owner; /**< Weak pointer to owning element. */
            };

            /**
             * @brief Handle a simple named internal event.
             *
             * Default implementation does nothing. Derived classes or frameworks
             * may use text-based event identifiers to trigger element-specific logic.
             *
             * @param eventType String identifier describing the event.
             */
            virtual void onEvent( const String &eventType );

            /** @brief Create and configure the element's state context and listener. */
            virtual void createStateContext();

            /** @brief Destroy and unregister the element's state context if present. */
            virtual void destroyStateContext();

            /**< Weak reference to the UI container this element belongs to. */
            AtomicWeakPtr<IUILayoutContainer> m_container;

            struct wp_widget *m_widget = nullptr; /**< Associated Core widget (if any). */

            Vector2F m_referenceSize = Vector2F( 1920.0f, 1080.0f );

            /// Used to store user properties (may be lazily created).
            mutable SmartPtr<ISharedObject> m_userProperties;

            /// Counter used to generate unique element default names.
            static u32 m_nextGeneratedNameExt;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, UIElementCore, T, core::Prototype<T> );

        template <class T>
        u32 UIElementCore<T>::m_nextGeneratedNameExt = 0;

        template <class T>
        UIElementCore<T>::UIElementCore()
        {
            static const String uiElementName = "UIElement";
            auto name = uiElementName + StringUtil::toString( m_nextGeneratedNameExt++ );
            UIElementCore<T>::setName( name );
        }

        template <class T>
        UIElementCore<T>::~UIElementCore()
        {
            this->destroyStateContext();

            if( UIElementCore<T>::isLoaded() )
            {
                unload( nullptr );
            }
        }

        template <class T>
        void UIElementCore<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                this->removeAllChildren();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void UIElementCore<T>::update()
        {
            // sort by order
            auto children = this->getChildren();
            std::sort( children.begin(), children.end(),
                       []( const SmartPtr<IUIElement> &a, const SmartPtr<IUIElement> &b ) {
                           return a->getOrder() < b->getOrder();
                       } );

            for( auto child : children )
            {
                child->update();
            }
        }

        template <class T>
        void UIElementCore<T>::addChild( SmartPtr<IUIElement> child )
        {
            T::addChild( child );

            if( child )
            {
                if( !this->template isDerived<ui::IUILayoutWindow>() )
                {
                    auto layout = T::getLayout();
                    child->setLayout( layout );
                }

                this->updateZOrder();
            }
        }

        template <class T>
        void UIElementCore<T>::addAnimator( SmartPtr<IAnimator> &animator )
        {
        }

        template <class T>
        bool UIElementCore<T>::removeAnimator( SmartPtr<IAnimator> &animator )
        {
            return false;
        }

        template <class T>
        SmartPtr<IUILayoutContainer> UIElementCore<T>::getContainer() const
        {
            auto p = m_container.load();
            return p.lock();
        }

        template <class T>
        void UIElementCore<T>::setContainer( SmartPtr<IUILayoutContainer> container )
        {
            m_container = container;
        }

        template <class T>
        void UIElementCore<T>::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }

        template <class T>
        bool UIElementCore<T>::handleEvent( const SmartPtr<IInputEvent> &event )
        {
            auto applicationManager = core::IApplicationManager::instance();

            //switch( auto eventType = event->getEventType() )
            //...
            // Default base class does not handle input.
            return false;
        }

        template <class T>
        bool UIElementCore<T>::isThreadSafe() const
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto renderTask = graphicsSystem->getRenderTask();

            auto task = Thread::getCurrentTask();

            const auto &loadingState = T::getLoadingState();

            return loadingState == LoadingState::Loaded && task == renderTask;
        }

        template <class T>
        void UIElementCore<T>::addMessage( SmartPtr<IStateMessage> message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            if( auto stateContext = UIElementCore<T>::getStateContext() )
            {
                const auto stateTask = graphicsSystem->getStateTask();
                stateContext->addMessage( stateTask, message );
            }
        }

        template <class T>
        void UIElementCore<T>::onEvent( const String &eventType )
        {
            // for (u32 i = 0; i < m_children.size(); ++i)
            //	((UIElement*)m_children[i])->onEvent(eventType);

            // for (u32 i = 0; i < m_listeners.size(); ++i)
            //	m_listeners[i]->onEvent(eventType);
        }

        template <class T>
        SmartPtr<Properties> UIElementCore<T>::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();

            auto size = this->getSize();
            auto position = this->getPosition();
            auto enabled = this->isEnabled();
            auto visible = this->isVisible();
            auto colour = this->getColour();
            auto sameLine = this->getSameLine();
            auto order = this->getOrder();
            auto referenceSize = this->getReferenceSize();

            properties->setProperty( "size", size );
            properties->setProperty( "position", position );
            properties->setProperty( "enabled", enabled );
            properties->setProperty( "visible", visible );
            properties->setProperty( "colour", colour );
            properties->setProperty( "same_line", sameLine );
            properties->setProperty( "order", order );
            properties->setProperty( "reference_size", referenceSize );
            properties->setButtonPressed( "make dirty" );
            return properties;
        }

        template <class T>
        void UIElementCore<T>::setProperties( SmartPtr<Properties> properties )
        {
            auto enabled = this->isEnabled();
            auto visible = this->isVisible();
            auto colour = this->getColour();
            auto order = this->getOrder();
            auto sameLine = this->getSameLine();
            auto size = this->getSize();
            auto position = this->getPosition();
            auto referenceSize = this->getReferenceSize();

            properties->getPropertyValue( "size", size );
            properties->getPropertyValue( "position", position );
            properties->getPropertyValue( "enabled", enabled );
            properties->getPropertyValue( "visible", visible );
            properties->getPropertyValue( "colour", colour );
            properties->getPropertyValue( "same_line", sameLine );
            properties->getPropertyValue( "order", order );
            properties->getPropertyValue( "reference_size", referenceSize );

            this->setSize( size );
            this->setPosition( position );
            this->setEnabled( enabled );
            this->setVisible( visible );
            this->setSameLine( sameLine );
            this->setColour( colour );
            this->setReferenceSize( referenceSize );

            if( properties->isButtonPressed( "make dirty" ) )
            {
                if( auto stateContext = this->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }
        }

        template <class T>
        Vector2F UIElementCore<T>::getReferenceSize() const
        {
            return m_referenceSize;
        }

        template <class T>
        void UIElementCore<T>::setReferenceSize( const Vector2F &referenceSize )
        {
            m_referenceSize = Vector2F( MathF::max( referenceSize.X(), 1.0f ),
                                        MathF::max( referenceSize.Y(), 1.0f ) );
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> UIElementCore<T>::getChildObjects() const
        {
            auto objects = Array<SmartPtr<ISharedObject>>();
            objects.reserve( 32 );

            objects.push_back( this->getDragSource() );
            objects.push_back( this->getDropTarget() );

            return objects;
        }

        template <class T>
        void UIElementCore<T>::createStateContext()
        {
            WP_ASSERT( this->getStateContext() == nullptr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );

            auto listener = factoryManager->make_ptr<ElementStateListener>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( this->getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            state->setData( stateData );

            auto transformState = factoryManager->make_ptr<State>();
            transformState->setId( this->getId() );
            transformState->setOwner( this );
            stateContext->addState( transformState );

            auto transformStateData = factoryManager->make_ptr<UITransformStateData>();
            transformState->setData( transformStateData );

            this->setStateContext( stateContext );
            this->setStateListener( listener );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

        template <class T>
        void UIElementCore<T>::destroyStateContext()
        {
            if( auto stateContext = this->getStateContext() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager->getStateManagerPtr();

                if( auto stateListener = this->getStateListener() )
                {
                    stateListener->unload( nullptr );
                    stateContext->removeStateListener( stateListener );
                    this->setStateListener( nullptr );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                this->setStateContext( nullptr );
            }
        }

        template <class T>
        bool UIElementCore<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto renderUI = applicationManager->getRenderUI();
            WP_ASSERT( renderUI );

            ScopedLock lock( graphicsSystem );

            if( UIElementCore<T>::isLoaded() )
            {
                auto stateData = state->getData();

                if( stateData->isDerived<UITransformStateData>() )
                {
                    auto transformStateData =
                        workphone::static_pointer_cast<UITransformStateData>( stateData );

                    auto position = transformStateData->position;
                    auto size = transformStateData->size;
                    auto zOrder = transformStateData->zorder;

                    auto iPosition = Vector2I::zero();
                    auto iSize = Vector2I::zero();

                    if( auto uiWindow = applicationManager->getSceneRenderWindow() )
                    {
                        auto sceneWindowPosition = uiWindow->getPosition();
                        auto sceneWindowSize = uiWindow->getSize();

                        sceneWindowSize = m_referenceSize;

                        auto pos = position * sceneWindowSize;
                        auto sz = size * sceneWindowSize;

                        iPosition = Vector2I( (s32)pos.X(), (s32)pos.Y() );
                        iSize = Vector2I( (s32)sz.X(), (s32)sz.Y() );
                    }
                    else
                    {
                        if( auto mainWindow = applicationManager->getWindow() )
                        {
                            auto mainWindowSize = mainWindow->getSize();
                            auto mainWindowSizeF =
                                Vector2F( (f32)mainWindowSize.x, (f32)mainWindowSize.y );

                            mainWindowSizeF = m_referenceSize;

                            auto pos = position * mainWindowSizeF;
                            auto sz = size * mainWindowSizeF;

                            iPosition = Vector2I( (s32)pos.X(), (s32)pos.Y() );
                            iSize = Vector2I( (s32)sz.X(), (s32)sz.Y() );
                        }
                    }
                }
                else if( stateData->isDerived<UIElementStateData>() )
                {
                    auto elementState = workphone::static_pointer_cast<UIElementStateData>( stateData );

                    auto inheritsPick = BitUtil::getFlagValue( elementState->flags,
                                                               (u16)IUIElement::handleInputEventsFlag );

                    auto visible =
                        BitUtil::getFlagValue( elementState->flags, (u16)IUIElement::visibleFlag );

                    return true;
                }
            }

            return false;
        }

        template <class T>
        bool UIElementCore<T>::ElementStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            ScopedLock lock( graphicsSystem );

            auto pOwner = getOwner();
            if( auto owner = workphone::dynamic_pointer_cast<UIElementCore<T>>( pOwner ) )
            {
                if( message->isExactly<StateMessageObject>() )
                {
                    auto objectMessage = workphone::static_pointer_cast<StateMessageObject>( message );
                    auto messageType = objectMessage->getType();
                    auto object = objectMessage->getObject();

                    if( messageType == IUIElement::STATE_MESSAGE_ADD_CHILD )
                    {
                        owner->addChild( object );
                        return true;
                    }
                    else if( messageType == IUIElement::STATE_MESSAGE_REMOVE_CHILD )
                    {
                        owner->removeChild( object );
                        return true;
                    }
                }
                else if( message->isExactly<StateMessageMaterial>() )
                {
                    auto materialMessage =
                        workphone::static_pointer_cast<StateMessageMaterial>( message );

                    owner->setupMaterial( materialMessage->getMaterial() );
                }
            }

            return false;
        }

        template <class T>
        bool UIElementCore<T>::ElementStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            auto pOwner = getOwner();
            if( auto owner = workphone::dynamic_pointer_cast<UIElementCore<T>>( pOwner ) )
            {
                return owner->handleStateChanged( state );
            }

            return false;
        }

        template <class T>
        SmartPtr<IUIElement> UIElementCore<T>::ElementStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        template <class T>
        void UIElementCore<T>::ElementStateListener::setOwner( SmartPtr<IUIElement> owner )
        {
            m_owner = owner;
        }

        template <class T>
        void UIElementCore<T>::setupMaterial( SmartPtr<render::IMaterial> material )
        {
        }

        template <class T>
        struct wp_widget *UIElementCore<T>::getWidget() const
        {
            return m_widget;
        }

        template <class T>
        void UIElementCore<T>::setWidget( struct wp_widget *widget )
        {
            m_widget = widget;
        }

        template <class T>
        void UIElementCore<T>::invalidate()
        {
            if( auto stateContext = this->getStateContext() )
            {
                stateContext->invalidateState();
            }
        }

        template <class T>
        void UIElementCore<T>::lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto renderUI = applicationManager->getRenderUIPtr();
            renderUI->lock();
        }

        template <class T>
        void UIElementCore<T>::unlock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto renderUI = applicationManager->getRenderUIPtr();
            renderUI->unlock();
        }

        template <class T>
        SmartPtr<core::IPrototype> UIElementCore<T>::getParentPrototype() const
        {
            return nullptr;
        }

        template <class T>
        void UIElementCore<T>::setParentPrototype( SmartPtr<core::IPrototype> prototype )
        {
        }

    }  // end namespace ui
}  // namespace workphone

#endif  // UIElement_h__
