#ifndef ILearning_h__
#define ILearning_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * Interface for an ai agent to store and
     * load learning data and provide logic to aid learning.
     */
    class WPCore_API ILearning : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~ILearning() override;

        /** Train the learning agent with the given data. */
        virtual void train( const std::vector<float> &inputs, const std::vector<float> &targets ) = 0;

        /** Predict or evaluate outputs for the given inputs. */
        virtual std::vector<float> predict( const std::vector<float> &inputs ) const = 0;

        /** Save the learning state to a file. */
        virtual bool save( const std::string &filename ) const = 0;

        /** Load the learning state from a file. */
        virtual bool load( const std::string &filename ) = 0;

        /** Reset or clear the learning state. */
        virtual void reset() = 0;

        /** Set parameters or hyperparameters for learning. */
        virtual void setParameters( const std::vector<float> &params ) = 0;

        /** Get current parameters or hyperparameters. */
        virtual std::vector<float> getParameters() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ILearning_h__
