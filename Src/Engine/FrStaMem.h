/*=============================================================================
    FrStack.h: Stack based memory allocator.
    Copyright 2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    CMemPool.
-----------------------------------------------------------------------------*/

//
// Fast memory pool allocator.
//
class CMemPool
{
private:
	// Variables.
	String			Name;
	Byte*			Data;
	DWord			Size;
	Byte*			Top;
	Byte*			EndAddr;

public:
	// Constructor.
	CMemPool( const Char* InName, DWord InSize )
	{
		Name	= InName;
		Data	= Top = new Byte[align( InSize, 32 )];
		Size	= InSize;
		EndAddr	= Data + InSize;
	}

	// Destructor.
	~CMemPool()
	{
		delete[] Data;
		Size	= 0;
		Data	= Top = EndAddr = nullptr;
	}

	// Release all pushed items, they are still valid,
	// but may be corrupted.
	inline void PopAll()
	{
		Top = Data;
	}

	// Pop some memory which was allocated
	// after NewTop address.
	inline void Pop( const void* NewTop )
	{
		Top = (Byte*)NewTop;
		if( Top < Data || Top >= EndAddr )
			error( L"Pool '%s' out of range", *Name );
	}

	// Allocate memory fast, without any checks.
	// So be careful, with it.
	inline void* PushFast( DWord Count )
	{
		void* Result = Top;
		Top	+= Count;
		return Result;
	}

	// Allocate a 'dirty 'memory. 
	inline void* Push( DWord Count )
	{
		void* Result = Top;
		Count	= align( Count, 8 );
		Top		+= Count;
		if( Top > EndAddr )
			error( L"Pool '%s' overflow", *Name );
		return Result;
	}

	// Allocate a memory and initialize it.
	// If stack overflows its invoke error.
	inline void* Push0( DWord Count )
	{
		void* Result = Top;
		Count	= align( Count, 8 );
		Top		+= Count;
		if( Top > EndAddr )
			error( L"Pool '%s' overflow", *Name );
		MemZero( Result, Count );
		return Result;
	}

	// Return true, if pool have available
	// Count byte for alloc.
	inline Bool CanPush( DWord Count )
	{
		return (DWord)Top+Count < (DWord)EndAddr;
	}

	// Output debug information about pool.
	void DebugPool() const
	{
		log( L"**CMemPool '%s' info**",					*Name );
		log( L"   Pool: Total allocated %i Kb",			Size / 1024 );
		log( L"   Pool: Now used %i percents of pool",	Integer(Top-Data)*100/Size );
	}
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/