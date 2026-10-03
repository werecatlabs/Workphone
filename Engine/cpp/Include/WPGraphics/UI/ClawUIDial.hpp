#ifndef _ClawUIDIAL_H
#define _ClawUIDIAL_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIDial.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        //! interface for dial class used to make speedometers etc
        class ClawUIDial : public ClawUIElement<IUIDial>
        {
        public:
            ClawUIDial();
            ~ClawUIDial() override;

            virtual void initialise();

            void setNeedlePosition( f32 position ) override;

            void draw( struct wp_context *ctx ) override;

        protected:
            f32 m_fStartAngle;   // the minimum angle of the dial needle
            f32 m_fEndAngle;     // the maximum angle the dial need
            f32 m_fNeedleAngle;  // the angle of the dial needle

            bool m_bIsVisible;  // value to know if the dial is visible or not
        };
    }  // end namespace ui
}  // namespace workphone

#endif
