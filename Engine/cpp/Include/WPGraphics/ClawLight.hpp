#ifndef ClawLight_h__
#define ClawLight_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsLight.hpp>

struct wp_light;

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawLight
         * @brief Workphone graphics light backed by the C89 wp_light implementation.
         */
        class WPGraphics_API ClawLight : public GraphicsLight
        {
        public:
            ClawLight();
            ~ClawLight() override;

            ClawLight( const ClawLight & ) = delete;
            ClawLight &operator=( const ClawLight & ) = delete;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Sets the type of the light. */
            void setType( LightTypes type ) override;

            /** @brief Gets the type of the light. */
            LightTypes getType() const override;

            /** @brief Sets the diffuse color of the light. */
            void setDiffuseColour( const ColourF &colour ) override;

            /** @brief Gets the diffuse color of the light. */
            ColourF getDiffuseColour() const override;

            /** @brief Sets the specular color of the light. */
            void setSpecularColour( const ColourF &colour ) override;

            /** @brief Gets the specular color of the light. */
            ColourF getSpecularColour() const override;

            /** @brief Sets the attenuation parameters for the light. */
            void setAttenuation( f32 range, f32 constant, f32 linear, f32 quadratic ) override;
            
            /** @brief Gets the attenuation range. */
            f32 getAttenuationRange() const override;
            
            /** @brief Gets the constant attenuation factor. */
            f32 getAttenuationConstant() const override;
            
            /** @brief Gets the linear attenuation factor. */
            f32 getAttenuationLinear() const override;
            
            /** @brief Gets the quadratic attenuation factor. */
            f32 getAttenuationQuadric() const override;

            /** @brief Sets the direction of the light. */
            void setDirection( const Vector3<real_Num> &direction ) override;
            
            /** @brief Gets the direction of the light. */
            Vector3<real_Num> getDirection() const override;
            
            /** @brief Gets the derived direction of the light. */
            Vector3<real_Num> getDerivedDirection() const override;

            /** @brief Gets the power scale of the light. */
            f32 getPowerScale() const override;
            
            /** @brief Sets the power scale of the light. */
            void setPowerScale( f32 powerScale ) override;

            /** @brief Sets whether the light is visible. */
            void setVisible( bool visible ) override;
            
            /** @brief Returns whether the light is visible. */
            bool isVisible() const override;
            
            /** @brief Sets whether the light casts shadows. */
            void setCastShadows( bool castShadows ) override;
            
            /** @brief Returns whether the light casts shadows. */
            bool getCastShadows() const override;
            
            /** @brief Sets the visibility flags for the light. */
            void setVisibilityFlags( u32 flags ) override;
            
            /** @brief Gets the visibility flags for the light. */
            u32 getVisibilityFlags() const override;

            /** @brief Creates a clone of the light object. */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;
            
            /** @brief Internal method to retrieve the underlying raw object pointer. */
            void _getObject( void **ppObject ) const override;

            /** @brief Sets the position of the light. */
            void setPosition( const Vector3<real_Num> &position );
            
            /** @brief Gets the position of the light. */
            Vector3<real_Num> getPosition() const;

            /**
             * @brief Process a state message sent to this graphics object.
             *
             * Messages are delivered by the engine state system and can be used to apply
             * asynchronous updates. Derived classes can handle specific message types.
             *
             * @param message Message to process.
             * @return True if the message was handled; false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Called when an attached IState object changes.
             *
             * This callback is intended for derived classes to react to state changes.
             *
             * @param state The state that changed.
             * @return True if the change was handled; false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state );


            void _getObject(void** ppObject) const override;

            WP_CLASS_REGISTER_DECL;

        private:
            wp_light *m_light = nullptr;  ///< Underlying Claw light implementation.
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawLight_h__
