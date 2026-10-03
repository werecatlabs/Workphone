#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageSetTexture.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageSetTexture, StateMessage );

    StateMessageSetTexture::StateMessageSetTexture() = default;

    StateMessageSetTexture::~StateMessageSetTexture() = default;

    auto StateMessageSetTexture::getTexture() const -> SmartPtr<render::ITexture>
    {
        return m_texture;
    }

    void StateMessageSetTexture::setTexture( SmartPtr<render::ITexture> texture )
    {
        m_texture = texture;
    }

    auto StateMessageSetTexture::getTextureName() const -> String
    {
        return m_textureName;
    }

    void StateMessageSetTexture::setTextureName( const String &textureName )
    {
        m_textureName = textureName;
    }

    auto StateMessageSetTexture::getTextureIndex() const -> u32
    {
        return m_textureIndex;
    }

    void StateMessageSetTexture::setTextureIndex( u32 textureIndex )
    {
        m_textureIndex = textureIndex;
    }
}  // namespace workphone
