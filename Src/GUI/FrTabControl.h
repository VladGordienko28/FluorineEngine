/*=============================================================================
    FrTabControl.h: Tab control.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    WTabPage.
-----------------------------------------------------------------------------*/

//
// A single page of tab control.
//
class WTabPage: public WContainer
{
public:
	// Variables.
	TColor			Color;
	Integer			TabWidth;
	WTabControl*	TabControl;

	// WTabPage interface.
	WTabPage( WContainer* InOwner, WWindow* InRoot );
	virtual Bool OnQueryClose()
	{
		return true;
	}
	virtual void OnOpen()
	{}
	virtual void Close( Bool bForce = false );

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );
};


/*-----------------------------------------------------------------------------
    WTabControl.
-----------------------------------------------------------------------------*/

//
// A tab control.
//
class WTabControl: public WContainer
{
public:
	// Variables.
	TArray<WTabPage*>	Pages;
	Integer				iActive;

	// WTabControl interface.	
	WTabControl( WContainer* InOwner, WWindow* InRoot );
	void ActivateTabPage( Integer iPage );
	void ActivateTabPage( WTabPage* Page );
	void CloseTabPage( Integer iPage, Bool bForce = false );
	Integer AddTabPage( WTabPage* Page );
	WTabPage* GetActivePage();

	// WWidget interface.
	void OnActivate();
	void OnPaint( CGUIRenderBase* Render );
	void OnMouseLeave();
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );
	void OnMouseUp( EMouseButton Button, Integer X, Integer Y );

private:
	// Internal.
	WPopupMenu*			Popup;
	Integer				iHighlight;
	Integer				iCross;
	WTabPage*			DragPage;
	Bool				bWasDrag;
	Bool				bOverflow;

	Integer XToIndex( Integer InX );
	Integer XYToCross( Integer InX, Integer InY );
	void PopupPageClick( WWidget* Sender );
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/