#ifndef __FBIndexBuffer__H
#define __FBIndexBuffer__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>

namespace workphone
{
    /**
     * @brief Implementation of an index buffer for storing mesh indices.
     *
     * IndexBuffer provides a concrete implementation of the IIndexBuffer interface
     * for managing vertex indices in mesh data. It supports different index types
     * (16-bit and 32-bit) and handles memory management for index data storage.
     *
     * The index buffer is used in 3D rendering to define the order in which
     * vertices should be connected to form triangles or other primitives.
     *
     * @see IIndexBuffer
     * @author Workphone Engine Team
     */
    class WPCore_API IndexBuffer : public IIndexBuffer
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes an empty index buffer with default values:
         * - Index type: IT_32BIT
         * - Number of indices: 0
         * - Index size: 0
         * - Index data: nullptr
         */
        IndexBuffer();

        /**
         * @brief Destructor.
         *
         * Cleans up allocated index data and releases any resources.
         */
        ~IndexBuffer() override;

        /**
         * @brief Unloads the index buffer and releases associated resources.
         * @param data Shared object data for cleanup coordination
         * @copydoc IIndexBuffer::unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Sets the type of indices stored in this buffer.
         * @param indexType The index type (16-bit or 32-bit)
         * @copydoc IIndexBuffer::setIndexType
         * @see Type
         */
        void setIndexType( Type indexType ) override;

        /**
         * @brief Gets the current index type.
         * @return The current index type (16-bit or 32-bit)
         * @copydoc IIndexBuffer::getIndexType
         * @see Type
         */
        Type getIndexType() const override;

        /**
         * @brief Sets the number of indices in the buffer.
         * @param numIndices The number of indices to store
         * @note This does not allocate memory. Call createIndexData() to allocate storage.
         */
        void setNumIndices( u32 numIndices ) override;

        /**
         * @brief Gets the number of indices in the buffer.
         * @return The number of indices currently stored
         */
        u32 getNumIndices() const override;

        /**
         * @brief Gets the size in bytes of each individual index.
         * @return Size in bytes per index (typically 2 for 16-bit, 4 for 32-bit)
         */
        u32 getIndexSize() const override;

        /**
         * @brief Sets the size in bytes of each individual index.
         * @param size Size in bytes per index
         * @note Should match the index type (2 for 16-bit, 4 for 32-bit)
         */
        void setIndexSize( u32 size ) override;

        /**
         * @brief Creates and allocates memory for index data storage.
         * @return Pointer to the allocated index data buffer
         * @note The returned pointer should be cast to the appropriate type based on getIndexType()
         * @warning The caller is responsible for not exceeding the allocated buffer size
         */
        void *createIndexData() override;

        /**
         * @brief Gets a pointer to the current index data.
         * @return Const pointer to the index data buffer, or nullptr if not allocated
         * @note The returned pointer should be cast to the appropriate type based on getIndexType()
         */
        void *getIndexData() const override;

        /**
         * @brief Creates a deep copy of this index buffer.
         * @return Smart pointer to a new IndexBuffer instance with copied data
         * @note The cloned buffer will have its own independent copy of the index data
         */
        SmartPtr<IIndexBuffer> clone() const override;

        /**
         * @brief Compares this index buffer with another for equality.
         * @param other The index buffer to compare against
         * @return true if buffers are identical (same type, size, and data), false otherwise
         * @note Performs deep comparison including index data content
         */
        bool compare( SmartPtr<IIndexBuffer> other ) const override;

    protected:
        /** @brief The type of indices stored (16-bit or 32-bit). */
        Type m_indexType = Type::IT_32BIT;

        /** @brief The total number of indices in the buffer. */
        u32 m_numIndices = 0;

        /** @brief The size in bytes of each individual index. */
        u32 m_indexSize = 0;

        /** @brief Pointer to the raw index data storage. */
        void *m_indexData = nullptr;
    };

}  // namespace workphone

#endif
