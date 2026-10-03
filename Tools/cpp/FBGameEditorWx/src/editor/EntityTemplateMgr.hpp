#ifndef EntityTemplateMgr_h__
#define EntityTemplateMgr_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Base/Map.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class EntityTemplateMgr : public CSharedObject<ISharedObject>
		{
		public:
			EntityTemplateMgr();
			~EntityTemplateMgr();
	
			void addTemplate(const String& name, SmartPtr<EntityTemplate> entityTemplate);
			void removeTemplate(const String& name);
			void removeTemplate(SmartPtr<EntityTemplate> entityTemplate);
			SmartPtr<EntityTemplate> findTemplate(const String& name);
	
		protected:
			typedef Map<String, SmartPtr<EntityTemplate>> EntityTemplates;
			EntityTemplates m_templates;
		};
		
	
	
	} // end namespace editor
} // end namespace fb



#endif // EntityTemplateMgr_h__