/*=============================================================================
    FrList.cpp: List based widgets.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "GUI.h"

/*-----------------------------------------------------------------------------
    WComboBox implementation.
-----------------------------------------------------------------------------*/

//
// ComboBox constructor.
//
WComboBox::WComboBox( WContainer* InOwner, WWindow* InRoot )
	:	WList( InOwner, InRoot ),
		bHighlight( false )
{
	// Create dropdown list.
	DropList				= new WListBox( Root, Root );	
	DropList->bStayOnTop	= true;
	DropList->EventChange	= TNotifyEvent( this, (TNotifyEvent::TEvent)&WComboBox::DropListChanged );

	// Set drop list really on top, sort of hack, but
	// works well. Because DropList should stay on the top of top!
	for( Integer i=Root->Children.FindItem(DropList); i<Root->Children.Num()-1; i++ )
		Root->Children.Swap( i, i+1 );

	HideDropList();
	SetSize( 150, 18 );
}


//
// ComboBox destructor.
//
WComboBox::~WComboBox()
{
	// Figure out, is DropList still valid?
	if( DropList && (Root->Children.FindItem(DropList) != -1) )
		delete DropList;
}


//
// ComboBox paint.
//
void WComboBox::OnPaint( CGUIRenderBase* Render )
{
	WList::OnPaint( Render );

	TPoint Base = ClientToWindow(TPoint::Zero);

	// Draw pad.
	Render->DrawRegion
				(
					Base,
					Size,
					GUI_COLOR_LIST,
					GUI_COLOR_LIST_BORDER,
					BPAT_Solid 
				);

	// Highlight button.
	if( bHighlight || IsExpanded() )
		Render->DrawRegion
					( 
						TPoint( Base.X + Size.Width - 18, Base.Y ),
						TSize(18, Size.Height), 
						IsExpanded() ? GUI_COLOR_LIST_SEL : GUI_COLOR_LIST_HIGHLIGHT, 
						GUI_COLOR_LIST_BORDER,
						BPAT_Solid 
					);

	// Draw a little triangle.
	Render->DrawPicture
				( 
					TPoint( Base.X+Size.Width-12, Base.Y+Size.Height/2-2 ), 
					TSize(6, 3), 
					TPoint(15, 0), 
					TSize(6, 3), 
					Root->Icons 
				);

	// Draw selected item text.
	if( ItemIndex != -1 )
	{
		TListItem& Item	= Items[ItemIndex];
		TPoint TextPos	= TPoint( Base.X + 3, Base.Y + (Size.Height-Root->Font1->Height) / 2 );
		Render->SetClipArea( Base, TSize( Size.Width-19, Size.Height ) );
		Render->DrawText( TextPos, Item.Name, bEnabled ? GUI_COLOR_TEXT : GUI_COLOR_TEXT_OFF, Root->Font1 );
	}	
}


//
// Dbl clicked - increment index.
//
void WComboBox::OnDblClick( EMouseButton Button, Integer X, Integer Y )
{
	WList::OnDblClick( Button, X, Y );

	if( Items.Num() && X < Size.Width-18 )
	{
		ItemIndex = ( ItemIndex + 1 ) % Items.Num();	  
		OnChange();
	}
}


//
// Mouse press comboBox.
//
void WComboBox::OnMouseDown( EMouseButton Button, Integer X, Integer Y )
{
	WList::OnMouseDown( Button, X, Y );

	if( Button == MB_Left && X > Size.Width-18 )
	{
		// Toggle.
		if( IsExpanded() )		
			HideDropList();
		 else
			ShowDropList();
	}
}


//
// Cursor enter area.
//
void WComboBox::OnMouseEnter()
{
	WList::OnMouseEnter();
	bHighlight	= true;
}


//
// Cursor leave area.
//
void WComboBox::OnMouseLeave()
{
	WList::OnMouseLeave();
	bHighlight	= false;
}


