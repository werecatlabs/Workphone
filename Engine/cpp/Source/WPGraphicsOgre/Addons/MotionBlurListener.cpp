#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/MotionBlurListener.hpp"

template <>
workphone::MotionBlurListener *Ogre::Singleton<workphone::MotionBlurListener>::msSingleton = NULL;

namespace workphone
{

    MotionBlurListener::MotionBlurListener()
    {
        m_iLastRenderTime = 0;
        m_blurAdjust = 20;
    }

    MotionBlurListener::~MotionBlurListener()
    {
    }

    void MotionBlurListener::notifyViewportSize( int width, int height )
    {
    }

    void MotionBlurListener::notifyCompositor( Ogre::CompositorInstance *instance )
    {
    }

    void MotionBlurListener::notifyMaterialSetup( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat )
    {
        if( pass_id == 666 )
        {
            m_blurParameters = mat->getTechnique( 0 )->getPass( 0 )->getFragmentProgramParameters();
        }
    }

    void MotionBlurListener::notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat )
    {
        if( pass_id == 666 )
        {
            unsigned long iRenderTime = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
            Ogre::Real ElapsedTime = Ogre::Real( iRenderTime - m_iLastRenderTime ) / 1000.0;
            if( ElapsedTime < 0 )
                ElapsedTime = 0;
            Ogre::Real Blur = Ogre::Math::Exp( -ElapsedTime * m_blurAdjust );
            m_blurParameters->setNamedConstant( "blur", Blur );
            m_iLastRenderTime = iRenderTime;
        }
    }

    void MotionBlurListener::setBlurAdjust( Ogre::Real blurAdjust )
    {
        m_blurAdjust = blurAdjust;
    }

    Ogre::Real MotionBlurListener::getBlurAdjust() const
    {
        return m_blurAdjust;
    }

}  // namespace workphone
