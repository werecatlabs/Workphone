#ifndef MaterialTechniqueState_h__
#define MaterialTechniqueState_h__

#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/State/States/StateData.hpp>

namespace workphone
{

    class WPCore_API MaterialTechniqueStateData : public StateData, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        MaterialTechniqueStateData();
        ~MaterialTechniqueStateData() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // MaterialTechniqueState_h__
