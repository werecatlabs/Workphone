#ifndef DynamicLines_h__
#define DynamicLines_h__

#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IDynamicLines.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief A dynamic, editable collection of 3D points used for line rendering.
         *
         * DynamicLines stores a list of 3D points which can be updated at runtime.
         * The stored points are intended to be rendered using a render operation
         * type (for example, line strip, line list, etc.). The object tracks a
         * dirty flag so renderers can know when the vertex data changed and needs
         * to be re-uploaded or otherwise refreshed.
         *
         * This class implements the IDynamicLines interface and extends the
         * GraphicsObject base for integration with the engine's graphics system.
         */
        class WPCore_API DynamicLines : public GraphicsObject<IDynamicLines>
        {
        public:
            /**
             * @brief Construct a new DynamicLines instance.
             *
             * Initializes an empty point list. The default render operation type
             * is set to OT_LINE_STRIP (implementation-defined value 3).
             */
            DynamicLines();

            /**
             * @brief Destroy the DynamicLines instance.
             */
            ~DynamicLines() override;

            /**
             * @brief Append a point to the end of the internal point list.
             *
             * Marks the object dirty so dependent systems know the geometry has changed.
             *
             * @param point 3D point to append.
             */
            void addPoint( const Vector3<real_Num> &point ) override;

            /**
             * @brief Replace the point at the specified index.
             *
             * Index must be a valid index into the current points array.
             * This operation marks the object dirty.
             *
             * @param index Zero-based index of the point to replace.
             * @param point New 3D point value.
             */
            void setPoint( u32 index, const Vector3<real_Num> &point ) override;

            /**
             * @brief Retrieve a copy of the point at the given index.
             *
             * The returned Vector3 is a value copy. Caller is responsible for bounds
             * checking if necessary (behavior for out-of-range index is determined
             * by the implementation).
             *
             * @param index Zero-based index of the requested point.
             * @return Vector3<real_Num> The point at the given index.
             */
            Vector3<real_Num> getPoint( u32 index ) const override;

            /**
             * @brief Get the number of points currently stored.
             *
             * @return u32 Number of stored points.
             */
            u32 getNumPoints() const override;

            /**
             * @brief Remove all points from the collection.
             *
             * Clears the internal point list and marks the object dirty.
             */
            void clear() override;

            /**
             * @brief Mark the dynamic lines as dirty.
             *
             * When dirty, renderers or upload subsystems should re-evaluate or re-upload
             * vertex data prior to rendering.
             */
            void setDirty() override;

            /**
             * @brief Set the render operation type used to draw the points.
             *
             * The operation type corresponds to the engine's RenderOperationType enum
             * (for example: OT_POINT_LIST, OT_LINE_LIST, OT_LINE_STRIP, etc.).
             *
             * @param opType Operation type id (enum value).
             */
            void setOperationType( u32 opType ) override;

            /**
             * @brief Get the current render operation type.
             *
             * @return u32 Operation type id currently set on this object.
             */
            u32 getOperationType() const override;

            /**
             * @brief Create a copy of this DynamicLines object.
             *
             * The clone will produce a new IGraphicsObject instance with the same
             * point data and operation type. A name may be supplied for the cloned object.
             *
             * @param name Optional name to assign to the new object.
             * @return SmartPtr<IGraphicsObject> Smart pointer owning the cloned object.
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Obtain the underlying native graphics object pointer.
             *
             * Implementation-specific method used by the engine to retrieve the
             * concrete low-level graphics object. The pointer is returned through
             * the provided void**.
             *
             * @param ppObject Pointer to receive the underlying object pointer.
             */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        private:
            /** Ordered list of 3D points that make up the dynamic lines. */
            Array<Vector3<real_Num>> m_points;

            /**
             * Render operation type used when drawing this collection of points.
             * Default value set to 3 which corresponds to OT_LINE_STRIP in this
             * engine's RenderOperationType definition.
             */
            u32 m_operationType = 3;

            /** True when points or operation type have changed and need reprocessing. */
            bool m_dirty = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // DynamicLines_h__
