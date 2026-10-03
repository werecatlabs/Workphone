#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIAnimatedMaterial.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIAnimatedMaterial::ClawUIAnimatedMaterial()
    {
        m_type = "AnimatedMaterial";
    }

    ClawUIAnimatedMaterial::~ClawUIAnimatedMaterial() = default;

    void ClawUIAnimatedMaterial::update()
    {
        if( m_isPlaying && isEnabled() && isVisible() )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto timer = applicationManager->getTimer() )
                {
                    m_time += static_cast<f32>( timer->getDeltaTime() );
                    if( m_frameTime > 0.0f && m_numFrames > 0 )
                    {
                        const auto frame = static_cast<u32>( m_time / m_frameTime ) % m_numFrames;
                        if( frame != m_currentFrame )
                        {
                            m_currentFrame = frame;
                            OnEnterFrame();
                        }
                    }
                }
            }
        }

        ClawUIElement::update();
    }

    void ClawUIAnimatedMaterial::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    String ClawUIAnimatedMaterial::getMaterialName() const
    {
        return m_materialName;
    }

    void ClawUIAnimatedMaterial::play()
    {
        m_isPlaying = true;
    }

    void ClawUIAnimatedMaterial::pause()
    {
        m_isPlaying = false;
    }

    void ClawUIAnimatedMaterial::stop()
    {
        m_isPlaying = false;
        m_time = 0.0f;
        m_currentFrame = 0;
    }

    void ClawUIAnimatedMaterial::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUIAnimatedMaterial::setSize( const Vector2F &size )
    {
        ClawUIElement::setSize( size );
    }

    void ClawUIAnimatedMaterial::setFrameTime( f32 time )
    {
        m_frameTime = MathF::max( time, 0.0f );
    }

    f32 ClawUIAnimatedMaterial::getFrameTime() const
    {
        return m_frameTime;
    }

    u32 ClawUIAnimatedMaterial::getCurrentFrame() const
    {
        return m_currentFrame;
    }

    u32 ClawUIAnimatedMaterial::getNumFrames() const
    {
        return m_numFrames;
    }

    void ClawUIAnimatedMaterial::OnEnterFrame() const
    {
    }

    void ClawUIAnimatedMaterial::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        wp_button_color( ctx, ClawUIWorkphoneContext::toWorkphoneColor( getColour() ) );
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
