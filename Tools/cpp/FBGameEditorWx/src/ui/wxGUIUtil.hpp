#ifndef _wxGUIUtil_H
#define _wxGUIUtil_H



#include <GameEditorPrerequisites.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <FBCore/Base/Properties.hpp>



namespace fb
{
	namespace editor
	{


	
		//--------------------------------------------
		class wxGUIUtil
		{
		public:
			/** */
			static void populateProperties(const fb::Properties& propertyGroup, wxPropertyGrid* pg);
	
			/** */
			static void setPropertyValue(fb::Properties& properties, wxPropertyGrid* pg,  wxPGProperty* property);
		};
	


	} // end namespace editor	
} // end namespace fb



#endif


