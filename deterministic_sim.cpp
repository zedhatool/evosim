/*
    I was initially going to build this as an option into our original simulation but we need
    fundamentally different data structures to do this deterministic updating than our original
    stochastic simulation
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
    Storage for our beautiful boys (global constants)
*/
const float GROUP_CONFLICT_CHANCE (0.25); //value from BCH. We may need to have this vary over time, see fig 5
const int INITIAL_GROUPS = 100; //I randomly chose ten. We can change this later
const float INSTITUTIONAL_CHANGE_CHANCE (0.1); //benchmark from BCH
const float TIME_STEP (0.1); //time step for RK4 integration
const int N_ITERATIONS (1);
const float INDIVIDUAL_MUTATION_RATE (0.005);

class Group {
/*
    We don't need a tax rate for the deterministic updating, but we do need a segmentation rate.
    The segmentation rate actually affects the replicator dynamics, but the tax rate does not.
    The tax rate won't affect the order of payoffs and hence won't modify the structure of the game.
*/
public:
    float proportionCooperative;
    float segmentationRate;
    float groupPayoff;

    /*
        Constructors
    */
    Group() {
        proportionCooperative = 0;
        segmentationRate = 0;
        groupPayoff = 0;
    }
    Group(float pCoop, float sRate, float gPayoff) {
        proportionCooperative = pCoop;
        segmentationRate = sRate;
        groupPayoff = gPayoff;
    }

    float getPropCoop() {
        return proportionCooperative;
    }
    float getSegRate() {
        return segmentationRate;
    }
    void setPropCoop(float newCoop) {
        proportionCooperative = newCoop;
    }
    void setSegRate(float newSeg) {
        segmentationRate = newSeg;
    }
    void setPayoff(float newPayoff) {
        groupPayoff = newPayoff;
    }
    float getPayoff() {
        return groupPayoff;
    }
};

/*
    the replicator equation for the PD with segmentation rate seg is
    y * (seg * R + (1 - seg) * (y * R + (1 - y) * S) - y * (seg * R + (1 - seg) * (y * R + (1 - y) * S))
        - (1 - y) * (seg * P + (1 - seg) * (y * T + (1 - y) * P)))
    where y is the proportion of cooperators. We'll update this by doing some RK4 integration
    steps
*/
float replicatorEquation(float pCoop, float seg) {
    float S = 0; //Sucker's payoff
    float T = 1; //Temptation to defect
    float R = 0.6; //Reward for cooperation
    float P = 0.3; //Punishment for mutual defection

    return pCoop * (seg * R + (1 - seg) * (pCoop * R + (1 - pCoop) * S)
        - pCoop * (seg * R + (1 - seg) * (pCoop * R + (1 - pCoop) * S))
        - (1 - pCoop) * (seg * P + (1 - seg) * (pCoop * T + (1 - pCoop) * P)))
        - pCoop * INDIVIDUAL_MUTATION_RATE + (1 - pCoop) * INDIVIDUAL_MUTATION_RATE; //I think I got that right
}

/*
    Run a single RK4 iteration
*/
void playWithinGroup(Group& group, float dt, int N) {
    float newPCoop = group.getPropCoop();

    for (int i (0); i < N; ++i) {
        float k1 = dt * replicatorEquation(newPCoop, group.getSegRate());
        float k2 = dt * replicatorEquation(newPCoop + 0.5 * k1, group.getSegRate()); //no explicit t dep
        float k3 = dt * replicatorEquation(newPCoop + 0.5 * k2, group.getSegRate());
        float k4 = dt * replicatorEquation(newPCoop + k3, group.getSegRate());

        newPCoop += (k1 + 2 * k2 + 2 * k3 + k4) / 6.0;
    }
    group.setPropCoop(newPCoop);
}

/*
    Since the groups now have infinite size, we just compare which one is more cooperative
*/
void playBetweenGroups(Group& groupOne, Group& groupTwo) {
    float S = 0; //Sucker's payoff
    float T = 1; //Temptation to defect
    float R = 0.6; //Reward for cooperation
    float P = 0.3; //Punishment for mutual defection
    float pCoopOne = groupOne.getPropCoop();
    float segOne = groupOne.getSegRate();
    float pCoopTwo = groupTwo.getPropCoop();
    float segTwo = groupTwo.getSegRate();

    float expectedPayoffOne = pCoopOne * (segOne * R + (1 - segOne) * (pCoopOne * R + (1 - pCoopOne) * S))
    + (1 - pCoopOne) * (segOne * P + (1 - segOne) * (pCoopOne * T + (1 - pCoopOne) * P));

    float expectedPayoffTwo = pCoopTwo * (segTwo * R + (1 - segTwo) * (pCoopTwo * R + (1 - pCoopTwo) * S))
    + (1 - pCoopTwo) * (segTwo * P + (1 - segTwo) * (pCoopTwo * T + (1 - pCoopTwo) * P));

    expectedPayoffOne *= (1 - segOne * segOne);
    expectedPayoffTwo *= (1 - segTwo * segTwo);

    if (expectedPayoffOne >= expectedPayoffTwo) {//break ties in favour of group one
        groupTwo.setPropCoop(groupOne.getPropCoop());
        groupTwo.setSegRate(groupOne.getSegRate());
    }
    else {
        groupOne.setPropCoop(groupTwo.getPropCoop());
        groupOne.setSegRate(groupTwo.getSegRate());
    }
}

