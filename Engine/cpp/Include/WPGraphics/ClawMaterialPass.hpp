#ifndef ClawMaterialPass_h__
#define ClawMaterialPass_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>

namespace workphone::render
{
    /** Material pass that installs its state in the Claw material manager context. */
    class WPGraphics_API ClawMaterialPass : public MaterialPass
    {
    public:
        ClawMaterialPass();
        ~ClawMaterialPass() override;

        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

        bool handleStateChanged( SmartPtr<IState> &state ) override;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::render

#endif  // ClawMaterialPass_h__
