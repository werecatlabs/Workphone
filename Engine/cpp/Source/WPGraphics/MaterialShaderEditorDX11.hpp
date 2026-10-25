#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
namespace workphone::render
{
    class ClawMaterial;
    namespace detail
    {
        void shaderReloadAttached(ClawMaterial *,u64);
        void prepareShaderReloadStatus(ClawMaterial *);
        void forgetShaderReload(ClawMaterial *) noexcept;
        void appendShaderFiles(const ClawMaterial *,SmartPtr<Properties>);
        void copyShaderFiles(const ClawMaterial *,ClawMaterial *);
    }
}
