#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/TableLayout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/TableCell.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TableLayout, LayoutContainer );

    // Static property key definitions
    const String TableLayout::numRowsStr = String( "numRows" );
    const String TableLayout::numColumnsStr = String( "numColumns" );
    const String TableLayout::cellWidthStr = String( "cellWidth" );
    const String TableLayout::cellHeightStr = String( "cellHeight" );
    const String TableLayout::resizeContentStr = String( "resizeContent" );
    const String TableLayout::cellNamePrefixStr = String( "cellNamePrefix" );
    const String TableLayout::cellNameSeparatorStr = String( "cellNameSeparator" );
    const String TableLayout::createButtonStr = String( "create" );

    TableLayout::TableLayout() = default;

    TableLayout::~TableLayout() = default;

    void TableLayout::updateTransform()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto actor = getActor();
        auto contentActor = actor;

        auto dirty = false;

        //if( !m_tableParent )
        //{
        //    m_tableParent = sceneManager->createActor();
        //    m_tableParent->setName( "TableParent" );

        //    actor->addChild( m_tableParent );

        //    dirty = true;
        //}

        auto numRows = getNumRows();
        auto numColumns = getNumColumns();

        auto cellWidth = getCellWidth();
        auto cellHeight = getCellHeight();

        auto cellNamePrefix = getCellNamePrefix();
        auto cellNameSeparator = getCellNameSeparator();

        for( s32 row = 0; row < numRows; row++ )
        {
            for( s32 col = 0; col < numColumns; col++ )
            {
                if( contentActor )
                {
                    // Create a cell GameObject
                    const auto rowStr = StringUtil::toString( row );
                    const auto colStr = StringUtil::toString( col );
                    const auto cellName = cellNamePrefix + rowStr + cellNameSeparator + colStr;

                    auto cell = contentActor->findChild( cellName );
                    if( !cell )
                    {
                        cell = sceneManager->createActor();
                    }

                    if( cell )
                    {
                        cell->setName( cellName );

                        if( cell->getParent() != contentActor )
                        {
                            contentActor->addChild( cell );
                        }

                        // Add a transform component to the cell
                        auto transform = cell->getComponent<LayoutTransform>();
                        if( !transform )
                        {
                            transform = cell->addComponent<LayoutTransform>();
                        }

                        if( transform )
                        {
                            transform->setSize( Vector2<real_Num>( cellWidth, cellHeight ) );
                            transform->setPosition(
                                Vector2<real_Num>( col * cellWidth, row * cellHeight ) );
                        }

                        auto tableCell = cell->getComponent<TableCell>();
                        if( !tableCell )
                        {
                            tableCell = cell->addComponent<TableCell>();
                        }

                        if( tableCell )
                        {
                            tableCell->setTableLayout( this );
                        }

                        dirty = true;
                    }
                }
            }
        }

        if( dirty )
        {
            applicationManager->triggerEvent( EventType::Scene, IEvent::addActor, Array<Parameter>(),
                                              this, actor, nullptr );
        }
    }

    SmartPtr<Properties> TableLayout::getProperties() const
    {
        auto properties = LayoutContainer::getProperties();
        properties->setProperty( numRowsStr, getNumRows() );
        properties->setProperty( numColumnsStr, getNumColumns() );

        properties->setProperty( cellWidthStr, getCellWidth() );
        properties->setProperty( cellHeightStr, getCellHeight() );
        properties->setProperty( resizeContentStr, getResizeContent() );
        properties->setProperty( cellNamePrefixStr, getCellNamePrefix() );
        properties->setProperty( cellNameSeparatorStr, getCellNameSeparator() );

        properties->setButtonPressed( createButtonStr );

        return properties;
    }

    void TableLayout::setProperties( SmartPtr<Properties> properties )
    {
        LayoutContainer::setProperties( properties );

        auto numRows = getNumRows();
        auto numColumns = getNumColumns();
        auto cellWidth = getCellWidth();
        auto cellHeight = getCellHeight();
        auto resizeContent = getResizeContent();
        auto cellNamePrefix = getCellNamePrefix();
        auto cellNameSeparator = getCellNameSeparator();

        properties->getPropertyValue( numRowsStr, numRows );
        properties->getPropertyValue( numColumnsStr, numColumns );

        properties->getPropertyValue( cellWidthStr, cellWidth );
        properties->getPropertyValue( cellHeightStr, cellHeight );
        properties->getPropertyValue( resizeContentStr, resizeContent );
        properties->getPropertyValue( cellNamePrefixStr, cellNamePrefix );
        properties->getPropertyValue( cellNameSeparatorStr, cellNameSeparator );

        setNumRows( numRows );
        setNumColumns( numColumns );
        setCellWidth( cellWidth );
        setCellHeight( cellHeight );
        setResizeContent( resizeContent );
        setCellNamePrefix( cellNamePrefix );
        setCellNameSeparator( cellNameSeparator );

        updateTransform();
    }

    void TableLayout::setResizeContent( bool resizeContent )
    {
        m_resizeContent = resizeContent;
    }

    bool TableLayout::getResizeContent() const
    {
        return m_resizeContent;
    }

    void TableLayout::setCellHeight( f32 cellHeight )
    {
        m_cellHeight = cellHeight;
    }

    f32 TableLayout::getCellHeight() const
    {
        return m_cellHeight;
    }

    void TableLayout::setCellWidth( f32 cellWidth )
    {
        m_cellWidth = cellWidth;
    }

    f32 TableLayout::getCellWidth() const
    {
        return m_cellWidth;
    }

    void TableLayout::setNumColumns( s32 numColumns )
    {
        m_numColumns = numColumns;
    }

    s32 TableLayout::getNumColumns() const
    {
        return m_numColumns;
    }

    void TableLayout::setNumRows( s32 numRows )
    {
        m_numRows = numRows;
    }

    s32 TableLayout::getNumRows() const
    {
        return m_numRows;
    }

    String TableLayout::getCellNamePrefix() const
    {
        return m_cellNamePrefix;
    }

    void TableLayout::setCellNamePrefix( const String &cellNamePrefix )
    {
        m_cellNamePrefix = cellNamePrefix;
    }

    String TableLayout::getCellNameSeparator() const
    {
        return m_cellNameSeparator;
    }

    void TableLayout::setCellNameSeparator( const String &cellNameSeparator )
    {
        m_cellNameSeparator = cellNameSeparator;
    }
}  // namespace workphone::scene
