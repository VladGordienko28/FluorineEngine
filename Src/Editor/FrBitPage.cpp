/*=============================================================================
    FrBitPage.cpp: Bitmap page.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "Editor.h"

/*-----------------------------------------------------------------------------
    WBitmapPage implementation.
-----------------------------------------------------------------------------*/

//
// Bitmap page constructor.
//
WBitmapPage::WBitmapPage( FBitmap* InBitmap, WContainer* InOwner, WWindow* InRoot )
	:	WEditorPage( InOwner, InRoot ),
		Scale( 1.f ),
		Pan( 0, 0 ),
		DragMode( DRAG_None ),
		DragFrom( 0, 0 ),
		bMouseMove( false ),
		EditForm( nullptr )

{
	// Initialize own fields.
	Padding		= TArea( 0, 0, 0, 0 );
	Bitmap		= InBitmap;
	Caption		= InBitmap->GetName();
	TabWidth	= Root->Font1->TextWidth( *Caption ) + 30;
	PageType	= PAGE_Bitmap;
	Color		= PAGE_COLOR_BITMAP;

	// Create toolbar and buttons.
	ToolBar = new WToolBar( this, Root );
	ToolBar->SetSize( 3000, 28 );

	EditButton				= new WButton( ToolBar, Root );
	EditButton->Caption		= L"Edit";
	EditButton->Tooltip		= L"Open Effects Editor";
	EditButton->bEnabled	= false;
	EditButton->EventClick	= WIDGET_EVENT(WBitmapPage::ButtonEditClick);
	EditButton->SetSize( 70, 22 );
	ToolBar->AddElement( EditButton );
	ToolBar->AddElement( nullptr );

	EraseButton				= new WButton( ToolBar, Root );
	EraseButton->Caption	= L"Erase";
	EraseButton->Tooltip	= L"Erase Effects";
	EraseButton->bEnabled	= false;
	EraseButton->EventClick = WIDGET_EVENT(WBitmapPage::ButtonEraseClick);
	EraseButton->SetSize( 70, 22 );
	ToolBar->AddElement( EraseButton );
	ToolBar->AddElement( nullptr );

	ZoomInButton				= new WButton( ToolBar, Root );
	ZoomInButton->Caption		= L"+";
	ZoomInButton->Tooltip		= L"Zoom In";
	ZoomInButton->EventClick	= WIDGET_EVENT(WBitmapPage::ButtonZoomInClick);	
	ZoomInButton->SetSize( 25, 22 );
	ToolBar->AddElement( ZoomInButton );

	ZoomOutButton				= new WButton( ToolBar, Root );
	ZoomOutButton->Caption		= L"-";
	ZoomOutButton->Tooltip		= L"Zoom Out";
	ZoomOutButton->EventClick	= WIDGET_EVENT(WBitmapPage::ButtonZoomOutClick);
	ZoomOutButton->SetSize( 25, 22 );
	ToolBar->AddElement( ZoomOutButton );

	// Create the parameters form.
	if( Bitmap->IsA(FFireBitmap::MetaClass) || 
		Bitmap->IsA(FWaterBitmap::MetaClass) )
	{
		// Create form.
		EditForm	= new WBitmapEditForm( Bitmap, this, Root );
		EditForm->Show( 20, 60 );

		// Turn on buttons.
		EditButton->bEnabled	= true;
		EraseButton->bEnabled	= true;
	}
}


//
// Tick the page.
//
void WBitmapPage::TickPage( Float Delta )
{
}


//
// Bitmap page destructor.
//
WBitmapPage::~WBitmapPage()
{ 
	// Remove the parameters editor.
	freeandnil(EditForm);
}


//
// Ask user for page closing.
//
Bool WBitmapPage::OnQueryClose()
{
	// Ok, let user close the page without
	// any asking.
	return true;
}


