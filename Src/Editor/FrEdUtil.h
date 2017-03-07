/*=============================================================================
    FrEdUtil.h: Various editor stuff.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    CGUIRender.
-----------------------------------------------------------------------------*/

//
// GUI Render.
//
class CGUIRender: public CGUIRenderBase
{
public:
	CCanvas*	Canvas;
	Float		Brightness;

	// CGUIRender interface.
	CGUIRender();
	~CGUIRender();
	void BeginPaint( CCanvas* InCanvas );
	void EndPaint();

	// CGUIRenderBase interface.
	void DrawRegion( TPoint P, TSize S, TColor Color, TColor BorderColor, EBrushPattern Pattern );
	void DrawText( TPoint P, const Char* Text, Integer Len, TColor Color, FFont* Font );
	void SetClipArea( TPoint P, TSize S );
	void DrawPicture( TPoint P, TSize S, TPoint BP, TSize BS, FBitmap* Bitmap );
	void SetBrightness( Float Brig );
};


/*-----------------------------------------------------------------------------
    CSG.
-----------------------------------------------------------------------------*/

// CSG functions.
extern void CSGUnion( FBrushComponent* Brush, FLevel* Level );
extern void CSGIntersection( FBrushComponent* Brush, FLevel* Level );
extern void CSGDifference( FBrushComponent* Brush, FLevel* Level );


/*-----------------------------------------------------------------------------
    Dialogs.
-----------------------------------------------------------------------------*/

// Winapi wrap functions.
extern Bool ExecuteColorDialog( TColor& PickedColor, const TColor Default = COLOR_Black );
extern Bool ExecuteOpenFileDialog( String& FileName, String Directory, const Char* Filters );
extern Bool ExecuteSaveFileDialog( String& FileName, String Directory, const Char* Filters );


/*-----------------------------------------------------------------------------
    Misc.
-----------------------------------------------------------------------------*/

// File functions.
extern String GetFileName( String FileName );
extern String GetFileDir( String FileName );


/*-----------------------------------------------------------------------------
    WinApi Resources.
-----------------------------------------------------------------------------*/

// GUI objects loaders.
extern TStaticBitmap* LoadBitmapFromResource( LPCTSTR ResID );
extern TStaticFont* LoadFontFromResource( LPCTSTR FontID, LPCTSTR BitmapID );


/*-----------------------------------------------------------------------------
    WRenameDialog.
-----------------------------------------------------------------------------*/

//
// A resource rename dialog.
//
class WRenameDialog: public WForm, public CRefsHolder
{
public:
	// WRenameDialog interface.
	WRenameDialog( WWindow* InRoot );
	void SetResource( FResource* InResource );

	// WForm interface.
	void OnClose();
	void Show( Integer X, Integer Y );
	void Hide();

	// Notifications.
	void ButtonCancelClick( WWidget* Sender );
	void ButtonOkClick( WWidget* Sender );

	// CRefsHolder interface.
	void CountRefs( CSerializer& S );

private:
	// Internal.
	FResource*			Resource;
	WLabel*				NameLabel;
	WLabel*				GroupLabel;
	WEdit*				NameEdit;
	WEdit*				GroupEdit;
	WPanel*				TopPanel;
	WButton*			OkButton;
	WButton*			CancelButton;
	WForm*				OldModal;
};
extern WRenameDialog*	GRenameDialog;


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/