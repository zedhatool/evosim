#include <iostream>
#include <fstream>
#include <cmath>
#include <array>
#include <map>

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

void solveTriDiag(std::vector<double> &a, std::vector<double> &b, std::vector<double> &c, std::vector<double> &r, std::vector<double> &u) {
    //Algorithm copied from Press et al. section 2.4
    int j;
    int n = a.size();
    double bet;
    std::vector<double> gam(n);
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

    std::cout << "For how long should the model run?" << std::endl;
    std::cin >> Tf; //getting rid of this for now so the model will run
    //int Tf = 1000;

    double dx (0.01);

    int xPoints = (int) (1 / dx);
    std::vector<double> D (xPoints);

    for (int p(0); p < xPoints; ++p) {
        D[p] = getDiffusion(p * dx);
    }

    diffusion_max = std::max_element(D.begin(), D.end());

    float dt = 0.5 * dx / (diffusion_max);

    std::vector<double> U (xPoints); // state at time t
    std::vector<double> V (xPoints); //state at time t + 1
    std::vector<double> A (xPoints); //A = C in this problem, see solveTriDiag function
    std::vector<double> B (xPoints);

    for (int i (0); i < xPoints; ++i) {
        A[i] = -1 * getDiffusion(i * dx) * dt / (dx * dx);
        B[i] = (1 - 2 * A[i]);
    }

    float mean;
    float variance;
    std::cout << "Define the mean of the initial Gaussian." << std::endl;
    std::cin >> mean;
    std::cout << "Define the variance of the initial Gaussian." << std::endl;
    std::cin >> variance;

    for (int y (0); y < xPoints; ++y) {
        U[y] = (1 / std::sqrt(2 * PI * variance)) *
            std::pow(E, -0.5 * (y * dx - mean) * (y * dx - mean) / variance);
    }

    //define the file output stuff
    std::ofstream outf ("pde_out.csv");

    if (!outf) {
        std::cerr << "Well, cock. Some C++ nonsense means the file output didn't work.\n";
        return 1;
    }

    /*
        Set up CSV header
    */

    outf << "Time,";

    for (int j(0); j < xPoints - 1; ++j) {
        outf << j * dx << ",";
    }

    outf << 1 << std::endl;

    double uHalfLaxPlus (0);
    double uHalfLaxMinus (0);
    double fHalfLaxPlus (0);
    double fHalfLaxMinus (0);

    int t (0);
    while (t * dt < Tf) {
        outf << t * dt << ",";
        for (int x (1); x < xPoints; ++x) {
            outf << U[x] << ",";
            uHalfLaxPlus = 0.5 * (U[x + 1] + U[x])
                - (dt / dx) * (getVelocity((x + 1) * dx) - getVelocity(x * dx));
            uHalfLaxMinus = 0.5 * (U[x] + U[x - 1])
                - (dt / dx) * (getVelocity(x * dx) - getVelocity((x - 1) * dx));
            fHalfLaxPlus = uHalfLaxPlus * getVelocity((x + 0.5) * dx);
            fHalfLaxMinus = uHalfLaxMinus * getVelocity((x - 0.5) * dx);

            V[x] = U[x] - (dt / dx) * (fHalfLaxPlus - fHalfLaxMinus); //Lax Step
        }
        outf << std::endl;
        solveTriDiag(A, B, A, U, V); //Crank-Nicolson step
        U = V;
        ++t;
    }

    outf.close();

    return 0;
}
