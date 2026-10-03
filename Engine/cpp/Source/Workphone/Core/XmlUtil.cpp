#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/XmlUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <tinyxml.h>

namespace workphone
{

    // Static XML element and attribute name definitions
    const String XmlUtil::ROOT_ELEMENT = "Root";
    const String XmlUtil::PROPERTY_ELEMENT = "Property";
    const String XmlUtil::CHILDREN_ELEMENT = "Children";
    const String XmlUtil::PROPERTIES_ELEMENT = "Properties";
    const String XmlUtil::NAME_ATTRIBUTE = "Name";
    const String XmlUtil::TYPE_ATTRIBUTE = "Type";
    const String XmlUtil::LABEL_ATTRIBUTE = "label";
    const String XmlUtil::READONLY_ATTRIBUTE = "readOnly";
    const String XmlUtil::DEFAULT_PROPERTY_TYPE = "string";

    auto XmlUtil::loadFile( const String &filePath ) -> SharedPtr<TiXmlDocument>
    {
        WP_ASSERT( !filePath.empty() );

        auto doc = workphone::make_shared<TiXmlDocument>();
        WP_ASSERT( doc );

        if( filePath.empty() )
        {
            return doc;
        }

        const auto loaded = doc->LoadFile( filePath.c_str() );
        WP_ASSERT( loaded );
        WP_ASSERT( !doc->Error() );

        return doc;
    }

    auto XmlUtil::loadFile( const StringW &filePath ) -> SharedPtr<TiXmlDocument>
    {
        WP_ASSERT( !filePath.empty() );

        auto doc = workphone::make_shared<TiXmlDocument>();
        WP_ASSERT( doc );

        if( filePath.empty() )
        {
            return doc;
        }

#if defined WP_PLATFORM_WIN32
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return doc;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );
        if( !fileSystem )
        {
            return doc;
        }

        auto stream = fileSystem->open( StringUtil::toUTF16to8( filePath ) );
        WP_ASSERT( stream );
        if( stream && stream->isOpen() )
        {
            auto dataStr = stream->getAsString();
            WP_ASSERT( !dataStr.empty() );

            doc->Parse( dataStr.c_str() );
            WP_ASSERT( !doc->Error() );
        }
        else
        {
            WP_ASSERT( false );
        }
#endif

