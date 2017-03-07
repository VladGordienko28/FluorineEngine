/*=============================================================================
    FrColDlg.cpp: Color chooser dialog.
    Copyright Dec.2016 Vlad Gordienko.
=============================================================================*/

#include "GUI.h"

/*-----------------------------------------------------------------------------
    Color dialog bitmaps.
-----------------------------------------------------------------------------*/

//
// Bitmaps.
//
static	TStaticBitmap*		SLBitmap	= nullptr;
static	TStaticBitmap*		HBitmap		= nullptr;


//
// Initialize bitmaps.
//
void InitHSLBitmaps()
{
	// HBitmap.
	if( !HBitmap )
	{
		HBitmap					= new TStaticBitmap();
		HBitmap->Format			= BF_RGBA;
		HBitmap->USize			= 1;
		HBitmap->VSize			= 256;
		HBitmap->UBits			= IntLog2(HBitmap->USize);
		HBitmap->VBits			= IntLog2(HBitmap->VSize);
		HBitmap->Filter			= BFILTER_Nearest;
		HBitmap->BlendMode		= BLEND_Regular;
		HBitmap->RenderInfo		= -1;
		HBitmap->PanUSpeed		= 0.f;
		HBitmap->PanVSpeed		= 0.f;
		HBitmap->Saturation		= 1.f;
		HBitmap->AnimSpeed		= 0.f;
		HBitmap->bDynamic		= false;
		HBitmap->bRedrawn		= false;
		HBitmap->Data.SetNum(256*sizeof(TColor));

		TColor* Data = (TColor*)HBitmap->GetData();
		for( Integer i=0; i<256; i++ )
			Data[i]	= TColor::HSLToRGB( i, 0xff, 0x80 );
	}

	// SLBitmap.
	if( !SLBitmap )
	{
		SLBitmap				= new TStaticBitmap();
		SLBitmap->Format		= BF_RGBA;
		SLBitmap->USize			= 256;
		SLBitmap->VSize			= 256;
		SLBitmap->UBits			= IntLog2(SLBitmap->USize);
		SLBitmap->VBits			= IntLog2(SLBitmap->VSize);
		SLBitmap->Filter		= BFILTER_Nearest;
		SLBitmap->BlendMode		= BLEND_Regular;
		SLBitmap->RenderInfo	= -1;
		SLBitmap->PanUSpeed		= 0.f;
		SLBitmap->PanVSpeed		= 0.f;
		SLBitmap->Saturation	= 1.f;
		SLBitmap->AnimSpeed		= 0.f;
		SLBitmap->bDynamic		= true;
		SLBitmap->bRedrawn		= true;
		SLBitmap->Data.SetNum(256*256*sizeof(TColor));
	}
}


/*-----------------------------------------------------------------------------
    WColorChooser implementation.
-----------------------------------------------------------------------------*/

//
// Globals.
//
TColor		WColorChooser::SharedColor	= COLOR_White;


//
// Color chooser constructor.
//
WColorChooser::WColorChooser( WWindow* InRoot, TNotifyEvent InOk )
	:	WForm( InRoot, InRoot )
{
	// Dialog fields.
	bSizeableW			= false;
	bSizeableH			= false;
	bCanClose			= true;
	Caption				= L"Color Chooser";
	SetSize( 408, 328 );	
	SetLocation
	( 
		(Root->Size.Width-Size.Width)/2, 
		(Root->Size.Height-Size.Height)/2 
	);

	// Allocate controls.
	OkButton					= new WButton( this, Root );
	OkButton->Caption			= L"Ok";
	OkButton->EventClick		= WIDGET_EVENT(WColorChooser::ButtonOkClick);
	OkButton->SetSize( 75, 25 );
	OkButton->SetLocation( 10, 293 );

	CancelButton				= new WButton( this, Root );
	CancelButton->Caption		= L"Cancel";
	CancelButton->EventClick	= WIDGET_EVENT(WColorChooser::ButtonCancelClick);
	CancelButton->SetSize( 75, 25 );
	CancelButton->SetLocation( 90, 293 );

	REdit				= new WEdit( this, Root );
	REdit->EditType		= EDIT_Integer;
	REdit->EventChange	= WIDGET_EVENT(WColorChooser::UpdateFromRGB);
	REdit->SetSize( 36, 18 );
	REdit->SetLocation( 312, 102 );

	GEdit				= new WEdit( this, Root );
	GEdit->EditType		= EDIT_Integer;
	GEdit->EventChange	= WIDGET_EVENT(WColorChooser::UpdateFromRGB);
	GEdit->SetSize( 36, 18 );
	GEdit->SetLocation( 312, 122 );

	BEdit				= new WEdit( this, Root );
	BEdit->EditType		= EDIT_Integer;
	BEdit->EventChange	= WIDGET_EVENT(WColorChooser::UpdateFromRGB);
	BEdit->SetSize( 36, 18 );
	BEdit->SetLocation( 312, 142 );

	HEdit				= new WEdit( this, Root );
	HEdit->EditType		= EDIT_Integer;
	HEdit->EventChange	= WIDGET_EVENT(WColorChooser::UpdateFromHSL);
	HEdit->SetSize( 36, 18 );
	HEdit->SetLocation( 362, 102 );

	SEdit				= new WEdit( this, Root );
	SEdit->EditType		= EDIT_Integer;
	SEdit->EventChange	= WIDGET_EVENT(WColorChooser::UpdateFromHSL);
	SEdit->SetSize( 36, 18 );
	SEdit->SetLocation( 362, 122 );

	LEdit				= new WEdit( this, Root );
	LEdit->EditType		= EDIT_Integer;
	LEdit->EventChange	= WIDGET_EVENT(WColorChooser::UpdateFromHSL);
	LEdit->SetSize( 36, 18 );
	LEdit->SetLocation( 362, 142 );

	// Init bitmaps, if they wasn't.
	InitHSLBitmaps();

	// Setup.
	EventOk		= InOk;
	Selected	= SharedColor;
	bMoveSL		= false;
	bMoveH		= false;
	REdit->Text	= String::Format( L"%i", Selected.R );
	GEdit->Text	= String::Format( L"%i", Selected.G );
	BEdit->Text	= String::Format( L"%i", Selected.B );
	UpdateFromRGB( this );

	// Store old modal, and current modal.
	OldModal		= Root->Modal;
	Root->Modal		= this;
}


