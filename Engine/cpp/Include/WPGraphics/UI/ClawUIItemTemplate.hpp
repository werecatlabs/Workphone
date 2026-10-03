#ifndef _ClawUIItemTemplate_H
#define _ClawUIItemTemplate_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include "Workphone/Core/Properties.hpp"
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIItemTemplate : public ClawUIElement<IUIElement>
        {
        public:
            ClawUIItemTemplate();
            ~ClawUIItemTemplate() override;

            void initialise( SmartPtr<IUIElement> &parent, const TiXmlNode *pNode );

            const Properties &getPropertyGroup() const;
            Properties &getPropertyGroup();

        private:
            Properties m_propertyGroup;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
