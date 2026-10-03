#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageFragmentParam.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageFragmentParam, StateMessage );

    //--------------------------------------------
    StateMessageFragmentParam::StateMessageFragmentParam() = default;

    //--------------------------------------------
    StateMessageFragmentParam::~StateMessageFragmentParam() = default;

    auto StateMessageFragmentParam::getName() const -> String
    {
        return m_name;
    }

    void StateMessageFragmentParam::setName( const String &value )
    {
        m_name = value;
    }

    //--------------------------------------------
    auto StateMessageFragmentParam::getFloat() const -> f32
    {
        return m_data.fData[0];
    }

    //--------------------------------------------
    void StateMessageFragmentParam::setFloat( f32 value )
    {
        m_data.fData[0] = value;
    }

    //--------------------------------------------
    auto StateMessageFragmentParam::getVector2f() const -> Vector2<real_Num>
    {
        return { m_data.fData[0], m_data.fData[1] };
    }

    //--------------------------------------------
    void StateMessageFragmentParam::setVector2f( const Vector2<real_Num> &value )
    {
        m_data.fData[0] = value[0];
        m_data.fData[1] = value[1];
    }

    //--------------------------------------------
    auto StateMessageFragmentParam::getVector3f() const -> Vector3<real_Num>
    {
        return { m_data.fData[0], m_data.fData[1], m_data.fData[2] };
    }

    //--------------------------------------------
    void StateMessageFragmentParam::setVector3f( const Vector3<real_Num> &value )
    {
        m_data.fData[0] = value[0];
        m_data.fData[1] = value[1];
        m_data.fData[2] = value[2];
    }

    //--------------------------------------------
    auto StateMessageFragmentParam::getVector4f() const -> Vector4F
    {
        return { m_data.fData[0], m_data.fData[1], m_data.fData[2], m_data.fData[3] };
    }

    //--------------------------------------------
    void StateMessageFragmentParam::setVector4f( const Vector4F &value )
    {
        m_data.fData[0] = value[0];
        m_data.fData[1] = value[1];
        m_data.fData[2] = value[2];
        m_data.fData[3] = value[3];
    }

    //--------------------------------------------
    auto StateMessageFragmentParam::getColourf() const -> ColourF
    {
        return { m_data.fData[0], m_data.fData[1], m_data.fData[2], m_data.fData[3] };
    }

    //--------------------------------------------
    void StateMessageFragmentParam::setColourf( const ColourF &value )
    {
        m_data.fData[0] = value.r;
        m_data.fData[1] = value.g;
        m_data.fData[2] = value.b;
        m_data.fData[3] = value.a;
    }
}  // namespace workphone
