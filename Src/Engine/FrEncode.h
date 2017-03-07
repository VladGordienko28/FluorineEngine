/*=============================================================================
    FrEncode.h: Data compressors.
    Copyright Nov.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    CCompressor.
-----------------------------------------------------------------------------*/

//
// An abstract data compressor.
//
class CCompressor
{
public:
	// CCompressor interface.
	virtual void Encode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize ) = 0;
	virtual void Decode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize ) = 0;
	virtual DWord ForecastSize( const void* InBuffer, DWord InSize ) = 0;
};


/*-----------------------------------------------------------------------------
    CRLECompressor.
-----------------------------------------------------------------------------*/

//
// Run-Length-Encoding compressor.
//
class CRLECompressor
{
public:
	// CCompressor interface.
	CRLECompressor();
	void Encode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize );
	void Decode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize );
	DWord ForecastSize( const void* InBuffer, DWord InSize );
};


/*-----------------------------------------------------------------------------
    CLZWCompressor.
-----------------------------------------------------------------------------*/

//
// Lempel-Ziv-Welch compressor.
//
class CLZWCompressor
{
public:
	// CCompressor interface.
	CLZWCompressor();
	void Encode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize );
	void Decode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize );
	DWord ForecastSize( const void* InBuffer, DWord InSize );
};


/*-----------------------------------------------------------------------------
    CFutileCompressor.
-----------------------------------------------------------------------------*/

//
// A futile compressor for testing and research.
//
class CFutileCompressor
{
public:
	// CCompressor interface.
	CFutileCompressor();
	void Encode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize );
	void Decode( const void* InBuffer, DWord InSize, void*& OutBuffer, DWord& OutSize );
	DWord ForecastSize( const void* InBuffer, DWord InSize );
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/