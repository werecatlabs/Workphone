#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsDeferredShading.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsDeferredShading, ISharedObject );

    IGraphicsDeferredShading::~IGraphicsDeferredShading() = default;

}  // namespace workphone::render
