#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDecalCursor.hpp>
#include <WPGraphicsOgreNext/DecalCursor.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone::render
{

    CDecalCursor::CDecalCursor()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        //m_stateContext = stateManager->addStateContext();

        m_stateListener = SmartPtr<IStateListener>( new CDecalCursorStateListener( this ) );
        //m_stateContext->addStateListener( m_stateListener );
    }

    CDecalCursor::~CDecalCursor() = default;

    void CDecalCursor::initialise( SmartPtr<render::IGraphicsScene> sceneMgr,
                                   const String &terrainMaterial, const String &decalTextureName,
                                   const Vector2F &size )
    {
        Ogre::SceneManager *smgr = nullptr;
        sceneMgr->_getObject( reinterpret_cast<void **>( &smgr ) );

        m_sceneMgr = sceneMgr.get();

        Ogre::MaterialManager *materialMgr = Ogre::MaterialManager::getSingletonPtr();
        Ogre::MaterialPtr terrainMat = materialMgr->getByName( terrainMaterial.c_str() );

        //m_decalCursor = new DecalCursor( smgr, terrainMat, Ogre::Vector2( size.X(), size.Y() ),
        //                                 decalTextureName.c_str() );
    }

    auto CDecalCursor::isVisible() const -> bool
    {
        return m_isVisible;
    }

    void CDecalCursor::setVisible( bool visible )
    {
        m_isVisible = visible;
    }

    auto CDecalCursor::getPosition() const -> Vector3F
    {
        return m_position;
    }

    void CDecalCursor::setPosition( const Vector3F &position )
    {
        if( Thread::getCurrentTask() != TaskId::Render )
        {
            //SmartPtr<StateMessageVector3> message( new StateMessageVector3( position ) );
            //m_stateContext->addMessage( TaskId::Render, message );
        }
        else
        {
            //m_decalCursor->setPosition( Ogre::Vector3( position.X(), position.Y(), position.Z() ) );
        }

        if( m_node )
        {
            m_node->setPosition( position );
        }
    }

    auto CDecalCursor::getSize() const -> Vector2F
    {
        return m_size;
    }

    void CDecalCursor::setSize( const Vector2F &size )
    {
        m_size = size;
    }

    void CDecalCursor::addDebugEntity( const String &entityName, const Vector3F &scale )
    {
        m_entity = m_sceneMgr->addGraphicsObjectByType<render::IGraphicsMesh>();
        m_entity->setName( entityName );

        m_node = m_sceneMgr->getRootSceneNode()->addChildSceneNode();
        m_node->attachObject( m_entity );
        m_node->setScale( scale );
        // m_node->getStateContext()->add();
    }

    void CDecalCursor::removeDebugEntity()
    {
    }

    auto CDecalCursor::getTextureName() const -> String
    {
        return m_textureName;
    }

    void CDecalCursor::setTextureName( const String &textureName )
    {
        m_textureName = textureName;
    }

    bool CDecalCursor::CDecalCursorStateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        // if(message->isExactly(StateMessageVector3::TYPE_INFO))
        //{
        //	SmartPtr<StateMessageVector3> positionMessage = message;

        //	Vector3F position = positionMessage->getValue();
        //	m_owner->setPosition(position);
        //}

        return false;
    }

    bool CDecalCursor::CDecalCursorStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    CDecalCursor::CDecalCursorStateListener::~CDecalCursorStateListener() = default;

    CDecalCursor::CDecalCursorStateListener::CDecalCursorStateListener( CDecalCursor *owner ) :
        m_owner( owner )
    {
    }

}  // namespace workphone::render
