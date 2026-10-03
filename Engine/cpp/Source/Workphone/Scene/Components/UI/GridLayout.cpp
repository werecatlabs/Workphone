#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/GridLayout.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, GridLayout, LayoutContainer );

    const String GridLayout::ColumnsStr = String( "columns" );
    const String GridLayout::CellSizeStr = String( "cellSize" );
    const String GridLayout::SpacingStr = String( "spacing" );
    const String GridLayout::ModifyChildSizeStr = String( "modifyChildSize" );

    GridLayout::GridLayout() = default;

    GridLayout::~GridLayout() = default;

    void GridLayout::updateTransform()
    {
        try
        {
            if( isEnabled() )
            {
                auto columns = getColumnCount();
                auto cellSize = getCellSize();
                auto spacing = getSpacing();

                if( auto actor = getActor() )
                {
                    auto children = actor->getChildren();

                    auto i = 0;
                    for( auto child : children )
                    {
                        if( child->isEnabled() )
                        {
                            int row = i / columns;
                            int column = i % columns;

                            auto position = Vector2<real_Num>( ( cellSize.x + spacing.x ) * column,
                                                               ( cellSize.y + spacing.y ) * row );

                            auto transform = child->getComponent<LayoutTransform>();
                            if( transform )
                            {
                                transform->setPosition( position );

                                if( getModifyChildSize() )
                                {
                                    transform->setSize( cellSize );
                                }
                            }

                            ++i;
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> GridLayout::getProperties() const
    {
        try
        {
            auto properties = LayoutContainer::getProperties();
            properties->setProperty( ColumnsStr, m_columns );
            properties->setProperty( CellSizeStr, m_cellSize );
            properties->setProperty( SpacingStr, m_spacing );
            properties->setProperty( ModifyChildSizeStr, m_modifyChildSize );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void GridLayout::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            LayoutContainer::setProperties( properties );

            properties->getPropertyValue( ColumnsStr, m_columns );
            properties->getPropertyValue( CellSizeStr, m_cellSize );
            properties->getPropertyValue( SpacingStr, m_spacing );
            properties->getPropertyValue( ModifyChildSizeStr, m_modifyChildSize );

            if( auto actor = getActor() )
            {
                actor->updateTransform();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    s32 GridLayout::getColumnCount() const
    {
        return m_columns;
    }

    void GridLayout::setColumnCount( s32 columnCount )
    {
        m_columns = columnCount;
    }

    Vector2<real_Num> GridLayout::getCellSize() const
    {
        return m_cellSize;
    }

    void GridLayout::setCellSize( const Vector2<real_Num> &cellSize )
    {
        m_cellSize = cellSize;
    }

    Vector2<real_Num> GridLayout::getSpacing() const
    {
        return m_spacing;
    }

    void GridLayout::setSpacing( const Vector2<real_Num> &spacing )
    {
        m_spacing = spacing;
    }

    void GridLayout::setModifyChildSize( bool modifyChildSize )
    {
        m_modifyChildSize = modifyChildSize;
    }

    bool GridLayout::getModifyChildSize() const
    {
        return m_modifyChildSize;
    }

}  // namespace workphone::scene
