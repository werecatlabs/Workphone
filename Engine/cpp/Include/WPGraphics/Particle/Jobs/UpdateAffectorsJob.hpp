#ifndef UpdateAffectorsJob_h__
#define UpdateAffectorsJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace render
    {
        class UpdateAffectorsJob : public Job
        {
        public:
            UpdateAffectorsJob( const Array<SmartPtr<IParticleAffector>> &affectors );
            ~UpdateAffectorsJob() override;

            void execute() override;

        protected:
            Array<SmartPtr<IParticleAffector>> m_affectors;
            s32 m_task;
            time_interval m_t;
            time_interval m_dt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // UpdateAffectorsJob_h__
