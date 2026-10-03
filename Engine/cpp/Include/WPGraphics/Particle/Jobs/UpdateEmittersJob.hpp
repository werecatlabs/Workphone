#ifndef UpdateEmittersJob_h__
#define UpdateEmittersJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace render
    {
        class UpdateEmittersJob : public Job
        {
        public:
            UpdateEmittersJob( const Array<SmartPtr<IParticleEmitter>> &emitters );
            ~UpdateEmittersJob() override;

            void execute() override;

        protected:
            Array<SmartPtr<IParticleEmitter>> m_emitters;
            s32 m_task;
            time_interval m_t;
            time_interval m_dt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // UpdateEmittersJob_h__