//
// User has down on bitmap page.
//
void WBitmapPage::OnMouseDown( EMouseButton Button, Integer X, Integer Y )
{
	WEditorPage::OnMouseDown( Button, X, Y );

	// Figure out drag mode.
	if( Bitmap->IsA(FDemoBitmap::MetaClass) )
	{
		// Drawing bitmap.
		TPoint BitPos	= TPoint
							(	
								(Pan.X + X - (Size.Width-Bitmap->USize*Scale)/2)/Scale, 
								(Pan.Y + Y - (Size.Height-Bitmap->VSize*Scale)/2)/Scale  
							);

		if	(	
				BitPos.X >= 0 && 
				BitPos.X < Bitmap->USize && 
				BitPos.Y >= 0 && 
				BitPos.Y < Bitmap->VSize 
			)
		{
			// Cursor inside bitmap.
			DragMode	= DRAG_Drawing;
		}
		else
		{
			// Cursor outside bitmap.
			if( Button == MB_Left )
			{
				DragMode	= DRAG_Panning;
				DragFrom	= TPoint( X, Y );
			}
		}
	}
	else
	{
		// Simple bitmap, always panning it.
		if( Button == MB_Left )
		{
			DragMode	= DRAG_Panning;
			DragFrom	= TPoint( X, Y );
		}
	}

	// Start track for click.
	bMouseMove	= false;
}


//
// User has release mouse button.
//
void WBitmapPage::OnMouseUp( EMouseButton Button, Integer X, Integer Y )
{
	WEditorPage::OnMouseUp( Button, X, Y );

	// Notify about click?
	if( !bMouseMove && Bitmap->IsA(FDemoBitmap::MetaClass) && DragMode == DRAG_Drawing )
	{
		// Figure out mouse location onto bitmap.
		TPoint BitPos	= TPoint
							(	
								(Pan.X + X - (Size.Width-Bitmap->USize*Scale)/2)/Scale, 
								(Pan.Y + Y - (Size.Height-Bitmap->VSize*Scale)/2)/Scale  
							);

		// Test bounds and notify.
		if	(	BitPos.X >= 0 && 
				BitPos.X < Bitmap->USize && 
				BitPos.Y >= 0 && 
				BitPos.Y < Bitmap->VSize 
			)
				Bitmap->MouseClick( Button, BitPos.X, BitPos.Y );
	}

	// Cleanup.
	DragMode	= DRAG_None;
	bMouseMove	= false;
}


//
// Mouse move bitmap.
//
void WBitmapPage::OnMouseMove( EMouseButton Button, Integer X, Integer Y )
{
	WEditorPage::OnMouseMove( Button, X, Y );

	// Mark it.
	bMouseMove	= true;

	// Are we inside bitmap?
	TPoint BitPos	= TPoint
						(	
							(Pan.X + X - (Size.Width-Bitmap->USize*Scale)/2)/Scale, 
							(Pan.Y + Y - (Size.Height-Bitmap->VSize*Scale)/2)/Scale  
						);

	if( DragMode == DRAG_Panning )
	{
		// Panning the bitmap.
		Pan.X		-= X - DragFrom.X;
		Pan.Y		-= Y - DragFrom.Y;
		DragFrom	= TPoint( X, Y );
	}

	if	(	
			BitPos.X >= 0 && 
			BitPos.X < Bitmap->USize && 
			BitPos.Y >= 0 && 
			BitPos.Y < Bitmap->VSize 
		)
	{
		// We are inside bitmap, but we can draw?
		if( DragMode == DRAG_Drawing || Button == MB_None )
			Bitmap->MouseMove( Button, BitPos.X, BitPos.Y );

		// Update status bar.
		GEditor->StatusBar->Panels[0].Text	= String::Format( L"U: %d", BitPos.X );
		GEditor->StatusBar->Panels[1].Text	= String::Format( L"V: %d", BitPos.Y );
	}
	else
	{
		// Update status bar.
		GEditor->StatusBar->Panels[0].Text	= L"U: n/a";
		GEditor->StatusBar->Panels[1].Text	= L"V: n/a";
	}
}


//
// Open an effect editor form.
//
void WBitmapPage::ButtonEditClick( WWidget* Sender )
{
	if( EditForm && !EditForm->bVisible )
		EditForm->Show( 20, 60 );
}


//
// When page has been opened/reopened.
//
void WBitmapPage::OnOpen()
{
	// Let inspector show bitmap's properties.
	if( Bitmap )
		GEditor->Inspector->SetEditObject( Bitmap );
}


//
// Erase temporal bitmap effects.
//
void WBitmapPage::ButtonEraseClick( WWidget* Sender )
{
	Bitmap->Erase();
}


