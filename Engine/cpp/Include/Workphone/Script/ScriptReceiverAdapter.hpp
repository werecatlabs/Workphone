#ifndef ScriptReceiverAdapter_h__
#define ScriptReceiverAdapter_h__

#include <Workphone/Interface/Script/IScriptReceiver.hpp>

namespace workphone
{
    template <class T>
    class ScriptReceiverAdapter : public IScriptReceiver
    {
    public:
        explicit ScriptReceiverAdapter( T *listener ) : m_listener( listener )
        {
        }

        s32 setProperty( hash_type hash, const String &value ) override
        {
            return m_listener->setProperty( hash, value );
        }

        s32 getProperty( hash_type hash, String &value ) const override
        {
            return m_listener->getProperty( hash, value );
        }

        s32 setProperty( hash_type hash, const Parameter &param ) override
        {
            return m_listener->setProperty( hash, param );
        }

        s32 setProperty( hash_type hash, const Parameters &params ) override
        {
            return m_listener->setProperty( hash, params );
        }

        s32 setProperty( hash_type hash, void *param ) override
        {
            return m_listener->setProperty( hash, param );
        }

        s32 getProperty( hash_type hash, Parameter &param ) const override
        {
            return m_listener->getProperty( hash, param );
        }

        s32 getProperty( hash_type hash, Parameters &params ) const override
        {
            return m_listener->getProperty( hash, params );
        }

        s32 getProperty( hash_type hash, void *param ) const override
        {
            return m_listener->getProperty( hash, param );
        }

        s32 callFunction( u32 hash, const Parameters &params, Parameters &results )
        {
            return m_listener->callFunction( hash, params, results );
        }

        s32 callFunction( u32 hash, SmartPtr<ISharedObject> object, Parameters &results )
        {
            return m_listener->callFunction( hash, object, results );
        }

    private:
        T *m_listener = nullptr;
    };
}  // namespace workphone

#endif  // ScriptReceiverAdapter_h__
