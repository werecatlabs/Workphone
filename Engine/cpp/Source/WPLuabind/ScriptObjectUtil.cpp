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
        if( scriptObj->isDerived<scene::ProceduralRaceScene>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<scene::ProceduralRaceScene>( scriptObj ) );
            return true;
        }

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
                luabind::detail::convert_to_lua( L, SmartPtr<ICityGenerator>( scriptObj ) );
                return true;
            }

            luabind::detail::convert_to_lua( L, SmartPtr<IProceduralGenerator>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IProceduralManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IProceduralManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IProceduralScene>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IProceduralScene>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IRoad>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IRoad>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<INativeFileDialog>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<INativeFileDialog>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<Properties>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<Properties>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IResource>() )
        {
            if( scriptObj->isDerived<IMaterial>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IMaterial>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ITexture>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ITexture>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::IGameActor>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<scene::IGameActor>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::IGameScene>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<scene::IGameScene>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ISound>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ISound>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IBuildDirector>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IBuildDirector>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::ISubComponent>() )
            {
                if( scriptObj->isDerived<scene::TerrainBlendMap>() )
                {
                    luabind::detail::convert_to_lua( L, SmartPtr<scene::TerrainBlendMap>( scriptObj ) );
                    return true;
                }
                if( scriptObj->isDerived<scene::TerrainGrassLayer>() )
                {
                    luabind::detail::convert_to_lua( L, SmartPtr<scene::TerrainGrassLayer>( scriptObj ) );
                    return true;
                }
                if( scriptObj->isDerived<scene::TerrainLayer>() )
                {
                    luabind::detail::convert_to_lua( L, SmartPtr<scene::TerrainLayer>( scriptObj ) );
                    return true;
                }
                if( scriptObj->isDerived<scene::TerrainTreeLayer>() )
                {
                    luabind::detail::convert_to_lua( L, SmartPtr<scene::TerrainTreeLayer>( scriptObj ) );
                    return true;
                }

                luabind::detail::convert_to_lua( L, SmartPtr<scene::ISubComponent>( scriptObj ) );
                return true;
            }

            luabind::detail::convert_to_lua( L, SmartPtr<IResource>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<scene::IGameEditor>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<scene::IGameEditor>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ui::IUIApplication>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIApplication>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ui::IUIElement>() )
        {
            if( scriptObj->isDerived<ui::IUIButton>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIButton>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIColourPicker>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIColourPicker>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUICollapsingHeader>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUICollapsingHeader>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUICheckbox>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUICheckbox>( scriptObj ) );
                return true;
            }

            if( scriptObj->isDerived<ui::IUIDropdown>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIDropdown>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIImage>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIImage>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelDropdownPair>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUILabelDropdownPair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelTogglePair>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUILabelTogglePair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelSliderPair>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUILabelSliderPair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUILabelTextInputPair>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUILabelTextInputPair>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIMenu>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIMenu>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIMenuItem>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIMenuItem>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIMenubar>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIMenubar>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIPropertyGrid>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIPropertyGrid>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITabBar>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUITabBar>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITabItem>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUITabItem>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIText>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIText>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITextEntry>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUITextEntry>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUITreeCtrl>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUITreeCtrl>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ui::IUIWindow>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIWindow>( scriptObj ) );
                return true;
            }

            luabind::detail::convert_to_lua( L, SmartPtr<ui::IUIElement>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsShape2>() )
        {
            if( scriptObj->isDerived<IBoxShape2>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IBoxShape2>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ISphereShape2>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ISphereShape2>( scriptObj ) );
                return true;
            }
            luabind::detail::convert_to_lua( L, SmartPtr<IPhysicsShape2>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsShape3>() )
        {
            if( scriptObj->isDerived<IBoxShape3>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IBoxShape3>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<ISphereShape3>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<ISphereShape3>( scriptObj ) );
                return true;
            }
            luabind::detail::convert_to_lua( L, SmartPtr<IPhysicsShape3>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsBody2D>() )
        {
            if( scriptObj->isDerived<IRigidBody2>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IRigidBody2>( scriptObj ) );
                return true;
            }
            // else if( scriptObj->isDerived<IStaticBody2>() )
            //{
            //     luabind::detail::convert_to_lua( L, SmartPtr<IStaticBody2>( scriptObj ) );
            //     return true;
            // }
            luabind::detail::convert_to_lua( L, SmartPtr<IPhysicsBody2D>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<IPhysicsBody3>() )
        {
            if( scriptObj->isDerived<IRigidBody3>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IRigidBody3>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IRigidStatic3>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IRigidStatic3>( scriptObj ) );
                return true;
            }
            luabind::detail::convert_to_lua( L, SmartPtr<IPhysicsBody3>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<scene::IComponent>() )
        {
            if( scriptObj->isDerived<scene::Script>() )
            {
                WP_ASSERT( dynamic_cast<scene::Script *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Script>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Camera>() )
            {
                WP_ASSERT( dynamic_cast<scene::Camera *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Camera>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Mesh>() )
            {
                WP_ASSERT( dynamic_cast<scene::Mesh *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Mesh>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::ParticleSystem>() )
            {
                WP_ASSERT( dynamic_cast<scene::ParticleSystem *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::ParticleSystem>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Renderer>() )
            {
                WP_ASSERT( dynamic_cast<scene::Renderer *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Renderer>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::UIComponent>() )
            {
                WP_ASSERT( dynamic_cast<scene::UIComponent *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::UIComponent>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Light>() )
            {
                WP_ASSERT( dynamic_cast<scene::Light *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Light>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::AudioEmitter>() )
            {
                WP_ASSERT( dynamic_cast<scene::AudioEmitter *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::AudioEmitter>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Animation>() )
            {
                WP_ASSERT( dynamic_cast<scene::Animation *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Animation>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Animator>() )
            {
                WP_ASSERT( dynamic_cast<scene::Animator *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Animator>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Billboard>() )
            {
                WP_ASSERT( dynamic_cast<scene::Billboard *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Billboard>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Billboards>() )
            {
                WP_ASSERT( dynamic_cast<scene::Billboards *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Billboards>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Cubemap>() )
            {
                WP_ASSERT( dynamic_cast<scene::Cubemap *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Cubemap>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Collision>() )
            {
                WP_ASSERT( dynamic_cast<scene::Collision *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Collision>( scriptObj ) );
            }
            else if( scriptObj->isDerived<scene::CollisionBox>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionBox *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::CollisionBox>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionMesh>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionMesh *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::CollisionMesh>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionPlane>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionPlane *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::CollisionPlane>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionSphere>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionSphere *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::CollisionSphere>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CollisionTerrain>() )
            {
                WP_ASSERT( dynamic_cast<scene::CollisionTerrain *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::CollisionTerrain>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::TerrainSystem>() )
            {
                WP_ASSERT( dynamic_cast<scene::TerrainSystem *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::TerrainSystem>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::CarController>() )
            {
                WP_ASSERT( dynamic_cast<scene::CarController *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::CarController>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::VehicleController>() )
            {
                WP_ASSERT( dynamic_cast<scene::VehicleController *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::VehicleController>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::VideoPlayer>() )
            {
                WP_ASSERT( dynamic_cast<scene::VideoPlayer *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::VideoPlayer>( scriptObj ) );
                return true;
            }
            else if( scriptObj->isDerived<scene::WheelController>() )
            {
                WP_ASSERT( dynamic_cast<scene::WheelController *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::WheelController>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<scene::Material>() )
            {
                WP_ASSERT( dynamic_cast<scene::Material *>( scriptObj ) );
                luabind::detail::convert_to_lua( L, SmartPtr<scene::Material>( scriptObj ) );
                return true;
            }
            luabind::detail::convert_to_lua( L, SmartPtr<scene::IComponent>( scriptObj ) );
            return true;
        }
        if( scriptObj->isDerived<editor::FileSelection>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<editor::FileSelection>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsObject>() )
        {
            if( scriptObj->isDerived<IGraphicsCamera>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsCamera>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IFrustum>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IFrustum>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IGraphicsMesh>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsMesh>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IGraphicsLight>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsLight>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IInstancedObject>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IInstancedObject>( scriptObj ) );
                return true;
            }
            if( scriptObj->isDerived<IParticleSystem>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IParticleSystem>( scriptObj ) );
                return true;
            }
            luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsObject>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IRenderTarget>() )
        {
            if( scriptObj->isDerived<IGraphicsWindow>() )
            {
                luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsWindow>( scriptObj ) );
                return true;
            }
            luabind::detail::convert_to_lua( L, SmartPtr<IRenderTarget>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsScene>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsScene>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsSceneNode>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsSceneNode>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IAnimationStateController>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IAnimationStateController>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IAnimationController>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IAnimationController>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IParticleTechnique>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IParticleTechnique>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ITextureManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<ITextureManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsTerrain>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsTerrain>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsWater>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsWater>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsDeferredShading>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsDeferredShading>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IViewport>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IViewport>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IGraphicsSystem>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IGraphicsSystem>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<scene::IGameManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<scene::IGameManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFSMManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IFSMManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFSM>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IFSM>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFSMListener>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IFSMListener>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ISoundManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<ISoundManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IVideoManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IVideoManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IVideo>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IVideo>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IDatabaseQuery>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IDatabaseQuery>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IDatabase>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IDatabase>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IPhysicsManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IPhysicsManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IInputDeviceManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IInputDeviceManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IAnimator>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IAnimator>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IMesh>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IMesh>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ISubMesh>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<ISubMesh>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IFileSystem>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IFileSystem>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IStateContext>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IStateContext>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IStateManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IStateManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ITimer>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<ITimer>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IStateMessage>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IStateMessage>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IEvent>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IEvent>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<ISelectionManager>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<ISelectionManager>( scriptObj ) );
            return true;
        }

        if( scriptObj->isDerived<IEventListener>() )
        {
            luabind::detail::convert_to_lua( L, SmartPtr<IEventListener>( scriptObj ) );
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
            luabind::detail::value_converter().apply( L, p );
        }
        else if( ptr->isDerived<physics::ISphereShape2>() )
        {
            SmartPtr<physics::ISphereShape2> p = ptr;
            luabind::detail::value_converter().apply( L, p );
        }
        else
        {
            luabind::detail::value_converter().apply( L, ptr );
        }
    }

    template class CastUtil<physics::IPhysicsShape2>;
} // namespace workphone
