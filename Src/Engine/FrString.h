/*=============================================================================
    FrString.h: Dynamic sizable string.
    Copyright Jun.2016 Vlad Gordienko.
	Totally rewritten Nov.2016.
=============================================================================*/

/*-----------------------------------------------------------------------------
    String.
-----------------------------------------------------------------------------*/

//
// An advanced Unicode string.
//
class String
{
public:
	// Default constructor.
	String()
		:	Self( nullptr )
	{}

	// Copy constructor.
	String( const String& Other )
	{
		Self	= Other.Self;
		if( Self )
			Self->RefsCount++;
	}

	// Characters array constructor.
	String( const Char* Str )
	{
		Self	= nullptr;
		if( Str && *Str )
		{
			Integer L		= wcslen(Str);
			NewString(L);
			MemCopy( Self->Data, Str, L*sizeof(Char) );
			Self->Data[L]	= '\0';
		}
	}

	// Characters array constructor.
	String( const Char* Str, Integer InLen )
	{
		Self	= nullptr;
		if( Str && *Str && InLen )
		{
			NewString(InLen);
			MemCopy( Self->Data, Str, InLen*sizeof(Char) );
			Self->Data[InLen]	= '\0';
		}
	}

	// String destruction.
	~String()
	{
		DeleteString();
	}

	// Return string's hash code.
	DWord HashCode() const
	{
		DWord Hash = 2139062143;
		if( Self )
			for( Char* C = Self->Data; *C; C++ )
				Hash = 37 * Hash + *C;
		return Hash;
	}

	// Return string length.
	inline Integer Len() const
	{
		return Self ? Self->Length : 0;
	}

	// Return references count to this string.
	inline Integer RefsCount() const
	{
		return Self ? Self->RefsCount : 0;
	}

	// String to numberic conversion.
	Bool ToInteger( Integer& OutValue, Integer Default = 0 ) const;
	Bool ToFloat( Float& OutValue, Float Default = 0.f ) const;


	// Operators.
	Char* operator*() const
	{
		return Self ? Self->Data : L"\0";
	}
	String& operator=( const Char* Str )
	{
		DeleteString();
		Integer L	= wcslen(Str);
		if( L )
		{
			NewString(L);
			MemCopy( Self->Data, Str, L*sizeof(Char) );
			Self->Data[L]	= '\0';
		}
		return *this;
	}
	String& operator=( const String& Other )
	{
		DeleteString();
		if( Other.Self )
		{
			Self	= Other.Self;
			Self->RefsCount++;
		}
		return *this;
	}
	Char operator()( Integer i ) const
	{
		return Self ? Self->Data[i] : '\0';
	}
	const Char& operator[]( Integer i ) const
	{
		return Self ? Self->Data[i] : '\0';
	}
	Char& operator[]( Integer i )
	{
		if( Self )
		{
			if( Self->RefsCount > 1 )
			{
				TSelf* New = (TSelf*)MemMalloc(sizeof(TSelf)+(Self->Length+1)*sizeof(Char));
				New->Length	= Self->Length;
				New->RefsCount	= 1;
				MemCopy( New->Data, Self->Data, (Self->Length+1)*sizeof(Char) );
				Self->RefsCount--;
				Self	= New;
			}
			return Self->Data[i];
		}
		else
		{
			static Char bad;
			return bad;
		}
	}
	Bool operator==( const String& Other ) const
	{
		if( Other.Self != Self )
		{
			if( !Other.Self ) return Self == nullptr;
			if( !Self ) return Other.Self == nullptr;

			Integer L1 = Len();
			Integer L2 = Other.Len();
			if( L1 != L2 )
				return false;

			return wcscmp( Self->Data, Other.Self->Data ) == 0;
		}
		else
			return true;
	}
	Bool operator==( const Char* Str ) const
	{
		if( !Str || !*Str ) return Self == nullptr;
		if( !Self ) return !Str || !*Str;

		Integer L1 = Len();
		Integer L2 = wcslen(Str);
		if( L1 != L2 )
			return false;

		return wcscmp( Self->Data, Str ) == 0;
	}
	Bool operator!=( const String& Other ) const
	{
		return !operator==(Other);
	}
	Bool operator!=( const Char* Str ) const
	{
		return !operator==(Str);
	}
	Bool operator>( const String& Other ) const
	{
		return wcscmp( **this, *Other ) > 0;
	}
	Bool operator<( const String& Other ) const
	{
		return wcscmp( **this, *Other ) < 0;
	}
	Bool operator>=( const String& Other ) const
	{
		return wcscmp( **this, *Other ) >= 0;
	}
	Bool operator<=( const String& Other ) const
	{
		return wcscmp( **this, *Other ) <= 0;
	}
	String& operator+=( const Char* Str )
	{
		Integer StrLen = wcslen(Str);
		if( StrLen )
		{
			if( Self )
			{
				Integer L1 = Len();
				Integer L2 = StrLen;
				Integer LR = L1 + L2;
				SetLength( LR );
				MemCopy( &Self->Data[L1], Str, L2*sizeof(Char) );
				Self->Data[LR]	= '\0';
			}
			else
				*this = Str;
		}
		return *this;
	}
	String& operator+=( const String& Other )
	{
		if( Other.Len() )
		{
			if( Self )
			{
				Integer L1 = Len();
				Integer L2 = Other.Len();
				Integer LR = L1 + L2;
				SetLength( LR );
				MemCopy( &Self->Data[L1], &Other.Self->Data[0], L2*sizeof(Char) );
				Self->Data[LR]	= '\0';
			}
			else
				*this = Other;
		}
		return *this;
	}
	String operator+( const Char* Str ) 
	{
		return String(*this) += Str;   
	}
	String operator+( const String& Other )
	{
		return operator+(*Other);
	}
	operator Bool() const
	{
		return Self != nullptr;
	}

