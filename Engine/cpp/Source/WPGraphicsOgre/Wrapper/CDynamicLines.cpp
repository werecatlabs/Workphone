#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDynamicLines.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <WPGraphicsOgre/Addons/DynamicLines.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CDynamicLines, CGraphicsObjectOgre<IDynamicLines> );

        CDynamicLines::CDynamicLines( SmartPtr<IGraphicsScene> creator ) : m_creator( creator )
        {
        }

        CDynamicLines::~CDynamicLines()
        {
            if( m_dynamicLines )
            {
                delete m_dynamicLines;
                m_dynamicLines = nullptr;
            }
        }

        void CDynamicLines::initialise()
        {
            m_dynamicLines = new DynamicLinesOgre;
        }

        void CDynamicLines::update()
        {
            if( m_dynamicLines )
            {
                m_dynamicLines->update();
            }
        }

        void CDynamicLines::setMaterialName( const String &materialName, s32 index )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            // m_dynamicLines->setMaterial(materialName.c_str());
        }

        SmartPtr<IGraphicsObject> CDynamicLines::clone( const String &name ) const
        {
            return nullptr;
        }

        void CDynamicLines::_getObject( void **ppObject ) const
        {
            *ppObject = m_dynamicLines;
        }

        void CDynamicLines::addPoint( const Vector3F &point )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_dynamicLines->addPoint( point.X(), point.Y(), point.Z() );
        }

        void CDynamicLines::setPoint( u32 index, const Vector3F &point )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            Ogre::Vector3 ogrePoint;  // (point);
            m_dynamicLines->setPoint( index, ogrePoint );
        }

        Vector3F CDynamicLines::getPoint( u32 index ) const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            Ogre::Vector3 point = m_dynamicLines->getPoint( index );
            return Vector3F( point.ptr() );
        }

        u32 CDynamicLines::getNumPoints() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return m_dynamicLines->getNumPoints();
        }

        void CDynamicLines::clear()
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_dynamicLines->clear();
        }

        void CDynamicLines::setDirty()
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_dynamicLines->update();
        }

        void CDynamicLines::setOperationType( u32 opType )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            // m_dynamicLines->setOperationType((Ogre::RenderOperation::OperationType)opType);
        }

        u32 CDynamicLines::getOperationType() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            return 0;  //(u32)m_dynamicLines->getOperationType();
        }
    }  // end namespace render
}  // namespace workphone
