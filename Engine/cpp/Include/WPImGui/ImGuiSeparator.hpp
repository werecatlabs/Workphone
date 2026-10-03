#pragma once

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUISeparator.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiSeparator : public ImGuiElement<IUISeparator>
        {
        public:
            ImGuiSeparator();
            ~ImGuiSeparator() override;

            void update() override;

            void setHorizontal( bool horizontal ) override;

            bool isHorizontal() const override;

            void setThickness( f32 thickness ) override;

            f32 getThickness() const override;

            void setMargin( f32 margin ) override;

            f32 getMargin() const override;

            void setPosition( const Vector2<real_Num> &position ) override;

            void setSize( const Vector2<real_Num> &size ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace ui
}  // namespace workphone
