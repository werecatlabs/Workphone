#include <WPSQLite/SQLiteQuery.hpp>
#include <Workphone/Workphone.hpp>
#include <tinyxml.h>

namespace workphone
{

    SQLiteQuery::SQLiteQuery()
    {
    }

    SQLiteQuery::SQLiteQuery( CppSQLite3Query &query )
    {
        setQuery( query );
    }

    void SQLiteQuery::setQuery( CppSQLite3Query &query )
    {
        try
        {
            while( !query.eof() )
            {
                Rows currentRow;

                int numFields = query.numFields();
                currentRow.fields.reserve( numFields );

                for( int index = 0; index < numFields; ++index )
                {
                    const char *pFieldName = query.fieldName( index );
                    const char *pFieldValue = query.fieldValue( index );

                    Field field;
                    field.name = pFieldName ? pFieldName : "";
                    field.value = pFieldValue ? pFieldValue : "";
                    currentRow.fields.push_back( field );
                }

                m_rows.push_back( currentRow );

                query.nextRow();
            }
        }
        catch( CppSQLite3Exception &e )
        {
            WP_LOG_ERROR( e.errorMessage() );
        }
    }

    SQLiteQuery::~SQLiteQuery()
    {
    }

    String SQLiteQuery::getFieldName( u32 index )
    {
        if( m_currentRow < m_rows.size() )
        {
            auto &row = m_rows[m_currentRow];
            if( index < row.fields.size() )
            {
                auto &field = row.fields[index];
                return field.name;
            }
        }

        return "";
    }

    String SQLiteQuery::getFieldValue( u32 index )
    {
        if( m_currentRow < m_rows.size() )
        {
            auto &row = m_rows[m_currentRow];
            if( index < row.fields.size() )
            {
                auto &field = row.fields[index];
                return field.value;
            }
        }

        return "";
    }

    String SQLiteQuery::getFieldValue( const String &field )
    {
        try
        {
            if( m_currentRow < m_rows.size() )
            {
                for( auto &i : m_rows[m_currentRow].fields )
                {
                    if( i.name == field )
                    {
                        return i.value;
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return "";
    }

    s32 SQLiteQuery::getFieldValueAsInt( const String &fieldName )
    {
        return StringUtil::parseInt( getFieldValue( fieldName ) );
    }

    f32 SQLiteQuery::getFieldValueAsFloat( const String &fieldName )
    {
        return StringUtil::parseFloat( getFieldValue( fieldName ) );
    }

    size_t SQLiteQuery::getNumFields() const
    {
        if( m_currentRow < m_rows.size() )
        {
            return m_rows[m_currentRow].fields.size();
        }

        return 0;
    }

    void SQLiteQuery::nextRow()
    {
        ++m_currentRow;
    }

    bool SQLiteQuery::isFieldValueNull( const String &field )
    {
        try
        {
            if( m_currentRow < m_rows.size() )
            {
                for( auto &i : m_rows[m_currentRow].fields )
                {
                    if( i.name == field )
                    {
                        return StringUtil::isNullOrEmpty( i.value );
                    }
                }
            }
        }
        catch( CppSQLite3Exception &e )
        {
            WP_LOG_ERROR( e.errorMessage() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return true;
    }

    bool SQLiteQuery::eof()
    {
        return !( m_currentRow < m_rows.size() );
    }

    String SQLiteQuery::toXML()
    {
        if( eof() )
        {
            return String( "" );
        }

        // create xml
        TiXmlDocument doc;
        // doc.LinkEndChild(WP_NEW TiXmlDeclaration());

        // TiXmlElement* rootElem = WP_NEW TiXmlElement("query");
        // doc.LinkEndChild(rootElem);

        auto resultsElem = new TiXmlElement( "results" );
        // rootElem->LinkEndChild(resultsElem);
        doc.LinkEndChild( resultsElem );

        while( !eof() )
        {
            auto resultElem = new TiXmlElement( "result" );
            resultsElem->LinkEndChild( resultElem );

            u32 numFields = (u32)getNumFields();
            for( u32 i = 0; i < numFields; ++i )
            {
                String fieldName = getFieldName( i );
                String fieldValue = getFieldValue( i );

                auto fieldElem = new TiXmlElement( fieldName.c_str() );
                resultElem->LinkEndChild( fieldElem );

                auto fieldValueElem = new TiXmlText( fieldValue.c_str() );
                fieldElem->LinkEndChild( fieldValueElem );
            }

            nextRow();
        }

        TiXmlPrinter printer;
        printer.SetIndent( "	" );

        doc.Accept( &printer );
        const char *str = printer.CStr();
        String xmltext = str ? str : "";

#if _DEBUG
        doc.SaveFile( "Query.xml" );
#endif

        return xmltext;
    }

    String SQLiteQuery::toXMLGroup()
    {
        if( eof() )
            return String( "" );

        // create xml
        TiXmlDocument doc;
        // doc.LinkEndChild(WP_NEW TiXmlDeclaration());

        // TiXmlElement* rootElem = WP_NEW TiXmlElement("query");
        // doc.LinkEndChild(rootElem);

        auto resultsElem = new TiXmlElement( "results" );
        // rootElem->LinkEndChild(resultsElem);
        doc.LinkEndChild( resultsElem );

        int channel = 0;
        int powerChannel;
        TiXmlElement *groupElem;

        while( !eof() )
        {
            powerChannel = StringUtil::parseInt( getFieldValue( 2 ) );
            if( powerChannel > channel )
            {
                ++channel;
                groupElem = new TiXmlElement( "group" );
                resultsElem->LinkEndChild( groupElem );
                String powerField = "powerchannel";
                auto powerElem = new TiXmlElement( powerField.c_str() );
                groupElem->LinkEndChild( powerElem );
                powerField = getFieldValue( 2 );
                auto powerText = new TiXmlText( powerField.c_str() );
                powerElem->LinkEndChild( powerText );
            }

            auto resultElem = new TiXmlElement( "result" );
            groupElem->LinkEndChild( resultElem );

            for( int i = 0; i < 2; ++i )
            {
                String fieldName = getFieldName( i );
                String fieldValue = getFieldValue( i );

                auto fieldElem = new TiXmlElement( fieldName.c_str() );
                resultElem->LinkEndChild( fieldElem );

                auto fieldValueElem = new TiXmlText( fieldValue.c_str() );
                fieldElem->LinkEndChild( fieldValueElem );
            }

            nextRow();
        }

        TiXmlPrinter printer;
        printer.SetIndent( "	" );

        doc.Accept( &printer );
        const char *str = printer.CStr();
        String xmltext = str ? str : "";

#if _DEBUG
        doc.SaveFile( "Query.xml" );
#endif

        return xmltext;
    }

    SmartPtr<Properties> SQLiteQuery::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();

        for( auto &row : m_rows )
        {
            auto child = workphone::make_ptr<Properties>();
            properties->addChild( child );

            for( auto &field : row.fields )
            {
                child->setProperty( field.name, field.value );
            }
        }

        return properties;
    }

    SQLiteQuery::Rows::Rows( const Rows &other ) : fields( other.fields )
    {
    }

    SQLiteQuery::Rows::Rows()
    {
    }

    SQLiteQuery::Rows::~Rows()
    {
    }

    SQLiteQuery::Field::Field( const Field &other ) : name( other.name ), value( other.value )
    {
    }

    SQLiteQuery::Field::Field( const String &name, const String &value ) : name( name ), value( value )
    {
    }

    SQLiteQuery::Field::Field()
    {
    }

    SQLiteQuery::Field::~Field()
    {
    }

}  // namespace workphone
