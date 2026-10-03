//
// Created by Zane Desir on 11/11/2021.
//

#include <GameEditorPCH.hpp>
#include <commands/AddResourceCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/ProjectManager.hpp>
#include <editor/Project.hpp>
#include <FBObjectTemplates/ScriptTemplate.hpp>
#include <FBCore/Interface/IApplicationManager.hpp>
#include <FBCore/Base/DataUtil.hpp>
#include <FBData/FBData.hpp>
#include <FBApplication/ApplicationUtil.hpp>



namespace fb
{
	namespace editor
	{


		AddResourceCmd::AddResourceCmd()
		{

		}

		AddResourceCmd::~AddResourceCmd()
		{

		}

		void AddResourceCmd::redo()
		{

		}

		void AddResourceCmd::execute()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto fileSystem = applicationManager->getFileSystem();
				FB_ASSERT(fileSystem);

				auto editorManager = EditorManager::getSingletonPtr();
				auto projectManager = editorManager->getProjectManager();

				switch (m_resourceType)
				{
				case AddResourceCmd::ResourceType::Material:
				{
					auto material = ApplicationUtil::createDefaultMaterial();
					FB_ASSERT(material);

					auto pData = material->toData();
					FB_ASSERT(pData);
					
					auto mat = pData->getDataAsType<data::material_graph>();

					auto materialStr = DataUtil::toString(mat, true);
					FB_ASSERT(!StringUtil::isNullOrEmpty(materialStr));

					if (!fileSystem->isExistingFile(m_filePath))
					{
						fileSystem->writeAllText(m_filePath, materialStr);
					}
					else
					{
						auto fileIndex = 0;
						auto maxRetries = 1000;

						auto path = Path::getFilePath(m_filePath);
						auto fileName = Path::getFileNameWithoutExtension(m_filePath);
						auto fileExt = Path::getFileExtension(m_filePath);

						auto filePath = path + fileName + StringUtil::toString(fileIndex) + fileExt;
						while (fileSystem->isExistingFile(filePath) && fileIndex < maxRetries)
						{
							++fileIndex;
							filePath = path + fileName + StringUtil::toString(fileIndex) + fileExt;
						}

						fileSystem->writeAllText(filePath, materialStr);
					}
				}
				break;
				};

				auto refreshPath = Path::getFilePath(m_filePath);
				fileSystem->refreshPath(refreshPath, true);
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}

		void AddResourceCmd::undo()
		{

		}

		String AddResourceCmd::getFilePath() const
		{
			return m_filePath;
		}

		void AddResourceCmd::setFilePath(const String& filePath)
		{
			m_filePath = filePath;
		}

		AddResourceCmd::ResourceType AddResourceCmd::getResourceType() const
		{
			return m_resourceType;
		}

		void AddResourceCmd::setResourceType(AddResourceCmd::ResourceType resourceType)
		{
			m_resourceType = resourceType;
		}


	} // end namespace editor
} // end namespace fb


