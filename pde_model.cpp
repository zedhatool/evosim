#include <iostream>
#include <fstream>


/*
    Payoffs for lower level game
*/
const float S = 0; //Sucker's payoff
const float T = 1; //Temptation to defect
const float R = 0.6; //Reward for cooperation
const float P = 0.3; //Punishment for mutual defection

/*
    Payoffs for upper level game
*/
const float V = 0.5;
const float F = 1;

// p_t = -(getVelocity)_c
float getVelocity(float c) {
    return c * (1 - c) * (c * (R - T) + (1 - c) * (S - P));
}

float getDiffusion(float c) {
    return c * (1 - c) * (c * V + 0.5 * (1 - c) * (V - F))
}

/*
    Do two-stage Lax-Wendroff scheme on the advection term and Crank-Nicolson for the diffusion part
*/
int main() {
    //Xf = 1
    int Tf;
    std::cout >> "How Long to run the model?" >> std::endl;
    std::cin << Tf;

    float dt (0.01);
    float dx (0.01);

    int xPoints = (int) (1 / dx);

    float U[Tf][xPoints]; //solution matrix
    float uhalfLaxPlus (0);
    float uhalfLaxMinus (0);
    float fhalfLaxPlus (0);
    float fhalfLaxMinus (0);
    float uhalfCrankPlus (0);
    float uhalfCrankMinus (0);

    for (int t (1); t < Tf; ++t) {
        for (int x (1); x < xPoints - 1; ++x) {
            //Lax-Wendroff Step
            uhalfLaxPlus = 0.5 * (U[t][x + 1] + U[t][x]) - (dt / dx) * (U[t][x + 1] * getVelocity(xPoints * dx + dx) - U[t][x] * getVelocity(xPoints * dx));
            uhalfLaxMinus = 0.5 * (U[t][x - 1] + U[t][x]) - (dt / dx) * (U[t][x-1] * getVelocity(xPoints * dx - dx) - U[t][x] * getVelocity(xPoints * dx));
            fhalfLaxPlus = uhalfLaxPlus * getVelocity(xPoints * dx + 0.5 * dx);
            fhalfLaxMinus = uhalfLaxMinus * getVelocity(xPoints * dx - 0.5 * dx);

            //Crank-Nicolson Step

            U[t + 1][x] += U[t][x] - (dt / dx) * (fhalfLaxPlus - fhalfLaxMinus);


        }
    }

    return 0;
}
