#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/Wrapper/CAnimationTextureControl.hpp"

namespace workphone::render
{

    //--------------------------------------------
    CAnimationTextureControl::CAnimationTextureControl() = default;

    //--------------------------------------------
    CAnimationTextureControl::~CAnimationTextureControl() = default;

    //--------------------------------------------
    void CAnimationTextureControl::initialise( Ogre::TextureUnitState *textureUnit )
    {
        m_textureUnit = textureUnit;
        m_textureUnit->setCurrentFrame( 0 );
        m_animController = m_textureUnit->_getAnimController();
        m_prevFrame = 0;
    }

    //--------------------------------------------
    void CAnimationTextureControl::update( const s32 &task, const time_interval &t,
                                           const time_interval &dt )
    {
        if( ( m_prevFrame > m_textureUnit->getCurrentFrame() ) ||
            ( m_textureUnit->getCurrentFrame() >= m_textureUnit->getNumFrames() - 1 ) )
        {
            setAnimationEnabled( false );
        }

        m_prevFrame = m_textureUnit->getCurrentFrame();
    }

    //--------------------------------------------
    auto CAnimationTextureControl::setAnimationEnabled( bool enabled ) -> bool
    {
        m_textureUnit->setCurrentFrame( 0 );
        return false;
    }

    //--------------------------------------------
    auto CAnimationTextureControl::setAnimationEnabled( bool enabled, f32 timePosition ) -> bool
    {
        return false;
    }

    //--------------------------------------------
    auto CAnimationTextureControl::isAnimationEnabled() -> bool
    {
        return false;
    }

    //--------------------------------------------
    auto CAnimationTextureControl::hasAnimationEnded() -> bool
    {
        return false;
    }

    //--------------------------------------------
    void CAnimationTextureControl::setAnimationLoop( bool loop )
    {
    }

    //--------------------------------------------
    auto CAnimationTextureControl::isAnimationLooping() const -> bool
    {
        return false;
    }

    //--------------------------------------------
    void CAnimationTextureControl::setAnimationReversed()
    {
    }

    //--------------------------------------------
    auto CAnimationTextureControl::isAnimationReversed() -> bool
    {
        return false;
    }

    //--------------------------------------------
    auto CAnimationTextureControl::setTimePosition( f32 timePosition ) -> bool
    {
        return false;
    }

    //--------------------------------------------
    auto CAnimationTextureControl::getTimePosition() const -> f32
    {
        return 0.f;
    }

}  // namespace workphone::render
