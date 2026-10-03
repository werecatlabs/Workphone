#ifndef LODSystem_h__
#define LODSystem_h__

#include <Workphone/Scene/Systems/ComponentSystem.hpp>
#include <Workphone/Math/Vector3.hpp>

#include <atomic>
#include <memory>
#include <unordered_map>

namespace workphone
{
    namespace scene
    {
        class Camera;
        class LODGroup;

        /**
         * @brief Data-oriented, job-system-backed level-of-detail selector.
         *
         * The system snapshots component and camera state into flat structure-of-arrays buffers.
         * Worker jobs only read those buffers and write disjoint result ranges. Completed results
         * are consumed on the scene/application thread on a later update, where renderer visibility
         * is safe to change.
         */
        class WPCore_API LODSystem : public ComponentSystem
        {
        public:
            /** Snapshot for applications that render without a scene Camera component. */
            struct View
            {
                Vector3<real_Num> position = Vector3<real_Num>::zero();
                f32 verticalFovRadians = Math<f32>::DegToRad( 60.0f );
                f32 orthographicHeight = 10.0f;
                f32 nearClipDistance = 0.1f;
                f32 lodBias = 1.0f;
                bool orthographic = false;
            };

            LODSystem();
            ~LODSystem() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            u32 addComponent( SmartPtr<IComponent> component ) override;
            void removeComponent( SmartPtr<IComponent> component ) override;
            void removeComponent( u32 id ) override;

            void setGlobalLODBias( f32 bias );
            f32 getGlobalLODBias() const;

            /** Override automatic camera discovery; publishing a view never selects LODs itself. */
            void setViewOverride( const View &view );
            void clearViewOverride();

            void setGrainSize( u32 grainSize );
            u32 getGrainSize() const;

            /** Pure helpers exposed for tooling and focused unit tests. */
            static f32 calculatePerspectiveScreenRelativeHeight( f32 worldRadius, f32 distance,
                                                                 f32 verticalFovRadians,
                                                                 f32 lodBias = 1.0f );
            static f32 calculateOrthographicScreenRelativeHeight( f32 worldRadius,
                                                                  f32 orthographicHeight,
                                                                  f32 lodBias = 1.0f );
            static s32 selectLOD( f32 screenRelativeHeight, const Array<f32> &thresholds,
                                  s32 previousLOD, s32 forcedLOD, f32 hysteresis,
                                  bool cullBelowLastLOD );

            WP_CLASS_REGISTER_DECL;

        protected:
            void reserveData( size_t size ) override;

        private:
            struct LODBatch
            {
                Vector3<real_Num> cameraPosition = Vector3<real_Num>::zero();
                f32 verticalFovRadians = Math<f32>::DegToRad( 60.0f );
                f32 orthographicHeight = 10.0f;
                f32 nearClipDistance = 0.1f;
                bool orthographic = false;

                Array<Vector3<real_Num>> worldReferencePoints;
                Array<f32> worldRadii;
                Array<f32> lodBiases;
                Array<f32> hysteresis;
                Array<s32> previousLODs;
                Array<s32> forcedLODs;
                Array<u8> cullBelowLastLOD;

                Array<u32> slots;
                Array<u64> generations;
                Array<u64> revisions;
                Array<u32> thresholdOffsets;
                Array<u32> thresholdCounts;
                Array<f32> thresholds;
                Array<s32> results;

                std::atomic<u32> remainingJobs{ 0 };
            };

            std::shared_ptr<LODBatch> buildBatch() const;
            void dispatchBatch( const std::shared_ptr<LODBatch> &batch );
            void applyBatch( const std::shared_ptr<LODBatch> &batch );

            static void calculateRange( const std::shared_ptr<LODBatch> &batch, size_t begin,
                                        size_t end );
            static s32 selectLOD( f32 screenRelativeHeight, const f32 *thresholds, size_t thresholdCount,
                                  s32 previousLOD, s32 forcedLOD, f32 hysteresis,
                                  bool cullBelowLastLOD );

            Array<LODGroup *> m_groups;
            Array<u64> m_generations;
            Array<s32> m_currentLODs;
            Array<u64> m_appliedRevisions;
            std::unordered_map<LODGroup *, u32> m_groupSlots;

            std::shared_ptr<LODBatch> m_pendingBatch;

            u64 m_nextGeneration = 1;
            f32 m_globalLODBias = 1.0f;
            u32 m_grainSize = 64;
            View m_viewOverride;
            bool m_hasViewOverride = false;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // LODSystem_h__
