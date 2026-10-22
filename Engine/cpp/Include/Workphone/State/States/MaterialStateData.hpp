#ifndef __MaterialStateData_h__
#define __MaterialStateData_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{
    // Root lifecycle/type marker retained by the material's existing context.
    // Surface/texture values belong to MaterialPassStateData; resource identity
    // and paths belong to Material. Do not introduce a second value owner here.
    class WPCore_API MaterialStateData : public StateData
    {
    public:
        MaterialStateData();
        ~MaterialStateData() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // __MaterialStateData_h__
