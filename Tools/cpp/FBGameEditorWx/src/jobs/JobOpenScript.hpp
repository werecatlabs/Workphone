#ifndef JobOpenScript_h__
#define JobOpenScript_h__


#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/CJob.hpp>



namespace fb
{
	namespace editor
	{


		class JobOpenScript : public CJob
		{
		public:
			JobOpenScript();
			~JobOpenScript();

			void execute();

			SmartPtr<ScriptTemplate> getScriptTemplate() const;
			void setScriptTemplate(SmartPtr<ScriptTemplate> val);

		protected:
			SmartPtr<ScriptTemplate> m_scriptTemplate;
		};



	} // end namespace editor	
} // end namespace fb


#endif // JobOpenScript_h__
