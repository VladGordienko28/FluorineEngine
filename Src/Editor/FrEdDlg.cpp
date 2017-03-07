/*=============================================================================
    FrEdDlg.cpp: Editor dialogs wrapping functions.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "Editor.h"

/*-----------------------------------------------------------------------------
    Windows dialogs.
-----------------------------------------------------------------------------*/

//
// Open a color pick dialog, set its default
// color as default, return true.
//
Bool ExecuteColorDialog( TColor& PickedColor, const TColor Default )
{
	static COLORREF CustomColors[16] = {};
	CHOOSECOLOR CC;
	MemZero( &CC, sizeof(CHOOSECOLOR) );
	CC.lStructSize	= sizeof(CHOOSECOLOR);
	CC.hwndOwner	= GEditor->hWnd;
	CC.rgbResult	= RGB( Default.R, Default.G, Default.B );
	CC.Flags		= CC_FULLOPEN | CC_RGBINIT;
	CC.lpCustColors = CustomColors;

	if( ChooseColor(&CC) == TRUE )
	{
		// Pick some color.
		PickedColor	= TColor
						( 
							GetRValue(CC.rgbResult), 
							GetGValue(CC.rgbResult), 
							GetBValue(CC.rgbResult), 
							PickedColor.A 
						);
		return true;
	}
	else
	{
		// Canceled.
		return false;
	}
}


//
// Open a file open dialog.
//
Bool ExecuteOpenFileDialog( String& FileName, String Directory, const Char* Filters )
{
	OPENFILENAME	OPN;
	Char			File[256];
	MemZero( &OPN, sizeof(OPENFILENAME) );
	MemZero( &File[0], sizeof(File) );
	OPN.lStructSize		= sizeof(OPENFILENAME);
	OPN.hwndOwner		= GEditor->hWnd;
	OPN.lpstrFile		= File;
	OPN.lpstrFile[0]	= L'\0';
	OPN.nMaxFile		= sizeof(File);
	OPN.lpstrFilter		= Filters;
	OPN.nFilterIndex	= 1;
	OPN.lpstrFileTitle	= nullptr;
	OPN.nMaxFileTitle	= 0;
	OPN.lpstrInitialDir	= *Directory;
	OPN.Flags			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if( GetOpenFileName( &OPN ) == TRUE )
	{
		// File opened.
		FileName	= File;
		SetCurrentDirectory(*GDirectory);
		return true;
	}
	else
	{
		// Failed.
		FileName	= L"";
		SetCurrentDirectory(*GDirectory);
		return false;
	}
}


//
// Open a file save dialog.
//
Bool ExecuteSaveFileDialog( String& FileName, String Directory, const Char* Filters )
{
	OPENFILENAME	SAV;
	Char			File[256];
	MemZero( &SAV, sizeof(OPENFILENAME) );
	MemZero( &File[0], sizeof(File) );
	SAV.lStructSize		= sizeof(OPENFILENAME);
	SAV.hwndOwner		= GEditor->hWnd;
	SAV.lpstrFile		= File;
	SAV.lpstrFile[0]	= L'\0';
	SAV.nMaxFile		= sizeof(File);
	SAV.lpstrFilter		= Filters;
	SAV.nFilterIndex	= 1;
	SAV.lpstrFileTitle	= nullptr;
	SAV.nMaxFileTitle	= 0;
	SAV.lpstrInitialDir	= *Directory;
	SAV.Flags			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if( GetSaveFileName( &SAV ) == TRUE )
	{
		// File saved.
		FileName	= File;
		SetCurrentDirectory(*GDirectory);
		return true;
	}
	else
	{
		// Failed.
		FileName	= L"";
		SetCurrentDirectory(*GDirectory);
		return false;
	}
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/