/*=============================================================================
    FrEdit.h: Text/Numbers edit classes.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    WEdit.
-----------------------------------------------------------------------------*/

//
// An edit text type.
//
enum EEditType
{
	EDIT_String,
	EDIT_Integer,
	EDIT_Float
};


//
// A box with editable text.
//
class WEdit: public WWidget
{
public:
	// Variables.
	TNotifyEvent	EventAccept;
	TNotifyEvent	EventChange;
	Bool			bReadOnly;
	EEditType		EditType;
	String			Text;

	// WEdit interface.
	WEdit( WContainer* InOwner, WWindow* InRoot );
	void SetText( String NewText, Bool bNotify = true );

	// WWidget interface.
	void OnDeactivate();
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseUp( EMouseButton Button, Integer X, Integer Y );
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );
	void OnDblClick( EMouseButton Button, Integer X, Integer Y );
	void OnKeyDown( Integer Key );
	void OnCharType( Char TypedChar );
	void OnPaint( CGUIRenderBase* Render );

	// Notifications.
	virtual void OnChange();
	virtual void OnAccept();

protected:
	// Internal.
	Integer		ScrollX;
	Integer		CaretBegin;
	Integer		CaretEnd;
	TSize		CharSize;
	Float		OldFloat;
	Integer		OldInteger;

	Integer CaretToPixel( Integer C );
	Integer PixelToCaret( Integer X );
	void ScrollToCaret();
	void SelectAll();
	void ClearSelected();
	void Store();
};


// Not implemented.
#if 0
/*-----------------------------------------------------------------------------
    WSpinner.
-----------------------------------------------------------------------------*/

//
// Widget to edit numberic values.
//
class WSpinner: public WEdit
{
public:
	// Variables.
	Float			Min;
	Float			Max; 
	Float			Scale;

	// WSpinner interface.
	WSpinner( WContainer* InOwner, WWindow* InRoot );

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseUp( EMouseButton Button, Integer X, Integer Y );
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );
	void OnDblClick( EMouseButton Button, Integer X, Integer Y );

private:
	// Spinner internal.
	Bool			bSpin;
};
#endif

/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/