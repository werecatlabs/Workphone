#ifndef _AppData_H
#define _AppData_H



#include <GameEditorPrerequisites.hpp>




namespace fb
{
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class ApplicationData : public Singleton<ApplicationData>
		{
		public: 
			ApplicationData();
			~ApplicationData();
	
			void loadData(const String& filePath);
			void saveData(const String& filePath);
	
			void setOutputFilePrefix(const String& prefix);
			const String& getOutputFilePrefix() const;	
	
			void setScenePath(const String& filePath);
			String getScenePath() const;	
	
			void setBuildingNames(Array<String> val);
			const Array<String>& getBuildingNames() const;
		
			void setTreeNames(const Array<String>& val);
			const Array<String>& getTreeNames() const;
		
			void setPropertyGroup(const Properties& propertyGroup);
			void getPropertyGroup(Properties& propertyGroup);
	
			void setHalfExtents(const Vector2F& val);
			const Vector2F& getHalfExtents() const;	
	
			void setNumCells(const Vector2F& val);
			const Vector2F& getNumCells() const;
	
			String getOutFolder() const;
			void setOutFolder(const String& val);
	
			String getMaterialFile() const;
			void setMaterialFile(const String& val);
	
			String getSceneFileName() const;
			void setSceneFileName(const String& val);
	
			String getIgnoreNodes() const;
			void setIgnoreNodes(const String& val);
	
		protected:
			Array<String> m_buildingNames;
			Array<String> m_treeNames;
			Vector2F m_halfExtents;
			Vector2F m_numCells;
			String m_scenePath;
			String m_outputFilePrefix;
			String m_outFolder;
			String m_outSceneFile;
			String m_materialFile;
			String m_ignoreNodes;
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif


