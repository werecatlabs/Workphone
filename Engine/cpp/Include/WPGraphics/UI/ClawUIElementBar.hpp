#ifndef ClawUIElementBar_h__
#define ClawUIElementBar_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIBar.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIElementBar : public ClawUIElement<IUIBar>
        {
        public:
            ClawUIElementBar();
            ~ClawUIElementBar() override;

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

            void draw( struct wp_context *ctx ) override;

        private:
            String m_materialName;
            f32 m_targetPoints = 0.0f;
            f32 m_prevPoints = 0.0f;
            f32 m_curPoints = 0.0f;
            f32 m_maxPoints = 1.0f;
            u8 m_barOrientation = static_cast<u8>( BarOrientation::BO_HORIZONTAL );

            Array<SmartPtr<IUIImage>> m_images;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ClawUIElementBar_h__
