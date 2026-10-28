#ifndef LODSystem_h__
#define LODSystem_h__

#include <Workphone/Scene/Systems/ComponentSystem.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Atomics/Atomic.hpp>
#include <Workphone/Memory/SharedPtr.hpp>
#include <memory>
#include <Workphone/System/SourceBudgetContainers.hpp>
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
            struct LODBatch : SourceBudgetObject
            {
                Vector3<real_Num> cameraPosition = Vector3<real_Num>::zero();
                f32 verticalFovRadians = Math<f32>::DegToRad( 60.0f );
                f32 orthographicHeight = 10.0f;
                f32 nearClipDistance = 0.1f;
                bool orthographic = false;

                Array<Vector3<real_Num>> worldReferencePoints{ Allocator<Vector3<real_Num>>( sourceBudget ) };
                Array<f32> worldRadii{ Allocator<f32>( sourceBudget ) };
                Array<u32> detailOffsets{ Allocator<u32>( sourceBudget ) };
                Array<u32> detailCounts{ Allocator<u32>( sourceBudget ) };
                Array<f32> lodBiases{ Allocator<f32>( sourceBudget ) };
                Array<f32> hysteresis{ Allocator<f32>( sourceBudget ) };
                Array<s32> previousLODs{ Allocator<s32>( sourceBudget ) };
                Array<s32> forcedLODs{ Allocator<s32>( sourceBudget ) };
                Array<u8> cullBelowLastLOD{ Allocator<u8>( sourceBudget ) };

                Array<u32> slots{ Allocator<u32>( sourceBudget ) };
                Array<u64> generations{ Allocator<u64>( sourceBudget ) };
                Array<u64> revisions{ Allocator<u64>( sourceBudget ) };
                Array<u32> thresholdOffsets{ Allocator<u32>( sourceBudget ) };
                Array<u32> thresholdCounts{ Allocator<u32>( sourceBudget ) };
                Array<f32> thresholds{ Allocator<f32>( sourceBudget ) };
                Array<s32> results{ Allocator<s32>( sourceBudget ) };

                Atomic<u32> remainingJobs{ 0 };
            };

            SharedPtr<LODBatch> buildBatch() const;
            void dispatchBatch( const SharedPtr<LODBatch> &batch );
            void applyBatch( const SharedPtr<LODBatch> &batch );

            static void calculateRange( const SharedPtr<LODBatch> &batch, size_t begin, size_t end );
            static s32 selectLOD( f32 screenRelativeHeight, const f32 *thresholds, size_t thresholdCount,
                                  s32 previousLOD, s32 forcedLOD, f32 hysteresis,
                                  bool cullBelowLastLOD );

            Array<LODGroup *> m_groups{ Allocator<LODGroup *>( sourceBudget ) };
            Array<u64> m_generations{ Allocator<u64>( sourceBudget ) };
            Array<s32> m_currentLODs{ Allocator<s32>( sourceBudget ) };
            Array<u64> m_appliedRevisions{ Allocator<u64>( sourceBudget ) };
            std::unordered_map<LODGroup *, u32, std::hash<LODGroup *>, std::equal_to<LODGroup *>,
                SourceBudgetAllocator<std::pair<LODGroup * const, u32>>> m_groupSlots;

            SharedPtr<LODBatch> m_pendingBatch;

            u64 m_nextGeneration = 1;
            f32 m_globalLODBias = 1.0f;
            u32 m_grainSize = 64;
            View m_viewOverride;
            bool m_hasViewOverride = false;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // LODSystem_h__
