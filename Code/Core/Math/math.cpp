/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Math (MTH)	 											**
**																			**
**	File name:		math.cpp												**
**																			**
**	Created by:		11/24/99	-	mjb										**
**																			**
**	Description:	Math Library code										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#ifdef __PLAT_WN32__
#include <math.h>
#endif
#ifdef __PLAT_XBOX__
#include <math.h>
#endif

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

#include <core/math/math.h>

#if DEBUGGING_REPLAY_RND

#include <sys\timer.h>

#include <string.h>

extern int *gReplayTestRndLine;
extern uint64 *gReplayTestRndFunc;
static int gRndIndex = 0;
static int gTestMode = 0;
#define MAX_RND_TEST_INDEX  3666
#endif



namespace Mth
{

	
/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


/*****************************************************************************
**								Private Types								**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

static int RandSeed;
static int RandA;
static int RandB;

static int RandSeed2;
static int RandC;
static int RandD;

void InitialRand(int a)
{
   RandSeed = a;
   RandA = 314159265;
   RandB = 178453311;
   RandSeed2 = a;
   RandC = 314159265;
   RandD = 178453311;
}

#if DEBUGGING_REPLAY_RND

static bool gFuckedUp = false;

bool RndFuckedUp( void )
{
	if ( gTestMode != 2 )
		return ( false );
	return ( gFuckedUp );
}

int Rnd_DbgVersion(int n, int line, char *file)
{
	
	if ( gRndIndex < MAX_RND_TEST_INDEX )
	{
		if ( gTestMode == 1 )
		{
			// record:
			gReplayTestRndLine[ gRndIndex ] = line;
			strncpy( ( char * )( &gReplayTestRndFunc[ gRndIndex ] ), file, 8 );
			gRndIndex++;
		}
		else if ( gTestMode == 2 )
		{
			// compare:
			if ( line != gReplayTestRndLine[ gRndIndex ] )
			{
				char temp[ 9 ];
				strncpy( temp, ( char * )( &gReplayTestRndFunc[ gRndIndex ] ), 8 );
				temp[ 8 ] = '\0';
				Dbg_Message( "********Rnd Fuckup!********* num %d time %d\ncurrent line %d file %s conflicts with prev line %d file %s",
					Tmr::GetTime( ), gRndIndex, line, file, gReplayTestRndLine[ gRndIndex ], temp );
				gFuckedUp = true;
			}
			else
			{
				gFuckedUp = false;
			}
			gRndIndex++;
		}
	}
	RandSeed=RandSeed*RandA+RandB;
	RandA = (RandA ^ RandSeed) + (RandSeed>>4);
	RandB += (RandSeed>>3) - 0x10101010L;
	return (int)((RandSeed&0xffff) * n)>>16;
}

void SetRndTestMode( int mode )
{
	gTestMode = mode;
	gRndIndex = 0;
}

#else
int Rnd(int n)
{
   RandSeed=RandSeed*RandA+RandB;
   RandA = (RandA ^ RandSeed) + (RandSeed>>4);
   RandB += (RandSeed>>3) - 0x10101010L;
   return (int)((RandSeed&0xffff) * n)>>16;
}
#endif

int Rnd2(int n)
{
   RandSeed2=RandSeed2*RandC+RandD;
   RandC = (RandC ^ RandSeed2) + (RandSeed2>>4);
   RandD += (RandSeed2>>3) - 0x10101010L;
   return (int)((RandSeed2&0xffff) * n)>>16;
}

// Return a random number in the range +/- n
float PlusOrMinus(float n)
{
	float range = (float)(Rnd(10000));
	range -= 5000.0f;
	return n * range / 5000.0f;
}

#if 0
// 8192 makes Kensinf and sinf match to all 6 decimal places, (when x is in the range -2pi to 2pi)
// apart from very occassionally where there might be a .000001 difference.
// The table size must be a power of two.
#define SINF_NUM_DIVISIONS 8192
static float pSinLookup[SINF_NUM_DIVISIONS+5]; // +5 for good measure, since sometimes we look one beyond the end.
void InitSinLookupTable()
{
	// The lookup table covers a whole period (0 to 2pi) of the sin function.
	// This saves a few if's in the sin function.
	for (int i=0; i<SINF_NUM_DIVISIONS+5; ++i)
	{
		pSinLookup[i]=sinf(i*2.0f*3.141592654f/SINF_NUM_DIVISIONS); // Mth::PI won't compile for some reason??
	}
}


float Kensinf(float x)
{
	// Get the index into the table as a floating point value. This value may be negative
	// or out of bounds of the array, but don't worry about that yet.
	float r=(x*(SINF_NUM_DIVISIONS/2))/3.141592654f;
	// Get the integer part.
	int i=(int)r;
	// Make r be the fractional part, so that r is a value between 0 and 1 (or 0 and -1) 
	// that indicates how far we are between the two adjacent entries in the table.
	r-=i;
	
	// Make i be in range, which can be done with an integer 'and' because the table size
	// is chosen to be a power of two.
	// Note that this still works the way we want it to if i is negative.
	// If i is a certain distance below 0, anding it will map it to that distance below the
	// end of the table, which is what we want because the lookup table covers a whole
	// period of the sine function.
	// (Generally, -x & (n-1) is n-x when n is a power of 2, eg -3&7 = 5 = (8-3) )
	i&=(SINF_NUM_DIVISIONS-1);
	
	float *pPoo=pSinLookup+i;
	
	// Hmmm, the following is technically wrong, it should be -- not ++ for when x is negative.
	// However, always using ++ does not appear to affect the accuracy at all, so there's no need to fix it!
	// Doing a -- for when x is negative would mean having to do another if, and yet another to
	// check in case pPoo points to the first element, yuk.
	// Using ++ even when x is negative works because the gradient does not change
	// much between points. When x is negative, r will be negative, and using the next gradient
	// to move back gives roughly the same answer as using the previous gradient. Wahay!!
	float a=*pPoo++;
	float b=*pPoo;
	return a+(b-a)*r;
}

float Kencosf(float x)
{
	float r=(x*(SINF_NUM_DIVISIONS/2))/3.141592654f;
	int i=(int)r;
	r-=i;

	// This is the only difference from Kensinf.
	i+=(SINF_NUM_DIVISIONS/4); // Add a quarter period.
	
	i&=(SINF_NUM_DIVISIONS-1);
	
	float *pPoo=pSinLookup+i;
	
	float a=*pPoo++;
	float b=*pPoo;
	return a+(b-a)*r;
	/*
    float sqr = x*x;
    float result = 0.03705f;
    result *= sqr;
    result -= 0.4967;
    result *= sqr;
    result += 1.0f;
    return result;
	*/
}
#endif

