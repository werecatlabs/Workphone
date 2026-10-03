#ifndef ISidewalk_h__
#define ISidewalk_h__

#include <Workphone/Interface/Procedural/IProceduralObject.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API ISidewalk : public IProceduralObject
        {
        public:
            ~ISidewalk() override = default;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // ISidewalk_h__
