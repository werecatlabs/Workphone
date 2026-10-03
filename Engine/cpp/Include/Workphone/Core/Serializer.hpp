#ifndef __WP_Serializer_H__
#define __WP_Serializer_H__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    /**
     * @class Serializer
     * @brief Generic class for serializing and deserializing data to and from binary stream-based files.
     *
     * This class provides a number of useful methods for exporting and importing data
     * from stream-oriented binary files (e.g., .mesh and .skeleton files). It handles
     * endianness, chunked data, and supports reading and writing of various primitive types
     * and objects such as Vector3 and Quaternion.
     */
    class WPCore_API Serializer
    {
    public:
        /**
         * @brief Constructor.
         */
        Serializer();

        /**
         * @brief Virtual destructor.
         */
        virtual ~Serializer();

    protected:
        /**
         * @brief Writes the file header to the stream.
         *
         * This method should be overridden to write any file-specific header data.
         */
        virtual void writeFileHeader( void );

        /**
         * @brief Writes a chunk header to the stream.
         * @param id The chunk ID.
         * @param size The size of the chunk in bytes.
         */
        virtual void writeChunkHeader( u16 id, u32 size );

        /**
         * @brief Writes an array of floats to the stream.
         * @param pfloat Pointer to the float array.
         * @param count Number of floats to write.
         */
        void writeFloats( const float *pfloat, u32 count );

        /**
         * @brief Writes an array of doubles to the stream.
         * @param pfloat Pointer to the double array.
         * @param count Number of doubles to write.
         */
        void writeFloats( const double *pfloat, u32 count );

        /**
         * @brief Writes an array of unsigned shorts to the stream.
         * @param pShort Pointer to the unsigned short array.
         * @param count Number of shorts to write.
         */
        void writeShorts( const u16 *pShort, u32 count );

        /**
         * @brief Writes an array of unsigned integers to the stream.
         * @param pInt Pointer to the unsigned int array.
         * @param count Number of integers to write.
         */
        void writeInts( const u32 *pInt, u32 count );

        /**
         * @brief Writes an array of booleans to the stream.
         * @param pLong Pointer to the boolean array.
         * @param count Number of booleans to write.
         */
        void writeBools( const bool *pLong, u32 count );

        /**
         * @brief Writes a Vector3 object to the stream.
         * @param vec The Vector3 object to write.
         */
        void writeObject( const Vector3<real_Num> &vec );

        /**
         * @brief Writes a Quaternion object to the stream.
         * @param q The Quaternion object to write.
         */
        void writeObject( const Quaternion<real_Num> &q );

        /**
         * @brief Writes a string to the stream.
         * @param string The string to write.
         */
        void writeString( const String &string );

        /**
         * @brief Writes raw data to the stream.
         * @param buf Pointer to the data buffer.
         * @param size Size of each element in bytes.
         * @param count Number of elements to write.
         */
        void writeData( const void *buf, u32 size, u32 count );

        /**
         * @brief Reads the file header from the stream.
         * @param stream The input stream.
         *
         * This method should be overridden to read any file-specific header data.
         */
        virtual void readFileHeader( SmartPtr<IStream> &stream );

        /**
         * @brief Reads a chunk header from the stream.
         * @param stream The input stream.
         * @return The chunk ID.
         */
        virtual unsigned short readChunk( SmartPtr<IStream> &stream );

        /**
         * @brief Reads an array of booleans from the stream.
         * @param stream The input stream.
         * @param pDest Pointer to the destination boolean array.
         * @param count Number of booleans to read.
         */
        void readBools( SmartPtr<IStream> &stream, bool *pDest, u32 count );

        /**
         * @brief Reads an array of floats from the stream.
         * @param stream The input stream.
         * @param pDest Pointer to the destination float array.
         * @param count Number of floats to read.
         */
        void readFloats( SmartPtr<IStream> &stream, float *pDest, u32 count );

        /**
         * @brief Reads an array of doubles from the stream.
         * @param stream The input stream.
         * @param pDest Pointer to the destination double array.
         * @param count Number of doubles to read.
         */
        void readFloats( SmartPtr<IStream> &stream, double *pDest, u32 count );

        /**
         * @brief Reads an array of unsigned shorts from the stream.
         * @param stream The input stream.
         * @param pDest Pointer to the destination unsigned short array.
         * @param count Number of shorts to read.
         */
        void readShorts( SmartPtr<IStream> &stream, u16 *pDest, u32 count );

        /**
         * @brief Reads an array of unsigned integers from the stream.
         * @param stream The input stream.
         * @param pDest Pointer to the destination unsigned int array.
         * @param count Number of integers to read.
         */
        void readInts( SmartPtr<IStream> &stream, u32 *pDest, u32 count );

        /**
         * @brief Reads a Vector3 object from the stream.
         * @param stream The input stream.
         * @param pDest Reference to the destination Vector3 object.
         */
        void readObject( SmartPtr<IStream> &stream, Vector3<real_Num> &pDest );

        /**
         * @brief Reads a Quaternion object from the stream.
         * @param stream The input stream.
         * @param pDest Reference to the destination Quaternion object.
         */
        void readObject( SmartPtr<IStream> &stream, Quaternion<real_Num> &pDest );

        /**
         * @brief Reads a string from the stream.
         * @param stream The input stream.
         * @return The string read from the stream.
         */
        String readString( SmartPtr<IStream> &stream );

        /**
         * @brief Reads a string of a specified length from the stream.
         * @param stream The input stream.
         * @param numChars Number of characters to read.
         * @return The string read from the stream.
         */
        String readString( SmartPtr<IStream> &stream, u32 numChars );

        /**
         * @brief Flips the byte order of the data to little endian.
         * @param pData Pointer to the data.
         * @param size Size of each element in bytes.
         * @param count Number of elements to flip (default is 1).
         */
        virtual void flipToLittleEndian( void *pData, u32 size, u32 count = 1 );

        /**
         * @brief Flips the byte order of the data from little endian to native.
         * @param pData Pointer to the data.
         * @param size Size of each element in bytes.
         * @param count Number of elements to flip (default is 1).
         */
        virtual void flipFromLittleEndian( void *pData, u32 size, u32 count = 1 );

        /**
         * @brief Flips the byte order of the data.
         * @param pData Pointer to the data.
         * @param size Size of each element in bytes.
         * @param count Number of elements to flip.
         */
        virtual void flipEndian( void *pData, u32 size, u32 count );

        /**
         * @brief Flips the byte order of the data for a single element.
         * @param pData Pointer to the data.
         * @param size Size of the element in bytes.
         */
        virtual void flipEndian( void *pData, u32 size );

        /**
         * @brief Determines the endianness of the incoming stream compared to native.
         * @param stream The input stream.
         */
        virtual void determineEndianness( SmartPtr<IStream> &stream );

        /**
         * @brief Determines the endianness to write with based on the requested option.
         * @param requestedEndian The requested endianness.
         */
        virtual void determineEndianness( Endian requestedEndian );

        /// The current stream being used for serialization.
        SmartPtr<IStream> m_stream;

        /// The length of the current stream.
        u32 m_currentStreamLen;

        /// Whether to flip endianness (true if non-native endian is required).
        bool m_flipEndian;  // default to native endian, derive from header

        /// The version string of the file format.
        String m_version;
    };

    /** @} */
    /** @} */
}  // namespace workphone

#endif
