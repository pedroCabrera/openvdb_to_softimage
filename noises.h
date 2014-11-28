#pragma once
#include "MathUtils.h"
#include <vector>
#include <algorithm>

#undef min
#undef max

class CWorleyNoise
{
public:
	inline CWorleyNoise () { _init (); };
	inline CWorleyNoise ( int seed  ) { _init(seed); } ;


	inline void Reseed ( int seed, bool lazy=true )
	{
		if ( m_lastSeed == seed && lazy )
			return;
		m_lastSeed = seed;

	};

	inline float GetWorleyNoise3D( float xin, float yin, float zin )
	{
		float  v2[3],mv2[3];
		float new_at[3];
		int int_at[3];

		float F = 999999.9f;

		/* Make our own local copy, multiplying to make mean(F[0])==1.0  */
		new_at [0] = xin * m_densityAdjust;
		new_at [1] = yin * m_densityAdjust;
		new_at [2] = zin * m_densityAdjust;

		/* Find the integer cube holding the hit point */
		int_at[0] = _fastfloor(new_at[0]); /* A macro could make this slightly faster */
		int_at[1] = _fastfloor(new_at[1]);
		int_at[2] = _fastfloor(new_at[2]);

		// Test the central cube for closest point(s)
		_evalPoint3(int_at[0], int_at[1], int_at[2],  new_at, F);

		// We test if neighbor cubes are even POSSIBLE contributors by examining the combinations of the sum of the squared distances from the cube's lower or upper corners
		v2[0] = new_at[0] - int_at[0];
		v2[1] = new_at[1] - int_at[1];
		v2[2] = new_at[2] - int_at[2];
		mv2[0] = (1.0f-v2[0])*(1.0f-v2[0]);
		mv2[1] = (1.0f-v2[1])*(1.0f-v2[1]);
		mv2[2] = (1.0f-v2[2])*(1.0f-v2[2]);
		v2[0] *= v2[0];
		v2[1] *= v2[1];
		v2[2] *= v2[2];

		// Test 6 facing neighbors of center cube. These are closest and most likely to have a close feature point
		if (v2[0]<F)
			_evalPoint3(int_at[0]-1, int_at[1]  , int_at[2] ,   new_at, F   );
		if (v2[1]<F)
			_evalPoint3(int_at[0]  , int_at[1]-1, int_at[2] ,   new_at, F   );
		if (v2[2]<F)
			_evalPoint3(int_at[0]  , int_at[1]  , int_at[2]-1,   new_at, F   );

		if (mv2[0]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]  , int_at[2]  ,   new_at, F    );
		if (mv2[1]<F) 
			_evalPoint3(int_at[0]  , int_at[1]+1, int_at[2]  ,   new_at, F   );
		if (mv2[2]<F) 
			_evalPoint3(int_at[0]  , int_at[1]  , int_at[2]+1,   new_at, F   );

