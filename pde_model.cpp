#include <iostream>
#include <fstream>
#include <cmath>

/*
    Euler's number
*/
const float E = 2.71828;
const float PI = 3.14159;
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
    return c * (1 - c) * (c * V + 0.5 * (1 - c) * (V - F));
}

/*
    Do two-stage Lax-Wendroff scheme on the advection term and FTCS for the diffusion part
*/
int main() {
    //Xf = 1
    int Tf;

    std::cout << "How Long to run the model?" << std::endl;
    std::cin >> Tf; //getting rid of this for now so the model will run
    //Tf = 1000;

    float dx (0.01);
    float dt (0.001);

    const int xPoints = (int) (1 / dx);
    const int tPoints = (int) (Tf / dt);

    //flatten the array so that we can use dynamic allocation
    float* U = new float[tPoints * xPoints]; //dynamic-size solution matrix for if we let user define Tf

    float mean;
    float variance;
    std::cout << "Define the mean of the initial Gaussian" << std::endl;
    std::cin >> mean;
    std::cout << "And now its variance" << std::endl;
    std::cin >> variance;

    //define initial condition for solution U, just using a sharp Gaussian for initial function
    for (int y (0); y < xPoints; ++y) {
        U[y] = (1 / (std::sqrt(2 * PI * variance))) * std::pow(E, -0.5 * (y * dx - mean) * (y * dx - mean) / variance);
    }

    float uhalfLaxPlus (0);
    float uhalfLaxMinus (0);
    float fhalfLaxPlus (0);
    float fhalfLaxMinus (0);
    float dHalfCrankMinus (0);
    float dHalfCrankPlus (0);

    for (int t (0); t < tPoints; ++t) {
        for (int x (1); x < xPoints - 1; ++x) {
            //Lax-Wendroff Computation
            uhalfLaxPlus = 0.5 * (U[t * xPoints + x + 1] + U[t * xPoints + x])
            - (dt / dx) *
            (U[t * xPoints + x + 1] * getVelocity(x * dx + dx) - U[t * xPoints + x] * getVelocity(x * dx));

            uhalfLaxMinus = 0.5 * (U[t * xPoints + x - 1] + U[t * xPoints + x])
            - (dt / dx) *
            (U[t * xPoints + x] * getVelocity(x * dx) - U[t * xPoints + x - 1] * getVelocity(x * dx - dx));

            fhalfLaxPlus = uhalfLaxPlus * getVelocity(x * dx + 0.5 * dx);
            fhalfLaxMinus = uhalfLaxMinus * getVelocity(x * dx - 0.5 * dx);

            //FTCS Computation
            dHalfCrankPlus = getDiffusion(x * dx + 0.5 * dx);
            dHalfCrankMinus = getDiffusion(x * dx - 0.5 * dx);


            U[(t + 1) * xPoints + x] = U[t * xPoints + x] + 0.5 * (dt / (dx * dx)) *
            (dHalfCrankPlus * (U[(t + 1) * xPoints + x + 1] + U[(t + 1) * xPoints + x - 1] - 2 * U[(t + 1) * xPoints + x]) -
                dHalfCrankMinus * (U[t * xPoints + x + 1] + U[t * xPoints + x - 1] - 2 * U[t * xPoints + x]))
                - (dt / dx) * (fhalfLaxPlus - fhalfLaxMinus);

        } //Now impose Neumann boundary condition
        U[tPoints * xPoints - 1] = U[tPoints * xPoints - 2]; //x = 1
        U[0] = U[1];
    }

    //define the file output stuff
    std::ofstream outf ("pde_out.csv");

    if (!outf) {
        std::cerr << "Well, cock. Some C++ nonsense means the file output didn't work.\n";
        return 1;
    }

    outf << "Time" << ",";

    for (int k (0); k < xPoints - 1; ++k) {
        outf << k * dx << ",";
    }

    outf << 1 << std::endl;

    for (int i (0); i < tPoints; ++i) {
        outf << i * dt << ",";
        for (int j (0); j < xPoints - 1; ++j) {
            outf << U[i * xPoints + j] * dx << ",";
        }
        outf << U[i * xPoints + xPoints - 1] << std::endl;
    }

    outf.close();
    delete [] U;

    return 0;
}
