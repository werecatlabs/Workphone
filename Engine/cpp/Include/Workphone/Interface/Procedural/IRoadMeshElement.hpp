#ifndef IRoadMeshElement_h__
#define IRoadMeshElement_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Polygon3.hpp>
#include <Workphone/Interface/Procedural/IRoadSection.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IRoadMeshElement : public ISharedObject
        {
        public:
            ~IRoadMeshElement() override;

            // Parent section access
            virtual SmartPtr<IRoadSection> getParentSection() const = 0;
            virtual void setParentSection( SmartPtr<IRoadSection> section ) = 0;

            // Mesh data access
            virtual Array<Vector3<real_Num>> getVertices() const = 0;
            virtual void setVertices( const Array<Vector3<real_Num>> &vertices ) = 0;

            virtual Array<u32> getIndices() const = 0;
            virtual void setIndices( const Array<u32> &indices ) = 0;

            // Optional: polygons (if mesh is polygonal)
            virtual Array<Polygon3<real_Num>> getPolygons() const = 0;
            virtual void setPolygons( const Array<Polygon3<real_Num>> &polygons ) = 0;

            // Build/update methods
            virtual void build() = 0;
            virtual void updateMesh() = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadMeshElement_h__
