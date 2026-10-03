#ifndef __IData_h__
#define __IData_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/RawPtr.hpp>

namespace workphone
{
    /**
     * @brief An interface for passing data.
     */
    class WPCore_API IData : public ISharedObject
    {
    public:
        /**
         * @brief Destructor.
         */
        ~IData() override;

        /**
         * @brief Set the object data.
         * @param data A pointer to the data.
         */
        virtual void setData( void *data ) = 0;

        /**
         * @brief Get the object data.
         * @return A pointer to the data.
         */
        virtual void *getData() = 0;

        /**
         * @brief Get the object data.
         * @return A pointer to the data.
         */
        virtual const void *getData() const = 0;

        /**
         * @brief Get the object data as a type.
         * @tparam B The data type.
         * @return The data as a type.
         */
        template <typename B>
        RawPtr<B> getDataAsType() const;

        WP_CLASS_REGISTER_DECL;
    };

    template <typename B>
    RawPtr<B> IData::getDataAsType() const
    {
        auto p = getData();
        auto data = const_cast<void *>( p );
        auto pData = static_cast<B *>( data );
        return RawPtr<B>( pData );
    }

}  // namespace workphone

#endif  // IData_h__
