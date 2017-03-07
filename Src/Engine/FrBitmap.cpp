/*=============================================================================
    FrBitmap.cpp: FBitmap class.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    TPalette implementation.
-----------------------------------------------------------------------------*/

//
// Palette default constructor.
//
TPalette::TPalette()
{}


//
// Palette copy constructor.
//
TPalette::TPalette( const TPalette& Other )
{
	assert( Other.Colors.Num() > 0 && 
		    Other.Colors.Num() <= 256 );

	Colors.SetNum( Other.Colors.Num() );
	for( Integer i=0; i<Other.Colors.Num(); i++ )
		Colors[i]	= Other.Colors[i];
}


//
// Palette allocator.
//
void TPalette::Allocate( DWord NumCols )
{
	assert( NumCols > 0 && NumCols <= 256 );
	Colors.SetNum( NumCols );
}


//
// Release the palette.
// 
void TPalette::Release()
{
	Colors.Empty();
}


//
// Find the most close color from palette.
// Don't compare alpha channel.
//
Byte TPalette::FindMatched( TColor InColor )
{
#define R_FACTOR	30
#define G_FACTOR	59
#define B_FACTOR	11

	Byte Best = 0;
	Integer BestCost = 0x7fffffff, Cost;

	for( Integer i=0; i < Colors.Num(); i++ )
	{
		Cost	= R_FACTOR * Sqr( (Integer)InColor.R - (Integer)Colors[i].R ) +
			      G_FACTOR * Sqr( (Integer)InColor.G - (Integer)Colors[i].G ) +
				  B_FACTOR * Sqr( (Integer)InColor.B - (Integer)Colors[i].B );

		if( Cost < BestCost )
		{
			Best		= i;
			BestCost	= Cost;
		}
	}

	return Best;

#undef R_FACTOR
#undef G_FACTOR
#undef B_FACTOR
}


//
// TPalette serialization.
//
void Serialize( CSerializer& S, TPalette& V )
{
	Serialize( S, V.Colors );
}


/*-----------------------------------------------------------------------------
    FBitmap implementation.
-----------------------------------------------------------------------------*/

//
// Bitmap constructor.
//
FBitmap::FBitmap()
	:	Format( BF_Palette8 ),
		Filter( BFILTER_Nearest ),
		BlendMode( BLEND_Regular )
{}


//
// Bitmap destructor.
//
FBitmap::~FBitmap()
{
}


//
// When user changed some field.
//
void FBitmap::EditChange()
{
	FBlockResource::EditChange();

	// Validate input data.
	PanUSpeed	= Clamp( PanUSpeed,		-10.f,	+10.f );
	PanVSpeed	= Clamp( PanVSpeed,		-10.f,	+10.f );
	Saturation	= Clamp( Saturation,	-10.f,	+10.f );
	AnimSpeed	= Clamp( AnimSpeed,		0.01f,	60.f );
}


//
// Erase temporal bitmap effects.
//
void FBitmap::Erase()
{
}


//
// Initialize bitmap, used to setup bitmap
// use it only in editor.
//
void FBitmap::Init( Integer InU, Integer InV )
{
	// Resolution should be power of two.
    assert(((InU)&(InU-1)) == 0);
    assert(((InV)&(InV-1)) == 0);

	// Editor only.
	assert(GIsEditor);

	// Setup fields.
	USize				= InU;
	VSize				= InV;
	UBits				= IntLog2(USize);
	VBits				= IntLog2(VSize);
	Format				= BF_Palette8;
	Filter				= BFILTER_Nearest;
	BlendMode			= BLEND_Regular;
	RenderInfo			= -1;
	PanUSpeed			= 0.f;
	PanVSpeed			= 0.f;
	Saturation			= 1.f;
	AnimSpeed			= 30.f;
	bDynamic			= false;
	bRedrawn			= false;
	LastRenderTime		= 0.0;

    //!! Data, Palette must be initialized in
    // importer or dynamic bitmaps subclasses.
}


//
// Handle mouse click in subclasses. For
// dynamic bitmaps.
//
void FBitmap::MouseClick( Integer Button, Integer X, Integer Y )
{
}


//
// Handle mouse move in subclasses. For
// dynamic bitmaps.
//
void FBitmap::MouseMove( Integer Button, Integer X, Integer Y )
{
}


//
// After loading code, place to restore
// some fields.
//
void FBitmap::PostLoad()
{
	FBlockResource::PostLoad();

	// Restore.
	USize			= 1 << UBits;
	VSize			= 1 << VBits;
	bDynamic		= false;
	bRedrawn		= false;
	LastRenderTime	= 0.0;

	// Force OpenGL/DirectX upload bitmap.
	RenderInfo	= -1;
}


//
// Redraw, used only for dynamic bitmaps.
//
void FBitmap::Redraw()
{
}


