#include <GameEditorPCH.hpp>
#include "editor/ComponentTemplateMgr.hpp"
#include <FBCore/Interface/Database/IDatabaseQuery.hpp>
#include <FBObjectTemplates/ComponentTemplate.hpp>
#include <tinyxml.hpp>
#include <FBSQLite/FBSQLite.hpp>



namespace fb
{	
	namespace editor
	{
	
		
	
		//--------------------------------------------
		ComponentTemplateMgr::ComponentTemplateMgr()
		{
		}
	
	
	
		//--------------------------------------------
		ComponentTemplateMgr::~ComponentTemplateMgr()
		{
		}
	
	
	
		//--------------------------------------------
		String ComponentTemplateMgr::nullCheck(const String& val)
		{
			if(val == "")
			{
				return String("NULL");
			}
	
			return String("\"") + val +  String("\"");
		}
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::saveComponents()
		{
			//SmartPtr<IDatabase> database = createSQLiteDB();
			//database->load("../Media/components.db");
	
			//Array<SmartPtr<ComponentTemplate>> componentTempates = getComponents();
			//for(u32 componentIdx=0; componentIdx<componentTempates.size(); ++componentIdx)
			//{
			//	SmartPtr<ComponentTemplate> componentTemplate = componentTempates[componentIdx];
			//	write(componentTemplate, database);
			//}
		}
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::loadComponents()
		{
			SmartPtr<IDatabase> database = createSQLiteDB();
			database->load("../Media/components.db");
			SmartPtr<IDatabaseQuery> componentQuery = database->query("select * from components");
			while(!componentQuery->eof())
			{
				SmartPtr<ComponentTemplate> componentTemplate(new ComponentTemplate);
	
				String id = componentQuery->getFieldValue("component_id");
				componentTemplate->setId(id);
	
				Properties properties;
	
				String propertiesQueryStr = String("select * from properties where component_id = \"") + id + String("\";");
				SmartPtr<IDatabaseQuery> propertiesQuery = database->query(propertiesQueryStr);
				while(!propertiesQuery->eof())
				{
					properties.addProperty(propertiesQuery->getFieldValue("name"), propertiesQuery->getFieldValue("value"));
					propertiesQuery->nextRow();
				}
	
				properties.setProperty("id", id);
				componentTemplate->setProperties(properties);
	
				String eventsQueryStr = String("select * from events where component_ref = \"") + id + String("\";");
				SmartPtr<IDatabaseQuery> eventsQuery = database->query(eventsQueryStr);
				while(!eventsQuery->eof())
				{
					SmartPtr<EventTemplate> eventTemplate(new EventTemplate);
					eventTemplate->setType(eventsQuery->getFieldValue("type"));
					eventTemplate->setClassName(eventsQuery->getFieldValue("class_name"));
					eventTemplate->setFunction(eventsQuery->getFieldValue("function"));
					eventTemplate->setLabel(eventsQuery->getFieldValue("label"));
	
					Array<String> parameters;
					StringUtil::parseArray(eventsQuery->getFieldValue("parameters"), parameters);
					eventTemplate->setParameters(parameters);
					componentTemplate->addEventTemplate(eventTemplate);
	
					eventsQuery->nextRow();
				}
	
				m_components.push_back(componentTemplate);
	
				componentQuery->nextRow();
			}
		}
	
	
	
		//--------------------------------------------
		SmartPtr<ComponentTemplate> ComponentTemplateMgr::getTemplateByType( const String& type )
		{
			for(u32 i=0; i<m_components.size(); ++i)
			{
				SmartPtr<ComponentTemplate>& compTemplate = m_components[i];
				if(type==(compTemplate->getType()))
					return compTemplate;
			}
	
			return nullptr;
		}
	
		
	
		//--------------------------------------------
		SmartPtr<ComponentTemplate> ComponentTemplateMgr::getTemplateByName(const String& name)
		{
			for(u32 i=0; i<m_components.size(); ++i)
			{
				SmartPtr<ComponentTemplate>& compTemplate = m_components[i];
				if(name==(compTemplate->getName()))
					return compTemplate;
			}
	
			return nullptr;
		}
	
	
	
