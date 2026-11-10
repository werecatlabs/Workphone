#ifndef ImGuiCollisionMatrix_h__
#define ImGuiCollisionMatrix_h__

#include <WPImGui/ImGuiElement.hpp>
#include <set>

namespace workphone::ui
{
    class ImGuiCollisionMatrix final : public ImGuiElement<IUIElement>
    {
    public:
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        void update() override;

        WP_CLASS_REGISTER_DECL;

    private:
        bool drawMatrix();
        static void drawLayerName( const String &layer );

        Array<String> m_layers;
        std::set<StringPair> m_disabledPairs;
        String m_disabledPairsText;
        bool m_validPairs = true;
    };
}  // namespace workphone::ui

#endif  // ImGuiCollisionMatrix_h__
