#ifndef StateMessageObjectsArray_h__
#define StateMessageObjectsArray_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageObjectsArray : public StateMessage
    {
    public:
        StateMessageObjectsArray();
        ~StateMessageObjectsArray() override;

        Array<SmartPtr<ISharedObject>> getObjects() const;
        void setObjects( const Array<SmartPtr<ISharedObject>> &objects );

        WP_CLASS_REGISTER_DECL;

    protected:
        Array<SmartPtr<ISharedObject>> m_objects;
    };
}  // namespace workphone

#endif  // StateMessageObjectsArray_h__
