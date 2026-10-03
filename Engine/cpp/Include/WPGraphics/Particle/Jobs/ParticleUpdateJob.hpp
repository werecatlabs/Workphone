#ifndef ParticleUpdateJob_h__
#define ParticleUpdateJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace render
    {
        class ParticleUpdateJob : public Job
        {
        public:
            ParticleUpdateJob( Array<SmartPtr<IParticle>> &particles );
            ~ParticleUpdateJob() override;

            void execute() override;

        protected:
            Array<SmartPtr<IParticle>> &m_particles;
            s32 m_task;
            time_interval m_t;
            time_interval m_dt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ParticleUpdateJob_h__
