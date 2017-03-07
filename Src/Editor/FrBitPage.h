/*=============================================================================
    FrBitPage.h: Bitmap page.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    WBitmapEditForm.
-----------------------------------------------------------------------------*/

//
// Bitmap editor form.
//
class WBitmapEditForm: public WForm
{
public:
	// An information about parameter for demo effect.
	struct TParam
	{
	public:
		String		Name;
		WLabel*		Label;
		WSlider*	Slider;
		Byte*		Value;
	};

	// Variables.
	CClass*			BitmapClass;
	WComboBox*		DrawType;
	TParam			Params[4];

	// Parameters.
	union
	{
		FFireBitmap::TFireDrawParams*	FireParams;
		FWaterBitmap::TWaterDrawParams*	WaterParams;
	};

	// WBitmapEditForm interface.
	WBitmapEditForm( FBitmap* InBitmap, WContainer* InOwner, WWindow* InRoot );

	// WForm interface.
	void OnClose();

	// Notifications.
	void DrawTypeChange( WWidget* Sender );
	void Slider0Change( WWidget* Sender );
	void Slider1Change( WWidget* Sender );
	void Slider2Change( WWidget* Sender );
	void Slider3Change( WWidget* Sender );
};


/*-----------------------------------------------------------------------------
    WBitmapPage.
-----------------------------------------------------------------------------*/

//
// Bitmap editor page.
//
class WBitmapPage: public WEditorPage, public CRefsHolder
{
public:
	// Variables.
	FBitmap*			Bitmap;
	WBitmapEditForm*	EditForm;

	// WBitmapPage interface.
	WBitmapPage( FBitmap* InBitmap, WContainer* InOwner, WWindow* InRoot );
	~WBitmapPage();

	// WTabPage interface.
	Bool OnQueryClose();
	void OnOpen();

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseUp( EMouseButton Button, Integer X, Integer Y );
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );

	// WEditorPage interface.
	void TickPage( Float Delta );
	FResource* GetResource()
	{ 
		return Bitmap; 
	}

private:
	// Internal.
	enum EDragMode
	{
		DRAG_None,
		DRAG_Panning,
		DRAG_Drawing
	};

	EDragMode			DragMode;
	TPoint				DragFrom;
	Float				Scale;
	TPoint				Pan;
	Bool				bMouseMove;

	// Internal widgets.
	WToolBar*			ToolBar;
	WButton*			EditButton;
	WButton*			EraseButton;
	WButton*			ZoomInButton;		
	WButton*			ZoomOutButton;		

	// Widgets notifications.
	void ButtonEditClick( WWidget* Sender );
	void ButtonEraseClick( WWidget* Sender );
	void ButtonZoomInClick( WWidget* Sender );
	void ButtonZoomOutClick( WWidget* Sender );

public:
	// CRefsHolder interface.
	void CountRefs( CSerializer& S )
	{
		Serialize( S, Bitmap );

		if( !Bitmap )
			this->Close( true );
	}
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/