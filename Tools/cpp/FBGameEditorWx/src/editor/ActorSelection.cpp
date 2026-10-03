#include <GameEditorPCH.hpp>
#include "editor/ActorSelection.hpp"

#include <FBCore/Reflection/ReflectionClassDefinition.hpp>



namespace fb
{
	namespace editor
	{

		FB_CLASS_REGISTER_DERIVED(editor, ActorSelection, ISharedObject);

		ActorSelection::ActorSelection()
		{

		}

		ActorSelection::~ActorSelection()
		{

		}

		String ActorSelection::getFilePath() const
		{
			return m_filePath;
		}

		void ActorSelection::setFilePath(const String& val)
		{
			m_filePath = StringUtil::replaceAll(val, "\\", "/");
		}



	} // end namespace editor	
} // end namespace fb


