/*
This is a replication of the algorithm found here.
https://sites.santafe.edu/~bowles/artificial_history/algorithm_coevolution.htm
*/

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <map>

/*
Storage place for our beautiful boys (aka global constants)
*/
const float GROUP_CONFLICT_CHANCE (0.25); //value from BCH. We may need to have this vary over time, see fig 5
const float INDIVIDUAL_MUTATION_RATE (0.005); //benchmark from BCH, will need to do parameter search
const float INSTITUTIONAL_CHANGE_CHANCE (0.1); //benchmark from BCH
const int INITIAL_GROUPS = 100; //I randomly chose ten. We can change this later
const int AGENTS_MULTIPLIER = 20; //benchmark value is this is 20
const int GROUP_SIZE_LOWER_BOUND = 4; //from BCH. Makes sense because that way there are 2 PD pairings
const float BLACK_SWAN_CHANCE = 0.01;
const float CONVERSION_CHANCE = 0.3; //about the prevalence of fascism in post crisis societies
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

    Agent() {
        trait = 'd';
        payoff = 0;
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

    Agent& operator= (const Agent& other) {
        trait = other.getTrait();
        payoff = other.getPayoff();
        return *this;
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
        int numCooperative = groupSize * (int) proportionCooperative;

        std::vector<Agent> agentsTemp;
        for (int i = 0; i < numAgents; ++i) {
            if (i <= numCooperative) {
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

    void updateGroupSize() {
        groupSize = agents.size();
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

        proportionCooperative = dummyCoop;
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

    void removeAgentByIndex(int index) {
        agents.erase(agents.begin() + index);
        groupSize--;
    }

    void setTraitByIndex(int index, char newTrait) {
        agents[index].setTrait(newTrait);
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

    void overhaulAgents(std::vector<Agent> newAgents) {
        agents = newAgents;
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
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 randomizer; //for good (pseudo) randomness
    randomizer.seed((unsigned long)seed);
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

    int rpLength (randomPool.size());

    //check if rpLength is odd
    if (rpLength % 2 != 0) {
        randomPool.pop_back();
        --rpLength; //make sure to actually reflect the fact that we changed the length of the random pool
    }

    if (rpLength > 0) {

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
    }

    float transfer = taxPool / (float) group.getAgents().size();

    for (size_t n (0); n < length; ++n) {
        group.transferByIndex(n, transfer); //add transfer amount to everyone in the group's payoff
    }
}

/*
Step 3b and 3c of the algorithm
*/
void haveChildren(Group& group) {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 randomizer;
    randomizer.seed((unsigned long)seed);
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

    int sexHavers = 0;

    std::vector<Agent> childPool;

    while (sexHavers < groupSize) {
        float randResult = d(randomizer);
        int randIndex = (int) randResult;
        Agent agent = group.getAgents()[randIndex];
        float mutation = dis(randomizer);

        //individual mutation chance for every agent
        if (mutation <= INDIVIDUAL_MUTATION_RATE && agent.getTrait() == 'c') {
            agent.setTrait('d');
        }
        else if (mutation <= INDIVIDUAL_MUTATION_RATE && agent.getTrait() == 'd') {
            agent.setTrait('c');
        }

        childPool.push_back(agent);

        ++sexHavers;
    }

    group.overhaulAgents(childPool);
    group.updateGroupData();

    std::uniform_int_distribution<> twoCoin(0, 3);
    /*
    0 = increase tax rate, increase seg rate
    1 = increase tax rate, decrease seg rate
    2 = decrease tax rate, increase seg rate
    3 = decrease tax rate, decrease seg rate
    */
    int flips = twoCoin(randomizer);
    float institutionalChange = dis(randomizer);

    if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 0 && group.getTaxRate() <= 0.9 && group.getSegRate() <= 0.4) {//1 = heads, increase
        group.setInstitutions(group.getTaxRate() + 0.1, group.getSegRate() + 0.1);
    } else if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 1 && group.getSegRate() >= 0.1 && group.getTaxRate() <= 0.9) {
        group.setInstitutions(group.getTaxRate() + 0.1, group.getSegRate() - 0.1);
    } else if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 2  && group.getTaxRate() >= 0.1 && group.getSegRate() <= 0.4) {
        group.setInstitutions(group.getTaxRate() - 0.1, group.getSegRate() + 0.1);
    } else if (institutionalChange <= INSTITUTIONAL_CHANGE_CHANCE && flips == 3 && group.getSegRate() >= 0.1 && group.getTaxRate() >= 0.1) {
        group.setInstitutions(group.getTaxRate() - 0.1, group.getSegRate() - 0.1);
    }

}

/*
Define how the game works for groups. Note that not every group participates in conflict, a group is drawn in
with chance GROUP_CONFLICT_CHANCE (defined way above)
*/
void playGroupGame(Group& groupOne, Group& groupTwo) {
    std::mt19937 randomizer;

    float payoffOne (groupOne.getTotalPayoff());
    float payoffTwo (groupTwo.getTotalPayoff());
    std::vector<Agent> pool;

    if (payoffOne >= payoffTwo) {//arbitrarily break ties in favor of group 1. randomize going forward?
        groupTwo.setInstitutions(groupOne.getTaxRate(), groupOne.getSegRate()); //group 2 gets 1's institutions

        size_t numCoopTwo = (size_t) ((float) groupTwo.getSize() * groupOne.getPropCoop());

        //generate the enlarged pool to split between groups
        for (size_t i(0); i < groupOne.getSize() + groupTwo.getSize(); ++i) {
            if (i < groupOne.getSize()) {
                pool.push_back(groupOne.getAgents()[i]);
            }
            else if (i >= groupOne.getSize() && i < groupOne.getSize() + numCoopTwo) {
                Agent agent ('c', 0);
                pool.push_back(agent);
            }
            else {
                Agent agent ('d', 0);
                pool.push_back(agent);
            }
        }

    int poolSize (pool.size());
    std::uniform_int_distribution<> uniZ(4, poolSize - 4);

    int sizeTwo = uniZ(randomizer); //pick a random index to split groups 1 and 2

    //split groups randomly, I think
    groupOne.overhaulAgents(std::vector<Agent>(pool.begin(), pool.end() - sizeTwo));
    groupTwo.overhaulAgents(std::vector<Agent>(pool.end() - sizeTwo, pool.end()));

    groupOne.updateGroupData();
    groupTwo.updateGroupData();

    } //group one wins
    else {
        groupOne.setInstitutions(groupTwo.getTaxRate(), groupOne.getTaxRate());

        size_t numCoopOne = (size_t) ((float) groupOne.getSize() * groupTwo.getPropCoop());

        //generate the enlarged pool to split between groups
        for (size_t i(0); i < groupOne.getSize() + groupTwo.getSize(); ++i) {
            if (i < groupTwo.getSize()) {
                pool.push_back(groupTwo.getAgents()[i]);
            }
            else if (i >= groupTwo.getSize() && i < groupOne.getSize() + numCoopOne) {
                Agent agent ('c', 0);
                pool.push_back(agent);
            }
            else {
                Agent agent ('d', 0);
                pool.push_back(agent);
            }
        }

        int poolSize (pool.size());
        std::uniform_int_distribution<> uniZ(4, poolSize - 4);

        int sizeOne = uniZ(randomizer); //pick a random index to split groups 1 and 2

        //split groups randomly, I think
        groupTwo.overhaulAgents(std::vector<Agent>(pool.begin(), pool.end() - sizeOne));
        groupOne.overhaulAgents(std::vector<Agent>(pool.end() - sizeOne, pool.end()));
        groupOne.updateGroupData();
        groupTwo.updateGroupData();
    } //group two wins
}

/*
Extensions go down here
*/
/*
    Migration: in every group G, each agent a has a probability to migrate based on the ratio of their payoff
    with the group's total payoff. They randomly choose among groups with a higher total payoff than their own,
    and migrate
*/
void migrate(std::vector<Group>& world) {
    std::mt19937 randomizer;
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    randomizer.seed((unsigned long)seed);
    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::uniform_int_distribution<> d(0.0, world.size() - 1);
    std::map <int, std::vector<int> > dict;
    std::map<int, std::vector<int> >::iterator it;

    for (int i = 0; i < world.size(); i++) {
        std::vector<int> tempIndices;
        Group group = world[i];
        for (int j = 0; j < group.getSize(); j++) {
            if (group.getAgents()[j].getPayoff() >= group.getTotalPayoff() / (float) group.getSize()) {
                float chance = group.getAgents()[j].getPayoff() / group.getTotalPayoff();
                int numMigrated = 0;
                if (dis(randomizer) <= chance && group.getSize() - numMigrated > 4) {
                    tempIndices.push_back(j);
                    int randIndex = d(randomizer);
                    if (randIndex != j) {
                       world[randIndex].addAgent(group.getAgents()[j]);
                    }
                    numMigrated++;
                }
            }
        }
        dict[i] = tempIndices;
    }

    for (int p (0); p < world.size(); ++p) {
        world[p].updateGroupData();
    }

    for (it = dict.begin(); it != dict.end(); it++) {
        Group group = world[it->first];
        std::vector<int> a = it->second;
        int j = 0;
        for (int i = 0; i < a.size(); i++) {
            group.removeAgentByIndex(a[i-j]);
            j++;
        }
    }
}

void lightningEmoji(std::vector<Group>& world) {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 randomizer;
    randomizer.seed((unsigned long)seed);

    std::uniform_real_distribution<> defect(0.0, 0.5);
    std::uniform_real_distribution<> coop(0.0, 1.0); //percentage of payoff that we take

    float price = 0;
    float conversion = 0;
    float haircut = 0;

    for (int i (0); i < world.size(); ++i) {
        haircut  = coop(randomizer);
        world[i].setInstitutions(haircut * world[i].getTaxRate(), haircut * world[i].getSegRate());
        for (int j (0); j < world[i].getSize(); ++j) {
            price = 0;
            conversion = coop(randomizer);
            if (world[i].getAgents()[j].getTrait() == 'c' && conversion <= CONVERSION_CHANCE) {
                price = coop(randomizer);
                world[i].updatePayoffByIndex(j, (1 - price) * world[i].getAgents()[j].getPayoff());
                world[i].setTraitByIndex(j, 'd');
            }
            else if (world[i].getAgents()[j].getTrait() == 'c' && conversion > CONVERSION_CHANCE) {
                price = coop(randomizer);
                world[i].updatePayoffByIndex(j, (1 - price) * world[i].getAgents()[j].getPayoff());
            }
            else {
                price = defect(randomizer);
                world[i].updatePayoffByIndex(j, (1 - price) * world[i].getAgents()[j].getPayoff());
            }
        }
    }
}

int main() {
    /*
    Getting the main simulation up and running
    */

    //Start by creating a vector of groups
    std::vector<Group> world;

    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 randomizer;
    randomizer.seed((unsigned long)seed);

    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::poisson_distribution<> pois(AGENTS_MULTIPLIER); //Poisson noise
    int totalAgents (0);

    for (int i (0); i < INITIAL_GROUPS; ++i) {
        // propCoop, taxRate, segRate, totalPayoff, groupSize
        Group group (0, 0, 0, 0, 0); //setting groups to be of zero size for a second
        int numAgents = pois(randomizer); //average group size is 20, but with Poisson noise

        if (numAgents < 4) {
            numAgents = 4;
            totalAgents += 4;
        }

        for (int i (0); i < numAgents; ++i) {
            Agent agent ('d', 0);
            group.addAgent(agent);
            totalAgents++;
        }

        world.push_back(group);
    }

    std::binomial_distribution<> war (INITIAL_GROUPS, GROUP_CONFLICT_CHANCE); //how big is the war

    //define the file output stuff
    std::ofstream outf ("data.csv");
    float pCoop;
    float avgTRate;
    float avgSRate;
    float shock;

    if (!outf) {
        std::cerr << "Well, cock. Some C++ nonsense means the file output didn't work.\n";
        return 1;
    }

    outf << "Time,Proportion of Cooperators,Average Tax Rate,Average Segmentation Rate,Shock" << std::endl;


    int iterations; //how many times to repeat the simulation
    std::cout << "How many iterations?" << std::endl; //quality of life
    std::cin >> iterations;

    for (int j (0); j < iterations; ++j) { //Now run everything

        //reset output values
        pCoop = 0;
        avgTRate = 0;
        avgSRate = 0;
        shock = 0;

            for (int k (0); k < INITIAL_GROUPS; ++k) { //Within-group phases
            playWithinGroup(world[k]);
            //migrate(world);
            haveChildren(world[k]);
            }

            if (dis(randomizer) <= BLACK_SWAN_CHANCE) { //we have a black swan event
                lightningEmoji(world);
                shock = 1;
            }

            //for the war, shuffle the world and choose the first warSize groups
            std::shuffle(world.begin(), world.end(), randomizer);
            int warSize = war(randomizer);

            if (warSize % 2 != 0) {
                ++warSize;
            }

            int l (0); //this logic lets us use one loop instead of two
            while (l < INITIAL_GROUPS) { //Let's have a war!
                if (l < warSize - 1) {
                    playGroupGame(world[l], world[l+1]);
                    pCoop += (world[l].getPropCoop() + world[l+1].getPropCoop());
                    avgTRate += (world[l].getTaxRate() + world[l+1].getTaxRate());
                    avgSRate += (world[l].getSegRate() + world[l+1].getSegRate());
                    l += 2;
                }
                else {
                    pCoop += world[l].getPropCoop();
                    avgTRate += world[l].getTaxRate();
                    avgSRate += world[l].getSegRate();
                    ++l;
                }
            }

        pCoop /= (float) INITIAL_GROUPS;
        avgTRate /= (float) INITIAL_GROUPS;
        avgSRate /= (float) INITIAL_GROUPS;

        outf << j << "," << pCoop << "," << avgTRate << "," << avgSRate << "," << shock << std::endl;

        }
        outf.close();
        return 0;
    }
