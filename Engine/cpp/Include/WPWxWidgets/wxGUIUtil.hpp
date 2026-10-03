#ifndef _wxGUIUtil_H
#define _wxGUIUtil_H

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        class wxGUIUtil
        {
        public:
            /** */
            static void populateProperties( const Properties &propertyGroup, wxPropertyGrid *pg );

            /** */
            static void setPropertyValue( Properties &properties, wxPropertyGrid *pg,
                                          wxPGProperty *property );
        };

    }  // namespace ui
}  // namespace workphone

#endif
