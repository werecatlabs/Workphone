#ifndef WPSQLiteQuery_h__
#define WPSQLiteQuery_h__

#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/Core/Array.hpp>
#include <WPSQLite/extern/CppSQLite3.hpp>

namespace workphone
{
    /**
     * @class SQLiteQuery
     * @brief A class that implements IDatabaseQuery interface for SQLite database operations
     *
     * This class provides functionality to handle SQLite query results, including field access,
     * row navigation, and data type conversions. It wraps the CppSQLite3Query functionality
     * and provides a more convenient interface for working with query results.
     */
    class SQLiteQuery : public IDatabaseQuery
    {
    public:
        /**
         * @brief Default constructor
         */
        SQLiteQuery();

        /**
         * @brief Constructor that takes a CppSQLite3Query object
         * @param query The SQLite query object to wrap
         */
        SQLiteQuery( CppSQLite3Query &query );

        /**
         * @brief Destructor
         */
        ~SQLiteQuery() override;

        /**
         * @brief Sets the internal query object
         * @param query The SQLite query object to wrap
         */
        void setQuery( CppSQLite3Query &query );

        /**
         * @brief Gets the name of a field at the specified index
         * @param index The zero-based index of the field
         * @return The name of the field as a String
         */
        String getFieldName( u32 index ) override;

        /**
         * @brief Gets the value of a field at the specified index
         * @param index The zero-based index of the field
         * @return The value of the field as a String
         */
        String getFieldValue( u32 index ) override;

        /**
         * @brief Gets the value of a field by its name
         * @param field The name of the field
         * @return The value of the field as a String
         */
        String getFieldValue( const String &field ) override;

        /**
         * @brief Gets the value of a field as an integer
         * @param fieldName The name of the field
         * @return The value of the field as a signed 32-bit integer
         */
        s32 getFieldValueAsInt( const String &fieldName );

        /**
         * @brief Gets the value of a field as a float
         * @param fieldName The name of the field
         * @return The value of the field as a 32-bit float
         */
        f32 getFieldValueAsFloat( const String &fieldName );

        /**
         * @brief Gets the number of fields in the current row
         * @return The number of fields as size_t
         */
        size_t getNumFields() const override;

        /**
         * @brief Checks if a field's value is NULL
         * @param field The name of the field to check
         * @return true if the field value is NULL, false otherwise
         */
        bool isFieldValueNull( const String &field ) override;

        /**
         * @brief Moves to the next row in the result set
         */
        void nextRow() override;

        /**
         * @brief Checks if the current position is at the end of the result set
         * @return true if at the end of the result set, false otherwise
         */
        bool eof() override;

        /**
         * @brief Converts the query results to XML format
         * @return A string containing the XML representation of the query results
         */
        String toXML();

        /**
         * @brief Converts the query results to XML format with grouping
         * @return A string containing the grouped XML representation of the query results
         */
        String toXMLGroup();

        SmartPtr<Properties> getProperties() const override;

    protected:
        /**
         * @class Field
         * @brief Internal class representing a database field
         */
        class Field
        {
        public:
            /**
             * @brief Default constructor
             */
            Field();

            /**
             * @brief Constructor that initializes a field with name and value
             * @param name The name of the field
             * @param value The value of the field
             */
            Field( const String &name, const String &value );

            /**
             * @brief Copy constructor
             * @param other The Field object to copy from
             */
            Field( const Field &other );

            /**
             * @brief Destructor
             */
            ~Field();

            String name;   ///< The name of the field
            String value;  ///< The value of the field
        };

        /**
         * @class Rows
         * @brief Internal class representing a collection of fields in a row
         */
        class Rows
        {
        public:
            /**
             * @brief Default constructor
             */
            Rows();

            /**
             * @brief Copy constructor
             * @param other The Rows object to copy from
             */
            Rows( const Rows &other );

            /**
             * @brief Destructor
             */
            ~Rows();

            Array<Field> fields;  ///< Array of fields in the row
        };

        s32 m_currentRow = 0;  ///< Current row index in the result set
        Array<Rows> m_rows;    ///< Collection of rows in the result set
    };
}  // namespace workphone

#endif  // WPSQLiteQuery_h__
