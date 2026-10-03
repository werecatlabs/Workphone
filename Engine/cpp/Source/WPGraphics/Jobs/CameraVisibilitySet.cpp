#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Jobs/CameraVisibilitySet.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, CameraVisibilitySet, ISharedObject );

        CameraVisibilitySet::CameraVisibilitySet( hash_type cameraId ) : m_cameraId( cameraId )
        {
        }

        void CameraVisibilitySet::addVisible( const SmartPtr<IGraphicsSceneNode> &node )
        {
            if( node )
            {
                m_visibleNodes.push_back( node );
            }
        }

        void CameraVisibilitySet::clear()
        {
            m_visibleNodes.clear();
        }

        Array<SmartPtr<IGraphicsSceneNode>> CameraVisibilitySet::snapshot() const
        {
            return m_visibleNodes.snapshot();
        }

        hash_type CameraVisibilitySet::getCameraId() const
        {
            return m_cameraId;
        }

        u32 CameraVisibilitySet::getNumVisible() const
        {
            return static_cast<u32>( m_visibleNodes.snapshot().size() );
        }

    }  // namespace render
}  // namespace workphone
