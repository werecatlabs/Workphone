#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Core/Quad.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <OgreRectangle2D2.h>
#include <Ogre.h>

namespace workphone::render
{

    Quad::Quad() = default;

    Quad::~Quad() = default;

    void Quad::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        using namespace Ogre;

        auto smgr = getSceneManager();

        Ogre::SceneManager *sceneManager = nullptr;
        smgr->_getObject( reinterpret_cast<void **>( &sceneManager ) );

        auto sky =
            OGRE_NEW Rectangle2D( Id::generateNewId<MovableObject>(),
                                  &sceneManager->_getEntityMemoryManager( SCENE_STATIC ), sceneManager );
        // We can't use BT_DYNAMIC_* because the scene may be rendered from multiple cameras
        // in the same frame, and dynamic supports only one set of values per frame
        sky->initialize( BT_DEFAULT, Rectangle2D::GeometryFlagQuad | Rectangle2D::GeometryFlagNormals );
        sky->setGeometry( -Ogre::Vector2::UNIT_SCALE, Ogre::Vector2( 2.0f ) );
        sky->setRenderQueueGroup( 212u );  // Render after most stuff
        sceneManager->getRootSceneNode( SCENE_STATIC )->attachObject( sky );

        setLoadingState( LoadingState::Loaded );
    }

    void Quad::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Quad::getRectangle() const -> Ogre::Rectangle2D *
    {
        return m_rectangle;
    }

    void Quad::setRectangle( Ogre::Rectangle2D *rectangle )
    {
        m_rectangle = rectangle;
    }

    auto Quad::getSceneNode() const -> Ogre::SceneNode *
    {
        return m_sceneNode;
    }

    void Quad::setSceneNode( Ogre::SceneNode *sceneNode )
    {
        m_sceneNode = sceneNode;
    }

    auto Quad::getSceneManager() const -> SmartPtr<workphone::render::IGraphicsScene>
    {
        return m_sceneManager;
    }

    void Quad::setSceneManager( SmartPtr<IGraphicsScene> sceneManager )
    {
        m_sceneManager = sceneManager;
    }

}  // namespace workphone::render