//
// Bitmap serialization.
//
void FBitmap::SerializeThis( CSerializer& S )
{
	FBlockResource::SerializeThis( S );

	SerializeEnum( S, Format );
	Serialize( S, UBits );
	Serialize( S, VBits );
	Serialize( S, Palette );
	SerializeEnum( S, Filter );
	SerializeEnum( S, BlendMode );
	Serialize( S, PanUSpeed );
	Serialize( S, PanVSpeed );
	Serialize( S, Saturation );
	Serialize( S, AnimSpeed );
}


//
// Tick texture effects.
//
void FBitmap::Tick()
{
	Double Time = GPlat->Now();
	Double Delta = Time - LastRenderTime;

	if( Delta > 1.0 )
	{
		// If not initialized.
		LastRenderTime = Time;
		return;
	}

	if( Delta > (60.01-AnimSpeed)/1000.0 )
	{
		Redraw();
		LastRenderTime = Time;
	}
}


//
// Import bitmap.
//
void FBitmap::Import( CImporterBase& Im )
{
	FBlockResource::Import( Im );

	IMPORT_BYTE( Format );
	IMPORT_BYTE( Filter );
	IMPORT_BYTE( BlendMode );
	IMPORT_BYTE( UBits );
	IMPORT_BYTE( VBits );

	IMPORT_FLOAT( PanUSpeed );
	IMPORT_FLOAT( PanVSpeed );
	IMPORT_FLOAT( Saturation );
	IMPORT_FLOAT( AnimSpeed );

	// Import the palette.
	if( Format == BF_Palette8 && this->IsA(FDemoBitmap::MetaClass) )
	{
		Palette.Allocate( Im.ImportInteger( L"NumColors" ) );

		for( Integer iColor=0; iColor<Palette.Colors.Num(); iColor++ )
			Palette.Colors[iColor]	= Im.ImportColor( *String::Format( L"Colors[%d]", iColor ) );
	}
}


//
// Export bitmap.
//
void FBitmap::Export( CExporterBase& Ex )
{
	FBlockResource::Export( Ex );

	EXPORT_BYTE( Format );
	EXPORT_BYTE( Filter );
	EXPORT_BYTE( BlendMode );
	EXPORT_BYTE( UBits );
	EXPORT_BYTE( VBits );

	EXPORT_FLOAT( PanUSpeed );
	EXPORT_FLOAT( PanVSpeed );
	EXPORT_FLOAT( Saturation );
	EXPORT_FLOAT( AnimSpeed );

	// Export the palette if has, and
	// it's a procedural bitmap, because simple
	// bitmaps will be imported.
	if( Format == BF_Palette8 && this->IsA(FDemoBitmap::MetaClass) )
	{
		Ex.ExportInteger( L"NumColors", Palette.Colors.Num() );
		for( Integer iColor=0; iColor<Palette.Colors.Num(); iColor++ )
			Ex.ExportColor( *String::Format( L"Colors[%d]", iColor ), Palette.Colors[iColor] );
	}
}


/*-----------------------------------------------------------------------------
    TStaticBitmap implementation.
-----------------------------------------------------------------------------*/

//
// GUI bitmap constructor.
//
TStaticBitmap::TStaticBitmap()
	:	FBitmap(),
		Data()
{
}


//
// GUI bitmap destructor.
// 
TStaticBitmap::~TStaticBitmap()
{
	Data.Empty();
}


//
// Return GUI bitmap data.
//
void* TStaticBitmap::GetData()
{
	return &Data[0];
}


//
// Return GUI bitmap size.
//
DWord TStaticBitmap::GetBlockSize()
{
	return Data.Num();
}


/*-----------------------------------------------------------------------------
    CDefaultBitmap implementation.
-----------------------------------------------------------------------------*/

//
//  The ÑDefaultBitmap it a temporal object, so do not register it and
//  avoid references to it, there only one instance and used as null bitmap for
//  rendering.
//
class CDefaultBitmap: public FBitmap
{
public:
	// Variables.
	TColor		ChessPattern[16][16];

	// CDefaultBitmap interface.
	CDefaultBitmap()
	{
		// Set fields.
		Format		= BF_RGBA;
		Filter		= BFILTER_Nearest;
		BlendMode	= BLEND_Regular;
		USize		= 16;
		VSize		= 16;
		UBits		= IntLog2( USize );
		VBits		= IntLog2( VSize );
		Saturation	= 1.f;
		RenderInfo	= -1;
		bDynamic	= false;
		Name		= L"HipHop";
		Id			= -1;

		// Plot cells.
		for( Integer v=0; v<16; v++ )
		for( Integer u=0; u<16; u++ )
		{
			ChessPattern[v][u].R	= (( u ^ v ) << 3) + 32;
			ChessPattern[v][u].G	= (( u ^ v ) << 3) + 32;
			ChessPattern[v][u].B	= (( u ^ v ) << 3) + 32;
			ChessPattern[v][u].A	= 0xff;
		}
	}
	~CDefaultBitmap()
	{}

	// FBitmap interface.
	void* GetData()
	{
		return ChessPattern;
	}
};


//
// Initialize instance.
//
static CDefaultBitmap	ChessBitmap;
FBitmap*				FBitmap::Default = &ChessBitmap;


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/