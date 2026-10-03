#ifndef StateMessageMaterialName_h__
#define StateMessageMaterialName_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    class WPCore_API StateMessageMaterialName : public StateMessage
    {
    public:
        StateMessageMaterialName();
        ~StateMessageMaterialName() override;

        String getMaterialName() const;
        void setMaterialName( const String &value );

        u32 getIndex() const;
        void setIndex( u32 index );

        WP_CLASS_REGISTER_DECL;

    protected:
        String m_value;
        u32 m_index = 0;
    };

}  // namespace workphone

#endif  // StateMessageMaterialName_h__