	// Statics.
	static String Format( String Fmt, ... );
	static String Copy( String Source, Integer StartChar, Integer Count );
	static Integer Pos( String Needle, String HayStack );
	static String UpperCase( String Str );
	static String LowerCase( String Str );
	static String Delete( String Str, Integer StartChar, Integer Count );
	static Integer CompareText( String Str1, String Str2 );
	static TArray<String> WrapText( String Text, Integer MaxColumnSize );

	// Friends.
	friend void Serialize( CSerializer& S, String& V );

private:
	// String internal used
	// struct.
	struct TSelf
	{
		Integer	Length;
		Integer	RefsCount;
		Char	Data[0];
	} *Self;

	// Allocate and initialize string data.
	void NewString( Integer InLen )
	{
		if( Self )
			DeleteString();
		Self				= (TSelf*)MemMalloc(sizeof(TSelf)+(InLen+1)*sizeof(Char));
		Self->Length		= InLen;
		Self->RefsCount		= 1;
		Self->Data[InLen]	= 0;
	}

	// String instance destruction.
	void DeleteString()
	{
		if( Self )
		{
			if( --Self->RefsCount == 0 )
				MemFree(Self);
			Self	= nullptr;
		}
	}

	// Change string length.
	void SetLength( Integer NewLen )
	{
		if( NewLen > 0 )
		{
			if( Self )
			{
				if( Self->RefsCount > 1 )
				{
					TSelf* New = (TSelf*)MemMalloc(sizeof(TSelf)+(NewLen+1)*sizeof(Char));
					New->Length	= NewLen;
					New->RefsCount	= 1;
					MemCopy( New->Data, Self->Data, Self->Length*sizeof(Char) );
					Self->RefsCount--;
					Self	= New;
				}
				else
				{
					Self = (TSelf*)MemRealloc( Self, sizeof(TSelf)+(NewLen+1)*sizeof(Char) );
					Self->Length	= NewLen;
				}
			}
			else
				NewString( NewLen );
		}
		else
			DeleteString();
	}
};


/*-----------------------------------------------------------------------------
    Inline string utility.
-----------------------------------------------------------------------------*/

//
// Character is a digit?
//
inline Bool IsDigit( Char Ch )
{
	return ( Ch >= L'0' ) && ( Ch <= L'9' );
}


//
// Character is a letter?
//
inline Bool IsLetter( Char Ch )
{
	return	( ( Ch >= L'A' )&&( Ch <= L'Z' ) )||
			( ( Ch >= L'a' )&&( Ch <= L'z' ) )||
			( Ch == L'_' );
}


//
// Convert hex character to the
// integer value.
//
inline Integer FromHex( Char Ch )
{
	if( Ch >= '0' && Ch <= '9' )
		return Ch-'0';
	else if( Ch >= 'a' && Ch <= 'f' )
		return Ch-'a'+10;
	else
		return 0;
}

/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/