#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IComputeShader.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, IComputeShader, IShader );

        IComputeShader::~IComputeShader() = default;
    }  // namespace render
}  // namespace workphone
