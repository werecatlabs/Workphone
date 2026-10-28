#pragma once
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <stdexcept>
#include <vector>

namespace ui_layout_traversal_contracts
{
    using namespace workphone;
    using namespace workphone::scene;
    inline bool measuring = false;

    inline void visit( unsigned &count )
    {
        ++count;
        if( measuring && count > 4 )
            throw std::runtime_error( "UI subtree walk revisited one component more than four times" );
    }

    class Actor : public GameActor
    {
    public:
        State getState() const override { return State::Create; }
        void setState( State, bool = true ) override {}
        bool isEnabledInScene() const override { return true; }
        bool isVisible() const override { return true; }
        void updateOrder( bool cascade = true ) override
        { visit( orders ); GameActor::updateOrder( cascade ); }
        unsigned orders = 0;
    };

    class Transform : public LayoutTransform
    {
    public:
        State getState() const override { return State::Edit; }
        void updateOrder() override { visit( orders ); LayoutTransform::updateOrder(); }
        void updateTransform() override { visit( transforms ); LayoutTransform::updateTransform(); }
        unsigned orders = 0, transforms = 0;
    };

    class Control : public UIComponent
    {
    public:
        State getState() const override { return State::Edit; }
        void updateOrder() override
        {
            visit( orders );
            if( throwOnce ) { throwOnce = false; throw std::runtime_error( "synthetic UI callback failure" ); }
            UIComponent::updateOrder();
        }
        unsigned orders = 0;
        bool throwOnce = false;
    };

    class Canvas : public Layout
    {
    public:
        State getState() const override { return State::Edit; }
        void updateOrder() override { visit( orders ); Layout::updateOrder(); }
        unsigned orders = 0;
    };

    struct Node
    {
        SmartPtr<Actor> actor;
        SmartPtr<Transform> transform;
        Array<SmartPtr<Control>> controls;
    };

