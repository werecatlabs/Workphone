#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/RigidbodyListener.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, RigidbodyListener, IEventListener );

    RigidbodyListener::RigidbodyListener() = default;

    RigidbodyListener::~RigidbodyListener() = default;

    auto RigidbodyListener::handleEvent( EventType eventType, hash_type eventValue,
                                         const Array<Parameter> &arguments,
                                         SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                         SmartPtr<IEvent> event ) -> Parameter
    {
        if( eventValue == IEvent::transform )
        {
            if( auto owner = getOwner() )
            {
                if( auto actor = owner->getActorPtr() )
                {
                    auto actorTransform = actor->getTransformPtr();
                    if( !actorTransform->isDirty() || !actorTransform->isLocalDirty() )
                    {
                        auto position = arguments[0].getVector3();
                        auto orientation = arguments[1].getQuaternion();
                        orientation.normalise();

                        const auto scale = actorTransform->getScale();

                        const auto t = Transform3<real_Num>( position, orientation, scale );
                        handleTransform( static_cast<physics::IPhysicsBody3 *>( sender.get() ), t );
                    }
                }
            }
        }

        return {};
    }

    void RigidbodyListener::handleTransform( physics::IPhysicsBody3 *body,
                                             const Transform3<real_Num> &t )
    {
        auto owner = getOwner();
        if( !owner )
            return;

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto timer = applicationManager->getTimerPtr();
        WP_ASSERT( timer );

        const auto time = timer->getTime();

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        auto actor = owner->getActorPtr();
        if( !actor )
            return;

        // Hoist shared position/orientation extraction — used by both branches.
        const auto &position = t.getPosition();
        const auto &orientation = t.getOrientation();

        auto actorTransform = actor->getTransform();
        if( actorTransform->getSmoothMotion() )
        {
            // History is keyed by the publishing task, including when smoothing
            // is enabled at runtime after the rigidbody entered play.
            actorTransform->setTask( Thread::getCurrentTask() );
            const auto actorHandle = actor->getHandle();
            const auto id = actorHandle->getInstanceId();

            if( body->isDerived<physics::IRigidBody3>() )
            {
                auto rigidBody = static_cast<physics::IRigidBody3 *>( body );
                sceneManager->addTransformState( id, time, t, rigidBody->getLinearVelocity(),
                                                 rigidBody->getAngularVelocity() );
            }
            else
            {
                sceneManager->addTransformState( id, time, t );
            }
        }
        else
        {
            if( auto transform = actor->getTransformPtr() )
            {
                transform->setLocalPosition( position );
                transform->setLocalOrientation( orientation );

                transform->setPosition( position );
                transform->setOrientation( orientation );

                transform->setDirty( true, true );
            }
        }
    }

    void RigidbodyListener::setOwner( Rigidbody *owner )
    {
        m_owner = owner;
    }

}  // namespace workphone::scene
