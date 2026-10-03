#ifndef __WP_Property_H__
#define __WP_Property_H__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Core/Any.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/ColourI.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    /**
     * @class Property
     * @brief Represents a key-value property with optional label, type, attributes, and value conversion
     * utilities.
     *
     * The Property class is designed to store a named value (as a string), with optional label, type,
     * and read-only flag. It also supports storing additional attributes as key-value pairs, and
     * provides utility functions to retrieve the value in various types (bool, int, float, vector,
     * etc.).
     */
    class WPCore_API Property
    {
    public:
        static const String defaultType;

        /**
         * @brief Default constructor. Initializes an empty property.
         */
        Property();

        /**
         * @brief Copy constructor.
         * @param other The property to copy from.
         */
        Property( const Property &other );

        /**
         * @brief Constructs a property with a name and value.
         * @param name The name/key of the property.
         * @param value The value of the property as a string.
         */
        Property( const String &name, const String &value );

        /**
         * @brief Constructs a property with a name, value, and type.
         * @param name The name/key of the property.
         * @param value The value of the property as a string.
         * @param type The type of the property (for metadata or UI purposes).
         */
        Property( const String &name, const String &value, const String &type );

        /**
         * @brief Constructs a property with a name, value, label, and type.
         * @param name The name/key of the property.
         * @param value The value of the property as a string.
         * @param label A user-friendly label for display purposes.
         * @param type The type of the property (for metadata or UI purposes).
         */
        Property( const String &name, const String &value, const String &label, const String &type );

        /**
         * @brief Constructs a property with a name, value, label, type, and read-only flag.
         * @param name The name/key of the property.
         * @param value The value of the property as a string.
         * @param label A user-friendly label for display purposes.
         * @param type The type of the property (for metadata or UI purposes).
         * @param readOnly If true, the property is read-only and cannot be modified.
         */
        Property( const String &name, const String &value, const String &label, const String &type,
                  bool readOnly );

        /**
         * @brief Destructor.
         */
        ~Property();

        /**
         * @brief Sets the name/key of the property.
         * @param name The new name for the property.
         */
        void setName( const String &name );

        /**
         * @brief Gets the name/key of the property.
         * @return The property name.
         */
        String getName() const;

        /**
         * @brief Gets the name/key of the property.
         * @return The property name.
         */
        c8 *getNamePtr();

        /**
         * @brief Gets the name/key of the property.
         * @return The property name.
         */
        const c8 *getNamePtr() const;

        /**
         * @brief Sets the value of the property.
         * @param value The new value as a string.
         */
        void setValue( const String &value );

        /**
         * @brief Gets the value of the property as a string.
         * @return The property value.
         */
        String getValue() const;

        /**
         * @brief Gets the value of the property as a string.
         * @return The property value.
         */
        c8 *getValuePtr();

        /**
         * @brief Gets the value of the property as a string.
         * @return The property value.
         */
        const c8 *getValuePtr() const;

        /**
         * @brief Gets the value of the property as a boolean.
         * @return The value converted to bool.
         */
        bool getValueAsBool() const;

        /**
         * @brief Gets the value of the property as a button.
         * @return The value converted to bool representing button state.
         */
        bool getValueAsButton() const;

        /**
         * @brief Sets the value of the property as a boolean.
         * @param value The boolean value to set.
         */
        void setValueAsBool( bool value );

        /**
         * @brief Sets the value of the property as a button (boolean).
         * @param value The button state to set.
         */
        void setValueAsButton( bool value );

        /**
         * @brief Gets the value of the property as an integer.
         * @return The value converted to s32.
         */
        s32 getValueAsEnum() const;

        /**
         * @brief Sets the value of the property as an integer.
         * @param value The integer value to set.
         */
        void setValueAsEnum( s32 value );

        /**
         * @brief Gets the value of the property as an integer.
         * @return The value converted to s32.
         */
        s32 getValueAsInt() const;

        /**
         * @brief Sets the value of the property as an integer.
         * @param value The integer value to set.
         */
        void setValueAsInt( s32 value );

        /**
         * @brief Gets the value of the property as an unsigned integer.
         * @return The value converted to u32.
         */
        u32 getValueAsUInt() const;

        /**
         * @brief Sets the value of the property as an unsigned integer.
         * @param value The integer value to set.
         */
        void setValueAsUInt( u32 value );

        /**
         * @brief Gets the value of the property as a float.
         * @return The value converted to f32.
         */
        f32 getValueAsFloat() const;

        /**
         * @brief Sets the value of the property as a float.
         * @param value The float value to set.
         */
        void setValueAsFloat( f32 value );

        /**
         * @brief Gets the value of the property as a 3D vector.
         * @return The value converted to Vector3F.
         */
        Vector3F getValueAsVector3f() const;

        /**
         * @brief Gets the value of the property as a 4D vector.
         * @return The value converted to Vector4F.
         */
        Vector4F getValueAsVector4f() const;

        /**
         * @brief Sets the type of the property.
         * @param type The new type for the property.
         */
        void setTypeName( const String &type );

        /**
         * @brief Gets the type of the property.
         * @return The property type.
         */
        String getTypeName() const;

        /**
         * @brief Sets the type of the property.
         * @param type The new type for the property.
         */
        void setType( ParameterType type );

        /**
         * @brief Gets the type of the property.
         * @return The property type.
         */
        ParameterType getType() const;

        /**
         * @brief Sets the property as read-only or writable.
         * @param readOnly If true, the property is read-only.
         */
        void setReadOnly( bool readOnly );

        /**
         * @brief Checks if the property is read-only.
         * @return True if the property is read-only, false otherwise.
         */
        bool isReadOnly() const;

        /**
         * @brief Sets an attribute key-value pair for the property.
         * @param name The attribute name.
         * @param value The attribute value.
         */
        void setAttribute( const String &name, const String &value );

        /**
         * @brief Gets the value of a specific attribute.
         * @param name The attribute name.
         * @return The attribute value.
         */
        String getAttribute( const String &name ) const;

        /**
         * @brief Gets all attributes as an array of key-value pairs.
         * @return Array of attribute pairs.
         */
        Array<Pair<String, String>> getAttributes() const;

        /**
         * @brief Gets an attribute by its index in the attribute map.
         * @param index The index of the attribute.
         * @return The attribute pair at the given index.
         */
        Pair<String, String> getAttributeByIndex( u32 index );

        /**
         * @brief Gets the number of attributes.
         * @return The number of attributes.
         */
        u32 getNumAttributes() const;

        /**
         * @brief Assignment operator.
         * @param other The property to assign from.
         * @return Reference to this property.
         */
        Property &operator=( const Property &other );

    private:
        union Data
        {
            bool bData;
            u32 uData;
            s32 iData;
            f32 fData;
        };

        void clearData();
        void setDataFromValue( const String &value );

        /** The parameter data. */
        Data m_data;

        /** The type of the property, used for metadata or UI. */
        ParameterType m_type = ParameterType::PARAM_TYPE_NULL;

        /** Indicates if the property is read-only. */
        u8 m_readOnly = 0;

        /** The name/key of the property. */
        FixedString<128> m_name;

        /** The value of the property as a string. */
        FixedString<256> m_value;

        /** The attributes associated with this property. */
        Array<Pair<FixedString<128>, FixedString<256>>> m_attributes;
    };

    inline c8 *Property::getNamePtr()
    {
        return (c8 *)m_name.c_str();
    }

    inline const c8 *Property::getNamePtr() const
    {
        return (c8 *)m_name.c_str();
    }

    inline c8 *Property::getValuePtr()
    {
        return (c8 *)m_value.c_str();
    }

    inline const c8 *Property::getValuePtr() const
    {
        return m_value.c_str();
    }

}  // namespace workphone

#endif
