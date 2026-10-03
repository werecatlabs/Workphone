#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IGraphicsMesh, IGraphicsObject );

    const hash_type IGraphicsMesh::RENDER_QUEUE_HASH = StringUtil::getHash( "renderQueue" );
    const hash_type IGraphicsMesh::VISIBILITY_FLAGS_HASH = StringUtil::getHash( "visibilityFlags" );

    const String IGraphicsMesh::meshNamePropertyStr = "MeshName";
    const String IGraphicsMesh::castShadowsPropertyStr = "castShadows";

    IGraphicsMesh::IGraphicsMesh() : IGraphicsObject( IGraphicsMesh::typeInfo() )
    {
    }

    IGraphicsMesh::~IGraphicsMesh() = default;

}  // namespace workphone::render