//
// Zoom in bitmap.
//
void WBitmapPage::ButtonZoomInClick( WWidget* Sender )
{
	Scale = Min( 4.f, Scale * 2.f );
}


//
// Zoom out bitmap.
//
void WBitmapPage::ButtonZoomOutClick( WWidget* Sender )
{
	Scale = Max( 0.125f, Scale * 0.5f );
}


//
// Draw the bitmap page.
//
void WBitmapPage::OnPaint( CGUIRenderBase* Render )
{
	TPoint Base = ClientToWindow(TPoint::Zero);

	// Clip to page.
	Render->SetClipArea( Base, Size );

	// Draw cool backdrop, with pattern.
	Render->DrawRegion
				( 
					Base, 
					TSize( Size.Width, Size.Height ), 
					TColor( 0x30, 0x30, 0x30, 0xff ), 
					TColor( 0x30, 0x30, 0x30, 0xff ), 
					BPAT_Solid 
				);

	Render->DrawRegion
				( 
					Base, 
					TSize( Size.Width, Size.Height ), 
					TColor( 0x3f, 0x3f, 0x3f, 0xff ), 
					TColor( 0x3f, 0x3f, 0x3f, 0xff ), 
					BPAT_PolkaDot 
				);

	// Draw bitmap.
	{
		Integer X = Base.X - Pan.X + ( Size.Width - Bitmap->USize * Scale ) / 2;
		Integer Y = Base.Y - Pan.Y + ( Size.Height - Bitmap->VSize * Scale ) / 2;
		Integer W = Bitmap->USize * Scale;
		Integer H = Bitmap->VSize * Scale;

		// Draw border.
		Render->DrawRegion
					( 
						TPoint( X, Y-1 ), 
						TSize( W+1, H+1 ), 
						COLOR_Black, 
						COLOR_Black, 
						BPAT_None 
					);

		// Draw image.
		Render->DrawPicture
						( 
							TPoint( X, Y ), 
							TSize( W, H ), 
							TPoint( 0, 0 ), 
							TSize( Bitmap->USize, Bitmap->VSize ), 
							Bitmap  
						);
	}

	// Draw bitmap's zoom.
	Render->DrawText
				( 
					TPoint( Base.X + 10, Base.Y + 38 ), 
					String::Format( L"x%.2f", Scale ), 
					COLOR_White, 
					Root->Font1 
				);
}


/*-----------------------------------------------------------------------------
    WBitmapEditForm implementation.
-----------------------------------------------------------------------------*/

