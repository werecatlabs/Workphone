#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/CameraVisibilityState.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        GraphicsObjectVisibility::GraphicsObjectVisibility() :
            m_nextUpdate( std::numeric_limits<u32>::max() ),
            m_pixelDrawn( std::numeric_limits<u32>::max() ),
            m_isCulled( false ),
            m_queryObject( true )
        {
            Ogre::RenderSystem *rs = Ogre::Root::getSingleton().getRenderSystem();
            m_occlusionQuery = rs->createHardwareOcclusionQuery();
        }

        void GraphicsObjectVisibility::update()
        {
            if( !m_queryObject && !m_occlusionQuery->isStillOutstanding() )
            {
                m_pixelDrawn = m_occlusionQuery->getLastQuerysPixelcount();
                m_queryObject = true;
            }
            else
            {
                m_queryObject = false;
            }
        }

        CameraVisibilityState::CameraVisibilityState()
        {
            m_state = STATE_READY;

            //m_renderQueueVisibilityStatePool.setNextSize( 3000 );
            m_planes = new Ogre::Plane[6];

            m_renderQueue = new Ogre::RenderQueue;

            // init render queues that do not need shadows
            m_renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_BACKGROUND )->setShadowsEnabled( false );
            m_renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_OVERLAY )->setShadowsEnabled( false );
            m_renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_SKIES_EARLY )->setShadowsEnabled( false );
            m_renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_SKIES_LATE )->setShadowsEnabled( false );
        }

        CameraVisibilityState::~CameraVisibilityState()
        {
            WP_SAFE_DELETE( m_planes );
        }

        bool CameraVisibilityState::isVisible( const Ogre::AxisAlignedBox &bound ) const
        {
            // Null boxes always invisible
            if( bound.isNull() )
                return false;

            // Infinite boxes always visible
            if( bound.isInfinite() )
                return true;

            // Get centre of the box
            Ogre::Vector3 centre = bound.getCenter();
            // Get the half-size of the box
            Ogre::Vector3 halfSize = bound.getHalfSize();

            // For each plane, see if all points are on the negative side
            // If so, object is not visible
            for( int plane = 0; plane < 6; ++plane )
            {
                Ogre::Plane::Side side = m_planes[plane].getSide( centre, halfSize );
                if( side == Ogre::Plane::NEGATIVE_SIDE )
                {
                    return false;
                }
            }

            return true;
        }

    }  // namespace render
}  // namespace workphone
