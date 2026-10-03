#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    const hash_type IGraphicsObject::STATE_MESSAGE_RENDER_QUEUE = StringUtil::getHash( "renderQueue" );
    const hash_type IGraphicsObject::STATE_MESSAGE_DIRECTION = StringUtil::getHash( "direction" );

    const u32 IGraphicsObject::AllProperties = ( 1 << 0 ); /* 0x01*/

    const u32 IGraphicsObject::OverlayFlag = 1 << 1;
    const u32 IGraphicsObject::UiFlag = 1 << 2;
    const u32 IGraphicsObject::SceneFlag = 1 << 3;

    const u32 IGraphicsObject::attachedFlag = 1 << 1;
    const u32 IGraphicsObject::receiveShadowsFlag = 1 << 2;
    const u32 IGraphicsObject::visibleFlag = 1 << 3;
    const u32 IGraphicsObject::castShadowsFlag = 1 << 4;

    // Property key string definitions
    const String IGraphicsObject::namePropertyStr = "name";
    const String IGraphicsObject::visiblePropertyStr = "visible";
    const String IGraphicsObject::castShadowsPropertyStr = "castShadows";
    const String IGraphicsObject::receiveShadowsPropertyStr = "receiveShadows";
    const String IGraphicsObject::renderTechniquePropertyStr = "renderTechnique";
    const String IGraphicsObject::renderQueueGroupPropertyStr = "renderQueueGroup";
    const String IGraphicsObject::zOrderPropertyStr = "zOrder";
    const String IGraphicsObject::visibilityMaskPropertyStr = "visibilityMask";
    const String IGraphicsObject::localAABBPropertyStr = "localAABB";
    const String IGraphicsObject::attachedPropertyStr = "attached";
    const String IGraphicsObject::flagsPropertyStr = "flags";

    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsObject, ISharedObject );

    IGraphicsObject::IGraphicsObject() : ISharedObject( IGraphicsObject::typeInfo() )
    {
    }

    IGraphicsObject::IGraphicsObject( u32 poolTypeId ) : ISharedObject( poolTypeId )
    {
    }

    IGraphicsObject::~IGraphicsObject() = default;

}  // namespace workphone::render
