#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawOverlayElementVector.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawOverlayElementVector, IOverlayElementVector );

    ClawOverlayElementVector::ClawOverlayElementVector() = default;
    ClawOverlayElementVector::~ClawOverlayElementVector() = default;

    String ClawOverlayElementVector::getFileName() const
    {
        return m_fileName;
    }

    void ClawOverlayElementVector::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }
}  // namespace workphone::render
