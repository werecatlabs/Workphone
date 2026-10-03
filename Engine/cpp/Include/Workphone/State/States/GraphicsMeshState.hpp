#ifndef GraphicsMeshState_h__
#define GraphicsMeshState_h__

#include <Workphone/State/States/GraphicsObjectData.hpp>

namespace workphone
{

    class WPCore_API GraphicsMeshState : public GraphicsObjectData
    {
    public:
        GraphicsMeshState();
        ~GraphicsMeshState() override;

        ///< Flag indicating if hardware animation is enabled.
        bool hardwareAnimationEnabled = false;

        FixedString<WP_MAX_PATH> meshName;      ///< The name of the mesh.
        FixedString<WP_MAX_PATH> materialName;  ///< The main material name for the mesh.

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // GraphicsMeshState_h__
