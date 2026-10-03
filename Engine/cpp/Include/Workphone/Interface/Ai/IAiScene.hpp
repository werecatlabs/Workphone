#ifndef __IAiScene_h__
#define __IAiScene_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @brief Interface describing an AI scene.
     *
     * The AI scene provides a lightweight representation of the world that
     * AI agents use for perception, navigation and decision-making.
     *
     * Implementations are responsible for maintaining collections of agents,
     * waypoints and any navigation tracks. Concrete classes decide lifecycle
     * and ownership semantics for objects stored in the scene — callers should
     * consult the concrete implementation or documentation for details.
     *
     * @note Implementations should document thread-safety guarantees. The
     *       interface itself does not impose any thread-safety requirements.
     */
    class WPCore_API IAiScene : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived destructors are invoked correctly through the interface.
         */
        ~IAiScene() override;

        // --- Agent Management ---
        /**
         * @brief Register an AI agent with this scene.
         *
         * The agent will be included in update and query operations.
         *
         * @param agent Pointer to the agent to add. Must be non-null.
         * @remarks The interface does not mandate ownership transfer. The
         *          concrete implementation should state whether it takes
         *          ownership or only holds a non-owning reference.
         */
        virtual void addAgent( SmartPtr<IAiAgent> agent ) = 0;

        /**
         * @brief Unregister an AI agent from this scene.
         *
         * After removal the agent will no longer be considered by the scene's
         * update and query operations.
         *
         * @param agent Pointer to the agent to remove. If the agent is not
         *              registered this method should be a no-op.
         */
        virtual void removeAgent( SmartPtr<IAiAgent> agent ) = 0;

        /**
         * @brief Get the list of agents currently registered with the scene.
         *
         * @return A constant reference to the vector of agent pointers.
         *         Implementations should guarantee the returned reference
         *         remains valid until the next non-const operation on the scene.
         */
        virtual Array<SmartPtr<IAiAgent>> getAgents() const = 0;

        // --- Environment Data ---
        /**
         * @brief Get the navigation track used by this scene.
         *
         * @return Pointer to the scene's track or nullptr if no track is set.
         */
        virtual SmartPtr<IAiTrack> getTrack() const = 0;

        /**
         * @brief Get the set of waypoints defined for this scene.
         *
         * @return A constant reference to the vector of waypoint pointers.
         *         May be empty if no waypoints are present.
         */
        virtual Array<SmartPtr<IAiWaypoint>> getWaypoints() const = 0;

        // --- Queries ---
        /**
         * @brief Find the nearest waypoint to the given position.
         *
         * This method performs a proximity query against the scene's waypoint
         * set and returns the closest waypoint. Implementations may use
         * spatial acceleration structures for performance where appropriate.
         *
         * @param position World-space position used for the search.
         * @return Pointer to the nearest waypoint, or nullptr if there are no
         *         waypoints in the scene.
         */
        virtual SmartPtr<IAiWaypoint> findNearestWaypoint( const Vector3<real_Num> &position ) const = 0;

        /**
         * @brief Reset the scene to its initial state.
         *
         * This should clear transient state, reinitialize agents/waypoints as
         * required and make the scene ready for a fresh simulation run.
         */
        virtual void reset() = 0;

        /** Registration macro required by the framework. */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // __IAiScene_h__
