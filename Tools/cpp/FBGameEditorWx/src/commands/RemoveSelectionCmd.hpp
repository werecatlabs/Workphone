#ifndef RemoveSelectionCmd_h__
#define RemoveSelectionCmd_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Interface/System/ICommand.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class RemoveSelectionCmd : public CSharedObject<ICommand>
		{
		public:
			RemoveSelectionCmd();
			~RemoveSelectionCmd();

			void undo() override;
			void redo() override;
			void execute() override;

		protected:
			class ActorData : public CSharedObject<ISharedObject>
			{
			public:
				ActorData();
				~ActorData();

				SmartPtr<IActor> getParent() const;
				void setParent(SmartPtr<IActor> val);

				SmartPtr<IActor> getActor() const;
				void setActor(SmartPtr<IActor> val);

				SmartPtr<IData> getActorData() const;
				void setActorData(SmartPtr<IData> val);

			private:
				SmartPtr<IActor> m_parent;
				SmartPtr<IActor> m_actor;
				SmartPtr<IData> m_actorData;
			};

			Array<SmartPtr<ActorData>> m_actorData;
		};



	} // end namespace editor
} // end namespace fb



#endif // RemoveSelectionCmd_h__


