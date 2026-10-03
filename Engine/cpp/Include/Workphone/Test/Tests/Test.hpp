#ifndef Test_h__
#define Test_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    class Test : public ISharedObject
    {
    public:
        Test();
        ~Test() override;

        virtual void run();
        virtual void report();

        bool isEnabled() const;
        void setEnabled( bool enabled );

    protected:
        bool m_isEnabled;
    };

}  // namespace workphone

#endif  // Test_h__
