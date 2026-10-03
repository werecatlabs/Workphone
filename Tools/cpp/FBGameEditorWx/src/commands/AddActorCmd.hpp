#ifndef AddEntityCmd_h__
#define AddEntityCmd_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class AddActorCmd : public CSharedObject<ICommand>
		{
		public:
			enum class ActorType
			{
				Button,
				Car,
				Canvas,
				Cube,
				CubeGround,
				Plane,
				Panel,
				ProceduralScene,
				Text,
				Terrain,

				EmptyActor
			};

			AddActorCmd();
			~AddActorCmd();
	
			void undo() override;
			void redo() override;
			void execute() override;

			SmartPtr<IActor> createActor();

			String getCommandId() const;
	
			SmartPtr<IActor> getActor() const;
			void setActor(SmartPtr<IActor> val);

			SmartPtr<IActor> getParent() const;
			void setParent(SmartPtr<IActor> val);

			AddActorCmd::ActorType getActorType() const;
			void setActorType(AddActorCmd::ActorType val);

		protected:
			SmartPtr<IActor> m_actor;
			SmartPtr<IActor> m_parent;
			AddActorCmd::ActorType m_actorType = AddActorCmd::ActorType::EmptyActor;
		};
		
	
	
	} // end namespace editor	
} // end namespace fb	



#endif // AddEntityCmd_h__

