/*=============================================================================
    FrResBrowser.cpp: A resource browser panel.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "Editor.h"
#include "FrMusPlay.h"

/*-----------------------------------------------------------------------------
    Resource browser colors.
-----------------------------------------------------------------------------*/

#define COLOR_RESBROW_ICON				TColor( 0x25, 0xff, 0xff, 0xff )
#define COLOR_RESBROW_ICON_PAD			TColor( 0x30, 0x30, 0x30, 0xff )
#define COLOR_RESBROW_ICON_BORDER		TColor( 0xc8, 0xc8, 0xc8, 0xff )
#define COLOR_RESBROW_HIGHLIGHT			TColor( 0xff, 0xba, 0x40, 0xff )


/*-----------------------------------------------------------------------------
    WBrowserPanel implementation.
-----------------------------------------------------------------------------*/

//
// Default size of the resource icon.
//
#define RES_ICON_SIZE	64


//
// Browser panel constructor.
//
WBrowserPanel::WBrowserPanel( WResourceBrowser* InBrowser, WWindow* InRoot )
	:	WPanel( InBrowser, InRoot ),
		Browser( InBrowser ),
		Selected( nullptr ),
		bReadyDrag( false ),
		ClassFilter( nullptr ),
		Icons()
{
	// Own variables.
	SetSize( 187, 256 );

	// Create scrollbar.
	ScrollBar				= new WSlider( this, InRoot );
	ScrollBar->Orientation	= SLIDER_Vertical;
	ScrollBar->Align		= AL_Right;
	ScrollBar->EventChange	= WIDGET_EVENT(WBrowserPanel::ScrollChange);
	ScrollBar->SetSize( 12, 50 );

	// Popup.
	Popup					= new WPopupMenu( Root, Root );
	Popup->AddItem( L"Edit",		WIDGET_EVENT(WBrowserPanel::PopEditClick) );
	Popup->AddItem( L"Rename...",	WIDGET_EVENT(WBrowserPanel::PopRenameClick) );
	Popup->AddItem( L"" );
	Popup->AddItem( L"Remove",		WIDGET_EVENT(WBrowserPanel::PopRemoveClick) );

	// Panel variables.
	Align					= AL_Client;
	Padding					= TArea( 0, 0, 0, 0 );
	bDrawEdges				= true;
}


//
// Browser panel destructor.
//
WBrowserPanel::~WBrowserPanel()
{
	Icons.Empty();

	// Since popup is not my own!
	delete Popup;
}


//
// Update resources list.
//
void WBrowserPanel::RefreshList()
{
	// No project.
	if( !GProject )
	{
		Icons.Empty();
		return;
	}

	// Precompute.
	String	NameFil	= String::UpperCase( Browser->NameFilter->Text );
	TArray<String> FoundGroups;
	FoundGroups.Push( L"" );
	Selected	= nullptr;

	// Build list of bitmaps.
	Icons.Empty();

	// For each object.
	for( Integer i=0; i<GProject->GObjects.Num(); i++ )
		if( GProject->GObjects[i] && GProject->GObjects[i]->IsA(FResource::MetaClass) )
		{
			FResource*	Res = (FResource*)GProject->GObjects[i];

			// Reject some resources.
			if( Res->IsA(FLevel::MetaClass) || Res->IsA(FProjectInfo::MetaClass) )
				continue;

			// Collect groups.
			FoundGroups.AddUnique( Res->Group );

			// Apply name filter.
			if( NameFil )
			{						
				if( String::Pos(NameFil, String::UpperCase(Res->GetName())) == -1 )
					continue;
			}

			// Apply group filter.
			if( SelGroup )
				if( String::UpperCase(Res->Group) != String::UpperCase(SelGroup) )
					continue;

			// Class filter.
			if( ClassFilter )
			{
				if( ClassFilter->IsA(FFont::MetaClass) )
				{
					// Show fonts and it bitmaps.
					if( !(Res->IsA(FFont::MetaClass) || (Res->GetOwner() && Res->GetOwner()->IsA(FFont::MetaClass))) )
						continue;
				}
				else
				{
					// Not font filter.
					if( !Res->IsA(ClassFilter) )
						continue;
				}
			}

			// Don't show folded resource, except font pages only.
			if( Res->GetOwner() && !(ClassFilter && ClassFilter->IsA(FFont::MetaClass)) )
				continue;

			// Initialize icon.
			TResourceIcon Icon;
			Icon.Position	= TPoint::Zero;		
			Icon.Resource	= Res;
			Icon.Picture	= nullptr;
			Icon.TypeName	= L"[BadRes]";

			if( Res->IsA(FBitmap::MetaClass) )
			{
				//
				// Draw bitmap as bitmap, of course.
				//
				Icon.Picture	= As<FBitmap>(Res);
				Icon.TypeName	= (Icon.Picture->USize==256 && Icon.Picture->VSize==1) ? L"[Palette]" : L"[Bitmap]";
				Icon.SrcPos		= TPoint( 0, 0 );
				Icon.SrcScale	= TSize( Icon.Picture->USize, Icon.Picture->VSize );

				// Compute new scale, keep aspect ratio.
				Integer Scale = Max( 1, Max( Icon.Picture->USize, Icon.Picture->VSize )/RES_ICON_SIZE );
				Icon.DstScale.Width		= Max( 1, Icon.Picture->USize/Scale );
				Icon.DstScale.Height	= Max( 1, Icon.Picture->VSize/Scale );
			}
			else if( Res->IsA(FAnimation::MetaClass) )
			{
				//
				// Draw first frame of animation.
				//
				FAnimation* Anim	= As<FAnimation>(Res);
				Icon.TypeName		= L"[Anim]";
				if( Anim->Sheet && Anim->Frames.Num() && Anim->Sequences.Num() )
				{
					// Valid animation frame.
					TRect Frame = Anim->GetTexCoords(Anim->Sequences[0].Start);
					Icon.Picture			= Anim->Sheet;

					Icon.SrcPos.X			= Integer( (Float)Anim->Sheet->USize*Frame.Min.X );
					Icon.SrcPos.Y			= Integer( (Float)Anim->Sheet->VSize*Frame.Max.Y );
					Icon.SrcScale.Width		= Integer( (Float)Anim->Sheet->USize*(Frame.Max.X-Frame.Min.X) );
					Icon.SrcScale.Height	= Integer( (Float)Anim->Sheet->VSize*(Frame.Min.Y-Frame.Max.Y) );

					Float Scale = Max( 1, Max( Icon.SrcScale.Width, Icon.SrcScale.Height )/RES_ICON_SIZE );
					Icon.DstScale.Width		= Max<Integer>( 1, Icon.SrcScale.Width/Scale );
					Icon.DstScale.Height	= Max<Integer>( 1, Icon.SrcScale.Height/Scale );
				}
				else
				{
					// Bad animation.
					Icon.Picture			= FBitmap::Default;
					Icon.SrcPos				= TPoint( 0, 0 );
					Icon.SrcScale			= TSize( Icon.Picture->USize, Icon.Picture->VSize );
					Icon.DstScale			= TSize( 32, 32 );
				}
			}
			else if( Res->IsA(FScript::MetaClass) )
			{
				//
				// Draw script resource as Jigsaw.
				//
				FScript* Script = As<FScript>(Res);
				Icon.TypeName	= L"[Script]";
				Icon.Picture	= Root->Icons;
				Icon.SrcPos		= TPoint( 0, 256-96 );
				Icon.SrcScale	= TSize( 32, 32 );
				Icon.DstScale	= TSize( 32, 32 );

				// Maybe draw as billboard.
				for( Integer e=0; e<Script->Components.Num(); e++ )
				{
					FExtraComponent* Com = Script->Components[e];
					if( Com->IsA(FAnimatedSpriteComponent::MetaClass) )
					{
						// Draw as animation.
						FAnimation* Anim	= As<FAnimatedSpriteComponent>(Com)->Animation;
						if( Anim && Anim->Sheet && Anim->Frames.Num() && Anim->Sequences.Num() )
						{
							TRect Frame = Anim->GetTexCoords(Anim->Sequences[0].Start);
							Icon.Picture			= Anim->Sheet;

							Icon.SrcPos.X			= Integer( (Float)Anim->Sheet->USize*Frame.Min.X );
							Icon.SrcPos.Y			= Integer( (Float)Anim->Sheet->VSize*Frame.Max.Y );
							Icon.SrcScale.Width		= Integer( (Float)Anim->Sheet->USize*(Frame.Max.X-Frame.Min.X) );
							Icon.SrcScale.Height	= Integer( (Float)Anim->Sheet->VSize*(Frame.Min.Y-Frame.Max.Y) );

							Float Scale = Max( 1, Max( Icon.SrcScale.Width, Icon.SrcScale.Height )/RES_ICON_SIZE );
							Icon.DstScale.Width		= Max<Integer>( 1, Icon.SrcScale.Width/Scale );
							Icon.DstScale.Height	= Max<Integer>( 1, Icon.SrcScale.Height/Scale );
							break;
						}
						else
							goto NoSprite;
					}
					else if( Com->IsA(FSpriteComponent::MetaClass) )
					{
						// Single sprite.
						FSpriteComponent* Sprite = As<FSpriteComponent>(Com);
						if( Sprite->Bitmap )
						{
							TRect	Frame			= Sprite->TexCoords;
							Icon.Picture			= Sprite->Bitmap;

							Icon.SrcPos.X			= Integer( Frame.Min.X );
							Icon.SrcPos.Y			= Integer( Frame.Min.Y );
							Icon.SrcScale.Width		= Integer( (Frame.Max.X-Frame.Min.X) );
							Icon.SrcScale.Height	= Integer( (Frame.Max.Y-Frame.Min.Y) );

							Float Scale = Max( 1, Max( Icon.SrcScale.Width, Icon.SrcScale.Height )/RES_ICON_SIZE );
							Icon.DstScale.Width		= Max<Integer>( 1, Icon.SrcScale.Width/Scale );
							Icon.DstScale.Height	= Max<Integer>( 1, Icon.SrcScale.Height/Scale );
							break;
						}
						else
							goto NoSprite;
					}			
					else if( Com->IsA(FDecoComponent::MetaClass) )
					{
						// Deco sprite.
						FDecoComponent* Sprite = As<FDecoComponent>(Com);
						if( Sprite->Bitmap )
						{
							TRect	Frame			= Sprite->TexCoords;
							Icon.Picture			= Sprite->Bitmap;

							Icon.SrcPos.X			= Integer( (Float)Icon.Picture->USize*Frame.Min.X );
							Icon.SrcPos.Y			= Integer( (Float)Icon.Picture->VSize*Frame.Max.Y );
							Icon.SrcScale.Width		= Integer( (Float)Icon.Picture->USize*(Frame.Max.X-Frame.Min.X) );
							Icon.SrcScale.Height	= Integer( (Float)Icon.Picture->VSize*(Frame.Min.Y-Frame.Max.Y) );

							Float Scale = Max( 1, Max( Icon.SrcScale.Width, Icon.SrcScale.Height )/RES_ICON_SIZE );
							Icon.DstScale.Width		= Max<Integer>( 1, Icon.SrcScale.Width/Scale );
							Icon.DstScale.Height	= Max<Integer>( 1, Icon.SrcScale.Height/Scale );
							break;
						}
						else
							goto NoSprite;
					}	
					continue;

					{
						// No sprite.
					NoSprite:;
						Icon.Picture			= FBitmap::Default;
						Icon.SrcPos				= TPoint( 0, 0 );
						Icon.SrcScale			= TSize( Icon.Picture->USize, Icon.Picture->VSize );
						Icon.DstScale			= TSize( 32, 32 );
						break;
					}
				}
			}
			else if( Res->IsA(FSound::MetaClass) )
			{
				//
				// Draw sound resource as Speaker.
				//
				Icon.TypeName	= L"[Sound]";
				Icon.Picture	= Root->Icons;
				Icon.SrcPos		= TPoint( 32, 256-96 );
				Icon.SrcScale	= TSize( 32, 32 );
				Icon.DstScale	= TSize( 32, 32 );
			}
			else if( Res->IsA(FMusic::MetaClass) )
			{
				//
				// Draw music resource as AudioTape.
				//
				Icon.TypeName	= L"[Music]";
				Icon.Picture	= Root->Icons;
				Icon.SrcPos		= TPoint( 64, 256-96 );
				Icon.SrcScale	= TSize( 32, 32 );
				Icon.DstScale	= TSize( 32, 32 );
			}
			else if( Res->IsA(FFont::MetaClass) )
			{
				//
				// Draw font as first page.
				//
				FFont* Font		= As<FFont>(Res);
				Icon.TypeName	= L"[Font]";
				Icon.Picture	= Font->Bitmaps[0];
				Icon.SrcPos		= TPoint( 0, 0 );
				Icon.SrcScale	= TSize( GLYPHS_ATLAS_SIZE, GLYPHS_ATLAS_SIZE );
				Icon.DstScale	= TSize( 64, 64 );
			}

			Icons.Push(Icon);	
		}

	// Alphabet sorting.
	Icons.Sort(ResourceIconCmp);

	// Update group combo.
	Browser->GroupFilter->Empty();
	for( Integer i=0; i<FoundGroups.Num(); i++ )
	{
		if( String::UpperCase(FoundGroups[i]) == String::UpperCase(SelGroup) ) 
			Browser->GroupFilter->ItemIndex	= i;
		Browser->GroupFilter->AddItem( FoundGroups[i], nullptr );
	}
	Browser->GroupFilter->ItemIndex	= Max( 0, Browser->GroupFilter->ItemIndex );

	// Let's scroll it.
	ScrollChange( this );
}


