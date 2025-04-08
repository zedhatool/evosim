#include <iostream>
#include <fstream>
#include <cmath>
#include <array>

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

void tridag(VecDoub_I &a, VecDoub_I &b, VecDoub_I &c, VecDoub_I &r, VecDoub_O &u) {
    //Algorithm copied from Press et al. section 2.4
    int j, n = a.size();
    Doub bet;
    VecDoub gam(n);
    if (b[0] == 0.0) throw("Error 1 in tridag");

    u[0] = r[0] / (bet = b[0]);

    for (j = 1; j < n; ++j) {
        gam[j] = c[j - 1] / bet;
        bet = b[j] - a[j] * gam[j];

        if (bet == 0.0) throw("Error 2 in tridag");
        u[j] = (r[j] - a[j] * u[j - 1]) / bet;
    }
    for (j = (n-2); j>=0; j--) {
        u[j] -= gam[j + 1] * u[j + 1];
    }
}

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
    //int Tf = 1000;

    double dx (0.01);
    double dt (0.1 * dx);

    int xPoints = (int) (1 / dx);
    int tPoints = (int) (Tf / dt);

    //flatten the array so that we can use dynamic allocation
    int len = (int) (Tf / (dx * dt));
    std::vector<double> U (len);

    for (int k (0); k < tPoints * xPoints; ++k) {
        U[k] = 0;
    }

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

    double uhalfLaxPlus (0);
    double uhalfLaxMinus (0);
    double fhalfLaxPlus (0);
    double fhalfLaxMinus (0);
    double dHalfCrankMinus (0);
    double dHalfCrankPlus (0);

    /*
    U[t * xPoints + x] + 0.5 * (dt / (dx * dx)) *
    ((dHalfCrankPlus * (U[(t + 1) * xPoints + x + 1] - U[(t + 1) * xPoints + x])
        - dHalfCrankMinus * (U[(t + 1) * xPoints + x] - U[(t + 1) * xPoints + x - 1])
        - dHalfCrankPlus * (U[t * xPoints + x + 1] - U[t * xPoints + x])
        - dHalfCrankMinus * (U[t * xPoints + x] - U[t * xPoints + x - 1]))) //our old, wrong scheme
    */

    for (int t (0); t < tPoints - 1; ++t) {
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

            //Lax-Wendroff Step
            U[(t + 1) * xPoints + x] =
                U[t * xPoints + x] - (dt / dx) * (fhalfLaxPlus - fhalfLaxMinus);

            //Crank-Nicolson Step
            tridag()

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
            outf << U[i * xPoints + j] << ",";
        }
        outf << U[i * xPoints + xPoints - 1] << std::endl; // array possibly wack?
    }

    outf.close();
    //delete [] U;

    return 0;
}
