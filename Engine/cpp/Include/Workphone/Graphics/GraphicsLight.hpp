#ifndef __GraphicsObject_Light_h__
#define __GraphicsObject_Light_h__

#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsLight.hpp>

namespace workphone
{
    namespace render
    {
        /** Implementation of the ILight interface.
         */
        class WPCore_API GraphicsLight : public GraphicsObject<IGraphicsLight>
        {
        public:
            /** Constructor. */
            GraphicsLight();

            /** Destructor. */
            ~GraphicsLight() override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setType */
            void setType( LightTypes type ) override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getType */
            LightTypes getType() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setDiffuseColour */
            void setDiffuseColour( const ColourF &colour ) override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getDiffuseColour */
            ColourF getDiffuseColour() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setSpecularColour */
            void setSpecularColour( const ColourF &colour ) override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getSpecularColour */
            ColourF getSpecularColour() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setAttenuation */
            void setAttenuation( f32 range, f32 constant, f32 linear, f32 quadratic ) override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getAttenuationRange */
            f32 getAttenuationRange() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getAttenuationConstant */
            f32 getAttenuationConstant() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getAttenuationLinear */
            f32 getAttenuationLinear() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getAttenuationQuadric */
            f32 getAttenuationQuadric() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setDirection */
            void setDirection( const Vector3<real_Num> &vec ) override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getDirection */
            Vector3<real_Num> getDirection() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getDerivedDirection */
            Vector3<real_Num> getDerivedDirection() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setPowerScale */
            f32 getPowerScale() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setPowerScale */
            void setPowerScale( f32 powerScale ) override;

            /** @copydoc GraphicsObject<IGraphicsLight>::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc GraphicsObject<IGraphicsLight>::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CLight_h__