		// Test 12 "edge cube" neighbors if necessary. They're next closest.
		if ( v2[0]+ v2[1]<F) 
			_evalPoint3(int_at[0]-1, int_at[1]-1, int_at[2]  ,   new_at, F   );
		if ( v2[0]+ v2[2]<F)
			_evalPoint3(int_at[0]-1, int_at[1]  , int_at[2]-1,   new_at, F   );
		if ( v2[1]+ v2[2]<F) 
			_evalPoint3(int_at[0]  , int_at[1]-1, int_at[2]-1,   new_at, F   );
		if (mv2[0]+mv2[1]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]+1, int_at[2]  ,   new_at, F   );
		if (mv2[0]+mv2[2]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]  , int_at[2]+1,   new_at, F   );
		if (mv2[1]+mv2[2]<F) 
			_evalPoint3(int_at[0]  , int_at[1]+1, int_at[2]+1,   new_at, F   );
		if ( v2[0]+mv2[1]<F) 
			_evalPoint3(int_at[0]-1, int_at[1]+1, int_at[2]  ,   new_at, F   );
		if ( v2[0]+mv2[2]<F) 
			_evalPoint3(int_at[0]-1, int_at[1]  , int_at[2]+1,   new_at, F   );
		if ( v2[1]+mv2[2]<F) 
			_evalPoint3(int_at[0]  , int_at[1]-1, int_at[2]+1,   new_at, F   );
		if (mv2[0]+ v2[1]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]-1, int_at[2]  ,   new_at, F   );
		if (mv2[0]+ v2[2]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]  , int_at[2]-1,   new_at, F   );
		if (mv2[1]+ v2[2]<F) 
			_evalPoint3(int_at[0]  , int_at[1]+1, int_at[2]-1,   new_at, F   );

		// Final 8 "corner" cubes
		if ( v2[0]+ v2[1]+ v2[2]<F) 
			_evalPoint3(int_at[0]-1, int_at[1]-1, int_at[2]-1,   new_at, F   );
		if ( v2[0]+ v2[1]+mv2[2]<F)
			_evalPoint3(int_at[0]-1, int_at[1]-1, int_at[2]+1,   new_at, F   );
		if ( v2[0]+mv2[1]+ v2[2]<F) 
			_evalPoint3(int_at[0]-1, int_at[1]+1, int_at[2]-1,   new_at, F   );
		if ( v2[0]+mv2[1]+mv2[2]<F)
			_evalPoint3(int_at[0]-1, int_at[1]+1, int_at[2]+1,   new_at, F   );
		if (mv2[0]+ v2[1]+ v2[2]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]-1, int_at[2]-1,   new_at, F   );
		if (mv2[0]+ v2[1]+mv2[2]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]-1, int_at[2]+1,   new_at, F   );
		if (mv2[0]+mv2[1]+ v2[2]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]+1, int_at[2]-1,   new_at, F   );
		if (mv2[0]+mv2[1]+mv2[2]<F) 
			_evalPoint3(int_at[0]+1, int_at[1]+1, int_at[2]+1,   new_at, F   );


		// We're done! Convert everything to right size scale

		return  ( 1.0f - sqrtf(F)*(1.0f/m_densityAdjustRev) *0.9f );      

	}

	// pseudo-4d because we dont use hypercube, instead a "soft" chaotic cycled motion is applied on cells, still looks very good
	inline float GetWorleyNoise4D( double time, float xin, float yin, float zin )
	{
		float  v2[3],mv2[3];
		float new_at[3];
		int int_at[3];


		m_nextSeedOffset = floor( time );
		time -= floor( time ); // results to [0:1)
		float F = 999999.9f;

		/* Make our own local copy, multiplying to make mean(F[0])==1.0  */
		new_at [0] = xin * m_densityAdjust;
		new_at [1] = yin * m_densityAdjust;
		new_at [2] = zin * m_densityAdjust;

		/* Find the integer cube holding the hit point */
		int_at[0] = _fastfloor(new_at[0]); /* A macro could make this slightly faster */
		int_at[1] = _fastfloor(new_at[1]);
		int_at[2] = _fastfloor(new_at[2]);

		// Test the central cube for closest point(s)
		_evalPoint4(int_at[0], int_at[1], int_at[2], time, new_at, F);

		// We test if neighbor cubes are even POSSIBLE contributors by examining the combinations of the sum of the squared distances from the cube's lower or upper corners
		v2[0] = new_at[0] - int_at[0];
		v2[1] = new_at[1] - int_at[1];
		v2[2] = new_at[2] - int_at[2];
		mv2[0] = (1.0f-v2[0])*(1.0f-v2[0]);
		mv2[1] = (1.0f-v2[1])*(1.0f-v2[1]);
		mv2[2] = (1.0f-v2[2])*(1.0f-v2[2]);
		v2[0] *= v2[0];
		v2[1] *= v2[1];
		v2[2] *= v2[2];

		// Test 6 facing neighbors of center cube. These are closest and most likely to have a close feature point
		if (v2[0]<F)
			_evalPoint4(int_at[0]-1, int_at[1]  , int_at[2] , time,  new_at, F   );
		if (v2[1]<F)
			_evalPoint4(int_at[0]  , int_at[1]-1, int_at[2] , time,  new_at, F   );
		if (v2[2]<F)
			_evalPoint4(int_at[0]  , int_at[1]  , int_at[2]-1,time,   new_at, F   );

		if (mv2[0]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]  , int_at[2]  , time,  new_at, F    );
		if (mv2[1]<F) 
			_evalPoint4(int_at[0]  , int_at[1]+1, int_at[2]  ,  time, new_at, F   );
		if (mv2[2]<F) 
			_evalPoint4(int_at[0]  , int_at[1]  , int_at[2]+1, time,  new_at, F   );

		// Test 12 "edge cube" neighbors if necessary. They're next closest.
		if ( v2[0]+ v2[1]<F) 
			_evalPoint4(int_at[0]-1, int_at[1]-1, int_at[2]  , time,  new_at, F   );
		if ( v2[0]+ v2[2]<F)
			_evalPoint4(int_at[0]-1, int_at[1]  , int_at[2]-1,time,   new_at, F   );
		if ( v2[1]+ v2[2]<F) 
			_evalPoint4(int_at[0]  , int_at[1]-1, int_at[2]-1, time,  new_at, F   );
		if (mv2[0]+mv2[1]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]+1, int_at[2]  , time,  new_at, F   );
		if (mv2[0]+mv2[2]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]  , int_at[2]+1, time,  new_at, F   );
		if (mv2[1]+mv2[2]<F) 
			_evalPoint4(int_at[0]  , int_at[1]+1, int_at[2]+1, time,  new_at, F   );
		if ( v2[0]+mv2[1]<F) 
			_evalPoint4(int_at[0]-1, int_at[1]+1, int_at[2]  , time,  new_at, F   );
		if ( v2[0]+mv2[2]<F) 
			_evalPoint4(int_at[0]-1, int_at[1]  , int_at[2]+1, time,  new_at, F   );
		if ( v2[1]+mv2[2]<F) 
			_evalPoint4(int_at[0]  , int_at[1]-1, int_at[2]+1, time,  new_at, F   );
		if (mv2[0]+ v2[1]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]-1, int_at[2]  , time,  new_at, F   );
		if (mv2[0]+ v2[2]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]  , int_at[2]-1, time,  new_at, F   );
		if (mv2[1]+ v2[2]<F) 
			_evalPoint4(int_at[0]  , int_at[1]+1, int_at[2]-1, time,  new_at, F   );

		// Final 8 "corner" cubes
		if ( v2[0]+ v2[1]+ v2[2]<F) 
			_evalPoint4(int_at[0]-1, int_at[1]-1, int_at[2]-1, time,  new_at, F   );
		if ( v2[0]+ v2[1]+mv2[2]<F)
			_evalPoint4(int_at[0]-1, int_at[1]-1, int_at[2]+1, time,  new_at, F   );
		if ( v2[0]+mv2[1]+ v2[2]<F) 
			_evalPoint4(int_at[0]-1, int_at[1]+1, int_at[2]-1, time,  new_at, F   );
		if ( v2[0]+mv2[1]+mv2[2]<F)
			_evalPoint4(int_at[0]-1, int_at[1]+1, int_at[2]+1, time,  new_at, F   );
		if (mv2[0]+ v2[1]+ v2[2]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]-1, int_at[2]-1, time,  new_at, F   );
		if (mv2[0]+ v2[1]+mv2[2]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]-1, int_at[2]+1, time,  new_at, F   );
		if (mv2[0]+mv2[1]+ v2[2]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]+1, int_at[2]-1, time,  new_at, F   );
		if (mv2[0]+mv2[1]+mv2[2]<F) 
			_evalPoint4(int_at[0]+1, int_at[1]+1, int_at[2]+1, time,  new_at, F   );


		// We're done! Convert everything to right size scale

		return  ( 1.0f - sqrtf(F)*(1.0f/m_densityAdjustRev) *0.9f );  
	}

	// spectral versions
	inline float GetSpectralWorleyNoise3D (  const float in_x,const float in_y, const float in_z, int nbOctaves, float lacunarity, float persistence  )
	{

		nbOctaves =  Clamp ( 1, INT_MAX, nbOctaves ) ;

		float total = 0;
		float frequency = 1; 
		float amplitude = 1;

		float maxAmplitude = 0.f;

		for( int i=0; i < nbOctaves; i++ )
		{
			total += GetWorleyNoise3D( in_x * frequency, in_y * frequency, in_z * frequency ) * amplitude;

			frequency *= lacunarity;
			maxAmplitude += amplitude;
			amplitude *= persistence;
		}

		return total / maxAmplitude;

	};

	inline float GetSpectralWorleyNoise4D ( const float in_time, const float in_x,const float in_y, const float in_z, int nbOctaves, float lacunarity, float persistence  )
	{
		// lacunarity should be 0.7-0.9 when multiple octaves used
		nbOctaves =  Clamp ( 1, INT_MAX, nbOctaves ) ;
		float total = 0;
		float frequency = 1; 
		float amplitude = 1;

		float maxAmplitude = 0.f;

		for( int i=0; i < nbOctaves; i++ )
		{
			total += GetWorleyNoise4D( frequency* in_time, in_x * frequency, in_y * frequency, in_z * frequency ) * amplitude;

			frequency *= lacunarity;
			maxAmplitude += amplitude;
			amplitude *= persistence;
		}

		return total / maxAmplitude;

	};

