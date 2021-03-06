/*
 * File:   default_bond.cpp
 * Author: suh
 *
 * Created on May 5, 2013, 12:32 PM
 *
 * Pure CIR bond
 */


#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>

#include "CouponBond.h"

using namespace std;

// intensity for default process;
double lambda(double s)
{
	return 1/(s*s);
    //return 0; // no default
}

int main()
{
    // constants for CIR interest rate process
    double a = 2.0; // mean reversion speed
    double b = 0.05; // mean interest rate
    double sigma_r = 0.4;

    // equity process
    double q = 0.01; //dividend yield;
    double sigma = 0.3;

    double rho = -0.5; // correlation coef between spot and interest rates

	// pure bond parameters
    double T = 1;  // maturity
    double F = 1000; // par value
    double c = 0.02; // coupon rate

    double R_v = 0.4*F; // value recovered from bond default

	CouponBond myBond(F, c, b, a, T, sigma_r);

	// create grid
    double s_max = 300;
    double r_max = 1;
    int I = 50; // number of equity steps
    int J = 50; // number of interest rate steps
    int K = 252; // number of temporal steps
    double v[I+1][J+1];
	double w[I+1][J+1];

    double deltaT = T/K;
    double deltaR = r_max/J;

    double deltaS = s_max/I;

	// set payoff values on grid

	// convertible with no barrier
    for (int i = 0; i < I+1; ++i)
    {
        for (int j = 0; j < J+1; ++j)
        {
			v[i][j] = F;
        }
    }
	for (int j=0; j < J+1; ++j)
	{
     	v[0][j] = R_v;
		v[1][j] = 0.5*(v[0][j] + v[2][j]); // for a uniform grid, the symmetry fits the cubic spline at the average

	}

    ofstream errorOutput("errors.txt");

        /* Begin fully implicit method */

    // each splitting stage will be solved using an iterative method
    // in particular, PSOR, so we set the tolerance level here
    double tol = 0.0005;
	double noIter = 0;

    double temp; // temp values so we can compute the error

    for (int k = 1; k < K + 1; ++k)
    {

        // set initial guess for SOR
        for (int i = 0; i < I + 1; ++i)
            for (int j = 0; j < J + 1; ++j)
			{
                w[i][j] = v[i][j]; // start with last time step values
			}

        double error = 100; // set error large so we at least enter while loop

		noIter = 0;
        while (error > tol)
        {
            error = 0;

            // begin SOR

            // BOUNDARY
            // CONDITIONS
            // for equity
            for (int j = 0; j < J + 1; ++j)
            {
                // store old values so we can check the error afterwards
				temp = v[0][j];

				//v[0][j] = R_v;
				v[0][j] = v[1][j];
				error = max(error, abs(temp-v[0][j]));

            }
            //for interest rate
            for (int i = 1; i < I; ++i)
            {
                // store old values so we can check the error afterwards
                temp = v[i][0];

                // set new values using boundary conditions
                v[i][0] = 2 * v[i][1] - v[i][2]; // vanishing gamma

                // store biggest error so far;
                error = max(error, abs(temp - v[i][0]));
            }



            // INTERIOR
            // compute interior values
            for (int i = 1; i < I; ++i)
			{
				for (int j = 1; j < J; ++j)
				{
					temp = v[i][j];
					v[i][j] = 0.5/deltaR*(j*sigma_r*sigma_r + a*(b-j*deltaR))*v[i][j+1]
							 +0.5/deltaR*(j*sigma_r*sigma_r - a*(b-j*deltaR))*v[i][j-1]
						     +0.5*(i*i*sigma*sigma + i*(j*deltaR - q + lambda(i*deltaS) - 0.5*sigma*sigma))*v[i+1][j]
						     +0.5*(i*i*sigma*sigma - i*(j*deltaR - q + lambda(i*deltaS) - 0.5*sigma*sigma))*v[i-1][j]
							 +w[i][j]/deltaT
						     +1/4 * rho *sqrt(j*deltaR)*sigma_r*sigma*i/deltaR *(v[i+1][j+1] + v[i-1][j-1] - v[i+1][j-1] - v[i-1][j+1]);
							 +lambda(i*deltaS)*R_v;
					v[i][j] = v[i][j] / (1/deltaT + j/deltaR*sigma_r*sigma_r + i*i*sigma*sigma + j*deltaR + lambda(i*deltaS));
					error = max(error, abs(v[i][j] - temp));
				}
			}

            // BOUNDARY
            // CONDITIONS
            // set boundary values:
            // for equity
            for (int j = 0; j < J + 1; ++j)
            {
                // store old values so we can check the error afterwards
                temp = v[I][j];

				v[I][j] = myBond.price(j*deltaR, T-k*deltaT);

                // store biggest error so far;
                error = max(error, abs(temp - v[I][j]));

            }

            // for interest rate
            for (int i = 1; i < I; ++i)
            {
                // store old values so we can check the error afterwards
                temp = v[i][J];

                // set new values
                //v[k][i][J] = 0; // bond is worthless when r=infinity
                v[i][J] = v[i][J - 1]; // vanishing delta
                //v[k][i][J] = 2*v[k][i][J-1] - v[k][i][J-2];

                // store biggest error so far;
                error = max(error, abs(temp - v[i][J]));

            }

            errorOutput << error << endl;
        }
    }




	errorOutput.close();

    //std::ofstream outputFile("bond_check.csv");
    std::ofstream outputFile("default_bond.csv");
    // output initial convertible bond prices
	for (int i = 0; i < I+1; ++i)
	{
		for (int j =0; j < J+1; ++j)
			outputFile << v[i][j] << ", ";
			//outputFile << v[i][j] - myBond.price(j*deltaR, 0) << ","; // to compare to riskfree bond
		outputFile << endl;
	}
	outputFile.close();
}




