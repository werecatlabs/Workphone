#ifndef StateMessageDefault_h__
#define StateMessageDefault_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include "Workphone/Core/Parameter.hpp"
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{

    class WPCore_API StateMessageDefault : public StateMessage
    {
    public:
        StateMessageDefault();
        ~StateMessageDefault() override;

        s32 setProperty( hash_type hash, const String &value );
        s32 getProperty( hash_type hash, String &value ) const;

        s32 setProperty( hash_type hash, const Parameter &param );
        s32 setProperty( hash_type hash, const Parameters &params );
        s32 getProperty( hash_type hash, Parameter &param ) const;
        s32 getProperty( hash_type hash, Parameters &params ) const;

        void setSubjectId( u32 id );
        u32 getSubjectId() const;

        WP_CLASS_REGISTER_DECL;

    private:
        using StringMap = std::map<hash_type, String>;
        StringMap m_stringMap;

        using WPParamMap = std::map<hash_type, Parameter>;
        WPParamMap m_paramMap;

        using WPParamListMap = std::map<hash_type, Parameters>;
        WPParamListMap m_paramListMap;

        u32 m_subjectId;
    };
}  // namespace workphone

#endif  // StateMessageDefault_h__
