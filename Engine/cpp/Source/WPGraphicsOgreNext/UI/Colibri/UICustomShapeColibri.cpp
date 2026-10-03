#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UICustomShapeColibri.hpp>

namespace workphone
{
    namespace ui
    {

        UICustomShapeColibri::UICustomShapeColibri( Colibri::ColibriManager *manager ) :
            CustomShape( manager )
        {
        }

        UICustomShapeColibri::~UICustomShapeColibri() = default;

        void UICustomShapeColibri::setState( Colibri::States::States state, bool smartHighlight )
        {
            Widget::setState( state, smartHighlight );
        }

    }  // namespace ui
}  // namespace workphone
