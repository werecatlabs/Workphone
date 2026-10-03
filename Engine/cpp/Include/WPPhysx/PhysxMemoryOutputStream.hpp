#ifndef MemoryOutputStream_h__
#define MemoryOutputStream_h__

#include "WPPhysx/WPPhysxPrerequisites.hpp"
#include "foundation/PxIO.h"
#include <PxPhysicsAPI.h>

namespace workphone
{
    namespace physics
    {

        /**
         * @class MemoryOutputStream
         * @brief Simple in-memory implementation of PhysX's PxOutputStream.
         *
         * This class collects bytes written via the `write` method into an
         * internally managed buffer and exposes accessors for the buffer
         * pointer and current size. It is intended for short-lived use where
         * serialized PhysX data or other binary blobs need to be captured in
         * memory.
         *
         * Thread-safety: Not thread-safe. Caller must synchronize external access.
         */
        class MemoryOutputStream : public physx::PxOutputStream
        {
        public:
            /**
             * @brief Construct a new MemoryOutputStream.
             *
             * Initializes internal buffer state. No memory is allocated until
             * the first write (implementation-dependent).
             */
            MemoryOutputStream();

            /**
             * @brief Destroy the MemoryOutputStream.
             *
             * Releases any internally allocated buffer memory.
             */
            ~MemoryOutputStream() override;

            /**
             * @brief Writes bytes into the internal buffer.
             *
             * This overrides physx::PxOutputStream::write.
             *
             * @param src Pointer to the source bytes to write.
             * @param count Number of bytes to write from `src`.
             * @return u32 The number of bytes actually written (should equal `count`
             *             on success).
             */
            u32 write( const void *src, u32 count ) override;

            /**
             * @brief Get the number of bytes written to the stream so far.
             *
             * @return u32 Current size (in bytes) of valid data in the buffer.
             */
            u32 getSize() const;

            /**
             * @brief Get a pointer to the internal data buffer.
             *
             * The pointer is owned by this object. The caller must not free it.
             * The data remains valid until the stream is destroyed or further
             * non-const modifying operations (such as additional writes) invalidate it.
             *
             * @return physx::PxU8* Pointer to the buffer containing written data,
             *                       or nullptr if no data has been written.
             */
            physx::PxU8 *getData() const;

        private:
            /**
             * @brief Pointer to the allocated buffer holding written bytes.
             *
             * Ownership: this class is responsible for allocation and deallocation.
             */
            physx::PxU8 *mData;

            /**
             * @brief Number of valid bytes currently stored in `mData`.
             */
            u32 mSize;

            /**
             * @brief Total capacity (in bytes) of the allocated `mData` buffer.
             *
             * mCapacity >= mSize at all times.
             */
            u32 mCapacity;
        };

    } // end namespace physics
} // namespace workphone

#endif // MemoryOutputStream_h__
