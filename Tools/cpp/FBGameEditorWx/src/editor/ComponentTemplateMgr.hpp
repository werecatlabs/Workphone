#ifndef ComponentTemplateMgr_h__
#define ComponentTemplateMgr_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Base/StringTypes.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class ComponentTemplateMgr : public CSharedObject<ISharedObject>
		{
		public:
			ComponentTemplateMgr();
			~ComponentTemplateMgr();
	
			void saveComponents();	
			void loadComponents();
	
			SmartPtr<ComponentTemplate> getTemplateByType(const String& type);
			SmartPtr<ComponentTemplate> getTemplateByName(const String& name);
	
			bool isExistingComponent(const String& name);
	
			Array<SmartPtr<ComponentTemplate>> getComponents() const;
			void setComponents(const Array<SmartPtr<ComponentTemplate>>& val);
			void addComponent(SmartPtr<ComponentTemplate> componentTemplate);
			void deleteComponent(const String& name);
			void componentModified(SmartPtr<ComponentTemplate> componentTemplate);		
	
		protected:
			void write( SmartPtr<ComponentTemplate> componentTemplate, SmartPtr<IDatabase> database );
			void delete_( SmartPtr<ComponentTemplate> componentTemplate, SmartPtr<IDatabase> database );
			String nullCheck(const String& val);
	
			/// 
			Array<SmartPtr<ComponentTemplate>> m_components;
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif // ComponentTemplateMgr_h__


