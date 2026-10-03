#ifndef UnityKeyMapping_h__
#define UnityKeyMapping_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    class UnityKeyMapping : public ISharedObject
    {
    public:
        WP_CLASS_REGISTER_DECL;

        String id;
        s32 m_unityKeycode;
        s32 m_oisKeycode;
    };

}  // namespace workphone

#endif  // UnityKeyMapping_h__
