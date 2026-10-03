/**
 * @file Properties.hpp
 * @brief Defines the Properties class for hierarchical property storage and management.
 *
 * @details
 * The Properties class provides a flexible and extensible system for storing, retrieving, and managing
 * properties of various types, including primitive types, complex math types, resource references, and
 * hierarchical property groups. It supports read-only properties, type information, and integration with
 * resource databases for object lookup by UUID. Properties can be organized in a tree structure using
 * child property groups, enabling complex configuration and serialization scenarios.
 */

#ifndef _FBProperties_H
#define _FBProperties_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Property.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone
{
    /**
     * @class Properties
     * @brief Manages a collection of named properties with support for hierarchical grouping.
     *
     * @details
     * The Properties class allows storage and retrieval of named properties of various types, including
     * primitive values, math types, and references to resources or scene objects. Properties can be
     * grouped hierarchically using child property groups, supporting complex data structures. The class
     * provides methods for adding, setting, removing, and querying properties, as well as for managing
     * child groups. Integration with the application resource database allows for automatic resolution
     * of resource handles from property values (e.g., UUIDs). Properties can be marked as read-only to
     * prevent modification.
     *
     * Example usage:
     * @code
     * workphone::Properties props;
     * props.setProperty("width", 1280);
     * props.setProperty("name", "MainWindow");
     * auto width = props.getPropertyAsInt("width");
     * @endcode
     */
    class WPCore_API Properties : public ISharedObject
    {
    public:
        /** @brief Empty string constant used for default values. */
        static const String emptyStr;
        /** @brief Resource string constant used for resource type properties. */
        static const String resourceStr;
        /** @brief Resource type string constant used for resource type identification. */
        static const String resourceTypeStr;
        /** @brief Material string constant used for material type properties. */
        static const String materialStr;
        /** @brief Texture string constant used for texture type properties. */
        static const String textureStr;
        /** @brief Component string constant used for component type properties. */
        static const String componentStr;
        /** @brief None string constant used for null/empty values. */
        static const String noneStr;
        /** @brief Attribute name string constant used for property attributes. */
        static const String attributeNameStr;
        /** @brief Default value string constant used for default property values. */
        static const String defaultValue;
        /** @brief Button type string constant used for button type properties. */
        static const String buttonTypeStr;
        /** @brief Enum type string constant used for enum type properties. */
        static const String enumTypeStr;

        static const String stringTypeStr;
        static const String intTypeStr;
        static const String doubleTypeStr;
        static const String boolTypeStr;

        static const String colourStr;
        static const String colourfStr;

        static const String aabbStr;
        static const String aabbdStr;

        static const String stringArrayStr;
        static const String u32Str;
        static const String unsignedLongLongStr;
        static const String unsignedLongStr;
        static const String floatStr;
        static const String doubleStr;
        static const String vector2iStr;
        static const String vector2Str;
        static const String vector2dStr;
        static const String vector3iStr;
        static const String vector3Str;
        static const String vector3dStr;
        static const String quaternionStr;
        static const String quaterniondStr;
        static const String transformStr;
        static const String transformdStr;

        /**
         * @brief Constructs an empty Properties object.
         * @post The property group contains no properties or children.
         */
        Properties();

        /**
         * @brief Copy constructor. Performs a deep copy of all properties and children.
         * @param other The Properties object to copy from.
         */
        Properties( const Properties &other );

        /**
         * @brief Destructor. Cleans up all properties and child property groups.
         */
        ~Properties() override;

        /**
         * @brief Removes all properties from this property group.
         * @param cascade If true, also clears all properties in child property groups recursively.
         *
         * @note If @p cascade is true, all descendant property groups will also be cleared.
         */
        void clearAll( bool cascade = false );

        /**
         * @brief Adds a property to the property group.
         * @param property The property to add. If a property with the same name exists, it is
         * overwritten.
         */
        void addProperty( const Property &property );

        /**
         * @brief Adds a property with the specified name, value, type, and read-only flag.
         * @param name The name of the property.
         * @param value The value of the property (as string).
         * @param type The type of the property (optional).
         * @param readOnly Whether the property is read-only (optional).
         *
         * @note This does not check for an existing property of the same name; it will be overwritten.
         */
        void addProperty( const String &name, const String &value,
                          const String &type = StringUtil::EmptyString, bool readOnly = false );

        /**
         * @brief Sets a property value as a string with type information.
         * @param name The name of the property.
         * @param value The value to set.
         * @param type The type of the property.
         *
         * If the property does not exist, it is created. The type parameter specifies the data type.
         */
        void setProperty( const String &name, const String &value, const String &type );

        /**
         * @brief Sets a property value as a string with type and read-only information.
         * @param name The name of the property.
         * @param value The value to set.
         * @param type The type of the property.
         * @param readOnly Whether the property should be read-only.
         *
         * If the property does not exist, it is created. The type parameter specifies the data type.
         */
        void setProperty( const String &name, const String &value, const String &type, bool readOnly );

        /**
         * @brief Sets the type of an existing property.
         * @param name The name of the property.
         * @param type The type to set.
         * @return True if the property type was set successfully, false if the property was not found.
         */
        bool setPropertyType( const String &name, const String &type );

        /**
         * @brief Removes a property from the property group.
         * @param name The name of the property to remove.
         * @return True if the property was removed, false if not found.
         */
        bool removeProperty( const String &name );

        /**
         * @brief Finds a property in the property group.
         * @param name The name of the property to find.
         * @param property Reference to store the found property.
         * @return True if the property was found, false otherwise.
         */
        bool getProperty( const String &name, Property &property ) const;

        /**
         * @brief Checks if the property group has a property with the given name.
         * @param name The name of the property to check.
         * @return True if the property exists, false otherwise.
         */
        bool hasProperty( const String &name ) const;

        /**
         * @brief Gets a property object by name.
         * @param name The name of the property.
         * @return Reference to the property object.
         * @throws Exception if the property is not found.
         */
        Property &getPropertyObject( const String &name );

        /**
         * @brief Gets a property object by name (const version).
         * @param name The name of the property.
         * @return Const reference to the property object.
         * @throws Exception if the property is not found.
         */
        const Property &getPropertyObject( const String &name ) const;

        /**
         * @brief Checks if a property value equals the given value.
         * @param name The name of the property.
         * @param value The value to compare against.
         * @return True if the property exists and its value equals @p value, false otherwise.
         */
        bool propertyValueEquals( const String &name, const String &value ) const;

        /**
         * @brief Gets a property value as a string.
         * @param name The name of the property.
         * @param defaultValue The default value to return if the property is not found.
         * @return The property value as a string, or @p defaultValue if not found.
         */
        String getProperty( const String &name, String defaultValue = "" ) const;

        /**
         * @brief Gets a property value as a boolean.
         * @param name The name of the property.
         * @param defaultValue The default value to return if the property is not found.
         * @return The property value as a boolean, or @p defaultValue if not found.
         */
        bool getPropertyAsBool( const String &name, bool defaultValue = false ) const;

        /**
         * @brief Gets a property value as an integer.
         * @param name The name of the property.
         * @param defaultValue The default value to return if the property is not found.
         * @return The property value as an integer, or @p defaultValue if not found.
         */
        s32 getPropertyAsInt( const String &name, s32 defaultValue = 0 ) const;

        /**
         * @brief Gets a property value as a float.
         * @param name The name of the property.
         * @param defaultValue The default value to return if the property is not found.
         * @return The property value as a float, or @p defaultValue if not found.
         */
        f32 getPropertyAsFloat( const String &name, f32 defaultValue = 0.0f ) const;

        /**
         * @brief Gets a property value as a Vector3F.
         * @param name The name of the property.
         * @param defaultValue The default value to return if the property is not found.
         * @return The property value as a Vector3F, or @p defaultValue if not found.
         */
        Vector3F getPropertyAsVector3( const String &name,
                                       Vector3F defaultValue = Vector3F::zero() ) const;

        /**
         * @brief Gets a property value as a Vector3D.
         * @param name The name of the property.
         * @param defaultValue The default value to return if the property is not found.
         * @return The property value as a Vector3D, or @p defaultValue if not found.
         */
        Vector3D getPropertyAsVector3D( const String &name,
                                        Vector3D defaultValue = Vector3D::zero() ) const;

        /**
         * @brief Sets properties from an array.
         * @param array The array of properties to set. Existing properties are replaced.
         */
        void setPropertiesAsArray( const Array<Property> &array );

        /**
         * @brief Gets all properties as an array.
         * @return Array containing all properties in this group.
         */
        Array<Property> getPropertiesAsArray() const;

        /**
         * @brief Assignment operator. Performs a deep copy of all properties and children.
         * @param other The Properties object to copy from.
         * @return Reference to this object.
         */
        Properties &operator=( const Properties &other );

        /**
         * @brief Gets the number of properties in this property group.
         * @return The number of properties.
         */
        u32 getNumProperties() const;

        /**
         * @brief Gets all child property groups.
         * @return Array of smart pointers to child property groups.
         */
        Array<SmartPtr<Properties>> getChildren() const;

        /**
         * @brief Adds a child property group.
         * @param propertyGroup The property group to add as a child.
         */
        void addChild( const SmartPtr<Properties> &propertyGroup );

        /**
         * @brief Removes a child property group by name.
         * @param name The name of the child to remove.
         */
        void removeChild( const String &name );

        /**
         * @brief Gets the number of child property groups.
         * @return The number of children.
         */
        u32 getNumChildren() const;

        /**
         * @brief Checks if this property group has a child with the given name.
         * @param name The name of the child to check for.
         * @return True if the child exists, false otherwise.
         */
        bool hasChild( const String &name ) const;

        /**
         * @brief Gets a child property group by index.
         * @param index The index of the child to get.
         * @return Smart pointer to the child property group.
         * @throws Exception if the index is out of range.
         */
        SmartPtr<Properties> getChild( u32 index ) const;

        /**
         * @brief Gets a child property group by name.
         * @param name The name of the child to get.
         * @return Smart pointer to the child property group.
         * @throws Exception if the child is not found.
         */
        SmartPtr<Properties> getChild( const String &name ) const;

        /**
         * @brief Gets all child property groups with the given name.
         * @param name The name of the children to get.
         * @return Array of smart pointers to child property groups with the given name.
         */
        Array<SmartPtr<Properties>> getChildrenByName( const String &name ) const;

        /**
         * @brief Sets a property value in child property groups.
         * @param name The name of the property.
         * @param value The value to set.
         * @param cascade Whether to set in all children recursively.
         * @param checkingExisting Whether to check for existing properties before setting.
         *
         * @note If @p cascade is true, the property will be set in all descendants.
         */
        void setPropertyInChildren( const String &name, const String &value, bool cascade = false,
                                    bool checkingExisting = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const String &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Array<String> &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const char *const &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const bool &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const s32 &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const u32 &value, bool readOnly = false );

#if defined WP_PLATFORM_WIN32
        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const unsigned long &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const unsigned long long &value, bool readOnly = false );
#elif defined WP_PLATFORM_APPLE
        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const unsigned long &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const unsigned long long &value, bool readOnly = false );
