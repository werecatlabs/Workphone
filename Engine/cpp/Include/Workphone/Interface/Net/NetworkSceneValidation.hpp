#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace workphone::network
{
    /** Snapshot of authoring settings; no scene pointers or engine runtime needed. */
    struct AuthoredNetworkView
    {
        std::uintptr_t actorIdentity = 0;
        std::int32_t objectId = -1;
        std::uint32_t ownerId = 0;
        int authority = 1;  // NetworkView: Owner=0, Server=1, AnyPeer=2.
        bool reliable = false;
        bool ownershipRequests = false;
        bool active = true;
    };

    struct NetworkAuthoringCapabilities
    {
        bool known = false;
        bool unreliable = false;
        bool reliable = false;
    };

    enum class NetworkSceneIssueCode
    {
        DuplicateComponent,
        UnassignedObjectId,
        DuplicateObjectId,
        InvalidOwnerId,
        UnsupportedAuthority,
        UnsupportedDelivery,
        UnsupportedOwnershipRequests
    };

    struct NetworkSceneIssue
    {
        static constexpr std::size_t NoRelatedView = static_cast<std::size_t>( -1 );
        NetworkSceneIssueCode code;
        std::size_t viewIndex;
        std::size_t relatedViewIndex = NoRelatedView;
    };

    inline const char *networkSceneIssueMessage( NetworkSceneIssueCode code )
    {
        switch( code )
        {
        case NetworkSceneIssueCode::DuplicateComponent:
            return "Only one Network View is supported per actor.";
        case NetworkSceneIssueCode::UnassignedObjectId:
            return "Assign a nonnegative View ID before network play.";
        case NetworkSceneIssueCode::DuplicateObjectId:
            return "View ID is also used by another active Network View.";
        case NetworkSceneIssueCode::InvalidOwnerId:
            return "Owner ID must be 0 (server) or a player ID from 1 through 65534.";
        case NetworkSceneIssueCode::UnsupportedAuthority:
            return "Authority must be Owner or Server; Any Peer has no supported receive policy.";
        case NetworkSceneIssueCode::UnsupportedDelivery:
            return "The active network backend does not support the selected delivery mode.";
        case NetworkSceneIssueCode::UnsupportedOwnershipRequests:
            return "Ownership requests require a backend with reliable ordered delivery.";
        }
        return "Invalid Network View settings.";
    }

    /** Validates the authored routing contract, without assigning IDs or mutating the scene.
     * Inactive views do not participate in routing or capability checks. Duplicate
     * components remain invalid even when disabled: NetworkView only supports one
     * component per actor. Unknown capabilities permit offline authoring.
     */
    inline std::vector<NetworkSceneIssue> validateNetworkViews(
        const std::vector<AuthoredNetworkView> &views, NetworkAuthoringCapabilities capabilities )
    {
        std::vector<NetworkSceneIssue> issues;
        std::unordered_map<std::uintptr_t, std::size_t> actors;
        std::unordered_map<std::int32_t, std::size_t> ids;
        for( std::size_t i = 0; i < views.size(); ++i )
        {
            const auto &view = views[i];
            const auto actor = actors.emplace( view.actorIdentity, i );
            if( !actor.second )
            {
                issues.push_back(
                    { NetworkSceneIssueCode::DuplicateComponent, i, actor.first->second } );
            }
            if( !view.active )
            {
                continue;
            }
            if( view.objectId < 0 )
            {
                issues.push_back( { NetworkSceneIssueCode::UnassignedObjectId, i } );
            }
            else
            {
                const auto identity = ids.emplace( view.objectId, i );
                if( !identity.second )
                {
                    issues.push_back(
                        { NetworkSceneIssueCode::DuplicateObjectId, i, identity.first->second } );
                }
            }
            if( view.ownerId > 65534 )
            {
                issues.push_back( { NetworkSceneIssueCode::InvalidOwnerId, i } );
            }
            if( view.authority != 0 && view.authority != 1 )
            {
                issues.push_back( { NetworkSceneIssueCode::UnsupportedAuthority, i } );
            }
            if( capabilities.known &&
                !( view.reliable ? capabilities.reliable : capabilities.unreliable ) )
            {
                issues.push_back( { NetworkSceneIssueCode::UnsupportedDelivery, i } );
            }
            if( capabilities.known && view.ownershipRequests && !capabilities.reliable )
            {
                issues.push_back( { NetworkSceneIssueCode::UnsupportedOwnershipRequests, i } );
            }
        }
        return issues;
    }
}  // namespace workphone::network
