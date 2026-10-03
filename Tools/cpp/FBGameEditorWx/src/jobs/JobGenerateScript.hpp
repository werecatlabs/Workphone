#ifndef JobGenerateScript_h__
#define JobGenerateScript_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/CJob.hpp>



namespace fb
{
	namespace editor
	{



		class JobGenerateScript : public CJob
		{
		public:
			JobGenerateScript();
			~JobGenerateScript();

			void execute();

			SmartPtr<ScriptTemplate> getScriptTemplate() const;
			void setScriptTemplate(SmartPtr<ScriptTemplate> val);

			SmartPtr<EventTemplate> getEvent() const;
			void setEvent(SmartPtr<EventTemplate> val);

			bool isExistingScript() const;
			void setExistingScript(bool val);

		protected:
			SmartPtr<ScriptTemplate> m_scriptTemplate;
			SmartPtr<EventTemplate> m_event;

			bool m_isExistingScript = false;
		};



	} // end namespace editor	
} // end namespace fb



#endif // JobGenerateScript_h__
