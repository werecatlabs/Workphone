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

    bool IGraphicsMesh::applySkeletonPose( SmartPtr<ISkeleton>, String &error )
    {
        error = "Skeleton pose publication is unsupported by this graphics backend.";
        return false;
    }

    bool IGraphicsMesh::loadResource(std::shared_ptr<const resource::RuntimeResource>,String &error,
                                     const String &,bool)
    {
        error="Cooked mesh installation is unsupported by this graphics backend.";
        return false;
    }

}  // namespace workphone::render
