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

    bool IGraphicsTerrain::loadRuntimeResource(
        std::shared_ptr<const resource::RuntimeResource>,
        std::shared_ptr<const resource::RuntimeResource>,
        std::shared_ptr<resource::ResourcePayloadMount>,const String &,bool,String &error)
    {
        error="This graphics backend does not support captured terrain resources";
        return false;
    }

}  // namespace workphone::render
