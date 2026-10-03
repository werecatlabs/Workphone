#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowEvent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsWindowEvent, IEvent );

    IGraphicsWindowEvent::IGraphicsWindowEvent() : IEvent( IGraphicsWindowEvent::typeInfo() )
    {
    }

    IGraphicsWindowEvent::~IGraphicsWindowEvent() = default;

}  // namespace workphone::render
