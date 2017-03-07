/*=============================================================================
    FrProjExp.cpp: A project explorer.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "Editor.h"

/*-----------------------------------------------------------------------------
	WLevelConstructor implementation
-----------------------------------------------------------------------------*/

//
// An level construction dialog.
//
class WLevelConstructor: public WForm
{
public:
	// Variables.
	WLabel*				NameLabel;
	WLabel*				CameraLabel;
	WEdit*				NameEdit;
	WComboBox*			CameraCombo;
	WPanel*				TopPanel;
	WButton*			OkButton;
	WButton*			CancelButton;
	WProjectExplorer*	Explorer;

	// WLevelConstructor interface.
	WLevelConstructor( WProjectExplorer* InExplorer, WWindow* InRoot )
		:	WForm( InRoot, InRoot ),
			Explorer( InExplorer )
	{
		// Initialize the form.
		Caption			= L"Level Constructor";
		bCanClose		= true;
		bSizeableH		= false;
		bSizeableW		= false;
		SetSize( 240, 123 );

		TopPanel			= new WPanel( this, Root );
		TopPanel->SetLocation( 8, 28 );
		TopPanel->SetSize( 224, 57 );	

		NameLabel			= new WLabel( TopPanel, Root );
		NameLabel->Caption	= L"Name:";
		NameLabel->SetLocation( 6, 8 );

		NameEdit			= new WEdit( TopPanel, Root );
		NameEdit->EditType	= EDIT_String;
		NameEdit->Location	= TPoint( 56, 7 );
		NameEdit->SetSize( 160, 18 );

		CameraLabel				= new WLabel( TopPanel, Root );
		CameraLabel->Caption	= L"Camera:";
		CameraLabel->SetLocation( 6, 32 );

		CameraCombo				= new WComboBox( TopPanel, Root );
		CameraCombo->Location	= TPoint( 56, 31 );
		CameraCombo->SetSize( 160, 18 );

		OkButton				= new WButton( this, Root );
		OkButton->Caption		= L"Ok";
		OkButton->EventClick	= WIDGET_EVENT(WLevelConstructor::ButtonOkClick);
		OkButton->SetLocation( 38, 90 );
		OkButton->SetSize( 64, 25 );

		CancelButton				= new WButton( this, Root );
		CancelButton->Caption		= L"Cancel";
		CancelButton->EventClick	= WIDGET_EVENT(WLevelConstructor::ButtonCancelClick);
		CancelButton->SetLocation( 138, 90 );
		CancelButton->SetSize( 64, 25 );

		Hide();
	}

	// WForm interface.
	void OnClose()
	{
		WForm::OnClose();
		Hide();
	}
	void Show( Integer X = 0, Integer Y = 0 )
	{
		WForm::Show( X, Y );
		NameEdit->Text			= L"";
		CameraCombo->Empty();
		CameraCombo->ItemIndex	= -1;
		for( Integer i=0; i<GProject->GObjects.Num(); i++ )
			if( GProject->GObjects[i] && GProject->GObjects[i]->IsA(FScript::MetaClass) )
			{
				FScript* Script = As<FScript>(GProject->GObjects[i]);
				if( Script->Base->IsA(FCameraComponent::MetaClass) )
					CameraCombo->AddItem( Script->GetName(), Script );
			}
	}

	// Notifications.
	void ButtonCancelClick( WWidget* Sender )
	{
		Hide();
	}
	void ButtonOkClick( WWidget* Sender )
	{
		if( NameEdit->Text.Len() == 0 )
		{
			Root->ShowMessage
			(
				L"Please specify the name of level",
				L"Level Constructor",
				true
			);
			return;
		}
		if( GObjectDatabase->FindObject( NameEdit->Text ) )
		{
			Root->ShowMessage
			(
				String::Format( L"Object '%s' already exists", *NameEdit->Text ),
				L"Level Constructor",
				true
			);	
			return;
		}
		if( CameraCombo->ItemIndex == -1 )
		{
			Root->ShowMessage
			(
				L"Camera script not selected",
				L"Level Constructor",
				true
			);
			return;
		}

		FLevel* Level	= NewObject<FLevel>( NameEdit->Text );
		Level->CreateEntity
						( 
							(FScript*)CameraCombo->Items[CameraCombo->ItemIndex].Data, 
							L"Camera", 
							TVector( 0.f, 0.f ) 
						);
		Level->RndFlags	= RND_Editor;
		Hide();
		Explorer->Refresh();
		GEditor->OpenPageWith( Level );
	}
}	*GLevelConstructor;


/*-----------------------------------------------------------------------------
	WLevelsList implementation
-----------------------------------------------------------------------------*/

