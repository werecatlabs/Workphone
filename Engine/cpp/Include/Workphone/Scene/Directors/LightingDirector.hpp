#ifndef SceneLightingDirector_h__
#define SceneLightingDirector_h__

#include <Workphone/Scene/Directors/ResourceDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class LightingDirector
         * @brief Director for scene lighting resources (ambient/hemisphere/environment settings).
         *
         * LightingDirector exposes serializable lighting parameters used by the rendering
         * subsystem such as ambient colour, hemisphere lighting colours and direction,
         * and environment map scale. These properties are applied to scene-level lighting
         * configuration when the resource is loaded.
         */
        class WPCore_API LightingDirector : public ResourceDirector
        {
        public:
            /** Property key for ambient colour in Properties objects. */
            static const String ambientColourStr;

            /** Property key for the upper hemisphere colour in Properties objects. */
            static const String upperHemisphereStr;

            /** Property key for the lower hemisphere colour in Properties objects. */
            static const String lowerHemisphereStr;

            /** Property key for the hemisphere direction vector in Properties objects. */
            static const String hemisphereDirStr;

            /** Property key for the environment map scale in Properties objects. */
            static const String envmapScaleStr;

            /**
             * @brief Construct a LightingDirector with default lighting values.
             */
            LightingDirector();

            /**
             * @brief Destructor.
             */
            ~LightingDirector() override;

            /**
             * @brief Get a Properties object representing the director's current settings.
             * @return SmartPtr<Properties> containing ambient/hemisphere/envmap parameters.
             * @copydoc ResourceDirector::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply settings from a Properties object to the director.
             * @param properties Properties containing lighting parameters to apply.
             * @copydoc ResourceDirector::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the ambient scene colour.
             * @return ColourF used as scene ambient light.
             */
            ColourF getAmbientColour() const;

            /**
             * @brief Set the ambient scene colour.
             * @param ambientColour Colour to use for ambient lighting.
             */
            void setAmbientColour( const ColourF &ambientColour );

            /**
             * @brief Get the upper hemisphere light colour (sky contribution).
             * @return ColourF representing the upper hemisphere colour.
             */
            ColourF getUpperHemisphere() const;

            /**
             * @brief Set the upper hemisphere light colour (sky contribution).
             * @param upperHemisphere Colour for the upper hemisphere.
             */
            void setUpperHemisphere( const ColourF &upperHemisphere );

            /**
             * @brief Get the lower hemisphere light colour (ground contribution).
             * @return ColourF representing the lower hemisphere colour.
             */
            ColourF getLowerHemisphere() const;

            /**
             * @brief Set the lower hemisphere light colour (ground contribution).
             * @param lowerHemisphere Colour for the lower hemisphere.
             */
            void setLowerHemisphere( const ColourF &lowerHemisphere );

            /**
             * @brief Get the hemisphere lighting direction vector.
             * @return Unit vector indicating the hemisphere's up direction.
             */
            Vector3<real_Num> getHemisphereDir() const;

            /**
             * @brief Set the hemisphere lighting direction vector.
             * @param hemisphereDir Direction vector (will typically be normalized by caller).
             */
            void setHemisphereDir( Vector3<real_Num> hemisphereDir );

            /**
             * @brief Get the environment map intensity scale.
             * @return Multiplier applied to environment map lighting.
             */
            f32 getEnvmapScale() const;

            /**
             * @brief Set the environment map intensity scale.
             * @param envmapScale Multiplier applied to environment map contribution.
             */
            void setEnvmapScale( f32 envmapScale );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Scene ambient colour. Default: White.
             */
            ColourF m_ambientColour = ColourF::White;

            /**
             * @brief Upper hemisphere colour (sky). Default: White.
             */
            ColourF m_upperHemisphere = ColourF::White;

            /**
             * @brief Lower hemisphere colour (ground). Default: White.
             */
            ColourF m_lowerHemisphere = ColourF::White;

            /**
             * @brief Direction used for hemisphere lighting (usually world up).
             * Default: unit Y vector.
             */
            Vector3<real_Num> m_hemisphereDir = Vector3<real_Num>::unitY();

            /**
             * @brief Multiplier applied to environment map lighting. Default: 1.0.
             */
            f32 m_envmapScale = 1.0f;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // SceneLightingDirector_h__
