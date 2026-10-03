#ifndef __WP_XmlUtil_h__
#define __WP_XmlUtil_h__

#include <Workphone/Core/Properties.hpp>

namespace workphone
{

    /**
     * @class XmlUtil
     * @brief Utility class for XML file and string manipulation, parsing, and property serialization.
     *
     * Provides static methods to load, parse, and extract data from XML documents, as well as serialize
     * and deserialize custom Properties objects to and from XML. Supports both narrow and wide string
     * file paths and text extraction.
     */
    class WPCore_API XmlUtil
    {
    public:
        // XML element and attribute name constants
        static const String ROOT_ELEMENT;
        static const String PROPERTY_ELEMENT;
        static const String CHILDREN_ELEMENT;
        static const String PROPERTIES_ELEMENT;
        static const String NAME_ATTRIBUTE;
        static const String TYPE_ATTRIBUTE;
        static const String LABEL_ATTRIBUTE;
        static const String READONLY_ATTRIBUTE;
        static const String DEFAULT_PROPERTY_TYPE;

        /**
         * @brief Loads an XML document from a file.
         *
         * @param filePath The path to the XML file to load.
         * @return A shared pointer to the loaded TiXmlDocument object, or nullptr if loading fails.
         */
        static SharedPtr<TiXmlDocument> loadFile( const String &filePath );

        /**
         * @brief Loads an XML document from a file with a wide string path.
         *
         * @param filePath The wide string path to the XML file to load.
         * @return A shared pointer to the loaded TiXmlDocument object, or nullptr if loading fails.
         */
        static SharedPtr<TiXmlDocument> loadFile( const StringW &filePath );

        /**
         * @brief Retrieves the content of an XML file as a string.
         *
         * @param filePath The path to the XML file.
         * @return The content of the XML file as a string. Returns an empty string if the file cannot be
         * read.
         */
        static String getFromFile( const String &filePath );

        /**
         * @brief Retrieves the content of an XML file as a string with a wide string path.
         *
         * @param filePath The wide string path to the XML file.
         * @return The content of the XML file as a string. Returns an empty string if the file cannot be
         * read.
         */
        static String getFromFile( const StringW &filePath );

        /**
         * @brief Parses an XML document from a string.
         *
         * @param xmlString The XML string to parse.
         * @return A shared pointer to the parsed TiXmlDocument object, or nullptr if parsing fails.
         */
        static SharedPtr<TiXmlDocument> parseDocument( const String &xmlString );

        /**
         * @brief Retrieves the text content of a child XML element by name.
         *
         * @param parent The parent XML element.
         * @param childName The name of the child element.
         * @return The text content of the child element, or an empty string if not found.
         */
        static String getText( TiXmlElement *parent, const String &childName );

        /**
         * @brief Retrieves the text content of an XML element.
         *
         * @param element The XML element.
         * @return The text content of the element, or an empty string if the element is null or has no
         * text.
         */
        static String getText( TiXmlElement *element );

        /**
         * @brief Retrieves the wide string text content of a child XML element by name.
         *
         * @param parent The parent XML element.
         * @param childName The name of the child element (wide string).
         * @return The wide string text content of the child element, or an empty string if not found.
         */
        static StringW getTextW( TiXmlElement *parent, const StringW &childName );

        /**
         * @brief Retrieves the wide string text content of an XML element.
         *
         * @param element The XML element.
         * @return The wide string text content of the element, or an empty string if the element is null
         * or has no text.
         */
        static StringW getTextW( TiXmlElement *element );

        /**
         * @brief Serializes a Properties object to an XML string.
         *
         * @param properties The Properties object to serialize.
         * @return The XML string representation of the properties.
         */
        static String toString( const Properties &properties );

        /**
         * @brief Parses an XML string and populates a Properties object.
         *
         * @param properties The Properties object to populate.
         * @param xmlString The XML string to parse.
         */
        static void parse( Properties &properties, const String &xmlString );

        /**
         * @brief Writes the properties of a property group to an XML node.
         *
         * @param properties The Properties object to write.
         * @param pCurNode The XML node to write the properties to.
         * @param recursive If true, writes child properties recursively; otherwise, only writes the
         * current level.
         */
        static void writeProperties( const Properties &properties, TiXmlNode *pCurNode,
                                     bool recursive = false );

        /**
         * @brief Writes the child properties of a property group to an XML node.
         *
         * @param properties The Properties object containing child properties to write.
         * @param pCurNode The XML node to write the child properties to.
         */
        static void writeChildProperties( const Properties &properties, TiXmlNode *pCurNode );

        /**
         * @brief Loads properties from an XML node representing a property group.
         *
         * @param properties The Properties object to populate with loaded properties.
         * @param propertiesNode The XML node representing the property group.
         */
        static void loadProperties( Properties &properties, const TiXmlElement *propertiesNode );

        /**
         * @brief Converts a C-style string to a String object.
         *
         * @param pStr The C-style string to convert.
         * @return The converted String object.
         */
        static String getString( const c8 *pStr );

        /**
         * @brief Converts a wide C-style string to a String object.
         *
         * @param pStr The wide C-style string to convert.
         * @return The converted String object.
         */
        static String createErrorXML( u32 lineNumber, const String &src );

        /**
         * @brief Creates an error XML string with line number, source class, and function name.
         *
         * @param lineNumber The line number where the error occurred.
         * @param srcClass The source class name where the error occurred.
         * @param func The function name where the error occurred.
         * @return An XML string representing the error information.
         */
        static String createErrorXML( u32 lineNumber, const String &srcClass, const String &func );
    };
}  // namespace workphone

#endif  // XmlUtil_h__
