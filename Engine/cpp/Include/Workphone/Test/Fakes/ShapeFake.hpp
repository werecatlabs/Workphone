#ifndef ShapeFake_h__
#define ShapeFake_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace test
    {

        class ShapeFake : public ISharedObject
        {
        public:
            ShapeFake();
            ~ShapeFake() override;

            void func();
            virtual void virtualfunc() const;
        };

    }  // end namespace test
}  // namespace workphone

#endif  // ShapeFake_h__
