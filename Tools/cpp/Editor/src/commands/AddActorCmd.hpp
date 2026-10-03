#ifndef AddEntityCmd_h__
#define AddEntityCmd_h__

#include <commands/Command.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{
    namespace editor
    {
        class AddActorCmd : public Command
        {
        public:
            enum class ActorType
            {
                Actor,
                Button,
                Camera,
                Car,
                Canvas,
                Checkbox,
                Constraint,
                Cube,
                CubeMesh,
                Cubemap,
                CubeGround,
                DirectionalLight,
                Dropdown,
                Helicopter,
                Panel,
                ParticleSystem,
                ParticleSystemSmoke,
                ParticleSystemSand,
                Plane,
                PlaneMesh,
                ProceduralScene,
                PointLight,
                PhysicsCube,
                Skybox,
                SimpleButton,
                Scrollbar,
                ScrollbarVertical,
                Scrollview,
                Slider,
                SliderVertical,
                TabView,
                TableLayout,
                ToggleButton,
                ToggleWithText,
                Terrain,
                Text,
                Vehicle,
                RenderTarget,

                EmptyActor
            };

            static const String prefabExtStr;

            AddActorCmd();
            ~AddActorCmd() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void undo() override;
            void redo() override;
            void execute() override;

            SmartPtr<scene::IGameActor> getActor() const;
            void setActor( SmartPtr<scene::IGameActor> actor );

            SmartPtr<scene::IGameActor> getParent() const;
            void setParent( SmartPtr<scene::IGameActor> parent );

            ActorType getActorType() const;
            void setActorType( ActorType actorType );

            String getFilePath() const;
            void setFilePath( const String &filePath );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<scene::IGameActor> createActor();

            AtomicSmartPtr<scene::IGameActor> m_actor;
            AtomicSmartPtr<scene::IGameActor> m_parent;
            AtomicValue<ActorType> m_actorType = ActorType::EmptyActor;
            AtomicObject<String> m_filePath;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // AddEntityCmd_h__