//
// List of levels.
//
class WLevelsList: public WListBox
{
public:
	// Variables.
	WPopupMenu*			Popup;
	WProjectExplorer*	Explorer;

	// WLevelsList interface.
	WLevelsList( WProjectExplorer* InOwner, WWindow* InRoot )
		:	WListBox( InOwner, InRoot ),
			Explorer( InOwner )
	{
		Popup			= new WPopupMenu( InRoot, InRoot );
		Popup->AddItem( L"Open Level", WIDGET_EVENT(WLevelsList::PopOpenClick) );
		Popup->AddItem( L"" );
		Popup->AddItem( L"Rename...", WIDGET_EVENT(WLevelsList::PopRenameClick) );
		Popup->AddItem( L"Show Properties", WIDGET_EVENT(WLevelsList::PopShowPropertiesClick) );
		Popup->AddItem( L"" );
		Popup->AddItem( L"Delete Level", WIDGET_EVENT(WLevelsList::PopDeleteClick) );
	}
	~WLevelsList()
	{
		delete Popup;
	}

	// WWidget interface.
	void OnMouseUp( EMouseButton Button, Integer X, Integer Y )
	{
		WListBox::OnMouseUp( Button, X, Y );
		if( Button == MB_Right && ItemIndex != -1 )
			Popup->Show(Root->MousePos);
	}

	// Popup events.
	void PopOpenClick( WWidget* Sender )
	{
		Explorer->ListLevelsDblClick( Sender );
	}
	void PopRenameClick( WWidget* Sender )
	{
		if( ItemIndex != -1 )
			GRenameDialog->SetResource((FLevel*)Items[ItemIndex].Data);	
	}
	void PopShowPropertiesClick( WWidget* Sender )
	{
		Explorer->ListLevelsChange( Sender );
	}
	void PopDeleteClick( WWidget* Sender )
	{
		Explorer->ButtonDeleteLevelClick( Sender );
	}
};


/*-----------------------------------------------------------------------------
    WProjectExplorer implementation.
-----------------------------------------------------------------------------*/

//
// Explorer constructor.
//
WProjectExplorer::WProjectExplorer( WContainer* InOwner, WWindow* InRoot )
	:	WPanel( InOwner, InRoot )
{
	// Initialize own fields.
	Caption		= L"Project Explorer";
	Padding		= TArea( FORM_HEADER_SIZE, 0, 0, 0 );

	// Create controls.
	HeaderPanel				= new WPanel( this, Root );
	HeaderPanel->Align		= AL_Top;
	HeaderPanel->bDrawEdges	= true;
	HeaderPanel->SetSize( 50, 26 );

	AddLevelButton				= new WPictureButton( HeaderPanel, Root );
	AddLevelButton->Tooltip		= L"Create New Level";
	AddLevelButton->Picture		= Root->Icons;
	AddLevelButton->Offset		= TPoint( 0, 128 );
	AddLevelButton->Scale		= TSize( 16, 16 );
	AddLevelButton->Location	= TPoint( 2, 2 );
	AddLevelButton->EventClick	= WIDGET_EVENT(WProjectExplorer::ButtonAddLevelClick);
	AddLevelButton->SetSize( 22, 22 );

	ProjectPropsButton				= new WPictureButton( HeaderPanel, Root );
	ProjectPropsButton->Tooltip		= L"Show Project Properties";
	ProjectPropsButton->Picture		= Root->Icons;
	ProjectPropsButton->Offset		= TPoint( 16, 128 );
	ProjectPropsButton->Scale		= TSize( 16, 16 );
	ProjectPropsButton->Location	= TPoint( 23, 2 );
	ProjectPropsButton->EventClick	= WIDGET_EVENT(WProjectExplorer::ButtonProjectPropsClick);
	ProjectPropsButton->SetSize( 22, 22 );

	DeleteLevelButton				= new WPictureButton( HeaderPanel, Root );
	DeleteLevelButton->Tooltip		= L"Delete Level";
	DeleteLevelButton->Picture		= Root->Icons;
	DeleteLevelButton->Offset		= TPoint( 32, 128 );
	DeleteLevelButton->Scale		= TSize( 16, 16 );
	DeleteLevelButton->Location		= TPoint( Size.Width-22-8, 2 );
	DeleteLevelButton->EventClick	= WIDGET_EVENT(WProjectExplorer::ButtonDeleteLevelClick);
	DeleteLevelButton->SetSize( 22, 22 );

	// List of levels.
	LevelsList						= new WLevelsList( this, Root );
	LevelsList->Margin				= TArea( -1, 0, 0, 0 );
	LevelsList->Align				= AL_Client;
	LevelsList->ItemsHeight			= 20;
	LevelsList->EventChange			= WIDGET_EVENT(WProjectExplorer::ListLevelsChange);
	LevelsList->EventDblClick		= WIDGET_EVENT(WProjectExplorer::ListLevelsDblClick);

	// Level constructor.
	GLevelConstructor		= new WLevelConstructor( this, Root );
}

