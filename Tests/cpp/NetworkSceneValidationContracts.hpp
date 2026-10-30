#pragma once

#include <Workphone/Scene/NetworkSceneValidation.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Core/Handle.hpp>
#include <WPNetwork/WPNetworkManager.hpp>
#include <algorithm>
#include <stdexcept>

inline bool networkSceneValidationContracts(
    workphone::SmartPtr<workphone::core::ApplicationManager> application )
{
    using namespace workphone;

    struct Fixture
    {
        SmartPtr<core::ApplicationManager> application;
        SmartPtr<scene::IGameManager> previous;
        bool editor;
        SmartPtr<scene::GameManager> manager = make_ptr<scene::GameManager>();
        SmartPtr<scene::GameScene> world = make_ptr<scene::GameScene>();
        SmartPtr<scene::GameActor> parent = make_ptr<scene::GameActor>();
        SmartPtr<scene::GameActor> child = make_ptr<scene::GameActor>();
        SmartPtr<scene::NetworkView> a = make_ptr<scene::NetworkView>();
        SmartPtr<scene::NetworkView> b = make_ptr<scene::NetworkView>();
        SmartPtr<scene::NetworkView> extra = make_ptr<scene::NetworkView>();
        bool hasExtra = false;

        explicit Fixture( SmartPtr<core::ApplicationManager> value ) :
            application( value ),
            previous( value->getGameManager() ),
            editor( value->isEditor() )
        {
            application->setGameManager( manager );
            application->setEditor( false );
            manager->setCurrentScene( world );
        }

        ~Fixture()
        {
            if( hasExtra )
            {
                child->removeComponentInstance( extra );
            }
            parent->removeComponentInstance( a );
            child->removeComponentInstance( b );
            parent->removeChild( child );
            world->setActors( {} );
            manager->setCurrentScene( nullptr );
            application->setGameManager( previous );
            application->setEditor( editor );
        }
    } fixture( application );

    auto require = []( bool condition, const char *message ) {
        if( !condition )
        {
            throw std::runtime_error( message );
        }
    };
    fixture.parent->getHandle()->setInstanceId( 1 );
    fixture.child->getHandle()->setInstanceId( 2 );
    fixture.parent->setEnabled( true );
    fixture.child->setEnabled( true );
    fixture.a->setEnabled( true );
    fixture.b->setEnabled( true );
    fixture.a->setViewId( 7 );
    fixture.b->setViewId( 7 );
    fixture.parent->addComponentInstance( fixture.a );
    fixture.child->addComponentInstance( fixture.b );
    fixture.parent->addChild( fixture.child );
    // A descendant listed both in the root snapshot and under its parent must
    // only be validated once. This uses real scene/actor component attachment.
    fixture.world->setActors( { fixture.parent, fixture.child } );
    auto backend = make_ptr<WPNetworkManager>();
    auto report = scene::validateNetworkScene( fixture.world, backend );
    require( report.views.size() == 2 && report.issues.size() == 1 &&
                 report.issues[0].code == network::NetworkSceneIssueCode::DuplicateObjectId,
             "real scene validation missed duplicate IDs or visited a descendant twice" );
    fixture.b->setViewId( 8 );
    require( scene::validateNetworkScene( fixture.world, backend ).issues.empty(),
             "valid real scene failed network validation" );
    fixture.b->setDeliveryMode( scene::NetworkView::DeliveryMode::Reliable );
    report = scene::validateNetworkScene( fixture.world, backend );
    require( report.issues.size() == 1 &&
                 report.issues[0].code == network::NetworkSceneIssueCode::UnsupportedDelivery,
             "real manager capabilities did not reject reliable UDP authoring" );
    fixture.parent->setEnabled( false );
    require( scene::validateNetworkScene( fixture.world, backend ).issues.empty(),
             "disabled hierarchy still participated in network routing validation" );
    fixture.parent->setEnabled( true );
    fixture.b->setDeliveryMode( scene::NetworkView::DeliveryMode::Unreliable );
    fixture.b->setViewId( -1 );
    fixture.b->setReplicationEnabled( false );
    require( scene::validateNetworkScene( fixture.world, backend ).issues.empty(),
             "disabled replication still required a routing ID" );
    fixture.child->addComponentInstance( fixture.extra );
    fixture.hasExtra = true;
    report = scene::validateNetworkScene( fixture.world, backend );
    require( std::any_of( report.issues.begin(), report.issues.end(),
                          []( const auto &issue ) {
                              return issue.code == network::NetworkSceneIssueCode::DuplicateComponent;
                          } ),
             "multiple real NetworkViews on one actor were not rejected" );
    return true;
}
