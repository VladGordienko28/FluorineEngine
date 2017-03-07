/*=============================================================================
    FrCore.cpp: Various engine core functions.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

//
// Globals.
//
CDebugOutputBase*	GOutput		= nullptr;
Bool				GIsEditor	= false;
CPlatformBase*		GPlat		= nullptr;
String				GDirectory	= L"";
DWord				GFrameStamp = 0;
String				GCmdLine[8]	= {};


/*-----------------------------------------------------------------------------
    String implementation.
-----------------------------------------------------------------------------*/

//
// String serialization.
//
void Serialize( CSerializer& S, String& V )
{
	if( S.GetMode() == SM_Load )
	{
		// Load string.
		Integer L;
		V.DeleteString();
		Serialize( S, L );
		if( L > 0 )
		{
			V.NewString( L );
			S.SerializeData( *V, sizeof(Char) * L );
		}
	}
	else
	{
		// Store or count string.
		Integer Len = V.Len();
		Serialize( S, Len );

		if( Len > 0 )
			S.SerializeData( *V, sizeof(Char)*Len );
	}
}


//
// Convert string into integer value, return true if 
// converted successfully, otherwise return false and out value
// will be set default.
//
Bool String::ToInteger( Integer& Value, Integer Default ) const
{
	if (!Len())
		return false;

	Integer		iChar	= 0;
	Bool		bNeg	= false;
	Value				= Default;

	// Detect sign.
	if( Self->Data[0] == L'-' )
	{
		iChar++;
		bNeg = true;
	}
	else if( Self->Data[0] == L'+' )
	{
		iChar++;
		bNeg = false;
	}

	// Parse digit by digit.
	Integer Result = 0;
	for (Integer i = iChar; i < Len(); i++)
	if (Self->Data[i] >= L'0' && Self->Data[i] <= L'9')
	{
		Result *= 10;
		Result += (Integer)(Self->Data[i] - L'0');
	}
	else
		return false;

	Value = bNeg ? -Result : Result;
	return true;
}


//
// Convert string into float value, return true if 
// converted successfully, otherwise return false and out value
// will be set default.
//
Bool String::ToFloat( Float& Value, Float Default ) const
{
	if (!Len())
		return false;

	Integer		iChar	= 0;
	Bool		bNeg	= false;
	Float		Frac = 0.f, Ceil = 0.f;
	Value				= Default;

	// Detect sign.
	if (Self->Data[0] == L'-')
	{
		iChar++;
		bNeg = true;
	}
	else if (Self->Data[0] == L'+')
	{
		iChar++;
		bNeg = false;
	}

	if( iChar < Len() )
	{
		if( Self->Data[iChar] == L'.' )
		{
			// Parse fractional part.
		ParseFrac:
			iChar++;
			Float m = 0.1f;
			for( ; iChar < Len(); iChar++ )
				if( Self->Data[iChar] >= L'0' && Self->Data[iChar] <= L'9' )
				{
					Frac += (Integer)(Self->Data[iChar] - L'0') * m;
					m /= 10.f;
				}
				else
					return false;
		}
		else if( Self->Data[iChar] >= L'0' && Self->Data[iChar] <= L'9' )
		{
			// Parse ceil part.
			for( ; iChar < Len(); iChar++ )
				if( Self->Data[iChar] >= L'0' && Self->Data[iChar] <= L'9' )
				{
					Ceil *= 10.f;
					Ceil += (Integer)(Self->Data[iChar] - L'0');
				}
				else if( Self->Data[iChar] == L'.' )
				{
					goto ParseFrac;
				}
				else
					return false;
		}
		else
			return false;
	}
	else
		return false;

	Value = bNeg ? -(Ceil+Frac) : +(Ceil+Frac);
	return true;
}


//
// Remove Count symbols from StartChar position. 
//
String String::Delete( String Str, Integer StartChar, Integer Count )
{
	StartChar	= Clamp( StartChar, 0, Str.Len()-1 );
	Count		= Clamp( Count, 0, Str.Len()-StartChar );
	return Copy( Str, 0, StartChar ) + Copy( Str, StartChar+Count, Str.Len()-StartChar-Count );	
}


//
// Copy substring.
//
String String::Copy( String Source, Integer StartChar, Integer Count )
{	
	StartChar = Clamp( StartChar, 0, Source.Len()-1 );
	Count = Clamp( Count, 0, Source.Len()-StartChar );
	return Count ? String( &Source.Self->Data[StartChar], Count ) : String();
}


