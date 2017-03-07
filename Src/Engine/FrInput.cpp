/*=============================================================================
    FrInput.cpp: Level input processing.
    Copyright Nov.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    CInput implementation.
-----------------------------------------------------------------------------*/

//
// Input subsystem constructor.
//
CInput::CInput()
{
	// Nothing pressed.
	MemZero( Keys, sizeof(Keys) );

	// Default system remap. In case of
	// some weird platform, platform should
	// change this table itself.
	for( Integer iKey=0; iKey<KEY_MAX; iKey++ )
		SystemRemap[iKey]	= iKey;

	// Default user's remap table. It should be
	// changed via config file.
	for( Integer iKey=0; iKey<KEY_MAX; iKey++ )
		ConfigRemap[iKey]	= iKey;

	// Mouse variables.
	MouseX		= 0;
	MouseY		= 0;
	WheelScroll	= 0;
	WorldCursor	= TVector( 0.f, 0.f );

	// Nothing to process.
	Level	= nullptr;

	// Notify.
	log( L"In: Input subsystem initialized" );
}


//
// Reset subsystem.
//
void CInput::Reset()
{
	// Unpress everything.
	MemZero( Keys, sizeof(Keys) );
	MouseX			= 0;
	MouseY			= 0;
	WheelScroll		= 0;
	WorldCursor		= TVector( 0.f, 0.f );

	// Set no level.
	Level	= nullptr;
}


//
// Initialize input system for a given level, all
// input events will be redirected to script system
// via FInputComponent.
//
void CInput::SetLevel( FLevel* InLevel )
{
	assert(InLevel);

	Reset();
	Level	= InLevel;
}


//
// Some key has been pressed.
//
void CInput::OnKeyDown( Integer iKey )
{
	iKey	= KEY_MAX & iKey;

	// Notify inputs.
	if( Level )
	{
		Integer Pressed = ConfigRemap[SystemRemap[iKey]];
		for( Integer i=0; i<Level->Inputs.Num(); i++ )
			Level->Inputs[i]->Entity->CallEvent( EVENT_OnKeyDown, Pressed );
	}

	// Store it.
	Keys[iKey]	= true;
}


//
// Some key has been unpressed.
//
void CInput::OnKeyUp( Integer iKey )
{
	iKey	= KEY_MAX & iKey;

	// Notify inputs.
	if( Level )
	{
		Integer Pressed = ConfigRemap[SystemRemap[iKey]];
		for( Integer i=0; i<Level->Inputs.Num(); i++ )
			Level->Inputs[i]->Entity->CallEvent( EVENT_OnKeyUp, Pressed );
	}

	// Store it.
	Keys[iKey]	= false;
}


//
// When user type some character, bring it to
// script.
//
void CInput::OnCharType( Char TypedChar )
{
	if( !Level )
		return;

	// Avoid really crazy characters such as
	// <return> or <esc>... Process only typeable
	// letters, digits, symbols.
	static const Char Symbols[] = L"~!@#$%^& *()_+{}[]/-;.,:";
	if	(
			!IsLetter(TypedChar) &&
			!IsDigit(TypedChar) &&
			!wcschr( Symbols, TypedChar )
		)
		return;

	// Pass to script as string.
	Char	Ch[2]	= { TypedChar, 0 };
	String	S		= Ch;

	for( Integer i=0; i<Level->Inputs.Num(); i++ )
		Level->Inputs[i]->Entity->CallEvent( EVENT_OnCharType, S );
}


//
// Read ConfigRemap from the ini file to apply user
// configuration preferences.
//
void CInput::RemapFromIni( CIniFile* InFile )
{
	// Get list of names.
	CEnum* InputKeys	= CClassDatabase::StaticFindEnum(L"EInputKeys");
	assert(InputKeys);
	assert(InputKeys->Aliases.Num()>=KEY_MAX);

	// Let's torment ini.
	for( Integer i=0; i<KEY_MAX; i++ )
		ConfigRemap[i]	= InFile->ReadInteger( L"Input", *InputKeys->Aliases[i], i );

}


//
// Return true, if given key are pressed.
//
Bool CInput::KeyIsPressed( Integer iKey )
{
	iKey	= KEY_MAX & iKey;
	return Keys[ConfigRemap[SystemRemap[iKey]]];
}


//
// Count references in system.
//
void CInput::CountRefs( CSerializer& S )
{
	FLevel* OldLevel = Level;
	Serialize( S, Level );

	// Reset system, if level gone far away.
	if( OldLevel && !Level )
		Reset();
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/