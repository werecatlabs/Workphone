#ifndef __CData_h__
#define __CData_h__

#include <Workphone/Interface/Memory/IData.hpp>

namespace workphone
{

    template <class T>
    class Data : public IData
    {
    public:
        Data()
        {
        }

        Data( const T &data )
        {
            m_data = data;
        }

        ~Data() override = default;

        void setData( void *data ) override
        {
            m_data = *static_cast<T *>( data );
        }

        void *getData() override
        {
            return static_cast<void *>( &m_data );
        }

        const void *getData() const override
        {
            auto p = const_cast<T *>( &m_data );
            return static_cast<void *>( p );
        }

    private:
        T m_data;
    };
}  // namespace workphone

#endif  // CData_h__
