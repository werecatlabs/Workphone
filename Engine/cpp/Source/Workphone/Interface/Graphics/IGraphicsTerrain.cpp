#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsTerrain, ISharedObject );

    IGraphicsTerrain::IGraphicsTerrain() : ISharedObject( IGraphicsTerrain::typeInfo() )
    {
    }

    IGraphicsTerrain::~IGraphicsTerrain() = default;

}  // namespace workphone::render
