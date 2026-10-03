//
// Created by Zane Desir on 11/11/2021.
//

#ifndef FB_ADDRESOURCECMD_H
#define FB_ADDRESOURCECMD_H


#include <GameEditorPrerequisites.hpp>
#include <FBCore/Interface/System/ICommand.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{
	namespace editor
	{


		class AddResourceCmd : public CSharedObject<ICommand>
		{
		public:
			enum class ResourceType
			{
				None,
				Script,
				Material,
				Scene,
			};

			AddResourceCmd();
			~AddResourceCmd();

			void redo();
			void execute();
			void undo();

			String getFilePath() const;
			void setFilePath(const String& filePath);

			ResourceType getResourceType() const;
			void setResourceType(ResourceType resourceType);

		protected:
			String m_filePath;
			AddResourceCmd::ResourceType m_resourceType = AddResourceCmd::ResourceType::None;
		};


	} // end namespace editor
} // end namespace fb

#endif //FB_ADDRESOURCECMD_H
