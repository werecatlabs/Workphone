#ifndef JsonWriter_h__
#define JsonWriter_h__

#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/JsonValue.hpp>
#include <cstddef>

namespace workphone
{

    /**
     * @brief Abstract output target for JsonWriter.
     *
     * Subclasses implement the low-level write operation used by
     * JsonWriter to emit JSON text. This abstraction allows writing
     * to different sinks (in-memory string, file, network, etc.).
     */
    struct WPCore_API JsonOutput
    {
        /**
         * @brief Write raw bytes to the output.
         * @param data Pointer to the bytes to write.
         * @param size Number of bytes to write.
         */
        virtual void write( const char *data, size_t size ) = 0;

        /**
         * @brief Virtual destructor for proper cleanup by subclasses.
         */
        virtual ~JsonOutput();
    };

    /**
     * @brief Simple in-memory JsonOutput implementation that appends
     * written bytes into a String buffer.
     */
    struct WPCore_API StringOutput : JsonOutput
    {
        /**
         * @brief Buffer that receives written JSON text.
         */
        String buffer;

        /**
         * @brief Construct a StringOutput with an initially empty buffer.
         */
        StringOutput();

        /**
         * @brief Append data to the internal buffer.
         * @param data Pointer to the bytes to append.
         * @param size Number of bytes to append.
         */
        void write( const char *data, size_t size ) override;
    };

    /**
     * @brief Streaming JSON writer that emits JSON tokens to a JsonOutput.
     *
     * This writer provides an imperative API for constructing JSON values:
     * - beginObject()/endObject() to create objects
     * - beginArray()/endArray() to create arrays
     * - key(...) to emit a member name inside an object
     * - null()/boolean()/number()/string() to emit primitive values
     *
     * The writer keeps an internal context stack to manage commas and
     * nesting. It validates JSON structure as it writes, rejects invalid
     * non-finite numbers, and validates emitted strings as UTF-8.
     */
    class WPCore_API JsonWriter
    {
    public:
        /**
         * @brief Construct a JsonWriter that emits to the given output.
         * @param out Reference to a JsonOutput implementation.
         */
        JsonWriter( JsonOutput &out );

        /**
         * @brief Start a JSON object (emits '{').
         *
         * After calling this, call `key(...)` for each member name followed
         * by a value-emitting call (e.g. `string`, `number`, `beginArray`, ...).
         */
        void beginObject();

        /**
         * @brief End the current JSON object (emits '}').
         */
        void endObject();

        /**
         * @brief Start a JSON array (emits '[').
         */
        void beginArray();

        /**
         * @brief End the current JSON array (emits ']').
         */
        void endArray();

        /**
         * @brief Emit an object member name (string key).
         * @param k Member name to write. Must be followed by a value.
         */
        void key( const String &k );

        /**
         * @brief Emit a JSON null literal.
         */
        void null();

        /**
         * @brief Emit a JSON boolean literal.
         * @param v Value to write (true or false).
         */
        void boolean( bool v );

        /**
         * @brief Emit a JSON number.
         * @param v Floating-point value to write.
         */
        void number( f64 v );

        /**
         * @brief Emit a JSON string (with proper escaping).
         * @param s String value to write.
         */
        void string( const String &s );

        /**
         * @brief Write a quoted JSON string including surrounding quotes and escapes.
         *
         * This is a low-level helper used when callers need a quoted string
         * fragment, for example in formatting utilities. It does not advance
         * the writer's JSON document state. Use string(...) to emit a JSON
         * string value as part of a document.
         *
         * @param s The string to serialize.
         */
        void writeString( const String &s );

        /**
         * @brief Convenience to write a null-terminated C string to the output.
         * @param s Null-terminated string to write.
         */
        void write( const c8 *s );

        /**
         * @brief Write any required prefix for a value.
         *
         * This will ensure commas and the transition from a key to its value
         * are handled correctly.
         */
        void writeValuePrefix();

        /**
         * @brief Write a comma if the current container already has elements.
         */
        void writeCommaIfNeeded();

        /**
         * @brief Validate that exactly one complete JSON root value has been emitted.
         *
         * @throws std::runtime_error if the document is empty, has unclosed
         * containers, or an object key is missing its value.
         */
        void finish() const;

        /**
         * @brief Return true when the writer has emitted a complete JSON document.
         */
        bool isComplete() const;

        /**
         * @brief Return true if any structural or validation errors were encountered during writing.
         */
        bool hasError() const;

    private:
        /**
         * @brief Output sink where JSON text is written.
         */
        JsonOutput &out;

        /**
         * @brief Type of the current container context.
         */
        enum class ContextType
        {
            Object,
            Array
        };

        /**
         * @brief Context information pushed for each open object/array.
         *
         * `first` is true while the container has no elements yet and is
         * used to control comma insertion between elements.
         */
        struct Context
        {
            ContextType type;
            bool first;
        };

        /**
         * @brief Stack of open container contexts (objects and arrays).
         */
        Array<Context> stack;

        /**
         * @brief True when the writer has just emitted a key and is expecting
         * a value next. This helps enforce correct key->value sequencing.
         */
        bool expectingValue = false;

        /**
         * @brief True after the single root value has been started.
         */
        bool rootValueWritten = false;

        /**
         * @brief Sticky flag indicating that one or more errors occurred during the JSON emission
         * process.
         */
        bool m_hasError = false;
    };

}  // namespace workphone

#endif  // JsonWriter_h__
