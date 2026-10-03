/**
 * @file IDatabaseQuery.hpp
 * @brief Interface for database query operations and result set management.
 */

#ifndef IDatabaseQuery_h__
#define IDatabaseQuery_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for executing and managing database queries.
     *
     * This interface provides a comprehensive set of methods to interact with database query results,
     * allowing access to field names, values, and navigation through result sets. It is designed to be
     * implemented by specific database drivers to provide a consistent interface for database operations
     * across different database systems.
     *
     * The interface supports:
     * - Field name and value retrieval by index or name
     * - Type conversion for common data types (int, float)
     * - NULL value checking
     * - Result set navigation
     * - End-of-result detection
     *
     * @note This class inherits from ISharedObject for memory management.
     * @see ISharedObject
     */
    class WPCore_API IDatabaseQuery : public ISharedObject
    {
    public:
        IDatabaseQuery();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived class resources and prevents memory leaks.
         */
        ~IDatabaseQuery() override;

        /**
         * @brief Gets the total number of fields in the current query result.
         *
         * @return size_t The number of fields/columns in the current row of the query result.
         * @note This count represents the number of columns in the current row and remains
         *       constant throughout the result set.
         */
        virtual size_t getNumFields() const = 0;

        /**
         * @brief Retrieves the name of a field at the specified index.
         *
         * @param index The zero-based index of the field to retrieve.
         * @return String The name of the field at the specified index.
         * @throw std::out_of_range if index is invalid (index >= getNumFields()).
         */
        virtual String getFieldName( u32 index ) = 0;

        /**
         * @brief Retrieves the value of a field at the specified index.
         *
         * @param index The zero-based index of the field to retrieve.
         * @return String The value of the field as a string.
         * @throw std::out_of_range if index is invalid (index >= getNumFields()).
         * @note The value is converted to a string representation regardless of its original type.
         */
        virtual String getFieldValue( u32 index ) = 0;

        /**
         * @brief Retrieves the value of a field by its name.
         *
         * @param field The name of the field to retrieve.
         * @return String The value of the field as a string.
         * @throw std::runtime_error if field name is not found in the result set.
         * @note The value is converted to a string representation regardless of its original type.
         */
        virtual String getFieldValue( const String &field ) = 0;

        /**
         * @brief Retrieves the value of a field as an integer.
         *
         * @param field The name of the field to retrieve.
         * @return s32 The value of the field as a signed 32-bit integer.
         * @throw std::runtime_error if field name is not found or value cannot be converted to integer.
         */
        virtual s32 getFieldValueAsInt( const String &field ) = 0;

        /**
         * @brief Retrieves the value of a field as a floating-point number.
         *
         * @param field The name of the field to retrieve.
         * @return f32 The value of the field as a 32-bit floating-point number.
         * @throw std::runtime_error if field name is not found or value cannot be converted to float.
         */
        virtual f32 getFieldValueAsFloat( const String &field ) = 0;

        /**
         * @brief Checks if a field's value is NULL in the database.
         *
         * @param field The name of the field to check.
         * @return bool True if the field value is NULL, false otherwise.
         * @throw std::runtime_error if field name is not found in the result set.
         * @note This method is useful for distinguishing between NULL values and empty strings.
         */
        virtual bool isFieldValueNull( const String &field ) = 0;

        /**
         * @brief Advances to the next row in the query result set.
         *
         * This method moves the cursor to the next row in the result set. If there are no more rows,
         * subsequent calls to getFieldValue() and related methods may throw exceptions or return
         * undefined values. Always check eof() after calling this method.
         *
         * @note This method should be called after processing the current row and before
         *       accessing field values of the next row.
         */
        virtual void nextRow() = 0;

        /**
         * @brief Checks if the end of the query result set has been reached.
         *
         * @return bool True if there are no more rows to process, false otherwise.
         * @note This should be checked before attempting to access field values after calling nextRow().
         *       If true is returned, the current row position is invalid and field access methods
         *       should not be called.
         */
        virtual bool eof() = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IDatabaseQuery_h__
