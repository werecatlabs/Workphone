#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/UI/ClawUILayout.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <Workphone/Interface/UI/IUICheckbox.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Scene/CameraManager.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <WorkphoneCore/workphone.h>

using namespace workphone;

namespace
{
    unsigned assertions = 0;

    struct Fixture
    {
        Fixture() : types( std::make_unique<TypeManager>() ),
                    previousTask( Thread::getCurrentTask() )
        {
            types->load();
            TypeManager::setInstance( types.get() );
            application = make_ptr<core::ApplicationManager>();
            core::IApplicationManager::setInstance( application );
            application->setFactoryManager( make_ptr<FactoryManager>() );
            application->setGraphicsSystem( make_ptr<render::ClawHammerSystem>() );
            application->setGameManager( make_ptr<scene::GameManager>() );
            Thread::setCurrentTask( TaskId::Render );
        }

        ~Fixture()
        {
            application->setRenderUI( nullptr );
            application->setGameManager( nullptr );
            application->setGraphicsSystem( nullptr );
            application->setFactoryManager( nullptr );
            core::IApplicationManager::setInstance( nullptr );
            application = nullptr;
            Thread::setCurrentTask( previousTask );
            TypeManager::setInstance( nullptr );
            types->unload();
        }

        std::unique_ptr<TypeManager> types;
        SmartPtr<core::ApplicationManager> application;
        TaskId previousTask;
    };

    bool check( bool condition, const char *message )
    {
        ++assertions;
        if( !condition )
            std::fprintf( stderr, "FAIL: %s\n", message );
        return condition;
    }

    class CollectionProbe : public ui::ClawUIElement<ui::IUIElement>
    {
    public:
        u32 getOrder() const override
        {
            ++orderReads;
            if( onOrder ) onOrder();
            return order;
        }

        void draw( wp_context * ) override
        {
            ++draws;
            if( observedDraws ) ++*observedDraws;
            if( onDraw ) onDraw();
        }

        u32 order = 0;
        mutable unsigned orderReads = 0;
        unsigned draws = 0;
        std::shared_ptr<unsigned> observedDraws;
        std::function<void()> onOrder;
        std::function<void()> onDraw;
    };

    bool testSortingExcludesOtherThreads()
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        auto layout = dynamic_pointer_cast<ui::ClawUILayout>( manager->addElement( ui::IUILayoutWindow::typeInfo() ) );
        if( !layout ) return check( false, "sort regression needs a native factory-created layout" );
        auto first = make_ptr<CollectionProbe>();
        auto second = make_ptr<CollectionProbe>();
        first->order = 2;
        second->order = 1;
        layout->addChild( first );
        layout->addChild( second );

        std::promise<void> compared, release;
        auto comparisonReached = compared.get_future();
        auto continueComparison = release.get_future().share();
        std::atomic<bool> observed{ false };
        first->onOrder = [&] {
            if( !observed.exchange( true ) )
            {
                compared.set_value();
                continueComparison.wait();
            }
        };
        std::exception_ptr sortFailure;
        std::thread sorter( [&] {
            try { layout->sortZOrder(); }
            catch( ... ) { sortFailure = std::current_exception(); }
        } );