protected:

	inline void _init ( int seed = 12345 )
	{
		m_densityAdjust =  0.398150f/1.8f;
		m_densityAdjustRev = 1.8f*0.398150f;

		int tempArr1[256]=
		{4,3,1,1,1,2,4,2,2,2,5,1,0,2,1,2,2,0,4,3,2,1,2,1,3,2,2,4,2,2,5,1,2,3,2,2,2,2,2,3,
		2,4,2,5,3,2,2,2,5,3,3,5,2,1,3,3,4,4,2,3,0,4,2,2,2,1,3,2,2,2,3,3,3,1,2,0,2,1,1,2,
		2,2,2,5,3,2,3,2,3,2,2,1,0,2,1,1,2,1,2,2,1,3,4,2,2,2,5,4,2,4,2,2,5,4,3,2,2,5,4,3,
		3,3,5,2,2,2,2,2,3,1,1,4,2,1,3,3,4,3,2,4,3,3,3,4,5,1,4,2,4,3,1,2,3,5,3,2,1,3,1,3,
		3,3,2,3,1,5,5,4,2,2,4,1,3,4,1,5,3,3,5,3,4,3,2,2,1,1,1,1,1,2,4,5,4,5,4,2,1,5,1,1,
		2,3,3,3,2,5,2,3,3,2,0,2,1,1,4,2,1,3,2,1,2,2,3,2,5,5,3,4,5,5,2,4,4,5,3,2,2,2,1,4,
		2,3,3,4,2,5,4,2,4,2,2,2,4,5,3,2};
		memcpy ( m_poissonPointsLUT, tempArr1, sizeof(tempArr1) );

		Reseed ( seed , false );
	};

	inline static int _fastfloor(float x)
	{
		int xi = (int)x;
		return x<xi ? xi-1 : xi;
	};

	inline void _evalPoint3(int xi, int yi, int zi, 	float * pos, float  & F)
	{
		float d[3],f[3];
		float d2;
		unsigned int seed = 702395077*xi + 915488749*yi + 2120969693*zi + m_lastSeed;
		int  count = m_poissonPointsLUT[seed>>24];/* 256 element lookup table. Use MSB */
		seed = 1402024253*seed + 586950981; /* churn the seed with good Knuth LCG */

		for (int j = 0; j < count; j++) /* test and insert each point into our solution */
		{
			seed = 1402024253*seed+586950981; /* churn */

			f[0] = (seed+0.5f)*(1.0f/4294967296.0f); 
			seed = 1402024253*seed+586950981; 
			f[1] = (seed+0.5f)*(1.0f/4294967296.0f);
			seed = 1402024253*seed+586950981; 
			f[2] = (seed+0.5f)*(1.0f/4294967296.0f);
			seed = 1402024253*seed+586950981;

			d[0] = xi + f[0] - pos[0]; 
			d[1] = yi + f[1] - pos[1];
			d[2] = zi + f[2] - pos[2];

			d2 = d[0]* d[0] +  d[1]* d[1] +  d[2]* d[2]; 
			d2 < F ? F = d2 : NULL;	/* Is this point close enough to rememember? */ 

		}

		return;
	};

	inline void _evalPoint4(int xi, int yi, int zi, float time, 	float * pos, float  & F)
	{
		float d[3];
		float f[4][3]; 
		float d2;
		unsigned int seed = 702395077*xi + 915488749*yi + 2120969693*zi + m_lastSeed;
		int  count = m_poissonPointsLUT[seed>>24];/* 256 element lookup table. Use MSB */
		seed = 1402024253*seed + 586950981; /* churn the seed with good Knuth LCG */


		seed += m_nextSeedOffset * 1402024253;	
		unsigned int seed1 = seed + 1402024253;
		unsigned int seed2 = seed1 + 1402024253;
		unsigned int seedm1 = seed - 1402024253;

		for (int j = 0; j < count; j++) /* test and insert each point into our solution */
		{

			seedm1 = 1402024253*seedm1+586950981; /* churn */
			f[0][0] = (seedm1+0.5f)*(1.0f/4294967296.0f); 
			seedm1 = 1402024253*seedm1+586950981; 
			f[0][1] = (seedm1+0.5f)*(1.0f/4294967296.0f);
			seedm1 = 1402024253*seedm1+586950981; 
			f[0][2] = (seedm1+0.5f)*(1.0f/4294967296.0f);
			seedm1 = 1402024253*seedm1+586950981;


			seed = 1402024253*seed+586950981; /* churn */
			f[1][0] = (seed+0.5f)*(1.0f/4294967296.0f); 
			seed = 1402024253*seed+586950981; 
			f[1][1] = (seed+0.5f)*(1.0f/4294967296.0f);
			seed = 1402024253*seed+586950981; 
			f[1][2] = (seed+0.5f)*(1.0f/4294967296.0f);
			seed = 1402024253*seed+586950981;


			seed1 = 1402024253*seed1+586950981;
			f[2][0] = (seed1+0.5f)*(1.0f/4294967296.0f); 
			seed1 = 1402024253*seed1+586950981; 
			f[2][1] = (seed1+0.5f)*(1.0f/4294967296.0f);
			seed1 = 1402024253*seed1+586950981; 
			f[2][2] = (seed1+0.5f)*(1.0f/4294967296.0f);
			seed1 = 1402024253*seed1+586950981;

			seed2 = 1402024253*seed2+586950981;
			f[3][0] = (seed2+0.5f)*(1.0f/4294967296.0f); 
			seed2 = 1402024253*seed2+586950981; 
			f[3][1] = (seed2+0.5f)*(1.0f/4294967296.0f);
			seed2 = 1402024253*seed2+586950981; 
			f[3][2] = (seed2+0.5f)*(1.0f/4294967296.0f);
			seed2 = 1402024253*seed2+586950981;

			d[0] = xi + Cuberp (f[0][0],f[1][0],f[2][0],f[3][0],time ) - pos[0]; 
			d[1] = yi + Cuberp (f[0][1],f[1][1],f[2][1],f[3][1],time ) - pos[1];
			d[2] = zi + Cuberp (f[0][2],f[1][2],f[2][2],f[3][2],time ) - pos[2];

			d2 = d[0]* d[0] +  d[1]* d[1] +  d[2]* d[2]; 

			d2 < F ? F = d2 : NULL;	/* Is this point close enough to rememember? */ 

		}


		return;
	};

	int m_lastSeed;
	int m_poissonPointsLUT[256];
	float m_densityAdjust;
	float m_densityAdjustRev;

	// for 4D
	int m_nextSeedOffset;
};


