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
const int N_ITERATIONS (10);

class Group {
/*
    We don't need a tax rate for the deterministic updating, but we do need a segmentation rate.
    The segmentation rate actually affects the replicator dynamics, but the tax rate does not.
    The tax rate won't affect the order of payoffs and hence won't modify the structure of the game.
*/
public:
    float proportionCooperative;
    float taxRate;
    float segmentationRate;

    /*
        Constructors
    */
    Group() {
        proportionCooperative = 0;
        segmentationRate = 0;
    }
    Group(float pCoop, float sRate) {
        proportionCooperative = pCoop;
        segmentationRate = sRate;
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
        - (1 - pCoop) * (seg * P + (1 - seg) * (pCoop * T + (1 - pCoop) * P))); //I think I got that right
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
    if (groupOne.getPropCoop() >= groupTwo.getPropCoop()) {//break ties in favour of group one
        groupTwo.setPropCoop(groupOne.getPropCoop());
        groupTwo.setSegRate(groupOne.getSegRate());
    }
    else {
        groupOne.setPropCoop(groupTwo.getPropCoop());
        groupOne.setSegRate(groupTwo.getSegRate());
    }
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

    std::binomial_distribution<> war (INITIAL_GROUPS, GROUP_CONFLICT_CHANCE); //how big is the war

    float ic (0);
    for (int i (0); i < INITIAL_GROUPS; ++i) {
        ic = dis(randomizer);
        Group group (ic, 0);
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
                playBetweenGroups(world[l], world[l+1]);
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

    pCoop /= INITIAL_GROUPS;
    avgSRate /= INITIAL_GROUPS;

    outf << j << "," << pCoop << "," << avgSRate << std::endl;
    }

    outf.close();
    return 0;
}
