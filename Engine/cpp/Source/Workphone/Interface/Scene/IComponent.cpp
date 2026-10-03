#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IComponent, IResource );

    const hash_type IComponent::actorFlagsChanged = StringUtil::getHash( "actorFlagsChanged" );
    const hash_type IComponent::actorReset = StringUtil::getHash( "actorReset" );
    const hash_type IComponent::actorUnload = StringUtil::getHash( "actorUnload" );
    const hash_type IComponent::sceneWasLoaded = StringUtil::getHash( "sceneWasLoaded" );
    const hash_type IComponent::parentChanged = StringUtil::getHash( "parentChanged" );
    const hash_type IComponent::hierarchyChanged = StringUtil::getHash( "hierarchyChanged" );
    const hash_type IComponent::childAdded = StringUtil::getHash( "childAdded" );
    const hash_type IComponent::childRemoved = StringUtil::getHash( "childRemoved" );
    const hash_type IComponent::childAddedInHierarchy = StringUtil::getHash( "childAddedInHierarchy" );
    const hash_type IComponent::childRemovedInHierarchy =
        StringUtil::getHash( "childRemovedInHierarchy" );

    const hash_type IComponent::visibilityChanged = StringUtil::getHash( "visibilityChanged" );
    const hash_type IComponent::enabledChanged = StringUtil::getHash( "enabledChanged" );
    const hash_type IComponent::staticChanged = StringUtil::getHash( "staticChanged" );

    const hash_type IComponent::triggerCollisionEnter = StringUtil::getHash( "triggerCollisionEnter" );
    const hash_type IComponent::triggerCollisionLeave = StringUtil::getHash( "triggerCollisionLeave" );
    const hash_type IComponent::componentLoaded = StringUtil::getHash( "componentLoaded" );

    const u32 IComponent::ComponentReservedFlag = ( 1 << 0 );
    const u32 IComponent::ComponentEnabledFlag = ( 1 << 1 );

    const Array<String> IComponent::componentStateNames = { "None", "Create", "Destroyed", "Edit",
                                                            "Play", "Pause",  "Reset",     "Count" };

    const String IComponent::enabledStr = "Enabled";
    const String IComponent::dirtyStr = "Dirty";
    const String IComponent::nameStr = "name";
    const String IComponent::componentFlagsStr = "componentFlags";
    const String IComponent::stateStr = "state";

    IComponent::IComponent() : IResource( IComponent::typeInfo() )
    {
    }

    IComponent::IComponent( u32 poolTypeID ) : IResource( poolTypeID )
    {
    }

    IComponent::~IComponent() = default;

}  // namespace workphone::scene
