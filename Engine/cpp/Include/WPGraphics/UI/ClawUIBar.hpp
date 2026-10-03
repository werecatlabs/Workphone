#ifndef _ClawUIBar_H
#define _ClawUIBar_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIBar.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>
#include "ClawUIImage.hpp"

struct wp_context;

namespace workphone
{
    namespace ui
    {
        class ClawUIBar : public ClawUIElement<IUIBar>
        {
        public:
            ClawUIBar();
            ~ClawUIBar() override;

            void setMaterialName( const String &materialName );

            void setPosition( const Vector2F &position ) override;

            //! the amount of points the bar has
            //! this represented as a whole number
            void setPoints( f32 points ) override;

            //! set the maximum amount of points to the bar can contain
            //! e.g. a player might have 250 health points
            void setMaxPoints( f32 maxPoints ) override;

            //! Update the energy the bar to display changes in energy
            void update() override;

            u8 getBarOrientation() const;

            void setBarOrientation( u8 barOrientation );

            /** @copydoc GuiElement::draw */
            void draw( struct wp_context *ctx ) override;

        private:
            String m_materialName;
            f32 m_targetPoints = 0.0f;
            f32 m_prevPoints = 0.0f;
            f32 m_curPoints = 0.0f;
            f32 m_maxPoints = 1.0f;
            u8 m_barOrientation = static_cast<u8>( BarOrientation::BO_HORIZONTAL );
        };
    }  // end namespace ui
}  // namespace workphone

#endif
