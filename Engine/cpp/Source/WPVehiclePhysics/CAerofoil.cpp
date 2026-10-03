#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAerofoil.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace vehicle
    {
        CAerofoil::CAerofoil()
        {
            setCL( workphone::make_ptr<LookupCurve<real_Num>>() );
            setCD( workphone::make_ptr<LookupCurve<real_Num>>() );
            setCM( workphone::make_ptr<LookupCurve<real_Num>>() );
        }

        CAerofoil::~CAerofoil()
        {
        }

        // Array<std::pair<real_Num, real_Num>> getPoints(
        //     Array<data::aircraft_curve_value> values )
        //{
        //     Array<std::pair<real_Num, real_Num>> points;
        //     points.reserve( values.size() );

        //    for(auto &p : values)
        //    {
        //        points.push_back( std::pair<real_Num, real_Num>( p.time, p.value ) );
        //    }

        //    return points;
        //}

        // std::array<std::pair<real_Num, real_Num>, 360> CAerofoil::getPointsArray(
        //     Array<data::aircraft_curve_value> values )
        //{
        //     if(m_reverseValues)
        //     {
        //         auto data = Deque<data::aircraft_curve_value>( values.begin(), values.end() );

        //        auto front = data.front();
        //        data.erase( data.begin() );

        //        std::reverse( data.begin(), data.end() );

        //        data.push_front( front );

        //        for(auto &p : data)
        //        {
        //            p.time = -p.time;
        //        }

        //        //values = Array<data::aircraft_curve_value>( data.begin(), data.end() );
        //    }

        //    std::array<std::pair<real_Num, real_Num>, 360> points;

        //    if(values.size() <= 360)
        //    {
        //        s32 count = 0;
        //        for(auto &p : values)
        //        {
        //            points[count++] = std::pair<real_Num, real_Num>( p.time, p.value );
        //        }
        //    }
        //    else
        //    {
        //        auto spline = fb::make_ptr<LinearSpline1<real_Num>>();

        //        Array<std::pair<real_Num, real_Num>> splinePoints;
        //        for(auto curvePoint : values)
        //        {
        //            splinePoints.push_back( std::pair<real_Num, real_Num>( curvePoint.time,
        //            curvePoint.value ) );
        //        }

        //        spline->setPoints( splinePoints );

        //        for(int i = 0; i < 360; ++i)
        //        {
        //            s32 index = i - 180;
        //            points[index + 180] = std::pair<real_Num, real_Num>(
        //                index, spline->interpolate( index ) );
        //        }
        //    }

        //    return points;
        //}

        void CAerofoil::load( const std::string &fileName )
        {
            //        WP_LOG( "Airfoil load" );
            //
            //        WP_ASSERT( StringUtil::isNullOrEmpty(fileName) == false );
            //
            //        auto callback = m_aircraft->getCallback();
            //        WP_ASSERT( callback );
            //
            //        if(callback)
            //        {
            //            auto dataStr = callback->getData( fileName );
            //            if(StringUtil::isNullOrEmpty( dataStr ) == false)
            //            {
            //                auto aerofoilData = fb::make_ptr<Data<data::aircraft_aerofoil_data>>();
            //                auto pData = aerofoilData->getDataAsType<data::aircraft_aerofoil_data>();
            //                DataUtil::parse( dataStr, pData );
            //
            //                auto &cl = pData->cl;
            //                auto &cd = pData->cd;
            //                auto &cm = pData->cm;
            //
            //                WP_ASSERT( cl.values.empty() == false );
            //                WP_ASSERT( cd.values.empty() == false );
            //                WP_ASSERT( cm.values.empty() == false );
            //
            // #if !WP_AIRFOIL_USE_SPLINE
            //                CL->setPoints( getPointsArray( cl.values ) );
            //                CD->setPoints( getPointsArray( cd.values ) );
            //                CM->setPoints( getPointsArray( cm.values ) );
            // #else
            //				auto clPoints = getPoints(cl.values);
            //				auto cdPoints = getPoints(cd.values);
            //				auto cmPoints = getPoints(cm.values);
            //
            //				CL->setPoints(clPoints);
            //				CD->setPoints(cdPoints);
            //				CM->setPoints(cmPoints);
            // #endif
            //            }
            //            else
            //            {
            //                WP_LOG_ERROR( "Airfoil not loaded." );
            //            }
            //        }
            //
            //        WP_LOG( "Airfoil loaded" );
        }

        LookupCurve<real_Num> *CAerofoil::getCLPtr()
        {
            return CL.get();
        }

        SmartPtr<LookupCurve<real_Num>> CAerofoil::getCL() const
        {
            return CL;
        }

        void CAerofoil::setCL( SmartPtr<LookupCurve<real_Num>> cl )
        {
            CL = cl;
        }

        LookupCurve<real_Num> *CAerofoil::getCDPtr()
        {
            return CD.get();
        }

        SmartPtr<LookupCurve<real_Num>> CAerofoil::getCD() const
        {
            return CD;
        }

        void CAerofoil::setCD( SmartPtr<LookupCurve<real_Num>> cd )
        {
            CD = cd;
        }

        LookupCurve<real_Num> *CAerofoil::getCMPtr()
        {
            return CM.get();
        }

        SmartPtr<LookupCurve<real_Num>> CAerofoil::getCM() const
        {
            return CM;
        }

        void CAerofoil::setCM( SmartPtr<LookupCurve<real_Num>> cm )
        {
            CM = cm;
        }

        IAircraft *CAerofoil::getAircraftPtr() const
        {
            return m_aircraft.get();
        }

        SmartPtr<IAircraft> CAerofoil::getAircraft() const
        {
            return m_aircraft;
        }

        void CAerofoil::setAircraft( SmartPtr<IAircraft> aircraft )
        {
            m_aircraft = aircraft;
        }

        bool CAerofoil::getReverseValues() const
        {
            return m_reverseValues;
        }

        void CAerofoil::setReverseValues( bool reverse )
        {
            m_reverseValues = reverse;
        }
    } // namespace vehicle
} // namespace workphone
