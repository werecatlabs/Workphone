#ifndef _ClawUIEVENT_H
#define _ClawUIEVENT_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIEvent : public ClawUIElement<IUIElement>
        {
        public:
            ClawUIEvent();
            ~ClawUIEvent() override;

            void initialise( SmartPtr<IUIElement> &parent, const TiXmlNode *pNode );

            void setEventType( const String &eventType );
            const String &getEventType() const;

            //
            // Callbacks
            //
            void OnActivateCallback();
            void OnSelectCallback();
            void OnDeselectCallback();

        private:
            String m_eventType;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
