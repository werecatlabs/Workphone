#ifndef AddDialog_h__
#define AddDialog_h__

#include <EditorPrerequisites.hpp>
#include "ui/EditorWindow.hpp"

namespace workphone
{
    namespace editor
    {
        class AddDialog : public EditorWindow
        {
        public:
            enum
            {
                CANCEL_BTN_ID,
                USE_DEFAULTS_CHK,
                PropertiesId,
            };

            AddDialog();
            ~AddDialog() override;

            String getLabel() const;
            bool getUseDefaults() const;

            void setProperties( SmartPtr<Properties> properties ) override;
            SmartPtr<Properties> getProperties() const override;

        protected:
            void OnCancelBtn();
            void OnPropertyChange();

            // wxPropertyGrid*	m_properties;

            // wxCheckBox* m_useDefaultsChkBox;
            // wxButton* m_okBtn;
            // wxButton* m_cancelBtn;

            SmartPtr<Properties> m_properties;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // AddDialog_h__