/*
    Alternative way of thinking about top-level game as a game of chicken. Will implement this for
    the stochastic model as well. The groups compete over a resource of value V. A fight costs F > V.
    If they both back down, they split V evenly. If both fight, they get (V - F) / 2. If one fights
    and the other backs down, the winner gets all of V and the loser gets nothing. Then,
    we do updating by payoff on a group level as well! We can determine a group's type
    by how cooperative/defective they are. Defective groups fight, cooperative
    groups back down.
*/
void playChickenGame(Group& groupOne, Group& groupTwo) {
    float V (0.5); //value of the resource
    float F (1.0); //cost of a fight

    if (groupOne.getPropCoop() < 0.5 && groupTwo.getPropCoop() < 0.5) { //let's have a war!
        groupOne.setPayoff(0.5 * (V - F));
        groupTwo.setPayoff(0.5 * (V - F));
    }
    else if (groupOne.getPropCoop() < 0.5 && groupTwo.getPropCoop() >= 0.5) { //group one takes it all
        groupOne.setPayoff(V);
        groupTwo.setPayoff(0);
    }
    else if (groupOne.getPropCoop() >= 0.5 && groupTwo.getPropCoop() >= 0.5) { //split evenly
        groupOne.setPayoff(0.5 * V);
        groupTwo.setPayoff(0.5 * V);
    }
    else {
        groupOne.setPayoff(0);
        groupTwo.setPayoff(V);
    }
}

std::vector<Group> replicateGroups(std::vector<Group>& world) { //produce a new world
    std::mt19937 randomizer;
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    randomizer.seed((unsigned long)seed);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    //do stochastic updating like in BCH model for players, but for groups
    float total (0);
    for (int i (0); i < world.size(); ++i) {
        total += world[i].getPayoff();
    } //calculate total payoff

    int worldSize = world.size();
    std::vector<int> subintervals;
    std::vector<float> weights;

    for (size_t j = 0; j <= worldSize; ++j) {
        subintervals.push_back(j);
    }
    for (size_t k = 0; k < worldSize; ++k) {
        float pj = world[k].getPayoff();
        weights.push_back(pj / total);
    }

    //This defines a probability mass function on subintervals given by the indices of groups
    std::piecewise_constant_distribution<> d(subintervals.begin(), subintervals.end(), weights.begin());

    int spreaders (0);
    std::vector<Group> newWorld; //the thing we're gonna return

    while (spreaders < worldSize) {
        float randResult = d(randomizer);
        int randIndex = (int) randResult;

        newWorld.push_back(world[randResult]);
        ++spreaders;
    }

    return newWorld;
}

void mutate(Group& group) {
    std::mt19937 randomizer;
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    randomizer.seed((unsigned long)seed);
    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::uniform_int_distribution<> coin(0, 1);

    if (dis(randomizer) <= INSTITUTIONAL_CHANGE_CHANCE && coin(randomizer) == 1 && group.getSegRate() <= 0.5) {
        group.setSegRate(group.getSegRate() + 0.1);
    } //segRate go up
    else if (dis(randomizer) <= INSTITUTIONAL_CHANGE_CHANCE && coin(randomizer) == 0 && group.getSegRate() >= 0.1) {
        group.setSegRate(group.getSegRate() - 0.1);
    } //segRate go down
    //can add individual-level mutations later, if need be
}

int main() {
    //generate groups
    std::mt19937 randomizer;
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    randomizer.seed((unsigned long)seed);

    float pCoop;
    float avgSRate;

    std::uniform_real_distribution<> dis(0.0, 1.0); //uniform distribution on [0, 1]
    std::vector<Group> world;
    std::vector<Group> dummyWorld;

    std::binomial_distribution<> war (INITIAL_GROUPS, GROUP_CONFLICT_CHANCE); //how big is the war

    //float ic (0);
    //float ic2 (0);
    for (int i (0); i < INITIAL_GROUPS; ++i) {
        //ic = dis(randomizer);
        //ic2 = dis(randomizer);
        Group group (0.0, 0.0, 0.0);
        world.push_back(group);
    }

    //define the file output stuff
    std::ofstream outf ("data.csv");

    if (!outf) {
        std::cerr << "Well, cock. Some C++ nonsense means the file output didn't work.\n";
        return 1;
    }

    outf << "Time,Proportion of Cooperators,Average Segmentation Rate" << std::endl;

    int iterations; //how many times to repeat the simulation
    std::cout << "How many iterations?" << std::endl; //quality of life
    std::cin >> iterations;

    for (int j (0); j < iterations; ++j) {

        pCoop = 0;
        avgSRate= 0;

        for (int i (0); i < world.size(); ++i) { //within-group phase
            playWithinGroup(world[i], TIME_STEP, N_ITERATIONS);
            mutate(world[i]);
        }

        //for the war, shuffle the world and choose the first warSize groups
        std::shuffle(world.begin(), world.end(), randomizer);
        int warSize = war(randomizer);

        if (warSize % 2 != 0) {
            ++warSize;
        }

        int l (0); //this logic lets us use one loop instead of two
        while (l < world.size()) { //Let's have a war!
            if (l < warSize - 1) {
                //playBetweenGroups(world[l], world[l+1]);
                playChickenGame(world[l], world[l+1]);
                pCoop += (world[l].getPropCoop() + world[l+1].getPropCoop());
                avgSRate += (world[l].getSegRate() + world[l+1].getSegRate());
                l += 2;
            }
            else {
                pCoop += world[l].getPropCoop();
                avgSRate += world[l].getSegRate();
                ++l;
            }
        }

    dummyWorld = replicateGroups(world);
    world = dummyWorld;

    pCoop /= INITIAL_GROUPS;
    avgSRate /= INITIAL_GROUPS;

    outf << j << "," << pCoop << "," << avgSRate << std::endl;
    }

    outf.close();
    return 0;
}
