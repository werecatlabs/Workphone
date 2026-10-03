#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/State/States/BoundingBoxStateData.hpp>
#include <Workphone/State/States/SceneNodeStateData.hpp>
#include <Workphone/State/States/TransformStateData.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsSceneNode,
                               SharedGraphicsObject<IGraphicsSceneNode> );

    static const String PROPERTY_LOOK_AT = "lookAt";
    static const String PROPERTY_FLAGS = "flags";
    static const String PROPERTY_POSITION = "position";
    static const String PROPERTY_ORIENTATION = "orientation";
    static const String PROPERTY_SCALE = "scale";

    u32 GraphicsSceneNode::m_idExt = 0;

    GraphicsSceneNode::GraphicsSceneNode()
    {
        setEventTaskFlags( Thread::Render_Flag );

        auto name = String( "SceneNode" ) + StringUtil::toString( m_idExt++ );
        auto id = StringUtil::getHash( name );

        setName( name );
        setId( id );
    }

    GraphicsSceneNode::GraphicsSceneNode( u32 poolTypeId )
    {
        setEventTaskFlags( Thread::Render_Flag );

        auto name = String( "SceneNode" ) + StringUtil::toString( m_idExt++ );
        auto id = StringUtil::getHash( name );

        setName( name );
        setId( id );
    }

    GraphicsSceneNode::~GraphicsSceneNode()
    {
        unload( nullptr );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( applicationManager )
        {
            if( auto stateManager = applicationManager->getStateManagerPtr() )
            {
                if( auto stateContext = getStateContextPtr() )
                {
                    auto states = stateContext->getStates();
                    for( auto &state : states )
                    {
                        if( state && state->getOwnerPtr() == this )
                        {
                            state->unload( nullptr );
                            stateContext->removeState( state );
                        }
                    }

                    setStateContext( nullptr );
                }
            }
        }
    }

    void GraphicsSceneNode::load( SmartPtr<ISharedObject> data )
    {
        SharedGraphicsObject<IGraphicsSceneNode>::load( data );

        if( auto stateContext = getStateContext() )
        {
            m_boundingBoxStateData = (BoundingBoxStateData *)stateContext->getStateDataPtrById(
                getId(), BoundingBoxStateData::typeInfo() );
            m_sceneNodeStateData = (SceneNodeStateData *)stateContext->getStateDataPtrById(
                getId(), SceneNodeStateData::typeInfo() );
            m_transformStateData = (TransformStateData *)stateContext->getStateDataPtrById(
                getId(), TransformStateData::typeInfo() );
        }
    }

    void GraphicsSceneNode::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            m_creator = nullptr;
            m_parent = nullptr;
            m_children.clear();
            m_graphicsObjects.clear();

            SharedGraphicsObject<IGraphicsSceneNode>::unload( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GraphicsSceneNode::detachObjectPtr( IGraphicsObject *object )
    {
        auto pObject = SmartPtr<IGraphicsObject>( object );
        m_graphicsObjects.erase(
            std::remove( m_graphicsObjects.begin(), m_graphicsObjects.end(), pObject ),
            m_graphicsObjects.end() );
    }

    void GraphicsSceneNode::setParent( SmartPtr<IGraphicsSceneNode> parent )
    {
        m_parent = parent;
    }

    auto GraphicsSceneNode::getParent() const -> SmartPtr<IGraphicsSceneNode>
    {
        auto p = m_parent.load();
        return p.lock();
    }

    auto GraphicsSceneNode::getCreator() const -> SmartPtr<IGraphicsScene>
    {
        auto p = m_creator.load();
        return p.lock();
    }

    void GraphicsSceneNode::setCreator( SmartPtr<IGraphicsScene> creator )
    {
        m_creator = creator;
    }

    Transform3<real_Num> GraphicsSceneNode::getTransform() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TransformStateData>( getId() ) )
            {
                return stateData->localTransform;
            }
        }

        return {};
    }

    void GraphicsSceneNode::setTransform( const Transform3<real_Num> &t )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<TransformStateData>( getId() ) )
            {
                stateData->localTransform = t;
            }
        }
    }

    Transform3<real_Num> GraphicsSceneNode::getWorldTransform() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TransformStateData>( getId() ) )
            {
                return stateData->worldTransform;
            }
        }

        return {};
    }

    void GraphicsSceneNode::setWorldTransform( const Transform3<real_Num> &t )
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), true );
                m_transformStateData->worldTransform = t;
            }
        }
    }

    void GraphicsSceneNode::setPosition( const Vector3<real_Num> &position )
    {
        WP_ASSERT( position.isFinite() );

        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), true );
                auto &t = m_transformStateData->localTransform;
                t.setPosition( position );
            }
        }
    }

    auto GraphicsSceneNode::getPosition() const -> Vector3<real_Num>
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), false );
                auto &t = m_transformStateData->localTransform;
                return t.getPosition();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto GraphicsSceneNode::getWorldPosition() const -> Vector3<real_Num>
    {
        if( auto parent = getParent() )
        {
            return parent->getWorldPosition() + parent->getTransform().transformVector( getPosition() );
        }

        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), false );
                auto &t = m_transformStateData->worldTransform;
                return t.getPosition();
            }
        }

        return Vector3<real_Num>::zero();
    }

    void GraphicsSceneNode::setRotationFromDegrees( const Vector3<real_Num> &degrees )
    {
        auto orientation = Quaternion<real_Num>::eulerDegrees( degrees.x, degrees.y, degrees.z );
        setOrientation( orientation );
    }

    void GraphicsSceneNode::setOrientation( const Quaternion<real_Num> &orientation )
    {
        WP_ASSERT( orientation.isSane() );

        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), true );
                auto &t = m_transformStateData->localTransform;
                t.setOrientation( orientation );
            }
        }
    }

    auto GraphicsSceneNode::getOrientation() const -> Quaternion<real_Num>
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), false );
                auto &t = m_transformStateData->localTransform;
                return t.getOrientation();
            }
        }

        return Quaternion<real_Num>::identity();
    }

    auto GraphicsSceneNode::getWorldOrientation() const -> Quaternion<real_Num>
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), false );
                auto &t = m_transformStateData->worldTransform;
                return t.getOrientation();
            }
        }

        return Quaternion<real_Num>::identity();
    }

    void GraphicsSceneNode::setScale( const Vector3<real_Num> &scale )
    {
        WP_ASSERT( scale.isFinite() );

        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), true );
                auto &t = m_transformStateData->localTransform;
                t.setScale( scale );
            }
        }
    }

    auto GraphicsSceneNode::getScale() const -> Vector3<real_Num>
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_transformStateData )
            {
                ScopedLock lock( m_transformStateData.get(), false );
                auto &t = m_transformStateData->localTransform;
                return t.getScale();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto GraphicsSceneNode::getWorldScale() const -> Vector3<real_Num>
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->getStateDataById<TransformStateData>( getId() ) )
            {
                auto &t = stateData->worldTransform;
                return t.getScale();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto GraphicsSceneNode::_getRenderSystemTransform() const -> void *
    {
        return nullptr;
    }

    void GraphicsSceneNode::lookAt( const Vector3<real_Num> &targetPoint )
    {
        auto direction = targetPoint - getPosition();
        auto orientation = MathUtil<real_Num>::getOrientationFromDirection(
            direction, -Vector3<real_Num>::unitZ(), true, Vector3<real_Num>::unitY() );
        setOrientation( orientation );
    }

    void GraphicsSceneNode::setFixedYawAxis(
        bool useFixed, const Vector3<real_Num> &fixedAxis /*= Vector3<real_Num>::UNIT_Y */ )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto stateData = stateContext->invalidateStateDataById<SceneNodeStateData>( getId() ) )
            {
                stateData->yawFixed = useFixed;
                stateData->yawFixedAxis = fixedAxis;
            }
        }
    }

    auto GraphicsSceneNode::getLocalAABB() const -> AABB3<real_Num>
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_boundingBoxStateData )
            {
                ScopedLock lock( m_boundingBoxStateData.get(), false );
                return m_boundingBoxStateData->localAABB;
            }
        }

        return {};
    }

    void GraphicsSceneNode::setLocalAABB( const AABB3<real_Num> &aabb )
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_boundingBoxStateData )
            {
                ScopedLock lock( m_boundingBoxStateData.get(), true );
                m_boundingBoxStateData->localAABB = aabb;
            }
        }
    }

    auto GraphicsSceneNode::getWorldAABB() const -> AABB3<real_Num>
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_boundingBoxStateData )
            {
                ScopedLock lock( m_boundingBoxStateData.get(), false );
                return m_boundingBoxStateData->worldAABB;
            }
        }

        return {};
    }

    void GraphicsSceneNode::setWorldAABB( const AABB3<real_Num> &aabb )
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_boundingBoxStateData )
            {
                ScopedLock lock( m_boundingBoxStateData.get(), true );
                m_boundingBoxStateData->worldAABB = aabb;
            }
        }
    }

    bool GraphicsSceneNode::isStatic() const
    {
        return m_isStatic.load();
    }

    void GraphicsSceneNode::setStatic( bool isstatic )
    {
        m_isStatic = isstatic;
    }

    void GraphicsSceneNode::attachObject( SmartPtr<IGraphicsObject> object )
    {
        m_graphicsObjects.push_back( object );
    }

    void GraphicsSceneNode::detachObject( SmartPtr<IGraphicsObject> object )
    {
        m_graphicsObjects.erase(
            std::remove( m_graphicsObjects.begin(), m_graphicsObjects.end(), object ),
            m_graphicsObjects.end() );
    }

    void GraphicsSceneNode::detachAllObjects()
    {
        m_graphicsObjects.clear();
    }

    auto GraphicsSceneNode::getObjects() const -> Array<SmartPtr<IGraphicsObject>>
    {
        auto graphicsObjects = m_graphicsObjects.snapshot();
        return { graphicsObjects.begin(), graphicsObjects.end() };
    }

    auto GraphicsSceneNode::getNumObjects() const -> u32
    {
        return (u32)m_graphicsObjects.size();
    }

    auto GraphicsSceneNode::addChildSceneNode( const String &name /*= StringUtil::EmptyString */ )
        -> SmartPtr<IGraphicsSceneNode>
    {
        if( auto creator = getCreator() )
        {
            auto sceneNode = creator->addSceneNode();
            addChild( sceneNode );
            sceneNode->setName( name );
            return sceneNode;
        }

        return nullptr;
    }

    auto GraphicsSceneNode::addChildSceneNode( const Vector3<real_Num> &position )
        -> SmartPtr<IGraphicsSceneNode>
    {
        if( auto creator = getCreator() )
        {
            auto sceneNode = creator->addSceneNode();
            addChild( sceneNode );
            sceneNode->setPosition( position );
            return sceneNode;
        }

        return nullptr;
    }

    void GraphicsSceneNode::addChild( SmartPtr<IGraphicsSceneNode> child )
    {
        if( child )
        {
            if( child->isLoaded() )
            {
                if( auto parent = child->getParent() )
                {
                    parent->removeChild( child );  // remove from old parent
                }
            }

            child->setParent( this );
            m_children.push_back( child );
        }
    }

    auto GraphicsSceneNode::removeChild( SmartPtr<IGraphicsSceneNode> child ) -> bool
    {
        auto it = std::find( m_children.begin(), m_children.end(), child );
        if( it != m_children.end() )
        {
            m_children.erase( it );

            child->setParent( nullptr );
            return true;
        }

        return false;
    }

    void GraphicsSceneNode::removeChildren()
    {
        try
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                if( child )
                {
                    removeChild( child );
                }
            }

            m_children.clear();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto GraphicsSceneNode::findChild( const String &name ) -> SmartPtr<IGraphicsSceneNode>
    {
        auto children = getChildren();

        for( auto &sceneNode : children )
        {
            const auto sceneNodeName = sceneNode->getName();
            if( name == sceneNodeName )
            {
                return sceneNode;
            }
        }

        return nullptr;
    }

    auto GraphicsSceneNode::getChildren() const -> Array<SmartPtr<IGraphicsSceneNode>>
    {
        auto children = m_children.snapshot();
        return { children.begin(), children.end() };
    }

    void GraphicsSceneNode::setChildren( const Array<SmartPtr<IGraphicsSceneNode>> &children )
    {
        m_children = { children.begin(), children.end() };
    }

    auto GraphicsSceneNode::getNumChildren() const -> u32
    {
        return (u32)m_children.size();
    }

    void GraphicsSceneNode::needUpdate( bool forceParentUpdate )
    {
    }

    auto GraphicsSceneNode::clone( SmartPtr<IGraphicsSceneNode> parent, const String &name ) const
        -> SmartPtr<IGraphicsSceneNode>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto newNode = factoryManager->make_object<GraphicsSceneNode>( "SceneNode" );

        auto data = newNode->toData();
        newNode->fromData( data );

        return newNode;
    }

    void GraphicsSceneNode::updateBounds()
    {
    }

    void GraphicsSceneNode::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    auto GraphicsSceneNode::getProperties() const -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto properties = factoryManager->make_ptr<Properties>();

        if( auto stateContext = getStateContext() )
        {
            auto stateData = stateContext->getStateData<SceneNodeStateData>();
            if( stateData )
            {
                properties->setProperty( PROPERTY_LOOK_AT, stateData->lookAt );
                properties->setProperty( PROPERTY_FLAGS, stateData->flags );
                properties->setProperty( PROPERTY_POSITION, getPosition() );
                properties->setProperty( PROPERTY_ORIENTATION, getOrientation() );
                properties->setProperty( PROPERTY_SCALE, getScale() );
            }
        }

        return properties;
    }

    void GraphicsSceneNode::setProperties( SmartPtr<Properties> properties )
    {
        SharedGraphicsObject<IGraphicsSceneNode>::setProperties( properties );
    }

    auto GraphicsSceneNode::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto childObjects = SharedGraphicsObject<IGraphicsSceneNode>::getChildObjects();
        childObjects.reserve( 4 );

        childObjects.emplace_back( getCreator() );
        childObjects.emplace_back( getParent() );
        return childObjects;
    }

    void GraphicsSceneNode::setFlag( u32 flag, bool value )
    {
        auto flags = getFlags();
        if( value )
        {
            flags |= flag;
        }
        else
        {
            flags &= ~flag;
        }

        setFlags( flags );
    }

    bool GraphicsSceneNode::getFlag( u32 flag ) const
    {
        auto flags = getFlags();
        return ( flags & flag ) != 0;
    }

    void GraphicsSceneNode::setFlags( u32 flags )
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_sceneNodeStateData )
            {
                ScopedLock lock( m_sceneNodeStateData.get(), true );
                m_sceneNodeStateData->flags = flags;
            }
        }
    }

    u32 GraphicsSceneNode::getFlags() const
    {
        ScopedLoadLock lock( (ISharedObject *)this );
        if( lock.isLoaded() )
        {
            if( m_sceneNodeStateData )
            {
                ScopedLock lock( m_sceneNodeStateData.get(), false );
                return m_sceneNodeStateData->flags;
            }
        }

        return 0;
    }

    void GraphicsSceneNode::makeDirty()
    {
        if( auto stateContext = getStateContext() )
        {
            stateContext->setDirty( true );
        }
    }

    bool GraphicsSceneNode::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool GraphicsSceneNode::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

}  // namespace workphone::render
