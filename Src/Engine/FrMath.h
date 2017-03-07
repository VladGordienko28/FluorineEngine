/*=============================================================================
    FrMath.h: Fluorine math library.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    Constants.
-----------------------------------------------------------------------------*/

// Common math constans.
#define EPSILON 0.0001f
#define PI 3.141592653f

// Flu math constants.
#define WORLD_SIZE 4096
#define WORLD_HALF (WORLD_SIZE/2)


/*-----------------------------------------------------------------------------
    Declarations.
-----------------------------------------------------------------------------*/

// Forward declaration.
struct TVector;
struct TAngle;
struct TCoords;
struct TRect;
struct TColor;


/*-----------------------------------------------------------------------------
    TVector.
-----------------------------------------------------------------------------*/

//
// A point or direction.
//
struct TVector
{
public:
	Float	X;
	Float	Y;

	// Constructors.
	TVector()
	{}
	TVector( Float InX, Float InY )
		:	X(InX), Y(InY)
	{}
	
	// Operators.
	TVector operator-() const
	{
		return TVector( -X, -Y );
	}
	TVector operator+() const
	{
		return TVector( X, Y );
	}
	TVector operator+( const TVector& V ) const
	{
		return TVector( X + V.X, Y + V.Y );
	}
	TVector operator-( const TVector& V ) const
	{
		return TVector( X - V.X, Y - V.Y );
	}
	Float operator*( const TVector& V ) const
	{
		return X * V.X + Y * V.Y;
	}
	TVector operator*( const Float F ) const
	{
		return TVector( X * F, Y * F );
	}
	Float operator/( const TVector& V ) const
	{
		return X * V.Y - Y * V.X;
	}
	TVector operator/( const Float F ) const
	{
		return TVector( -F * Y, F * X );	
	}
	TVector operator+=( const TVector& V )
	{
		X += V.X;
		Y += V.Y;
		return *this;
	}
	TVector operator-=( const TVector& V )
	{
		X -= V.X;
		Y -= V.Y;
		return *this;
	}
	TVector operator*=( const Float F )
	{
		X *= F;
		Y *= F;
		return *this;
	}
	Bool operator==( const TVector& V ) const
	{
		return X == V.X && Y == V.Y;
	}
	Bool operator!=( const TVector& V ) const
	{
		return X != V.X || Y != V.Y;
	}

	// Functions.
	TVector Cross() const
	{
		return TVector( -Y, X );
	}
	Float Size() const
	{
		return sqrtf( X * X + Y * Y ); 
	}
	Float SizeSquared() const
	{
		return X * X + Y * Y;
	}
	void Normalize()
	{
		Float f = Size();
		if( f > EPSILON )
		{
			X /= f;
			Y /= f;
		}
	}
	void Snap( Float Grid );

	// Friends.
	friend void Serialize( CSerializer& S, TVector& V )
	{
		Serialize( S, V.X );
		Serialize( S, V.Y );
	}
};


/*-----------------------------------------------------------------------------
    TAngle.
-----------------------------------------------------------------------------*/

//
// An angle value 0..0xffff.
//
struct TAngle
{
public:
	Integer	Angle;

	// Constructors.
	TAngle()
		:	Angle(0)
	{}
	TAngle( Integer InAngle )
		:	Angle(InAngle)
	{}
	TAngle( Float InAngle );

	// Operators.
    operator Integer() const
	{
		return Angle & 0xffff;
	}
	operator Bool() const
	{
		return (Angle & 0xffff) != 0;
	}
	TAngle operator+() const
	{
		return TAngle( Angle );
	}
	TAngle operator-() const
	{
		return TAngle( 0xffff - (Angle & 0xffff) );
	}
	TAngle operator+( const TAngle& R ) const
	{
		return TAngle( (Angle + R.Angle) & 0xffff );
	}
	TAngle operator-( const TAngle& R ) const
	{
		return TAngle( (Angle - R.Angle) & 0xffff );
	}
	TAngle operator*( const Float F ) const;

	Bool operator==( const TAngle& R ) const
	{
		return (R.Angle & 0xffff) == (Angle & 0xffff);
	}
	Bool operator!=( const TAngle& R ) const
	{
		return (R.Angle & 0xffff) != (Angle & 0xffff);
	}
	TAngle operator+=( const TAngle& R )
	{
		Angle = (Angle + R.Angle) & 0xffff;
		return *this;
	}
	TAngle operator-=( const TAngle& R )
	{
		Angle = (Angle - R.Angle) & 0xffff;
		return *this;
	}

	// Functions.
	void Snap( Integer Grid );
	Float ToRads() const;
	Float ToDegs() const;
	Float GetCos() const;
	Float GetSin() const;

