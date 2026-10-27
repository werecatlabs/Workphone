#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <memory>

namespace workphone
{
    /** Optional API backends receive the complete immutable project profile before
     * device creation. Validation must not create a device or change live state. */
    class WPCore_API IProjectRendererProvider
    {
    public:
        virtual ~IProjectRendererProvider() = default;
        virtual bool validate( SmartPtr<Properties> profile, String &error ) = 0;
        virtual SmartPtr<render::IRenderer> create( SmartPtr<Properties> profile, String &error ) = 0;
    };
    class WPCore_API IProjectShaderCompilerProvider
    {
    public:
        virtual ~IProjectShaderCompilerProvider() = default;
        /** Output must be executable by the selected API (DXBC for DX11).
         * source includes the native pass's feature defines. */
        virtual bool validate( SmartPtr<Properties> profile, const String &api, String &error ) = 0;
        virtual bool compile( SmartPtr<Properties> profile, const String &api,
                              const String &source, const String &entry, const String &target,
                              Array<u8> &bytecode, String &error ) = 0;
    };
    WPCore_API void registerProjectRendererProvider( const String &api,
        std::shared_ptr<IProjectRendererProvider> provider );
    WPCore_API std::shared_ptr<IProjectRendererProvider> getProjectRendererProvider( const String &api );
    WPCore_API void registerProjectShaderCompilerProvider( s32 language, s32 model,
        std::shared_ptr<IProjectShaderCompilerProvider> provider );
    WPCore_API std::shared_ptr<IProjectShaderCompilerProvider> getProjectShaderCompilerProvider( s32 language, s32 model );
    WPCore_API String projectRendererApi( SmartPtr<Properties> profile );
    WPCore_API bool validateProjectGraphicsProviders( SmartPtr<Properties> profile,
                                                     String &error, String &startupNotes );
    WPCore_API void releaseProjectGraphicsProviders();
}
