/* Description: Graindeer simulation main code. Uses graindeer.hpp.
* Uses OpenMP barriers with each thread controlling separate variables.
* GrainDeer(): Controls natural deer growth, based on grain supply
* Grain(): Controls grain growth, based on precipitation and temperature
* Watcher(): Controls precipitation and temperature, prints variables
* MyAgent(): In May/June, a random number between 1-3 deer are birthed
* In August to December, a random number between 0 to 3 deer are culled
* This variable will be known as DeerChange.
* 
* Separate phases occur throughout the simulation, where each thread must
* reach a barrier before the next phase begins.
* Calculation Phase: Deer, grain are calculated based on current values
* Assignment Phase: Deer and grain are moved to their respective "now" values
* Environment Phase: Environment variables are calculated and assigned
* MyAgent Phase: Deer numbers changed based on births or hunts
* Printing Phase: Variables are printed out to console. A local copy of 
* the previous environment variables will be printed. Month is incremented.
* Prints: month, temp, precip, deer, grain height and DeerChange
*********************************************************************/

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <stdio.h>
#include <iostream>
#include "graindeer.hpp"
using std::cout;

// Default number of threads
#ifndef NUMT
#define NUMT		4
#endif

// Global simulation variables
int	NowYear = 2019;		// 2019 - 2024
int	NowMonth = 0;		// 0 - 11
int overallMonth = 0;
float	NowPrecip = calcPrecip(NowMonth);		// inches of rain per month
int NowNumDeer = 1;
float NowHeight =  1.;
float NowTemp = calcTemp(NowMonth);
int NextNumDeer = 1;
float NextHeight = 1.;
int deerChange = 0;

// This function controls natural deer growth.
void GrainDeer()
{
    // Calculation Phase
    // Number of deer increases with adequate grain supply.
    int nextNumDeer = NowNumDeer;
    if (NowNumDeer > NowHeight)
    {
        nextNumDeer--;
    }
    // Otherwise number of deer decreases.
    else if (NowNumDeer <= NowHeight)
    {
        nextNumDeer++;
    }
    // Calculation Phase Done
    WaitBarrier();
    // Assignment Phase
    NowNumDeer = nextNumDeer;
    // Assignment Phase Done
    // Watcher() Environment Phase
    WaitBarrier();
    // MyAgent phase
    WaitBarrier();
    // Watcher() Print Phase
    WaitBarrier();
    
}

void Grain(const int NowNumDeer, float &NowHeight, const float NowTemp, const float NowPrecip)
{
    // Calculation Phase
    float NextHeight = NowHeight;
    // Get environment factors on grain growth
    float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );
    float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );
    // Grow the grain based on factors
    NextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
    // Deer eat grain
    NextHeight -= (float) NowNumDeer * ONE_DEER_EATS_PER_MONTH;
    // Clamp height to 0
    if (NextHeight < 0)
    {
        NextHeight = 0;
    }
    // Calculation Phase Done
    WaitBarrier();
    // Assignment Phase
    NowHeight = NextHeight;
    // Assignment Phase Done
    // Watcher() Environment Phase
    WaitBarrier();
    // MyAgent phase
    WaitBarrier();
    // Watcher() Print Phase
    WaitBarrier();
}

void Watcher(int NowNumDeer, float NowHeight)
{
    // Computation Phase
    WaitBarrier();
    // Assignment Phase
    WaitBarrier();
    // Watcher changes environment variables
    NowMonth++;
    if (NowMonth > 11)
    {
        NowYear++;
        NowMonth = 0;
    }
    
    float oldTemp = NowTemp;
    float oldPrecip = NowPrecip;
    
    NowTemp = calcTemp(NowMonth);
    NowPrecip = calcPrecip(NowMonth);
    // MyAgent phase
    WaitBarrier();
    // MyAgent done, time to print
    WaitBarrier();
    
    //printf("Time: %i %i\n", NowMonth, NowYear);
    //printf("Deer: %i Grain: %f\n", NowNumDeer, NowHeight);
    //cout << "Hunted: " << hunted << "\n";
    
    float cmHeight = NowHeight * 2.54;
    float cmPrecip = oldPrecip * 2.54;
    float celsius = (oldTemp - 32) * (5./9.);
    
    overallMonth++;
    
    cout << overallMonth << "\t" << celsius << "\t" << cmPrecip << "\t" << NowNumDeer << "\t"
    << cmHeight << "\t" << deerChange << "\n";
    
    
}

void MyAgent()
{
    // Computation Phase
    WaitBarrier();
    // Assignment Phase
    WaitBarrier();
    // Watcher() Environment Phase
    WaitBarrier();
    
    deerChange = 0;
    
    if (NowMonth == 4 || NowMonth == 5)
    {
        deerChange = (rand() % 2) + 1;
        NowNumDeer += deerChange;
    }
    else if (NowMonth >= 7)
    {
        deerChange = (rand() % 4);
        
        if ((NowNumDeer - deerChange) < 0)
        {
            deerChange = -1 * NowNumDeer;
            NowNumDeer = 0;
        }
        else 
        {
            NowNumDeer -= deerChange;
            deerChange = deerChange * -1;
        }
    }
    //cout << "deerChange: " << deerChange << "\n";
    // Watcher() Print Phase
    WaitBarrier();
}


int main()
{
    unsigned int seed = 0;
    
    InitBarrier(NUMT);
    
    omp_set_num_threads( 4 );	// same as # of sections
    while ( NowYear < 2025 )
    {
    	// compute a temporary next-value for this quantity
    	// based on the current state of the simulation:
    	#pragma omp parallel sections
        {
            
        	#pragma omp section
        	{
        		GrainDeer();
        	}
        
        	#pragma omp section
        	{
        		Grain(NowNumDeer, NowHeight, NowTemp, NowPrecip);
        	}
        
        	#pragma omp section
        	{
        		Watcher(NowNumDeer, NowHeight);
        	}
        
        	#pragma omp section
        	{
        		MyAgent( );	// your own
        	}
        }       
        // implied barrier -- all functions must return in order
        // to allow any of them to get past here
    }
}