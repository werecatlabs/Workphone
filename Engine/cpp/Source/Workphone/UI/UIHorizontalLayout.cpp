#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIHorizontalLayout.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Scene/Directors/TextureResourceDirector.hpp>
#include <Workphone/State/States/UIImageStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIHorizontalLayout, UIElement<IUIHorizontalLayout> );

        UIHorizontalLayout::UIHorizontalLayout() = default;

        UIHorizontalLayout::~UIHorizontalLayout() = default;

    }  // namespace ui
}  // namespace workphone
