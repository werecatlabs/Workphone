#ifndef LODGroup_h__
#define LODGroup_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace scene
    {
        class LODSystem;
        class Renderer;

        /**
         * @brief A single visual level in an LODGroup.
         *
         * Levels are ordered from highest to lowest screen-relative height. A renderer should
         * only occur in one level.
         */
        struct LODLevel
        {
            /** Minimum fraction of the viewport height occupied by this level. */
            f32 screenRelativeHeight = 0.0f;

            /** Renderers that are visible while this level is selected. */
            Array<SmartPtr<Renderer>> renderers;

            /** Reserved for a future cross-fade implementation. */
            f32 fadeTransitionWidth = 0.0f;
        };

        /** Detail bounds for members of a merged batch; independent of its culling bounds. */
        struct LODDetailBound
        {
            Vector3<real_Num> centre = Vector3<real_Num>::zero();
            f32 diameter = 1.0f;
        };

        /**
         * @brief Defines the visual levels and bounds used by the data-oriented LOD system.
         *
         * LODGroup stores authoring data only. LODSystem snapshots that data into flat arrays
         * before worker jobs calculate the selected levels.
         */
        class WPCore_API LODGroup : public Component
        {
        public:
            LODGroup();
            ~LODGroup() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            void updateVisibility() override;

            void addLevel( const LODLevel &level );
            void addLevel( f32 screenRelativeHeight, const Array<SmartPtr<Renderer>> &renderers );
            void setLevel( size_t index, const LODLevel &level );
            void removeLevel( size_t index );
            void clearLevels();

            Array<LODLevel> getLevels() const;
            void setLevels( const Array<LODLevel> &levels );
            size_t getNumLevels() const;

            void setLocalReferencePoint( const Vector3<real_Num> &referencePoint );
            Vector3<real_Num> getLocalReferencePoint() const;

            /** Set the approximate local-space diameter of the group. */
            void setSize( f32 size );
            f32 getSize() const;

            /** Retain detail whenever any member needs it. Empty uses the group size. */
            void setDetailBounds( const Array<LODDetailBound> &bounds );
            Array<LODDetailBound> getDetailBounds() const;

            /** Set the per-group LOD bias. Values above one retain detail for longer. */
            void setLODBias( f32 bias );
            f32 getLODBias() const;

            void setHysteresis( f32 hysteresis );
            f32 getHysteresis() const;

            /** Set a forced level, or -1 to use automatic selection. */
            void setForcedLOD( s32 lod );
            s32 getForcedLOD() const;

            void setCullBelowLastLOD( bool cull );
            bool getCullBelowLastLOD() const;

            void setLODEnabled( bool enabled );
            bool isLODEnabled() const;

            void sortLevels();
            bool validate( String *errorMessage = nullptr ) const;

            /** Recalculate the reference point and diameter from the owning actor's bounds. */
            void recalculateBounds();

            WP_CLASS_REGISTER_DECL;

        protected:
            friend class LODSystem;

            void applyLOD( s32 lodIndex );
            void restoreAllRenderers();
            u64 getRevision() const;
            void incrementRevision();

        private:
            static const String levelsStr;
            static const String levelStr;
            static const String screenRelativeHeightStr;
            static const String renderersStr;
            static const String fadeTransitionWidthStr;
            static const String localReferencePointStr;
            static const String sizeStr;
            static const String lodBiasStr;
            static const String hysteresisStr;
            static const String forcedLODStr;
            static const String cullBelowLastLODStr;
            static const String lodEnabledStr;
            static const String recalculateBoundsStr;

            Array<LODLevel> m_levels;
            Array<LODDetailBound> m_detailBounds;
            Vector3<real_Num> m_localReferencePoint = Vector3<real_Num>::zero();
            f32 m_size = 1.0f;
            f32 m_lodBias = 1.0f;
            f32 m_hysteresis = 0.1f;
            s32 m_forcedLOD = -1;
            bool m_cullBelowLastLOD = true;
            bool m_lodEnabled = true;
            atomic_u64 m_revision = 1;
            mutable RecursiveMutex m_lodMutex;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // LODGroup_h__