class CSimplexNoise
{


	// ######################3
protected:
	// reseeding
	struct PermPair_t
	{
		short m_uniqeval;
		short m_randval;

		bool operator < ( const PermPair_t a )
		{
			return m_randval<a.m_randval;
		};
	};

	struct Grad
	{
		float x, y, z, w;
		Grad():x(0),y(0),z(0),w(0){};
		Grad(float x_, float y_, float z_):x(x_),y(y_),z(z_),w(0){}
		Grad(float x_, float y_, float z_, float w_):x(x_),y(y_),z(z_),w(w_){}
	};

	// #########################################################
	// #########################################################
public:
	inline CSimplexNoise ( )  { _init ();  };
	inline CSimplexNoise (  int seed ) { _init (seed);  };


	inline void Reseed ( int seed, bool lazy=true )
	{
		if ( m_lastSeed == seed && lazy )
			return;
		m_lastSeed = seed;

		srand ( m_lastSeed );
		for ( int i=0;i<256;++i )
		{
			m_permTable[i].m_randval = rand();
			m_permTable[i].m_uniqeval = i;
		};

		// sort out by rands
		std::sort ( m_permTable.begin(), m_permTable.begin()+256 );
		memcpy (& m_permTable[256], &m_permTable[0], sizeof(PermPair_t)*256 );

		for(int i=0; i<512; i++)	
			m_permTableMod12[i] = m_permTable[i].m_uniqeval % 12;			
	};


