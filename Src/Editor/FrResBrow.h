/*=============================================================================
    FrResBrowser.h: A resource browser panel.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    WBrowserPanel.
-----------------------------------------------------------------------------*/

//
// A browser panel with resources.
//
class WBrowserPanel: public WPanel, public CRefsHolder
{
public:
	// A single resource icon.
	struct TResourceIcon
	{
	public:
		TPoint		Position;
		FResource*	Resource;
		FBitmap*	Picture;
		Char*		TypeName;
		TPoint		SrcPos;
		TSize		SrcScale;
		TSize		DstScale;

		friend Bool ResourceIconCmp( const TResourceIcon& A, const TResourceIcon& B )
		{
			return String::CompareText
									( 
										A.Resource->GetName(), 
										B.Resource->GetName() 
									) < 0;
		}
	};

	// Controls.
	WSlider*				ScrollBar;
	WResourceBrowser*		Browser;
	WPopupMenu*				Popup;

	// Resources.
	TArray<TResourceIcon>	Icons;
	FResource*				Selected;
	String					SelGroup;
	CClass*					ClassFilter;

	// WBrowserPanel interface.
	WBrowserPanel( WResourceBrowser* InBrowser, WWindow* InRoot );
	~WBrowserPanel();
	void RefreshList();

	// CRefHolder interface.
	void CountRefs( CSerializer& S );

	// WWidget events.
	void OnPaint( CGUIRenderBase* Render );
	void OnMouseScroll( Integer Delta );
	void OnMouseDown( EMouseButton Button, Integer X, Integer Y );
	void OnMouseUp( EMouseButton Button, Integer X, Integer Y );
	void OnMouseMove( EMouseButton Button, Integer X, Integer Y );
	void OnDblClick( EMouseButton Button, Integer X, Integer Y );    

private:
	// Panel internal.
	Bool		bReadyDrag;

	// Notifications.
	void ScrollChange( WWidget* Sender );
	void PopEditClick( WWidget* Sender );
	void PopRenameClick( WWidget* Sender );
	void PopRemoveClick( WWidget* Sender );
};


/*-----------------------------------------------------------------------------
    WResourceBrowser.
-----------------------------------------------------------------------------*/

//
// A resource browser.
//
class WResourceBrowser: public WContainer			
{
public:
	// Controls.
	WPanel*				HeaderPanel;
	WBrowserPanel*		BrowserPanel;
	WPictureButton*		ImportButton;
	WPictureButton*		CreateButton;
	WPictureButton*		ClassButton;
	WPictureButton*		RemoveButton;
	WComboBox*			GroupFilter;
	WEdit*				NameFilter;
	WPopupMenu*			NewPopup;
	WPopupMenu*			ClassPopup;		

	// WResourceBrowser interface.
	WResourceBrowser( WContainer* InOwner, WWindow* InRoot );
	~WResourceBrowser();
	void RefreshList();
	FResource* GetSelected() const;

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );	

	// Controls notification.
	void ButtonImportClick( WWidget* Sender );
	void ButtonCreateClick( WWidget* Sender );
	void ButtonClassClick( WWidget* Sender );
	void ButtonRemoveClick( WWidget* Sender );
	void EditNameChange( WWidget* Sender );
	void ComboGroupChange( WWidget* Sender );
	void PopClassAllClick( WWidget* Sender );
	void PopClassClick( WWidget* Sender );
	void PopNewEffectClick( WWidget* Sender );
	void PopNewAnimationClick( WWidget* Sender );
	void PopNewScriptClick( WWidget* Sender );
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/