//
// Bitmap edit form constructor.
//
WBitmapEditForm::WBitmapEditForm( FBitmap* InBitmap, WContainer* InOwner, WWindow* InRoot ) 
	:	WForm( InOwner, InRoot ),
		BitmapClass( InBitmap->GetClass() )
{
	// Initialize form.
	Caption		= String::Format( L"%s Editor", *BitmapClass->Alt );
	bSizeableH	= false;
	bSizeableW	= false;
	bCanClose	= true;
	SetSize( 300, 145 );

	// ComboBox.
	DrawType				= new WComboBox( this, Root );
	DrawType->Location		= TPoint( 10, 28 );
	DrawType->SetSize( 280, 18 );
	DrawType->EventChange	= WIDGET_EVENT(WBitmapEditForm::DrawTypeChange);

	if( BitmapClass->IsA(FFireBitmap::MetaClass) )
	{
		// Fire draw types.
		CEnum*	Enum = CClassDatabase::StaticFindEnum( L"ESparkType" );
		assert(Enum != nullptr);

		for( Integer i=0; i<Enum->Aliases.Num(); i++ )
			DrawType->AddItem( Enum->Aliases[i], nullptr );

		// Set default params.
		FireParams				= &((FFireBitmap*)InBitmap)->DrawParams;
		FireParams->Area		= 240;
		FireParams->Direction	= 0;
		FireParams->DrawType	= FFireBitmap::SPARK_Fireball;
		FireParams->Frequency	= 16;
		FireParams->Heat		= 255;
		FireParams->Life		= 100;
		FireParams->Size		= 64;
		FireParams->Speed		= 2;

		DrawType->ItemIndex		= FFireBitmap::SPARK_Fireball;
	}
	else if( BitmapClass->IsA(FWaterBitmap::MetaClass) )
	{
		// Water draw types.
		CEnum*	Enum = CClassDatabase::StaticFindEnum( L"EDropType" );
		assert(Enum != nullptr);

		for( Integer i=0; i<Enum->Aliases.Num(); i++ )
			DrawType->AddItem( Enum->Aliases[i], nullptr );

		// Set default params.
		WaterParams				= &((FWaterBitmap*)InBitmap)->DrawParams;
		WaterParams->Amplitude	= 255;
		WaterParams->Depth		= 255;
		WaterParams->DrawType	= FWaterBitmap::DROP_Oscillator;
		WaterParams->Frequency	= 8;
		WaterParams->Size		= 64;
		WaterParams->Speed		= 128;

		DrawType->ItemIndex		= FWaterBitmap::DROP_Oscillator;
	}

	// Allocate controls.
	for( Integer i=0; i<4; i++ )
	{
		TParam& P = Params[i];

		// Label.
		P.Label			= new WLabel( this, Root );
		P.Label->Location	= TPoint( 5, 55+i*20 );

		// Slider.
		P.Slider		= new WSlider( this, Root );
		P.Slider->SetSize( 200, 12 );
		P.Slider->Location	= TPoint( 90, 55+i*20 );
		P.Slider->SetOrientation(SLIDER_Horizontal);

		P.Name		= L"";
		P.Value		= nullptr;
	}

	// Bind slider events.
	Params[0].Slider->EventChange	= WIDGET_EVENT(WBitmapEditForm::Slider0Change);
	Params[1].Slider->EventChange	= WIDGET_EVENT(WBitmapEditForm::Slider1Change);
	Params[2].Slider->EventChange	= WIDGET_EVENT(WBitmapEditForm::Slider2Change);
	Params[3].Slider->EventChange	= WIDGET_EVENT(WBitmapEditForm::Slider3Change);

	// Set default params.
	DrawType->OnChange();
}


//
// When user click cross.
//
void WBitmapEditForm::OnClose()
{
	WForm::OnClose();
	Hide();
}


//
// Slider #3 has been changed.
//
void WBitmapEditForm::Slider3Change( WWidget* Sender )
{
	*Params[3].Value			= Round(Params[3].Slider->Value*255.f/100.f);
	Params[3].Label->Caption	= String::Format( L"%s %d ", *Params[3].Name, *Params[3].Value );
}


//
// Slider #2 has been changed.
//
void WBitmapEditForm::Slider2Change( WWidget* Sender )
{
	*Params[2].Value			= Round(Params[2].Slider->Value*255.f/100.f);
	Params[2].Label->Caption	= String::Format( L"%s %d ", *Params[2].Name, *Params[2].Value );
}


//
// Slider #1 has been changed.
//
void WBitmapEditForm::Slider1Change( WWidget* Sender )
{
	*Params[1].Value			= Round(Params[1].Slider->Value*255.f/100.f);
	Params[1].Label->Caption	= String::Format( L"%s %d ", *Params[1].Name, *Params[1].Value );
}

//
// Slider #0 has been changed.
//
void WBitmapEditForm::Slider0Change( WWidget* Sender )
{
	*Params[0].Value			= Round(Params[0].Slider->Value*255.f/100.f);
	Params[0].Label->Caption	= String::Format( L"%s %d ", *Params[0].Name, *Params[0].Value );
}


