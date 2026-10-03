#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IInstancedObject.hpp>

namespace workphone::render
{

    const hash_type IInstancedObject::RENDER_QUEUE_HASH = StringUtil::getHash( "renderQueue" );
    const hash_type IInstancedObject::VISIBILITY_FLAGS_HASH = StringUtil::getHash( "visibilityFlags" );
    const hash_type IInstancedObject::CUSTOM_PARAMETER_HASH =
        StringUtil::getHash( "custom_parameter_hash" );
    const u32 IInstancedObject::MAX_CUSTOM_PARAMS = 8;

    IInstancedObject::~IInstancedObject() = default;

}  // namespace workphone::render
