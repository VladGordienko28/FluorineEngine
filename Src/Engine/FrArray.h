/*=============================================================================
    FrArray.h: Dynamic array template.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    TArray.
-----------------------------------------------------------------------------*/

//
// A dynamic array template.
//
template<class T> class TArray
{
private:
	// Variables.
	T*			Data;
	Integer		Count;

public:
	// Constructors.
	TArray()
		:	Data( nullptr ),
			Count( 0 )
	{}
	TArray( Integer InitNum )
		:	Data( nullptr ),
			Count( 0 )
	{
		SetNum( InitNum );
	}
	TArray( const TArray<T>& Other )
		:	Data( nullptr ),
			Count( 0 )
	{
		SetNum( Other.Num() );
		for( Integer i=0; i<Other.Num(); i++ )
			Data[i] = Other.Data[i];
	}

	// Destructor.
	~TArray()
	{
		Empty();
	}

	// Return array length.
	inline Integer Num() const 
	{ 
		return Count; 
	}

	// Add an unique item to the array.
	Integer AddUnique( const T& InItem )
	{
		Integer i = FindItem( InItem );
		if( i == -1 )
			i = Push( InItem );
		return i;
	}

	// Cleanup array.
	void Empty()
	{
		SetNum( 0 );
	}

	// Find an item in the array, pretty sad O(n).
	Integer FindItem( const T& InItem ) const
	{
		for( Integer i=0; i < Num(); i++ )
			if( Data[i] == InItem )
				return i;
		return -1;
	}

	// Get a array item by inverse index.
	inline T& Last( Integer InvIdx = 0 )
	{
		assert( InvIdx>=0 && InvIdx<Count );
		return Data[Count-InvIdx-1];
	}

	// Pop the last item in the array.
	T Pop()
	{
		assert( Count > 0 );
		T tmp = Data[Count-1];
		SetNum( Count-1 );
		return tmp;
	}

	// Push a new item to the array and return it
	// index.
	Integer Push( const T& InItem )
	{
		SetNum( Count+1 );
		Data[Count-1] = InItem;
		return Count-1;
	}

	// Remove item from the array fast, but this
	// routine shuffle array.
	void Remove( Integer Idx )
	{
		assert( Idx>=0 && Idx<Count );
		assert( Count>0 );
		Data[Idx] = Data[Count-1];
		SetNum( Count-1 );
	}

	// Remove item from the array slowly, by shifting
	// all items to the released slot.
	void RemoveShift( Integer Idx )
	{
		assert( Idx>=0 && Idx<Count );
		assert( Count>0 );
		for( Integer i=Idx; i<Count-1; i++ )
			Data[i] = Data[i+1];
		SetNum( Count-1 );
	}

	// Swap an array's items by its indexes.
	void Swap( Integer A, Integer B )
	{
		assert( A>=0 && A<Count );
		assert( B>=0 && B<Count );
		T tmp = Data[A];
		Data[A] = Data[B];
		Data[B] = tmp;
	}

	// Remove all items matched to InItem from
	// the array.
	void RemoveUnique( const T& InItem )
	{
		for(;;)
		{
			Integer i = FindItem( InItem );
			if( i == -1 ) break;
			Remove( i );
		}
	}

	// Insert slot/s to the array starting from Index
	// InCount is a how mush slots added.
	void Insert( Integer Index, Integer InCount=1 )
	{
		assert( InCount>=0 );
		assert( Count>=0 );
		assert( Index>=0 && Index<=Count );
		Integer OldCount = Count;
		SetNum( Count + InCount );

		for( Integer i=Count-1; i>Index+InCount; i-- )
			Data[i]	= Data[i-InCount];
	}

	// Quick array sort.
	void Sort( Bool(*SortFunc)( const T& A, const T& B ) )
	{
		try
		{
			if( Count > 1 )
				qSort( 0, Count-1, SortFunc );
		}
		catch( ... )
		{
			error( L"QSort failure" );
		}
	}

	// Reallocate array.
	void SetNum( Integer NewNum )
	{
		if( NewNum == Count )
			return;

		for( Integer i = NewNum; i < Count; i++ )
			((T*)&Data[i])->~T();

		ReallocateArray
		( 
			*((void**)&Data), 
			Count, 
			NewNum, 
			sizeof(T) 
		);
	}

	// Array serialization.
	friend void Serialize( CSerializer& S, TArray<T>& V )
	{
		if( S.GetMode() == SM_Load )
		{
			Integer ArrLen;
			Serialize( S, ArrLen );
			V.Empty();
			V.SetNum( ArrLen );
		}
		else
		{
			Integer ArrLen = V.Num();
			Serialize( S, ArrLen );
		}
		for( Integer i=0; i<V.Num(); i++ )
			Serialize( S, V[i] );
	}

	// Operators.
	inline T& operator[]( Integer i )
	{
		assert( i>=0 && i<Count );
		return Data[i];
	}
	inline const T& operator[]( Integer i ) const
	{
		assert( i>=0 && i<Count );
		return Data[i];
	}
	Bool operator==( const TArray<T>& Other ) const
	{
		if( Num() != Other.Num() )
			return false;
		for( Integer i=0; i<Num(); i++ )
			if( Data[i] != Other.Data[i] )
				return false;
		return true;
	}
	Bool operator!=( const TArray<T>& Other ) const
	{
		if( Num() != Other.Num() )
			return true;
		for( Integer i=0; i<Num(); i++ )
			if( Data[i] != Other.Data[i] )
				return true;
		return false;
	}
	TArray<T>& operator=( const TArray<T>& Other )
	{
		SetNum( Other.Num() );
		for( Integer i=0; i<Num(); i++ )
			Data[i]	= Other[i];
		return *this;
	}

private:
	// Internal.
	void qSort( Integer Min, Integer Max, Bool(*SortFunc)( const T& A, const T& B ) )
	{
		Integer i = Min, j = Max;
		T Middle = Data[Max-(Max-Min)/2];
		while( i < j )
		{
			while( SortFunc( Data[i], Middle ) ) i++;
			while( SortFunc( Middle, Data[j] ) ) j--;
			if( i <= j )
			{
				Swap( i, j );
				i++;
				j--;
			}
		}
		if( Min < j ) qSort( Min, j, SortFunc );
		if( i < Max ) qSort( i, Max, SortFunc );
	}
};


//
// Dynamic array external functions.
//
extern void ReallocateArray( void*& Data, Integer& Count, Integer NewCount, DWord InnerSize );


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/