        return doc;
    }

    auto XmlUtil::getFromFile( const String &filePath ) -> String
    {
        auto doc = loadFile( filePath );
        WP_ASSERT( doc );
        if( !doc || doc->Error() )
        {
            WP_ASSERT( doc && !doc->Error() );
            return {};
        }

        TiXmlPrinter printer;
        printer.SetIndent( "	" );

        doc->Accept( &printer );
        const auto str = printer.CStr();
        return str ? str : "";
    }

    auto XmlUtil::getFromFile( const StringW &filePath ) -> String
    {
        auto doc = loadFile( filePath );
        WP_ASSERT( doc );
        if( !doc || doc->Error() )
        {
            WP_ASSERT( doc && !doc->Error() );
            return {};
        }

        TiXmlPrinter printer;
        printer.SetIndent( "	" );

        doc->Accept( &printer );
        const auto str = printer.CStr();
        return str ? str : "";
    }

    auto XmlUtil::parseDocument( const String &xmlString ) -> SharedPtr<TiXmlDocument>
    {
        WP_ASSERT( !xmlString.empty() );

        auto doc = workphone::make_shared<TiXmlDocument>();
        WP_ASSERT( doc );
        if( xmlString.empty() )
        {
            return doc;
        }

        doc->Parse( xmlString.c_str() );

        if( doc->Error() )
        {
            auto msg = String( "Error: could not parse document: " ) + xmlString;
            WP_LOG( msg.c_str() );
        }
        WP_ASSERT( !doc->Error() );

        return doc;
    }

    auto XmlUtil::getText( TiXmlElement *parent, const String &childName ) -> String
    {
        WP_ASSERT( parent );
        WP_ASSERT( !childName.empty() );

        if( !parent )
        {
            return {};
        }
        if( childName.empty() )
        {
            return {};
        }

        auto element = parent->FirstChildElement( childName.c_str() );
        WP_ASSERT( element );
        return getText( element );
    }

    auto XmlUtil::getText( TiXmlElement *element ) -> String
    {
        WP_ASSERT( element );
        return ( element && element->GetText() ) ? element->GetText() : "";
    }

    auto XmlUtil::getTextW( TiXmlElement *parent, const StringW &childName ) -> StringW
    {
        WP_ASSERT( parent );
        WP_ASSERT( !childName.empty() );

        if( !parent )
        {
            return {};
        }
        if( childName.empty() )
        {
            return {};
        }

        auto element = parent->FirstChildElement( StringUtil::toStringC( childName ).c_str() );
        WP_ASSERT( element );
        return getTextW( element );
    }

    auto XmlUtil::getTextW( TiXmlElement *element ) -> StringW
    {
        WP_ASSERT( element );
        return ( element && element->GetText() ) ? StringUtil::toStringW( element->GetText() ) : L"";
    }

    String XmlUtil::toString( const Properties &properties )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return {};
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );
        if( !factoryManager )
        {
            return {};
        }

        auto doc = factoryManager->make_shared<TiXmlDocument>();
        WP_ASSERT( doc );
        if( !doc )
        {
            return {};
        }

        // Add XML declaration
        auto decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
        WP_ASSERT( decl );
        doc->LinkEndChild( decl );

        // Create root element
        auto root = new TiXmlElement( ROOT_ELEMENT.c_str() );
        WP_ASSERT( root );

        auto propertiesName = properties.getName();
        root->SetAttribute( NAME_ATTRIBUTE.c_str(), propertiesName.c_str() );
        doc->LinkEndChild( root );

        writeProperties( properties, root, true );

        TiXmlPrinter printer;
        printer.SetIndent( "\t" );

        doc->Accept( &printer );

        const auto str = printer.CStr();
        return str ? str : String();
    }

    void XmlUtil::parse( Properties &properties, const String &xmlString )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );
        if( !factoryManager )
        {
            return;
        }

        auto doc = factoryManager->make_shared<TiXmlDocument>();
        WP_ASSERT( doc );
        if( !doc )
        {
            return;
        }

        WP_ASSERT( !xmlString.empty() );
        if( xmlString.empty() )
        {
            return;
        }

        doc->Parse( xmlString.c_str() );
        WP_ASSERT( !doc->Error() );
        if( doc->Error() )
        {
            auto msg = String( "Error: could not parse document: " ) + xmlString;
            WP_LOG_ERROR( msg );
            return;
        }

        if( auto root = doc->RootElement() )
        {
            const auto rootName = root->Value();
            WP_ASSERT( rootName );
            WP_ASSERT( rootName && String( rootName ) == ROOT_ELEMENT );

            if( auto element = root->FirstChildElement( PROPERTIES_ELEMENT.c_str() ) )
            {
                properties = Properties();
                loadProperties( properties, element );
            }
            else
            {
                WP_LOG_ERROR( "Error: could not find properties element in document." );
            }
        }
        else
        {
            auto msg = String( "Error: could not parse document: " ) + xmlString;
            WP_LOG_ERROR( msg );
        }
    }

    void XmlUtil::writeProperties( const Properties &properties, TiXmlNode *curNode, bool recursive )
    {
        WP_ASSERT( curNode );
        if( !curNode )
        {
            return;
        }

        auto propertiesElem = new TiXmlElement( PROPERTIES_ELEMENT.c_str() );
        WP_ASSERT( propertiesElem );
        curNode->LinkEndChild( propertiesElem );

        auto name = properties.getName();
        propertiesElem->SetAttribute( NAME_ATTRIBUTE.c_str(), name.c_str() );

        const auto propertiesArray = properties.getPropertiesAsArray();

        // loop though the objects keys
        for( const auto &property : propertiesArray )
        {
            auto keyNode = new TiXmlElement( PROPERTY_ELEMENT.c_str() );
            WP_ASSERT( keyNode );
            propertiesElem->LinkEndChild( keyNode );

            auto propertyName = property.getName();
            auto propertyType = property.getTypeName();
            auto propertyIsReadOnly = StringUtil::toString( property.isReadOnly() );
            WP_ASSERT( !propertyName.empty() );
            //WP_ASSERT( !propertyType.empty() );

            keyNode->SetAttribute( NAME_ATTRIBUTE.c_str(), propertyName.c_str() );
            keyNode->SetAttribute( TYPE_ATTRIBUTE.c_str(), propertyType.c_str() );
            keyNode->SetAttribute( READONLY_ATTRIBUTE.c_str(), propertyIsReadOnly.c_str() );

            //key Value
            auto valueStr = property.getValue();
            auto keyValueNode = new TiXmlText( valueStr.c_str() );
            WP_ASSERT( keyValueNode );

            keyNode->LinkEndChild( keyValueNode );
        }

        if( recursive )
        {
            auto numChildren = properties.getNumChildren();
            if( numChildren > 0 )
            {
                auto children = new TiXmlElement( CHILDREN_ELEMENT.c_str() );
                WP_ASSERT( children );
                propertiesElem->LinkEndChild( children );

                writeChildProperties( properties, children );
            }
        }
    }

    void XmlUtil::writeChildProperties( const Properties &properties, TiXmlNode *pCurNode )
    {
        WP_ASSERT( pCurNode );
        if( !pCurNode )
        {
            return;
        }

        // write child properties
        auto children = properties.getChildren();
        for( auto child : children )
        {
            WP_ASSERT( child );
            if( !child )
            {
                continue;
            }

            writeProperties( *child, pCurNode, true );
        }
    }

    void XmlUtil::loadProperties( Properties &properties, const TiXmlElement *propertiesNode )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );
        if( !factoryManager )
        {
            return;
        }

        WP_ASSERT( propertiesNode );
        if( !propertiesNode )
        {
            return;
        }

        const auto propertiesNodeName = propertiesNode->Value();
        WP_ASSERT( propertiesNodeName );
        WP_ASSERT( propertiesNodeName && String( propertiesNodeName ) == PROPERTIES_ELEMENT );

        const auto propertiesNamePtr = propertiesNode->Attribute( NAME_ATTRIBUTE.c_str() );
        WP_ASSERT( propertiesNamePtr );
        const auto propertiesName = getString( propertiesNamePtr );
        properties.setName( propertiesName );

        Array<Property> propertiesArray;
        propertiesArray.reserve( 12 );
        auto propertyCount = 0;

        auto propertyNode = propertiesNode->FirstChildElement( PROPERTY_ELEMENT.c_str() );
        while( propertyNode )
        {
            propertiesArray.resize( propertyCount + 1 );
            auto &property = propertiesArray[propertyCount++];

            auto propertyName = getString( propertyNode->Attribute( NAME_ATTRIBUTE.c_str() ) );
            WP_ASSERT( !propertyName.empty() );
            property.setName( propertyName );

            auto propertyType = getString( propertyNode->Attribute( TYPE_ATTRIBUTE.c_str() ) );
            if( propertyType.length() == 0 )
            {
                property.setTypeName( DEFAULT_PROPERTY_TYPE );
            }
            else
            {
                property.setTypeName( propertyType );
            }

            auto propertyTypeEnum = property.getType();
            const auto propertyValuePtr = propertyNode->GetText();
            const auto propertyValueStr = getString( propertyValuePtr );

            switch( propertyTypeEnum )
            {
            case ParameterType::PARAM_TYPE_BOOL:
            {
                property.setValueAsBool( StringUtil::parseBool( propertyValueStr ) );
            }
            break;
            case ParameterType::PARAM_TYPE_F32:
            case ParameterType::PARAM_TYPE_F64:
            {
                property.setValueAsFloat( StringUtil::parseFloat( propertyValueStr ) );
            }
            break;
            default:
            {
                property.setValue( propertyValueStr );
            }
            }

            auto propertyLabel = getString( propertyNode->Attribute( LABEL_ATTRIBUTE.c_str() ) );
            if( propertyLabel.length() != 0 )
            {
                static const auto attributeStr = "attribute";
                property.setAttribute( attributeStr, propertyLabel );
            }

            auto propertyIsReadOnly = getString( propertyNode->Attribute( READONLY_ATTRIBUTE.c_str() ) );
            if( propertyIsReadOnly.length() != 0 )
            {
                property.setReadOnly( StringUtil::parseBool( propertyIsReadOnly, false ) );
            }

            propertyNode = propertyNode->NextSiblingElement( PROPERTY_ELEMENT.c_str() );
        }

        properties.setPropertiesAsArray( propertiesArray );

        auto childNode = propertiesNode->FirstChildElement( CHILDREN_ELEMENT.c_str() );
        while( childNode )
        {
            auto childPropertiesNode = childNode->FirstChildElement( PROPERTIES_ELEMENT.c_str() );
            while( childPropertiesNode )
            {
                auto childProperties = factoryManager->make_ptr<Properties>();
                WP_ASSERT( childProperties );
                if( !childProperties )
                {
                    childPropertiesNode =
                        childPropertiesNode->NextSiblingElement( PROPERTIES_ELEMENT.c_str() );
                    continue;
                }

                loadProperties( *childProperties, childPropertiesNode );
                properties.addChild( childProperties );

                childPropertiesNode =
                    childPropertiesNode->NextSiblingElement( PROPERTIES_ELEMENT.c_str() );
            }

            childNode = childNode->NextSiblingElement( CHILDREN_ELEMENT.c_str() );
        }
    }

    auto XmlUtil::getString( const c8 *pStr ) -> String
    {
        return pStr ? pStr : String();
    }

    String XmlUtil::createErrorXML( u32 lineNumber, const String &src )
    {
        TiXmlDocument doc;

        static const auto errorStr = "error";

        auto rootElem = new TiXmlElement( errorStr );
        WP_ASSERT( rootElem );
        doc.LinkEndChild( rootElem );

        if( rootElem )
        {
            static const auto lineStr = "line";
            auto lineElem = new TiXmlElement( lineStr );
            WP_ASSERT( lineElem );
            rootElem->LinkEndChild( lineElem );

            auto lineNumberStr = StringUtil::toString( lineNumber );
            auto lineTxtElem = new TiXmlText( lineNumberStr.c_str() );
            WP_ASSERT( lineTxtElem );
            lineElem->LinkEndChild( lineTxtElem );

            static const auto sourceStr = "source";
            auto sourceElem = new TiXmlElement( sourceStr );
            WP_ASSERT( sourceElem );
            rootElem->LinkEndChild( sourceElem );

            TiXmlText *sourceTxtElem = new TiXmlText( src.c_str() );
            WP_ASSERT( sourceTxtElem );
            sourceElem->LinkEndChild( sourceTxtElem );
        }

        TiXmlPrinter printer;
        printer.SetIndent( "	" );

        doc.Accept( &printer );

        return printer.CStr() ? printer.CStr() : StringUtil::EmptyString;
    }

    String XmlUtil::createErrorXML( u32 lineNumber, const String &srcClass, const String &func )
    {
        TiXmlDocument doc;

        static const auto errorStr = "error";

        auto rootElem = new TiXmlElement( errorStr );
        WP_ASSERT( rootElem );
        doc.LinkEndChild( rootElem );

        if( rootElem )
        {
            static const auto lineStr = "line";

            auto lineElem = new TiXmlElement( lineStr );
            WP_ASSERT( lineElem );
            rootElem->LinkEndChild( lineElem );

            auto lineNumberStr = StringUtil::toString( lineNumber );

            auto lineTxtElem = new TiXmlText( lineNumberStr.c_str() );
            WP_ASSERT( lineTxtElem );
            lineElem->LinkEndChild( lineTxtElem );

            static const auto sourceStr = "source";

            auto sourceElem = new TiXmlElement( sourceStr );
            WP_ASSERT( sourceElem );
            rootElem->LinkEndChild( sourceElem );

            TiXmlText *sourceTxtElem = new TiXmlText( srcClass.c_str() );
            WP_ASSERT( sourceTxtElem );
            sourceElem->LinkEndChild( sourceTxtElem );
            static const auto functionStr = "function";

            auto functionElem = new TiXmlElement( functionStr );
            WP_ASSERT( functionElem );
            rootElem->LinkEndChild( functionElem );

            TiXmlText *functionTxtElem = new TiXmlText( func.c_str() );
            WP_ASSERT( functionTxtElem );
            functionElem->LinkEndChild( functionTxtElem );
        }

        TiXmlPrinter printer;
        printer.SetIndent( "	" );
        doc.Accept( &printer );
        return printer.CStr() ? printer.CStr() : StringUtil::EmptyString;
    }
}  // namespace workphone
