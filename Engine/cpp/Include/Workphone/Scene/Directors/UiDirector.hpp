#ifndef UiDirector_h__
#define UiDirector_h__

#include <Workphone/System/Director.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class UiDirector
         * @brief Director that configures global UI resources and default widgets.
         *
         * UiDirector provides configuration for UI-level resources such as the default
         * button director, start dialog director, and default background texture. It
         * also exposes feature toggles used by the UI (character/vehicle/scene select,
         * and workshop availability). These settings are serializable via the
         * Properties API and used by the UI system when building menus and dialogs.
         */
        class WPCore_API UiDirector : public Director
        {
        public:
            /**
             * @brief Construct a UiDirector with default settings.
             */
            UiDirector();

            /**
             * @brief Destructor.
             */
            ~UiDirector() override;

            /**
             * @brief Retrieve properties representing the UI configuration.
             * @return SmartPtr<Properties> containing serializable UI settings.
             * @copydoc IBuildDirector::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply UI configuration from a Properties object.
             * @param properties Properties containing UI settings to apply.
             * @copydoc IBuildDirector::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the default ButtonDirector used to style buttons.
             * @return SmartPtr<ButtonDirector> referencing the default button director (may be null).
             */
            SmartPtr<ButtonDirector> getDefaultButtonDirector() const;

            /**
             * @brief Set the default ButtonDirector used for newly created buttons.
             * @param defaultButtonDirector Smart pointer to the ButtonDirector (may be null).
             */
            void setDefaultButtonDirector( SmartPtr<ButtonDirector> defaultButtonDirector );

            /**
             * @brief Get the director used to construct the start (main) dialog.
             * @return SmartPtr<UiDialogDirector> referencing the start dialog director (may be null).
             */
            SmartPtr<UiDialogDirector> getStartDialogDirector() const;

            /**
             * @brief Set the director used to construct the start (main) dialog.
             * @param startDialogDirector Smart pointer to the UiDialogDirector to use.
             */
            void setStartDialogDirector( SmartPtr<UiDialogDirector> startDialogDirector );

            /**
             * @brief Get the default background texture used by UI panels.
             * @return SmartPtr<render::ITexture> referencing the texture (may be null).
             */
            SmartPtr<render::ITexture> getDefaultBackgroundTexture() const;

            /**
             * @brief Set the default background texture used by UI panels.
             * @param defaultBackgroundTexture Smart pointer to the texture (may be null).
             */
            void setDefaultBackgroundTexture( SmartPtr<render::ITexture> defaultBackgroundTexture );

            /**
             * @brief Query whether the UI includes a character selection screen.
             * @return True if character select is enabled.
             */
            bool hasCharacterSelect() const;

            /**
             * @brief Enable or disable the character selection UI.
             * @param hasCharacterSelect True to enable character select.
             */
            void setHasCharacterSelect( bool hasCharacterSelect );

            /**
             * @brief Query whether the UI includes a vehicle selection screen.
             * @return True if vehicle select is enabled.
             */
            bool hasVehicleSelect() const;

            /**
             * @brief Enable or disable the vehicle selection UI.
             * @param hasVehicleSelect True to enable vehicle select.
             */
            void setHasVehicleSelect( bool hasVehicleSelect );

            /**
             * @brief Query whether the UI includes a scene selection screen.
             * @return True if scene select is enabled.
             */
            bool hasSceneSelect() const;

            /**
             * @brief Enable or disable the scene selection UI.
             * @param hasSceneSelect True to enable scene select.
             */
            void setHasSceneSelect( bool hasSceneSelect );

            /**
             * @brief Query whether the UI includes workshop/editor functionality.
             * @return True if workshop features are enabled.
             */
            bool hasWorkshop() const;

            /**
             * @brief Enable or disable workshop/editor UI features.
             * @param hasWorkshop True to enable workshop features.
             */
            void setHasWorkshop( bool hasWorkshop );

            WP_CLASS_REGISTER_DECL;

            /** Property key for the default button director reference. */
            static const String defaultButtonDirectorStr;

            /** Property key for the start dialog director reference. */
            static const String startDialogDirectorStr;

            /** Property key for the default background texture reference. */
            static const String defaultBackgroundTextureStr;

            /** Property key for enabling/disabling character select UI. */
            static const String hasCharacterSelectStr;

            /** Property key for enabling/disabling vehicle select UI. */
            static const String hasVehicleSelectStr;

            /** Property key for enabling/disabling scene select UI. */
            static const String hasSceneSelectStr;

            /** Property key for enabling/disabling workshop UI. */
            static const String hasWorkshopStr;

        protected:
            /** Default director used to create and style buttons. */
            SmartPtr<ButtonDirector> m_defaultButtonDirector;

            /** Director used to construct the application's start/main dialog. */
            SmartPtr<UiDialogDirector> m_startDialogDirector;

            /** Default background texture applied to UI panels and menus (may be null). */
            SmartPtr<render::ITexture> m_defaultBackgroundTexture;

            /** Feature toggle: character select screen available. Default: false. */
            bool m_hasCharacterSelect = false;

            /** Feature toggle: vehicle select screen available. Default: false. */
            bool m_hasVehicleSelect = false;

            /** Feature toggle: scene select screen available. Default: false. */
            bool m_hasSceneSelect = false;

            /** Feature toggle: workshop/editor UI available. Default: false. */
            bool m_hasWorkshop = false;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // UiDirector_h__