	// Friends.
	friend void Serialize( CSerializer& S, TAngle& R )
	{
		Serialize( S, R.Angle );
	}
};


/*-----------------------------------------------------------------------------
    TRect.
-----------------------------------------------------------------------------*/

//
// An AABB rectangle.
//
struct TRect
{
public:
	TVector	Min;
	TVector	Max;

	// Constructors.
	TRect()
	{}
	TRect( const TVector InCenter, const TVector InSize )
	{
		TVector Half = InSize * 0.5f;
		Min = InCenter - Half;
		Max = InCenter + Half;
	}
	TRect( const TVector InCenter, const Float InSide )
	{
		Float Half = InSide * 0.5f;
		Min = TVector( InCenter.X - Half, InCenter.Y - Half );
		Max = TVector( InCenter.X + Half, InCenter.Y + Half );
	}
	TRect( const TVector* Verts, Integer NumVerts );

	// Operators.
	Bool operator==( const TRect& R ) const
	{
		return Min == R.Min && Max == R.Max;
	}
	Bool operator!=( const TRect& R ) const
	{
		return Min != R.Min || Max != R.Max;
	}
	operator Bool() const
	{
		return Min != Max;
	}
	TRect operator+( const TVector& V ) const;
	TRect operator+=( const TVector& V );

	// Functions.
	TVector Center() const
	{
		return (Min + Max) * 0.5;
	}
	TVector Size() const
	{
		return Max - Min;
	}
	Bool IsInside( const TVector P ) const
	{
		return P.X >= Min.X && P.X <= Max.X &&
			   P.Y >= Min.Y && P.Y <= Max.Y;
	}
	Float GetExtrema( Integer i ) const
	{
		return ((Float*)&Min)[i];
	}
	Bool AtBorder( const TVector& P, Float Thresh = 0.01 ) const;
	Bool IsOverlap( const TRect& Other ) const;
	Bool LineIntersect( const TVector& A, const TVector& B, TVector& V, TVector& Normal, Float& Time ) const;

	// Friends.
	friend void Serialize( CSerializer& S, TRect& R )
	{
		Serialize( S, R.Min );
		Serialize( S, R.Max );
	}
};


/*-----------------------------------------------------------------------------
    TCoords.
-----------------------------------------------------------------------------*/

//
// A coords system matrix.
// Technically it a coords basis.
// 
struct TCoords
{
public:
	TVector	Origin;
	TVector	XAxis;
	TVector YAxis;

	// Constructors.
	TCoords()
		:	Origin( 0.f, 0.f ),
			XAxis( 1.f, 0.f ),
			YAxis( 0.f, 1.f )
	{}
	TCoords( const TVector& InOrigin )
		:	Origin( InOrigin ),
			XAxis( 1.f, 0.f ),
			YAxis( 0.f, 1.f )
	{}
	TCoords( const TVector& InOrigin, const TVector& InX, const TVector& InY )
		:	Origin( InOrigin ),
			XAxis( InX ),
			YAxis( InY )
	{}
	TCoords( const TVector& InOrigin, const TVector& InX )
		:	Origin( InOrigin ),
			XAxis( InX ),
			YAxis( InX.Cross() )
	{}
	TCoords( const TAngle& R )
		:	Origin( 0.f, 0.f )
	{
		XAxis = TVector( R.GetCos(), R.GetSin() );
		YAxis = XAxis.Cross();
	}
	TCoords( const TVector& InOrigin, const TAngle& R )
		:	Origin( InOrigin )
	{
		XAxis = TVector( R.GetCos(), R.GetSin() );
		YAxis = XAxis.Cross();
	}	

	// Operators.
	Bool operator==( const TCoords& C ) const
	{
		return Origin == C.Origin &&
			   XAxis  == C.XAxis &&
			   YAxis  == C.YAxis;
	}
	Bool operator!=( const TCoords& C ) const
	{
		return Origin != C.Origin ||
			   XAxis  != C.XAxis ||
			   YAxis  != C.YAxis;
	}
	TCoords operator<<( const TVector& V ) const;
	TCoords operator<<( const TAngle& R ) const;
	TCoords operator>>( const TVector& V ) const;
	TCoords operator>>( const TAngle& R ) const;

	// Functions.
	TCoords Transpose() const;

	// Friends.
	friend void Serialize( CSerializer& S, TCoords& C )
	{
		Serialize( S, C.Origin );
		Serialize( S, C.XAxis );
		Serialize( S, C.YAxis );
	}
	static const TCoords Identity;
};


/*-----------------------------------------------------------------------------
    Math templates.
-----------------------------------------------------------------------------*/

