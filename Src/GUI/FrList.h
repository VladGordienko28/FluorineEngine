/*=============================================================================
    FrList.h: List based widgets.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    Definitions.
-----------------------------------------------------------------------------*/

//
// Common list item.
//
struct TListItem
{
public:
	// Variables.
	String		Name;
	void*		Data;

	// Constructors.
	TListItem()
		:	Name(),
			Data( nullptr )
	{}
	TListItem( String InName, void* InData )
		: Name( InName ),
		  Data( InData )
	{}
};


/*-----------------------------------------------------------------------------
    WList.
-----------------------------------------------------------------------------*/

//
// An abstract list widget.
//
class WList: public WContainer
{
public:
	// Variables.
	TNotifyEvent		EventChange;
	TNotifyEvent		EventDblClick;
	TArray<TListItem>	Items;
	Integer				ItemIndex;

	// WListWidget interface.
	WList( WContainer* InOwner, WWindow* InRoot );
	virtual Integer AddItem( String InName, void* InData );
	virtual void Remove( Integer iItem );
	virtual void Empty();
	virtual void SetItemIndex( Integer NewIdx, Bool bNotify = true );
	virtual void SelectNext();
	virtual void SelectPrev();
	virtual void OnChange();
	virtual void OnDoubleClick();

	// Utility.
	void AlphabetSort();
};


/*-----------------------------------------------------------------------------
    WListBox.
-----------------------------------------------------------------------------*/

//
// A Simple listbox.
//
class WListBox: public WList
{
public:
	// Variables.
	Integer			ItemsHeight;

	// WWidget interface.
	WListBox( WContainer* InOwner, WWindow* InRoot );
	void OnResize();
	void OnMouseLeave();
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y ) ;
	void OnDblClick( EMouseButton Button, Integer X, Integer Y );
	void OnPaint( CGUIRenderBase* Render );
	void OnKeyDown( Integer Key );
	void OnMouseScroll( Integer Delta );

protected:
	// Internal.
	WSlider*	Slider;
	Integer		iHighlight;

	Integer YToIndex( Integer Y ) const;
};


/*-----------------------------------------------------------------------------
    WComboBox.
-----------------------------------------------------------------------------*/

//
// A combination list box.
//
class WComboBox: public WList
{
public:
	// WComboBox interface.
	WComboBox( WContainer* InOwner, WWindow* InRoot );
	~WComboBox();

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );
	void OnDblClick( EMouseButton Button, Integer X, Integer Y );
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseEnter();
	void OnMouseLeave();
	void OnDeactivate();

private:
	// Internal.
	WListBox*	DropList;		
	Bool		bHighlight;

	void ShowDropList();
	void HideDropList();
	Bool IsExpanded();
	void DropListChanged( WWidget* Sender );
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/