	// ##################################################################################

	inline	float GetSimplexNoise2D(float xin, float yin)
	{
		float n0, n1, n2; // Noise contributions from the three corners
		// Skew the input space to determine which simplex cell we're in
		float s = (xin+yin)*F2; // Hairy factor for 2D
		int i = _fastfloor(xin+s);
		int j = _fastfloor(yin+s);
		float t = (i+j)*G2;
		float X0 = i-t; // Unskew the cell origin back to (x,y) space
		float Y0 = j-t;
		float x0 = xin-X0; // The x,y distances from the cell origin
		float y0 = yin-Y0;
		// For the 2D case, the simplex shape is an equilateral triangle.
		// Determine which simplex we are in.
		int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
		if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
		else {i1=0; j1=1;} // upper triangle, YX order: (0,0)->(0,1)->(1,1)
		// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
		// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
		// c = (3-sqrt(3))/6
		float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
		float y1 = y0 - j1 + G2;
		float x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
		float y2 = y0 - 1.0 + 2.0 * G2;
		// Work out the hashed gradient indices of the three simplex corners
		int ii = i & 255;
		int jj = j & 255;
		int gi0 = m_permTableMod12[ii+m_permTable[jj].m_uniqeval];
		int gi1 = m_permTableMod12[ii+i1+m_permTable[jj+j1].m_uniqeval];
		int gi2 = m_permTableMod12[ii+1+m_permTable[jj+1].m_uniqeval];
		// Calculate the contribution from the three corners
		float t0 = 0.5f - x0*x0-y0*y0;
		if(t0<0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * _dot(m_grad3[gi0], x0, y0); // (x,y) of grad3 used for 2D gradient
		}
		float t1 = 0.5f - x1*x1-y1*y1;
		if(t1<0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * _dot(m_grad3[gi1], x1, y1);
		}
		float t2 = 0.5f - x2*x2-y2*y2;
		if(t2<0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * _dot(m_grad3[gi2], x2, y2);
		}
		// Add contributions from each corner to get the final noise value.
		// The result is scaled to return values in the interval [-1,1].
		return 70.0 * (n0 + n1 + n2);
	}