        const auto reached = comparisonReached.wait_for( std::chrono::seconds( 5 ) ) ==
                             std::future_status::ready;
        bool acquired = false;
        if( reached )
        {
            acquired = layout->getChildrenRef().try_lock();
            if( acquired ) layout->getChildrenRef().unlock();
        }
        release.set_value();
        sorter.join();
        first->onOrder = {};
        bool ok = check( reached, "native child sorting must reach the controlled order capture" );
        ok &= check( !sortFailure, "native child sorting must finish without throwing" );
        ok &= check( reached && !acquired,
                     "sorting must retain exclusive child inventory ownership across comparisons" );
        const auto children = layout->getChildren();
        ok &= check( children.size() == 2 && children[0] == second && children[1] == first,
                     "the protected sort must preserve and order both native children" );
        layout->removeAllChildren();
        manager->clear();
        return ok;
    }

    bool testSortCapturesEachOrderOnce()
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        auto layout = dynamic_pointer_cast<ui::ClawUILayout>( manager->addElement( ui::IUILayoutWindow::typeInfo() ) );
        if( !layout ) return check( false, "order snapshot regression needs a native factory-created layout" );
        Array<SmartPtr<CollectionProbe>> probes;
        for( u32 order : { 3u, 1u, 1u, 0u } )
        {
            auto probe = make_ptr<CollectionProbe>();
            probe->order = order;
            layout->addChild( probe );
            probes.push_back( probe );
        }
        const auto before = layout->getChildren();
        for( auto probe : probes ) probe->orderReads = 0;
        layout->sortZOrder();
        bool ok = true;
        for( auto probe : probes )
        {
            ok &= check( probe->orderReads == 1,
                         "one native sort must capture each live order only once before comparing keys" );
        }
        const auto sorted = layout->getChildren();
        ok &= check( sorted.size() == 4 && sorted[0] == probes[3] && sorted[3] == probes[0],
                     "captured keys must order native collection membership" );
        Array<SmartPtr<ui::IUIElement>> tied;
        for( auto child : before )
            if( child == probes[1] || child == probes[2] ) tied.push_back( child );
        ok &= check( sorted.size() == 4 && tied.size() == 2 && sorted[1] == tied[0] && sorted[2] == tied[1],
                     "equal captured draw orders must retain their existing relative membership order" );
        layout->removeAllChildren();
        manager->clear();
        return ok;
    }

    bool testDrawPinsItsFrameMembers( Fixture &fixture )
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        fixture.application->setRenderUI( manager );
        auto layout = dynamic_pointer_cast<ui::ClawUILayout>( manager->addElement( ui::IUILayoutWindow::typeInfo() ) );
        if( !layout )
        {
            fixture.application->setRenderUI( nullptr );
            return check( false, "draw regression needs a native factory-created layout" );
        }
        layout->load( nullptr );
        auto first = make_ptr<CollectionProbe>();
        auto second = make_ptr<CollectionProbe>();
        auto replacement = make_ptr<CollectionProbe>();
        first->order = 0;
        second->order = 1;
        replacement->order = 2;
        auto removedDraws = std::make_shared<unsigned>( 0 );
        second->observedDraws = removedDraws;
        WeakPtr<ui::IUIElement> removed( second );
        layout->addChild( first );
        layout->addChild( second );
        bool changed = false, retainedDuringFrame = false;
        first->onDraw = [&] {
            if( !changed )
            {
                changed = true;
                layout->removeChild( second );
                second = nullptr;
                retainedDuringFrame = !removed.expired();
                layout->addChild( replacement );
            }
        };

        auto ctx = manager->getContext();
        bool ok = check( ctx != nullptr, "headless native draw needs its real UI context" );
        if( ctx )
        {
            wp_input_end( ctx );
            const auto frame = [&] {
                if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "CollectionSnapshot" ),
                              wp_make_rect( 0, 0, 320, 200 ), WORKPHONE_WINDOW_NO_SCROLLBAR ) )
                {
                    wp_layout_space_begin( ctx, WORKPHONE_STATIC, 200, 32 );
                    layout->draw( ctx );
                    wp_layout_space_end( ctx );
                }
                wp_end( ctx );
                wp_clear( ctx );
            };
            frame();
            ok &= check( changed && first->draws == 1,
                         "first native child must mutate membership from its draw callback" );
            ok &= check( retainedDuringFrame && *removedDraws == 1 && replacement->draws == 0,
                         "a native frame must draw its pinned members before newly added replacements" );
            ok &= check( removed.expired(), "the frame snapshot must release removed widgets after drawing" );
            frame();
            ok &= check( first->draws == 2 && *removedDraws == 1 && replacement->draws == 1,
                         "the following frame must use the replacement membership" );
        }
        first->onDraw = {};
        layout->removeAllChildren();
        layout->unload( nullptr );
        manager->clear();
        fixture.application->setRenderUI( nullptr );
        return ok;
    }

    bool testRemoveEachElement()
    {
        ui::ClawUIManager manager;
        const hash64 types[] = { ui::IUIButton::typeInfo(), ui::IUIImage::typeInfo(),
                                 ui::IUIText::typeInfo(), ui::IUITextEntry::typeInfo(),
                                 ui::IUICheckbox::typeInfo(), ui::IUIToggle::typeInfo(),
                                 ui::IUILayoutWindow::typeInfo(), ui::IUILayoutContainer::typeInfo() };
        bool ok = true;
        for( const auto type : types )
        {
            auto element = manager.addElement( type );
            ok &= check( element != nullptr, "each UI element must be created before removal" );
            if( !element )
                continue;
            WeakPtr<ui::IUIElement> weak( element );
            manager.removeElement( element );
            ok &= check( element->getLoadingState() == LoadingState::Unloaded,
                         "removing an element must unload it" );
            ok &= check( manager.getElements().empty(), "removal must release manager ownership" );
            manager.removeElement( element );
            element = nullptr;
            ok &= check( weak.expired(), "removed UI elements must actually be destroyed" );
        }
        return ok;
    }

    bool testSceneTextAppearance()
    {
        ui::ClawUIManager manager;
        auto element = manager.addElement( ui::IUIText::typeInfo() );
        auto textElement = dynamic_pointer_cast<ui::IUIText>( element );
        if( !textElement )
            return check( false, "text element must be created" );
        auto component = make_ptr<scene::Text>();
        component->setEnabled( true );
        component->setTextObject( textElement );
        component->setElement( element );
        const ColourF colour( 0.8f, 0.9f, 1.0f, 1.0f );
        component->setColour( colour );
        component->updateElementState();
        bool ok = check( element->getColour() == colour,
                         "text state updates must preserve the public colour setting" );
        const String telemetry( 2048, 'T' );
        component->setText( telemetry );
        ok &= check( component->getText() == telemetry && textElement->getText() == telemetry,
                     "HUD text longer than 128 characters must reach the render element intact" );
        auto properties = component->getProperties();
        String serializedText;
        properties->getPropertyValue( scene::Text::textPropertyStr, serializedText );
        ok &= check( serializedText == telemetry,
                     "long text must serialize without truncation" );
        component->setText( "short" );
        component->setProperties( properties );
        ok &= check( component->getText() == telemetry && textElement->getText() == telemetry,
                     "serialized long text must restore to the component and renderer" );
        ColourF serializedColour;
        properties->getPropertyValue( scene::Text::colourPropertyStr, serializedColour );
        ok &= check( serializedColour == colour,
                     "text properties must serialize the same colour that is rendered" );
        component->setHorizontalAlignment( static_cast<u8>( HorizontalAlignment::CENTER ) );
        component->setVerticalAlignment( static_cast<u8>( VerticalAlignment::CENTER ) );
        ok &= check( textElement->getHorizontalAlignment() == static_cast<u8>( HorizontalAlignment::CENTER ) &&
                     textElement->getVerticalAlignment() == static_cast<u8>( VerticalAlignment::CENTER ),
                     "alignment setters must immediately update the render element" );
        class VisibilityActor : public scene::GameActor
        {
        public:
            bool isEnabledInScene() const override { return enabledInScene; }
            bool isVisible() const override { return true; }
            bool enabledInScene = true;
        };
        auto actor = make_ptr<VisibilityActor>();
        component->setActor( actor );
        const auto enabledFlags = scene::IGameActor::ActorFlagEnabled |
                                  scene::IGameActor::ActorFlagEnabledInScene;
        actor->enabledInScene = false;
        component->updateFlags( scene::IGameActor::ActorFlagEnabled, enabledFlags );
        ok &= check( !element->isVisible() && !element->isEnabled(),
                     "hiding a UI parent must hide and disable its text" );
        actor->enabledInScene = true;
        component->updateFlags( enabledFlags, scene::IGameActor::ActorFlagEnabled );
        ok &= check( element->isVisible() && element->isEnabled(),
                     "showing a UI parent must restore its text" );
        component->setActor( nullptr );
        component->setTextObject( nullptr );
        component->setElement( nullptr );
        component = nullptr;
        manager.clear();
        return ok;
    }

    bool testEditorCameraDuringPlay( Fixture &fixture )
    {
        class CountingActor : public scene::GameActor
        {
        public:
            void update() override { ++updates; }
            u32 updates = 0;
        };
        auto actor = make_ptr<CountingActor>();
        scene::CameraManager manager;
        manager.load( nullptr );
        manager.setEditorCamera( actor );
        manager.setState( scene::ICameraManager::State::Play );
        Thread::setCurrentTask( TaskId::Application );
        fixture.application->setEditorCamera( true );
        manager.update();
        bool ok = check( actor->updates == 1,
                         "the selected editor camera must process transitions during Play" );
        fixture.application->setEditorCamera( false );
        manager.update();
        ok &= check( actor->updates == 1,
                     "an unselected editor camera must not consume game input" );
        manager.unload( nullptr );
        actor = nullptr;
        Thread::setCurrentTask( TaskId::Render );
        return ok;
    }

    bool testChildRemoval()
    {
        ui::ClawUIManager manager;
        auto parent = manager.addElement( ui::IUILayoutWindow::typeInfo() );
        auto child = manager.addElement( ui::IUIButton::typeInfo() );
        if( !parent || !child )
            return check( false, "parent and child must be created" );
        parent->addChild( child );
        WeakPtr<ui::IUIElement> weakChild( child );
        manager.removeElement( child );
        bool ok = check( parent->getChildren().empty() && !child->getParent() && !child->getLayout(),
                         "removal must detach both sides of a child relationship" );
        child = nullptr;
        ok &= check( weakChild.expired(), "a removed child must die while its parent remains alive" );
        WeakPtr<ui::IUIElement> weakParent( parent );
        manager.removeElement( parent );
        parent = nullptr;
        ok &= check( weakParent.expired(), "layout roots must not retain themselves after unload" );
        return ok;
    }

    bool testBulkRemoval()
    {
        ui::ClawUIManager manager;
        auto first = manager.addElement( ui::IUIButton::typeInfo() );
        auto second = manager.addElement( ui::IUIText::typeInfo() );
        auto survivor = manager.addElement( ui::IUIImage::typeInfo() );
        if( !first || !second || !survivor )
            return check( false, "bulk removal elements must be created" );
        WeakPtr<ui::IUIElement> weakFirst( first ), weakSecond( second ), weakSurvivor( survivor );
        manager.removeElements( { first, nullptr, second, first } );
        first = nullptr;
        second = nullptr;
        bool ok = check( weakFirst.expired() && weakSecond.expired(),
                         "bulk removal must destroy removed elements, including duplicate entries" );
        ok &= check( manager.getElements().size() == 1 && !weakSurvivor.expired(),
                     "bulk removal must preserve unrelated elements" );
        manager.clear();
        ok &= check( !weakSurvivor.expired() && survivor->getLoadingState() == LoadingState::Unloaded,
                     "clear must unload but preserve elements still owned by the caller" );
        survivor = nullptr;
        ok &= check( weakSurvivor.expired(), "cleared elements must die after the caller releases them" );
        return ok;
    }

    class TrackingInputManager : public InputDeviceManager
    {
    public:
        void addListener( SmartPtr<IEventListener> listener ) override
        {
            InputDeviceManager::addListener( listener );
            registered = listener;
            ++listenerCount;
        }
        void removeListener( SmartPtr<IEventListener> listener ) override
        {
            InputDeviceManager::removeListener( listener );
            --listenerCount;
        }
        WeakPtr<IEventListener> registered;
        int listenerCount = 0;
    };

    bool testInputListenerCleanup( Fixture &fixture )
    {
        auto input = make_ptr<TrackingInputManager>();
        fixture.application->setInputDeviceManager( input );
        auto manager = make_ptr<ui::ClawUIManager>();
        bool ok = check( input->listenerCount == 1 && !input->registered.expired(),
                         "UI manager must register one input listener" );
        manager->load( nullptr );
        ok &= check( input->listenerCount == 1, "load must not duplicate the input listener" );
        manager->unload( nullptr );
        manager->unload( nullptr );
        ok &= check( input->listenerCount == 0 && input->registered.expired(),
                     "repeated unload must unregister and destroy the input listener once" );
        manager->load( nullptr );
        ok &= check( input->listenerCount == 1 && !input->registered.expired(),
                     "loading after unload must register a new input listener" );
        manager = nullptr;
        ok &= check( input->listenerCount == 0 && input->registered.expired(),
                     "manager destruction must not leave a dangling input callback" );
        fixture.application->setInputDeviceManager( nullptr );
        return ok;
    }

    enum class Cleanup { Clear, Unload, Destructor };

    bool testTreeCleanup( Cleanup cleanup )
    {
        auto manager = make_ptr<ui::ClawUIManager>();
        manager->load( nullptr );
        auto root = manager->addElement( ui::IUILayoutWindow::typeInfo() );
        auto container = manager->addElement( ui::IUILayoutContainer::typeInfo() );
        auto leaf = manager->addElement( ui::IUIText::typeInfo() );
        if( !root || !container || !leaf )
            return check( false, "nested UI tree must be created" );
        root->addChild( container );
        container->addChild( leaf );
        WeakPtr<ui::IUIElement> weakRoot( root ), weakContainer( container ), weakLeaf( leaf );
        root = nullptr;
        container = nullptr;
        leaf = nullptr;
        bool ok = check( !weakRoot.expired() && !weakContainer.expired() && !weakLeaf.expired(),
                         "manager must own the tree before cleanup" );
        if( cleanup == Cleanup::Clear )
        {
            manager->clear();
            manager->clear();
            ok &= check( manager->getElements().empty(), "clear must empty the element registry" );
        }
        else if( cleanup == Cleanup::Unload )
        {
            manager->unload( nullptr );
            manager->unload( nullptr );
            ok &= check( manager->getElements().empty() && !manager->isLoaded(),
                         "unload must empty the registry and be safe to repeat" );
        }
        else
        {
            manager = nullptr;
        }
        ok &= check( weakRoot.expired() && weakContainer.expired() && weakLeaf.expired(),
                     "tree cleanup must destroy the layout, container, and leaf" );
        return ok;
    }
}

int main( int argc, char **argv )
{
    Fixture fixture;
    if( argc > 1 && std::strcmp( argv[1], "--collection-sort-only" ) == 0 )
    {
        bool ok = testSortingExcludesOtherThreads();
        ok &= testSortCapturesEachOrderOnce();
        std::printf( "Claw UI sorting contracts: 2 cases, %u assertions, %s.\n", assertions,
                     ok ? "passed" : "failed" );
        return ok ? 0 : 1;
    }
    bool ok = testRemoveEachElement();
    ok &= testSceneTextAppearance();
    ok &= testEditorCameraDuringPlay( fixture );
    ok &= testChildRemoval();
    ok &= testBulkRemoval();
    ok &= testTreeCleanup( Cleanup::Clear );
    ok &= testTreeCleanup( Cleanup::Unload );
    ok &= testTreeCleanup( Cleanup::Destructor );
    ok &= testInputListenerCleanup( fixture );
    ok &= testSortingExcludesOtherThreads();
    ok &= testSortCapturesEachOrderOnce();
    ok &= testDrawPinsItsFrameMembers( fixture );
    if( ok )
        std::printf( "Claw UI destruction tests: 12 cases, %u assertions passed.\n", assertions );
    return ok ? 0 : 1;
}
