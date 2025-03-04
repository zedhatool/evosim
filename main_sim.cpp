/*
This is a replication of the algorithm found here.
https://sites.santafe.edu/~bowles/artificial_history/algorithm_coevolution.htm
*/

#include <iostream> //where can I install this package?

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
    Agent(char t, float p)
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
    char trait; //'strategy' might be more accurate?
    float payoff
    //array of agents?

    Group(char t, float p /*, array here */)
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
}

/*
Define how the game works
*/
//void PrisonersDilemma

int main() {
    /*
    Actually running the simulation goes here, I am thinking I will have this save some data to a file so we
    can plot and analyze it in python with standard analysis tools, but for now it is just returning a void.
    */
    Agent test ('c', 0);
    std::cout << test.getTrait() << std::endl; //tests to make sure things work, and they do
    return 0;
}
