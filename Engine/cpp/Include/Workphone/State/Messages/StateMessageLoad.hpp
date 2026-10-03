#ifndef StateMessageLoad_h__
#define StateMessageLoad_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{
    class WPCore_API StateMessageLoad : public StateMessage
    {
    public:
        static const hash_type LOAD_HASH;
        static const hash_type RELOAD_HASH;

        static const hash_type LOADED_HASH;
        static const hash_type UNLOADED_HASH;

        StateMessageLoad();
        StateMessageLoad( const StateMessageLoad &other ) = delete;
        ~StateMessageLoad() override;

        SmartPtr<ISharedObject> getObject() const;
        void setObject( SmartPtr<ISharedObject> object );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The object.
        SmartPtr<ISharedObject> m_object;
    };
}  // namespace workphone

#endif  // StateMessageLoad_h__
