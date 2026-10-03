#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/VehicleAiManager.hpp>
#include <Workphone/AI/MLP.hpp>
#include <Workphone/Interface/Ai/IAiTrack.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <algorithm>
#include <utility>

namespace workphone
{
    namespace
    {
        constexpr unsigned long defaultActionVariableCount = 6;
        constexpr unsigned long defaultStateVariableCount = 1;
        constexpr auto neuralNetworkFileName = "VehicleAi.mlp";
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, VehicleAiManager, IVehicleAiManager );

    VehicleAiManager::VehicleAiManager() :
        m_ulNumberOfActionVariables( defaultActionVariableCount ),
        m_ulNumberOfStateVariables( defaultStateVariableCount )
    {
    }

    VehicleAiManager::~VehicleAiManager()
    {
        if( m_pMLP )
        {
            m_pMLP->Save( neuralNetworkFileName );
        }
    }

    auto VehicleAiManager::getVehicles() const -> Array<SmartPtr<vehicle::IVehicle>>
    {
        return m_vehicles;
    }

    void VehicleAiManager::setVehicles( const Array<SmartPtr<vehicle::IVehicle>> &vehicles )
    {
        m_vehicles = vehicles;
    }

    auto VehicleAiManager::getTrack() const -> SmartPtr<IAiTrack>
    {
        return m_track;
    }

    void VehicleAiManager::setTrack( SmartPtr<IAiTrack> track )
    {
        m_track = track;
    }

    auto VehicleAiManager::getMLP() const -> SharedPtr<MLP>
    {
        return m_pMLP;
    }

    void VehicleAiManager::setMLP( SharedPtr<MLP> mlp )
    {
        m_pMLP = mlp;
    }

    auto VehicleAiManager::getNumberOfActionVariables() const -> unsigned long
    {
        return m_ulNumberOfActionVariables;
    }

    void VehicleAiManager::setNumberOfActionVariables( unsigned long numberOfActionVariables )
    {
        m_ulNumberOfActionVariables = numberOfActionVariables;
    }

    auto VehicleAiManager::getNumberOfStateVariables() const -> unsigned long
    {
        return m_ulNumberOfStateVariables;
    }

    void VehicleAiManager::setNumberOfStateVariables( unsigned long numberOfStateVariables )
    {
        m_ulNumberOfStateVariables = numberOfStateVariables;
    }

    void VehicleAiManager::addVehicle( SmartPtr<vehicle::IVehicle> vehicle )
    {
        m_vehicles.push_back( vehicle );
    }

    void VehicleAiManager::removeVehicle( SmartPtr<vehicle::IVehicle> vehicle )
    {
        auto it = std::find( m_vehicles.begin(), m_vehicles.end(), vehicle );
        if( it != m_vehicles.end() )
        {
            m_vehicles.erase( it );
        }
    }
}  // namespace workphone
