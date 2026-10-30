#pragma once

#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/ScrollView.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <vector>

namespace layout_input_sort_contracts
{
    using namespace workphone;
    using namespace workphone::scene;

    class Actor final : public GameActor
    {
    public:
        State getState() const override { return State::Create; }
        void setState( State, bool = true ) override {}
        bool isEnabledInScene() const override { return enabled; }
        bool isVisible() const override { return visible; }
        Array<SmartPtr<IComponent>> getComponents() const override
        {
            ++componentSnapshots;
            return components;
        }
        Array<SmartPtr<IGameActor>> getAllChildren() const override
        {
            auto children = GameActor::getAllChildren();
            if( includeNull ) children.push_back( nullptr );
            return children;
        }
        void attach( const SmartPtr<IComponent> &component )
        {
            component->setActor( this );
            components.push_back( component );
        }

        Array<SmartPtr<IComponent>> components{ Allocator<SmartPtr<IComponent>>( sourceBudget ) };
        mutable unsigned componentSnapshots = 0;
        bool enabled = true, visible = true, includeNull = false;
    };

    class Transform final : public LayoutTransform
    {
    public:
        State getState() const override { return State::Edit; }
    };

    class Control final : public UIComponent
    {
    public:
        State getState() const override { return State::Edit; }
        bool handleEvent( const SmartPtr<IInputEvent> &event ) override
        {
            ++calls;
            delivered->push_back( id );
            if( onInput ) onInput( event );
            return consume;
        }
        std::vector<unsigned> *delivered = nullptr;
        std::function<void( const SmartPtr<IInputEvent> & )> onInput;
        unsigned id = 0, calls = 0;
        bool consume = false;
    };

    class Scroll final : public ScrollView
    {
    public:
        State getState() const override { return State::Edit; }
        bool handleEvent( const SmartPtr<IInputEvent> & ) override { return false; }
    };

    class Window final : public render::GraphicsWindow
    {
    public:
        Vector2I getSize() const override { return Vector2I( 1000, 1000 ); }
    };

    struct Node
    {
        SmartPtr<Actor> actor;
        SmartPtr<Transform> transform;
        SmartPtr<Control> control;
    };

    struct Context
    {
        explicit Context( const SmartPtr<core::ApplicationManager> &application ) : app( application )
        {
            previousWindow = app->getWindow();
            app->setWindow( make_ptr<Window>() );
            root = make_ptr<Actor>();
            root->getHandle()->setInstanceId( 1000 );
            layout = make_ptr<Layout>();
            canvas = make_ptr<Layout>();
            canvas->setReferenceSize( Vector2I( 1000, 1000 ) );
            layout->setEnabled( true );
            layout->setCanvas( canvas );
            layout->setEditorWindowBorderSize( Vector2<real_Num>::zero() );
            mouse = make_ptr<MouseState>();
            mouse->setEventType( IMouseState::Event::Moved );
            event = make_ptr<InputEvent>();
            event->setEventType( IInputEvent::EventType::Mouse );
            event->setMouseState( mouse );
        }
        ~Context()
        {
            for( auto &node : nodes ) if( node.control ) node.control->onInput = {};
            root->removeChildren();
            for( auto it = nodes.rbegin(); it != nodes.rend(); ++it )
            {
                if( it->actor )
                {
                    it->actor->removeChildren();
                    it->actor->components.clear();
                }
            }
            nodes.clear();
            layout->setCanvas( nullptr );
            app->setWindow( previousWindow );
        }
        size_t add( unsigned id, u32 order, bool withTransform = true )
        {
            Node node;
            node.actor = make_ptr<Actor>();
            node.actor->getHandle()->setInstanceId( id );
            if( withTransform )
            {
                node.transform = make_ptr<Transform>();
                node.transform->setZOrder( order, false );
                node.actor->attach( node.transform );
            }
            node.control = make_ptr<Control>();
            node.control->id = id;
            node.control->delivered = &delivered;
            node.actor->attach( node.control );
            root->addChild( node.actor );
            nodes.push_back( std::move( node ) );
            return nodes.size() - 1;
        }
        void reset()
        {
            delivered.clear();
            for( auto &node : nodes ) if( node.actor ) node.actor->componentSnapshots = 0;
        }
        SmartPtr<core::ApplicationManager> app;
        SmartPtr<render::IGraphicsWindow> previousWindow;
        SmartPtr<Actor> root;
        SmartPtr<Layout> layout, canvas;
        SmartPtr<MouseState> mouse;
        SmartPtr<InputEvent> event;
        std::vector<Node> nodes;
        std::vector<unsigned> delivered;
    };

