#pragma once

#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/JsonValue.hpp>
#include <cassert>
#include <cctype>
#include <stdexcept>

/**
 * @file JsonParser.hpp
 * @brief Lightweight JSON parser used by the Workphone engine.
 *
 * The parser provides simple, hand-written parsing of JSON text into
 * the project's JsonValue/JsonObject/JsonArray types. It is intended
 * for small configuration blobs and embedded use where a dependency on
 * a heavy-weight JSON library is undesirable.
 */

namespace workphone
{

    /**
     * @class JsonParser
     * @brief Parses a JSON text buffer into JsonValue trees.
     *
     * The parser holds a pointer to the JSON text and parses it on demand.
     * It does not take ownership of the provided text pointer; the caller
     * must ensure the buffer remains valid for the lifetime of the parser.
     */
    class WPCore_API JsonParser : public ISharedObject
    {
    public:
        /**
         * @brief Construct a parser for the given text buffer.
         *
         * @param text Pointer to a UTF-8 (or ASCII) encoded JSON text buffer.
         * @param size Number of bytes available in the text buffer.
         */
        JsonParser( const c8 *text, u32 size );

        /**
         * @brief Parse the top-level JSON as properties and append to @p properties.
         *
         * This is a convenience helper that parses an object at the root and
         * converts it to the project's Properties container. Throws on parse errors.
         *
         * @param properties Destination properties object to populate.
         */
        void parseToProperties( Properties *properties );

        /**
         * @brief Parse the JSON buffer and return the resulting JsonValue tree.
         *
         * @return Root JsonValue representing the parsed JSON.
         * @throws std::runtime_error on malformed input.
         */
        JsonValue parse();

        /**
         * @brief Peek at the next character without advancing the parser position.
         *
         * @return Next character or '\0' if at end of buffer.
         */
        c8 peek() const;

        /**
         * @brief Get the next character and advance the parser position.
         *
         * @return Next character or '\0' if at end of buffer.
         */
        c8 get();

        /**
         * @brief Skip any whitespace characters from the current position.
         */
        void skipWhitespace();

        /**
         * @brief Expect a specific character at the current position and consume it.
         *
         * Throws std::runtime_error when the expected character is not found.
         *
         * @param c Character that must be present.
         */
        void expect( c8 c );

        /**
         * @brief Parse a JSON value (object, array, string, number, true, false, null).
         *
         * @return Parsed JsonValue.
         */
        JsonValue parseValue();

        /**
         * @brief Parse a JSON object (dictionary) starting at the current position.
         *
         * @return JsonObject containing parsed key/value pairs.
         */
        JsonObject parseObject();

        /**
         * @brief Parse a JSON array starting at the current position.
         *
         * @return JsonArray containing parsed elements.
         */
        JsonArray parseArray();

        /**
         * @brief Parse a JSON string (handles escape sequences) and return it.
         *
         * @return Parsed string as a project String type.
         */
        String parseString();

        /**
         * @brief Parse the literal value "true" and return a JsonValue of boolean true.
         *
         * @return JsonValue representing true.
         */
        JsonValue parseTrue();

        /**
         * @brief Parse the literal value "false" and return a JsonValue of boolean false.
         *
         * @return JsonValue representing false.
         */
        JsonValue parseFalse();

        /**
         * @brief Parse the literal value "null" and return a JsonValue representing null.
         *
         * @return JsonValue representing null.
         */
        JsonValue parseNull();

        /**
         * @brief Parse a JSON number starting at the current position.
         *
         * Handles integer and floating point formats and returns the value as double.
         *
         * @return Parsed numeric value as f64.
         */
        f64 parseNumber();

    private:
        /**
         * @brief Check parser construction/state invariants.
         */
        bool isValidState() const;

        /**
         * @brief True when the parser has consumed the whole buffer.
         */
        bool isAtEnd() const;

        /**
         * @brief True when @p count bytes can be read from the current offset.
         */
        bool hasRemaining( u32 count ) const;

        /**
         * @brief Assert and throw if the parser state is unusable.
         */
        void assertValidState() const;

        /**
         * Pointer to the text buffer being parsed.
         * Note: parser does not own this memory.
         */
        const c8 *m_text = nullptr;

        /**
         * Size of the buffer in bytes.
         */
        u32 m_size = 0;

        /**
         * Current parser byte offset into the buffer.
         */
        u32 m_pos = 0;
    };

    inline bool JsonParser::isValidState() const
    {
        return ( m_text != nullptr || m_size == 0 ) && m_pos <= m_size;
    }

    inline bool JsonParser::isAtEnd() const
    {
        assert( isValidState() );
        if( !isValidState() )
        {
            return true;
        }

        return m_pos >= m_size;
    }

    inline bool JsonParser::hasRemaining( u32 count ) const
    {
        assert( isValidState() );
        if( !isValidState() )
        {
            return false;
        }

        return count <= ( m_size - m_pos );
    }

    inline void JsonParser::assertValidState() const
    {
        assert( m_text != nullptr || m_size == 0 );
        assert( m_pos <= m_size );

        if( !isValidState() )
        {
            throw std::logic_error( "JsonParser is in an invalid state" );
        }
    }

}  // namespace workphone