float Kenacosf(float x)
{
	// Got this formula off the internet, forgot where though ...
	if (x<0.0f)
	{
		x=-x;
		float root = sqrtf(1.0f-x);
	
		float  result = -0.0187293f;
		result *= x;
		result += 0.0742610f;
		result *= x;
		result -= 0.2121144f;
		result *= x;
		result += 1.5707288f;
		result *= root;
		return 3.141592654f-result;
	}
	else
	{
		float root = sqrtf(1.0f-x);
	
		float  result = -0.0187293f;
		result *= x;
		result += 0.0742610f;
		result *= x;
		result -= 0.2121144f;
		result *= x;
		result += 1.5707288f;
		result *= root;
		return result;
	}		
}

float FRunFilter( float target, float current, float delta )
{
	
	if ( target < current )
	{
		if ( ( current - target ) > fabsf( delta ) )
		{
			return ( current - fabsf( delta ) );
		}
	   	return ( target );
	}
	if ( ( target - current ) > fabsf( delta ) )
	{
		return ( current + fabsf( delta ) );
	}
	return ( target );
}

int RunFilter( int target, int current, int delta )
{
	if ( target < current )
	{
		if ( current - target > abs( delta ) )
			return ( target );
		return ( current - abs( delta ) );
	}
	if ( target - current > abs( delta ) )
		return ( target );
	return ( current + abs( delta ) );
}


// returns atan(y/x) with appropiate sign determinations
// although it does not work, as we cant; find atan ..... 

/*
float Atan2 (float y, float x)
{

    if (x == 0.0f)
	{
        if (y < 0.0f)     return(-PI/2.0f);

        else             return( PI/2.0f);

    }
	else
	{
		
	
        if (x < 0.0f) {
            if (y < 0.0f)
				return(atan(y/x)-PI);
            else
				return(atan(y/x)+PI);
        }
		else
		{
			return(atan(y/x));
		}

    }

}
*/

} // namespace Mth
