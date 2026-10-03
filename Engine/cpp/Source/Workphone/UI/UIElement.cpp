#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

// Template implementations are now in the header file.
// This source file can remain for any non-template helper functions if needed in the future.

template class WPCore_API workphone::core::Prototype<workphone::ui::IUIButton>;
template class WPCore_API workphone::core::Prototype<workphone::ui::IUIDropdown>;
template class WPCore_API workphone::core::Prototype<workphone::ui::IUIImage>;
template class WPCore_API workphone::core::Prototype<workphone::ui::IUIToggle>;
template class WPCore_API workphone::core::Prototype<workphone::ui::IUIWindow>;
template class WPCore_API workphone::core::Prototype<workphone::ui::IUIText>;

template class WPCore_API workphone::ui::UIElement<workphone::ui::IUIToggle>;
