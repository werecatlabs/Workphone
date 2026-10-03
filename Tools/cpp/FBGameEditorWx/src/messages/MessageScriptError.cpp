#include <GameEditorPCH.hpp>
#include "messages/MessageScriptError.hpp"



namespace fb
{
	
	namespace editor
	{
		
	
		MessageScriptError::MessageScriptError()
		{
	
		}
	
		MessageScriptError::~MessageScriptError()
		{
	
		}
	
		String MessageScriptError::getSourcePath() const
		{
			return m_sourcePath;
		}

		void MessageScriptError::setSourcePath(const String& val)
		{
			m_sourcePath = val;
		}

		int MessageScriptError::getLineNumber() const
		{
			return m_lineNumber;
		}

		void MessageScriptError::setLineNumber(int val)
		{
			m_lineNumber = val;
		}



	} // end namespace editor
} // end namespace fb



