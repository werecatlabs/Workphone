#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/Learning.hpp>
#include <Workphone/AI/MLP.hpp>
#include <cstdlib>

namespace workphone
{

    Learning::Learning( const unsigned long ulNewNumberOfStateVariables,
                        const unsigned long ulNewNumberOfActionVariables, MLP *_pMLP )
    {
        ulNumberOfStateVariables = ulNewNumberOfStateVariables;
        ulNumberOfActionVariables = ulNewNumberOfActionVariables;

        dLearningRateDecayFactor = 1.0;
        dBestActionSelectionProbability = 0.8;
        dDiscountFactor = 0.99;

        dRewardForPreviousAction = 0.0;
        dRewardForLatestAction = 0.0;
        dRewardForPreviousState = 0.0;
        dRewardForLatestState = 0.0;

        pMLP = _pMLP;

        AllocateMemory();
    }

    Learning::~Learning()
    {
        DeallocateMemory();
    }

    void Learning::AllocateMemory()
    {
        //Storage for the current and previous state action pairs (a,s). Previous state action pairs need to be
        //recorded so that the backups can be made
        pStateActionPair = new double[ulNumberOfActionVariables + ulNumberOfStateVariables];
        pPreviousStateActionPair = new double[ulNumberOfActionVariables + ulNumberOfStateVariables];
    }

    void Learning::DeallocateMemory()
    {
        delete[] pStateActionPair;
        delete[] pPreviousStateActionPair;
    }

    void Learning::ResetToState( const double *const pNewState )
    {
        unsigned long i;

        //Set the current state vector to the new state
        for( i = 0; i < ulNumberOfStateVariables; i++ )
        {
            pStateActionPair[i + ulNumberOfActionVariables] = pNewState[i];
        }

        //Reset reward variables
        dRewardForPreviousAction = 0.0;
        dRewardForLatestAction = 0.0;
        dRewardForPreviousState = 0.0;
        dRewardForLatestState = 0.0;

        //Recond that we're now in a starting state (i.e. no backup will be necessary in this state)
        boStartingState = true;

        //Reduce the learning rate of the AI by the amount specified. Note that this exponential reduction of
        //learning rate means that RL is no longer guaranteed to solve the problem we want. See the book for more
        //details
        pMLP->setLearningRate( pMLP->getLearningRate() * dLearningRateDecayFactor );
    }

    void Learning::MoveToTerminalState( const double *const pNewState,
                                        const double dRewardForFindingState )
    {
        //If we're not in a starting state perform a backup
        if( !boStartingState )
        {
            //Make the neural network aware of the state action pair that its about to receive a rewrad for
            pMLP->pdGetOutputs( pPreviousStateActionPair );

            //Show the neural network the reward the AI received for taking that action in that state
            //pMLP->dGradientBasedTrainingStep( 1, pPreviousStateActionPair, dRewardForFindingState );
        }
    }

    void Learning::MoveToState( const double *const pNewState, const double dRewardForFindingState )
    {
        unsigned long i;

        //If we're not in a starting state perform a backup
        if( !boStartingState )
        {
            //Make the neural network aware of the state action pair we're performing the backup for (because its a
            //backup the relevant state action pair is the previous action we took in the previous state)
            pMLP->pdGetOutputs( pPreviousStateActionPair );

            //Show the neural network the reward the AI received for taking the action it took in the previous state.
            //Note that the final parameter is the equation given for the backup that is described in the book
            //pMLP->dGradientBasedTrainingStep( 1, pPreviousStateActionPair,
            //                                  dDiscountFactor * dLargestRewardAvailableInThisState +
            //                                      dRewardForPreviousAction + dRewardForFindingState );
            //pMLP->dGradientBasedTrainingStep(1,pPreviousStateActionPair,dDiscountFactor*dRewardForPreviousAction+dRewardForFindingState);
        }

        //We're no longer in a starting state
        boStartingState = false;

        //What was the current state is not the previous one
        for( i = 0; i < ulNumberOfStateVariables + ulNumberOfActionVariables; i++ )
        {
            pPreviousStateActionPair[i] = pStateActionPair[i];
        }

        for( i = 0; i < ulNumberOfStateVariables; i++ )
        {
            pStateActionPair[i + ulNumberOfActionVariables] = pNewState[i];
        }
    }

    void Learning::RewardAction( const double dReward )
    {
        //We need temporary storage for rewards that are provided immediately in response to an action chosen by the
        //AI because the backup where the reward will be used will not occur until after another action has been
        //selected
        dRewardForPreviousAction = dRewardForLatestAction;
        dRewardForLatestAction = dReward;
    }

    auto Learning::ulGetAction( const double *const pGameState,
                                const unsigned long ulNumberOfActionsInThisState, double **ppdActions )
        -> unsigned long
    {
        unsigned long i, j;
        double dEstimatedReward;
        unsigned long ulSelectedAction;
        unsigned long ulBestActionInThisState;
        //char tempString[100];  //for debug

        //We're going to calculate the largest reward available from this state (as estimated by the Q-value
        //predicting neural network) so reset the current estimate to a small value so that it'll be
        //replaced
        dLargestRewardAvailableInThisState = -1e+99;

        //Set the state part of the state-action pair vector to the state passed to the function
        for( i = ulNumberOfActionVariables; i < ulNumberOfActionVariables + ulNumberOfStateVariables;
             i++ )
        {
            pStateActionPair[i] = pGameState[i - ulNumberOfActionVariables];
        }

        //Find best action
        ulBestActionInThisState = 0;
        for( i = 0; i < ulNumberOfActionsInThisState; i++ )
        {
            //Set up the action part of the state-action pair vector with the representation of the ith
            //vector that was passed into this function in the 2D action Array
            for( j = 0; j < ulNumberOfActionVariables; j++ )
            {
                pStateActionPair[j] = ppdActions[i][j];
            }

            //Pass the state-action pair vector to the neural network to get the estimated Q(a,s)
            dEstimatedReward = pMLP->pdGetOutputs( pStateActionPair )[0];

            //If if the estimated reward for the ith action is the highest so far record it and note that the ith
            //action is the best one found
            if( dEstimatedReward > dLargestRewardAvailableInThisState )
            {
                dLargestRewardAvailableInThisState = dEstimatedReward;
                ulBestActionInThisState = i;
            }
            //sprintf(tempString, "dEstimatedReward : %f\n", (float)dEstimatedReward);
            //printf(tempString);
        }

        //sprintf(tempString, "BestAction : %f\n", (float)ulBestActionInThisState);
        //printf(tempString);

        //Here we select an action according to dBestActionSelectionProbability. Select the best action with
        //probability dBestActionSelectionProbability and some other action random action with probability
        //1.0-dBestActionSelectionProbability
        if( ulNumberOfActionsInThisState < 2 ||
            static_cast<double>( rand() ) / static_cast<double>( RAND_MAX ) <
                dBestActionSelectionProbability )
        {
            ulSelectedAction = ulBestActionInThisState;
        }
        else
        {
            ulSelectedAction = rand() % ulNumberOfActionsInThisState;
        }

        //Record the state-action pair ready for a backup at some point in the future
        for( i = 0; i < ulNumberOfActionVariables; i++ )
        {
            pStateActionPair[i] = ppdActions[ulSelectedAction][i];
        }
        return ulSelectedAction;
    }

}  // namespace workphone
