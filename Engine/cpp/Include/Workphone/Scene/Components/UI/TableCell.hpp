#ifndef TableCell_h__
#define TableCell_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Represents a single cell within a table layout UI component.
         *
         * TableCell is a UI component that can be used as a child of TableLayout to display content
         * in a tabular structure. It manages its own transform and maintains a reference to its parent
         * TableLayout.
         */
        class WPCore_API TableCell : public UIComponent
        {
        public:
            static const String tableLayoutStr;
            static const String resizeChildLayoutTransformsStr;
            static const String updateChildLayoutTransformsStr;

            /**
             * @brief Constructs a new TableCell instance.
             */
            TableCell();

            /**
             * @brief Destroys the TableCell instance.
             */
            ~TableCell() override;

            /**
             * @brief Loads the TableCell with the specified data.
             *
             * This method initializes the TableCell using the provided shared data object.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the TableCell and releases associated resources.
             *
             * This method cleans up the TableCell using the provided shared data object.
             * @param data Shared object containing cleanup data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the transform of the TableCell.
             *
             * This method recalculates the TableCell's transform, typically called when the layout
             * changes.
             */
            void updateTransform() override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the parent TableLayout of this cell.
             * @return SmartPtr<TableLayout> Reference to the parent TableLayout, or nullptr if not set.
             */
            SmartPtr<TableLayout> getTableLayout() const;

            /**
             * @brief Sets the parent TableLayout for this cell.
             * @param tableLayout Smart pointer to the TableLayout to associate with this cell.
             */
            void setTableLayout( SmartPtr<TableLayout> tableLayout );

            /**
             * @brief Gets whether child layout transforms should be resized to match the cell size.
             * @return True if child layout transforms are resized, false otherwise.
             */
            bool getResizeChildLayoutTransforms() const;

            /**
             * @brief Sets whether child layout transforms should be resized to match the cell size.
             * @param resizeChildLayoutTransforms True to resize child layout transforms, false
             * otherwise.
             */
            void setResizeChildLayoutTransforms( bool resizeChildLayoutTransforms );

            /**
             * @brief Gets whether child layout transforms should be updated after cell layout changes.
             * @return True if child layout transforms are updated, false otherwise.
             */
            bool getUpdateChildLayoutTransforms() const;

            /**
             * @brief Sets whether child layout transforms should be updated after cell layout changes.
             * @param updateChildLayoutTransforms True to update child layout transforms, false
             * otherwise.
             */
            void setUpdateChildLayoutTransforms( bool updateChildLayoutTransforms );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Weak reference to the parent TableLayout.
             */
            WeakPtr<TableLayout> m_tableLayout;

            /** Whether child layout transforms should be resized to match the cell size. */
            bool m_resizeChildLayoutTransforms = true;

            /** Whether child layout transforms should be updated after cell layout changes. */
            bool m_updateChildLayoutTransforms = true;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TableCell_h__