//
// Color chooser destructor.
//
WColorChooser::~WColorChooser()
{
	// Restore old modal.
	Root->Modal	= OldModal;
}


//
// When dialog closed, its hides.
//
void WColorChooser::OnClose()
{
	WForm::OnClose();
	Hide();
}


//
// When user click 'Ok' button.
//
void WColorChooser::ButtonOkClick( WWidget* Sender )
{
	// Store color.
	SharedColor	= Selected;

	// Notify.
	EventOk( Sender );
	OnClose();
}


//
// When user click 'Cancel' button.
//
void WColorChooser::ButtonCancelClick( WWidget* Sender )
{
	OnClose();
}


//
// When dialog hides, its suicide.
//
void WColorChooser::Hide()
{
	WForm::Hide();
	delete this;
}


//
// Redraw color picker.
//
void WColorChooser::OnPaint( CGUIRenderBase* Render )
{ 
	// Call parent.
	WForm::OnPaint(Render); 
	TPoint Base	= ClientToWindow(TPoint::Zero);

	// Decompose selected color.
	Integer	H, S, L;
	HEdit->Text.ToInteger( H, 0 );
	SEdit->Text.ToInteger( S, 0 );
	LEdit->Text.ToInteger( L, 0 );

	// Draw SL panel.
	Render->DrawPicture
	(
		TPoint( Base.X+10, Base.Y+30 ),
		TSize( 256, 256 ),
		TPoint( 0, 0 ),
		TSize( 256, 256 ),
		SLBitmap
	);
	Render->DrawPicture
	(
		TPoint( Base.X+10+S-5, Base.Y+30+L-5 ),
		TSize( 11, 11 ),
		TPoint( 39, 9 ),
		TSize( 11, 11 ),
		WWindow::Icons		
	);

	// Draw H panel.
	Render->DrawPicture
	(
		TPoint( Base.X+271, Base.Y+30 ),
		TSize( 15, 256 ),
		TPoint( 0, 0 ),
		TSize( 1, 256 ),
		HBitmap
	);
	Render->DrawPicture
	(
		TPoint( Base.X+286, Base.Y+30+H-5 ),
		TSize( 8, 9 ),
		TPoint( 39, 0 ),
		TSize( 8, 9 ),
		WWindow::Icons		
	);

	// Draw labels next to edits.
	Render->DrawText( TPoint(Base.X+302, Base.Y+104), L"R", 1, GUI_COLOR_TEXT, WWindow::Font1 );
	Render->DrawText( TPoint(Base.X+302, Base.Y+124), L"G", 1, GUI_COLOR_TEXT, WWindow::Font1 );
	Render->DrawText( TPoint(Base.X+302, Base.Y+144), L"B", 1, GUI_COLOR_TEXT, WWindow::Font1 );
	Render->DrawText( TPoint(Base.X+352, Base.Y+104), L"H", 1, GUI_COLOR_TEXT, WWindow::Font1 );
	Render->DrawText( TPoint(Base.X+352, Base.Y+124), L"S", 1, GUI_COLOR_TEXT, WWindow::Font1 );
	Render->DrawText( TPoint(Base.X+352, Base.Y+144), L"L", 1, GUI_COLOR_TEXT, WWindow::Font1 );

	// Draw new color.
	Render->DrawRegion
	(
		TPoint( Base.X+298, Base.Y+45 ),
		TSize( 50, 50 ),
		Selected,
		Selected,
		BPAT_Solid
	);
	Render->DrawText
	( 
		TPoint( Base.X+298, Base.Y+30 ), 
		L"New" , 3, 
		GUI_COLOR_TEXT, 
		WWindow::Font1 
	);
	
	// Draw old color.
	Render->DrawRegion
	(
		TPoint( Base.X+348, Base.Y+45 ),
		TSize( 50, 50 ),
		SharedColor,
		SharedColor,
		BPAT_Solid
	);
	Render->DrawText
	( 
		TPoint( Base.X+348, Base.Y+30 ), 
		L"Old" , 3, 
		GUI_COLOR_TEXT, 
		WWindow::Font1 
	);
}


