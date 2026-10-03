#ifndef JobFunction_h__
#define JobFunction_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{

    /** Job class that executes a function. */
    class JobFunction : public Job
    {
    public:
        /** Constructor. */
        JobFunction();

        /** Destructor. */
        ~JobFunction() override;

        /** @copydoc Job::execute */
        void execute() override;

        /** Gets the function to execute. */
        std::function<void()> getFunction() const;

        /** Sets the function to execute. */
        void setFunction( std::function<void()> &function );

        WP_CLASS_REGISTER_DECL;

    protected:
        // The function to execute.
        std::function<void()> m_function;
    };

}  // namespace workphone

#endif  // JobFunction_h__
