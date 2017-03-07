/*=============================================================================
    FrSplit.h: Widgets to split area.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    WVSplitter.
-----------------------------------------------------------------------------*/

//
// A vertical splitter to split container.
//
class WVSplitter: public WWidget
{
public:
	// Variables.
	WWidget*	TopWidget;
	WWidget*	BottomWidget;
	Integer		TopMin;
	Integer		BottomMin;
	Integer		IndentTop;
	Integer		IndentBottom;

	// WVSplitter interface.
	WVSplitter( WContainer* InOwner, WWindow* InRoot );

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );
	void OnResize();

private:
	// Internal.
	Integer		HoldOffset;

	void UpdateSubWidgets();
};


/*-----------------------------------------------------------------------------
    WHSplitter.
-----------------------------------------------------------------------------*/

//
// A horizontal splitter to divide container.
//
class WHSplitter: public WWidget
{
public:
	// Variables.
	WWidget*	LeftWidget;
	WWidget*	RightWidget;
	Integer		LeftMin;
	Integer		RightMin;
	Integer		IndentLeft;
	Integer		IndentRight;

	// WHSplitter interface.
	WHSplitter( WContainer* InOwner, WWindow* InRoot );

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );
	void OnResize();

private:
	// Internal.
	Integer		HoldOffset;

	void UpdateSubWidgets();
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/