#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawSceneNode.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawLight.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_scenenode.h"
#include "workphone_graphics_light.h"

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawSceneNode, GraphicsSceneNode );

        namespace
        {
            void applyNativeTransform( wp_scenenode *node, const Transform3<real_Num> &transform )
            {
                const auto &position = transform.getPosition();
                wp_vec3f pos = { position.x, position.y, position.z };

                const auto &orientation = transform.getOrientation();
                wp_quatf ori = { static_cast<wp_f32>( orientation.w ),
                                 static_cast<wp_f32>( orientation.x ),
                                 static_cast<wp_f32>( orientation.y ),
                                 static_cast<wp_f32>( orientation.z ) };

                const auto &scale = transform.getScale();
                wp_vec3f s = { scale.x, scale.y, scale.z };

                wp_scenenode_set_position( node, pos );
                wp_scenenode_set_orientation( node, ori );
                wp_scenenode_set_scale( node, s );
            }
        }  // namespace

        ClawSceneNode::ClawSceneNode() : GraphicsSceneNode(), m_node( nullptr )
        {
        }

        ClawSceneNode::ClawSceneNode( SmartPtr<IGraphicsScene> creator ) : GraphicsSceneNode()
        {
            m_creator = creator;
            m_node = nullptr;

            setupStateContext();
        }

        ClawSceneNode::~ClawSceneNode()
        {
            // A queued load can create a standalone node after its scene unloads.
            // The base destructor cannot dispatch to this native cleanup.
            unload( nullptr );
        }

        void ClawSceneNode::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loading );

            if( !m_node )
            {
                auto creator = getCreator();
                if( creator && creator->isDerived<ClawScene>() )
                {
                    auto scene = workphone::dynamic_pointer_cast<ClawScene>( creator );
                    auto nativeScene = scene->getNativeScene();
                    m_node = wp_graphics_scene_create_node( nativeScene );
                    m_sceneOwnsNode = m_node != nullptr;
                }

                if( !m_node )
                {
                    m_node = wp_scenenode_create();
                    m_sceneOwnsNode = false;
                }
            }

            GraphicsSceneNode::load( data );
            // Transform updates may arrive before the render task creates m_node.
            applyNativeTransform( m_node, getTransform() );

            // Scene loading builds the C++ hierarchy and attachments while native
            // node creation is queued. Restore those links once this node exists.
            setParent( getParent() );
            for( auto &child : getChildren() )
            {
                child->setParent( this );
            }
            for( auto &object : getObjects() )
            {
                attachObject( object );
            }
            setLoadingState( LoadingState::Loaded );
        }

        void ClawSceneNode::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );

            for( auto &object : getObjects() )
            {
                detachObject( object );
            }

            if( m_node )
            {
                if( m_sceneOwnsNode )
                {
                    if( auto scene = dynamic_pointer_cast<ClawScene>( getCreator() ) )
                    {
                        if( scene->getNativeScene() )
                        {
                            wp_graphics_scene_destroy_node( scene->getNativeScene(), m_node );
                        }
                    }
                }
                else
                {
                    wp_scenenode_destroy( m_node );
                }
                m_node = nullptr;
                m_sceneOwnsNode = false;
            }

            GraphicsSceneNode::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        wp_scenenode *ClawSceneNode::getNativeNode() const
        {
            return m_node;
        }

        void ClawSceneNode::setTransform( const Transform3<real_Num> &transform )
        {
            // Keep state authoritative so a later dirty-state update cannot restore
            // an old transform, and retain updates made before native-node creation.
            if( m_node && isThreadSafe() )
            {
                if( getTransform() != transform )
                {
                    if( auto stateContext = getStateContext() )
                    {
                        if( auto stateData = stateContext->invalidateStateDataById<TransformStateData>(
                                getId(), false ) )
                        {
                            stateData->localTransform = transform;
                        }
                    }

                    applyNativeTransform( m_node, transform );
                }
            }
            else
            {
                if( getTransform() != transform )
                {
                    GraphicsSceneNode::setTransform( transform );
                }
            }
        }

        void ClawSceneNode::setWorldTransform( const Transform3<real_Num> &worldTransform )
        {
            setTransform( worldTransform );
        }

        void ClawSceneNode::setParent( SmartPtr<IGraphicsSceneNode> parent )
        {
            GraphicsSceneNode::setParent( parent );

            wp_scenenode *parentNative = nullptr;
            if( parent )
            {
                parent->_getObject( (void **)&parentNative );
            }

            wp_scenenode_set_parent( m_node, parentNative );
        }

        void ClawSceneNode::attachObject( SmartPtr<IGraphicsObject> object )
        {
            if( !object )
            {
                return;
            }

            auto owner = object->getOwner();
            if( owner != this )
            {
                if( owner )
                {
                    owner->detachObject( object );
                }
                GraphicsSceneNode::attachObject( object );
                object->setOwner( this );
            }

            // Cameras and lights expose their own native types, not wp_graphics_object.
            // Also retry native attachment when the C++ owner was assigned before load.
            if( auto camera = dynamic_pointer_cast<ClawCamera>( object ) )
            {
                camera->attachToParent( this );
            }
            else if( auto light = dynamic_pointer_cast<ClawLight>( object ) )
            {
                wp_light *nativeLight = nullptr;
                light->_getObject( (void **)&nativeLight );
                wp_light_attach_to_node( nativeLight, m_node );
            }
            else if( auto mesh = dynamic_pointer_cast<ClawMesh>( object ) )
            {
                auto nativeObject = mesh->getNativeRenderObject();

                if( m_node && nativeObject )
                {
                    wp_scenenode_attach_object( m_node, nativeObject );
                }
            }
            else
            {
                object->attachToParent( this );
            }
        }

        void ClawSceneNode::detachObject( SmartPtr<IGraphicsObject> object )
        {
            if( !object )
            {
                return;
            }

            auto owner = object->getOwner();
            if( owner != this )
            {
                return;
            }

            if( auto camera = dynamic_pointer_cast<ClawCamera>( object ) )
            {
                camera->detachFromParent( this );
            }
            else if( auto light = dynamic_pointer_cast<ClawLight>( object ) )
            {
                wp_light *nativeLight = nullptr;
                light->_getObject( (void **)&nativeLight );
                wp_light_detach_from_node( nativeLight, m_node );
            }
            else if( auto mesh = dynamic_pointer_cast<ClawMesh>( object ) )
            {
                auto nativeObject = mesh->getNativeRenderObject();

                if( m_node && nativeObject )
                {
                    wp_scenenode_detach_object( m_node, nativeObject );
                }
            }
            else
            {
                object->detachFromParent( this );
            }

            GraphicsSceneNode::detachObject( object );

            WP_ASSERT( object->getOwnerPtr() == this );
            object->setOwner( nullptr );
        }

        void ClawSceneNode::_getObject( void **ppObject ) const
        {
            *ppObject = m_node;
        }

        bool ClawSceneNode::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawSceneNode::handleStateChanged( SmartPtr<IState> &state )
        {
            if( state && state->getOwnerPtr() == this )
            {
                if( isLoaded() )
                {
                    auto result = GraphicsSceneNode::handleStateChanged( state );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                    WP_ASSERT( graphicsSystem );

                    if( auto stateData = state->getData() )
                    {
                        if( stateData->isDerived<TransformStateData>() )
                        {
                            auto transformState = SafeReadPtr<TransformStateData>( stateData );

                            const auto &transform = transformState->localTransform;
                            applyNativeTransform( m_node, transform );

                            result = true;
                        }

                        for( auto &graphicsObject : m_graphicsObjects )
                        {
                            if( graphicsObject->isDerived<ClawLight>() )
                            {
                                auto light = workphone::static_pointer_cast<ClawLight>( graphicsObject );
                                auto stateObject = light->getStateContextPtr();
                                if( stateObject )
                                {
                                    stateObject->setDirty( true );
                                }
                            }
                            else if( graphicsObject->isDerived<ClawCamera>() )
                            {
                                auto camera =
                                    workphone::static_pointer_cast<ClawCamera>( graphicsObject );
                                auto stateObject = camera->getStateContextPtr();
                                if( stateObject )
                                {
                                    stateObject->setDirty( true );
                                }
                            }
                        }

                        return result;
                    }
                }
            }

            return false;
        }

        void ClawSceneNode::setupStateContext()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManagerPtr();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto scene = getCreatorPtr();

            auto stateContext = scene->getSceneNodeContextPtr();
            WP_ASSERT( stateContext );
            setStateContext( stateContext );

            auto transformState = factoryManager->make_ptr<State>();
            transformState->setId( getId() );
            transformState->setOwner( this );
            stateContext->addState( transformState );

            auto transformData = factoryManager->make_ptr<TransformStateData>();
            transformState->setData( transformData );

            auto sceneNodeState = factoryManager->make_ptr<State>();
            sceneNodeState->setId( getId() );
            sceneNodeState->setOwner( this );
            stateContext->addState( sceneNodeState );

            auto sceneNodeData = factoryManager->make_ptr<SceneNodeStateData>();
            sceneNodeState->setData( sceneNodeData );

            auto boundingBoxState = factoryManager->make_ptr<State>();
            boundingBoxState->setId( getId() );
            boundingBoxState->setOwner( this );
            stateContext->addState( boundingBoxState );

            auto boundingBoxData = factoryManager->make_ptr<BoundingBoxStateData>();
            boundingBoxState->setData( boundingBoxData );
        }

    }  // namespace render
}  // namespace workphone
