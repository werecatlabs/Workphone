#ifndef ProjectTreeData_h__
#define ProjectTreeData_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/treebase.hpp>




namespace fb
{
	namespace editor
	{
	
	
	
		//-------------------------------------------------
		class ProjectTreeData : public wxTreeItemData
		{
		public:
			ProjectTreeData() = default;

			ProjectTreeData(const String& ownerType, const String& objectType,
					SmartPtr<ISharedObject> ownerData, SmartPtr<ISharedObject> objectData);
	
			ProjectTreeData(wxMenu*	contextMenu, const String& ownerType,
					const String& objectType, SmartPtr<ISharedObject> ownerData, SmartPtr<ISharedObject> objectData);
	
			wxMenu* getContextMenu() const;
			void setContextMenu(wxMenu* val);

			SmartPtr<ISharedObject> getOwnerData() const;
			void setOwnerData(SmartPtr<ISharedObject> val);

			SmartPtr<ISharedObject> getObjectData() const;
			void setObjectData(SmartPtr<ISharedObject> val);
	
			String getOwnerType() const;
			void setOwnerType(const String& val);
	
			String getObjectType() const;
			void setObjectType(const String& val);
			
		protected:
			wxMenu*	m_contextMenu = nullptr;

			SmartPtr<ISharedObject> m_ownerData;
			SmartPtr<ISharedObject> m_objectData;
	
			String m_ownerType;
	
			String m_objectType;
		};


	} // end namespace editor
} // end namespace fb


#endif // ProjectTreeData_h__