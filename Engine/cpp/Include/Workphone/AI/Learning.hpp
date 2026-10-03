#ifndef _CQLearning_
#define _CQLearning_

#include <Workphone/Interface/Ai/ILearning.hpp>

namespace workphone
{

    /**
     * QLearning class with a slightly modified implementation based on John Manslow's qlearning class
     * from the article "Using Reinforcement Learning to Solve AI Control Problems," which came with the
     * book "AI Programming Wisdom 2."
     *
     * Most of the comments were retained and modified. The properties controlling learning are displayed
     * in the console for debugging purposes.
     *
     * This implementation uses an MLP (Multilayer Perceptron) neural network to relate state-action
     * pairs to long-term discounted rewards. It's important to note that reinforcement learning (RL)
     * combined with neural networks, especially MLPs, can pose challenges. If using it for in-game
     * learning, it might be preferable to employ lookup tables for storing Q-values or alternative
     * methods instead of neural networks.
     */
    class WPCore_API Learning : public ILearning
    {
    public:
        /**
         * Constructor for Q-learning.
         * @param stateVars Number of variables in the state vector.
         * @param actionVars Number of variables for representing different actions.
         * @param pMLP Pointer to a neural network object.
         */
        Learning( unsigned long stateVars, unsigned long actionVars, MLP *pMLP );

        /** Destructor for Q-learning. */
        ~Learning();

        /** Allocate memory for state-action pairs. */
        void AllocateMemory( void );

        /** Deallocate memory used for state-action pairs. */
        void DeallocateMemory( void );

        /**
         * Notify Q-learning that the game world has transitioned to a new state.
         * @param newState State vector representing the new state.
         * @param reward Reward received for transitioning to the new state.
         */
        void MoveToState( const double *newState, double reward );

        /**
         * Notify Q-learning that the game world has been reset to a specific state.
         * @param resetState State vector representing the reset state.
         */
        void ResetToState( const double *resetState );

        /**
         * Notify Q-learning that the game world has moved to a terminal state.
         * @param terminalState State vector representing the terminal state.
         * @param reward Reward received for reaching the terminal state.
         */
        void MoveToTerminalState( const double *terminalState, double reward );

        /**
         * Get the action that Q-learning wants to take in the current state.
         * @param currentState State vector representing the current state.
         * @param numActions Number of possible actions in the current state.
         * @param encodedActions Encoded representation of possible actions.
         * @return Chosen action.
         */
        unsigned long ulGetAction( const double *currentState, unsigned long numActions,
                                   double **encodedActions );

        /**
         * Reward the action chosen by Q-learning with an immediate reward.
         * @param immediateReward Immediate reward for the chosen action.
         */
        void RewardAction( double immediateReward );

    private:
        // Storage for the state-action pairs (a,s) for the current and previous states
        double *pStateActionPair;
        double *pPreviousStateActionPair;

        // Records the highest reward available in the last state for which ulGetAction was called
        double dLargestRewardAvailableInThisState;

        // Variables used to store information relating to the rewards the AI received for its last and
        // latest actions and for finding itself in the last and latest states. The are used to perform
        // the backup
        double dRewardForPreviousAction;
        double dRewardForLatestAction;
        double dRewardForPreviousState;
        double dRewardForLatestState;

        // Inidcates whether the game world is in a starting state (and hence no backup needs to be done)
        bool boStartingState;

        // The number of variables that is used to represent the state of teh game world to the AI and
        // the number of variables used to represent the actions in can take in each state
        unsigned long ulNumberOfActionVariables;
        unsigned long ulNumberOfStateVariables;

        // Parameters that control learning
        double dLearningRateDecayFactor;
        double dBestActionSelectionProbability;
        double dDiscountFactor;

        // point to the neural network
        MLP *pMLP;
    };

}  // namespace workphone

#endif
