#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/ISubComponent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ISubComponent, IResource );

    ISubComponent::ISubComponent( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    ISubComponent::ISubComponent() : IResource( ISubComponent::typeInfo() )
    {
    }

    ISubComponent::~ISubComponent() = default;

}  // namespace workphone::scene