//
// Open a level constructor.
//
void WProjectExplorer::ButtonAddLevelClick( WWidget* Sender )
{
	if( !GProject )
		return;

	// Open dialog.
	GLevelConstructor->Show
						( 
							Root->Size.Width/2, 
							Root->Size.Height/3  
						);
}


//
// When level item changed.
//
void WProjectExplorer::ListLevelsChange( WWidget* Sender )
{
	if( LevelsList->ItemIndex != -1 )
	{
		FLevel* Level = As<FLevel>((FObject*)LevelsList->Items[LevelsList->ItemIndex].Data);
		GEditor->Inspector->SetEditObject( Level );
	}
}


//
// When delete button clicked.
//
void WProjectExplorer::ButtonDeleteLevelClick( WWidget* Sender )
{
	if( LevelsList->ItemIndex != -1 )
	{
		FLevel* Level = (FLevel*)LevelsList->Items[LevelsList->ItemIndex].Data;

		Root->AskYesNo
		(
			*String::Format( L"Do you really want to destroy level '%s'?", *Level->GetName() ),
			L"Project Explorer",
			true,
			WIDGET_EVENT(WProjectExplorer::MessageDeleteYesClick)
		);
	}
}


//
// User click yes, when destroy level.
//
void WProjectExplorer::MessageDeleteYesClick( WWidget* Sender )
{
	Root->Modal->OnClose();

	FLevel* Level = (FLevel*)LevelsList->Items[LevelsList->ItemIndex].Data;

	DestroyObject( Level, true );
	Refresh();
}


//
// Count all references.
//
void WProjectExplorer::CountRefs( CSerializer& S )
{
	Bool bRefresh = false;

	for( Integer i=0; i<LevelsList->Items.Num(); i++ )
	{
		FLevel* Level = (FLevel*)LevelsList->Items[i].Data;
		Serialize( S, Level );
		if( !Level )
		{
			// Found with bad reference.
			bRefresh	= true;
			break;
		}
	}

	if( bRefresh )
		Refresh();
}


//
// When double clicked on item.
//
void WProjectExplorer::ListLevelsDblClick( WWidget* Sender )
{
	if( LevelsList->ItemIndex != -1 )
	{
		// Open page.
		GEditor->OpenPageWith
						(
							(FLevel*)LevelsList->Items[LevelsList->ItemIndex].Data
						);
	}
}


//
// Show project properties.
//
void WProjectExplorer::ButtonProjectPropsClick( WWidget* Sender )
{
	assert(GProject->Info);
	GEditor->Inspector->SetEditObject( GProject->Info );
}


//
// Refresh the list of levels.
//
void WProjectExplorer::Refresh()
{
	LevelsList->Empty();

	if( !GProject )
		return;

	// Collect all levels.
	for( Integer i=0; i<GProject->GObjects.Num(); i++ )
		if	(	
				GProject->GObjects[i] && 
				GProject->GObjects[i]->IsA(FLevel::MetaClass) 
			)
		{
			FLevel* Level = (FLevel*)GProject->GObjects[i];
			if( !Level->IsTemporal() )
				LevelsList->AddItem( Level->GetName(), Level );
		}
}


//
// Redraw an explorer.
//
void WProjectExplorer::OnPaint( CGUIRenderBase* Render )
{
	WPanel::OnPaint( Render );
	TPoint Base = ClientToWindow(TPoint::Zero);
	
	// Draw header.
	Render->DrawRegion
					(
						Base, 
						TSize( Size.Width, FORM_HEADER_SIZE ),
						TColor( 0x33, 0x33, 0x33, 0xff ),
						GUI_COLOR_FORM_BORDER,
						BPAT_Diagonal
					);

	Render->DrawText
				( 
					TPoint( Base.X + 5, Base.Y+(FORM_HEADER_SIZE-Root->Font1->Height)/2 ), 
					Caption, 
					GUI_COLOR_TEXT, 
					Root->Font1 
				);

	// Turn on or turn off buttons.
	DeleteLevelButton->bEnabled		= GProject != nullptr && LevelsList->ItemIndex != -1;
	AddLevelButton->bEnabled		= GProject != nullptr;
	ProjectPropsButton->bEnabled	= GProject != nullptr;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/