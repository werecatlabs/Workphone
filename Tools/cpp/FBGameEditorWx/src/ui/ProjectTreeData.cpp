#include <GameEditorPCH.hpp>
#include "ui/ProjectTreeData.hpp"


namespace fb
{
	namespace editor
	{


		ProjectTreeData::ProjectTreeData(wxMenu* contextMenu, const String& ownerType, const String& objectType, SmartPtr<ISharedObject> ownerData, SmartPtr<ISharedObject> objectData)
			: m_contextMenu(contextMenu), m_ownerType(ownerType), m_objectType(objectType), m_ownerData(ownerData), m_objectData(objectData)
		{

		}

		ProjectTreeData::ProjectTreeData(const String& ownerType, const String& objectType, SmartPtr<ISharedObject> ownerData, SmartPtr<ISharedObject> objectData)
			: m_contextMenu(NULL), m_ownerType(ownerType), m_objectType(objectType), m_ownerData(ownerData), m_objectData(objectData)
		{

		}

		wxMenu* ProjectTreeData::getContextMenu() const
		{
			return m_contextMenu;
		}

		void ProjectTreeData::setContextMenu(wxMenu* val)
		{
			m_contextMenu = val;
		}

		SmartPtr<ISharedObject> ProjectTreeData::getOwnerData() const
		{
			return m_ownerData;
		}

		void ProjectTreeData::setOwnerData(SmartPtr<ISharedObject> val)
		{
			m_ownerData = val;
		}

		SmartPtr<ISharedObject> ProjectTreeData::getObjectData() const
		{
			return m_objectData;
		}

		void ProjectTreeData::setObjectData(SmartPtr<ISharedObject> val)
		{
			m_objectData = val;
		}

		String ProjectTreeData::getOwnerType() const
		{
			return m_ownerType;
		}

		void ProjectTreeData::setOwnerType(const String& val)
		{
			m_ownerType = val;
		}

		String ProjectTreeData::getObjectType() const
		{
			return m_objectType;
		}

		void ProjectTreeData::setObjectType(const String& val)
		{
			m_objectType = val;
		}



	} // end namespace editor
} // end namespace fb