//
// Process mouse movement over dialog.
//
void WColorChooser::OnMouseMove( EMouseButton Button, Integer X, Integer Y )
{ 
	WForm::OnMouseMove( Button, X, Y ); 

	if( bMoveH )
	{
		// Process Hue movement.
		Integer	Hue	= Clamp( Y-30, 0x00, 0xff );

		HEdit->Text	= String::Format( L"%i", Hue );
		UpdateFromHSL( this );
	}
	if( bMoveSL )
	{
		// Process Saturation/Lightness movement.
		Integer	Saturation	= Clamp( X-10, 0x00, 0xff ),
				Lightness	= Clamp( Y-30, 0x00, 0xff );

		SEdit->Text	= String::Format( L"%i", Saturation );
		LEdit->Text	= String::Format( L"%i", Lightness );
		UpdateFromHSL( this );
	}
}


//
// User just hold mouse button.
//
void WColorChooser::OnMouseDown( EMouseButton Button, Integer X, Integer Y )
{ 
	WForm::OnMouseDown( Button, X, Y ); 

	if( Button != MB_Left )
		return;

	// Test for SL area.
	if( X>=10 && Y>=30 && X<=266 && Y<=286 )
	{
		bMoveSL	= true;
		OnMouseMove( Button, X, Y );
	}

	// Test for H area.
	if( X>=271 && Y>=30 && X<=294 && Y<=286 )
	{
		bMoveH	= true;
		OnMouseMove( Button, X, Y );
	}
}	


//
// User just release mouse button.
//
void WColorChooser::OnMouseUp( EMouseButton Button, Integer X, Integer Y )
{ 
	WForm::OnMouseUp( Button, X, Y ); 

	bMoveH	= false;
	bMoveSL	= false;
}


//
// Update color, when changed via RGB edits.
//
void WColorChooser::UpdateFromRGB( WWidget* Sender )
{
	Integer	R, G, B;
	REdit->Text.ToInteger( R, Selected.R );
	GEdit->Text.ToInteger( G, Selected.G );
	BEdit->Text.ToInteger( B, Selected.B );

	Selected	= TColor
	(
		Clamp( R, 0x00, 0xff ),
		Clamp( G, 0x00, 0xff ),
		Clamp( B, 0x00, 0xff ),
		Selected.A
	);

	Byte H, S, L;
	TColor::RGBToHSL( Selected, H, S, L );
	HEdit->Text	= String::Format( L"%i", H );
	SEdit->Text	= String::Format( L"%i", S );
	LEdit->Text	= String::Format( L"%i", L );

	RefreshSL();
}


//
// Update color, when changed via HSL edits.
//
void WColorChooser::UpdateFromHSL( WWidget* Sender )
{
	Byte Hs, Ss, Ls;
	Integer H, S, L;

	TColor::RGBToHSL( Selected, Hs, Ss, Ls );
	HEdit->Text.ToInteger( H, Hs );
	SEdit->Text.ToInteger( S, Ss );
	LEdit->Text.ToInteger( L, Ls );

	Byte Alpha = Selected.A;
	Selected	= TColor::HSLToRGB
	(
		Clamp( H, 0x00, 0xff ),
		Clamp( S, 0x00, 0xff ),
		Clamp( L, 0x00, 0xff )
	);
	Selected.A	= Alpha;

	REdit->Text	= String::Format( L"%i", Selected.R );
	GEdit->Text	= String::Format( L"%i", Selected.G );
	BEdit->Text	= String::Format( L"%i", Selected.B );

	RefreshSL();
}


//
// Redraw saturation/lightness bitmap.
//
void WColorChooser::RefreshSL()
{
	TColor*	Data	= (TColor*)SLBitmap->GetData();

	// Decompose selected color.
	Integer	Hue;
	HEdit->Text.ToInteger( Hue, 0 );

	for( Integer L=0; L<256; L++ )
	{
		TColor	Col1	= TColor( L, L, L, 0xff );
		TColor	Col2	= TColor::HSLToRGB( Hue, 255, L );

		TColor*	Line	= &Data[L*256];

		// Use some fixed math 16:16 magic :3
		Integer	RWalk	= Col1.R * 65536,
				GWalk	= Col1.G * 65536,
				BWalk	= Col1.B * 65536,
				RStep	= (Col2.R-Col1.R) * 65536 / 256,
				GStep	= (Col2.G-Col1.G) * 65536 / 256,
				BStep	= (Col2.B-Col1.B) * 65536 / 256;

		for( Integer S=0; S<256; S++ )
		{
			Line[S]	= TColor( RWalk>>16, GWalk>>16, BWalk>>16, 0xff );

			RWalk	+= RStep;
			GWalk	+= GStep;
			BWalk	+= BStep;
		}
	}

	// Force to reload.
	SLBitmap->bRedrawn	= true;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/