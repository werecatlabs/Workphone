#ifndef WP_OverlayElementState_H
#define WP_OverlayElementState_H

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{
    class WPCore_API OverlayElementState : public StateData
    {
    public:
        OverlayElementState();
        ~OverlayElementState() override;

        Vector2<real_Num> getPosition() const;

        void setPosition( const Vector2<real_Num> &position );

        Vector2<real_Num> getSize() const;

        void setSize( const Vector2<real_Num> &size );

        String getName() const override;

        void setName( const String &name ) override;

        void setMetricsMode( [[maybe_unused]] u8 metricsMode );

        u8 getMetricsMode() const;

        void setHorizontalAlignment( [[maybe_unused]] u8 gha );

        u8 getHorizontalAlignment() const;

        void setVerticalAlignment( [[maybe_unused]] u8 gva );

        u8 getVerticalAlignment() const;

        SmartPtr<render::IMaterial> getMaterial() const;

        void setMaterial( SmartPtr<render::IMaterial> material );

        void setCaption( const String &text );

        String getCaption() const;

        void setVisible( bool visible );

        bool isVisible() const;

        u32 getZOrder() const;

        void setZOrder( u32 zOrder );

        void setColour( const ColourF &colour );

        ColourF getColour() const;

        WP_CLASS_REGISTER_DECL;

        AtomicSmartPtr<render::IMaterial> m_material;

        ColourF m_colour = ColourF::White;

        u32 m_zOrder = 0;

        atomic_u8 m_metricsMode = 0;
        atomic_u8 m_gha = 0;
        atomic_u8 m_gva = 0;

        Vector2<real_Num> m_position = Vector2<real_Num>::zero();
        Vector2<real_Num> m_size = Vector2<real_Num>::unit();
        FixedString<WP_MAX_CLASSNAME> m_name;
        FixedString<WP_MAX_CLASSNAME> m_caption;

        atomic_bool m_visible = true;
    };
}  // namespace workphone

#endif  // WP_OverlayElementState_H