		//--------------------------------------------
		bool ComponentTemplateMgr::isExistingComponent(const String& name)
		{
			for(u32 i=0; i<m_components.size(); ++i)
			{
				SmartPtr<ComponentTemplate>& compTemplate = m_components[i];
				if(name==(compTemplate->getLabel()))
					return true;
			}
	
			return false;
		}
	
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::deleteComponent(const String& name)
		{
			for(u32 i=0; i<m_components.size(); ++i)
			{
				SmartPtr<ComponentTemplate>& compTemplate = m_components[i];
				if(name==(compTemplate->getLabel()))
				{
					//m_components.erase_element_index(i);
					return;
				}
			}
		}
	
	
	
		//--------------------------------------------
		Array<SmartPtr<ComponentTemplate>> ComponentTemplateMgr::getComponents() const
		{
			return m_components;
		}
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::setComponents( const Array<SmartPtr<ComponentTemplate>>& val )
		{
			m_components = val;
		}
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::addComponent( SmartPtr<ComponentTemplate> componentTemplate )
		{
			m_components.push_back(componentTemplate);
	
			SmartPtr<IDatabase> database = createSQLiteDB();
			database->load("../Media/components.db");
			write( componentTemplate, database );
		}
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::componentModified(SmartPtr<ComponentTemplate> componentTemplate)
		{
			SmartPtr<IDatabase> database = createSQLiteDB();
			database->load("../Media/components.db");
	
			delete_(componentTemplate, database);
	
			write( componentTemplate, database );
		}
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::write( SmartPtr<ComponentTemplate> componentTemplate, SmartPtr<IDatabase> database )
		{
			String component_id = nullCheck(componentTemplate->getId());
	
			String componentQueryStr = String("INSERT INTO components VALUES(NULL, ") + component_id + String(");");
			database->query(componentQueryStr);
	
			Properties properties;
			componentTemplate->getProperties(properties);
			Array<Property> propertiesArray = properties.getPropertiesAsArray();
	
			for(u32 propertyIdx=0; propertyIdx<propertiesArray.size(); ++propertyIdx)
			{
				Property& property = propertiesArray[propertyIdx];
	
				String propertyName = nullCheck(property.getName());
				String propertyValue = nullCheck(property.getValue());
	
				String propertyQueryStr = String("INSERT INTO properties VALUES(NULL, ") + component_id + String(", ") + 
					propertyName + String(", ") + propertyValue + String(");");
				database->query(propertyQueryStr);
			}
	
			Array<SmartPtr<EventTemplate>> eventTemplates = componentTemplate->getEventTemplates();
			for(u32 eventIdx=0; eventIdx<eventTemplates.size(); ++eventIdx)
			{
				SmartPtr<EventTemplate> eventTemplate = eventTemplates[eventIdx];
	
				String type = nullCheck(eventTemplate->getType());
				String label = nullCheck(eventTemplate->getLabel());
				String className = nullCheck(eventTemplate->getClassName());
				String functionName = nullCheck(eventTemplate->getFunction());
				String params = nullCheck(StringUtil::toString(eventTemplate->getParameters()));
	
				String queryEventStr = String("INSERT INTO events VALUES(NULL, ") + component_id + String(", ") + 
					type + String(", ") + className + String(", ") + functionName + String(", ") + 
					label + String(", ") + params + String(");");
				database->query(queryEventStr);
			}
		}
	
	
	
		//--------------------------------------------
		void ComponentTemplateMgr::delete_( SmartPtr<ComponentTemplate> componentTemplate, SmartPtr<IDatabase> database )
		{
			String component_id = nullCheck(componentTemplate->getId());
			String componentQueryStr = String("DELETE FROM components WHERE component_id=") + component_id + String(";");
			database->query(componentQueryStr);
	
			String propertyQueryStr = String("DELETE FROM properties WHERE component_id=") + component_id + String(";");
			database->query(propertyQueryStr);
	
			String eventQueryStr = String("DELETE FROM events WHERE component_ref=") + component_id + String(";");
			database->query(eventQueryStr);
		}
	
	
	
	} // end namespace editor
} // end namespace fb


