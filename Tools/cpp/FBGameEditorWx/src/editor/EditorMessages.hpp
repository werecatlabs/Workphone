#ifndef EditorMessages_h__
#define EditorMessages_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/MessageStandard.hpp>


namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class EntityTreeItemSelected : public MessageStandard
		{
		public:
			EntityTreeItemSelected(){ setType("EntityTreeItemSelected"); }
			~EntityTreeItemSelected(){}
	
			String getComponentType() const { return m_componentType; }
			void setComponentType(fb::String val) { m_componentType = val; }
	
			String getComponentName() const { return m_componentName; }
			void setComponentName(fb::String val) { m_componentName = val; }
	
			SmartPtr<IEditableObject> getSelectedObject() const { return m_selectedObject; }
			void setSelectedObject(SmartPtr<IEditableObject> val) { m_selectedObject = val; }
	
		protected:
			/// 
			String m_componentType;
	
			/// 
			String m_componentName;
	
			/// 
			SmartPtr<IEditableObject> m_selectedObject;
		};
	
	
		typedef SmartPtr<EntityTreeItemSelected> EntityTreeItemSelectedPtr;
	
	
	
		//--------------------------------------------
		class ComponentItemSelected : public MessageStandard
		{
		public:
			ComponentItemSelected(){ setType("ComponentItemSelected"); }
			~ComponentItemSelected(){}
	
			String getComponentType() const { return m_componentType; }
			void setComponentType(fb::String val) { m_componentType = val; }
	
			String getComponentName() const { return m_componentName; }
			void setComponentName(fb::String val) { m_componentName = val; }
	
			SmartPtr<IEditableObject> getSelectedObject() const { return m_selectedObject; }
			void setSelectedObject(SmartPtr<IEditableObject> val) { m_selectedObject = val; }
	
		protected:
			/// 
			String m_componentType;
	
			/// 
			String m_componentName;
	
			/// 
			SmartPtr<IEditableObject> m_selectedObject;
		};
	
	
		typedef SmartPtr<ComponentItemSelected> ComponentItemSelectedPtr;
	
	
	
		//--------------------------------------------
		class ComponentDeleteCurrentProperty : public MessageStandard
		{
		public:
			ComponentDeleteCurrentProperty(){ setType("ComponentDeleteCurrentProperty"); }
			~ComponentDeleteCurrentProperty(){}
	
			String getComponentType() const { return m_componentType; }
			void setComponentType(fb::String val) { m_componentType = val; }
	
			String getComponentName() const { return m_componentName; }
			void setComponentName(fb::String val) { m_componentName = val; }
	
			SmartPtr<IEditableObject> getSelectedObject() const { return m_selectedObject; }
			void setSelectedObject(SmartPtr<IEditableObject> val) { m_selectedObject = val; }
	
		protected:
			/// 
			String m_componentType;
	
			/// 
			String m_componentName;
	
			/// 
			SmartPtr<IEditableObject> m_selectedObject;
		};
	
	
		typedef SmartPtr<ComponentDeleteCurrentProperty> ComponentDeleteCurrentPropertyPtr;
	
	
	
		//--------------------------------------------
		class EntityTreeItemActivated : public MessageStandard
		{
		public:
			EntityTreeItemActivated(){ setType("EntityTreeItemActivated"); }
			~EntityTreeItemActivated(){}
	
			String getComponentType() const { return m_componentType; }
			void setComponentType(fb::String val) { m_componentType = val; }
	
			String getComponentName() const { return m_componentName; }
			void setComponentName(fb::String val) { m_componentName = val; }
	
			SmartPtr<IEditableObject> getObject() const { return m_object; }
			void setObject(SmartPtr<IEditableObject> val) { m_object = val; }
	
		protected:
			/// 
			String m_componentType;
	
			/// 
			String m_componentName;
	
			/// 
			SmartPtr<IEditableObject> m_object;
		};
	
	
		typedef SmartPtr<EntityTreeItemActivated> EntityTreeItemActivatedPtr;
	
	
		//--------------------------------------------
		class LoadEntityTemplates : public MessageStandard
		{
		public:
		};
	
	
		typedef SmartPtr<LoadEntityTemplates> LoadEntityTemplatesPtr;
	
	
	
		//--------------------------------------------
		class SelectEntityTemplate : public MessageStandard
		{
		public:
			SelectEntityTemplate(){ setType("SelectEntityTemplate"); }
			~SelectEntityTemplate(){}
		};
	
	
		typedef SmartPtr<SelectEntityTemplate> SelectEntityTemplatePtr;
	
	
		//--------------------------------------------
		class LoadProjectMsg : public MessageStandard
		{
		public:
			LoadProjectMsg(){ setType("LoadProjectMsg"); }
			~LoadProjectMsg(){}
		};
	
	
		typedef SmartPtr<LoadProjectMsg> LoadProjectMsgPtr;
	
	
	
	} // end namespace editor
} // end namespace fb



#endif // EditorMessages_h__


