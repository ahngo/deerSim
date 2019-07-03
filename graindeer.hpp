// Global variables and functions for graindeer simulation.

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <stdio.h>
#include <iostream>

// Simulation variables
const float GRAIN_GROWS_PER_MONTH =		8.0;
const float ONE_DEER_EATS_PER_MONTH =	0.5;

const float AVG_PRECIP_PER_MONTH =		6.0;	
const float AMP_PRECIP_PER_MONTH =		6.0;	
const float RANDOM_PRECIP =			    2.0;	

const float AVG_TEMP =				    50.0;	
const float AMP_TEMP =				    20.0;	
const float RANDOM_TEMP =			    10.0;

const float MIDTEMP =			    	40.0;
const float MIDPRECIP =			    	10.0;

// lock/variables for barriers
omp_lock_t	Lock;
int		NumInThreadTeam;
int		NumAtBarrier;
int		NumGone;

// Random functions
unsigned int seed = 0;  // a thread-private variable

// Random number generator; float version
float Ranf( unsigned int *seedp,  float low, float high )
{
        float r = (float) rand_r( seedp );              // 0 - RAND_MAX

        return(   low  +  r * ( high - low ) / (float)RAND_MAX   );
}

// Random number generator; int version
int Ranf( unsigned int *seedp, int ilow, int ihigh )
{
        float low = (float)ilow;
        float high = (float)ihigh + 0.9999f;

        return (int)(  Ranf(seedp, low,high) );
}

// Barrier functions
void InitBarrier( int n )
{
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock( &Lock );
}

void WaitBarrier()
{
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
                        NumGone = 0;
                        NumAtBarrier = 0;
                        // let all other threads get back to what they were doing
		            	// before this one unlocks, knowing that they might immediately
		            	// call WaitBarrier( ) again:
                        while( NumGone != NumInThreadTeam-1 );
                        omp_unset_lock( &Lock );
                        return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
        NumGone++;			// this flags how many threads have returned
}

// Environment variable calculation functions

float calcTemp(int NowMonth)
{
    float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );
    float temp = AVG_TEMP - AMP_TEMP * cos( ang );
    
    temp += Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );
    return temp;
}

float calcPrecip(int NowMonth)
{
    float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );
    float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
    float NowPrecip = precip + Ranf( &seed,  -RANDOM_PRECIP, RANDOM_PRECIP );
    if( NowPrecip < 0. )
    	NowPrecip = 0.;
    	
    return NowPrecip;
}

// Squaring function
float SQR( float x )
{
        return x*x;
}