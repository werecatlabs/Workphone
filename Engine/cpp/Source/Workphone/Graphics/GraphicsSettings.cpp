#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsSettings.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsSettings, Director );

    GraphicsSettings::GraphicsSettings() = default;

    GraphicsSettings::~GraphicsSettings() = default;

    auto GraphicsSettings::getCreateWindow() const -> bool
    {
        return m_createWindow;
    }

    void GraphicsSettings::setCreateWindow( bool createWindow )
    {
        m_createWindow = createWindow;
    }

    auto GraphicsSettings::getShowDialog() const -> bool
    {
        return m_showDialog;
    }

    void GraphicsSettings::setShowDialog( bool showDialog )
    {
        m_showDialog = showDialog;
    }

    void GraphicsSettings::setCreateRenderUI( bool createRenderUI )
    {
        m_createRenderUI = createRenderUI;
    }

    bool GraphicsSettings::getCreateRenderUI() const
    {
        return m_createRenderUI;
    }

    bool GraphicsSettings::getVSync() const
    {
        return m_vsync;
    }

    void GraphicsSettings::setVSync( bool enabled )
    {
        m_vsync = enabled;
    }

    void GraphicsSettings::setAnisotropy( s32 anisotropy )
    {
        m_anisotropy = anisotropy;
    }

    s32 GraphicsSettings::getAnisotropy() const
    {
        return m_anisotropy;
    }

    void GraphicsSettings::setTextureFiltering( s32 textureFiltering )
    {
        m_textureFiltering = textureFiltering;
    }

    s32 GraphicsSettings::getTextureFiltering() const
    {
        return m_textureFiltering;
    }

}  // namespace workphone::render
