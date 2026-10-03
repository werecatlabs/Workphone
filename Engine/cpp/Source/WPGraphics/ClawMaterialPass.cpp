#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawMaterialPass.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMaterialPass, MaterialPass );

    ClawMaterialPass::ClawMaterialPass()
    {
        // MaterialPass already creates the state record in its own context.
    }

    bool ClawMaterialPass::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    bool ClawMaterialPass::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    ClawMaterialPass::~ClawMaterialPass() = default;
}  // namespace workphone::render
