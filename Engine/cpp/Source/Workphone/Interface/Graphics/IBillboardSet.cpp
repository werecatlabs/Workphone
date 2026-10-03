#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IBillboardSet.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone, IBillboardSet, IGraphicsObject );

    IBillboardSet::~IBillboardSet() = default;

}  // namespace workphone::render
