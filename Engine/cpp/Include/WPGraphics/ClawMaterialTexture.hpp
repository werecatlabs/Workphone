#ifndef ClawMaterialTexture_h__
#define ClawMaterialTexture_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>

namespace workphone::render
{
    /**
     * @class ClawMaterialTexture
     * @brief Production-ready implementation of IMaterialTexture for the Claw graphics system.
     *
     * This class handles the properties of a texture used within a material pass, including
     * the texture resource itself, scaling, tinting, and optional animation.
     *
     * @see IMaterialTexture
     */
    class WPGraphics_API ClawMaterialTexture : public MaterialTexture
    {
    public:
        /** @brief Default constructor. */
        ClawMaterialTexture();

        /** @brief Destructor. */
        ~ClawMaterialTexture() override;

        ClawMaterialTexture( const ClawMaterialTexture & ) = delete;
        ClawMaterialTexture &operator=( const ClawMaterialTexture & ) = delete;

        /** @copydoc IMaterialTexture::getTextureName */
        String getTextureName() const override;

        /** @copydoc IMaterialTexture::setTextureName */
        void setTextureName( const String &name ) override;

        /** @copydoc IMaterialTexture::getTexture */
        SmartPtr<ITexture> getTexture() const override;

        /** @copydoc IMaterialTexture::setTexture */
        void setTexture( SmartPtr<ITexture> texture ) override;

        /** @copydoc IMaterialTexture::setScale */
        void setScale( const Vector3<real_Num> &scale ) override;

        /** @copydoc IMaterialTexture::getAnimator */
        SmartPtr<IAnimator> getAnimator() const override;

        /** @copydoc IMaterialTexture::setAnimator */
        void setAnimator( SmartPtr<IAnimator> animator ) override;

        /** @copydoc IMaterialTexture::getTint */
        ColourF getTint() const override;

        /** @copydoc IMaterialTexture::setTint */
        void setTint( const ColourF &tint ) override;

        /** @copydoc IMaterialTexture::getTextureType */
        u32 getTextureType() const override;

        /** @copydoc IMaterialTexture::setTextureType */
        void setTextureType( u32 textureType ) override;

        /** @copydoc IMaterialTexture::_getObject */
        void _getObject( void **ppObject ) override;

        /**
         * @brief Retrieves properties for the game editor/inspector.
         * @return Smart pointer to a Properties object describing this texture unit.
         */
        SmartPtr<Properties> getProperties() const override;

        SmartPtr<ISharedObject> toData() const override;

        void fromData( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Applies editor-supplied properties to the texture unit.
         * @param properties Smart pointer to a Properties object.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @copydoc IMaterialNode::handleStateMessage */
        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

        /** @copydoc IMaterialNode::handleStateChanged */
        bool handleStateChanged( SmartPtr<IState> &state ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        String m_textureName;          ///< Path or name of the texture resource.
        SmartPtr<ITexture> m_texture;  ///< Loaded texture resource.
        Vector3<real_Num> m_scale = Vector3<real_Num>( 1.0f, 1.0f, 1.0f );  ///< UV scaling.
        ColourF m_tint = ColourF::White;  ///< Color tint applied to the texture.
        u32 m_textureType = 0;            ///< Type of texture (e.g. Diffuse, Normal, Specular).
        SmartPtr<IAnimator> m_animator;   ///< Optional animator for UVs or texture frames.
    };
}  // namespace workphone::render

#endif  // ClawMaterialTexture_h__
