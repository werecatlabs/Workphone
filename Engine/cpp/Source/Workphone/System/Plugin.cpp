#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Plugin.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

namespace workphone::core
{

    WP_CLASS_REGISTER_DERIVED( workphone::core, Plugin, IPlugin );

    Plugin::Plugin() = default;

    Plugin::~Plugin() = default;

    void Plugin::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        const auto filePath = getFilePath();

#if defined WP_PLATFORM_WIN32
        auto pFilePath = filePath.c_str();
        m_hMod = LoadLibraryW( pFilePath );
#elif defined WP_PLATFORM_APPLE
        auto filePathUTF8 = StringUtil::toUTF16to8( filePath );
        auto pFilePathUTF8 = filePathUTF8.c_str();
        auto mode = RTLD_LOCAL | RTLD_LAZY;
        m_hMod = dlopen( pFilePathUTF8, mode );
#elif defined WP_PLATFORM_LINUX
        auto filePathUTF8 = StringUtil::toUTF16to8( filePath );
        auto pFilePathUTF8 = filePathUTF8.c_str();
        auto mode = RTLD_LOCAL | RTLD_LAZY;
        m_hMod = dlopen( pFilePathUTF8, mode );
#endif

        if( !m_hMod )
        {
            const auto errorMessage = "Unable to load library: " + StringUtil::toUTF16to8( filePath );
            WP_LOG_ERROR( errorMessage );
        }

        setLoadingState( LoadingState::Loaded );
    }

    void Plugin::reload( SmartPtr<ISharedObject> data )
    {
        unload( data );
        reload( data );
    }

    void Plugin::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

#if defined WP_PLATFORM_WIN32
        if( m_hMod )
        {
            FreeLibrary( m_hMod );
            m_hMod = nullptr;
        }
#else
        if( m_hMod )
        {
            dlclose( m_hMod );
            m_hMod = 0;
        }
#endif

        setLoadingState( LoadingState::Unloaded );
    }

    auto Plugin::getLibraryHandle() const -> LibraryHandle
    {
        return m_hMod;
    }

    void Plugin::setLibraryHandle( LibraryHandle handle )
    {
        m_hMod = handle;
    }

    LibraryFunction Plugin::getFunction( const String &name ) const
    {
        auto handle = getLibraryHandle();

#if defined WP_PLATFORM_WIN32
        auto address = GetProcAddress( handle, name.c_str() );
#else
        auto address = dlsym( handle, name.c_str() );
#endif

        return address;
    }

    auto Plugin::getFilePath() const -> StringW
    {
        return m_filename;
    }

    void Plugin::setFilePath( const StringW &fileName )
    {
        m_filename = fileName;
    }

}  // namespace workphone::core