//
// Render all icons.
//
void WBrowserPanel::OnPaint( CGUIRenderBase* Render )
{
	WPanel::OnPaint( Render );
	TPoint Base = ClientToWindow(TPoint::Zero);

	// Clip icons.
	Render->SetClipArea
	(
		TPoint( Base.X, Base.Y+1 ), 
		TSize( Size.Width, Size.Height-2 )
	);

	// Iterate through all icons.
	for( Integer i=0; i<Icons.Num(); i++ )
	{
		TResourceIcon& Icon = Icons[i];

		// Visibility test.
		if( Icon.Position.Y <= -80 )		continue;
		if( Icon.Position.Y > Size.Height )	continue;

		// Draw icon frame.
		Render->DrawRegion
		( 
			TPoint( Base.X+Icon.Position.X-1, Base.Y+Icon.Position.Y-1 ), 
			TSize( RES_ICON_SIZE+2, RES_ICON_SIZE+2 ), 
			COLOR_RESBROW_ICON_PAD,
			Selected == Icon.Resource ? COLOR_RESBROW_HIGHLIGHT : COLOR_RESBROW_ICON_BORDER,
			BPAT_Solid 
		);

		// Icon label.
		Render->DrawText
		(			
			TPoint( Base.X+Icon.Position.X+1, Base.Y+Icon.Position.Y+RES_ICON_SIZE+1), 
			Icon.Resource->GetName(), 
			Selected == Icon.Resource ? COLOR_RESBROW_HIGHLIGHT : GUI_COLOR_TEXT, 
			Root->Font1 
		);

		// Draw picture.
		if( Icon.Picture )
		{
			// Temporally switch style for root icons.
			if( Icon.Picture == Root->Icons )
				Icon.Picture->BlendMode	= BLEND_Translucent;

			// Draw it.
			Render->DrawPicture
			( 
				Base + Icon.Position + TPoint( (RES_ICON_SIZE-Icon.DstScale.Width)/2, (RES_ICON_SIZE-Icon.DstScale.Height)/2 ),
				Icon.DstScale, 
				Icon.SrcPos, 
				Icon.SrcScale, 
				Icon.Picture
			);

			// Restore root icons default style.
			if( Icon.Picture == Root->Icons )
				Icon.Picture->BlendMode	= BLEND_Masked;
		}

		// Draw overlay label.
		Integer Width = Root->Font1->TextWidth( Icon.TypeName );
		Render->DrawText
		( 
			Base + Icon.Position + TPoint( RES_ICON_SIZE/2-Width/2, 47 ),
			Icon.TypeName, 
			Selected == Icon.Resource ? COLOR_RESBROW_HIGHLIGHT*0.85f : GUI_COLOR_TEXT, 
			Root->Font1 
		);	
	}
}


