#ifndef StateMessageFragmentParam_h__
#define StateMessageFragmentParam_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/ColourUtil.hpp>

namespace workphone
{

    class WPCore_API StateMessageFragmentParam : public StateMessage
    {
    public:
        StateMessageFragmentParam();
        ~StateMessageFragmentParam() override;

        String getName() const override;
        void setName( const String &value ) override;

        f32 getFloat() const;
        void setFloat( f32 value );

        Vector2<real_Num> getVector2f() const;
        void setVector2f( const Vector2<real_Num> &value );

        Vector3<real_Num> getVector3f() const;
        void setVector3f( const Vector3<real_Num> &value );

        Vector4F getVector4f() const;
        void setVector4f( const Vector4F &value );

        ColourF getColourf() const;
        void setColourf( const ColourF &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        String m_name;

        union Data
        {
            s32 iData[4];
            f32 fData[4];
        };

        Data m_data;
    };
}  // namespace workphone

#endif  // StateMessageFragmentParam_h__