    inline unsigned run( const SmartPtr<core::ApplicationManager> &app )
    {
        unsigned assertions = 0;
        const auto require = [&]( bool value, const char *message ) {
            ++assertions;
            if( !value ) throw std::runtime_error( message );
        };
        const auto orderIs = []( const std::vector<unsigned> &actual, std::initializer_list<unsigned> expected ) {
            return actual == std::vector<unsigned>( expected );
        };
        {
            Context test( app );
            require( !test.layout->handleEvent( test.root, test.event ), "an empty layout has no input candidate" );
            const auto first = test.add( 1, 0, false );
            test.reset();
            require( !test.layout->handleEvent( test.root, test.event ) && orderIs( test.delivered, { 1 } ) &&
                         test.nodes[first].actor->componentSnapshots == 1,
                     "a single child needs only dispatch lookup, without sort-key lookup" );
            test.nodes[first].actor->enabled = false;
            test.reset();
            require( !test.layout->handleEvent( test.root, test.event ) && test.delivered.empty() &&
                         test.nodes[first].actor->componentSnapshots == 0,
                     "a disabled single child needs neither a key lookup nor dispatch" );
            test.nodes[first].actor->enabled = true;
            test.add( 2, 10 );
            const auto front = test.add( 3, 30 );
            const auto tied = test.add( 4, 10 );
            const auto disabled = test.add( 5, 99 );
            const auto hidden = test.add( 6, 100 );
            test.nodes[disabled].actor->enabled = false;
            test.nodes[hidden].actor->visible = false;
            test.root->includeNull = true;
            test.reset();
            require( !test.layout->handleEvent( test.root, test.event ) && orderIs( test.delivered, { 3, 4, 2, 1 } ),
                     "frontmost keys, reverse-DFS equal-order ties and missing transforms retain dispatch order" );
            for( size_t i = 0; i < test.nodes.size(); ++i )
            {
                const auto expected = i == disabled || i == hidden ? 1u : 2u;
                require( test.nodes[i].actor->componentSnapshots == expected,
                         "each actor key is captured once, plus one lookup only when dispatched" );
            }
            test.nodes[front].control->consume = true;
            test.reset();
            require( test.layout->handleEvent( test.root, test.event ) && orderIs( test.delivered, { 3 } ),
                     "a handled frontmost event stops dispatch to lower controls" );
            test.nodes[front].control->consume = false;
            test.nodes[front].control->onInput = [&]( const SmartPtr<IInputEvent> & ) {
                test.nodes[tied].transform->setZOrder( 40, false );
            };
            test.reset();
            require( !test.layout->handleEvent( test.root, test.event ) && orderIs( test.delivered, { 3, 4, 2, 1 } ),
                     "a callback order change cannot reorder the current event snapshot" );
            test.nodes[front].control->onInput = {};
            test.reset();
            require( !test.layout->handleEvent( test.root, test.event ) && orderIs( test.delivered, { 4, 3, 2, 1 } ),
                     "the next event captures changed keys without a persistent stale cache" );
        }
        {
            Context test( app );
            const auto back = test.add( 1, 0 );
            const auto victim = test.add( 2, 10 );
            const auto front = test.add( 3, 30 );
            WeakPtr<Actor> retired = test.nodes[victim].actor;
            test.nodes[victim].actor = nullptr;
            bool pinnedDuringCallback = false;
            test.nodes[front].control->onInput = [&]( const SmartPtr<IInputEvent> & ) {
                auto removing = retired.lock();
                test.root->removeChild( removing );
                removing = nullptr;
                pinnedDuringCallback = !retired.expired();
                test.add( 4, 20 );
            };
            require( !test.layout->handleEvent( test.root, test.event ) && orderIs( test.delivered, { 3, 2, 1 } ) &&
                         pinnedDuringCallback && retired.expired(),
                     "callbacks can replace hierarchy membership while the current snapshot pins removed actors" );
            test.nodes[front].control->onInput = {};
            test.reset();
            require( !test.layout->handleEvent( test.root, test.event ) && orderIs( test.delivered, { 3, 4, 1 } ) &&
                         test.nodes[back].control->calls == 2,
                     "new and removed hierarchy members take effect on the following input event" );
        }
        {
            Context test( app );
            const auto view = test.add( 1, 0 );
            const auto content = test.add( 2, 1 );
            const auto button = test.add( 3, 20 );
            test.nodes[view].actor->components.clear();
            auto scroll = make_ptr<Scroll>();
            test.nodes[view].actor->attach( test.nodes[view].transform );
            test.nodes[view].actor->attach( scroll );
            scroll->setContentPanel( test.nodes[content].transform );
            test.nodes[view].actor->addChild( test.nodes[content].actor );
            test.nodes[content].actor->addChild( test.nodes[button].actor );
            test.nodes[view].transform->setAbsolutePosition( Vector2<real_Num>( 100, 100 ) );
            test.nodes[view].transform->setAbsoluteSize( Vector2<real_Num>( 300, 200 ) );
            auto control = test.nodes[button].control;
            control->consume = true;
            bool captured = false;
            control->onInput = [&]( const SmartPtr<IInputEvent> &event ) {
                const auto kind = event->getMouseState()->getEventType();
                if( kind == IMouseState::Event::LeftPressed ) captured = true;
                if( kind == IMouseState::Event::LeftReleased ) captured = false;
            };
            test.mouse->setEventType( IMouseState::Event::LeftPressed );
            test.mouse->setRelativePosition( Vector2<real_Num>( 0.7f, 0.7f ) );
            test.layout->handleEvent( test.root, test.event );
            require( control->calls == 0 && !captured, "clipped scroll descendants cannot begin an outside pointer press" );
            test.mouse->setRelativePosition( Vector2<real_Num>( 0.15f, 0.15f ) );
            require( test.layout->handleEvent( test.root, test.event ) && control->calls == 1 && captured,
                     "an unclipped frontmost child receives a press before its scroll container" );
            test.mouse->setRelativePosition( Vector2<real_Num>( 0.7f, 0.7f ) );
            test.mouse->setEventType( IMouseState::Event::Moved );
            require( test.layout->handleEvent( test.root, test.event ) && control->calls == 2 && captured,
                     "captured pointer motion still reaches a child outside the scroll viewport" );
            test.mouse->setEventType( IMouseState::Event::LeftReleased );
            require( test.layout->handleEvent( test.root, test.event ) && control->calls == 3 && !captured,
                     "captured pointer release still reaches a child outside the scroll viewport" );
            control->onInput = {};
            scroll->setContentPanel( nullptr );
        }
        std::printf( "Layout input sorting contracts: %u assertions passed.\n", assertions );
        return assertions;
    }
}
