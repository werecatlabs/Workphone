#ifndef _IDynamicLines_H_
#define _IDynamicLines_H_

#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for dynamic line graphics object.
         */
        class WPCore_API IDynamicLines : public IGraphicsObject
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IDynamicLines() override;

            /**
             * @brief Add a point to the point list.
             * @param point The point to be added.
             */
            virtual void addPoint( const Vector3<real_Num> &point ) = 0;

            /**
             * @brief Change the location of an existing point in the point list.
             * @param index The index of the point to be updated.
             * @param point The new location of the point.
             */
            virtual void setPoint( u32 index, const Vector3<real_Num> &point ) = 0;

            /**
             * @brief Return the location of an existing point in the point list.
             * @param index The index of the point.
             * @return The location of the point.
             */
            virtual Vector3<real_Num> getPoint( u32 index ) const = 0;

            /**
             * @brief Return the total number of points in the point list.
             * @return The total number of points in the point list.
             */
            virtual u32 getNumPoints() const = 0;

            /**
             * @brief Remove all points from the point list.
             */
            virtual void clear() = 0;

            /**
             * @brief Sets flag to update the mesh.
             */
            virtual void setDirty() = 0;

            /**
             * @brief Sets the operation type for rendering.
             * @param opType The new operation type.
             */
            virtual void setOperationType( u32 opType ) = 0;

            /**
             * @brief Gets the current operation type for rendering.
             * @return The current operation type.
             */
            virtual u32 getOperationType() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
