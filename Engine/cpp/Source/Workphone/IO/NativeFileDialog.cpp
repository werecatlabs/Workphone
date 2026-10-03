#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/NativeFileDialog.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <nativefiledialog/include/nfd.h>
#include <nativefiledialog/include/common.h>
#include <iostream>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, NativeFileDialog, INativeFileDialog );

    NativeFileDialog::NativeFileDialog() = default;

    NativeFileDialog::~NativeFileDialog() = default;

    void NativeFileDialog::load( SmartPtr<ISharedObject> data )
    {
    }

    void NativeFileDialog::reload( SmartPtr<ISharedObject> data )
    {
    }

    void NativeFileDialog::unload( SmartPtr<ISharedObject> data )
    {
    }

    auto NativeFileDialog::openDialog() -> INativeFileDialog::Result
    {
        nfdchar_t *pOutPath = nullptr;
        auto result = static_cast<nfdresult_t>( 0 );

        auto filterList = StringUtil::replaceAll( getFileExtension(), ".", "" );
        auto defaultPath = getFilePath();

#ifdef WP_PLATFORM_WIN32
        auto path = StringUtil::replaceAll( defaultPath, "/", "\\" );
#else
        auto path = StringUtil::cleanupPath( defaultPath );
#endif

        if( !Path::isExistingFile( path ) )
        {
            path = "";
        }

        auto mode = getDialogMode();
        switch( mode )
        {
        case DialogMode::Select:
        {
            result = NFD_PickFolder( path.c_str(), &pOutPath );
        }
        break;
        case DialogMode::Open:
        {
            result = NFD_OpenDialog( filterList.c_str(), path.c_str(), &pOutPath );
        }
        break;
        case DialogMode::Save:
        {
            result = NFD_SaveDialog( filterList.c_str(), path.c_str(), &pOutPath );
        }
        break;
        }

        if( pOutPath )
        {
            auto outPath = StringUtil::cleanupPath( pOutPath );
            setFilePath( outPath );
        }

        return static_cast<INativeFileDialog::Result>( result );
    }

    auto NativeFileDialog::getFilePath() const -> String
    {
        return m_filePath;
    }

    void NativeFileDialog::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    auto NativeFileDialog::getFileExtension() const -> String
    {
        return m_fileExtension;
    }

    void NativeFileDialog::setFileExtension( const String &fileExtension )
    {
        m_fileExtension = fileExtension;
    }

    auto NativeFileDialog::getDialogMode() const -> INativeFileDialog::DialogMode
    {
        return m_dialogMode;
    }

    void NativeFileDialog::setDialogMode( DialogMode mode )
    {
        m_dialogMode = mode;
    }

    auto NativeFileDialog::getFilterMode() const -> INativeFileDialog::FilterMode
    {
        return m_filterMode;
    }

    void NativeFileDialog::setFilterMode( FilterMode mode )
    {
        m_filterMode = mode;
    }
}  // namespace workphone
