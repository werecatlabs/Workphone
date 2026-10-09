#ifndef _ILight_H
#define _ILight_H

#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for a graphics light.
         */
        class WPCore_API IGraphicsLight : public IGraphicsObject
        {
        public:
            /**
             * Hash codes for material properties.
             */
            static const hash_type VISIBILITY_MASK_HASH;  ///< Hash code for the light visibility mask.
            static const hash_type LIGHT_TYPE_HASH;       ///< Hash code for the light type.
            static const hash_type DIFFUSE_COLOUR_HASH;   ///< Hash code for the light's diffuse colour.
            static const hash_type SPECULAR_COLOUR_HASH;  ///< Hash code for the light's specular colour.

            static const String lightStr;

            IGraphicsLight();

            /** Virtual destructor. */
            ~IGraphicsLight() override;

            /**
             * Sets the type of the light.
             * @param type The type of the light.
             */
            virtual void setType( LightTypes type ) = 0;

            /**
             * Returns the type of the light.
             * @return The type of the light.
             */
            virtual LightTypes getType() const = 0;

            /**
             * Sets the diffuse colour of the light.
             * @param colour The diffuse colour of the light.
             */
            virtual void setDiffuseColour( const ColourF &colour ) = 0;

            /**
             * Returns the diffuse colour of the light.
             * @return The diffuse colour of the light.
             */
            virtual ColourF getDiffuseColour() const = 0;

            /**
             * Sets the specular colour of the light.
             * @param colour The specular colour of the light.
             */
            virtual void setSpecularColour( const ColourF &colour ) = 0;

            /**
             * Returns the specular colour of the light.
             * @return The specular colour of the light.
             */
            virtual ColourF getSpecularColour() const = 0;

            /**
             * Sets the attenuation parameters of the light.
             * @param range The range of the attenuation.
             * @param constant The constant attenuation factor.
             * @param linear The linear attenuation factor.
             * @param quadratic The quadratic attenuation factor.
             */
            virtual void setAttenuation( f32 range, f32 constant, f32 linear, f32 quadratic ) = 0;

            /**
             * Returns the range of the attenuation of the light.
             * @return The range of the attenuation of the light.
             */
            virtual f32 getAttenuationRange() const = 0;

            /**
             * Returns the constant attenuation factor of the light.
             * @return The constant attenuation factor of the light.
             */
            virtual f32 getAttenuationConstant() const = 0;

            /**
             * Returns the linear attenuation factor of the light.
             * @return The linear attenuation factor of the light.
             */
            virtual f32 getAttenuationLinear() const = 0;

            /**
             * Returns the quadratic attenuation factor of the light.
             * @return The quadratic attenuation factor of the light.
             */
            virtual f32 getAttenuationQuadric() const = 0;

            /**
             * Sets the direction in which the light points.
             * @param vec The direction in which the light points.
             */
            virtual void setDirection( const Vector3<real_Num> &vec ) = 0;

            /**
             * Returns the direction in which the light points.
             * @return The direction in which the light points.
             */
            virtual Vector3<real_Num> getDirection() const = 0;

            /**
             * Returns the derived direction in which the light points.
             * @return The derived direction in which the light points.
             */
            virtual Vector3<real_Num> getDerivedDirection() const = 0;

            /**
             * Gets the power scale of the light.
             * @return The power scale of the light.
             */
            virtual f32 getPowerScale() const = 0;

            /**
             * Sets the power scale of the light.
             * @param powerScale The power scale of the light.
             */
            virtual void setPowerScale( f32 powerScale ) = 0;

            /** @brief Sets the position of the light. */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /** @brief Gets the position of the light. */
            virtual Vector3<real_Num> getPosition() const = 0;

            /** @brief Sets the range and angles for a spotlight. */
            virtual void setSpotlightRange( f32 innerAngle, f32 outerAngle, f32 falloff ) = 0;

            /** @brief Gets the inner angle of the spotlight. */
            virtual f32 getSpotlightInnerAngle() const = 0;

            /** @brief Gets the outer angle of the spotlight. */
            virtual f32 getSpotlightOuterAngle() const = 0;

            /** @brief Gets the falloff of the spotlight. */
            virtual f32 getSpotlightFalloff() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
