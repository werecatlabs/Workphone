#ifndef _ClawUICursor_H
#define _ClawUICursor_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUICursor.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUICursor : public ClawUIElement<IUICursor>
        {
        public:
            ClawUICursor();
            ~ClawUICursor() override;

            void initialise();

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            void setMaterialName( const String &materialName ) override;

            void setSize( const Vector2F &size ) override;

            void draw( struct wp_context *ctx ) override;

        private:
            String m_materialName;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
