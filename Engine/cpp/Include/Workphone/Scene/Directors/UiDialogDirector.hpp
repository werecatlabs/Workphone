#ifndef UiDialogDirector_h__
#define UiDialogDirector_h__

#include <Workphone/Scene/Directors/UiElementDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /** UI dialog director implementation. */
        class WPCore_API UiDialogDirector : public UiElementDirector
        {
        public:
            /** Constructor. */
            UiDialogDirector();

            /** Destructor. */
            ~UiDialogDirector() override;

            /** @copydoc Director::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Director::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Get button director.
             * @return The button director.
             */
            SmartPtr<ButtonDirector> getButtonDirector() const;

            /** Set button director.
             * @param buttonDirector The button director.
             */
            void setButtonDirector( SmartPtr<ButtonDirector> buttonDirector );

            /** Get tab button director.
             * @return The tab button director.
             */
            SmartPtr<ButtonDirector> getTabButtonDirector() const;

            /** Set tab button director.
             * @param tabButtonDirector The tab button director.
             */
            void setTabButtonDirector( SmartPtr<ButtonDirector> tabButtonDirector );

            /** Get background texture.
             * @return The background texture.
             */
            SmartPtr<render::ITexture> getBackgroundTexture() const;

            /** Set background texture.
             * @param backgroundTexture The background texture.
             */
            void setBackgroundTexture( SmartPtr<render::ITexture> backgroundTexture );

            /** Get title.
             * @return The title.
             */
            String getTitle() const;

            /** Set title.
             * @param title The title.
             */
            void setTitle( const String &title );

            WP_CLASS_REGISTER_DECL;

        protected:
            // The button director.
            SmartPtr<ButtonDirector> m_buttonDirector;

            // The tab button director.
            SmartPtr<ButtonDirector> m_tabButtonDirector;

            // The background texture.
            SmartPtr<render::ITexture> m_backgroundTexture;

            // The title text.
            String m_title;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // UiDialogDirector_h__
