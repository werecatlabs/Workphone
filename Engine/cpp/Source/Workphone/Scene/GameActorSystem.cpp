#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GameActorSystem.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Thread/ScopedLock.hpp>

namespace workphone::scene
{
    struct GameActorComponentUpdates
    {
        struct Entry
        {
            SmartPtr<IComponent> component;
            bool automaticUpdate;
        };

        Array<Entry> entries{ Allocator<Entry>( sourceBudget ) };
    };

    std::shared_ptr<const GameActorComponentUpdates> GameActorSystem::pinComponents( GameActor &actor )
    {
        {
            ScopedLock lock( &actor.m_componentsMutex, false );
            if( actor.m_componentUpdates || !actor.m_componentsHead )
            {
                return actor.m_componentUpdates;
            }
        }

        ScopedLock lock( &actor.m_componentsMutex );
        if( actor.m_componentUpdates || !actor.m_componentsHead )
        {
            return actor.m_componentUpdates;
        }

        auto updates = std::allocate_shared<GameActorComponentUpdates>(
            SourceBudgetAllocator<GameActorComponentUpdates>{} );
        size_t count = 0;
        for( auto component = actor.m_componentsHead; component; component = component->m_next.load() )
        {
            ++count;
        }
        updates->entries.reserve( count );
        for( auto component = actor.m_componentsHead; component; component = component->m_next.load() )
        {
            // RenderTexture::update is an explicit refresh, not a scene tick.
            updates->entries.push_back( { component, !component->isDerived<RenderTexture>() } );
        }
        actor.m_componentUpdates = updates;
        return updates;
    }

    void GameActorSystem::dispatch( GameActor &actor, Phase phase )
    {
        // Pin component lifetimes across callbacks, and release the membership
        // lock before entering code that may change the actor or lock graphics.
        const auto updates = pinComponents( actor );
        if( !updates )
        {
            return;
        }

        auto callback = &IComponent::update;
        if( phase == Phase::PreUpdate )
        {
            callback = &IComponent::preUpdate;
        }
        else if( phase == Phase::PostUpdate )
        {
            callback = &IComponent::postUpdate;
        }

        for( const auto &entry : updates->entries )
        {
            if( ( phase != Phase::Update || entry.automaticUpdate ) &&
                entry.component->getActorPtr() == &actor )
            {
                ( entry.component.get()->*callback )();
            }
        }
    }
}  // namespace workphone::scene
