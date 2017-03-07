/*=============================================================================
    FrProjExp.h: A project explorer panel.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    WProjectExplorer.
-----------------------------------------------------------------------------*/

//
// A project explorer panel.
//
class WProjectExplorer: public WPanel, public CRefsHolder
{
public:
	// Controls.
	WPictureButton*		AddLevelButton;
	WPictureButton*		DeleteLevelButton;
	WPictureButton*		ProjectPropsButton;
	WPanel*				HeaderPanel;
	WListBox*			LevelsList;
	
	// WProjectExplorer interface.
	WProjectExplorer( WContainer* InOwner, WWindow* InRoot );
	void Refresh();

	// Controls notifications.
	void ButtonAddLevelClick( WWidget* Sender );
	void ButtonDeleteLevelClick( WWidget* Sender );
	void ButtonProjectPropsClick( WWidget* Sender );
	void ListLevelsChange( WWidget* Sender );
	void ListLevelsDblClick( WWidget* Sender );
	void MessageDeleteYesClick( WWidget* Sender );

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render );	

	// CRefHolder interface.
	void CountRefs( CSerializer& S );
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/