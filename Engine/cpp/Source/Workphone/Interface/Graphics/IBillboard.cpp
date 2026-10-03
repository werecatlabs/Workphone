#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IBillboard.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone, IBillboard, ISharedObject );

    IBillboard::~IBillboard() = default;

}  // namespace workphone::render
