#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDynamicLines.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/DynamicLines.hpp>
#include <Ogre.h>
#include <Math/Array/OgreObjectMemoryManager.h>
#include <utility>

namespace workphone::render
{

    CDynamicLines::CDynamicLines( SmartPtr<render::IGraphicsScene> creator ) :
        m_creator( std::move( creator ) )
    {
    }

    CDynamicLines::~CDynamicLines()
    {
        if( m_dynamicLines )
        {
            delete m_dynamicLines;
            m_dynamicLines = nullptr;
        }

        if( m_memoryManager )
        {
            delete m_memoryManager;
            m_memoryManager = nullptr;
        }
    }

    void CDynamicLines::initialise()
    {
        Ogre::SceneManager *smgr = nullptr;
        m_creator->_getObject( reinterpret_cast<void **>( &smgr ) );

        auto id = Ogre::Id::generateNewId<DynamicLinesOgreNext>();
        m_memoryManager = new Ogre::ObjectMemoryManager();
        m_dynamicLines = new DynamicLinesOgreNext( id, m_memoryManager, smgr );
    }

    void CDynamicLines::update()
    {
        if( m_dynamicLines )
        {
            m_dynamicLines->update();
        }
    }

    void CDynamicLines::setRenderQueueGroup( u8 renderQueue )
    {
        // m_dynamicLines->setRenderQueueGroup(renderQueue);
    }

    auto CDynamicLines::clone( const String &name ) const -> SmartPtr<IGraphicsObject>
    {
        return nullptr;
    }

    void CDynamicLines::_getObject( void **ppObject ) const
    {
        *ppObject = m_dynamicLines;
    }

    void CDynamicLines::addPoint( const Vector3F &point )
    {
        m_dynamicLines->addPoint( point.X(), point.Y(), point.Z() );
    }

    void CDynamicLines::setPoint( u32 index, const Vector3F &point )
    {
        Ogre::Vector3 ogrePoint;  // (point);
        m_dynamicLines->setPoint( index, ogrePoint );
    }

    auto CDynamicLines::getPoint( u32 index ) const -> Vector3F
    {
        Ogre::Vector3 point = m_dynamicLines->getPoint( index );
        return Vector3F( point.ptr() );
    }

    auto CDynamicLines::getNumPoints() const -> u32
    {
        return m_dynamicLines->getNumPoints();
    }

    void CDynamicLines::clear()
    {
        m_dynamicLines->clear();
    }

    void CDynamicLines::setDirty()
    {
        m_dynamicLines->update();
    }

    void CDynamicLines::setOperationType( u32 opType )
    {
        // m_dynamicLines->setOperationType((Ogre::RenderOperation::OperationType)opType);
    }

    auto CDynamicLines::getOperationType() const -> u32
    {
        return 0;  //(u32)m_dynamicLines->getOperationType();
    }

    auto CDynamicLines::getMemoryManager() const -> Ogre::ObjectMemoryManager *
    {
        return m_memoryManager;
    }

    void CDynamicLines::setMemoryManager( Ogre::ObjectMemoryManager *memoryManager )
    {
        m_memoryManager = memoryManager;
    }

}  // namespace workphone::render