//
// Popup 'Edit' clicked.
//
void WBrowserPanel::PopEditClick( WWidget* Sender )
{
	if( Selected )
		GEditor->OpenPageWith( Selected );
}


//
// Popup 'Rename' clicked.
//
void WBrowserPanel::PopRenameClick( WWidget* Sender )
{
	if( Selected )
		GRenameDialog->SetResource( Selected );
}


//
// Popup 'Remove' clicked.
//
void WBrowserPanel::PopRemoveClick( WWidget* Sender )
{
	Browser->ButtonRemoveClick( Sender );
}


//
// User click on resource panel.
//
void WBrowserPanel::OnMouseDown( EMouseButton Button, Integer X, Integer Y )
{
	WPanel::OnMouseDown( Button, X, Y );
	
	if( Button == MB_Left || Button == MB_Right )
	{
		// Prepare for drag.
		bReadyDrag	= true;

		// Get clicked resource.
		FResource* Res = nullptr;
		for( Integer i=0; i<Icons.Num(); i++ )
		{
			TResourceIcon& Icon = Icons[i];

			if	(
					X >= Icon.Position.X &&
					Y >= Icon.Position.Y &&
					X < Icon.Position.X+RES_ICON_SIZE &&
					Y < Icon.Position.Y+RES_ICON_SIZE
				)
			{
				// Gotcha.
				Res	= Icon.Resource;
				break;
			}
		}

		// Select it!
		Selected	= Res;
	}

	// Popup it!
	if( Button == MB_Right && Selected )
	{
		bReadyDrag	= false;
		Popup->Show(Root->MousePos);
		Popup->Items[0].bEnabled	= !Selected->IsA(FSound::MetaClass) && !Selected->IsA(FFont::MetaClass);
	}
}


//
// User hover mouse above panel.
//
void WBrowserPanel::OnMouseMove( EMouseButton Button, Integer X, Integer Y )
{
	WPanel::OnMouseMove( Button, X, Y );

	if( bReadyDrag && Selected )
	{
		// Start drag object.
		BeginDrag( Selected );
		bReadyDrag	= false;
	}
}


//
// User just release mouse button.
//
void WBrowserPanel::OnMouseUp( EMouseButton Button, Integer X, Integer Y )
{
	WPanel::OnMouseUp( Button, X, Y );
	bReadyDrag = false;
}


//
// Scroll resource icons.
//
void WBrowserPanel::ScrollChange( WWidget* Sender )
{
	// Prepare for walking.
	Integer XCount	= (Size.Width - 16) / (RES_ICON_SIZE + 16);
	Integer	XWalk	= 16;
	Integer	YWalk	= 16;

	// Walk through all icons and compute initial location,
	// without scroll yet.
	for( Integer i=0; i<Icons.Num(); i++ )
	{
		TResourceIcon& Icon = Icons[i];

		Icon.Position.X	= XWalk;
		Icon.Position.Y	= YWalk;

		// Iterate to next.
		XWalk += RES_ICON_SIZE + 16;
		if( XWalk+64 > Size.Width )
		{
			XWalk	= 16;
			YWalk	+= RES_ICON_SIZE + 24;
		}
	}

	// Apply scroll for each icon.
	Integer YScroll = Max( 0, YWalk-RES_ICON_SIZE-30 )*ScrollBar->Value / 100;
	for( Integer i=0; i<Icons.Num(); i++ )
		Icons[i].Position.Y -= YScroll;
}


//
// Scroll browser via mouse wheel.
//
void WBrowserPanel::OnMouseScroll( Integer Delta )
{
	ScrollBar->Value	= Clamp
							( 
								ScrollBar->Value-Delta/120, 
								0, 
								100 
							);
	ScrollChange( this );
}


//
// Serialize resource icon, only refs
// counting.
//
void Serialize( CSerializer& S, WBrowserPanel::TResourceIcon& V )
{
	Serialize( S, V.Resource );
}


//
// Count references and remove unused.
//
void WBrowserPanel::CountRefs( CSerializer& S )
{
	// Serialize refs.
	Serialize( S, Selected );
	Serialize( S, Icons );

	// Detect whether changed some icon.
	Bool bChanged = false;
	for( Integer i=0; i<Icons.Num(); i++ )
		if( Icons[i].Resource == nullptr )
		{
			bChanged	= true;
			break;
		}

	// Update if was changed.
	if( bChanged )
		RefreshList();
}	


/*-----------------------------------------------------------------------------
    WDemoEffectBuilder implementation.
-----------------------------------------------------------------------------*/

//
// Demo effect builder form.
//
class WDemoEffectBuilder: public WForm
{
public:
	// Variables.
	WResourceBrowser*	Browser;
	WComboBox*			ClassCombo;		
	WEdit*				NameEdit;
	WEdit*				GroupEdit;
	WComboBox*			WidthCombo;
	WComboBox*			HeightCombo;
	WButton*			OkButton;
	WLabel*				ClassLabel;
	WLabel*				NameLabel;
	WLabel*				GroupLabel;
	WLabel*				WidthLabel;
	WLabel*				HeightLabel;

	// WDemoEffectBuilder interface.
	WDemoEffectBuilder( WResourceBrowser* InBrowser, WWindow* InRoot )	
		:	WForm( InRoot, InRoot ),
			Browser( InBrowser )
	{
		// Initialize form.
		SetSize( 220, 175 );
		Caption		= L"Demo Effect Builder";
		bCanClose	= true;
		bSizeableH	= false;
		bSizeableW	= false;

		// Create controls.
		ClassLabel				= new WLabel( this, InRoot );
		ClassLabel->Caption		= L"Class: ";
		ClassLabel->Location	= TPoint( 10, 30 );

		ClassCombo				= new WComboBox( this, InRoot );	
		ClassCombo->Location	= TPoint( 50, 29 );
		ClassCombo->SetSize( 160, 18 );
		for( Integer i=0; i<CClassDatabase::GClasses.Num(); i++ )
		{
			CClass*	Class	= CClassDatabase::GClasses[i];
			if( Class->IsA(FDemoBitmap::MetaClass) && !(Class->Flags & CLASS_Abstract) )
				ClassCombo->AddItem( Class->Alt, Class );
		}
		ClassCombo->AlphabetSort();
		ClassCombo->ItemIndex	= 0;
		ClassCombo->EventChange	= WIDGET_EVENT(WDemoEffectBuilder::ComboClassChange);

		NameLabel				= new WLabel( this, InRoot );
		NameLabel->Caption		= L"Name: ";
		NameLabel->Location		= TPoint( 10, 54 );

		NameEdit				= new WEdit( this, InRoot );
		NameEdit->Location		= TPoint( 50, 53 );
		NameEdit->SetSize( 160, 18 );
		NameEdit->EditType		= EDIT_String;

		GroupLabel				= new WLabel( this, InRoot );
		GroupLabel->Caption		= L"Group: ";
		GroupLabel->Location	= TPoint( 10, 78 );

		GroupEdit				= new WEdit( this, InRoot );
		GroupEdit->Location		= TPoint( 50, 77 );
		GroupEdit->SetSize( 160, 18 );
		GroupEdit->EditType		= EDIT_String;

		WidthLabel				= new WLabel( this, InRoot );
		WidthLabel->Caption		= L"Width: ";
		WidthLabel->Location	= TPoint( 30, 104 );

		HeightLabel				= new WLabel( this, InRoot );
		HeightLabel->Caption	= L"Height: ";
		HeightLabel->Location	= TPoint( 120, 104 );

		WidthCombo				= new WComboBox( this, InRoot );
		WidthCombo->Location	= TPoint( 20, 120 );
		WidthCombo->SetSize( 64, 18 );

		HeightCombo				= new WComboBox( this, InRoot );
		HeightCombo->Location	= TPoint( 110, 120 );
		HeightCombo->SetSize( 64, 18 );

		for( Integer i=5; i<=8; i++ )
		{
			WidthCombo->AddItem( String::Format( L"%d", 1 << i ), nullptr );
			HeightCombo->AddItem( String::Format( L"%d", 1 << i ), nullptr );
		}

		WidthCombo->ItemIndex	= 3;
		HeightCombo->ItemIndex	= 3;

		OkButton				= new WButton( this, Root );
		OkButton->EventClick	= WIDGET_EVENT(WDemoEffectBuilder::ButtonOkClick);
		OkButton->Caption		= L"Ok";
		OkButton->SetSize( 200, 20 );
		OkButton->Location		= TPoint( 8, 147 );

		Hide();
	}

