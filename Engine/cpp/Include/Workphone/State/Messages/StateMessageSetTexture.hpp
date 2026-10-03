#ifndef StateMessageSetTexture_h__
#define StateMessageSetTexture_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    class WPCore_API StateMessageSetTexture : public StateMessage
    {
    public:
        StateMessageSetTexture();
        ~StateMessageSetTexture() override;

        SmartPtr<render::ITexture> getTexture() const;

        void setTexture( SmartPtr<render::ITexture> texture );

        String getTextureName() const;
        void setTextureName( const String &textureName );

        u32 getTextureIndex() const;
        void setTextureIndex( u32 textureIndex );

        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<render::ITexture> m_texture;
        String m_textureName;
        u32 m_textureIndex = 0;
    };
}  // namespace workphone

#endif  // StateMessageSetTexture_h__
