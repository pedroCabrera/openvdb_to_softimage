#pragma once

#include <math.h>
#include <time.h>

typedef long long LLONG;
typedef unsigned long long ULLONG;

static const float pi = 3.14159265359f;
static const float pi_half = pi / 2.f;
static const float pi_doubled = pi * 2.f;



template < typename T >
inline T Lerp ( T a, T b, float mix )
{
	return a + (b - a)*mix; // linear iterpolation
};

template < typename T >
inline T Coserp(  T a,T b, float mix)
{
  float  mu2 = (1.f-cos(mix*3.14159f))*0.5f;
   return(a*(1.f-mu2)+b*mu2);
};

template < typename T >
inline T Cuberp(  T y0,T y1, T y2,T y3, float mix)
{
 
  float mu2 = mix*mix;
  T a0 = y3 - y2 - y0 + y1;
  T a1 = y0 - y1 - a0;
  T a2 = y2 - y0;
  T a3 = y1;

   return(a0*mix*mu2+a1*mu2+a2*mix+a3);
};

template < typename T1,typename T2 >
inline T1 Maxv ( T1 _a, T2 _b )
{
	return _a > _b ? _a : _b; 
};

template < typename T >
inline T Minv ( T _a, T _b )
{
	return _a < _b ? _a : _b; 
};

template < typename T, typename T1 >
inline bool IsInRangeIncl ( T1 _DownTresh, T1 _UpTresh, T _Value )
{
	return _Value >= _DownTresh ? ( _Value <= _UpTresh ? true : false ) : false;
};

template < typename T, typename T1 >
inline bool IsInRangeExcl ( T1 _DownTresh, T1 _UpTresh, T _Value )
{
	return _Value > _DownTresh ? ( _Value < _UpTresh ? true : false ) : false;
};

template < typename T, typename T1 , typename T2 >
inline T Clamp (   T1 _DownTresh, T2 _UpTresh,  T _Value )
{
	return _Value >= _DownTresh ? ( _Value <= _UpTresh ? _Value : _UpTresh ) : _DownTresh;
};

template < typename T >
inline bool IsPointInBox ( T _Point, T & _BMin, T & _BMax )
{
	if ( ( ( _Point.GetX() >= _BMin.GetX() ) && ( _Point.GetY() >= _BMin.GetY() ) && ( _Point.GetZ() >= _BMin.GetZ() ) ) &&
	     ( ( _Point.GetX() <= _BMax.GetX() ) && ( _Point.GetY() <= _BMax.GetY() ) && ( _Point.GetZ() <= _BMax.GetZ() ) ) ) return true;
	else 
		return false;
};

template < typename T >
inline float GetLength ( T _P1 , T _P2 )
{
	return _P2.SubInPlace(_P1).GetLength ();
};

template < typename T >
inline float GetDistanceToLine ( T _Point, T _L1, T _L2 )
{
	float length = _L2.SubInPlace ( _L1 ).GetLength ();
	return  _L2.Cross ( _L2,  _L1.SubInPlace ( _Point ) ).GetLength () / length;
};

template < typename T, typename T1 >
inline float GetDistanceSquared ( T & in_a, T1 & in_b )
{
	return  
			(in_a.GetX()-in_b.GetX())*(in_a.GetX()-in_b.GetX())+  
			(in_a.GetY()-in_b.GetY())*(in_a.GetY()-in_b.GetY())+
			(in_a.GetZ()-in_b.GetZ())*(in_a.GetZ()-in_b.GetZ());
};

template < typename T >
inline T GetCrossProduct ( T & in_vec3A, T & in_vec3B )
{

   return T( in_vec3A.GetY() * in_vec3B.GetZ() -
			in_vec3A.GetZ() * in_vec3B.GetY(), -in_vec3A.GetX() * in_vec3B.GetZ() +
			in_vec3A.GetZ() * in_vec3B.GetX(), in_vec3A.GetX() * in_vec3B.GetY() -
			in_vec3A.GetY() * in_vec3B.GetX() );
};

template < typename T >
inline T& Normalize ( T & _V )
{
	_V.NormalizeInPlace();
	return  _V;
};

template <class T>
inline void SwapValues ( T & a, T & b )
{
	T c = a;
	a = b;
	b = c;
};

// timer
class TIMER
{
public:
	TIMER () { m_tmInit=clock(); };
	
	double GetElapsedTime () 
	{   
		m_tmFinal=clock()-m_tmInit;
		m_tmInit=clock();
		return (double)m_tmFinal / ((double)CLOCKS_PER_SEC); 
	};

	void Reset ()
	{
		m_tmInit=clock(); return;
	};

private:
	clock_t m_tmInit, m_tmFinal;
};
