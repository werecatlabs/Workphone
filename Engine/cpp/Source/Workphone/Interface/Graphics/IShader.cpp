#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IShader.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IShader, ISharedObject );

    IShader::~IShader() = default;

}  // namespace workphone::render