	// WForm interface.
	void Show( Integer X = 0, Integer Y = 0 )
	{
		WForm::Show( X, Y );
		NameEdit->Text		= L"";
		GroupEdit->Text		= Browser->BrowserPanel->SelGroup;
	}
	void OnClose()
	{
		WForm::OnClose();
		Hide();
	}

	// Controls notifications.
	void ComboClassChange( WWidget* Sender )
	{
	}
	void ButtonOkClick( WWidget* Sender )
	{
		if( NameEdit->Text.Len() == 0 )
		{
			Root->ShowMessage( L"Please specify the name of effect", L"Effect Builder", true );
			return;
		}
		if( GObjectDatabase->FindObject( NameEdit->Text ) )
		{
			Root->ShowMessage
			(
				String::Format( L"Object \"%s\" already exists", *NameEdit->Text ),
				L"Effect Builder",
				true
			);	
			return;
		}

		CClass* DemoClass	= (CClass*)ClassCombo->Items[ClassCombo->ItemIndex].Data;
		assert(DemoClass);
		DWord	USize		= 1 << (WidthCombo->ItemIndex + 5);
		DWord	VSize		= 1 << (HeightCombo->ItemIndex + 5);
		assert(USize>=2 && USize<=256);
		assert(VSize>=2 && VSize<=256);

		FBitmap* Bitmap		= NewObject<FBitmap>( DemoClass, NameEdit->Text, nullptr );
		Bitmap->Init( USize, VSize );
		Bitmap->Group		= GroupEdit->Text;

		Hide();
		Browser->RefreshList();
		GEditor->OpenPageWith( Bitmap );
	}
}	*GDemoEffectBuilderForm;


/*-----------------------------------------------------------------------------
    WScriptBuilder implementation.
-----------------------------------------------------------------------------*/

//
// Script builder form.
//
class WScriptBuilder: public WForm
{
public:
	// Variables.
	WResourceBrowser*	Browser;
	WLabel*				NameLabel;
	WLabel*				BaseLabel;
	WLabel*				GroupLabel;
	WLabel*				ClassLabel;
	WPanel*				ExtraPanel;
	WCheckBox*			HasTextCheck;
	WButton*			OkButton;
	WEdit*				NameEdit;
	WEdit*				GroupEdit;
	WComboBox*			BaseCombo;
	WListBox*			ExtraList;
	WListBox*			ExtraUsedList;
	WButton*			AddExtraButton;
	WButton*			RemoveExtraButton;
	WEdit*				ExtraNameEdit;

	// WScriptBuilder interface.
	WScriptBuilder( WResourceBrowser* InBrowser, WWindow* InRoot )
		:	WForm( InRoot, InRoot ),
			Browser( InBrowser )
	{
		// Initialize the form.
		Caption		= L"Script Builder";
		bCanClose	= true;
		bSizeableH	= false;
		bSizeableW	= false;
		SetSize( 445, 404 );

		NameLabel				= new WLabel( this, InRoot );
		NameLabel->Caption		= L"Name: ";
		NameLabel->Location		= TPoint( 100, 30 );

		GroupLabel				= new WLabel( this, InRoot );
		GroupLabel->Caption		= L"Group: ";
		GroupLabel->Location	= TPoint( 100, 54 );

		GroupEdit				= new WEdit( this, InRoot );
		GroupEdit->Location		= TPoint( 145, 53 );
		GroupEdit->EditType		= EDIT_String;
		GroupEdit->SetSize( 160, 18 );

		BaseLabel				= new WLabel( this, InRoot );
		BaseLabel->Caption		= L"Base: ";
		BaseLabel->Location		= TPoint( 100, 78 );

		BaseCombo				= new WComboBox( this, InRoot );	
		BaseCombo->Location		= TPoint( 145, 77 );
		BaseCombo->SetSize( 160, 18 );
		for( Integer i=0; i<CClassDatabase::GClasses.Num(); i++ )
		{
			CClass*	Class	= CClassDatabase::GClasses[i];
			if( Class->IsA(FBaseComponent::MetaClass) && !(Class->Flags & CLASS_Abstract) )
				BaseCombo->AddItem( Class->Alt, Class );
		}
		BaseCombo->AlphabetSort();
		BaseCombo->ItemIndex	= -1;

		NameEdit				= new WEdit( this, InRoot );
		NameEdit->Location		= TPoint( 145, 29 );
		NameEdit->EditType		= EDIT_String;
		NameEdit->SetSize( 160, 18 );

		ExtraPanel				= new WPanel( this, InRoot );
		ExtraPanel->Location	= TPoint( 10, 107 );
		ExtraPanel->SetSize( 425, 256 );	

		ExtraList				= new WListBox( ExtraPanel, InRoot );
		ExtraList->Location		= TPoint( 10, 10 );
		ExtraList->SetSize( 180, 200 );
		for( Integer i=0; i<CClassDatabase::GClasses.Num(); i++ )
		{
			CClass*	Class	= CClassDatabase::GClasses[i];
			if( Class->IsA(FExtraComponent::MetaClass) && !(Class->Flags & CLASS_Abstract) )
				ExtraList->AddItem( Class->Alt, Class );
		}
		ExtraList->AlphabetSort();
		ExtraList->ItemIndex		= -1;	
		ExtraList->EventChange		= WIDGET_EVENT(WScriptBuilder::ListExtraChange);
		ExtraList->EventDblClick	= WIDGET_EVENT(WScriptBuilder::ButtonAddExtraClick);

		AddExtraButton				= new WButton( ExtraPanel, InRoot );
		AddExtraButton->Caption		= L">>";
		AddExtraButton->Tooltip		= L"Add Extra Component";
		AddExtraButton->Location	= TPoint( 200, 90 );
		AddExtraButton->EventClick	= WIDGET_EVENT(WScriptBuilder::ButtonAddExtraClick);
		AddExtraButton->SetSize( 25, 22 );

		RemoveExtraButton				= new WButton( ExtraPanel, InRoot );
		RemoveExtraButton->Caption		= L"<<";
		RemoveExtraButton->Tooltip		= L"Remove Extra Component";
		RemoveExtraButton->Location		= TPoint( 200, 120 );
		RemoveExtraButton->EventClick	= WIDGET_EVENT(WScriptBuilder::ButtonRemoveExtraClick);
		RemoveExtraButton->SetSize( 25, 22 );

		ExtraUsedList					= new WListBox( ExtraPanel, InRoot );
		ExtraUsedList->Location			= TPoint( 235, 10 );
		ExtraUsedList->ItemIndex		= -1;	
		ExtraUsedList->EventDblClick	= WIDGET_EVENT(WScriptBuilder::ButtonRemoveExtraClick);
		ExtraUsedList->EventChange		= WIDGET_EVENT(WScriptBuilder::ListUsedClick);
		ExtraUsedList->SetSize( 180, 200 );

		HasTextCheck			= new WCheckBox( this, InRoot );
		HasTextCheck->Caption	= L"Has Text?";
		HasTextCheck->Tooltip	= L"Whether Script Has Editable Text?";
		HasTextCheck->Location	= TPoint( 15, 372 );
		HasTextCheck->bChecked	= true;

		OkButton				= new WButton( this, Root );
		OkButton->Caption		= L"Ok";
		OkButton->Location		= TPoint( 371, 369 );
		OkButton->EventClick	= WIDGET_EVENT(WScriptBuilder::ButtonOkClick);
		OkButton->SetSize( 64, 25 );

		ExtraNameEdit				= new WEdit( ExtraPanel, InRoot );
		ExtraNameEdit->Location		= TPoint( 235, 222 );
		ExtraNameEdit->EditType		= EDIT_String;
		ExtraNameEdit->EventChange	= WIDGET_EVENT(WScriptBuilder::EditExtraNameChange);
		ExtraNameEdit->SetSize( 180, 18 );

		ClassLabel				= new WLabel( ExtraPanel, InRoot );
		ClassLabel->Caption		= L"";
		ClassLabel->Location	= TPoint( 15, 223 );

		Hide();
	}

