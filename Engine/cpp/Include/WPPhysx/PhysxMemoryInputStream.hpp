#ifndef PhysxMemoryInputStream_h__
#define PhysxMemoryInputStream_h__

#include "WPPhysx/WPPhysxPrerequisites.hpp"
#include "foundation/PxIO.h"
#include <PxPhysicsAPI.h>

namespace workphone
{
    namespace physics
    {

        /**
         * @file PhysxMemoryInputStream.hpp
         * @brief Memory-based implementation of physx::PxInputData for reading serialized PhysX data
         * from memory.
         */

        /**
         * @class MemoryInputData
         * @brief Implements a PhysX input stream backed by a contiguous memory buffer.
         *
         * This class wraps a raw memory buffer and exposes the `physx::PxInputData`
         * interface so PhysX serializers/loaders can read from memory instead of a file.
         * The stream keeps an internal read position and optionally manages the
         * lifetime of the provided buffer.
         */
        class MemoryInputStream : public physx::PxInputData
        {
        public:
            /**
             * @brief Default constructor. Creates an empty stream.
             *
             * The stream will contain no data and the read position is set to zero.
             */
            MemoryInputStream();

            /**
             * @brief Constructs a memory input stream that wraps a buffer.
             *
             * @param data Pointer to the buffer containing the data to read.
             *             The pointer is stored as a `const physx::PxU8*` because
             *             the stream does not modify the pointed data.
             * @param length Size of the buffer in bytes.
             * @param manageMemory If true the stream takes ownership of `data` and
             *                     will free it on destruction. If false the caller
             *                     remains responsible for the buffer's lifetime.
             */
            MemoryInputStream( physx::PxU8 *data, u32 length, bool manageMemory = true );

            /**
             * @brief Destructor. Frees the buffer if ownership was requested.
             *
             * If `m_manageMemory` is true the implementation will free the wrapped
             * buffer. Otherwise the buffer is left intact.
             */
            ~MemoryInputStream();

            /**
             * @brief Read bytes from the stream into `dest`.
             *
             * Reads up to `count` bytes from the current read position into the
             * provided destination buffer. Reading past the end of the underlying
             * buffer will result in fewer bytes being read.
             *
             * @param dest Destination buffer to receive the bytes.
             * @param count Number of bytes to attempt to read.
             * @return The number of bytes actually read.
             */
            u32 read( void *dest, u32 count ) override;

            /**
             * @brief Returns the total size of the underlying buffer.
             *
             * @return Buffer size in bytes.
             */
            u32 getLength() const override;

            /**
             * @brief Move the current read position.
             *
             * Sets the internal position to `pos`. If `pos` is larger than the
             * buffer size the position will be clamped to the buffer size.
             *
             * @param pos New read position (byte offset from the start).
             */
            void seek( u32 pos ) override;

            /**
             * @brief Returns the current read position.
             *
             * @return Current position as a byte offset from the start of the buffer.
             */
            u32 tell() const override;

        private:
            /// Total size of the wrapped buffer in bytes.
            u32 mSize;

            /**
             * Pointer to the wrapped buffer.
             *
             * Stored as `const physx::PxU8*` because this stream does not modify
             * the bytes. Ownership semantics are controlled by `m_manageMemory`.
             */
            const physx::PxU8 *mData;

            /// Current read position (byte offset from the start).
            u32 mPos;

            /**
             * If true the stream is responsible for freeing `mData` on destruction.
             * Defaults to true to preserve previous behavior; callers may explicitly
             * pass false to avoid transfer of ownership.
             */
            bool m_manageMemory = true;
        };

    } // end namespace physics
} // namespace workphone

#endif // PhysxMemoryStream_h__