//
// Open up drop list.
//
void WComboBox::ShowDropList()
{
	// Update items list.
	DropList->Empty();
	for( Integer i=0; i<Items.Num();i++ )
		DropList->AddItem( Items[i].Name, Items[i].Data );

	// Compute show location and size.
	TPoint P = ClientToWindow(TPoint::Zero);
	//DropList->SetSize( Size.Width, DropList->ItemsHeight*Items.Num()+3 );
	DropList->Size.Width	= Size.Width;
	DropList->Size.Height	= DropList->ItemsHeight*Items.Num()+3;

	// Fuck! Fucked Visual Studio crashes when debug step-by-step
	// if without variable! Crap!
	Integer MaxY = Root->Size.Height;

	// Show it upward or downward.
	if( P.Y+Size.Height+DropList->Size.Height > MaxY )
		P.Y -= DropList->Size.Height-1;	
	else
		P.Y += Size.Height-1;

	DropList->Location = P;
	
	// Show.log( L"%i", Root->Size.Height );
	DropList->bVisible = true;

	// Set drop list really on top, sort of hack, but
	// works well. Because DropList should stay on the top of top!
	for( Integer i=Root->Children.FindItem(DropList); i<Root->Children.Num()-1; i++ )
		Root->Children.Swap( i, i+1 );
}


//
// Shut down drop list.
//
void WComboBox::HideDropList()
{
	DropList->bVisible = false;
}


//
// Return true, if drop list are visible.
//
Bool WComboBox::IsExpanded()
{
	return DropList->bVisible;
}


//
// Notification from drop list.
//
void WComboBox::DropListChanged( WWidget* Sender )
{
	ItemIndex = DropList->ItemIndex;
	OnChange();
	HideDropList();
}


//
// ComboBox lost focus.
//
void WComboBox::OnDeactivate()
{
	WList::OnDeactivate();
	HideDropList();
}


/*-----------------------------------------------------------------------------
    WListBox implementation.
-----------------------------------------------------------------------------*/

//
// ListBox constructor.
//
WListBox::WListBox( WContainer* InOwner, WWindow* InRoot )
	: WList( InOwner, InRoot ),
	  iHighlight( -1 ),
	  ItemsHeight( 12 )
{
	// Allocate slider.
	Slider = new WSlider( this, Root );
	Slider->SetOrientation( SLIDER_Vertical );
	Slider->SetValue( 0 );
	SetSize( 150, 110 );		
}


//
// When listbox resized.
//
void WListBox::OnResize()
{
	WList::OnResize();
	Slider->SetLocation( Size.Width-12, 0 );
	Slider->SetSize( 12, Size.Height );
}


//
// When mouse leave listbox.
//
void WListBox::OnMouseLeave()
{
	WList::OnMouseLeave();	
	iHighlight = -1;
}


//
// Mouse hover list box.
//
void WListBox::OnMouseMove( EMouseButton Button, Integer X, Integer Y )
{
	WList::OnMouseMove( Button, X, Y );
	iHighlight = YToIndex( Y );
}


//
// On item click.
//
void WListBox::OnMouseDown( EMouseButton Button, Integer X, Integer Y ) 
{
	WList::OnMouseDown( Button, X, Y );   
	ItemIndex = YToIndex( Y );
	OnChange();
}


//
// Dbl click on item.
//
void WListBox::OnDblClick( EMouseButton Button, Integer X, Integer Y )
{
	WList::OnDblClick( Button, X, Y );
	OnDoubleClick();
}


//
// When user press some key.
//
void WListBox::OnKeyDown( Integer Key )
{
	WList::OnKeyDown(Key);
	if( Key == 38 )		// <Up>
		SelectPrev();
	if( Key == 40 )		// <Down>
		SelectNext();

	// Scroll to show selected.
	if( Slider->bVisible && Items.Num()>1 )
		Slider->Value	= Clamp( ItemIndex*100/(Items.Num()-1), 0, 100 );
}


//
// Mouse scroll over list box.
//
void WListBox::OnMouseScroll( Integer Delta )
{
	WList::OnMouseScroll(Delta);
	if( Slider->bVisible )
	{
		Slider->Value	= Clamp
		( 
			Slider->Value-Delta/120, 
			0, 
			100 
		);
	}
}


