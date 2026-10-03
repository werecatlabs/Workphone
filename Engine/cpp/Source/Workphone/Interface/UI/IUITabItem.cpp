#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUITabItem.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUITabItem, IUIElement );

    auto IUITabItem::getLabel() const -> String
    {
        return m_label;
    }

    void IUITabItem::setLabel( const String &label )
    {
        m_label = label;
    }

    IUITabItem::~IUITabItem() = default;

}  // namespace workphone::ui
