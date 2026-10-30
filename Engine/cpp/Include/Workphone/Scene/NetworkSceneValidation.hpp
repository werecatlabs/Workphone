#pragma once

#include <Workphone/Interface/Net/NetworkSceneValidation.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Scene/Components/NetworkView.hpp>
#include <unordered_set>

namespace workphone::scene
{
    struct NetworkSceneValidation
    {
        std::vector<SmartPtr<NetworkView>> views;
        std::vector<SmartPtr<IGameActor>> actors;
        std::vector<network::NetworkSceneIssue> issues;
    };

    /** Capture the scene under its existing Editor/application task locks.
     * Traversal is iterative and deduplicates actors, including scenes whose root
     * list also contains descendants. This makes validation safe for malformed
     * cyclic hierarchies without interpreting the cycle as duplicate components.
     */
    inline NetworkSceneValidation validateNetworkScene( SmartPtr<IGameScene> scene,
                                                        SmartPtr<INetworkManager> manager )
    {
        NetworkSceneValidation result;
        if( !scene )
        {
            return result;
        }
        std::vector<SmartPtr<IGameActor>> pending;
        std::unordered_set<IGameActor *> visited;
        for( auto actor : scene->getActors() )
        {
            pending.push_back( actor );
        }
        std::vector<network::AuthoredNetworkView> settings;
        while( !pending.empty() )
        {
            auto actor = pending.back();
            pending.pop_back();
            if( !actor || !visited.insert( actor.get() ).second )
            {
                continue;
            }
            const bool enabled = actor->isEnabledInScene();
            for( auto component : actor->getComponents() )
            {
                if( auto view = workphone::dynamic_pointer_cast<NetworkView>( component ) )
                {
                    result.views.push_back( view );
                    result.actors.push_back( actor );
                    settings.push_back(
                        { reinterpret_cast<std::uintptr_t>( actor.get() ), view->getViewId(),
                          view->getOwnerId(), static_cast<int>( view->getAuthorityMode() ),
                          view->getDeliveryMode() == NetworkView::DeliveryMode::Reliable,
                          view->getAllowOwnershipRequests(),
                          enabled && view->isEnabled() && view->isReplicationEnabled() } );
                }
            }
            for( auto child : actor->getChildren() )
            {
                pending.push_back( child );
            }
        }
        network::NetworkAuthoringCapabilities capabilities;
        if( manager )
        {
            capabilities.known = true;
            const auto flags = manager->getCapabilities();
            capabilities.unreliable = ( flags & INetworkManager::UnreliableDelivery ) != 0;
            capabilities.reliable = ( flags & INetworkManager::ReliableOrderedDelivery ) != 0;
        }
        result.issues = network::validateNetworkViews( settings, capabilities );
        return result;
    }
}  // namespace workphone::scene
