#ifndef __Cutscene_h__
#define __Cutscene_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Serializable cutscene resource that owns a set of animation tracks.
         *
         * A cutscene is an engine asset composed of one or more tracks. Each track
         * targets a named actor in the scene and animates one property over time.
         * Keyframes within a track are stored with a timestamp and a value, and are
         * evaluated using linear interpolation.
         */
        class WPCore_API Cutscene : public Resource<IResource>
        {
        public:
            /** @brief Property type that a track can animate. */
            enum class TrackType
            {
                Position,
                Rotation,
                Scale,
                CameraFOV
            };

            /** @brief Single keyed value on a track. */
            struct Keyframe
            {
                f32 time = 0.0f;
                Vector3<real_Num> vectorValue = Vector3<real_Num>::zero();
                f32 scalarValue = 0.0f;
            };

            /** @brief A named collection of keyframes that target one actor property. */
            struct Track
            {
                String targetActorName;
                TrackType type = TrackType::Position;
                Array<Keyframe> keyframes;
            };

            static const String nameStr;
            static const String lengthStr;
            static const String loopingStr;
            static const String tracksStr;
            static const String trackStr;
            static const String targetActorStr;
            static const String trackTypeStr;
            static const String keyframesStr;
            static const String keyframeStr;
            static const String timeStr;
            static const String vectorValueStr;
            static const String scalarValueStr;

            /** @brief Constructor. */
            Cutscene();

            /** @brief Destructor. */
            ~Cutscene() override;

            /** @copydoc Resource::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource::saveToFile */
            void saveToFile( const String &filePath ) override;

            /** @copydoc Resource::loadFromFile */
            void loadFromFile( const String &filePath ) override;

            /** @copydoc Resource::save */
            void save() override;

            /** @copydoc Resource::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Resource::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @brief Sets the total duration of the cutscene in seconds. */
            void setLength( f32 length );

            /** @brief Gets the total duration of the cutscene in seconds. */
            f32 getLength() const;

            /** @brief Sets whether the cutscene loops when it reaches the end. */
            void setLooping( bool looping );

            /** @brief Returns true if the cutscene loops. */
            bool isLooping() const;

            /** @brief Adds a new track and returns its index. */
            s32 addTrack( const Track &track );

            /** @brief Removes the track at the given index. */
            void removeTrack( s32 index );

            /** @brief Returns the current set of tracks. */
            Array<Track> getTracks() const;

            /** @brief Replaces all tracks. */
            void setTracks( const Array<Track> &tracks );

            /**
             * @brief Evaluates the cutscene at the given time.
             *
             * The caller is responsible for resolving target actor names and applying
             * the returned values. Time is clamped to [0, length] unless looping is
             * enabled, in which case it wraps around.
             *
             * @param time Time in seconds.
             * @return Map from (actorName, trackType) to evaluated values.
             */
            void evaluate( f32 time, Map<Pair<String, TrackType>, Keyframe> &outValues ) const;

            /** @brief Converts a track type to a string. */
            static String trackTypeToString( TrackType type );

            /** @brief Parses a track type from a string. */
            static TrackType stringToTrackType( const String &str );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Sorts keyframes on every track by time. */
            void sortKeyframes();

            f32 m_length = 10.0f;
            bool m_looping = false;
            Array<Track> m_tracks;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // __Cutscene_h__