	// WForm interface.
	void Show( Integer X = 0, Integer Y = 0 )
	{
		WForm::Show( X, Y );

		// Reset everything.
		HasTextCheck->bChecked		= true;
		NameEdit->Text				= L"";
		GroupEdit->Text				= Browser->BrowserPanel->SelGroup;
		BaseCombo->ItemIndex		= -1;
		ExtraList->ItemIndex		= -1;
		ExtraUsedList->Empty();
		ExtraNameEdit->Text			= L"";
		AddExtraButton->bEnabled	= false;
		RemoveExtraButton->bEnabled	= false;
		ExtraNameEdit->bEnabled		= false;
	}
	void OnClose()
	{
		WForm::OnClose();
		Hide();
	}

	// Controls notification.
	void ButtonRemoveExtraClick( WWidget* Sender )
	{
		if( ExtraUsedList->ItemIndex != -1 )
		{
			// Remove from the list extra component.
			ExtraUsedList->Remove( ExtraUsedList->ItemIndex );
			ExtraUsedList->SetItemIndex( -1, true );
		}
	}
	void ButtonAddExtraClick( WWidget* Sender )
	{
		if( ExtraList->ItemIndex != -1 )
		{
			// Add a new extra component.
			CClass* ComClass = (CClass*)ExtraList->Items[ExtraList->ItemIndex].Data;

			// Make friendly name.
			assert(String::Pos( L"Component", ComClass->Alt ) != -1);
			ExtraUsedList->AddItem( String::Copy( ComClass->Alt, 0, ComClass->Alt.Len()-9 ), ComClass );
		}
	}
	void ListUsedClick( WWidget* Sender )
	{
		if( ExtraUsedList->ItemIndex != -1 )
		{
			ClassLabel->Caption	= ((CClass*)ExtraUsedList->Items[ExtraUsedList->ItemIndex].Data)->Alt + L":";
			ExtraNameEdit->Text	= ExtraUsedList->Items[ExtraUsedList->ItemIndex].Name;
		}
		else
		{
			ExtraNameEdit->Text	= L"";
			ClassLabel->Caption	= L"";
		}
		RemoveExtraButton->bEnabled	= ExtraUsedList->ItemIndex != -1;
		ExtraNameEdit->bEnabled		= ExtraUsedList->ItemIndex != -1;
	}
	void EditExtraNameChange( WWidget* Sender )
	{
		if( ExtraUsedList->ItemIndex != -1 )
		{
			ExtraUsedList->Items[ExtraUsedList->ItemIndex].Name = ExtraNameEdit->Text;
		}
	}
	void ListExtraChange( WWidget* Sender )
	{
		AddExtraButton->bEnabled	= ExtraList->ItemIndex != -1;
	}
	void ButtonOkClick( WWidget* Sender )
	{
		if( NameEdit->Text.Len() == 0 )
		{
			Root->ShowMessage( L"Please specify the name of script", L"Script Builder", true );
			return;
		}
		if( GObjectDatabase->FindObject( NameEdit->Text ) )
		{
			Root->ShowMessage
			(
				String::Format( L"Object \"%s\" already exists", *NameEdit->Text ),
				L"Script Builder",
				true
			);
			return;
		}
		if( BaseCombo->ItemIndex == -1 )
		{
			Root->ShowMessage( L"Please specify the base component", L"Script Builder", true );
			return;
		}
		for( Integer i=0; i<ExtraUsedList->Items.Num(); i++ )
		{
			CClass* Class = (CClass*)ExtraUsedList->Items[i].Data;
			String  Name  = ExtraUsedList->Items[i].Name;

			if( Name.Len() == 0 )
			{
				Root->ShowMessage
				(
					String::Format( L"Please specify the name of %d extra component", i ),
					L"Script Builder",
					true
				);
				return;
			}

			for( Integer j=i+1; j<ExtraUsedList->Items.Num(); j++ )
				if( ExtraUsedList->Items[j].Name == Name )
				{
					Root->ShowMessage
					(
						String::Format( L"Component name '%s' redefined", *Name ),
						L"Script Builder",
						true
					);
					return;
				}
		}

		// Create script.
		FScript* Script			= NewObject<FScript>( NameEdit->Text );
		Script->bHasText		= HasTextCheck->bChecked;
		Script->InstanceBuffer	= Script->bHasText ? new CInstanceBuffer(Script) : nullptr;
		Script->FileName		= Script->bHasText ? String::Format( L"%s.flu", *Script->GetName() ) : L"";
		Script->Group			= GroupEdit->Text;

		// Base.
		CClass* BaseClass = (CClass*)BaseCombo->Items[BaseCombo->ItemIndex].Data;
		FBaseComponent* Base = NewObject<FBaseComponent>( BaseClass, L"Base", Script );
		Base->InitForScript( Script );

		// Extras.
		for( Integer i=0; i<ExtraUsedList->Items.Num(); i++ )
		{
			CClass*	ExtraClass	= (CClass*)ExtraUsedList->Items[i].Data;
			String	ExtraName	= ExtraUsedList->Items[i].Name;

			FExtraComponent* Extra	= NewObject<FExtraComponent>( ExtraClass, ExtraName, Script );
			Extra->InitForScript( Script );
		}

		// If script has text - write some text to compile successfully.
		if( Script->bHasText )
		{
			Script->Text.Push(String::Format( L"/**" ));
			Script->Text.Push(String::Format( L" * @%s: ...", *Script->GetName() ));
			Script->Text.Push(String::Format( L" * @Author: ..." ));
			Script->Text.Push(String::Format( L" */" ));
			Script->Text.Push(String::Format( L"script %s", *Script->GetName() ));
			Script->Text.Push(String::Format( L"{" ));
			Script->Text.Push(String::Format( L" " ));
			Script->Text.Push(String::Format( L"}" ));
		}

		Hide();
		Browser->RefreshList();
		if( Script->bHasText )
			GEditor->OpenPageWith( Script );
	}
}	*GScriptBuilderForm;



/*-----------------------------------------------------------------------------
    WImportDialog implementation.
-----------------------------------------------------------------------------*/

//
// A resource import dialog.
// 
class WImportDialog: public WForm
{
public:
	// Variables.
	WResourceBrowser*	Browser;
	WLabel*				NameLabel;
	WLabel*				GroupLabel;
	WEdit*				NameEdit;
	WEdit*				GroupEdit;
	WPanel*				TopPanel;
	WButton*			OkButton;
	WButton*			CancelButton;

	// WImportDialog interface.
	WImportDialog( WResourceBrowser* InBrowser, WWindow* InRoot )
		:	WForm( InRoot, InRoot ),
			Browser( InBrowser )
	{
		// Initialize the form.
		Caption			= L"Resource Importer";
		bCanClose		= true;
		bSizeableH		= false;
		bSizeableW		= false;
		SetSize( 240, 123 );

		TopPanel			= new WPanel( this, Root );
		TopPanel->SetLocation( 8, 28 );
		TopPanel->SetSize( 224, 57 );		

		NameLabel			= new WLabel( TopPanel, Root );
		NameLabel->Caption	= L"Name:";
		NameLabel->SetLocation( 8, 8 );

		NameEdit			= new WEdit( TopPanel, Root );
		NameEdit->EditType	= EDIT_String;
		NameEdit->Location	= TPoint( 56, 7 );
		NameEdit->SetSize( 160, 18 );

		GroupLabel			= new WLabel( TopPanel, Root );
		GroupLabel->Caption	= L"Group:";
		GroupLabel->SetLocation( 8, 32 );

		GroupEdit			= new WEdit( TopPanel, Root );
		GroupEdit->EditType	= EDIT_String;
		GroupEdit->Location	= TPoint( 56, 31 );
		GroupEdit->SetSize( 160, 18 );

		OkButton				= new WButton( this, Root );
		OkButton->Caption		= L"Ok";
		OkButton->EventClick	= WIDGET_EVENT(WImportDialog::ButtonOkClick);
		OkButton->SetLocation( 38, 90 );
		OkButton->SetSize( 64, 25 );

		CancelButton				= new WButton( this, Root );
		CancelButton->Caption		= L"Cancel";
		CancelButton->EventClick	= WIDGET_EVENT(WImportDialog::ButtonCancelClick);
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
		// Start with dialog.
		if( ExecuteOpenFileDialog
							( 
								FileName, 
								GDirectory, 
								L"Any Resource File\0*.bmp;*.tga;*.wav;*.ogg;*.flf\0\0" 
							) )
		{
			// Show the form.
			WForm::Show( X, Y );

			// Fill fields.
			NameEdit->Text		= GetFileName(FileName);
			GroupEdit->Text		= Browser->BrowserPanel->SelGroup;
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
			Root->ShowMessage( L"Please specify the name of resource", L"Importer", true );
			return;
		}
		if( GObjectDatabase->FindObject( NameEdit->Text ) )
		{
			Root->ShowMessage
			(
				String::Format( L"Object '%s' already exists", *NameEdit->Text ),
				L"Script Builder",
				true
			);
			return;
		}

		FResource* Res = GEditor->ImportResource( FileName, NameEdit->Text );
		if( Res )
		{
			// Res has been imported.
			Res->Group	= GroupEdit->Text;
		}
		else 
		{
			// Resource import failure.
			log( L"Import '%s' failed.", *FileName );
		}

		Hide();
		Browser->RefreshList();
		// Do not open, just imported resource!
	}

private:
	// Importer internal.
	String			FileName;

}	*GImportDialogForm;


