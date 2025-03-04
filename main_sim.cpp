/*
This is a replication of the algorithm found here.
https://sites.santafe.edu/~bowles/artificial_history/algorithm_coevolution.htm
*/

#include <iostream>
#include <random>
#include <vector>

/*
Define the structure of an agent
*/

class Agent {
public:
    char trait; //possible values are 'c' cooperate and 'd' defect
    float payoff;
    /*
    Constructor function
    */
    Agent(char t = 'd', float p = 0) //setting default agent to defective. Might be a problem later
        : trait (t)
        , payoff (p)
        {}

    void setTrait(char newTrait) {
        trait = newTrait;
    }

    char getTrait(){
        return trait;
    }

    void setPayoff(float newPayoff) {
        payoff = newPayoff;
    }

    float getPayoff(){
        return payoff;
    }
};

/*
A standard evolutionary prisoner's dilemma. We treat a player's strategy as being "programmed in"
by their trait—either cooperative or defective. As is standard, we take payoffs T > R > P > S,
and 2R > T+ S. See Hofbauer and Sigmund 1998 for details.
*/
void playPrisonersDilemma(Agent& playerOne, Agent& playerTwo) {
    /*
    Defining these values so we can use them later. I chose smallish values because some of the transition
    probabilities Bowles describes look small and I want to make sure payoffs don't overwhelm mutations.
    */
    float suckersPayoff = 0; //S
    float temptationToDefect = 1; //T
    float rewardForCooperation = 0.6; //R
    float mutualPunishment = 0.3; //P

    if (playerOne.getTrait() == 'c' && playerTwo.getTrait() == 'c') {
        playerOne.setPayoff(rewardForCooperation);
        playerTwo.setPayoff(rewardForCooperation); //case 1, both cooperate
    }
    else if (playerOne.getTrait() == 'c' && playerTwo.getTrait() == 'd') {
        playerOne.setPayoff(suckersPayoff);
        playerTwo.setPayoff(temptationToDefect); //case 2, p1 coop p2 defect
    }
    else if (playerOne.getTrait() == 'd' && playerTwo.getTrait() == 'd') {
        playerOne.setPayoff(mutualPunishment);
        playerTwo.setPayoff(mutualPunishment); //case 3, both defect
    }
    else {
        playerOne.setPayoff(temptationToDefect);
        playerTwo.setPayoff(suckersPayoff); //reverse of case 2
    }
}

int main() {
    /*
    Actually running the simulation goes here, I am thinking I will have this save some data to a file so we
    can plot and analyze it in python with standard analysis tools.
    */
    Agent testOne ('c', 0);
    Agent testTwo ('d', 0);
    playPrisonersDilemma(testOne, testTwo);
    std::cout << testTwo.getPayoff() << std::endl; //this in fact returns 1 so the game works

    /*
    Below is code for running many prisoner's dilemmas on a vector of Agents. We probably will want to
    turn this into its own function because we will be running it many times but for now this is an idea of
    how the within-group dynamics will work
    */
    std::vector<char> possibleTraits ('c', 'd'); //this is a vector for now because we might want to add more types

    std::vector<Agent> group (10); //important that this have an even number of elements
    size_t length  = group.size();

    for (int i = 0; i < length; ++i) { //will need to find a better way to do this in the future
        int randomIndex = rand() % 2; //pick a random trait, this returns either 0 or 1
        group[i].setTrait(possibleTraits[randomIndex]);
    }

    for (int j = 0; j < length - 1; ++j) {
        playPrisonersDilemma(group[j], group[j+1]);
        std::cout << group[j].getPayoff() << std::endl;
    }

    return 0;
}
