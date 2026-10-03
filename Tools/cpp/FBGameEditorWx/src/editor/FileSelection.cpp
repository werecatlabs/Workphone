#include <GameEditorPCH.hpp>
#include "editor/FileSelection.hpp"

#include <FBCore/Reflection/ReflectionClassDefinition.hpp>



namespace fb
{
	namespace editor
	{

		FB_CLASS_REGISTER_DERIVED(editor, FileSelection, ISharedObject);

		FileSelection::FileSelection()
		{

		}

		FileSelection::~FileSelection()
		{

		}

		String FileSelection::getFilePath() const
		{
			return m_filePath;
		}

		void FileSelection::setFilePath(const String& val)
		{
			m_filePath = StringUtil::replaceAll(val, "\\", "/");
		}



	} // end namespace editor	
} // end namespace fb