#elif defined WP_PLATFORM_LINUX || defined WP_PLATFORM_ANDROID
        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const u64 &value, bool readOnly = false );
#endif

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const f32 &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const f64 &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Vector2I &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Vector2F &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Vector2D &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Vector3I &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Vector3F &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Vector3D &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const QuaternionF &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const QuaternionD &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Transform3F &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const Transform3D &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const AABB3F &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const AABB3D &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const ColourI &value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, const ColourF &value, bool readOnly = false );

        /**
         * @brief Sets the a property value as an enum.
         * @param name The name of the property.
         * @param value The value to set.
         * @param values The array of possible values.
         * @param readOnly Whether the property is read-only.
         */
        void setPropertyAsEnum( const String &name, s32 value, const Array<String> &values,
                                bool readOnly = false );

        /**
         * @brief Sets the a property value as an enum.
         * @param name The name of the property.
         * @param value The value to set.
         * @param values The array of possible values.
         * @param readOnly Whether the property is read-only.
         */
        void setPropertyAsEnum( const String &name, const String &value, const Array<String> &values,
                                bool readOnly = false );

        /**
         * @brief Sets the a property value as a button.
         * @param name The name of the button.
         * @param value The value to set.
         */
        void setButtonPressed( const String &name, bool value = false );

        /**
         * @brief Checks if a button is pressed.
         * @param name The name of the button.
         * @return true if the button is pressed, false otherwise.
         */
        bool isButtonPressed( const String &name ) const;

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        template <class T>
        void setPropertyAsType( const String &name, SmartPtr<T> value, bool readOnly = false );

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        template <class T>
        bool getPropertyAsType( const String &name, SmartPtr<T> &value ) const;

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, SmartPtr<ISharedObject> value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, SmartPtr<render::IMaterial> value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, SmartPtr<render::ITexture> value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, SmartPtr<scene::IGameActor> value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, SmartPtr<scene::IComponent> value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, SmartPtr<IMeshResource> value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, SmartPtr<ISound> value, bool readOnly = false );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        void setProperty( const String &name, Array<SmartPtr<scene::IComponent>> value,
                          bool readOnly = false );

        /**
         * @brief Sets the a property.
         * @param property The property to set.
         */
        void setProperty( const Property &property );

        /**
         * @brief Sets the a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @param readOnly Sets whether or not the property is read only.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        template <class T>
        void setProperty( const String &name, const Array<SmartPtr<T>> &value, bool readOnly = false );

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, String &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, bool &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, s32 &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, u32 &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, f32 &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, f64 &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, Vector2I &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, Vector2F &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, Vector2D &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, Vector3I &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, Vector3F &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, Vector3D &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, QuaternionF &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, QuaternionD &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, Transform3F &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, Transform3D &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, AABB3F &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, AABB3D &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, ColourI &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, ColourF &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, SmartPtr<ISharedObject> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, SmartPtr<render::IMaterial> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, SmartPtr<render::ITexture> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, SmartPtr<scene::IGameActor> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, SmartPtr<scene::IComponent> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, SmartPtr<IMeshResource> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, SmartPtr<ISound> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        bool getPropertyValue( const String &name, Array<SmartPtr<scene::IComponent>> &value ) const;

        /**
         * @brief Gets a property value. Returns true if the property was found.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return Returns true if the property was found. Returns false if the property was not found.
         */
        bool getPropertyValue( const String &name, Array<String> &value ) const;

        /**
         * @brief Gets a property value.
         * @param name The name of the property.
         * @param value A reference to the value.
         * @return true if the property was found, false otherwise.
         */
        template <class T>
        bool getPropertyValue( const String &name, Array<SmartPtr<T>> &value ) const;

