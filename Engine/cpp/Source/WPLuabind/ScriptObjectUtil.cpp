#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/ScriptObjectUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>
#include <luabind/detail/convert_to_lua.hpp>

namespace workphone
{
    bool ScriptObjectUtil::toLua( lua_State *L, ISharedObject *scriptObj )
    {
        using namespace luabind;
        using namespace render;
        using namespace physics;
        using namespace procedural;

        WP_ASSERT( scriptObj->isExactly<ISharedObject>() == false );

        if( scriptObj->isDerived<core::IApplicationManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<core::IApplicationManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<core::IApplication>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<core::IApplication>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ui::IUIManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IResourceDatabase>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IResourceDatabase>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IPackageManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IPackageManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFactoryManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IFactoryManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IProceduralGenerator>() )
        {
            if( scriptObj->isDerived<ICityGenerator>() )
            {
                detail::convert_to_lua( L, SmartPtr<ICityGenerator>( scriptObj ) );
                return true;
            }

            detail::convert_to_lua( L, SmartPtr<IProceduralGenerator>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IProceduralManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<IProceduralManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IProceduralScene>() )
        {
            detail::convert_to_lua( L, SmartPtr<IProceduralScene>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IRoad>() )
        {
            detail::convert_to_lua( L, SmartPtr<IRoad>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<INativeFileDialog>() )
        {
            detail::convert_to_lua( L, SmartPtr<INativeFileDialog>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<Properties>() )
        {
            detail::convert_to_lua( L, SmartPtr<Properties>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IResource>() )
        {
            if( scriptObj->isDerived<IMaterial>() )
            {
                detail::convert_to_lua( L, SmartPtr<IMaterial>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ITexture>() )
            {
                detail::convert_to_lua( L, SmartPtr<ITexture>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::IGameActor>() )
            {
                detail::convert_to_lua( L, SmartPtr<scene::IGameActor>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::IGameScene>() )
            {
                detail::convert_to_lua( L, SmartPtr<scene::IGameScene>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ISound>() )
            {
                detail::convert_to_lua( L, SmartPtr<ISound>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IBuildDirector>() )
            {
                detail::convert_to_lua( L, SmartPtr<IBuildDirector>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::ISubComponent>() )
            {
                if( scriptObj->isDerived<scene::TerrainBlendMap>() )
                {
                    detail::convert_to_lua( L, SmartPtr<scene::TerrainBlendMap>( scriptObj ) );
                    return true;
                }
                if( scriptObj->isDerived<scene::TerrainGrassLayer>() )
                {
                    detail::convert_to_lua( L, SmartPtr<scene::TerrainGrassLayer>( scriptObj ) );
                    return true;
                }
                if( scriptObj->isDerived<scene::TerrainLayer>() )
                {
                    detail::convert_to_lua( L, SmartPtr<scene::TerrainLayer>( scriptObj ) );
                    return true;
                }
                if( scriptObj->isDerived<scene::TerrainTreeLayer>() )
                {
                    detail::convert_to_lua( L, SmartPtr<scene::TerrainTreeLayer>( scriptObj ) );
                    return true;
                }

                detail::convert_to_lua( L, SmartPtr<scene::ISubComponent>( scriptObj ) );
                return true;
            }

            detail::convert_to_lua( L, SmartPtr<IResource>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<scene::IGameEditor>() )
        {
            detail::convert_to_lua( L, SmartPtr<scene::IGameEditor>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ui::IUIApplication>() )
        {
            detail::convert_to_lua( L, SmartPtr<ui::IUIApplication>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ui::IUIElement>() )
        {
            if( scriptObj->isDerived<ui::IUIButton>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIButton>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIColourPicker>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIColourPicker>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUICollapsingHeader>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUICollapsingHeader>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUICheckbox>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUICheckbox>( scriptObj ) );
                return true;
            }

            if( scriptObj->isDerived<ui::IUIDropdown>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIDropdown>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIImage>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIImage>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelDropdownPair>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUILabelDropdownPair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelTogglePair>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUILabelTogglePair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelSliderPair>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUILabelSliderPair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelTextInputPair>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUILabelTextInputPair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIMenu>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIMenu>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIMenuItem>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIMenuItem>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIMenubar>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIMenubar>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIPropertyGrid>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIPropertyGrid>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITabBar>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUITabBar>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITabItem>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUITabItem>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIText>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIText>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITextEntry>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUITextEntry>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITreeCtrl>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUITreeCtrl>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIWindow>() )
            {
                detail::convert_to_lua( L, SmartPtr<ui::IUIWindow>( scriptObj ) );
                return true;
            }

            detail::convert_to_lua( L, SmartPtr<ui::IUIElement>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsShape2>() )
        {
            if( scriptObj->isDerived<IBoxShape2>() )
            {
                detail::convert_to_lua( L, SmartPtr<IBoxShape2>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ISphereShape2>() )
            {
                detail::convert_to_lua( L, SmartPtr<ISphereShape2>( scriptObj ) );
                return true;
            }
            detail::convert_to_lua( L, SmartPtr<IPhysicsShape2>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsShape3>() )
        {
            if( scriptObj->isDerived<IBoxShape3>() )
            {
                detail::convert_to_lua( L, SmartPtr<IBoxShape3>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ISphereShape3>() )
            {
                detail::convert_to_lua( L, SmartPtr<ISphereShape3>( scriptObj ) );
                return true;
            }
            detail::convert_to_lua( L, SmartPtr<IPhysicsShape3>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsBody2D>() )
        {
            if( scriptObj->isDerived<IRigidBody2>() )
            {
                detail::convert_to_lua( L, SmartPtr<IRigidBody2>( scriptObj ) );
                return true;
            }
            // else if( scriptObj->isDerived<IStaticBody2>() )
            //{
            //     luabind::detail::convert_to_lua( L, SmartPtr<IStaticBody2>( scriptObj ) );
            //     return true;
            // }
            detail::convert_to_lua( L, SmartPtr<IPhysicsBody2D>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsBody3>() )
        {
            if( scriptObj->isDerived<IRigidBody3>() )
            {
                detail::convert_to_lua( L, SmartPtr<IRigidBody3>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IRigidStatic3>() )
            {
                detail::convert_to_lua( L, SmartPtr<IRigidStatic3>( scriptObj ) );
                return true;
            }
            detail::convert_to_lua( L, SmartPtr<IPhysicsBody3>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<scene::IComponent>() )
        {
            if( scriptObj->isDerived<scene::Script>() )
            {
                WP_ASSERT( dynamic_cast<scene::Script *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Script>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Camera>() )
            {
                WP_ASSERT( dynamic_cast<scene::Camera *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Camera>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Mesh>() )
            {
                WP_ASSERT( dynamic_cast<scene::Mesh *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Mesh>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::ParticleSystem>() )
            {
                WP_ASSERT( dynamic_cast<scene::ParticleSystem *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::ParticleSystem>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Renderer>() )
            {
                WP_ASSERT( dynamic_cast<scene::Renderer *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Renderer>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::UIComponent>() )
            {
                WP_ASSERT( dynamic_cast<scene::UIComponent *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::UIComponent>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Light>() )
            {
                WP_ASSERT( dynamic_cast<scene::Light *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Light>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::AudioEmitter>() )
            {
                WP_ASSERT( dynamic_cast<scene::AudioEmitter *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::AudioEmitter>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Animation>() )
            {
                WP_ASSERT( dynamic_cast<scene::Animation *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Animation>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Animator>() )
            {
                WP_ASSERT( dynamic_cast<scene::Animator *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Animator>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Billboard>() )
            {
                WP_ASSERT( dynamic_cast<scene::Billboard *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Billboard>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Billboards>() )
            {
                WP_ASSERT( dynamic_cast<scene::Billboards *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Billboards>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Cubemap>() )
            {
                WP_ASSERT( dynamic_cast<scene::Cubemap *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Cubemap>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Collision>() )
            {
                WP_ASSERT( dynamic_cast<scene::Collision *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Collision>( scriptObj ) );
            }
            else if( scriptObj->isDerived<scene::CollisionBox>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionBox *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::CollisionBox>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionMesh>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionMesh *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::CollisionMesh>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionPlane>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionPlane *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::CollisionPlane>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionSphere>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionSphere *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::CollisionSphere>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionTerrain>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionTerrain *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::CollisionTerrain>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::TerrainSystem>() )
            {
                WP_ASSERT( dynamic_cast<scene::TerrainSystem *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::TerrainSystem>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CarController>() )
            {
                WP_ASSERT( dynamic_cast<scene::CarController *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::CarController>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::VehicleController>() )
            {
                WP_ASSERT( dynamic_cast<scene::VehicleController *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::VehicleController>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::VideoPlayer>() )
            {
                WP_ASSERT( dynamic_cast<scene::VideoPlayer *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::VideoPlayer>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::WheelController>() )
            {
                WP_ASSERT( dynamic_cast<scene::WheelController *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::WheelController>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Material>() )
            {
                WP_ASSERT( dynamic_cast<scene::Material *>( scriptObj ) );
                detail::convert_to_lua( L, SmartPtr<scene::Material>( scriptObj ) );
                return true;
            }
            detail::convert_to_lua( L, SmartPtr<scene::IComponent>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<editor::FileSelection>() )
        {
            detail::convert_to_lua( L, SmartPtr<editor::FileSelection>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsObject>() )
        {
            if( scriptObj->isDerived<IGraphicsCamera>() )
            {
                detail::convert_to_lua( L, SmartPtr<IGraphicsCamera>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IFrustum>() )
            {
                detail::convert_to_lua( L, SmartPtr<IFrustum>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IGraphicsMesh>() )
            {
                detail::convert_to_lua( L, SmartPtr<IGraphicsMesh>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IGraphicsLight>() )
            {
                detail::convert_to_lua( L, SmartPtr<IGraphicsLight>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IInstancedObject>() )
            {
                detail::convert_to_lua( L, SmartPtr<IInstancedObject>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IParticleSystem>() )
            {
                detail::convert_to_lua( L, SmartPtr<IParticleSystem>( scriptObj ) );
                return true;
            }
            detail::convert_to_lua( L, SmartPtr<IGraphicsObject>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IRenderTarget>() )
        {
            if( scriptObj->isDerived<IGraphicsWindow>() )
            {
                detail::convert_to_lua( L, SmartPtr<IGraphicsWindow>( scriptObj ) );
                return true;
            }
            detail::convert_to_lua( L, SmartPtr<IRenderTarget>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsScene>() )
        {
            detail::convert_to_lua( L, SmartPtr<IGraphicsScene>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsSceneNode>() )
        {
            detail::convert_to_lua( L, SmartPtr<IGraphicsSceneNode>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IAnimationStateController>() )
        {
            detail::convert_to_lua( L, SmartPtr<IAnimationStateController>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IAnimationController>() )
        {
            detail::convert_to_lua( L, SmartPtr<IAnimationController>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IParticleTechnique>() )
        {
            detail::convert_to_lua( L, SmartPtr<IParticleTechnique>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ITextureManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<ITextureManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsTerrain>() )
        {
            detail::convert_to_lua( L, SmartPtr<IGraphicsTerrain>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsWater>() )
        {
            detail::convert_to_lua( L, SmartPtr<IGraphicsWater>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsDeferredShading>() )
        {
            detail::convert_to_lua( L, SmartPtr<IGraphicsDeferredShading>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IViewport>() )
        {
            detail::convert_to_lua( L, SmartPtr<IViewport>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsSystem>() )
        {
            detail::convert_to_lua( L, SmartPtr<IGraphicsSystem>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<scene::IGameManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<scene::IGameManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFSMManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<IFSMManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFSM>() )
        {
            detail::convert_to_lua( L, SmartPtr<IFSM>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFSMListener>() )
        {
            detail::convert_to_lua( L, SmartPtr<IFSMListener>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ISoundManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<ISoundManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IVideoManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<IVideoManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IVideo>() )
        {
            detail::convert_to_lua( L, SmartPtr<IVideo>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IDatabaseQuery>() )
        {
            detail::convert_to_lua( L, SmartPtr<IDatabaseQuery>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IDatabase>() )
        {
            detail::convert_to_lua( L, SmartPtr<IDatabase>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IPhysicsManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<IPhysicsManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IInputDeviceManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<IInputDeviceManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IAnimator>() )
        {
            detail::convert_to_lua( L, SmartPtr<IAnimator>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IMesh>() )
        {
            detail::convert_to_lua( L, SmartPtr<IMesh>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ISubMesh>() )
        {
            detail::convert_to_lua( L, SmartPtr<ISubMesh>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFileSystem>() )
        {
            detail::convert_to_lua( L, SmartPtr<IFileSystem>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IStateContext>() )
        {
            detail::convert_to_lua( L, SmartPtr<IStateContext>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IStateManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<IStateManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ITimer>() )
        {
            detail::convert_to_lua( L, SmartPtr<ITimer>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IStateMessage>() )
        {
            detail::convert_to_lua( L, SmartPtr<IStateMessage>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IEvent>() )
        {
            detail::convert_to_lua( L, SmartPtr<IEvent>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ISelectionManager>() )
        {
            detail::convert_to_lua( L, SmartPtr<ISelectionManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IEventListener>() )
        {
            detail::convert_to_lua( L, SmartPtr<IEventListener>( scriptObj ) );
            return true;
        }

        return false;
    }

    template <>
    void CastUtil<physics::IPhysicsShape2>::down_cast( lua_State                               *L,
                                                       const SmartPtr<physics::IPhysicsShape2> &ptr )
    {
        using namespace luabind;

        if( ptr->isDerived<physics::IBoxShape2>() )
        {
            SmartPtr<physics::IBoxShape2> p = ptr;
            detail::value_converter().apply( L, p );
        }
        else if( ptr->isDerived<physics::ISphereShape2>() )
        {
            SmartPtr<physics::ISphereShape2> p = ptr;
            detail::value_converter().apply( L, p );
        }
        else
        {
            detail::value_converter().apply( L, ptr );
        }
    }

    template class CastUtil<physics::IPhysicsShape2>;
} // namespace workphone
