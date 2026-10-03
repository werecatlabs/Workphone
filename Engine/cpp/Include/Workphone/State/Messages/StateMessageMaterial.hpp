#ifndef StateMessageMaterial_h__
#define StateMessageMaterial_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    class WPCore_API StateMessageMaterial : public StateMessage
    {
    public:
        StateMessageMaterial() = default;
        ~StateMessageMaterial() override = default;

        SmartPtr<render::IMaterial> getMaterial() const;
        void setMaterial( SmartPtr<render::IMaterial> material );

        s32 getIndex() const;
        void setIndex( s32 index );

        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<render::IMaterial> m_material;
        s32 m_index = -1;
    };

}  // namespace workphone

#endif  // StateMessageMaterial_h__
