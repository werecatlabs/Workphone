#ifndef AddComponentCmd_h__
#define AddComponentCmd_h__

#include <commands/Command.hpp>

namespace workphone
{
    namespace editor
    {
        class AddComponentCmd : public Command
        {
        public:
            AddComponentCmd();
            ~AddComponentCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;

            SmartPtr<IFactory> getFactory() const;

            void setFactory( SmartPtr<IFactory> factory );

            SmartPtr<scene::IComponent> getComponent() const;

            void setComponent( SmartPtr<scene::IComponent> component );

            SmartPtr<scene::IGameActor> getActor() const;

            void setActor( SmartPtr<scene::IGameActor> actor );

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicSmartPtr<scene::IGameActor> m_actor;
            AtomicSmartPtr<scene::IComponent> m_component;
            AtomicSmartPtr<IFactory> m_factory;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // AddComponentCmd_h__
