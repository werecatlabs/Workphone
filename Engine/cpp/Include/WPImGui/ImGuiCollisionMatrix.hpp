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
        enum class LayerEdit { None, Add, Rename };

        bool drawLayerList();
        bool drawMatrix();
        bool commitLayerName();
        void removeSelectedLayer();
        static void drawLayerName( const String &layer );

        Array<String> m_layers;
        std::set<StringPair> m_disabledPairs;
        String m_layersText;
        String m_disabledPairsText;
        String m_layerName;
        String m_layerError;
        int m_selectedLayer = -1;
        LayerEdit m_layerEdit = LayerEdit::None;
        bool m_scrollToSelectedLayer = false;
        bool m_validPairs = true;
        bool m_editCollisionPairs = true;
    };
}  // namespace workphone::ui

#endif  // ImGuiCollisionMatrix_h__
