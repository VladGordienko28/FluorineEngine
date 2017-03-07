/*=============================================================================
    FrSplit.cpp: Splitter family classes.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "GUI.h"

/*-----------------------------------------------------------------------------
    WVSplitter implementation.
-----------------------------------------------------------------------------*/

//
// Splitter constructor.
//
WVSplitter::WVSplitter( WContainer* InOwner, WWindow* InRoot )
	:	WWidget( InOwner, InRoot ),
		TopWidget( nullptr ),
		BottomWidget( nullptr ),
		TopMin( 10 ),
		BottomMin( 10 ),
		IndentTop( 0 ),
		IndentBottom( 0 )
{
	bStayOnTop		= false;
	Cursor			= CR_SizeNS;
	Size			= TSize( 100, 7 );
}


//
// Mouse move on splitter.
//
void WVSplitter::OnMouseMove( EMouseButton Button, Integer X, Integer Y )
{
	WWidget::OnMouseMove( Button, X, Y );

	if( Button == MB_Left )
	{
		Location.Y = Clamp
						( 
							Y + Location.Y - HoldOffset, 
							TopMin + IndentTop, 
							Owner->Size.Height - Size.Height - BottomMin - IndentBottom 
						);

		UpdateSubWidgets();
	}
}


//
// Mouse down on splitter.
// 
void WVSplitter::OnMouseDown( EMouseButton Button, Integer X, Integer Y )
{
	WWidget::OnMouseDown( Button, X, Y );

	if( Button == MB_Left )
		HoldOffset	= Y;
}


//
// Redraw splitter.
//
void WVSplitter::OnPaint( CGUIRenderBase* Render )
{
	WWidget::OnPaint( Render );

	Render->DrawRegion
				( 
					ClientToWindow(TPoint::Zero),
					Size,
					GUI_COLOR_SPLITTER,
					GUI_COLOR_SPLITTER, 
					BPAT_Solid 
				);
}


//
// Splitter resize.
//
void WVSplitter::OnResize()
{
	WWidget::OnResize();

	// Let splitter always fit owner.
	Location.X	= Owner->Padding.Left;
	Location.Y	= Clamp( Location.Y, TopMin+IndentTop, Owner->Size.Height - Size.Height - BottomMin - IndentBottom );
	Size.Width	= Owner->Size.Width - Owner->Padding.Left - Owner->Padding.Right;

	UpdateSubWidgets();
}


//
// Locate sub-widgets.
//
void WVSplitter::UpdateSubWidgets()
{
	if( TopWidget )
	{
		TopWidget->Location.X	= Owner->Padding.Left;
		TopWidget->Location.Y	= Owner->Padding.Top + IndentTop + TopWidget->Margin.Top;
		TopWidget->Size.Height	= Location.Y - IndentTop - TopWidget->Margin.Top - TopWidget->Margin.Bottom;
		TopWidget->Size.Width	= Size.Width;

		// Notify.
		TopWidget->WidgetProc( WPE_Resize, TWidProcParms() );
	}

	if( BottomWidget )
	{
		BottomWidget->Location.X	= Owner->Padding.Left;
		BottomWidget->Location.Y	= Location.Y + Size.Height + IndentBottom;
		BottomWidget->Size.Width	= Size.Width;
		BottomWidget->Size.Height	= Owner->Size.Height - Location.Y - Size.Height - Owner->Padding.Bottom - IndentBottom;

		// Notify.
		BottomWidget->WidgetProc( WPE_Resize, TWidProcParms() );	
	}
}


/*-----------------------------------------------------------------------------
    WHSplitter.
-----------------------------------------------------------------------------*/

//
// Splitter constructor.
//
WHSplitter::WHSplitter( WContainer* InOwner, WWindow* InRoot )
	:	WWidget( InOwner, InRoot ),
		LeftWidget( nullptr ),
		RightWidget( nullptr ),
		LeftMin( 10 ),
		RightMin( 10 ),
		IndentLeft( 0 ),
		IndentRight( 0 )
{
	bStayOnTop		= false;
	Cursor			= CR_SizeWE;
	Size			= TSize( 7, 100 );
}


//
// Mouse move on splitter.
//
void WHSplitter::OnMouseMove( EMouseButton Button, Integer X, Integer Y )
{
	WWidget::OnMouseMove( Button, X, Y );

	if( Button == MB_Left )
	{
		Location.X = Clamp
						( 
							X + Location.X - HoldOffset, 
							LeftMin + IndentLeft, 
							Owner->Size.Width - Size.Width - RightMin - IndentRight 
						);

		UpdateSubWidgets();
	}
}


//
// Mouse down on splitter.
// 
void WHSplitter::OnMouseDown( EMouseButton Button, Integer X, Integer Y )
{
	WWidget::OnMouseDown( Button, X, Y );

	if( Button == MB_Left )
		HoldOffset	= X;
}


//
// Redraw splitter.
//
void WHSplitter::OnPaint( CGUIRenderBase* Render )
{
	WWidget::OnPaint( Render );

	Render->DrawRegion
				( 
					ClientToWindow(TPoint::Zero),
					Size,
					GUI_COLOR_SPLITTER,
					GUI_COLOR_SPLITTER, 
					BPAT_Solid 
				);
}


//
// Splitter resize.
//
void WHSplitter::OnResize()
{
	WWidget::OnResize();

	// Let splitter always fit owner.
	Location.Y	= Owner->Padding.Top;
	Location.X	= Clamp( Location.X, LeftMin+IndentLeft, Owner->Size.Width - Size.Width - RightMin - IndentRight );
	Size.Height	= Owner->Size.Height - Owner->Padding.Top - Owner->Padding.Bottom; 

	UpdateSubWidgets();
}


//
// Locate sub-widgets.
//
void WHSplitter::UpdateSubWidgets()
{
	if( LeftWidget )
	{
		LeftWidget->Location.X	= Owner->Padding.Left + IndentLeft;
		LeftWidget->Location.Y	= Owner->Padding.Top;
		LeftWidget->Size.Width	= Location.X - IndentLeft;
		LeftWidget->Size.Height	= Size.Height;

		// Notify.
		LeftWidget->WidgetProc( WPE_Resize, TWidProcParms() );
	}

	if( RightWidget )
	{
		RightWidget->Location.X		= Location.X + Size.Width + IndentRight;
		RightWidget->Location.Y		= Owner->Padding.Top;
		RightWidget->Size.Width		= Owner->Size.Width - Location.X - Size.Width - Owner->Padding.Right - IndentRight;
		RightWidget->Size.Height	= Size.Height;

		// Notify.
		RightWidget->WidgetProc( WPE_Resize, TWidProcParms() );
	}
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/