//
// Search needle in haystack :), of string
// of course. if not found return -1.
//
Integer String::Pos( String Needle, String HayStack )
{
	const Char* P = wcsstr( *HayStack, *Needle );
	return P ? ((Integer)P - (Integer)*HayStack)/sizeof(Char) : -1;
}


//
// Return string copy with upper case.
//
String String::UpperCase( String Str )
{
	String N;
	N.NewString(Str.Len());
	for( Integer i=0; i<Str.Len(); i++ )
	{
		N.Self->Data[i] = towupper(Str(i));
	}
	return N;
}


//
// Return string copy with lower case.
//
String String::LowerCase( String Str )
{
	String N;
	N.NewString(Str.Len());
	for( Integer i=0; i<Str.Len(); i++ )
	{
		N.Self->Data[i] = towlower(Str(i));
	}
	return N;
}


//
// Format string.
//
String String::Format( String Fmt, ... )
{
	static Char Dest[2048] = {};
	va_list ArgPtr;
	va_start( ArgPtr, Fmt );
	_vsnwprintf( Dest, 2048, *Fmt, ArgPtr );
	va_end( ArgPtr );
	return Dest;
}


//
// String comparison.
// Return:
//   < 0: Str1 < Str2.
//   = 0: Str1 = Str2.
//	 > 0: Str1 > Str2.
//
Integer String::CompareText( String Str1, String Str2 )
{
#if 0
	return wcscmp( *Str1, *Str2 );
#else
	return _wcsicmp( *Str1, *Str2 );
#endif
}


//
// Wrap text and put lines into array of string.
//
TArray<String> String::WrapText( String Text, Integer MaxColumnSize )
{
	TArray<String> Result;

	Integer i=0;
	do
	{
		Integer iCleanWordEnd	= 0;
		Bool	bGotWord		= false;
		Integer	iTestWord		= 0;

		while	( 
					( i+iTestWord < Text.Len() )&&
					(Text(i+iTestWord) != '\n') 
				)
		{
			if( iTestWord++ > MaxColumnSize )
				break;

			Bool bWordBreak = (Text(iTestWord+i)==' ')||(Text(iTestWord+i)=='\n')||(iTestWord+i>=Text.Len());
			if( bWordBreak || !bGotWord )
			{
				iCleanWordEnd	= iTestWord;
				bGotWord		= bGotWord || bWordBreak;
			}
		}

		if( iCleanWordEnd == 0 )
			break;

		// Add to array.
		Result.Push(String::Copy( Text, i, iCleanWordEnd ));
		i += iCleanWordEnd;

		// Skip whitespace after word.
		while( Text(i) == ' ' )
			i++;

	} while( i<Text.Len() );

	return Result;
}


/*-----------------------------------------------------------------------------
    TColor implementation.
-----------------------------------------------------------------------------*/

//
// Convert RGB color value to HSL value
// where H(Hue), S(Saturation), L(Lightness).
//
void TColor::RGBToHSL( TColor Color, Byte& H, Byte& S, Byte& L )
{
	Float R	= Color.R / 256.f;
	Float G	= Color.G / 256.f;
	Float B = Color.B / 256.f;
	
	Float MinValue	= Min( R, Min( G, B ) );
	Float MaxValue	= Max( R, Max( G, B ) );

	if( R == G && G == B )
	{
		H	= 0;
		S	= 0;
		L	= Color.G;
	}
	else
	{
		Float FH, FS, FL;

		FL	= ( MinValue + MaxValue ) * 0.5f;

		if( FL < 0.5f )
			FS	= ( MaxValue - MinValue ) / ( MaxValue + MinValue );
		else
			FS	= ( MaxValue - MinValue ) / ( 2.f - MaxValue - MinValue );

		if( R == MaxValue )
			FH	= ( G - B ) / ( MaxValue - MinValue );
		else if( G == MaxValue )
			FH	= 2.f + ( B - R ) / ( MaxValue - MinValue );
		else if( B == MaxValue )
			FH	= 4.f + ( R - G ) / ( MaxValue - MinValue );

		FH /= 6.f;
		if( FH < 0.f ) FH += 1.f;

		H	= Trunc( FH * 254.9f );
		S	= Trunc( FS * 254.9f );
		L	= Trunc( FL * 254.9f );
	}
}