    inline unsigned run( SmartPtr<core::ApplicationManager> app )
    {
        unsigned assertions = 0;
        const auto require = [&]( bool value, const char *message ) {
            ++assertions;
            if( !value ) throw std::runtime_error( message );
        };
        auto previousUI = app->getRenderUI();
        auto manager = make_ptr<ui::ClawUIManager>();
        app->setRenderUI( manager );
        std::vector<Node> nodes;
        auto canvas = make_ptr<Canvas>();
        auto nativeCanvas = dynamic_pointer_cast<ui::IUILayoutWindow>(
            manager->addElement( ui::IUILayoutWindow::typeInfo() ) );
        auto survivor = manager->addElement( ui::IUIImage::typeInfo() );
        const auto cleanup = [&] {
            measuring = false;
            // Tear down deepest-first through real actor/component removal,
            // keeping failed complexity probes bounded as well.
            for( auto i = nodes.rbegin(); i != nodes.rend(); ++i ) i->actor->removeChildren();
            for( auto i = nodes.rbegin(); i != nodes.rend(); ++i )
            {
                auto components = i->actor->getComponents();
                for( auto c = components.rbegin(); c != components.rend(); ++c )
                    i->actor->removeComponentInstance( *c );
            }
            nodes.clear();
            if( canvas )
            {
                canvas->setCanvas( nullptr );
                canvas->setLayoutTransform( nullptr );
            }
            canvas = nullptr;
            nativeCanvas = nullptr;
        };
        try
        {
            canvas->setLayout( nativeCanvas );
            canvas->setElement( nativeCanvas );
            for( unsigned i = 0; i < 25; ++i )
            {
                Node node;
                node.actor = make_ptr<Actor>();
                node.actor->getHandle()->setInstanceId( i );
                if( !nodes.empty() ) nodes.back().actor->addChild( node.actor );
                // Include actor-only bridges. Descendant layout work must cross
                // these nodes without skipping transforms or visiting them twice.
                if( i == 0 || i % 4 != 0 )
                {
                    node.transform = make_ptr<Transform>();
                    node.actor->addComponentInstance( node.transform );
                    node.transform->setAutoCalculateOrder( false, false );
                    node.transform->setZOrder( 100 + i * 5, false );
                    if( i == 0 )
                    {
                        node.actor->addComponentInstance( canvas );
                        canvas->setLayoutTransform( node.transform.get() );
                    }
                    for( unsigned j = 0; j < 2; ++j )
                    {
                        auto control = make_ptr<Control>();
                        node.actor->addComponentInstance( control );
                        control->setElement( manager->addElement( ui::IUIImage::typeInfo() ) );
                        control->setLayoutTransform( node.transform.get() );
                        node.controls.push_back( control );
                    }
                }
                nodes.push_back( std::move( node ) );
            }
            const auto reset = [&] {
                measuring = false;
                canvas->orders = 0;
                for( auto &node : nodes )
                {
                    node.actor->orders = 0;
                    if( node.transform ) node.transform->orders = node.transform->transforms = 0;
                    for( auto control : node.controls ) control->orders = 0;
                }
                measuring = true;
            };
            reset();
            nodes[0].actor->updateOrder( true );
            unsigned controlCount = 0;
            for( const auto &node : nodes )
            {
                require( node.actor->orders == 1, "actor subtree order must visit each actor exactly once" );
                if( node.transform )
                {
                    require( node.transform->orders == 1, "actor order must visit each LayoutTransform exactly once" );
                    for( auto control : node.controls )
                    {
                        ++controlCount;
                        require( control->orders == 1, "multiple UI components must each receive one local order update" );
                        require( control->getElement()->getOrder() == node.transform->getZOrder(),
                                 "explicit draw order must survive a subtree update" );
                        require( control->getElement()->getParent() == nativeCanvas &&
                                     control->getElement()->getLayout() == nativeCanvas,
                                 "explicit order must still attach every native element to its canvas" );
                    }
                }
            }
            require( canvas->orders == 1 && nativeCanvas->getNumChildren() == controlCount &&
                         !nativeCanvas->getParent(),
                     "canvas membership must be complete without self-parenting or duplicate children" );
            reset();
            nodes[0].actor->updateOrder( false );
            require( nodes[0].transform->orders == 1 && canvas->orders == 1,
                     "cascade=false must still update all local components" );
            for( size_t i = 1; i < nodes.size(); ++i )
            {
                require( nodes[i].actor->orders == 0, "cascade=false must not visit descendant actors" );
                if( nodes[i].transform ) require( nodes[i].transform->orders == 0,
                    "cascade=false must not leak through LayoutTransform into descendants" );
                for( auto control : nodes[i].controls ) require( control->orders == 0,
                    "cascade=false must not leak through UIComponent into descendants" );
            }
            reset();
            nodes[0].controls[0]->updateOrder();
            require( nodes[0].controls[0]->orders == 1 && nodes[0].controls[1]->orders == 0,
                     "direct UI order updates the requested local component" );
            for( size_t i = 1; i < nodes.size(); ++i )
            {
                require( nodes[i].actor->orders == 1, "direct UI order must cascade once per child actor" );
                if( nodes[i].transform ) require( nodes[i].transform->orders == 1,
                    "direct UI order must remain linear through nested transforms" );
            }
            reset();
            nodes[0].transform->updateOrder();
            require( nodes[0].transform->orders <= 2 && canvas->orders == 1,
                     "direct transform order may delegate once to its owning actor" );
            for( size_t i = 1; i < nodes.size(); ++i )
                if( nodes[i].transform ) require( nodes[i].transform->orders == 1,
                    "direct transform order must not re-enter descendant component traversal" );
            reset();
            canvas->updateOrder();
            for( size_t i = 1; i < nodes.size(); ++i )
                require( nodes[i].actor->orders == 1, "direct canvas order must preserve one subtree walk" );
            reset();
            nodes[0].controls[0]->throwOnce = true;
            bool rejected = false;
            try { nodes[0].actor->updateOrder(); }
            catch( const std::runtime_error & ) { rejected = true; }
            require( rejected, "a failing UI callback must propagate to the traversal caller" );
            reset();
            nodes[0].controls[0]->updateOrder();
            require( nodes.back().actor->orders == 1,
                     "exception unwinding must release actor traversal ownership for the next direct UI update" );
            reset();
            nodes[0].transform->updateTransform();
            for( const auto &node : nodes )
                if( node.transform ) require( node.transform->transforms == 1,
                    "layout transform invalidation must visit each transform once across actor-only bridges" );
            measuring = false;
            nodes[0].transform->setAutoCalculateOrder( false, true );
            nodes[0].transform->setAutoCalculateOrder( true, false );
            for( size_t i = 1; i < nodes.size(); ++i )
                if( nodes[i].transform ) require( !nodes[i].transform->getAutoCalculateOrder(),
                    "local automatic-order changes must preserve descendant flags" );
            nodes[0].transform->setZOrder( 10, true );
            unsigned depth = 0;
            for( const auto &node : nodes ) if( node.transform )
                require( node.transform->getZOrder() == 10 + depth++,
                         "cascaded explicit order increments once per transform, including across bridges" );
            nodes[0].transform->setZOrder( 777, false );
            require( nodes[1].transform->getZOrder() == 11,
                     "a local explicit order change must leave child order intact" );
            nodes[0].transform->setAutoCalculateOrder( true, true );
            reset();
            nodes[0].actor->updateOrder();
            unsigned expected = 0;
            for( size_t i = 1; i < nodes.size(); ++i ) if( nodes[i].transform )
            {
                for( auto control : nodes[i].controls )
                    require( control->getElement()->getOrder() == expected,
                             "automatic order must preserve actor preorder with multiple UI components" );
                expected += static_cast<unsigned>( nodes[i].controls.size() );
            }
            measuring = false;
            WeakPtr<ui::IUIElement> retiredCanvas( nativeCanvas );
            cleanup();
            require( retiredCanvas.expired() && manager->getElements().size() == 1 &&
                         manager->getElements()[0] == survivor,
                     "retiring a deep generated hierarchy must release its canvas and preserve unrelated native UI" );
            std::printf( "UI traversal contracts: %u assertions, 25 nested actors, counted local/cascade/transform work and native root retirement passed.\n", assertions );
        }
        catch( ... )
        {
            cleanup();
            manager->clear();
            app->setRenderUI( previousUI );
            throw;
        }
        manager->clear();
        app->setRenderUI( previousUI );
        return assertions;
    }
}
