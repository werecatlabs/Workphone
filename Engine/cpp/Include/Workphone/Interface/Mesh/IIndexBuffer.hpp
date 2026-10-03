#ifndef IIndexBuffer_h__
#define IIndexBuffer_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * An interface for an index buffer.
     */
    class WPCore_API IIndexBuffer : public ISharedObject
    {
    public:
        /**
         * The type of the index buffer.
         */
        enum class Type
        {
            IT_16BIT, /**< A 16-bit index buffer. */
            IT_32BIT  /**< A 32-bit index buffer. */
        };

        IIndexBuffer();

        /**
         * Virtual destructor.
         */
        ~IIndexBuffer() override;

        /**
         * Sets the type of the index buffer.
         * @param indexType The type of the index buffer.
         */
        virtual void setIndexType( Type indexType ) = 0;

        /**
         * Gets the type of the index buffer.
         * @return The type of the index buffer.
         */
        virtual Type getIndexType() const = 0;

        /**
         * Sets the number of indices in the buffer.
         * @param numIndices The number of indices in the buffer.
         */
        virtual void setNumIndices( u32 numIndices ) = 0;

        /**
         * Gets the number of indices in the buffer.
         * @return The number of indices in the buffer.
         */
        virtual u32 getNumIndices() const = 0;

        /**
         * Gets the size of an index in bytes.
         * @return The size of an index in bytes.
         */
        virtual u32 getIndexSize() const = 0;

        /**
         * Sets the size of an index in bytes.
         * @param size The size of an index in bytes.
         */
        virtual void setIndexSize( u32 size ) = 0;

        /**
         * Creates the index data buffer and returns a pointer to it.
         * @return A pointer to the index data buffer.
         */
        virtual void *createIndexData() = 0;

        /**
         * Gets a pointer to the index data buffer.
         * @return A pointer to the index data buffer.
         */
        virtual void *getIndexData() const = 0;

        /**
         * Clones the index buffer.
         * @return A shared pointer to a new index buffer that is a copy of the original.
         */
        virtual SmartPtr<IIndexBuffer> clone() const = 0;

        /**
         * Compares this index buffer with another index buffer.
         * @param other A shared pointer to the index buffer to compare to.
         * @return True if the index buffers are equal, false otherwise.
         */
        virtual bool compare( SmartPtr<IIndexBuffer> other ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IIndexBuffer_h__
