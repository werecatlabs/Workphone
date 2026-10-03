#ifndef TableLayout_h__
#define TableLayout_h__

#include <Workphone/Scene/Components/UI/LayoutContainer.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class TableLayout
         * @brief Arranges child UI elements in a table (grid) layout.
         *
         * The TableLayout component organizes its children into a grid of rows and columns.
         * Each cell in the table can contain a single child element. The number of rows and columns,
         * as well as the size of each cell, can be configured. Optionally, the layout can resize its
         * content to fit the available space.
         *
         * @ingroup UI
         */
        class WPCore_API TableLayout : public LayoutContainer
        {
        public:
            // Property key strings (static const so they can be referenced without hard-coded literals)
            static const String numRowsStr;
            static const String numColumnsStr;
            static const String cellWidthStr;
            static const String cellHeightStr;
            static const String resizeContentStr;
            static const String cellNamePrefixStr;
            static const String cellNameSeparatorStr;
            static const String createButtonStr;

            /**
             * @brief Constructs a TableLayout with default settings.
             *
             * By default, the table has 3 rows and 3 columns, with each cell sized 100x50 units,
             * and content resizing enabled.
             */
            TableLayout();

            /**
             * @brief Destructor.
             */
            ~TableLayout() override;

            /**
             * @brief Updates the transform of the layout and its children.
             *
             * Recalculates the positions and sizes of child elements based on the current
             * table configuration (rows, columns, cell size, and resize settings).
             */
            void updateTransform() override;

            /**
             * @brief Gets the properties of the TableLayout.
             * @return A smart pointer to the Properties object containing the layout's properties.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the TableLayout.
             * @param properties A smart pointer to the Properties object to apply to the layout.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the number of rows in the table.
             * @return The number of rows.
             */
            s32 getNumRows() const;

            /**
             * @brief Sets the number of rows in the table.
             * @param numRows The new number of rows.
             */
            void setNumRows( s32 numRows );

            /**
             * @brief Gets the number of columns in the table.
             * @return The number of columns.
             */
            s32 getNumColumns() const;

            /**
             * @brief Sets the number of columns in the table.
             * @param numColumns The new number of columns.
             */
            void setNumColumns( s32 numColumns );

            /**
             * @brief Gets the width of each cell in the table.
             * @return The cell width.
             */
            f32 getCellWidth() const;

            /**
             * @brief Sets the width of each cell in the table.
             * @param cellWidth The new cell width.
             */
            void setCellWidth( f32 cellWidth );

            /**
             * @brief Gets the height of each cell in the table.
             * @return The cell height.
             */
            f32 getCellHeight() const;

            /**
             * @brief Sets the height of each cell in the table.
             * @param cellHeight The new cell height.
             */
            void setCellHeight( f32 cellHeight );

            /**
             * @brief Checks if the layout resizes its content to fit the available space.
             * @return True if content resizing is enabled, false otherwise.
             */
            bool getResizeContent() const;

            /**
             * @brief Sets whether the layout should resize its content to fit the available space.
             * @param resizeContent True to enable content resizing, false to disable.
             */
            void setResizeContent( bool resizeContent );

            /**
             * @brief Gets the prefix used when generated cell actors are named.
             * @return The generated cell actor name prefix.
             */
            String getCellNamePrefix() const;

            /**
             * @brief Sets the prefix used when generated cell actors are named.
             * @param cellNamePrefix The generated cell actor name prefix.
             */
            void setCellNamePrefix( const String &cellNamePrefix );

            /**
             * @brief Gets the separator used between row and column in generated cell actor names.
             * @return The generated cell actor name separator.
             */
            String getCellNameSeparator() const;

            /**
             * @brief Sets the separator used between row and column in generated cell actor names.
             * @param cellNameSeparator The generated cell actor name separator.
             */
            void setCellNameSeparator( const String &cellNameSeparator );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Number of rows in the table. */
            s32 m_numRows = 3;

            /** Number of columns in the table. */
            s32 m_numColumns = 3;

            /** Width of each cell in the table. */
            f32 m_cellWidth = 100.0f;

            /** Height of each cell in the table. */
            f32 m_cellHeight = 50.0f;

            /** Whether to resize content to fit the table layout. */
            bool m_resizeContent = true;

            /** Prefix used when generated cell actors are named. */
            String m_cellNamePrefix = String( "Cell_" );

            /** Separator used between row and column in generated cell actor names. */
            String m_cellNameSeparator = String( "_" );
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TableLayout_h__
