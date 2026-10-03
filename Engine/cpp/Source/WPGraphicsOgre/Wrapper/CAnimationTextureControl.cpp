#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CAnimationTextureControl.hpp>

namespace workphone
{
    namespace render
    {
        CAnimationTextureControl::CAnimationTextureControl()
        {
        }

        CAnimationTextureControl::~CAnimationTextureControl()
        {
        }

        void CAnimationTextureControl::initialise( Ogre::TextureUnitState *textureUnit )
        {
            m_textureUnit = textureUnit;
            m_textureUnit->setCurrentFrame( 0 );
            m_animController = m_textureUnit->_getAnimController();
            m_prevFrame = 0;
        }

        void CAnimationTextureControl::update()
        {
            if( ( m_prevFrame > m_textureUnit->getCurrentFrame() ) ||
                ( m_textureUnit->getCurrentFrame() >= m_textureUnit->getNumFrames() - 1 ) )
            {
                setAnimationEnabled( false );
            }

            m_prevFrame = m_textureUnit->getCurrentFrame();
        }

        bool CAnimationTextureControl::setAnimationEnabled( bool enabled )
        {
            m_textureUnit->setCurrentFrame( 0 );
            return false;
        }

        bool CAnimationTextureControl::setAnimationEnabled( bool enabled, f32 timePosition )
        {
            return false;
        }

        bool CAnimationTextureControl::isAnimationEnabled()
        {
            return false;
        }

        bool CAnimationTextureControl::hasAnimationEnded()
        {
            return false;
        }

        void CAnimationTextureControl::setAnimationLoop( bool loop )
        {
        }

        bool CAnimationTextureControl::isAnimationLooping() const
        {
            return false;
        }

        void CAnimationTextureControl::setAnimationReversed()
        {
        }

        bool CAnimationTextureControl::isAnimationReversed()
        {
            return false;
        }

        bool CAnimationTextureControl::setTimePosition( f32 timePosition )
        {
            return false;
        }

        f32 CAnimationTextureControl::getTimePosition() const
        {
            return 0.f;
        }
    }  // namespace render
}  // namespace workphone
