#ifndef PanelDirector_h__
#define PanelDirector_h__

#include <Workphone/System/Director.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class PanelDirector
         * @brief Director responsible for panel UI resources and their textures.
         *
         * PanelDirector manages an optional texture resource used to render 2D panels
         * in the UI or scene. It provides serialization via the Properties API so the
         * texture can be referenced in build/deserialization workflows.
         */
        class WPCore_API PanelDirector : public Director
        {
        public:
            /**
             * @brief Construct a PanelDirector with no texture assigned.
             */
            PanelDirector();

            /**
             * @brief Destructor.
             */
            ~PanelDirector() override;

            /**
             * @brief Return a Properties object describing this director's configuration.
             * @return SmartPtr<Properties> containing the panel's serializable settings.
             * @copydoc IBuildDirector::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply configuration from a Properties object to this director.
             * @param properties Properties object containing texture references and settings.
             * @copydoc IBuildDirector::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the texture used by this panel director.
             * @return SmartPtr<render::ITexture> referencing the panel texture (may be null).
             */
            SmartPtr<render::ITexture> getTexture() const;

            /**
             * @brief Set the texture to be used by this panel director.
             * @param texture Smart pointer to the texture resource (may be null to clear).
             */
            void setTexture( SmartPtr<render::ITexture> texture );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Optional texture resource used to render the panel.
             *
             * Stored as a smart pointer so the manager can hold or share ownership of the
             * render resource. May be null when no texture is set.
             */
            SmartPtr<render::ITexture> m_texture;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // PanelDirector_h__
