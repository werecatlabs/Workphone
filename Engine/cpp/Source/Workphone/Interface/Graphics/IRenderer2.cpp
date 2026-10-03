#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IRenderer2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IRenderer2, IRenderer );

    IRenderer2::~IRenderer2() = default;

}  // namespace workphone::render
