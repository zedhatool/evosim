/*
This is a replication of the algorithm found here.
https://sites.santafe.edu/~bowles/artificial_history/algorithm_coevolution.htm
*/

#include <iostream>
#include <random>
#include <vector>
#include <numeric>

/*
Storage place for our beautiful boys (aka global constants)
*/
const float GROUP_CONFLICT_CHANCE (0.25); //value from BCH. We may need to have this vary over time, see fig 5
const float INDIVIDUAL_MUTATION_RATE (0.001); //benchmark from BCH, will need to do parameter search
const float INSTITUTIONAL_CHANGE_CHANCE (0.1); //benchmark from BCH

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
    Agent(char t, float p) {
        trait = t;
        payoff = p;
    }

    void setTrait(char newTrait) {
        trait = newTrait;
    }

    char getTrait() const{
        return trait;
    }

    void setPayoff(float newPayoff) {
        payoff = newPayoff;
    }

    float getPayoff() const{
        return payoff;
    }

    Agent operator= (const Agent &other) {
        Agent agent (other.getTrait(), other.getPayoff());
        return agent;
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
    size_t groupSize;
    std::vector<Agent> agents;

    //Group(float pCoop = 0, float tRate = 0, float sRate = 0, float tPayoff = 0, std::vector<Agent> a)
    Group(float pCoop, float tRate, float sRate, float tPayoff, size_t gSize) {
      proportionCooperative = pCoop;
      taxRate = tRate;
      segmentationRate = sRate;
      totalPayoff = tPayoff;
      groupSize = gSize;
      initAgents(groupSize);
    }

    Group() {
        proportionCooperative = 0;
        taxRate = 0;
        segmentationRate = 0;
        totalPayoff = 0;
        groupSize = 0;
        } //I thought it might be a good idea to move agent initialization to a method within the Group class, but let me know if there's a problem with this

    void initAgents(size_t numAgents) {
        //get the number of agents who are cooperative based on groupSize and propCoop
        size_t num_cooperative = groupSize * (size_t) proportionCooperative;

        std::vector<Agent> agentsTemp;
        for (int i = 0; i < numAgents; ++i) {
            if (i <= num_cooperative) {
                Agent agent ('c', 0);
                agentsTemp.push_back(agent);
            }
            else {
                Agent agent ('d', 0);
                agentsTemp.push_back(agent);
            }
        }
        agents = agentsTemp;
    }

    size_t getSize() {
        return groupSize;
    }

    /*
    This method updates the two variables which are functions of the group data, rather than things we define;
    namely, it computes the total payoff and then updates the proportion of the group which is cooperative.
    */

    void updateGroupData() {
        size_t length (agents.size());
        float dummyPayoff (0);
        float dummyCoop (0);

        float cost (0.5 * (segmentationRate * segmentationRate + taxRate * taxRate)); //institutions are costly, see algorithm description
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
        groupSize = agents.size();
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

    void setInstitutions(float tR, float sR) {
        taxRate = tR;
        segmentationRate = sR;
    }

    void addAgent(Agent a) {
        agents.push_back(a);
        groupSize++;
    }

    //this is giving some kind of strange error
    /*
    void removeAgent(Agent a) {
        agents.erase(find(agents.begin(), agents.end(), a));
        groupSize--;
        // TODO: add agent vector init here
    }
    */

    void updatePayoffByIndex(size_t index, float newPayoff) {
        agents[index].setPayoff(newPayoff);
    }

    void transferByIndex(size_t index, float transferAmount) {
        agents[index].setPayoff(agents[index].getPayoff() + transferAmount);
    }

    void updateTraitByIndex(size_t index, char newTrait) {
        agents[index].setTrait(newTrait);
    }

    std::vector<Agent> getAgents() {
        return agents;
    }
};


void playWithinGroup(Group& group) {
    /*
    Defining these values so we can use them later. I chose smallish values because some of the transition
    probabilities Bowles describes look small and I want to make sure payoffs don't overwhelm mutations.
    */
    float suckersPayoff = 0; //S
    float temptationToDefect = 1; //T
    float rewardForCooperation = 0.6; //R
    float mutualPunishment = 0.3; //P
    float taxPool = 0;
    float T = group.getTaxRate();

    /*
    Randomness in pairings. Create three pools, then randomize the third pool and play the game within
    groups
    */
    std::mt19937 randomizer; //for good (pseudo) randomness
    std::uniform_real_distribution<> dis(0.0, 1.0); //uniform distribution on [0, 1]
    std::vector<size_t> cooperators; //the indices corresponding to cooperators who are paired
    std::vector<size_t> defectors;
    std::vector<size_t> randomPool;

    size_t length (group.getAgents().size());

    //first pooling according to the segmentation rate
    for (size_t j (0); j < length; ++j) {
        //compare uniform random number between 0 and 1 against segmentation rate.
        float randResult = dis(randomizer);
        if (randResult <= group.getSegRate() && group.getAgents()[j].getTrait() == 'c') {
            cooperators.push_back(j);
        } //cooperators get paired with probability = segmentation rate
        else if (randResult <= group.getSegRate() && group.getAgents()[j].getTrait() == 'd') {
            defectors.push_back(j);
        } //defectors get paired with probability = segmentation rate
        else {
            randomPool.push_back(j);
        } //pair those who weren't segmented into a group that itself gets randomized
    }
    //now shuffle the randomPool by randomly permuting the elements
    std::shuffle(randomPool.begin(), randomPool.end(), randomizer);

    //now play the game for every collection of indices

    for (size_t k (0); k < cooperators.size(); ++k) {
        group.updatePayoffByIndex(cooperators[k], (1 - T) * rewardForCooperation); //1 - T removes taxes
        taxPool += 2 * T * rewardForCooperation;
    }

    for (size_t l (0); l < defectors.size(); ++l) {
        group.updatePayoffByIndex(defectors[l], (1 - T) * mutualPunishment);
        taxPool += 2 * T * mutualPunishment;
    }

    size_t rpLength (randomPool.size());

    if (rpLength % 2 != 0) {
        randomPool.erase(randomPool.begin() + rpLength - 1);
    }

    for (size_t m (0); m < rpLength - 1; m+=2) {
        if (group.getAgents()[m].getTrait() == 'c' && group.getAgents()[m+1].getTrait() == 'c') {
            group.updatePayoffByIndex(m, rewardForCooperation);
            group.updatePayoffByIndex(m+1, rewardForCooperation); //case 1, both cooperate
            taxPool += 2 * T * (rewardForCooperation);
        }
        else if (group.getAgents()[m].getTrait() == 'c' && group.getAgents()[m+1].getTrait() == 'd') {
            group.updatePayoffByIndex(m, suckersPayoff);
            group.updatePayoffByIndex(m+1, temptationToDefect); //case 2, p1 coop p2 defect
            taxPool += T * (suckersPayoff + temptationToDefect);
        }
        else if (group.getAgents()[m].getTrait() == 'd' && group.getAgents()[m+1].getTrait() == 'd') {
            group.updatePayoffByIndex(m, mutualPunishment);
            group.updatePayoffByIndex(m+1, mutualPunishment); //case 3, both defect
            taxPool += 2 * T * (mutualPunishment);
        }
        else {
            group.updatePayoffByIndex(m, temptationToDefect);
            group.updatePayoffByIndex(m+1, suckersPayoff); //reverse of case 2
            taxPool += T * (suckersPayoff + temptationToDefect);
        }
    }


    float transfer = taxPool / group.getAgents().size();

    for (size_t n (0); n < length; ++n) {
        group.transferByIndex(n, transfer); //add transfer amount to everyone in the group's payoff
    }
}

/*
Step 3b and 3c of the algorithm
*/
void haveChildren(Group& group) {
    std::mt19937 randomizer;
    std::uniform_real_distribution<> dis(0.0, 1.0);
    float total = group.getTotalPayoff();

    size_t groupSize = group.getSize();
    std::vector<int> subintervals;
    std::vector<float> weights;

    for (size_t i = 0; i <= groupSize; ++i) {
        subintervals.push_back(i);
    }
    for (size_t j = 0; j < groupSize; ++j) {
        float pj = group.getAgents()[j].getPayoff();
        weights.push_back(pj / total);
    }

    //This defines a probability mass function on subintervals given by the indices of agents
    std::piecewise_constant_distribution<> d(subintervals.begin(), subintervals.end(), weights.begin());

    size_t sexHavers = 0;

    while (sexHavers < groupSize) {
        float randResult = d(randomizer);
        size_t randIndex = (size_t) randResult;
        Agent agent = group.getAgents()[randIndex];
        float mutation = dis(randomizer);

        //individual mutation chance for every agent
        if (mutation <= INDIVIDUAL_MUTATION_RATE && agent.getTrait() == 'c') {
            agent.setTrait('d');
        }
        else if (mutation <= INDIVIDUAL_MUTATION_RATE && agent.getTrait() == 'd') {
            agent.setTrait('c');
        }

        group.addAgent(agent);

        ++sexHavers;
    }

    std::uniform_int_distribution<> twoCoin(0, 3);
    /*
    0 = increase tax rate, increase seg rate
    1 = increase tax rate, decrease seg rate
    2 = decrease tax rate, increase seg rate
    3 = decrease tax rate, decrease seg rate
    */
    int flips = twoCoin(randomizer);
    float institutionalChange = dis(randomizer);
    if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 0) {//1 = heads, increase
        group.setInstitutions(group.getTaxRate() + 0.1, group.getSegRate() + 0.1);
    } else if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 1) {
        group.setInstitutions(group.getTaxRate() + 0.1, group.getSegRate() - 0.1);
    } else if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 2) {
        group.setInstitutions(group.getTaxRate() - 0.1, group.getSegRate() + 0.1);
    } else if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 3) {
        group.setInstitutions(group.getTaxRate() - 0.1, group.getSegRate() - 0.1);
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
    Test of the two games. Recall that we initialize a group with
    float proportionCooperative; //need to track this for when groups win conflicts
    float taxRate; //Proportion of an agent's payoff which is redistributed to the group
    float segmentationRate; //Chance an agent is matched with their own type. Gives some spatial structure
    float totalPayoff; // total (not average!) payoff of agents in the group
    size_t groupSize;
    */
    Group groupOne (20, 0.3, 0.4, 0, 100);
    Group groupTwo (40, 0.2, 0.7, 0, 100);

    playWithinGroup(groupOne);
    playWithinGroup(groupTwo);

    playGroupGame(groupOne, groupTwo);

    haveChildren(groupOne);


    for (int p (0); p < groupOne.getAgents().size(); ++p) {
        std::cout  << groupOne.getAgents()[p].getPayoff() << std::endl;
    }

    return 0;
}