	inline	float GetSimplexNoise3D(float xin, float yin, float zin)
	{
		float n0, n1, n2, n3; // Noise contributions from the four corners
		// Skew the input space to determine which simplex cell we're in
		float s = (xin+yin+zin)*F3; // Very nice and simple skew factor for 3D
		int i = _fastfloor(xin+s);
		int j = _fastfloor(yin+s);
		int k = _fastfloor(zin+s);
		float t = (i+j+k)*G3;
		float X0 = i-t; // Unskew the cell origin back to (x,y,z) space
		float Y0 = j-t;
		float Z0 = k-t;
		float x0 = xin-X0; // The x,y,z distances from the cell origin
		float y0 = yin-Y0;
		float z0 = zin-Z0;
		// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
		// Determine which simplex we are in.
		int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
		int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords
		if(x0>=y0) {
			if(y0>=z0)
			{ i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; } // X Y Z order
			else if(x0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; } // X Z Y order
			else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; } // Z X Y order
		}
		else { // x0<y0
			if(y0<z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; } // Z Y X order
			else if(x0<z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; } // Y Z X order
			else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; } // Y X Z order
		}
		// A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
		// a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
		// a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
		// c = 1/6.
		float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
		float y1 = y0 - j1 + G3;
		float z1 = z0 - k1 + G3;
		float x2 = x0 - i2 + 2.0*G3; // Offsets for third corner in (x,y,z) coords
		float y2 = y0 - j2 + 2.0*G3;
		float z2 = z0 - k2 + 2.0*G3;
		float x3 = x0 - 1.0 + 3.0*G3; // Offsets for last corner in (x,y,z) coords
		float y3 = y0 - 1.0 + 3.0*G3;
		float z3 = z0 - 1.0 + 3.0*G3;
		// Work out the hashed gradient indices of the four simplex corners
		int ii = i & 255;
		int jj = j & 255;
		int kk = k & 255;
		int gi0 = m_permTableMod12[ii+m_permTable[jj+m_permTable[kk].m_uniqeval].m_uniqeval];
		int gi1 = m_permTableMod12[ii+i1+m_permTable[jj+j1+m_permTable[kk+k1].m_uniqeval].m_uniqeval];
		int gi2 = m_permTableMod12[ii+i2+m_permTable[jj+j2+m_permTable[kk+k2].m_uniqeval].m_uniqeval];
		int gi3 = m_permTableMod12[ii+1+m_permTable[jj+1+m_permTable[kk+1].m_uniqeval].m_uniqeval];
		// Calculate the contribution from the four corners
		float t0 = 0.5f - x0*x0 - y0*y0 - z0*z0;
		if(t0<0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * _dot(m_grad3[gi0], x0, y0, z0);
		}
		float t1 = 0.5f - x1*x1 - y1*y1 - z1*z1;
		if(t1<0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * _dot(m_grad3[gi1], x1, y1, z1);
		}
		float t2 = 0.5f - x2*x2 - y2*y2 - z2*z2;
		if(t2<0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * _dot(m_grad3[gi2], x2, y2, z2);
		}
		float t3 = 0.5f- x3*x3 - y3*y3 - z3*z3;
		if(t3<0) n3 = 0.0;
		else {
			t3 *= t3;
			n3 = t3 * t3 * _dot(m_grad3[gi3], x3, y3, z3);
		}
		// Add contributions from each corner to get the final noise value.
		// The result is scaled to stay just inside [-1,1]
		return 32.0*(n0 + n1 + n2 + n3);
	};


	inline	float GetSimplexNoise4D(float x, float y, float z, float w)
	{
		float n0, n1, n2, n3, n4; // Noise contributions from the five corners
		// Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
		float s = (x + y + z + w) * F4; // Factor for 4D skewing
		int i = _fastfloor(x + s);
		int j = _fastfloor(y + s);
		int k = _fastfloor(z + s);
		int l = _fastfloor(w + s);
		float t = (i + j + k + l) * G4; // Factor for 4D unskewing
		float X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
		float Y0 = j - t;
		float Z0 = k - t;
		float W0 = l - t;
		float x0 = x - X0; // The x,y,z,w distances from the cell origin
		float y0 = y - Y0;
		float z0 = z - Z0;
		float w0 = w - W0;
		// For the 4D case, the simplex is a 4D shape I won't even try to describe.
		// To find out which of the 24 possible simplices we're in, we need to
		// determine the magnitude ordering of x0, y0, z0 and w0.
		// Six pair-wise comparisons are performed between each possible pair
		// of the four coordinates, and the results are used to rank the numbers.
		int rankx = 0;
		int ranky = 0;
		int rankz = 0;
		int rankw = 0;
		x0 > y0 ? rankx++:  ranky++;
		x0 > z0 ? rankx++:  rankz++;
		x0 > w0 ? rankx++:  rankw++;
		y0 > z0  ?ranky++:  rankz++;
		y0 > w0 ? ranky++:  rankw++;
		z0 > w0  ?rankz++:  rankw++;
		int i1, j1, k1, l1; // The integer offsets for the second simplex corner
		int i2, j2, k2, l2; // The integer offsets for the third simplex corner
		int i3, j3, k3, l3; // The integer offsets for the fourth simplex corner
		// simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
		// Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
		// impossible. Only the 24 indices which have non-zero entries make any sense.
		// We use a thresholding to set the coordinates in turn from the largest magnitude.
		// Rank 3 denotes the largest coordinate.
		i1 = rankx >= 3 ? 1 : 0;
		j1 = ranky >= 3 ? 1 : 0;
		k1 = rankz >= 3 ? 1 : 0;
		l1 = rankw >= 3 ? 1 : 0;
		// Rank 2 denotes the second largest coordinate.
		i2 = rankx >= 2 ? 1 : 0;
		j2 = ranky >= 2 ? 1 : 0;
		k2 = rankz >= 2 ? 1 : 0;
		l2 = rankw >= 2 ? 1 : 0;
		// Rank 1 denotes the second smallest coordinate.
		i3 = rankx >= 1 ? 1 : 0;
		j3 = ranky >= 1 ? 1 : 0;
		k3 = rankz >= 1 ? 1 : 0;
		l3 = rankw >= 1 ? 1 : 0;
		// The fifth corner has all coordinate offsets = 1, so no need to compute that.
		float x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
		float y1 = y0 - j1 + G4;
		float z1 = z0 - k1 + G4;
		float w1 = w0 - l1 + G4;
		float x2 = x0 - i2 + 2.0*G4; // Offsets for third corner in (x,y,z,w) coords
		float y2 = y0 - j2 + 2.0*G4;
		float z2 = z0 - k2 + 2.0*G4;
		float w2 = w0 - l2 + 2.0*G4;
		float x3 = x0 - i3 + 3.0*G4; // Offsets for fourth corner in (x,y,z,w) coords
		float y3 = y0 - j3 + 3.0*G4;
		float z3 = z0 - k3 + 3.0*G4;
		float w3 = w0 - l3 + 3.0*G4;
		float x4 = x0 - 1.0 + 4.0*G4; // Offsets for last corner in (x,y,z,w) coords
		float y4 = y0 - 1.0 + 4.0*G4;
		float z4 = z0 - 1.0 + 4.0*G4;
		float w4 = w0 - 1.0 + 4.0*G4;
		// Work out the hashed gradient indices of the five simplex corners
		int ii = i & 255;
		int jj = j & 255;
		int kk = k & 255;
		int ll = l & 255;
		int gi0 = m_permTable[ii+m_permTable[jj+m_permTable[kk+m_permTable[ll].m_uniqeval].m_uniqeval].m_uniqeval].m_uniqeval % 32;
		int gi1 = m_permTable[ii+i1+m_permTable[jj+j1+m_permTable[kk+k1+m_permTable[ll+l1].m_uniqeval].m_uniqeval].m_uniqeval].m_uniqeval % 32;
		int gi2 = m_permTable[ii+i2+m_permTable[jj+j2+m_permTable[kk+k2+m_permTable[ll+l2].m_uniqeval].m_uniqeval].m_uniqeval].m_uniqeval % 32;
		int gi3 = m_permTable[ii+i3+m_permTable[jj+j3+m_permTable[kk+k3+m_permTable[ll+l3].m_uniqeval].m_uniqeval].m_uniqeval].m_uniqeval % 32;
		int gi4 = m_permTable[ii+1+m_permTable[jj+1+m_permTable[kk+1+m_permTable[ll+1].m_uniqeval].m_uniqeval].m_uniqeval].m_uniqeval % 32;
		// Calculate the contribution from the five corners
		float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0 - w0*w0;
		if(t0<0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * _dot(m_grad4[gi0], x0, y0, z0, w0);
		}
		float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1 - w1*w1;
		if(t1<0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * _dot(m_grad4[gi1], x1, y1, z1, w1);
		}
		float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2 - w2*w2;
		if(t2<0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * _dot(m_grad4[gi2], x2, y2, z2, w2);
		}
		float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3 - w3*w3;
		if(t3<0) n3 = 0.0;
		else {
			t3 *= t3;
			n3 = t3 * t3 * _dot(m_grad4[gi3], x3, y3, z3, w3);
		}
		float t4 = 0.6f - x4*x4 - y4*y4 - z4*z4 - w4*w4;
		if(t4<0) n4 = 0.0;
		else {
			t4 *= t4;
			n4 = t4 * t4 * _dot(m_grad4[gi4], x4, y4, z4, w4);
		}
		// Sum up and scale the result to cover the range [-1,1]
		return 27.0 * (n0 + n1 + n2 + n3 + n4);
	};


	// spectral version consist of multiple frequenses ( few bands of noise with different settings )
	inline float GetSpectralSimplexNoise4D ( const float in_time, const float in_x,const float in_y, const float in_z, int nbOctaves, float lacunarity, float persistence  )
	{

		nbOctaves = nbOctaves =  Clamp ( 1, INT_MAX, nbOctaves ) ;

		float total = 0;
		float frequency = 1; 
		float amplitude = 1;

		float maxAmplitude = 0.0001;

		for( int i=0; i < nbOctaves; i++ )
		{
			total += GetSimplexNoise4D( in_x * frequency, in_y * frequency, in_z * frequency, in_time*frequency ) * amplitude;

			frequency *= lacunarity;
			maxAmplitude += amplitude;
			amplitude *= persistence;
		}

		return total / maxAmplitude;

	};


