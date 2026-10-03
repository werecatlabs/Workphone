#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSkeleton.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsSkeleton, ISharedObject );

    IGraphicsSkeleton::~IGraphicsSkeleton() = default;

}  // namespace workphone::render