//
// Convert HSL color value to RGB value
// where H(Hue), S(Saturation), L(Lightness).
//
TColor TColor::HSLToRGB( Byte H, Byte S, Byte L )
{
	Float	FH	= H / 256.f;
	Float	FS	= S / 256.f;
	Float	FL	= L / 256.f;

	if( S == 0 )
	{
		return TColor( L, L, L, 0xff );
	}
	else
	{
		Float	FR, FG, FB, temp1, temp2, tempR, tempG, tempB;

		if( FL < 0.5f )
			temp2	= FL * ( 1.f + FS );
		else
			temp2	= ( FL + FS ) - ( FL * FS );

		temp1	= 2.f * FL - temp2;
		tempR	= FH + 1.f / 3.f;

		if( tempR > 1.f ) tempR -= 1.f;
		tempG	= FH;
		tempB	= FH - 1.f / 3.f;
		if( tempB < 0.f ) tempB += 1.f;

		// Red channel.
		if( tempR < 1.f/6.f )
			FR	= temp1 + (temp2-temp1)*6.f * tempR;
		else if( tempR < 0.5f )
			FR	= temp2;
		else if( tempR < 2.f/3.f )
			FR	= temp1 + (temp2-temp1)*((2.f/3.f)-tempR)*6.f;
		else
			FR	= temp1;

		// Green channel.
		if( tempG < 1.f/6.f )
			FG	= temp1 + (temp2-temp1)*6.f * tempG;
		else if( tempG < 0.5f )
			FG	= temp2;
		else if( tempG < 2.f/3.f )
			FG	= temp1 + (temp2-temp1)*((2.f/3.f)-tempG)*6.f;
		else
			FG	= temp1;

		// Blue channel.
		if( tempB < 1.f/6.f )
			FB	= temp1 + (temp2-temp1)*6.f * tempB;
		else if( tempB < 0.5f )
			FB	= temp2;
		else if( tempB < 2.f/3.f )
			FB	= temp1 + (temp2-temp1)*((2.f/3.f)-tempB)*6.0;
		else
			FB	= temp1;

		return TColor( Trunc( FR * 254.9f ),
					   Trunc( FG * 254.9f ),
					   Trunc( FB * 254.9f ),
					   0xff );
	}
}


/*-----------------------------------------------------------------------------
    TArray implementation.
-----------------------------------------------------------------------------*/

//
// Figure out how much extra items should alloc for array,
// this depend on array' inner size. Result is always power
// of two.
//
Integer ExtraSpace( DWord InnerSize )
{
	if( InnerSize <= 1 )
		return 128;
	else if( InnerSize <= 4 )
		return 64;
	else if( InnerSize <= 8 )
		return 32;
	else 
		return 16;
}


//
// Main reallocation function, now it's allocate more items,
// to make faster next allocation.
//
void ReallocateArray( void*& Data, Integer& Count, Integer NewCount, DWord InnerSize )
{
	// Don't reallocate.
	if( Count == NewCount )
		return;

	if( NewCount == 0 )
	{
		// Get rid data.
		MemFree( Data );
		Data	= nullptr;
		Count	= 0;
	} 
	else if( Data == nullptr )
	{
		// Allocate new data.
		Integer OverItems	= ExtraSpace( InnerSize ) - 1;
		Integer TrueNew		= NewCount | OverItems;
		Data				= MemAlloc( TrueNew * InnerSize );
		Count				= NewCount;
	}
	else
	{
		// Reallocate array.
		if( NewCount > Count )
		{
			// Add new item.
			Integer OverItems	= ExtraSpace( InnerSize ) - 1;
			Integer TrueOld		= Count | OverItems;
			if( NewCount >= TrueOld )
			{
				// Need extra items.
				Integer TrueNew	= NewCount | OverItems;
				Data	= MemRealloc( Data, TrueNew * InnerSize );
				MemZero
				( 
					(Byte*)Data + Count * InnerSize,
					(NewCount - Count) * InnerSize
				);
			}
			else
			{
				// Memory enough, but need zero.
				MemZero
				( 
					(Byte*)Data + Count * InnerSize,
					(NewCount - Count) * InnerSize
				);
			}

			Count	= NewCount;
		}
		else
		{
			// Remove some items.
			Integer OverItems	= ExtraSpace( InnerSize ) - 1;
			Integer TrueNew		= OverItems | NewCount;
			Integer TrueOld		= OverItems | Count;

			if( TrueOld != TrueNew )
				Data = MemRealloc( Data, TrueNew * InnerSize );

			Count	= NewCount;
		}
	}
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/