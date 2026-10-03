#ifndef MessageScriptError_h__
#define MessageScriptError_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/MessageStandard.hpp>



namespace fb
{	
	namespace editor
	{
		
	
	
		class MessageScriptError : public MessageStandard
		{
		public:
			MessageScriptError();
			~MessageScriptError();
	
			String getSourcePath() const;
			void setSourcePath(const String& val);
	
			int getLineNumber() const;
			void setLineNumber(int val);
	
		protected:
			String m_sourcePath;
			int m_lineNumber;
		};
	

	
	} // end namespace editor
} // end namespace fb



#endif // MessageScriptError_h__