/*-----------------------------------------------------------------------------
    WAnimationBuilder implementation.
-----------------------------------------------------------------------------*/

//
// An animation builder dialog.
// 
class WAnimationBuilder: public WForm
{
public:
	// Variables.
	WResourceBrowser*	Browser;
	WLabel*				NameLabel;
	WLabel*				GroupLabel;
	WEdit*				NameEdit;
	WEdit*				GroupEdit;
	WPanel*				TopPanel;
	WButton*			OkButton;
	WButton*			CancelButton;

	// WAnimationBuilder interface.
	WAnimationBuilder( WResourceBrowser* InBrowser, WWindow* InRoot )
		:	WForm( InRoot, InRoot ),
			Browser( InBrowser )
	{
		// Initialize the form.
		Caption			= L"Animation Builder";
		bCanClose		= true;
		bSizeableH		= false;
		bSizeableW		= false;
		SetSize( 240, 123 );

		TopPanel			= new WPanel( this, Root );
		TopPanel->SetLocation( 8, 28 );
		TopPanel->SetSize( 224, 57 );		

		NameLabel			= new WLabel( TopPanel, Root );
		NameLabel->Caption	= L"Name:";
		NameLabel->SetLocation( 8, 8 );

		NameEdit			= new WEdit( TopPanel, Root );
		NameEdit->EditType	= EDIT_String;
		NameEdit->Location	= TPoint( 56, 7 );
		NameEdit->SetSize( 160, 18 );

		GroupLabel			= new WLabel( TopPanel, Root );
		GroupLabel->Caption	= L"Group:";
		GroupLabel->SetLocation( 8, 32 );

		GroupEdit			= new WEdit( TopPanel, Root );
		GroupEdit->EditType	= EDIT_String;
		GroupEdit->Location	= TPoint( 56, 31 );
		GroupEdit->SetSize( 160, 18 );

		OkButton				= new WButton( this, Root );
		OkButton->Caption		= L"Ok";
		OkButton->EventClick	= WIDGET_EVENT(WAnimationBuilder::ButtonOkClick);
		OkButton->SetLocation( 38, 90 );
		OkButton->SetSize( 64, 25 );

		CancelButton				= new WButton( this, Root );
		CancelButton->Caption		= L"Cancel";
		CancelButton->EventClick	= WIDGET_EVENT(WAnimationBuilder::ButtonCancelClick);
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
		NameEdit->Text		= L"";
		GroupEdit->Text		= Browser->BrowserPanel->SelGroup;
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
			Root->ShowMessage( L"Please specify the name of animation", L"Animation Builder", true );
			return;
		}
		if( GObjectDatabase->FindObject( NameEdit->Text ) )
		{
			Root->ShowMessage
			(
				String::Format( L"Object \"%s\" already exists", *NameEdit->Text ),
				L"Rename Dialog",
				true
			);
			return;
		}

		FAnimation* Animation	= NewObject<FAnimation>( NameEdit->Text );
		Animation->Group		= GroupEdit->Text;

		Hide();
		Browser->RefreshList();
		GEditor->OpenPageWith( Animation );
	}
}	*GAnimationBuilderForm;


/*-----------------------------------------------------------------------------
    WFontViewDialog implementation.
-----------------------------------------------------------------------------*/

//
// An font view dialog.
//
class WFontViewDialog: public WForm, public CRefsHolder
{
public:
	// Variables.
	FFont*				Font;
	WEdit*				Edit;

	// WFontViewDialog interface.
	WFontViewDialog( WContainer* InOwner, WWindow* InRoot )
		:	WForm( InOwner, InRoot ),
			Font( nullptr )
	{
		// Initialize the form.
		bCanClose		= true;
		bSizeableH		= false;
		bSizeableW		= false;
		Padding			= TArea( 22, 0, 1, 1 );
		SetSize( 400, 120 );

		Edit			= new WEdit( this, InRoot );
		Edit->Text		= L"The quick brown fox";
		Edit->Align		= AL_Top;
		Edit->EditType	= EDIT_String;
		Edit->SetSize( 150, 18 );

		Hide();
		SetFont( nullptr );
	}
	void SetFont( FFont* NewFont )
	{
		Font		= NewFont;
		if( Font )
			Caption	= String::Format( L"Font Viewer [%s]", *Font->GetName() );
		else
			Caption	= L"Font Viewer";
	}

	// WWidget interface.
	void OnPaint( CGUIRenderBase* Render )
	{
		WForm::OnPaint(Render);
		Render->SetClipArea
		(
			ClientToWindow(TPoint::Zero),
			TSize( Size.Width-1, Size.Height-1 )
		);

		if( Font )
			Render->DrawText
			(
				ClientToWindow(TPoint::Zero) + TPoint( (Size.Width-Font->TextWidth(*Edit->Text))/2, 40+(Size.Height-80+Font->Height)/2 ),
				*Edit->Text,
				COLOR_White,
				Font
			);
	}

	// WForm interface.
	void Show( Integer X = 0, Integer Y = 0 )
	{
		WForm::Show( X, Y );
		SetFont( nullptr );
	}
	void Hide()
	{
		WForm::Hide();
		SetFont( nullptr );
	}
	void OnClose()
	{
		WForm::OnClose();
		Hide();
	}

	// CRefsHolder interface.
	void CountRefs( CSerializer& S )
	{
		Serialize( S, Font );
		SetFont( Font );
		if( !Font )
			Hide();
	}
}	*GFontViewDialog;


/*-----------------------------------------------------------------------------
    WResourceBrowser implementation.
-----------------------------------------------------------------------------*/

