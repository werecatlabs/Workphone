#include <WPGraphics/UI/ClawUIManager.hpp>
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
#include <cstdio>
#include <memory>

using namespace workphone;

namespace
{
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
            Thread::setCurrentTask( TaskId::Render );
        }

        ~Fixture()
        {
            application->setRenderUI( nullptr );
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
        if( !condition )
            std::fprintf( stderr, "FAIL: %s\n", message );
        return condition;
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

int main()
{
    Fixture fixture;
    bool ok = testRemoveEachElement();
    ok &= testChildRemoval();
    ok &= testBulkRemoval();
    ok &= testTreeCleanup( Cleanup::Clear );
    ok &= testTreeCleanup( Cleanup::Unload );
    ok &= testTreeCleanup( Cleanup::Destructor );
    ok &= testInputListenerCleanup( fixture );
    if( ok )
        std::puts( "Claw UI destruction tests passed." );
    return ok ? 0 : 1;
}
