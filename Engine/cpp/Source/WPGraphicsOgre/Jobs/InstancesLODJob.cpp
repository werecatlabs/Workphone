#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Jobs/InstancesLODJob.hpp"
#include <Workphone/Math/Math.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        InstancesLODJob::InstancesLODJob( Array<CInstanceObjectOld *> &instances,
                                          Ogre::Viewport *viewport ) :
            m_index( 0 )
        {
            m_instances = instances;
            m_viewport = viewport;

            //setState( IJob::State::Ready );
        }

        InstancesLODJob::~InstancesLODJob()
        {
        }

        void InstancesLODJob::update()
        {
            if( this->getState() != IJob::State::Executing )
            {
                m_index = 0;
                setState( IJob::State::Executing );

                numInstancesLess150 = 0;
                numInstancesLess300 = 0;
                numInstancesLess500 = 0;
                numInstancesGreater500 = 0;
            }

            if( !m_instances.empty() )
            {
                int numProcessed = 0;
                while( numProcessed < 100 )
                {
                    CInstanceObjectOld *instanceObject = m_instances[m_index++];
                    processInstance( instanceObject );

                    if( m_instances.size() <= m_index )
                    {
                        setState( IJob::State::Finish );
                        break;
                    }

                    ++numProcessed;
                }
            }
        }

        s32 InstancesLODJob::getPriority() const
        {
            return m_priority;
        }

        void InstancesLODJob::setPriority( s32 priority )
        {
            m_priority = priority;
        }

        u32 InstancesLODJob::getProgress() const
        {
            return m_progress;
        }

        void InstancesLODJob::setProgress( u32 progress )
        {
            m_progress = progress;
        }

        void InstancesLODJob::processInstance( CInstanceObjectOld *instanceObject )
        {
            // instanceObject->setVisible(false);
            // return;

            Ogre::Camera *camera = m_viewport->getCamera();
            Ogre::Vector3 cameraPosition = camera->getRealPosition();

            bool showBilloard = false;

            Ogre::Vector3 directionVector = instanceObject->getPosition() - cameraPosition;
            f32 distanceFromCamera = directionVector.length();

            if( MathF::Abs( instanceObject->m_lastDistance - distanceFromCamera ) > 10.0f )
            {
                instanceObject->m_lastDistance = distanceFromCamera;

                if( distanceFromCamera < 30 )
                {
                    instanceObject->setVisible( true );
                }
                else if( distanceFromCamera < 80 )
                {
                    ++numInstancesLess150;
                    if( numInstancesLess150 < 4 )
                    {
                        instanceObject->setVisible( false );
                        // showBilloard = true;
                    }
                    else
                    {
                        instanceObject->setVisible( true );
                        numInstancesLess150 = 0;
                    }
                }
                else if( distanceFromCamera < 200 )
                {
                    ++numInstancesLess300;
                    if( numInstancesLess300 < 10 )
                    {
                        instanceObject->setVisible( false );
                    }
                    else
                    {
                        instanceObject->setVisible( true );
                        numInstancesLess300 = 0;
                    }
                }
                else
                {
                    instanceObject->setVisible( false );
                    showBilloard = true;
                }

                /*
                if(showBilloard)
                {
                Ogre::Vector3 dir = directionVector.normalisedCopy();
                dir.y = 0.f;

                Ogre::Quaternion orientation = Ogre::Vector3::NEGATIVE_UNIT_Z.getRotationTo(dir);

                Ogre::InstancedEntity* billboard = instanceObject->getBillboard();
                billboard->setOrientation(orientation);
                billboard->setVisible(true);
                }
                else
                {
                Ogre::InstancedEntity* billboard = instanceObject->getBillboard();
                billboard->setVisible(false);
                }
                */
            }
        }

    }  // end namespace render
}  // namespace workphone
