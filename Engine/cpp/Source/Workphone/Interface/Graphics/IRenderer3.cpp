#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IRenderer3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IRenderer3, IRenderer );

    IRenderer3::~IRenderer3() = default;

}  // namespace workphone::render
