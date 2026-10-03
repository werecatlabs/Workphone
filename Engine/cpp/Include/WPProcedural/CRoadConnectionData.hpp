#ifndef CRoadConnectionData_h__
#define CRoadConnectionData_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IRoadConnectionData.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Math/Line2.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @class CRoadConnectionData
         * @brief Concrete implementation of IRoadConnectionData.
         *
         * Stores per-road descriptor data for one road arm in a road connection
         * (e.g. which road, which marker index, which connection slot, and the
         * world-space transform of the attachment point).  All fields are
         * accessible via the property system for game-editor inspection.
         */
        class WPProcedural_API CRoadConnectionData : public IRoadConnectionData
        {
        public:
            CRoadConnectionData();
            ~CRoadConnectionData() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            void setRoad( SmartPtr<IRoad> road ) override;
            SmartPtr<IRoad> getRoad() const override;

            void setMarker( s32 marker ) override;
            int getMarker() const override;

            void setConnection( s32 connection ) override;
            int getConnection() const override;

            void setTransform( Transform3<real_Num> transform ) override;
            Transform3<real_Num> getTransform() const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IRoad> m_road;            ///< The road this data record describes.
            Transform3<real_Num> m_transform;  ///< World-space attachment transform.
            s32 m_connection = -1;             ///< Index of the connection slot on the junction mesh.
            s32 m_marker = -1;                 ///< Marker (node) index on the road spine.
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // CRoadConnectionData_h__
