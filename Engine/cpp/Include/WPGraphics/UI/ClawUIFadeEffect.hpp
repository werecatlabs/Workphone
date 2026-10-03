#ifndef _ClawUIFadeEffect_H
#define _ClawUIFadeEffect_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIFadeEffect : public ClawUIElement<IUIElement>
        {
        public:
            enum FadeState
            {
                FS_IDLE,
                FS_FADEIN,
                FS_FADEOUT,

                FS_COUNT
            };

            ClawUIFadeEffect();
            ~ClawUIFadeEffect() override;

            void update() override;

            void setMaterialName( const String &materialName );

            void setPosition( const Vector2F &position ) override;
            void setSize( const Vector2F &size ) override;

            void OnEnterState( u8 state );
            void OnUpdateState( u8 state );
            void OnLeaveState( u8 state );

            u8 GetStateFromName( const String &stateName ) const;
            String GetStateNameFromId( u8 stateId ) const;

            SmartPtr<IFSM> &getFSM();

            void draw( struct wp_context *ctx ) override;

        private:
            void OnEnterIdleState();
            void OnEnterFadeInState();
            void OnEnterFadeOutState();

            void OnUpdateIdleState();
            void OnUpdateFadeInState();
            void OnUpdateFadeOutState();

            void OnLeaveIdleState();
            void OnLeaveFadeInState();
            void OnLeaveFadeOutState();

            void handleEvent( const String &eventType );

            SmartPtr<IFSM> m_fsm;
            String m_materialName;
            FadeState m_fadeState = FS_IDLE;
            f32 m_alpha = 0.0f;
            u32 m_displayHitUntilTime = 0;
        };

        using ClawUIFadeEffectPtr = SmartPtr<ClawUIFadeEffect>;
    }  // end namespace ui
}  // namespace workphone

#endif