#if WP_ENABLE_TRACE
        s32 addReference();
        bool removeReference();
#endif

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief The array containing all properties in this group.
         */
        Array<Property> m_properties;

        /**
         * @brief Array of child property groups, allowing for hierarchical property organization.
         */
        Array<SmartPtr<Properties>> m_children;
    };

    template <class T>
    void Properties::setProperty( const String &name, const Array<SmartPtr<T>> &value, bool readOnly )
    {
        Array<String> uuids;
        uuids.reserve( value.size() );

        for( const auto &item : value )
        {
            auto handle = item->getHandle();
            auto uuid = handle->getUUIDAsString();
            uuids.push_back( uuid );
        }

        setProperty( name, uuids, readOnly );
    }

    template <class T>
    bool Properties::getPropertyValue( const String &name, Array<SmartPtr<T>> &value ) const
    {
        if( hasProperty( name ) )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto resourceDatabase = applicationManager->getResourceDatabase();

            const auto &property = getPropertyObject( name );

            auto propertyValue = property.getValue();

            Array<String> uuids;
            StringUtil::parseArray( propertyValue, uuids );

            for( const auto &uuid : uuids )
            {
                auto handle = StringUtil::parseUUID( uuid );

                if( auto resource = resourceDatabase->getObject( handle ) )
                {
                    if( resource->isDerived<T>() )
                    {
                        value.push_back( workphone::static_pointer_cast<T>( resource ) );
                    }
                }
            }

            return true;
        }

        return false;
    }

    template <class T>
    void Properties::setPropertyAsType( const String &name, SmartPtr<T> value, bool readOnly )
    {
        auto typeManager = TypeManager::instance();

        if( value )
        {
            auto typeInfo = value->getTypeInfo();
            auto typeName = typeManager->getName( typeInfo );

            auto handle = value->getHandle();
            auto uuid = handle->getUUIDAsString();
            setProperty( name, uuid, resourceStr, false );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, typeName );
        }
        else
        {
            setProperty( name, "", resourceStr, false );

            auto typeInfo = T::typeInfo();
            auto typeName = typeManager->getName( typeInfo );

            auto &property = getPropertyObject( name );
            property.setAttribute( resourceTypeStr, typeName );
        }
    }

    template <class T>
    bool Properties::getPropertyAsType( const String &name, SmartPtr<T> &value ) const
    {
        if( hasProperty( name ) )
        {
            const auto &property = getPropertyObject( name );
            const auto sUUID = property.getValue();

            if( !StringUtil::isNullOrEmpty( sUUID ) )
            {
                auto uuid = StringUtil::parseUUID( sUUID );

                auto applicationManager = core::IApplicationManager::instance();
                auto resourceDatabase = applicationManager->getResourceDatabase();

                if( auto resource = resourceDatabase->getObject( uuid ) )
                {
                    if( resource->isDerived<T>() )
                    {
                        value = workphone::dynamic_pointer_cast<T>( resource );
                    }
                    else if( resource->isDerived<scene::IGameActor>() )
                    {
                        auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( resource );
                        value = actor->getComponent<T>();
                    }
                }

                return true;
            }
        }

        return false;
    }
}  // namespace workphone

#endif
