#ifndef CNativeFileDialog_h__
#define CNativeFileDialog_h__

#include <Workphone/Interface/IO/INativeFileDialog.hpp>

namespace workphone
{
    /** NativeFileDialog class.
     *  This class is used to open a native file dialog window.
     */
    class WPCore_API NativeFileDialog : public INativeFileDialog
    {
    public:
        /** Default constructor. */
        NativeFileDialog();

        /** Destructor. */
        ~NativeFileDialog() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::reload */
        void reload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc INativeFileDialog::openDialog */
        Result openDialog() override;

        /** @copydoc INativeFileDialog::getFilePath */
        String getFilePath() const override;

        /** @copydoc INativeFileDialog::setFilePath */
        void setFilePath( const String &filePath ) override;

        /** @copydoc INativeFileDialog::getFileExtension */
        String getFileExtension() const override;

        /** @copydoc INativeFileDialog::setFileExtension */
        void setFileExtension( const String &fileExtension ) override;

        /** @copydoc INativeFileDialog::getDialogMode */
        DialogMode getDialogMode() const override;

        /** @copydoc INativeFileDialog::setDialogMode */
        void setDialogMode( DialogMode mode ) override;

        /** @copydoc INativeFileDialog::getFilterMode */
        FilterMode getFilterMode() const override;

        /** @copydoc INativeFileDialog::setFilterMode */
        void setFilterMode( FilterMode mode ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        // The selected file path.
        String m_filePath;

        // The selected file extension.
        String m_fileExtension;

        // The dialog mode (Select, Open or Save).
        DialogMode m_dialogMode = DialogMode::Open;

        // The filter mode (Files or Directories).
        FilterMode m_filterMode = FilterMode::FilterMode_Dirs;
    };
}  // namespace workphone

#endif  // CNativeFileDialog_h__
