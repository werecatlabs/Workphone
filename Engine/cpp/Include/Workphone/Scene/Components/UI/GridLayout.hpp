#ifndef GridLayout_h__
#define GridLayout_h__

#include <Workphone/Scene/Components/UI/LayoutContainer.hpp>

namespace workphone
{
    namespace scene
    {
        /** Grid layout container. */
        class WPCore_API GridLayout : public LayoutContainer
        {
        public:
            // Property key strings
            static const String ColumnsStr;
            static const String CellSizeStr;
            static const String SpacingStr;
            static const String ModifyChildSizeStr;

            /** Constructor. */
            GridLayout();

            /** Destructor. */
            ~GridLayout() override;

            /** @copydoc LayoutContainer::updateTransform */
            void updateTransform() override;

            /** @copydoc LayoutContainer::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc LayoutContainer::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Get the number of columns in the grid.
             * @return The number of columns in the grid.
             */
            s32 getColumnCount() const;

            /** Set the number of columns in the grid.
             * @param columnCount The number of columns in the grid.
             */
            void setColumnCount( s32 columnCount );

            /** Get the size of each cell.
             * @return The size of each cell.
             */
            Vector2<real_Num> getCellSize() const;

            /** Set the size of each cell.
             * @param cellSize The size of each cell.
             */
            void setCellSize( const Vector2<real_Num> &cellSize );

            /** Get the spacing between child elements.
             * @return The spacing between child elements.
             */
            Vector2<real_Num> getSpacing() const;

            /** Set the spacing between child elements.
             * @param spacing The spacing between child elements.
             */
            void setSpacing( const Vector2<real_Num> &spacing );

            /** Get if the child size should be modified.
             * @return If the child size should be modified.
             */
            bool getModifyChildSize() const;

            /** Set if the child size should be modified.
             * @param modifyChildSize If the child size should be modified.
             */
            void setModifyChildSize( bool modifyChildSize );

            WP_CLASS_REGISTER_DECL;

        protected:
            // Number of columns in the grid
            s32 m_columns = 3;

            // Size of each cell
            Vector2<real_Num> m_cellSize = Vector2<real_Num>( 100.0, 100.0 );

            // Spacing between child elements
            Vector2<real_Num> m_spacing = Vector2<real_Num>( 5.0, 5.0 );

            // To if the child size should be modified.
            bool m_modifyChildSize = true;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // GridLayout_h__
