#ifndef RemoveSelectionCmd_h__
#define RemoveSelectionCmd_h__

#include <commands/Command.hpp>

namespace workphone
{
    namespace editor
    {
        /** Command to remove selected actors from the scene.
         */
        class RemoveSelectionCmd : public Command
        {
        public:
            class ActorData : public ISharedObject
            {
            public:
                ActorData();
                ~ActorData() override;

                SmartPtr<scene::IGameActor> getParent() const;
                void setParent( SmartPtr<scene::IGameActor> parent );

                SmartPtr<scene::IGameActor> getActor() const;
                void setActor( SmartPtr<scene::IGameActor> actor );

                SmartPtr<ISharedObject> getActorData() const;
                void setActorData( SmartPtr<ISharedObject> actorData );

                WP_CLASS_REGISTER_DECL;

            private:
                SmartPtr<scene::IGameActor> m_parent;
                SmartPtr<scene::IGameActor> m_actor;
                SmartPtr<ISharedObject> m_actorData;
            };

            /** Constructor.
             */
            RemoveSelectionCmd();

            /** Destructor.
             */
            ~RemoveSelectionCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            struct ComponentData
            {
                SmartPtr<scene::IGameActor> actor;
                SmartPtr<scene::IComponent> component;
            };

            Array<SmartPtr<ActorData>> m_actorData;
            Array<ComponentData> m_componentData;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // RemoveSelectionCmd_h__
