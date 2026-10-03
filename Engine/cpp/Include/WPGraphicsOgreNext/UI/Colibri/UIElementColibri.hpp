#ifndef __UIElementOgreNext_h__
#define __UIElementOgreNext_h__

/**
 * @file UIElementOgreNext.hpp
 * @brief Colibri/Ogre-backed UI element base template used by the render UI.
 *
 * This header declares `UIElementOgreNext<T>`, a CRTP template providing
 * common functionality required by UI elements implemented with the
 * Colibri GUI library and the Ogre render system. It integrates with the
 * engine's state system, exposes hooks for child/parent relationships,
 * and manages an optional Colibri widget attachment.
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
#include <Workphone/State/States/State.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/State/Messages/StateMessageObject.hpp>
#include <Workphone/State/Messages/StateMessageMaterial.hpp>
#include <Workphone/State/States/UIElementStateData.hpp>
#include <Workphone/System/Prototype.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp>
#include <ColibriGui/ColibriManager.h>
#include <ColibriGui/ColibriWidget.h>

namespace workphone
{
    namespace ui
    {

        /**
         * @brief CRTP base class for UI elements backed by Colibri + Ogre.
         *
         * This template implements common UI element behavior needed by the engine's
         * RenderUI. It handles parent/child management, visibility/enabled state
         * notifications, focus/select/activation propagation, state context creation
         * and handling, and convenient integration with a Colibri::Widget when one
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
        class UIElementColibri : public T
        {
        public:
            /**
             * @brief Construct a UI element.
             *
             * Generates a unique default name for the element and initializes
             * internal pointers to null. Concrete derived classes may set up
             * a Colibri widget or state listeners later during load.
             */
            UIElementColibri();

            /**
             * @brief Virtual destructor.
             *
             * Ensures attached state context (if present) is destroyed and that
             * any loaded resources are unloaded.
             */
            ~UIElementColibri() override;

            /**
             * @brief Unload and release renderer/UI resources owned by this element.
             *
             * This removes any Colibri widget listeners, destroys the associated
             * Colibri widget using the Colibri manager (if present), clears the
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
             * @brief Get the associated Colibri widget pointer.
             * @return Raw pointer to the Colibri::Widget or nullptr if not set.
             */
            Colibri::Widget *getWidget() const;

            /**
             * @brief Associate a Colibri widget with this element.
             *
             * When a widget is assigned the internal widget listener (if present)
             * is automatically registered/unregistered with the widget.
             *
             * @param widget Raw Colibri::Widget pointer to associate, or nullptr to clear.
             */
            void setWidget( Colibri::Widget *widget );

            /**
             * @brief Notification received from the Colibri widget when an action occurs.
             *
             * Base implementation triggers layout activation on PrimaryActionPerform.
             * Override to customize handling of widget actions.
             *
             * @param widget Pointer to the widget issuing the action.
             * @param action The action enum value.
             */
            virtual void notifyWidgetAction( Colibri::Widget *widget, Colibri::Action::Action action );

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
             * @brief Colibri widget action listener that forwards widget actions to the owner.
             *
             * The listener keeps an atomic weak pointer to the owning UI element template
             * instance and invokes `notifyWidgetAction()` on action events.
             */
            class WidgetListener : public Colibri::WidgetActionListener
            {
            public:
                virtual ~WidgetListener()
                {
                }

                /**
                 * @brief Called by Colibri when a widget action occurs.
                 *
                 * Forwards the call to the owner element if it is still alive.
                 *
                 * @param widget The widget that issued the action.
                 * @param action The action code.
                 */
                void notifyWidgetAction( Colibri::Widget *widget,
                                         Colibri::Action::Action action ) override
                {
                    if( auto owner = getOwner() )
                    {
                        owner->notifyWidgetAction( widget, action );
                    }
                }

                /**
                 * @brief Get the owner element (strong pointer).
                 * @return Smart pointer to the owning UIElementOgreNext<T> or nullptr.
                 */
                SmartPtr<UIElementColibri<T>> getOwner() const
                {
                    auto p = m_owner.load();
                    return p.lock();
                }

                /**
                 * @brief Set the owner element.
                 * @param owner Smart pointer to the owner element.
                 */
                void setOwner( SmartPtr<UIElementColibri<T>> owner )
                {
                    m_owner = owner;
                }

            protected:
                AtomicWeakPtr<UIElementColibri<T>> m_owner; /**< Weak pointer to owner. */
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

            Colibri::Widget *m_widget = nullptr;        /**< Associated Colibri widget (if any). */
            WidgetListener *m_widgetListener = nullptr; /**< Listener attached to the Colibri widget. */

            /// Used to store user properties (may be lazily created).
            mutable SmartPtr<ISharedObject> m_userProperties;

            /// Counter used to generate unique element default names.
            static u32 m_nextGeneratedNameExt;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, UIElementColibri, T, core::Prototype<T> );

        template <class T>
        u32 UIElementColibri<T>::m_nextGeneratedNameExt = 0;

        template <class T>
        UIElementColibri<T>::UIElementColibri()
        {
            static const String uiElementName = "UIElement";
            auto name = uiElementName + StringUtil::toString( m_nextGeneratedNameExt++ );
            UIElementColibri<T>::setName( name );

            //m_widgetListener = new WidgetListener();
            //m_widgetListener->setOwner( this );
        }

        template <class T>
        UIElementColibri<T>::~UIElementColibri()
        {
            destroyStateContext();

            if( UIElementColibri<T>::isLoaded() )
            {
                unload( nullptr );
            }
        }

        template <class T>
        void UIElementColibri<T>::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto ui = workphone::static_pointer_cast<UIManagerColibri>(
                    applicationManager->getRenderUI() );
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                ScopedLock lock( graphicsSystem );

                auto window = ui->getLayoutWindow();
                auto colibriManager = ui->getColibriManager();

                if( m_widgetListener )
                {
                    if( auto widget = getWidget() )
                    {
                        widget->removeActionListener( m_widgetListener );
                    }

                    delete m_widgetListener;
                    m_widgetListener = nullptr;
                }

                if( auto widget = getWidget() )
                {
                    setWidget( nullptr );

                    if( colibriManager )
                    {
                        colibriManager->destroyWidget( widget );
                    }
                }

                this->setContainer( nullptr );

                this->removeAllChildren();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void UIElementColibri<T>::update()
        {
            if( !this->isVisible() )
            {
                return;
            }

            if( !this->isEnabled() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            if( task == TaskId::Application )
            {
                auto children = this->getChildren();
                for( auto &child : children )
                {
                    child->update();
                }
            }
        }

        template <class T>
        void UIElementColibri<T>::addChild( SmartPtr<IUIElement> child )
        {
            if( Thread::getTaskFlag( Thread::Render_Flag ) )
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

                auto childElement = workphone::static_pointer_cast<UIElementColibri>( child );

                if( auto stateContext = childElement->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }
            else
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto factoryManager = applicationManager->getFactoryManagerPtr();

                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setSender( this );
                message->setType( IUIElement::STATE_MESSAGE_ADD_CHILD );
                message->setObject( child );

                if( auto stateContext = this->getStateContext() )
                {
                    stateContext->addMessage( TaskId::Render, message );
                }
            }
        }

        template <class T>
        void UIElementColibri<T>::addAnimator( SmartPtr<IAnimator> &animator )
        {
        }

        template <class T>
        bool UIElementColibri<T>::removeAnimator( SmartPtr<IAnimator> &animator )
        {
            return false;
        }

        template <class T>
        SmartPtr<IUILayoutContainer> UIElementColibri<T>::getContainer() const
        {
            auto p = m_container.load();
            return p.lock();
        }

        template <class T>
        void UIElementColibri<T>::setContainer( SmartPtr<IUILayoutContainer> container )
        {
            m_container = container;
        }

        template <class T>
        void UIElementColibri<T>::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }

        template <class T>
        bool UIElementColibri<T>::handleEvent( const SmartPtr<IInputEvent> &event )
        {
            auto applicationManager = core::IApplicationManager::instance();

            //switch( auto eventType = event->getEventType() )
            //...
            // Default base class does not handle input.
            return false;
        }

        template <class T>
        bool UIElementColibri<T>::isThreadSafe() const
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto renderTask = graphicsSystem->getRenderTask();

            auto task = Thread::getCurrentTask();

            const auto &loadingState = T::getLoadingState();

            return loadingState == LoadingState::Loaded && task == renderTask;
        }

        template <class T>
        void UIElementColibri<T>::addMessage( SmartPtr<IStateMessage> message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            if( auto stateContext = UIElementColibri<T>::getStateContext() )
            {
                const auto stateTask = graphicsSystem->getStateTask();
                stateContext->addMessage( stateTask, message );
            }
        }

        template <class T>
        void UIElementColibri<T>::onEvent( const String &eventType )
        {
            // for (u32 i = 0; i < m_children.size(); ++i)
            //	((UIElement*)m_children[i])->onEvent(eventType);

            // for (u32 i = 0; i < m_listeners.size(); ++i)
            //	m_listeners[i]->onEvent(eventType);
        }

        template <class T>
        SmartPtr<Properties> UIElementColibri<T>::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();

            if( auto widget = this->getWidget() )
            {
                auto pos = widget->getCenter();
                auto size = widget->getSize();

                properties->setProperty( "widget_pos", Vector2F( pos.x, pos.y ) );
                properties->setProperty( "widget_size", Vector2F( size.x, size.y ) );
            }

            auto size = this->getSize();
            auto position = this->getPosition();
            auto enabled = this->isEnabled();
            auto visible = this->isVisible();
            auto colour = this->getColour();
            auto sameLine = this->getSameLine();
            auto order = this->getOrder();

            properties->setProperty( "size", size );
            properties->setProperty( "position", position );
            properties->setProperty( "enabled", enabled );
            properties->setProperty( "visible", visible );
            properties->setProperty( "colour", colour );
            properties->setProperty( "same_line", sameLine );
            properties->setProperty( "order", order );
            properties->setButtonPressed( "make dirty" );
            return properties;
        }

        template <class T>
        void UIElementColibri<T>::setProperties( SmartPtr<Properties> properties )
        {
            auto enabled = this->isEnabled();
            auto visible = this->isVisible();
            auto colour = this->getColour();
            auto order = this->getOrder();
            auto sameLine = this->getSameLine();

            //properties->getPropertyValue( "size", m_size );
            //properties->getPropertyValue( "position", m_position );
            properties->getPropertyValue( "enabled", enabled );
            properties->getPropertyValue( "visible", visible );
            properties->getPropertyValue( "colour", colour );
            properties->getPropertyValue( "same_line", sameLine );
            properties->getPropertyValue( "order", order );

            this->setEnabled( enabled );
            this->setVisible( visible );
            this->setSameLine( sameLine );
            this->setColour( colour );

            if( properties->isButtonPressed( "make dirty" ) )
            {
                if( auto stateContext = this->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> UIElementColibri<T>::getChildObjects() const
        {
            auto objects = Array<SmartPtr<ISharedObject>>();
            objects.reserve( 32 );

            //if( auto stateContext = m_stateContext.load() )
            //{
            //    objects.push_back( stateContext );
            //}

            //if( auto stateListener = m_stateListener.load() )
            //{
            //    objects.push_back( stateListener );
            //}

            //if( auto owner = m_owner.load() )
            //{
            //    objects.push_back( owner );
            //}

            objects.push_back( this->getDragSource() );
            objects.push_back( this->getDropTarget() );

            //if( auto element = getOverlayElement() )
            //{
            //    objects.push_back( element );
            //}

            //if( m_container )
            //{
            //    objects.push_back( m_container );
            //}

            //if( m_parent )
            //{
            //    objects.push_back( m_parent );
            //}

            //if( m_layout )
            //{
            //    objects.push_back( m_layout );
            //}

            return objects;
        }

        template <class T>
        void UIElementColibri<T>::createStateContext()
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
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            state->setData( stateData );

            this->setStateContext( stateContext );
            this->setStateListener( listener );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

        template <class T>
        void UIElementColibri<T>::destroyStateContext()
        {
            if( auto stateContext = this->getStateContext() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager->getStateManagerPtr();
                WP_ASSERT( stateManager );

                if( auto stateListener = this->getStateListener() )
                {
                    stateListener->unload( nullptr );
                    stateContext->removeStateListener( stateListener );
                    this->setStateListener( nullptr );
                }

                stateManager->removeStateContext( stateContext );
                this->setStateContext( nullptr );
            }
        }

        template <class T>
        bool UIElementColibri<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto renderUI = applicationManager->getRenderUI();
            WP_ASSERT( renderUI );

            ScopedLock lock( graphicsSystem );

            if( UIElementColibri<T>::isLoaded() )
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

                        sceneWindowSize = Vector2F( 1920, 1080 );  //TODO: get reference size

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

                            mainWindowSizeF = Vector2F( 1920, 1080 );  //TODO: get reference size

                            auto pos = position * mainWindowSizeF;
                            auto sz = size * mainWindowSizeF;

                            iPosition = Vector2I( (s32)pos.X(), (s32)pos.Y() );
                            iSize = Vector2I( (s32)sz.X(), (s32)sz.Y() );
                        }
                    }

                    if( auto widget = getWidget() )
                    {
                        widget->setZOrder( zOrder );

                        widget->setTransform(
                            Ogre::Vector2( (Ogre::Real)iPosition.x, (Ogre::Real)iPosition.y ),
                            Ogre::Vector2( (Ogre::Real)iSize.x, (Ogre::Real)iSize.y ) );

                        return true;
                    }
                }
                else if( stateData->isDerived<UIElementStateData>() )
                {
                    auto elementState = workphone::static_pointer_cast<UIElementStateData>( stateData );

                    auto inheritsPick = BitUtil::getFlagValue( elementState->flags,
                                                               (u16)IUIElement::handleInputEventsFlag );

                    auto visible =
                        BitUtil::getFlagValue( elementState->flags, (u16)IUIElement::visibleFlag );

                    if( auto widget = getWidget() )
                    {
                        widget->setHidden( !visible );

                        elementState->flags = BitUtil::setFlagValue(
                            elementState->flags, (u16)IUIElement::elementVisibleFlag, visible );

                        return true;
                    }
                }
            }

            return false;
        }

        template <class T>
        bool UIElementColibri<T>::ElementStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            ScopedLock lock( graphicsSystem );

            auto pOwner = getOwner();
            if( auto owner = workphone::dynamic_pointer_cast<UIElementColibri<T>>( pOwner ) )
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
        bool UIElementColibri<T>::ElementStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            auto pOwner = getOwner();
            if( auto owner = workphone::dynamic_pointer_cast<UIElementColibri<T>>( pOwner ) )
            {
                return owner->handleStateChanged( state );
            }

            return false;
        }

        template <class T>
        SmartPtr<IUIElement> UIElementColibri<T>::ElementStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        template <class T>
        void UIElementColibri<T>::ElementStateListener::setOwner( SmartPtr<IUIElement> owner )
        {
            m_owner = owner;
        }

        template <class T>
        void UIElementColibri<T>::setupMaterial( SmartPtr<render::IMaterial> material )
        {
        }

        template <class T>
        Colibri::Widget *UIElementColibri<T>::getWidget() const
        {
            return m_widget;
        }

        template <class T>
        void UIElementColibri<T>::setWidget( Colibri::Widget *widget )
        {
            if( m_widget )
            {
                if( m_widgetListener )
                {
                    m_widget->removeActionListener( m_widgetListener );
                }
            }

            m_widget = widget;

            if( m_widget )
            {
                if( m_widgetListener )
                {
                    m_widget->addActionListener( m_widgetListener );
                }
            }
        }

        template <class T>
        void UIElementColibri<T>::notifyWidgetAction( Colibri::Widget *widget,
                                                      Colibri::Action::Action action )
        {
            if( action == Colibri::Action::Action::PrimaryActionPerform )
            {
                if( auto layout = this->getLayout() )
                {
                }
            }
        }

        template <class T>
        void UIElementColibri<T>::invalidate()
        {
            if( auto stateContext = this->getStateContext() )
            {
                stateContext->invalidateState();
            }
        }

        template <class T>
        void UIElementColibri<T>::lock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto renderUI = applicationManager->getRenderUI();
            renderUI->lock();
        }

        template <class T>
        void UIElementColibri<T>::unlock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto renderUI = applicationManager->getRenderUI();
            renderUI->unlock();
        }

        template <class T>
        SmartPtr<core::IPrototype> UIElementColibri<T>::getParentPrototype() const
        {
            return nullptr;
        }

        template <class T>
        void UIElementColibri<T>::setParentPrototype( SmartPtr<core::IPrototype> prototype )
        {
        }

    }  // end namespace ui
}  // namespace workphone

#endif  // UIElement_h__