//
// ListBox paint.
//
void WListBox::OnPaint( CGUIRenderBase* Render )
{
	WList::OnPaint( Render );
	TPoint Base = ClientToWindow(TPoint(0, 0));
	
	// Show or hide the slider.
	if( Items.Num() * ItemsHeight > Size.Height   )
	{
		Slider->bVisible = true;
	}
	else
	{
		Slider->bVisible = false;
		Slider->SetValue( 0 );
	}

	// Draw pad.
	Render->DrawRegion
			( 
				Base,
				Size,
				GUI_COLOR_LIST,
				GUI_COLOR_LIST_BORDER,
				BPAT_Solid 
			);

	// Turn on clipping.
	Render->SetClipArea
					( 
						Base, 
						TSize( Size.Width-1, Size.Height ) 
					);

	// Draw items.
	if( Items.Num() > 0 )
	{
		Integer TextY	= (ItemsHeight - Root->Font1->Height) / 2;

		// For each item.
		for( Integer i = 0, iItem = YToIndex(0); 
			 i < Size.Height/ItemsHeight && iItem < Items.Num(); 
			 i++, iItem++ )
		{			
			TListItem& Item = Items[iItem];

			if( iHighlight == iItem )
				Render->DrawRegion
						( 
							TPoint( Base.X + 1, Base.Y+i * ItemsHeight + 1 ),
							TSize( Size.Width - 2, ItemsHeight ),
							GUI_COLOR_LIST_HIGHLIGHT,
							GUI_COLOR_LIST_HIGHLIGHT,
							BPAT_Solid 
						);

			if( ItemIndex == iItem )
				Render->DrawRegion
						( 
							TPoint( Base.X + 1, Base.Y+i * ItemsHeight + 1 ),
							TSize( Size.Width - 2, ItemsHeight ),
							GUI_COLOR_LIST_SEL,
							GUI_COLOR_LIST_SEL,
							BPAT_Solid 
						);

			// Draw item text.
			TPoint TextLoc = TPoint( Base.X + 3, Base.Y + i*ItemsHeight + 1 + TextY );		
			Render->DrawText
					( 
						TextLoc, 
						Item.Name, 
						GUI_COLOR_TEXT, 
						Root->Font1 
					);
		}
	}
}


//
// convert Y value to index, also handle
// slider offset. If no item found return -1.
//
Integer WListBox::YToIndex( Integer Y ) const
{
	Integer NumVis = Size.Height / ItemsHeight;
	Integer Offset = Round((Items.Num() - NumVis) * Slider->Value / 100.f);
	Integer Index  = Offset + Y / ItemsHeight;
	return Index < 0 ? -1 : Index >= Items.Num() ? -1 : Index;
}


/*-----------------------------------------------------------------------------
    WList implementation.
-----------------------------------------------------------------------------*/

//
// List constructor.
//
WList::WList( WContainer* InOwner, WWindow* InRoot )
	:	WContainer( InOwner, InRoot ),
		Items(),
		ItemIndex( -1 )
{
}


//
// Add a new item to the list.
//
Integer WList::AddItem( String InName, void* InData )			
{
	return Items.Push(TListItem( InName, InData ));
}



//
// Remove an item from list, if invalid index,
// nothing will happened.
//
void WList::Remove( Integer iItem )	
{
	if( iItem >= 0 && iItem < Items.Num() )
	{
		Items.RemoveShift(iItem);
		ItemIndex = Clamp( ItemIndex, -1, Items.Num()-1 );	
	}
}


//
// Cleanup the list.
//
void WList::Empty()	
{
	Items.Empty();
	ItemIndex = -1;
}


//
// Update selected item.
//
void WList::SetItemIndex( Integer NewIdx, Bool bNotify )	
{
	ItemIndex = Clamp( NewIdx, -1, Items.Num()-1 );

	if( bNotify )
		OnChange();
}


//
// Selected item changed notification.
//
void WList::OnChange()	
{
	EventChange( this );
}


//
// Double click on item notification.
//
void WList::OnDoubleClick()	
{
	EventDblClick( this );
}


//
// List items comparison.
//
Bool ListItemCmp( const TListItem& A, const TListItem& B )
{
	return String::CompareText( A.Name, B.Name ) < 0;
}


//
// Sort list alphabet order.
//
void WList::AlphabetSort()
{
	Items.Sort(ListItemCmp);
}


//
// Select next item.
//
void WList::SelectNext()
{
	if( ItemIndex < Items.Num()-1 )
		ItemIndex++;
}


//
// Select prev item.
//
void WList::SelectPrev()
{
	if( ItemIndex > 0 )
		ItemIndex--;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/