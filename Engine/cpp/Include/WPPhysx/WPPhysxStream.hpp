#ifndef __WPPhysxStream_h__
#define __WPPhysxStream_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include "foundation/PxIO.h"

namespace workphone
{
    namespace physics
    {
        /**
         * @file WPPhysxStream.hpp
         * @brief Adapter between the engine's IStream and PhysX's PxInputStream.
         *
         * This header declares `PhysxStream`, a small wrapper that exposes a
         * `Workphone::IStream`-based data source to PhysX serialization APIs
         * expecting a `physx::PxInputStream`.
         */

        /**
         * @class PhysxStream
         * @brief Bridges Workphone file/stream abstraction with PhysX input stream.
         *
         * @details
         * `PhysxStream` implements the `physx::PxInputStream` interface by
         * delegating reads to an underlying `SmartPtr<IStream>`. It also
         * provides convenient typed read helpers (byte/word/dword/float/double)
         * and mirrored store... helpers that return the underlying `PxInputStream&`
         * for chaining when required by callers.
         *
         * The wrapped `IStream` is stored in `m_dataStream`. It is marked
         * `mutable` because many `const` member helpers perform non-logical
         * state changes such as lazy initialization or caching of the pointer
         * for read operations; these mutations do not change the observable
         * behaviour of `PhysxStream` from the caller's perspective.
         */
        class PhysxStream : public physx::PxInputStream
        {
        public:
            /**
             * @brief Default-constructs an empty PhysxStream.
             *
             * The stream will be invalid until a valid `IStream` is provided
             * (for example via the constructor below or by other means).
             */
            PhysxStream();

            /**
             * @brief Constructs a PhysxStream that reads from the provided IStream.
             * @param ds Smart pointer to the engine's data stream to use as source.
             */
            PhysxStream( SmartPtr<IStream> ds );

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup in derived contexts and releases the
             * wrapped `IStream` smart pointer.
             */
            ~PhysxStream() override;

            /**
             * @brief Read a single unsigned byte from the underlying stream.
             * @return The byte read as `physx::PxU8`.
             */
            physx::PxU8 readByte() const;

            /**
             * @brief Read a 16-bit unsigned word from the underlying stream.
             * @return The word read as `physx::PxU16`.
             */
            physx::PxU16 readWord() const;

            /**
             * @brief Read a 32-bit unsigned double-word from the underlying stream.
             * @return The dword read as `physx::PxU32`.
             */
            physx::PxU32 readDword() const;

            /**
             * @brief Read a single-precision float from the underlying stream.
             * @return The float value read.
             */
            float readFloat() const;

            /**
             * @brief Read a double-precision float from the underlying stream.
             * @return The double value read.
             */
            double readDouble() const;

            /**
             * @brief Read raw bytes into a buffer.
             * @param buffer Destination buffer to fill.
             * @param size Number of bytes to read.
             *
             * @note The caller is responsible for ensuring `buffer` has at least
             * `size` bytes available.
             */
            void readBuffer( void *buffer, physx::PxU32 size ) const;

            /**
             * @brief Store a byte value into the stream wrapper and return reference for chaining.
             * @param b The byte to store.
             * @return Reference to the underlying `PxInputStream` (for API chaining).
             */
            PxInputStream &storeByte( physx::PxU8 b );

            /**
             * @brief Store a 16-bit word into the stream wrapper and return reference for chaining.
             * @param w The 16-bit value to store.
             * @return Reference to the underlying `PxInputStream` (for API chaining).
             */
            PxInputStream &storeWord( physx::PxU16 w );

            /**
             * @brief Store a 32-bit dword into the stream wrapper and return reference for chaining.
             * @param d The 32-bit value to store.
             * @return Reference to the underlying `PxInputStream` (for API chaining).
             */
            PxInputStream &storeDword( physx::PxU32 d );

            /**
             * @brief Store a single-precision float into the stream wrapper and return reference for
             * chaining.
             * @param f The float value to store.
             * @return Reference to the underlying `PxInputStream` (for API chaining).
             */
            PxInputStream &storeFloat( physx::PxReal f );

            /**
             * @brief Store a double-precision float into the stream wrapper and return reference for
             * chaining.
             * @param f The double value to store.
             * @return Reference to the underlying `PxInputStream` (for API chaining).
             */
            PxInputStream &storeDouble( physx::PxF64 f );

            /**
             * @brief Store raw bytes into the stream wrapper and return reference for chaining.
             * @param buffer Source buffer containing bytes to write/store.
             * @param size Number of bytes to store.
             * @return Reference to the underlying `PxInputStream` (for API chaining).
             */
            PxInputStream &storeBuffer( const void *buffer, physx::PxU32 size );

            /**
             * @brief Access the underlying engine data stream.
             * @return Smart pointer to the wrapped `IStream`.
             */
            SmartPtr<IStream> getOgreDataStream();

            /**
             * @brief PhysX input stream read implementation.
             * @param dest Destination buffer to fill.
             * @param count Number of bytes to read.
             * @return Number of bytes actually read.
             *
             * @note This overrides `physx::PxInputStream::read` and delegates to
             * the wrapped `IStream`. The function is marked `override`.
             */
            physx::PxU32 read( void *dest, physx::PxU32 count ) override;

        private:
            /**
             * @brief Underlying engine data stream backing this adapter.
             *
             * Marked `mutable` because several `const` helpers will perform
             * read operations that may change internal state (for example lazy
             * initialization or caching). Those mutations are not observable to
             * callers as logical state changes of the adapter.
             */
            mutable SmartPtr<IStream> m_dataStream;
        };
    } // end namespace physics
} // namespace workphone

#endif // __WPPhysxStream_h__