//
// General purpose.
//
template<class T> inline T Min( T A, T B )
{
	return A < B ? A : B;
}
template<class T> inline T Max( T A, T B )
{
	return A > B ? A : B;
}
template<class T> inline T Clamp( T V, T A, T B )
{
	return V < A ? A : V > B ? B : V;
}
template<class T> inline void Exchange( T& A, T& B )
{
	T C = A;
	A = B;
	B = C;
}
template<class T> inline T Sqr( T V )
{
	return V * V;
}
template<class T> inline T Lerp( T A, T B, Float Alpha )
{
	return A + ( B - A ) * Alpha;
}
template<class T> inline T Abs( T V )
{
	return (V < (T)0) ? -V : V;
}
template<class T> inline Bool InRange( T V, T A, T B )
{
	return (V>=A) && (V<=B);
}


/*-----------------------------------------------------------------------------
    Math functions.
-----------------------------------------------------------------------------*/

inline Float Sin( Float F )
{
	return sinf( F );
}
inline Float Cos( Float F )
{
	return cosf( F );
}
inline Float Pow( Float Base, Float P )
{
	return pow( Base, P );
}
inline Integer Floor( Float F )
{
	return (Integer)floorf(F);
}
inline Integer Ceil( Float F )
{
	return (Integer)ceilf(F);
}
inline Integer Round( Float F )
{
	return (Integer)roundf(F);
}
inline Integer Trunc( Float F )
{
	return (Integer)truncf(F);
}
inline Float Sqrt( Float F )
{
	return sqrtf( F );
}
inline Float ArcTan( Float X )
{
	return (Float)atan( X );
}
inline Float ArcTan2( Float Y, Float X )
{
	return atan2f( Y, X );
}
inline Float Ln( Float X )
{	
// Fuck :(
#pragma push_macro("log")
#undef log
	return log(X);
#pragma pop_macro("log")
}
inline Float Frac( Float X )
{
	return X - Floor(X);
}
inline void Snap( Float Grid, Float& F )
{
	if( Grid != 0.f ) F = roundf(F / Grid) * Grid;
}
extern inline Float FastSinF( Float F );
extern inline Float FastCosF( Float F );
extern inline Float FastSqrt( Float F );
extern inline Float FastArcTan( Float X );
extern inline Float FastArcTan2( Float Y, Float X );
extern inline Float Sin8192( Integer i );
extern inline Float Wrap( Float V, Float Min, Float Max );
extern inline DWord IntLog2( DWord A );
extern TAngle AngleLerp( TAngle AFrom, TAngle ATo, Float Alpha, Bool bCCW );

//
// Transformation functions.
//
extern inline TVector TransformVectorBy( const TVector& V, const TCoords& C );
extern inline TVector TransformPointBy( const TVector& P, const TCoords& C );


//
// Vector math functions.
//
inline Float Distance( const TVector& A, const TVector& B )
{
	return Sqrt( Sqr(A.X - B.X) + Sqr(A.Y - B.Y) );
}
inline TAngle VectorToAngle( const TVector& V )
{
	return TAngle( ArcTan2( V.Y, V.X ) );
}
inline TVector AngleToVector( const TAngle R )
{
	return TVector( R.GetCos(), R.GetSin() );
}
inline Bool IsWalkable( const TVector& FloorNormal )
{
	return ( FloorNormal.X <= +0.7f ) &&
		   ( FloorNormal.X >= -0.7f ) &&
		   ( FloorNormal.Y > 0.0f );
}
inline Bool PointsAreNear( const TVector& A, const TVector& B, Float Dist )
{
	return !(( Abs(A.X - B.X) > Dist )||
		     ( Abs(A.Y - B.Y) > Dist ));
}
inline Float PointLineDist( TVector& P, TVector& Origin, TVector& Normal )
{
	return ( P - Origin ) * Normal;
}
inline TVector LineSegmentInter( TVector P1, TVector P2, TVector Origin, TVector Normal )
{
	TVector V = P2 - P1;
	return P1 + V* (((Origin - P1) * Normal) / (V * Normal));
}


//
// Polygon functions.
//
extern inline Bool IsConvexPoly( const TVector* Verts, Integer NumVerts );
extern inline Bool IsPointInsidePoly( const TVector P, TVector* Verts, Integer NumVerts );
extern Bool LineIntersectPoly( TVector A, TVector B, TVector* Verts, Integer NumVerts, TVector& V, TVector& Normal );
extern Bool SegmentsIntersect( TVector A1, TVector A2, TVector B1, TVector B2, TVector& V );
extern Bool PointOnSegment( TVector P, TVector A, TVector B, Float Thresh = EPSILON );


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/