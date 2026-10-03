#include <GameEditorPCH.hpp>
#include "ApplicationData.hpp"
#include <FBApplication/Util/PropertiesSerializeUtil.hpp>



template<> fb::editor::ApplicationData* fb::Singleton<fb::editor::ApplicationData>::m_singleton = nullptr;



namespace fb
{
	namespace editor
	{
	
	
	
		//--------------------------------------------
		ApplicationData::ApplicationData()
		{
			m_outputFilePrefix = "BatchedScene_";
			setScenePath("./Scene.scene");
	
			m_halfExtents = Vector2F(750.0f, 750.0f);
			m_numCells = Vector2F(6,6);
	
			m_outFolder = "./";
			m_outSceneFile = "BatchedScene.scene";
			m_materialFile = "BatchedMaterials.material";
		}
	
	
	
		//--------------------------------------------
		ApplicationData::~ApplicationData()
		{
	
		}
	
	
	
		//--------------------------------------------
		void ApplicationData::loadData( const String& filePath )
		{
			Properties properties;
			PropertiesSerializeUtil::loadFromXML(properties, filePath);	
			setPropertyGroup(properties);
		}
	
	
	
		//--------------------------------------------
		void ApplicationData::saveData(const String& filePath)
		{
			Properties properties;
			getPropertyGroup( properties );
			PropertiesSerializeUtil::loadFromXML(properties, filePath);
		}
	
	
	
		//--------------------------------------------
		void ApplicationData::setOutputFilePrefix( const String& prefix )
		{
			m_outputFilePrefix = prefix;
		}
	
	
	
		//--------------------------------------------
		const String& ApplicationData::getOutputFilePrefix() const
		{
			return m_outputFilePrefix;
		}
	
	
	
		//--------------------------------------------
		void ApplicationData::setScenePath( const String& filePath )
		{
			m_scenePath = filePath;
		}
	
	
	
		//--------------------------------------------
		String ApplicationData::getScenePath() const
		{
			return m_scenePath;
		}
	
	
	
		//--------------------------------------------
		void ApplicationData::setPropertyGroup(const Properties& propertyGroup)
		{
			propertyGroup.getPropertyValue("OutputFilePrefix", m_outputFilePrefix);
			propertyGroup.getPropertyValue("ScenePath", m_scenePath);
	
			String buildingNames;
			propertyGroup.getPropertyValue("BuildingNames", buildingNames);
			m_buildingNames.clear();
			StringUtil::parseArray(buildingNames, m_buildingNames);
	
			String treeNames;
			propertyGroup.getPropertyValue("TreeNames", treeNames);
			m_treeNames.clear();
			StringUtil::parseArray(treeNames, m_treeNames);
	
			propertyGroup.getPropertyValue("HalfExtent", m_halfExtents);
			propertyGroup.getPropertyValue("NumCells", m_numCells);
	
			propertyGroup.getPropertyValue("OutputFolder", m_outFolder);
			propertyGroup.getPropertyValue("OutputSceneFile", m_outSceneFile);
			propertyGroup.getPropertyValue("MaterialFile", m_materialFile);
		}
	
	
	
		//--------------------------------------------
		void ApplicationData::getPropertyGroup( Properties& propertyGroup )
		{
			propertyGroup.setProperty("OutputFilePrefix", m_outputFilePrefix, String("string"));
			propertyGroup.setProperty("ScenePath", m_scenePath, String("file"));
	
			String buildingNames = StringUtil::toString(m_buildingNames);
			propertyGroup.setProperty("BuildingNames", buildingNames, String("Array"));
	
			String treeNames = StringUtil::toString(m_treeNames);
			propertyGroup.setProperty("TreeNames", treeNames, String("Array"));	
	
			propertyGroup.setProperty("HalfExtent", m_halfExtents);
			propertyGroup.setProperty("NumCells", m_numCells);
	
			propertyGroup.setProperty("OutputFolder", m_outFolder, String("folder"));
			propertyGroup.setProperty("OutputSceneFile", m_outSceneFile, String("file"));
			propertyGroup.setProperty("MaterialFile", m_materialFile, String("file"));
		}
	
		void ApplicationData::setBuildingNames( Array<String> val )
		{
			m_buildingNames = val;
		}
	
		const Array<String>& ApplicationData::getBuildingNames() const
		{
			return m_buildingNames;
		}
	
		void ApplicationData::setTreeNames( const Array<String>& val )
		{
			m_treeNames = val;
		}
	
		const Array<String>& ApplicationData::getTreeNames() const
		{
			return m_treeNames;
		}
	
		void ApplicationData::setHalfExtents( const Vector2F& val )
		{
			m_halfExtents = val;
		}
	
		const Vector2F& ApplicationData::getHalfExtents() const
		{
			return m_halfExtents;
		}
	
		void ApplicationData::setNumCells( const Vector2F& val )
		{
			m_numCells = val;
		}
	
		const Vector2F& ApplicationData::getNumCells() const
		{
			return m_numCells;
		}
	
	
	
		fb::String ApplicationData::getOutFolder() const
		{
			return m_outFolder;
		}

		void ApplicationData::setOutFolder(const String& val)
		{
			m_outFolder = val;
		}

		fb::String ApplicationData::getMaterialFile() const
		{
			return m_materialFile;
		}

		void ApplicationData::setMaterialFile(const String& val)
		{
			m_materialFile = val;
		}

		fb::String ApplicationData::getSceneFileName() const
		{
			return m_outSceneFile;
		}

		void ApplicationData::setSceneFileName(const String& val)
		{
			m_outSceneFile = val;
		}

		fb::String ApplicationData::getIgnoreNodes() const
		{
			return m_ignoreNodes;
		}

		void ApplicationData::setIgnoreNodes(const String& val)
		{
			m_ignoreNodes = val;
		}

	} // end namespace editor
	
	
}
