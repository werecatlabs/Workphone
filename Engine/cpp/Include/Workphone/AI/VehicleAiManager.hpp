#ifndef CVehicleAiManager_h__
#define CVehicleAiManager_h__

#include <memory>

#include <Workphone/Interface/Ai/IVehicleAiManager.hpp>

namespace workphone
{
    class MLP;

    /**
     * @class VehicleAiManager
     * @brief Concrete implementation of the vehicle AI manager.
     *
     * Responsible for coordinating a collection of AI-controlled vehicles,
     * managing their associated track, and interfacing with the underlying
     * neural network (MLP) for decision making.
     */
    class WPCore_API VehicleAiManager : public IVehicleAiManager
    {
    public:
        /**
         * @brief Default constructor.
         */
        VehicleAiManager();
        /**
         * @brief Destructor.
         */
        ~VehicleAiManager() override;

        /**
         * @brief Adds a vehicle to the AI manager.
         * @param vehicle The vehicle to be added.
         */
        void addVehicle( SmartPtr<vehicle::IVehicle> vehicle ) override;
        /**
         * @brief Removes a vehicle from the AI manager.
         * @param vehicle The vehicle to be removed.
         */
        void removeVehicle( SmartPtr<vehicle::IVehicle> vehicle ) override;

        /**
         * @brief Gets the list of all vehicles managed by the AI.
         * @return An array of smart pointers to the managed vehicles.
         */
        Array<SmartPtr<vehicle::IVehicle>> getVehicles() const override;
        /**
         * @brief Sets the list of vehicles managed by the AI.
         * @param vehicles An array of smart pointers to the vehicles.
         */
        void setVehicles( const Array<SmartPtr<vehicle::IVehicle>> &vehicles ) override;

        /**
         * @brief Gets the track used by the AI vehicles.
         * @return A smart pointer to the track.
         */
        SmartPtr<IAiTrack> getTrack() const;
        /**
         * @brief Sets the track used by the AI vehicles.
         * @param track A smart pointer to the track.
         */
        void setTrack( SmartPtr<IAiTrack> track );

        /**
         * @brief Gets the Multi-Layer Perceptron (MLP) used for AI calculations.
         * @return A shared pointer to the MLP.
         */
        SharedPtr<MLP> getMLP() const;
        /**
         * @brief Sets the Multi-Layer Perceptron (MLP) used for AI calculations.
         * @param mlp A shared pointer to the MLP.
         */
        void setMLP( SharedPtr<MLP> mlp );

        /**
         * @brief Gets the number of action variables for the AI.
         * @return The number of action variables.
         */
        unsigned long getNumberOfActionVariables() const;
        /**
         * @brief Sets the number of action variables for the AI.
         * @param numberOfActionVariables The number of action variables to set.
         */
        void setNumberOfActionVariables( unsigned long numberOfActionVariables );

        /**
         * @brief Gets the number of state variables for the AI.
         * @return The number of state variables.
         */
        unsigned long getNumberOfStateVariables() const;
        /**
         * @brief Sets the number of state variables for the AI.
         * @param numberOfStateVariables The number of state variables to set.
         */
        void setNumberOfStateVariables( unsigned long numberOfStateVariables );

        WP_CLASS_REGISTER_DECL;

    protected:
        Array<SmartPtr<vehicle::IVehicle>> m_vehicles;  ///< The collection of managed vehicles.
        SmartPtr<IAiTrack> m_track;                     ///< The AI track associated with this manager.
        SharedPtr<MLP> m_pMLP;                          ///< The neural network used for AI.
        unsigned long m_ulNumberOfActionVariables = 0;  ///< Total number of output action variables.
        unsigned long m_ulNumberOfStateVariables = 0;   ///< Total number of input state variables.
    };
}  // namespace workphone

#endif  // CVehicleAiManager_h__
