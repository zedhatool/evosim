/*
This is a replication of the algorithm found here.
https://sites.santafe.edu/~bowles/artificial_history/algorithm_coevolution.htm
*/

#include <iostream>
#include <random>
#include <vector>

/*
Storage place for our beautiful boys (aka global constants)
*/
const float GROUP_CONFLICT_CHANCE (0.25); //value from BCH. We may need to have this vary over time, see fig 5

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
    Agent(char t='d', float p=0)
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
I guess we make another object for the 'group' level.
What is the most efficient way to structure it?
e.g. what data structure do we use to store the members?
Also, for interactions are we using methods or 'general' functions?
(not a CS student, correct me if I'm using the wrong terminology)
*/

class Group {
public:
    float proportionCooperative; //need to track this for when groups win conflicts
    float taxRate; //Proportion of an agent's payoff which is redistributed to the group
    float segmentationRate; //Chance an agent is matched with their own type. Gives some spatial structure
    float totalPayoff; // total (not average!) payoff of agents in the group
    int groupSize;
    std::vector<Agent> agents; //Should we make a variable tracking group size? Or just use .size() on the vector

    //Group(float pCoop = 0, float tRate = 0, float sRate = 0, float tPayoff = 0, std::vector<Agent> a)
    Group(float pCoop = 0, float tRate = 0, float sRate = 0, float tPayoff = 0, int gSize = 10)
    : proportionCooperative (pCoop)
    , taxRate (tRate)
    , segmentationRate (sRate)
    , totalPayoff (tPayoff)
    , groupSize (gSize)
    {initAgents(gSize)}

    /*
    This method updates the two variables which are functions of the group data, rather than things we define;
    namely, it computes the total payoff and then updates the proportion of the group which is cooperative.
    */
    
    void initAgents(int numAgents) {
        std::vector<Agent> agents (numAgents)
        for (int i = 0; i < length; ++i) { //will need to find a better way to do this in the future
            int randomIndex = rand() % 2; //pick a random trait, this returns either 0 or 1
            testGroupOne.updateTraitByIndex(i, possibleTraits[randomIndex]); //randomize traits within the group
        }
    }

    int getSize() {
        return groupSize;
    }

    void updateGroupData() {
        size_t length (agents.size());
        float dummyPayoff (0);
        float dummyCoop (0);

        float cost (0.5 * (sRate * sRate + tRate * tRate)); //institutions are costly, see algorithm description
        /*
        this^^ is the reason it is important that in the Prisoners dilemma 2R > T + S, otherwise groups
        which are more defective will have higher payoffs. You can think of this as defecting and agreeing to split
        the higher reward T + S instead of splitting 2R. See e.g. Hofbauer and Sigmund for more info.
        */

        for (size_t i = 0; i < length; ++i) {
            dummyPayoff += (agents[i].getPayoff() - cost);

            if (agents[i].getTrait() == 'c') { //is there a more efficient way to do this? I don't know
                dummyCoop += 1;
            }
        }

        dummyCoop /= (float)length; //uh I think this is fine
        totalPayoff = dummyPayoff;
    }

    float getTotalPayoff() {
        return totalPayoff;
    }

    float getPropCoop(){
        return proportionCooperative;
    }

    float getTaxRate(){
        return taxRate;
    }

    float getSegRate(){
        return segmentationRate;
    }

    void setInstitutions(float& tR, float& sR) {
        taxRate = tR;
        segmentationRate = sR;
    }

    void addAgent(Agent a) {
        agents.push_back(a);
        groupSize++;
    }

    void removeAgent(Agent a) {
        agents.erase(find(agents.begin(), agents.end(), a));
        groupSize--;
        // TODO: add agent vector init here
    }

    void updatePayoffByIndex(int index, float newPayoff) {
        agents[i].setPayoff(newPayoff);
    }

    void updateTraitByIndex(int index, char newTrait) {
        agents[i].setTrait(newTrait);
    }

    std::vector<Agent> getAgents() {
        return agents;
    }
}

/*
Define how the game works for players. We will need to figure out how to use this to determine reproductive rate...
Have this all commented out because I replaced it below with one that works for a Group. Keeping this in
case we need it later.

void playPrisonersDilemma(Agent& playerOne, Agent& playerTwo) {

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
*/
void playPrisonersDilemma(&Group group) {
    /*
    Defining these values so we can use them later. I chose smallish values because some of the transition
    probabilities Bowles describes look small and I want to make sure payoffs don't overwhelm mutations.
    */
    float suckersPayoff = 0; //S
    float temptationToDefect = 1; //T
    float rewardForCooperation = 0.6; //R
    float mutualPunishment = 0.3; //P

    size_t length (group.getAgents().size());

    for (size_t i (0); i < length - 1; ++i) {
        if (group.getAgents()[i].getTrait() == 'c' && group.getAgents()[i + 1].getTrait() == 'c') {
            group.updatePayoffByIndex(i, rewardForCooperation);
            group.updatePayoffByIndex(i+1, rewardForCooperation); //case 1, both cooperate
        }
        else if (group.getAgents()[i].getTrait() == 'c' && group.getAgents()[i+1].getTrait() == 'd') {
            group.updatePayoffByIndex(i, suckersPayoff);
            group.updatePayoffByIndex(i+1, temptationToDefect); //case 2, p1 coop p2 defect
        }
        else if (group.getAgents()[i].getTrait() == 'd' && group.getAgents()[i+1].getTrait() == 'd') {
            group.updatePayoffByIndex(i, mutualPunishment);
            group.updatePayoffByIndex(i+1, mutualPunishment); //case 3, both defect
        }
        else {
            group.updatePayoffByIndex(i, temptationToDefect);
            group.updatePayoffByIndex(i+1, suckersPayoff); //reverse of case 2
        }
    }
}

/*
Define how the game works for groups. Note that not every group participates in conflict, a group is drawn in
with chance GROUP_CONFLICT_CHANCE (defined way above)
*/
void playGroupGame(Group& groupOne, Group& groupTwo) {
    float payoffOne (groupOne.getTotalPayoff());
    float payoffTwo (groupTwo.getTotalPayoff());

    if (payoffOne >= payoffTwo) {//arbitrarily break ties in favor of group 1. randomize going forward?
        groupTwo.setInstitutions(groupOne.getTaxRate(), groupOne.getSegRate()); //group 2 gets 1's institutions
        /*
        IMPORTANT: What needs to happen when one side wins is that we take the proportion of the winning group
        that's cooperative, and use that to generate a number of new agents equal to the size of the losing group.
        Then, we pool all the agents in the winning group with the new agents, and split the group in half randomly.
        One half stays in the winning group, the other half takes over the losing group, and gets all the institutions
        of the winning group. Making the institutions of the losing group the same is easy, the first part is the
        harder part.
        */
    } //group one wins
    else {
        groupOne.setInstitutions(groupTwo.getTaxRate(), groupOne.getTaxRate());
    } //group two wins
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

    //std::vector<Agent> agents (10); //important that this have an even number of elements
    Group testGroupOne (0, 0, 0, 10);
    Group testGroupTwo (0, 0, 0, 10);

    size_t length  = testGroupOne.getSize();

    /*
    Broke this and need to figure out how to make it work
    */

    /*
    for (int i = 0; i < length; ++i) { //will need to find a better way to do this in the future
        int randomIndex = rand() % 2; //pick a random trait, this returns either 0 or 1
        testGroupOne.updateTraitByIndex(i, possibleTraits[randomIndex]); //randomize traits within the group
    }

    testGroupOne.updateGroupData();
    */
   
    for (int j = 0; j < length - 1; ++j) {
        playPrisonersDilemma(testGroupOne.getAgents()[j], testGroupOne.getAgents()[j+1]); //This doesn't work <- I think it's because we hve redefined it to take a group as input, but here we are inputting Agents? Maybe we should have groupGames and individualGames?
        std::cout << testGroupOne.getAgents()[j].getPayoff() << std::endl;
    }
    

    return 0;
}