protected:



	// Skewing and unskewing factors for 2, 3, and 4 dimensions
	float F2; 
	float G2; 
	float F3;
	float G3 ;
	float F4 ;
	float G4; 


	Grad m_grad3[12];
	Grad m_grad4[32];

	int m_lastSeed;
	std::vector <PermPair_t> m_permTable;
	short m_permTableMod12 [ 512 ];

	inline void _init ( int seed = 12345 )
	{
		// Skewing and unskewing factors for 2, 3, and 4 dimensions
		F2 = 0.5*(sqrt(3.0)-1.0);
		G2 = (3.0-sqrt(3.0))/6.0;
		F3 = 1.0/3.0;
		G3 = 1.0/6.0;
		F4 = (sqrt(5.0)-1.0)/4.0;
		G4 = (5.0-sqrt(5.0))/20.0;


		Grad tmparr3 []=		
		{Grad(1,1,0), Grad(-1,1,0), Grad(1,-1,0), Grad(-1,-1,0),
		Grad(1,0,1), Grad(-1,0,1), Grad(1,0,-1), Grad(-1,0,-1),
		Grad(0,1,1), Grad(0,-1,1), Grad(0,1,-1), Grad(0,-1,-1) };
		memcpy ( m_grad3,tmparr3,sizeof(tmparr3) );


		Grad tmparr4[]= 
		{
			Grad(0,1,1,1), Grad(0,1,1,-1), Grad(0,1,-1,1), Grad(0,1,-1,-1),
			Grad(0,-1,1,1), Grad(0,-1,1,-1), Grad(0,-1,-1,1), Grad(0,-1,-1,-1),
			Grad(1,0,1,1), Grad(1,0,1,-1), Grad(1,0,-1,1), Grad(1,0,-1,-1),
			Grad(-1,0,1,1), Grad(-1,0,1,-1), Grad(-1,0,-1,1), Grad(-1,0,-1,-1),
			Grad(1,1,0,1), Grad(1,1,0,-1), Grad(1,-1,0,1), Grad(1,-1,0,-1),
			Grad(-1,1,0,1), Grad(-1,1,0,-1), Grad(-1,-1,0,1), Grad(-1,-1,0,-1),
			Grad(1,1,1,0), Grad(1,1,-1,0), Grad(1,-1,1,0), Grad(1,-1,-1,0),
			Grad(-1,1,1,0), Grad(-1,1,-1,0), Grad(-1,-1,1,0), Grad(-1,-1,-1,0)
		};
		memcpy ( m_grad4,tmparr4,sizeof(tmparr4) );




		m_permTable.resize ( 512 );
		Reseed ( seed, false );

	};


	inline static int _fastfloor(float x)
	{
		int xi = (int)x;
		return x<xi ? xi-1 : xi;
	};
	inline static float _dot(Grad g, float x, float y)
	{
		return g.x*x + g.y*y;
	};
	inline static float _dot(Grad g, float x, float y, float z)
	{
		return g.x*x + g.y*y + g.z*z;
	};
	inline static float _dot(Grad g, float x, float y, float z, float w)
	{
		return g.x*x + g.y*y + g.z*z + g.w*w;
	};
};
