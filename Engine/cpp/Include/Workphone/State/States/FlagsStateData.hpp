#ifndef FlagsState_h__
#define FlagsState_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{
    class FlagsStateData : public StateData
    {
    public:
        FlagsStateData();
        ~FlagsStateData() override;

        u32 getFlags() const;

        void setFlags( u32 flags );

        void setFlag( u32 flag, bool value );

        bool getFlag( u32 flag ) const;

        WP_CLASS_REGISTER_DECL;

        atomic_u32 m_flags = 0;
    };
}  // namespace workphone

#endif  // FlagsState_h__
