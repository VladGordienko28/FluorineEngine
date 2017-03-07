/*=============================================================================
    FrBase.h: Platform specific functions.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    Types.
-----------------------------------------------------------------------------*/

//
// Override standard C++ types.
//
typedef unsigned char		Byte;
typedef signed char			SByte;
typedef signed short		SWord;
typedef unsigned short		Word;
typedef float				Float;
typedef double				Double;
typedef int					Integer;
typedef unsigned int		DWord;
typedef bool				Bool;
typedef wchar_t				Char;
typedef char				AnsiChar;
typedef unsigned long long	QWord;


/*-----------------------------------------------------------------------------
    Macroses.
-----------------------------------------------------------------------------*/

// Its upset me :(
#define WIDEN2(x) L ## x 
#define WIDEN(x) WIDEN2(x) 
#define __WFILE__ WIDEN(__FILE__) 

//
// Output macro.
//
#define assert(expr) { if(!(expr)) error( L"Assertion failed: \"%s\" [File: %s][Line: %i]", L#expr, __WFILE__, __LINE__ ); }
#define log		if( ::GOutput ) ::GOutput->Logf
#define error	if( ::GOutput ) ::GOutput->Errorf
#define warn	if( ::GOutput ) ::GOutput->Warnf

//
// Debug macro.
//
#define benchmark_begin(op) \
{\
	Char* BenchOp = L#op; \
	DWord InitTime = GPlat->Cycles();\

#define benchmark_end \
	log( L"Operation \"%s\" take %d cycles", BenchOp, GPlat->Cycles()-InitTime ); \
}\


//
// Various macro.
//
#define align(value, bound) ((value)+(bound)-1)&(~((bound)-1))
#define freeandnil(Obj) { if( Obj ){ delete Obj; Obj = nullptr; } }
#define array_length(arr) (sizeof(arr)/sizeof(arr[0]))


/*-----------------------------------------------------------------------------
    Memory functions.
-----------------------------------------------------------------------------*/

inline void* MemAlloc( DWord Count )
{
	return calloc( Count, 1 );
}
inline void* MemMalloc( DWord Count )
{
	return malloc( Count );
}
inline void* MemRealloc( void* Addr, DWord NewCount )
{
	return realloc( Addr, NewCount );
}
inline void MemFree( void* Addr )
{
	free( Addr );
}
inline void MemZero( void* Addr, DWord Count )
{
	memset( Addr, 0, Count );
}
inline void MemSet( void* Addr, DWord Count, Byte Value )
{
	memset( Addr, Value, Count );
}
inline Bool MemCmp( const void* A, const void* B, DWord Count )
{
	return memcmp( A, B, Count ) == 0;
}
inline void MemCopy( void* Dst, const void* Src, DWord Count )
{
	memcpy( Dst, Src, Count );
}


/*-----------------------------------------------------------------------------
    CPlatformBase.
-----------------------------------------------------------------------------*/

//
// Platform global functions.
//
class CPlatformBase
{
public:
	virtual Double TimeStamp() = 0;
	virtual Double Now() = 0;
	virtual DWord Cycles() = 0;
	virtual Bool FileExists( String FileName ) = 0;
	virtual Bool DirectoryExists( String Dir ) = 0;
	virtual void ClipboardCopy( Char* Str ) = 0;
	virtual String ClipboardPaste() = 0;
	virtual void Launch( const Char* Target, const Char* Parms ) = 0;
};


/*-----------------------------------------------------------------------------
    Global variables.
-----------------------------------------------------------------------------*/

extern Bool				GIsEditor;
extern CPlatformBase*	GPlat;
extern String			GDirectory;
extern DWord			GFrameStamp;
extern String			GCmdLine[8];


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/