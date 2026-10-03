#ifndef MaterialResourceDirector_h__
#define MaterialResourceDirector_h__

#include <Workphone/Scene/Directors/ResourceDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class MaterialResourceDirector
         * @brief Director responsible for material resource configuration.
         *
         * MaterialResourceDirector exposes material-related build and runtime settings
         * such as preferred texture type and maximum texture size. These values are
         * serializable via the Properties API and consumed when preparing or loading
         * material assets for the scene.
         */
        class WPCore_API MaterialResourceDirector : public ResourceDirector
        {
        public:
            /** Property key for preferred texture type (for example "2D", "Cube"). */
            static const String textureTypeStr;

            /** Property key for maximum texture size (in pixels). */
            static const String textureSizeStr;

            /**
             * @brief Construct a MaterialResourceDirector with default values.
             *
             * Default texture size is set to 8192 unless overridden via properties.
             */
            MaterialResourceDirector();

            /**
             * @brief Destructor.
             */
            ~MaterialResourceDirector() override;

            /**
             * @brief Retrieve a Properties object representing current material settings.
             * @return SmartPtr<Properties> containing texture type/size properties.
             * @copydoc IBuildDirector::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply material settings from a Properties object.
             * @param properties Properties containing configuration values to apply.
             * @copydoc IBuildDirector::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the preferred texture type for materials.
             * @return String describing the texture type (implementation-defined semantics).
             */
            String getTextureType() const;

            /**
             * @brief Set the preferred texture type for materials.
             * @param textureType String describing the texture type (e.g. "2D", "Cube").
             */
            void setTextureType( const String &textureType );

            /**
             * @brief Get the configured maximum texture size (in pixels).
             * @return Maximum texture dimension (e.g. 8192).
             */
            s32 getTextureSize() const;

            /**
             * @brief Set the maximum texture size (in pixels).
             * @param textureSize Maximum allowed texture dimension.
             */
            void setTextureSize( s32 textureSize );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Preferred texture type for materials. Empty string when unspecified.
             */
            String m_textureType;

            /**
             * @brief Maximum texture size (width/height) used when building/loading materials.
             * Default value: 8192.
             */
            s32 m_textureSize = 8192;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // MaterialResourceDirector_h__