//
// Draw type changed.
//
void WBitmapEditForm::DrawTypeChange( WWidget* Sender )
{
	if( BitmapClass->IsA(FFireBitmap::MetaClass) )
	{
		// Change according to fire effect.
		FireParams->DrawType	= (FFireBitmap::ESparkType)DrawType->ItemIndex;

		// Hide everything default.
		for( Integer i=0; i<4; i++ )
		{
			Params[i].Label->bVisible	= false;
			Params[i].Slider->bVisible	= false;
			Params[i].Value				= nullptr;
		}

		switch( FireParams->DrawType )	
		{
			case FFireBitmap::SPARK_Point:
			case FFireBitmap::SPARK_RandomPoint:
			case FFireBitmap::SPARK_Fireball:
			{
				// Heat only.
				Params[0].Value		= &FireParams->Heat;
				Params[0].Name		= L"Heat";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)FireParams->Heat*100.f/255.f) );
				break;
			}
			case FFireBitmap::SPARK_Jitter:
			case FFireBitmap::SPARK_Twister:
			{
				// Heat, size, freq and speed.
				Params[0].Value		= &FireParams->Heat;
				Params[0].Name		= L"Heat";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)FireParams->Heat*100.f/255.f) );

				Params[1].Value		= &FireParams->Size;
				Params[1].Name		= L"Size";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)FireParams->Size*100.f/255.f) );

				Params[2].Value		= &FireParams->Frequency;
				Params[2].Name		= L"Freq";
				Params[2].Label->bVisible	= true;
				Params[2].Slider->bVisible	= true;
				Params[2].Slider->SetValue( Round((Float)FireParams->Frequency*100.f/255.f) );

				Params[3].Value		= &FireParams->Speed;
				Params[3].Name		= L"Speed";
				Params[3].Label->bVisible	= true;
				Params[3].Slider->bVisible	= true;
				Params[3].Slider->SetValue( Round((Float)FireParams->Speed*100.f/255.f) );
				break;
			}
			case FFireBitmap::SPARK_JetDownward:
			case FFireBitmap::SPARK_JetLeftward:
			case FFireBitmap::SPARK_JetRightward:
			case FFireBitmap::SPARK_JetUpward:
			case FFireBitmap::SPARK_Spermatozoa:
			{
				// Heat and life.
				Params[0].Value		= &FireParams->Heat;
				Params[0].Name		= L"Heat";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)FireParams->Heat*100.f/255.f) );

				Params[1].Value		= &FireParams->Life;
				Params[1].Name		= L"Life";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)FireParams->Life*100.f/255.f) );
				break;
			}
			case FFireBitmap::SPARK_RampLighting:
			case FFireBitmap::SPARK_RandomLighting:
			case FFireBitmap::SPARK_LineLighting:
			case FFireBitmap::SPARK_Phase:
			{
				// Heat and frequency.
				Params[0].Value		= &FireParams->Heat;
				Params[0].Name		= L"Heat";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)FireParams->Heat*100.f/255.f) );

				Params[1].Value		= &FireParams->Frequency;
				Params[1].Name		= L"Freq";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)FireParams->Frequency*100.f/255.f) );
				break;
			}
			case FFireBitmap::SPARK_Whirligig:
			{
				// Heat, life, area, freq.
				Params[0].Value		= &FireParams->Heat;
				Params[0].Name		= L"Heat";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)FireParams->Heat*100.f/255.f) );

				Params[1].Value		= &FireParams->Frequency;
				Params[1].Name		= L"Freq";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)FireParams->Frequency*100.f/255.f) );

				Params[2].Value		= &FireParams->Life;
				Params[2].Name		= L"Life";
				Params[2].Label->bVisible	= true;
				Params[2].Slider->bVisible	= true;
				Params[2].Slider->SetValue( Round((Float)FireParams->Life*100.f/255.f) );

				Params[3].Value		= &FireParams->Area;
				Params[3].Name		= L"Area";
				Params[3].Label->bVisible	= true;
				Params[3].Slider->bVisible	= true;
				Params[3].Slider->SetValue( Round((Float)FireParams->Area*100.f/255.f) );
				break;
			}
			case FFireBitmap::SPARK_Cloud:
			{
				// HorizSpeed, VertSpeed, Life & Area.
				Params[0].Value		= &FireParams->Direction;
				Params[0].Name		= L"Direct";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)FireParams->Direction*100.f/255.f) );

				Params[1].Value		= &FireParams->Speed;
				Params[1].Name		= L"Speed";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)FireParams->Speed*100.f/255.f) );

				Params[2].Value		= &FireParams->Life;
				Params[2].Name		= L"Life";
				Params[2].Label->bVisible	= true;
				Params[2].Slider->bVisible	= true;
				Params[2].Slider->SetValue( Round((Float)FireParams->Life*100.f/255.f) );

				Params[3].Value		= &FireParams->Area;
				Params[3].Name		= L"Area";
				Params[3].Label->bVisible	= true;
				Params[3].Slider->bVisible	= true;
				Params[3].Slider->SetValue( Round((Float)FireParams->Area*100.f/255.f) );
				break;
			}
			case FFireBitmap::SPARK_BallLighting:
			{
				// Heat, size, freq.
				Params[0].Value		= &FireParams->Heat;
				Params[0].Name		= L"Heat";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)FireParams->Heat*100.f/255.f) );

				Params[1].Value		= &FireParams->Frequency;
				Params[1].Name		= L"Freq";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)FireParams->Frequency*100.f/255.f) );

				Params[2].Value		= &FireParams->Size;
				Params[2].Name		= L"Size";
				Params[2].Label->bVisible	= true;
				Params[2].Slider->bVisible	= true;
				Params[2].Slider->SetValue( Round((Float)FireParams->Size*100.f/255.f) );
				break;
			}
		}
	}
	else if( BitmapClass->IsA(FWaterBitmap::MetaClass) )
	{
		// Change according to water effect.
		WaterParams->DrawType	= (FWaterBitmap::EDropType)DrawType->ItemIndex;

		// Hide everything default.
		for( Integer i=0; i<4; i++ )
		{
			Params[i].Label->bVisible	= false;
			Params[i].Slider->bVisible	= false;
			Params[i].Value				= nullptr;
		}

		switch( WaterParams->DrawType )
		{
			case FWaterBitmap::DROP_Point:
			{
				// Depth only.
				Params[0].Value		= &WaterParams->Depth;
				Params[0].Name		= L"Depth";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)WaterParams->Depth*100.f/255.f) );
				break;
			}
			case FWaterBitmap::DROP_RandomPoint:
			{
				// Amplitude only.
				Params[0].Value		= &WaterParams->Amplitude;
				Params[0].Name		= L"Ampl";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)WaterParams->Amplitude*100.f/255.f) );
				break;
			}
			case FWaterBitmap::DROP_Tap:
			case FWaterBitmap::DROP_RainDrops:
			{
				// Depth & freq.
				Params[0].Value		= &WaterParams->Depth;
				Params[0].Name		= L"Depth";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)WaterParams->Depth*100.f/255.f) );

				Params[1].Value		= &WaterParams->Frequency;
				Params[1].Name		= L"Freq";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)WaterParams->Frequency*100.f/255.f) );
				break;
			}
			case FWaterBitmap::DROP_Surfer:
			{
				// Depth & freq.
				Params[0].Value		= &WaterParams->Depth;
				Params[0].Name		= L"Depth";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)WaterParams->Depth*100.f/255.f) );

				Params[1].Value		= &WaterParams->Speed;
				Params[1].Name		= L"Speed";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)WaterParams->Speed*100.f/255.f) );
				break;
			}
			case FWaterBitmap::DROP_Oscillator:
			{
				// Ampl & freq.
				Params[0].Value		= &WaterParams->Amplitude;
				Params[0].Name		= L"Ampl";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)WaterParams->Amplitude*100.f/255.f) );

				Params[1].Value		= &WaterParams->Frequency;
				Params[1].Name		= L"Freq";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)WaterParams->Frequency*100.f/255.f) );
				break;
			}
			case FWaterBitmap::DROP_VertLine:
			case FWaterBitmap::DROP_HorizLine:
			{
				// Ampl & freq size.
				Params[0].Value		= &WaterParams->Amplitude;
				Params[0].Name		= L"Ampl";
				Params[0].Label->bVisible	= true;
				Params[0].Slider->bVisible	= true;
				Params[0].Slider->SetValue( Round((Float)WaterParams->Amplitude*100.f/255.f) );

				Params[1].Value		= &WaterParams->Frequency;
				Params[1].Name		= L"Freq";
				Params[1].Label->bVisible	= true;
				Params[1].Slider->bVisible	= true;
				Params[1].Slider->SetValue( Round((Float)WaterParams->Frequency*100.f/255.f) );

				Params[2].Value		= &WaterParams->Size;
				Params[2].Name		= L"Size";
				Params[2].Label->bVisible	= true;
				Params[2].Slider->bVisible	= true;
				Params[2].Slider->SetValue( Round((Float)WaterParams->Size*100.f/255.f) );
				break;
			}
		}
	}
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/