//
// Resource browser constructor.
//
WResourceBrowser::WResourceBrowser( WContainer* InOwner, WWindow* InRoot )
	:	WContainer( InOwner, InRoot )
{
	// Setup own variables.
	SetSize( 187, 64 );
	Align	= AL_Left;
	Caption	= L"Resource Browser";
	Padding = TArea( FORM_HEADER_SIZE, 0, 0, 0 );

	// Create controls.
	HeaderPanel					= new WPanel( this, Root );
	HeaderPanel->Align			= AL_Top;
	HeaderPanel->bDrawEdges		= true;
	HeaderPanel->SetSize( 50, 65 );

	ImportButton				= new WPictureButton( HeaderPanel, Root );
	ImportButton->Tooltip		= L"Import Resource";
	ImportButton->Picture		= Root->Icons;
	ImportButton->Offset		= TPoint( 0, 96 );
	ImportButton->Scale			= TSize( 16, 16 );
	ImportButton->Location		= TPoint( 2, 2 );
	ImportButton->EventClick	= WIDGET_EVENT(WResourceBrowser::ButtonImportClick);
	ImportButton->SetSize( 22, 22 );

	CreateButton				= new WPictureButton( HeaderPanel, Root );
	CreateButton->Tooltip		= L"Create Resource";
	CreateButton->Picture		= Root->Icons;
	CreateButton->Offset		= TPoint( 16, 96 );
	CreateButton->Scale			= TSize( 16, 16 );
	CreateButton->Location		= TPoint( 23, 2 );
	CreateButton->EventClick	= WIDGET_EVENT(WResourceBrowser::ButtonCreateClick);
	CreateButton->SetSize( 22, 22 );

	ClassButton					= new WPictureButton( HeaderPanel, Root );
	ClassButton->Tooltip		= L"Type Filter";
	ClassButton->Picture		= Root->Icons;
	ClassButton->Offset			= TPoint( 32, 96 );
	ClassButton->Scale			= TSize( 16, 16 );
	ClassButton->Location		= TPoint( 44, 2 );
	ClassButton->EventClick		= WIDGET_EVENT(WResourceBrowser::ButtonClassClick);
	ClassButton->SetSize( 22, 22 );

	RemoveButton				= new WPictureButton( HeaderPanel, Root );
	RemoveButton->Tooltip		= L"Remove Resource";
	RemoveButton->Picture		= Root->Icons;
	RemoveButton->Offset		= TPoint( 48, 96 );
	RemoveButton->Scale			= TSize( 16, 16 );
	RemoveButton->Location		= TPoint( Size.Width-22-2, 2 );
	RemoveButton->EventClick	= WIDGET_EVENT(WResourceBrowser::ButtonRemoveClick);
	RemoveButton->SetSize( 22, 22 );

	// Name filter.
	NameFilter					= new WEdit( HeaderPanel, Root );
	NameFilter->EditType		= EDIT_String;
	NameFilter->Location		= TPoint( 2, 44 );
	NameFilter->EventChange		= WIDGET_EVENT(WResourceBrowser::EditNameChange);
	NameFilter->SetSize( Size.Width-4, 18 );

	// Class filter.
	GroupFilter					= new WComboBox( HeaderPanel, Root );
	GroupFilter->Location		= TPoint( 2, 25 );
	GroupFilter->EventChange	= WIDGET_EVENT(WResourceBrowser::ComboGroupChange);
	GroupFilter->SetSize( Size.Width-4, 18 );
	GroupFilter->AddItem( L"", nullptr );

	// New popup.
	NewPopup					= new WPopupMenu( Root, Root );
	NewPopup->AddItem( L"New Animation",		WIDGET_EVENT(WResourceBrowser::PopNewAnimationClick) );
	NewPopup->AddItem( L"New Script",			WIDGET_EVENT(WResourceBrowser::PopNewScriptClick) );
	NewPopup->AddItem( L"New Demoscene Effect", WIDGET_EVENT(WResourceBrowser::PopNewEffectClick) );

	// Class popup.
	ClassPopup					= new WPopupMenu( Root, Root );
	ClassPopup->AddItem( L"All", WIDGET_EVENT(WResourceBrowser::PopClassAllClick), true );
	ClassPopup->AddItem( L"" );
	for( Integer i=0; i<CClassDatabase::GClasses.Num(); i++ )
	{
		CClass* Class = CClassDatabase::GClasses[i];
		if	(	
				!(Class->Flags & CLASS_Abstract) &&
				Class->IsA(FResource::MetaClass) && 
				!Class->IsA(FLevel::MetaClass) &&
				!Class->IsA(FProjectInfo::MetaClass) &&
				(Class->Super==FResource::MetaClass || Class->Super==FBlockResource::MetaClass)
			)
		{
			ClassPopup->AddItem( Class->Alt, WIDGET_EVENT(WResourceBrowser::PopClassClick), true );
		}
	}
	for( Integer i=0; i<ClassPopup->Items.Num(); i++ )
		ClassPopup->Items[i].bChecked	= true;

	// Panel.
	BrowserPanel			= new WBrowserPanel( this, Root );
	BrowserPanel->Margin	= TArea( -1, 0, 0, 0 );

	// Create builders.
	GDemoEffectBuilderForm	= new WDemoEffectBuilder( this, Root );
	GAnimationBuilderForm	= new WAnimationBuilder( this, Root );
	GScriptBuilderForm		= new WScriptBuilder( this, Root );
	GImportDialogForm		= new WImportDialog( this, Root );
	GRenameDialog			= new WRenameDialog( Root );
	GMusicPlayer			= new WMusicPlayer( Root, Root );
	GFontViewDialog			= new WFontViewDialog( Root, Root );
}


//
// Resource browser destructor.
//
WResourceBrowser::~WResourceBrowser()
{
	// Release popups, because they owned by Root.
	delete NewPopup;
	delete ClassPopup;

	// Release global dialogs.
	freeandnil(GDemoEffectBuilderForm);
	freeandnil(GImportDialogForm);
	freeandnil(GAnimationBuilderForm);
	freeandnil(GScriptBuilderForm);
	freeandnil(GMusicPlayer);
	freeandnil(GFontViewDialog);
}


//
// Destroy selected resource.
//
void WResourceBrowser::ButtonRemoveClick( WWidget* Sender )
{
	FResource* Res = GetSelected();
	if( Res )
	{
		if( !Res->IsA(FScript::MetaClass) )
		{
			// Some resource.
			Integer S	= MessageBox
			(
				GEditor->hWnd,
				*String::Format( L"Do you really want to destroy resource '%s'?", *Res->GetName() ),
				L"Resource Browser",
				MB_YESNO | MB_ICONQUESTION
			);

			if( S == IDYES )
			{
				// If its a font, also kill it bitmaps.
				FFont* Font = As<FFont>(Res);
				if( Font )
				{
					for( Integer i=0; i<Font->Bitmaps.Num(); i++ )
						if( Font->Bitmaps[i] )
							DestroyObject( Font->Bitmaps[i], true );
				}

				// Destroy resource.
				DestroyObject( Res, true );
				RefreshList();
			}
		}
		else
		{
			// A script resource.
			FScript* Script	= As<FScript>(Res);

			// Count entities being this script.
			Integer NumEnts = 0;
			for( Integer i=0; i<GProject->GObjects.Num(); i++ )
				if( GProject->GObjects[i] && GProject->GObjects[i]->IsA(FEntity::MetaClass) )
				{
					FEntity* Entity	= As<FEntity>(GProject->GObjects[i]);
					if( Entity->Script == Script )
						NumEnts++;
				}


			// Don't destroy camera script, if it in use.
			if( NumEnts && Script->Base->IsA(FCameraComponent::MetaClass) )
			{
				MessageBox
				( 
					GEditor->hWnd, 
					L"Cannot destroy system camera script", 
					L"Resource Browser", 
					MB_OK | MB_ICONWARNING 
				);
				return;
			}

			Integer S	= MessageBox
			(
				GEditor->hWnd,
				*String::Format( L"Do you really want to destroy script '%s' and %d entities being it?", *Script->GetName(), NumEnts ),
				L"Resource Browser",
				MB_YESNO | MB_ICONQUESTION
			);

			if( S == IDYES )
			{
				// Release all refs to this script.
				GEditor->DropAllScripts();

				// Kill all entities.
				for( Integer i=0; i<GProject->GObjects.Num(); i++ )
					if( GProject->GObjects[i] && GProject->GObjects[i]->IsA(FEntity::MetaClass) )
					{
						FEntity* Entity	= As<FEntity>(GProject->GObjects[i]);
						if( Entity->Script == Script )
						{
							Entity->Level->Entities.RemoveUnique( Entity );
							DestroyObject( Entity, true );
						}
					}

				// Destroy script itself.
				DestroyObject( Script, true );
				RefreshList();
			}
		}
	}
}


//
// Import resource click.
//
void WResourceBrowser::ButtonImportClick( WWidget* Sender )
{
	GImportDialogForm->Show
							( 
								Root->Size.Width/3, 
								Root->Size.Height/3 
							);
}


//
// On group filter changed.
//
void WResourceBrowser::ComboGroupChange( WWidget* Sender )
{
	BrowserPanel->SelGroup	= GroupFilter->Items[GroupFilter->ItemIndex].Name;
	RefreshList();
}


