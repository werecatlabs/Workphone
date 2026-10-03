#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/TableCell.hpp>
#include <Workphone/Scene/Components/UI/TableLayout.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TableCell, UIComponent );

    const String TableCell::tableLayoutStr = String( "tableLayout" );
    const String TableCell::resizeChildLayoutTransformsStr = String( "resizeChildLayoutTransforms" );
    const String TableCell::updateChildLayoutTransformsStr = String( "updateChildLayoutTransforms" );

    TableCell::TableCell() = default;

    TableCell::~TableCell() = default;

    void TableCell::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        UIComponent::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void TableCell::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        UIComponent::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void TableCell::updateTransform()
    {
        if( auto actor = getActor() )
        {
            auto layoutTransform = actor->getComponent<LayoutTransform>();
            if( !layoutTransform )
            {
                return;
            }

            auto cellSize = layoutTransform->getSize();

            auto layoutTransforms = actor->getComponentsInChildren<LayoutTransform>();
            for( auto layoutTransform : layoutTransforms )
            {
                if( getResizeChildLayoutTransforms() )
                {
                    layoutTransform->setSize( cellSize );
                }

                if( getUpdateChildLayoutTransforms() )
                {
                    layoutTransform->updateTransform();
                }
            }
        }
    }

    SmartPtr<Properties> TableCell::getProperties() const
    {
        auto properties = UIComponent::getProperties();
        properties->setProperty( tableLayoutStr,
                                 workphone::static_pointer_cast<IComponent>( getTableLayout() ) );
        properties->setProperty( resizeChildLayoutTransformsStr, getResizeChildLayoutTransforms() );
        properties->setProperty( updateChildLayoutTransformsStr, getUpdateChildLayoutTransforms() );

        return properties;
    }

    void TableCell::setProperties( SmartPtr<Properties> properties )
    {
        UIComponent::setProperties( properties );

        SmartPtr<IComponent> tableLayoutComponent;
        auto resizeChildLayoutTransforms = getResizeChildLayoutTransforms();
        auto updateChildLayoutTransforms = getUpdateChildLayoutTransforms();

        properties->getPropertyValue( tableLayoutStr, tableLayoutComponent );
        properties->getPropertyValue( resizeChildLayoutTransformsStr, resizeChildLayoutTransforms );
        properties->getPropertyValue( updateChildLayoutTransformsStr, updateChildLayoutTransforms );

        if( tableLayoutComponent && tableLayoutComponent->isDerived<TableLayout>() )
        {
            setTableLayout( workphone::static_pointer_cast<TableLayout>( tableLayoutComponent ) );
        }

        setResizeChildLayoutTransforms( resizeChildLayoutTransforms );
        setUpdateChildLayoutTransforms( updateChildLayoutTransforms );

        updateTransform();
    }

    void TableCell::setTableLayout( SmartPtr<TableLayout> tableLayout )
    {
        m_tableLayout = tableLayout;
    }

    SmartPtr<TableLayout> TableCell::getTableLayout() const
    {
        return m_tableLayout.lock();
    }

    bool TableCell::getResizeChildLayoutTransforms() const
    {
        return m_resizeChildLayoutTransforms;
    }

    void TableCell::setResizeChildLayoutTransforms( bool resizeChildLayoutTransforms )
    {
        m_resizeChildLayoutTransforms = resizeChildLayoutTransforms;
    }

    bool TableCell::getUpdateChildLayoutTransforms() const
    {
        return m_updateChildLayoutTransforms;
    }

    void TableCell::setUpdateChildLayoutTransforms( bool updateChildLayoutTransforms )
    {
        m_updateChildLayoutTransforms = updateChildLayoutTransforms;
    }
}  // namespace workphone::scene
