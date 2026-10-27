#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/MeshFace.hpp"

namespace workphone
{
    namespace procedural
    {
        MeshFace::MeshFace() = default;

        MeshFace::MeshFace( const MeshFace &other ) : ISharedObject()
        {
            setGeometry( other.Points, other.UVs );
        }

        MeshFace &MeshFace::operator=( const MeshFace &other )
        {
            if( this != &other )
            {
                setGeometry( other.Points, other.UVs );
            }
            return *this;
        }

        MeshFace::~MeshFace() = default;

        void MeshFace::setGeometry( const Array<Vector3F> &points, const Array<Vector2F> &uvs )
        {
            auto nextPoints = makeSourceBudgetSnapshot( points );
            auto nextUVs = makeSourceBudgetSnapshot( uvs );
            Points.swap( nextPoints );
            UVs.swap( nextUVs );
        }

        void MeshFace::unload( SmartPtr<ISharedObject> data )
        {
            {
                auto points = makeSourceBudgetSnapshot<Vector3F>();
                auto uvs = makeSourceBudgetSnapshot<Vector2F>();
                Points.swap( points );
                UVs.swap( uvs );
            }
            ISharedObject::unload( data );
        }
    }  // end namespace procedural
}  // namespace workphone