//
// On class filter checked.
//
static Bool GOldFilter[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
void WResourceBrowser::PopClassClick( WWidget* Sender )
{
	// 'Not all are now'.
	Integer iChecked = 0;
	for( iChecked=1; iChecked<ClassPopup->Items.Num(); iChecked++ )
		if( ClassPopup->Items[iChecked].bChecked != GOldFilter[iChecked] )
			break;

	for( Integer i=0; i<ClassPopup->Items.Num(); i++ )
	{
		ClassPopup->Items[i].bChecked	= false;
		GOldFilter[i]					= false;
	}

	ClassPopup->Items[iChecked].bChecked	= true;
	GOldFilter[iChecked]					= true;

	String RealName = String(L"F") + ClassPopup->Items[iChecked].Text;
	BrowserPanel->ClassFilter	= CClassDatabase::StaticFindClass( *RealName );

	// And refresh list.
	RefreshList();
}


//
// On class filter 'All' checked.
//
void WResourceBrowser::PopClassAllClick( WWidget* Sender )
{
	// If 'all' selected, mark everything.
	for( Integer i=0; i<ClassPopup->Items.Num(); i++ )
	{
		ClassPopup->Items[i].bChecked	= true;
		GOldFilter[i]					= true;
	}

	BrowserPanel->ClassFilter	= nullptr;

	// Just refresh list.
	RefreshList();
}


//
// Open a script builder.
//
void WResourceBrowser::PopNewScriptClick( WWidget* Sender )
{
	GScriptBuilderForm->Show
							( 
								Root->Size.Width/3, 
								Root->Size.Height/3 
							);
}


//
// Open an animation builder.
//
void WResourceBrowser::PopNewAnimationClick( WWidget* Sender )
{
	GAnimationBuilderForm->Show
							( 
								Root->Size.Width/3, 
								Root->Size.Height/3 
							);
}


//
// Open an effect builder.
//
void WResourceBrowser::PopNewEffectClick( WWidget* Sender )
{
	GDemoEffectBuilderForm->Show
							( 
								Root->Size.Width/3, 
								Root->Size.Height/3 
							);
}


//
// Name filter has been changed.
//
void WResourceBrowser::EditNameChange( WWidget* Sender )
{
	BrowserPanel->RefreshList();
}


//
// Refresh resources list.
//
void WResourceBrowser::RefreshList()
{
	// Delegate it to BrowserPanel.
	BrowserPanel->RefreshList();
}


//
// Popup 'Resource constructor'
//
void WResourceBrowser::ButtonCreateClick( WWidget* Sender )
{
	NewPopup->Show( Root->MousePos );
}


//
// Popup 'Class filter'.
//
void WResourceBrowser::ButtonClassClick( WWidget* Sender )
{
	ClassPopup->Show( Root->MousePos );
}


//
// Return selected resource.
//
FResource* WResourceBrowser::GetSelected() const
{
	return BrowserPanel->Selected;
}


//
// Draw resource browser.
//
void WResourceBrowser::OnPaint( CGUIRenderBase* Render )
{
	WContainer::OnPaint( Render );
	TPoint Base = ClientToWindow(TPoint::Zero);

	// Draw frame.
	Render->DrawRegion
					(
						Base,
						Size,
						GUI_COLOR_FORM_BG,
						GUI_COLOR_FORM_BORDER,
						BPAT_Solid
					);

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
	RemoveButton->bEnabled		= GProject != nullptr && GetSelected() != nullptr;
	ImportButton->bEnabled		= GProject != nullptr;
	ClassButton->bEnabled		= GProject != nullptr;
	CreateButton->bEnabled		= GProject != nullptr;
}


/*-----------------------------------------------------------------------------
    WBrowserPanel implementation.
-----------------------------------------------------------------------------*/

//
// Double click over browser.
//
void WBrowserPanel::OnDblClick( EMouseButton Button, Integer X, Integer Y )
{
	WPanel::OnDblClick( Button, X, Y );
	bReadyDrag		= false;
	if( Selected )
	{
		if( Selected->IsA(FSound::MetaClass) )
		{
			GEditor->GAudio->PlayFX( As<FSound>(Selected), 1.f, 1.f );
		}
		else if( Selected->IsA(FMusic::MetaClass) )
		{
			GMusicPlayer->Show( Root->Size.Width/3, Root->Size.Height/3 );
			GMusicPlayer->SetMusic( As<FMusic>(Selected) );
		}
		else if( Selected->IsA(FFont::MetaClass) )
		{
			GFontViewDialog->Show( Root->Size.Width/3, Root->Size.Height/3 );
			GFontViewDialog->SetFont( As<FFont>(Selected) );
		}
		else
			GEditor->OpenPageWith( Selected );
	}
}   

/*-----------------------------------------------------------------------------
    WRenameDialog implementation.
-----------------------------------------------------------------------------*/

//
// Global dialog instance.
//
WRenameDialog*	GRenameDialog	= nullptr;


//
// Rename dialog constructor.
//
WRenameDialog::WRenameDialog( WWindow* InRoot )
	:	WForm( InRoot, InRoot ),
		Resource( nullptr ),
		OldModal( nullptr )
{
	// Initialize the form.
	Caption			= L"Rename Dialog";
	bCanClose		= true;
	bSizeableH		= false;
	bSizeableW		= false;
	SetSize( 240, 123 );

	TopPanel			= new WPanel( this, Root );
	TopPanel->SetLocation( 8, 28 );
	TopPanel->SetSize( 224, 57 );		

	NameLabel			= new WLabel( TopPanel, Root );
	NameLabel->Caption	= L"Name:";
	NameLabel->SetLocation( 8, 8 );

	NameEdit			= new WEdit( TopPanel, Root );
	NameEdit->EditType	= EDIT_String;
	NameEdit->Location	= TPoint( 56, 7 );
	NameEdit->SetSize( 160, 18 );

	GroupLabel			= new WLabel( TopPanel, Root );
	GroupLabel->Caption	= L"Group:";
	GroupLabel->SetLocation( 8, 32 );

	GroupEdit			= new WEdit( TopPanel, Root );
	GroupEdit->EditType	= EDIT_String;
	GroupEdit->Location	= TPoint( 56, 31 );
	GroupEdit->SetSize( 160, 18 );

	OkButton				= new WButton( this, Root );
	OkButton->Caption		= L"Ok";
	OkButton->EventClick	= WIDGET_EVENT(WRenameDialog::ButtonOkClick);
	OkButton->SetLocation( 38, 90 );
	OkButton->SetSize( 64, 25 );

	CancelButton				= new WButton( this, Root );
	CancelButton->Caption		= L"Cancel";
	CancelButton->EventClick	= WIDGET_EVENT(WRenameDialog::ButtonCancelClick);
	CancelButton->SetLocation( 138, 90 );
	CancelButton->SetSize( 64, 25 );

	Hide();
}


//
// Just show rename dialog.
//
void WRenameDialog::Show( Integer X, Integer Y )
{
	WForm::Show( X, Y );
	OldModal	= Root->Modal;
	Root->Modal	= this;
}


//
// Set resource to edit and open dialog.
//
void WRenameDialog::SetResource( FResource* InResource )
{
	assert(InResource);
	Resource	= InResource;

	NameEdit->Text	= Resource->GetName();
	GroupEdit->Text	= Resource->Group;

	// Turn off group for level.
	GroupEdit->bEnabled	=
	GroupLabel->bEnabled	= !Resource->IsA(FLevel::MetaClass);

	Show( Root->Size.Width/3, Root->Size.Height/3 );
}


//
// Rename an object.
//
void WRenameDialog::ButtonOkClick( WWidget* Sender )
{
	if( NameEdit->Text.Len() == 0 )
	{
		Root->ShowMessage( L"Please specify new name of resource", L"Rename Dialog", true );
		return;
	}
	if( GObjectDatabase->FindObject( NameEdit->Text ) )
	{
		Root->ShowMessage
		(
			String::Format( L"Object \"%s\" already exists", *NameEdit->Text ),
			L"Rename Dialog",
			true
		);
		return;
	}

	// Change resource group.
	Resource->Group	= GroupEdit->Text;

	// Object renaming is tricky a little, since all
	// objects are hashed in global table.
	GObjectDatabase->RenameObject( Resource, NameEdit->Text );
	Hide();

	GEditor->Explorer->Refresh();
}


//
// Close dialog.
//
void WRenameDialog::OnClose()
{
	WForm::OnClose();
	Hide();
}


//
// Don't rename.
//
void WRenameDialog::ButtonCancelClick( WWidget* Sender )
{
	Hide();
}


//
// Count references in rename dialog.
//
void WRenameDialog::CountRefs( CSerializer& S )
{
	Serialize( S, Resource );
	if( !Resource )
		Hide();
}


//
// When rename dialog hides.
//
void WRenameDialog::Hide()
{
	WForm::Hide();
	Root->Modal		= OldModal;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/