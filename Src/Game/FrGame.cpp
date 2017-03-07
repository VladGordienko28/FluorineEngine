/*=============================================================================
    FrGame.cpp: Game application class.
    Copyright Nov.2016 Vlad Gordienko.
=============================================================================*/

#include "Game.h"
#include "FrJoy.h"
#include "Res\resource.h"

/*-----------------------------------------------------------------------------
    Declarations.
-----------------------------------------------------------------------------*/

//
// Game window minimum size.
//
#define MIN_GAME_X		320
#define MIN_GAME_Y		240


//
// Game directories.
//
#define SPLASH_DIR			L"Doc\\Splash.bmp"
#define CONFIG_DIR			L"Game.ini"


//
// Forward declaration.
//
static LRESULT CALLBACK WndProc( HWND hWnd, UINT Message, WPARAM WParam, LPARAM LParam );
static Char* StackTrace( LPEXCEPTION_POINTERS InException );
static Integer HandleException( LPEXCEPTION_POINTERS InException );
static String GetFileName( String FileName );
static String GetFileDir( String FileName );
static TStaticBitmap* LoadBitmapFromResource( LPCTSTR ResID );
static TStaticFont* LoadFontFromResource( LPCTSTR FontID, LPCTSTR BitmapID );


//
// Globals.
//
CGame*	GGame	= nullptr;


/*-----------------------------------------------------------------------------
    CGame implementation.
-----------------------------------------------------------------------------*/

//
// Game pre-initialization.
//
CGame::CGame()
	:	CApplication(),
		Console( nullptr ),
		Level( nullptr )
{
	// Say hello to user.
	log( L"========================="			);
	log( L"=    Fluorine Engine    ="			);
	log( L"=      %s        =",			FLU_VER	);
	log( L"========================="			);
	log( L"" );

	// Initialize global variables.
	GIsEditor			= false;
	GGame				= this;

	// Exe-directory.
	Char Directory[256];
	_wgetcwd( Directory, array_length(Directory) );
	GDirectory	= Directory;

	// Parse command line.
	Integer	NumArgs;
	LPWSTR*	StrArgs	= CommandLineToArgvW( GetCommandLine(), &NumArgs );
	for( Integer i=0; i<Min<Integer>(NumArgs, array_length(GCmdLine)); i++ )
	{
		GCmdLine[i]	= StrArgs[i];
		log( L"CmdLine[%d]: '%s'", i, *GCmdLine[i] );
	}
	LocalFree( StrArgs );

	// Initialize C++ stuff.
	srand(GetTickCount() ^ 0x20162016);
}


//
// Game destruction.
//
CGame::~CGame()
{
	GGame	= nullptr;
}


//
// Load game from file.
//
Bool CGame::LoadGame( String Directory, String Name )
{
	// Load game file.
	if( !CApplication::LoadGame( Directory, Name ) )
		return false;

	// Build list of all levels.
	Level	= nullptr;
	LevelList.Empty();
	for( Integer i=0; i<GObjectDatabase->GObjects.Num(); i++ )
	{
		FObject* Object = GObjectDatabase->GObjects[i];
		if( Object && Object->IsA(FLevel::MetaClass) )
			LevelList.Push((FLevel*)Object);
	}

	// Refresh window.
	SetSize
	( 
		Project->Info->DefaultWidth, 
		Project->Info->DefaultHeight, 
		Project->Info->WindowType
	);

	return true;
}


/*-----------------------------------------------------------------------------
    Game initialization.
-----------------------------------------------------------------------------*/

//
// Initialize the game.
//
void CGame::Init( HINSTANCE InhInstance )
{
	// Init window class.
    WNDCLASSEX wcex;
	static Char*	FluWinCls = L"FluEngine_Game";

	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance		= InhInstance;
	wcex.hIcon			= LoadIcon( hInstance, MAKEINTRESOURCE(IDI_FLUICON) );
	wcex.hCursor		= LoadCursor( nullptr, IDC_ARROW );
	wcex.hbrBackground	= nullptr;				
	wcex.lpszMenuName	= nullptr;
	wcex.lpszClassName	= FluWinCls;
	wcex.hIconSm		= LoadIcon( hInstance, MAKEINTRESOURCE(IDI_FLUICON) );

	// Register window.
	if( !RegisterClassEx(&wcex) )
	{	
		log( L"GetLastError %i", GetLastError() );
		error( L"RegisterClassEx failure" );
	}

	// Create the window.
	hWnd	= CreateWindow
	(
		FluWinCls,
		L"Fluorine Game",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600,
		nullptr,
		nullptr,
		hInstance,
		0
	);
	assert(hWnd);

	// Load ini-file.
	Config		= new CIniFile(GDirectory+L"\\"+CONFIG_DIR);

	// Allocate subsystems.
	GRender		= new COpenGLRender( hWnd );
	GAudio		= new COpenALAudio();
	GInput		= new CInput();

	// Set default audio volume.
	GAudio->MasterVolume	= Config->ReadFloat( L"Audio",	L"MasterVolume",	1.f );
	GAudio->MusicVolume		= Config->ReadFloat( L"Audio",	L"MusicVolume",		1.f );
	GAudio->FXVolume		= Config->ReadFloat( L"Audio",	L"FXVolume",		1.f );

	// Show the window.
	ShowWindow( hWnd, /*SW_SHOWNORMAL*/SW_SHOWMAXIMIZED );
	UpdateWindow( hWnd );

	// Allocate console.
	Console			= new CConsole();
	CConsole::Font	= LoadFontFromResource
	(
		MAKEINTRESOURCE(IDR_FONT2),
		MAKEINTRESOURCE(IDB_FONT2)
	);

	// Notify.
	log( L"Game: Game initialized" );

	// Load game? What way we will use?
	if( GCmdLine[1] )
	{
		// Open Command-Line game.
		String FileName	= String::Pos( L":\\", GCmdLine[1] ) != -1 ? 
							GCmdLine[1] : 
							GDirectory+L"\\"+GCmdLine[1];

		log( L"Game: Load game from '%s'", *FileName );

		if( !GPlat->FileExists(FileName) )
			error(L"Game file '%s' not found", *FileName);

		String Directory	= GetFileDir(GCmdLine[1]);
		String Name			= GetFileName(GCmdLine[1]);

		// Load it.
		LoadGame( Directory, Name );

		// Run some initial level.
		FLevel* Entry = nullptr;
		if( GCmdLine[2] )
			Entry = FindLevel( GCmdLine[2] );
		else
			log( L"Game: Entry level is not specified." );

		if( !Entry && LevelList.Num() )
			Entry = LevelList[0];

		if( Entry )
		{
			RunLevel( Entry, true );
		}
		else
			log( L"Game: Entry level not found!" );
	}
	else
	{
		// Try to find some file in directory.
		String			FileName;
		WIN32_FIND_DATA	FindData;
		HANDLE			hFind;

		hFind	= FindFirstFile( *(GDirectory+L"\\*"+PROJ_FILE_EXT), &FindData );
		{
			assert(hFind != INVALID_HANDLE_VALUE);
			FileName	= GDirectory + L"\\" + FindData.cFileName;
		}
		FindClose(hFind);

		if( !GPlat->FileExists(FileName) )
			error(L"Game file not found");

		log( L"Game: Game file '%s' detected!", *FileName );

		String Directory	= GetFileDir(FileName);
		String Name			= GetFileName(FileName);

		// Load it.
		LoadGame( Directory, Name );

		// Try to find entry level.
		FLevel* Entry = FindLevel( L"Entry" );
		if( !Entry )
		{
			log( L"Game: Entry level not found!" );
			if( LevelList.Num() )
				Entry	= LevelList[0];
		}

		// Run it.
		if( Entry )
			RunLevel( Entry, true );
		else
			log( L"No level's found in '%s'", *FileName );
	}
}


/*-----------------------------------------------------------------------------
    Game tick.
-----------------------------------------------------------------------------*/

//
// Tick game.
//
void CGame::Tick( Float Delta )
{
	CCanvas* Canvas		= GRender->Lock();
	{
		// Tick, things, which need tick.
		if( Level )	
		{
			Level->Tick( Delta );
			GAudio->Tick( Delta, Level );
		}

		if( Project )
			Project->BlockMan->Tick( Delta );

		// Render level.
		if( Level )
			GRender->RenderLevel
			(
				Canvas,
				Level,
				0, 0,
				WinWidth, WinHeight
			);

		// Render console.
		if( Console->IsActive() )
			Console->Render( Canvas );
	}
	GRender->Unlock();

	// Handle level's travel. Play page don't allow to
	// travel, so just notify player about it.
	if( GIncomingLevel )
	{
		RunLevel
		(
			GIncomingLevel.Destination,
			GIncomingLevel.bCopy
		);

		// Unmark it.
		GIncomingLevel.Destination	= nullptr;
		GIncomingLevel.Teleportee	= nullptr;
		GIncomingLevel.bCopy		= false;
	}
}


/*-----------------------------------------------------------------------------
    Game deinitialization.
-----------------------------------------------------------------------------*/

//
// Exit the game.
//
void CGame::Exit()
{
	// Shutdown project, if any.
	if( Project )
	{
		// But first of all - kill current level.
		if( Level )
		{
			assert(Level->bIsPlaying);
			Level->EndPlay();
			if( Level->IsTemporal() )
				DestroyObject( Level );
		}

		// Kill entire project.
		delete Project;
	}

	// Delete console.
	freeandnil(Console);
	freeandnil(CConsole::Font);

	// Shutdown subsystems.
	freeandnil(GInput);
	freeandnil(GAudio);
	freeandnil(GRender);
	freeandnil(Config);

	log( L"Game: Application shutdown" ); 
}


/*-----------------------------------------------------------------------------
    Game main loop.
-----------------------------------------------------------------------------*/

//
// Global timing variables.
//
static Double		GOldTime;
static Double		GfpsTime;
static Integer		GfpsCount;

//
// Exception handle variables.
//
static Char*		GErrorText;


//
// Loop, until exit.
//
void CGame::MainLoop()
{
	// Start time.
	GOldTime		= GPlat->TimeStamp();
	GfpsTime		= 0.0;
	GfpsCount		= 0;
	
	// Entry point.
#ifndef _DEBUG
	__try
#endif
	{
		for( ; ; )
		{	
			// Here we expressly reduce FPS, to not eat too
			// many CPU time.
			if( FPS > 48 )
			{	
				// 175 FPS maximum.
				Sleep(1000 / 175);
			}

			// Compute 'now' time.
			Double Time			= GPlat->TimeStamp();
			Double DeltaTime	= Time - GOldTime;
			GOldTime			= Time;

			// Count FPS stats.
			GFrameStamp++;
			GfpsCount++;
			GfpsTime += DeltaTime;
			if( GfpsTime > 1.0 )
			{
				FPS			= GfpsCount;
				GfpsTime	= 0.0;
				GfpsCount	= 0;	
			}

			// Update the client.
			Tick( (Float)DeltaTime );
			
			// Process joystick input.
			if( !(GFrameStamp & 3) )
				JoystickTick();

			// Process incoming messages.
			MSG	Msg;
			while( PeekMessage( &Msg, 0, 0, 0, PM_REMOVE ) )
			{
				if( Msg.message == WM_QUIT )
					goto ExitLoop;

				TranslateMessage( &Msg );
				DispatchMessage( &Msg );
			}
		} 

	ExitLoop:;
	}
#ifndef _DEBUG
	__except( HandleException(GetExceptionInformation()) )
	{
		// GPF Error.
		error( L"General protection fault in '%s'", GErrorText );
	}
#endif
}


/*-----------------------------------------------------------------------------
    Game WndProc.
-----------------------------------------------------------------------------*/

//
// Process a game window messages.
//
LRESULT CALLBACK WndProc( HWND HWnd, UINT Message, WPARAM WParam, LPARAM LParam )
{
	static Bool bCapture = false;

	switch( Message )
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			//
			// Key has been pressed.
			//
			if( !GGame->Console->IsActive() )
				GGame->GInput->OnKeyDown( (Integer)WParam );
			if(  WParam==VK_ESCAPE && GGame->Project && GGame->Project->Info->bQuitByEsc )
				SendMessage( GGame->hWnd, WM_CLOSE, 0, 0 );
			break;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			//
			// Key has been released.
			//
			if( !GGame->Console->IsActive() )
				GGame->GInput->OnKeyUp( (Integer)WParam );
			break;
		}
		case WM_MOUSEMOVE:
		{
			//
			// Mouse has been moved.
			//
			if( !GGame->Console->IsActive() )
			{
				static Integer OldX, OldY;
				Integer	X	= GET_X_LPARAM( LParam ), 
						Y	= GET_Y_LPARAM( LParam );

				if( OldX != X || OldY != Y )
				{
					FLevel*	Level		= GGame->Level;
					Float	WinWidth	= GGame->WinWidth;
					Float	WinHeight	= GGame->WinHeight;
					
					if( Level )
					{
						TVector	RealFOV		= Level->Camera->GetFitFOV( WinWidth, WinHeight );
						TVector	CamFOV		= Level->Camera->FOV;
						Float	lx			= 0.f,
								ly			= WinHeight*((RealFOV.Y-CamFOV.Y)/2.f)/RealFOV.Y,
								lw			= WinWidth,
								lh			= WinHeight*(CamFOV.Y/RealFOV.Y);
						Integer	TestX		= X,
								TestY		= Y-ly;

						// Test bounds.
						if( TestX>=0 && TestY>=0 && TestX<lw && TestY<lh )
						{
							// Screen cursor.
							GGame->GInput->MouseX		= TestX;
							GGame->GInput->MouseY		= TestY;

							// World cursor.
							GGame->GInput->WorldCursor	= TViewInfo
							(
								Level->Camera->Location,
								Level->Camera->Rotation,
								RealFOV,
								Level->Camera->Zoom,
								false,
								0.f, 0.f,
								WinWidth, WinHeight
							).Deproject( X, Y );
						}
					}

					OldX	= X;
					OldY	= Y;
				}
			}
			break;
		}
		case WM_SIZE:
		{
			//
			//  Window resizing.
			//
			GGame->WinWidth		= LOWORD(LParam);
			GGame->WinHeight	= HIWORD(LParam);

			GGame->GRender->Resize( GGame->WinWidth, GGame->WinHeight );
			break;
		}
		case WM_CHAR:
		{
			//
			// When user type some char.
			//
#if FLU_CONSOLE
			// Show or hide console.
			if( (Integer)WParam == CON_TOGGLE_BUTTON )
				GGame->Console->ShowToggle();
#endif
			if( GGame->Console->IsActive() )
			{
				if( WParam != CON_TOGGLE_BUTTON )
					GGame->Console->CharType((Char)WParam);
			}
			else
			{
				GGame->GInput->OnCharType( (Char)WParam );
			}
			break;
		}
		case WM_GETMINMAXINFO:
		{
			//
			// Tell minimum window size.
			//
			LPMINMAXINFO MMIPtr	= (LPMINMAXINFO)LParam;

			MMIPtr->ptMinTrackSize.x	= MIN_GAME_X;
			MMIPtr->ptMinTrackSize.y	= MIN_GAME_Y;
			break;
		}
		case WM_DESTROY:
		{
			//
			// When window destroing.
			//
			PostQuitMessage( 0 );
			break;
		}
		case WM_LBUTTONDBLCLK:
		{
			//
			// Left button double click.
			//
			if ( !GGame->Console->IsActive() )
			{
				GGame->GInput->OnKeyDown( KEY_DblClick );
				GGame->GInput->OnKeyUp( KEY_DblClick );
			}
			break;
		}
		case WM_MOUSEWHEEL:
		{
			//
			// Process mouse wheel event.
			//
			if ( !GGame->Console->IsActive() )
			{
				Integer SrlDlt	= GET_WHEEL_DELTA_WPARAM(WParam);
				Integer	Times	= Clamp( SrlDlt/120, -5, 5 );
				Integer	Key		= Times > 0 ? KEY_WheelUp : KEY_WheelDown;

				GGame->GInput->WheelScroll	+= SrlDlt;
				for( Integer i=0; i<Abs(Times); i++ )
				{
					GGame->GInput->OnKeyDown( Key );
					GGame->GInput->OnKeyUp( Key );
				}
			}	
			break;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			//
			// Some mouse button has been released.
			//
			if ( !GGame->Console->IsActive() )
			{
				Integer	Key	=	Message == WM_LBUTTONUP	?	KEY_LButton :
								Message == WM_RBUTTONUP	?	KEY_RButton :
															KEY_MButton;
				GGame->GInput->OnKeyUp( Key );

				if( bCapture && !(	GGame->GInput->KeyIsPressed(KEY_LButton) || 
									GGame->GInput->KeyIsPressed(KEY_RButton)) )
				{
					ReleaseCapture();
					bCapture	= false;
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{	
			//
			// Some mouse button has been pressed.
			//
			if ( !GGame->Console->IsActive() )
			{
				Integer	Key	=	Message == WM_LBUTTONDOWN	?	KEY_LButton :
								Message == WM_RBUTTONDOWN	?	KEY_RButton :
																KEY_MButton;
				GGame->GInput->OnKeyDown( Key );

				if( !bCapture )
				{
					SetCapture( HWnd );
					bCapture	= true;
				}
			}
			break;
		}
		case WM_ACTIVATE:
		{
			//
			// Window has activated or deactivated.
			//
			Bool bActive	= LOWORD(WParam) != WA_INACTIVE;

			if( !bActive && GGame->Level && !GGame->Project->Info->bNoPause )
				GGame->Level->bIsPause	= true;
			break;
		}
		default:
		{
			//
			// Default event processing.
			//
			return DefWindowProc( HWnd, Message, WParam, LParam );
		}
	}

	return 0;
}


/*-----------------------------------------------------------------------------
    Levels managment.
-----------------------------------------------------------------------------*/

//
// Run a level. if bCopy then level will duplicated.
// It's in most cases important to not affect source level.
// Will be weird to lost your enemies, coins, bonuses..
// Due to lose in previous game.
//
void CGame::RunLevel( FLevel* Source, Bool bCopy )
{
	assert(Source);

	// Shutdown previous level.
	if( Level )
	{
		assert(Level->bIsPlaying);
		Level->EndPlay();

		// If level is temporal - eliminate it.
		if( Level->IsTemporal() )
			DestroyObject( Level, true );

		Level	= nullptr;
	}

	// Duplicate level, if required.
	if( bCopy )
		Source	= Project->DuplicateLevel(Source);

	// Unload cache.
	Flush();

	// Let's play!
	Level	= Source;
	Level->RndFlags			= RND_Game;
	Level->BeginPlay();
	GInput->SetLevel( Level );

	// Notify.
	log( L"Game: Level '%s' running", *Level->GetName() );
}


//
// Find level by it name. If level not found 
// return nullptr. This function are case insensitive.
//
FLevel* CGame::FindLevel( String LevName )
{
	for( Integer i=0; i<LevelList.Num(); i++ )
		if(	String::UpperCase(LevelList[i]->GetName()) == 
			String::UpperCase(LevName) )
				return LevelList[i];

	return nullptr;
}


/*-----------------------------------------------------------------------------
    Console commands execution.
-----------------------------------------------------------------------------*/
	
//
// Retrieve the name of the file.
//
static String GetFileName( String FileName )
{
	Integer i, j;

	for( i=FileName.Len()-1; i>=0; i-- )
		if( FileName[i] == L'\\' )
			break;

	j = String::Pos( L".", FileName );
	return String::Copy( FileName, i+1, j-i-1 );
}


//
// Retrieve the directory of the file.
//
static String GetFileDir( String FileName )
{ 
	Integer i;
	for( i=FileName.Len()-1; i>=0; i-- )
		if( FileName[i] == L'\\' )
			break;

	return String::Copy( FileName, 0, i );
}


//
// Parse a word from a string.
//
static String ParseWord( Char*& Line )
{
	// Skip whitespace.
	while( *Line && *Line==' ' )
		Line++;

	// Parse word.
	Char Word[64]={}, *Walk=Word;
	while( *Line && *Line!=' ' )
		*Walk++	= *Line++;	

	return Word;
}


//
// Match a word with just parsed lexem. If missmatched
// return line pointer to the initial pos.
//
static Bool MatchWord( Char*& Line, Char* Test )
{
	Char* From = Line;

	// Skip whitespace.
	while( *Line && *Line==' ' )
		Line++;

	// Compare char by char.
	Bool Result = true;
	while( *Line && *Test )
	{
		if( towlower(*Line) != towlower(*Test) )
		{
			Result	= false;
			break;
		}
		Line++;
		Test++;
	}
	if( *Test )
		Result	= false;

	if( !Result )
		Line	= From;
	return Result;
}


//
// Main console command execution function.
//
void CGame::ConsoleExecute( String Cmd )
{
	Char* Line = *Cmd;

	if( MatchWord( Line, L"Clr" ) )
	{
		// Clear console.
		Console->Clear();
	}
	else if( MatchWord( Line, L"BlockMan" ) )
	{
		// Debug resource manager.
		if( Project )
			Project->BlockMan->DebugManager();
	}
	else if( MatchWord( Line, L"Quit" ) )
	{
		// Shutdown application.
		SendMessage( hWnd, WM_CLOSE, 0, 0 );
	}
	else if( MatchWord( Line, L"Flush" ) )
	{
		// Unload cached data.
		Flush();
	}
	else if( MatchWord( Line, L"Run" ) )
	{
		// Run a level.
		String	LevName = ParseWord(Line);
		FLevel*	Lev		= FindLevel(LevName);
		if( Lev )
			RunLevel( Lev, true );
		else
			log( L"Game: Level '%s' not found.", *LevName );
	}
	else if( MatchWord( Line, L"Restart" ) )
	{
		// Restart a currect level.
		if( Level && Level->IsTemporal() )
		{
			RunLevel( Level->Original, true );
		}
		else
			log( L"Game: Current level is not restartable" );
	}
	else if( MatchWord( Line, L"Hash" ) )
	{
		// Collision hash info.
		if( Level )
			Level->CollHash->DebugHash();
	}
	else if( MatchWord( Line, L"RMode" ) )
	{
		// Change render mode.
		if( Level )
			Level->RndFlags	= String::LowerCase(ParseWord(Line))==L"editor" ? RND_Editor : RND_Game;
	}
	else if( MatchWord( Line, L"Lighting" ) )
	{
		// Toggle lighting.
		if( Level )
		{
			Bool bOn = String::LowerCase(ParseWord(Line))==L"on";
			if( bOn )
				Level->RndFlags |= RND_Lighting;
			else
				Level->RndFlags &= ~RND_Lighting;
		}
	}
	else if( MatchWord( Line, L"GFX" ) )
	{
		// Toggle post effect.
		if( Level )
		{
			Bool bOn = String::LowerCase(ParseWord(Line))==L"on";
			if( bOn )
				Level->RndFlags |= RND_Effects;
			else
				Level->RndFlags &= ~RND_Effects;
		}
	}
	else if( MatchWord( Line, L"Pause" ) )
	{
		// Toggle level pause.
		if( Level )
		{
			Level->bIsPause	^= 1;
			Console->LogCallback( Level->bIsPause ? L"Pause On" : L"Pause Off", TCR_Yellow );
		}
	}
	else if( MatchWord( Line, L"GPF" ) )
	{
		// Crash application!
		*((Integer*)0)	= 1720;
	}
	else if( Level && Level->bIsPlaying )
	{
		// Try to execute for entity.
		String	Cmd	= String::LowerCase(ParseWord(Line)),
				Arg	= String::LowerCase(ParseWord(Line));

		for( Integer iEntity=0; iEntity<Level->Entities.Num(); iEntity++ )
			Level->Entities[iEntity]->CallEvent( EVENT_OnProcess, Cmd, Arg );
	}
	else
	{
		// Bad command.
		log( L"Game: Unrecognized Command '%s'", *Cmd );
	}
}


/*-----------------------------------------------------------------------------
    Game utility.
-----------------------------------------------------------------------------*/

//
// Set an game window caption.
//
void CGame::SetCaption( String NewCaption )
{
	SetWindowText( hWnd, *NewCaption );
}


//
// Resize an application window.
//
void CGame::SetSize( Integer NewWidth, Integer NewHeight, EAppWindowType NewType )
{
	// Device caps.	
	Integer	ScreenWidth		= GetSystemMetrics(SM_CXSCREEN);
	Integer	ScreenHeight	= GetSystemMetrics(SM_CYSCREEN);
	
	// Clamp size.
	NewWidth	= Clamp( NewWidth, MIN_GAME_X, ScreenWidth );
	NewHeight	= Clamp( NewHeight, MIN_GAME_Y, ScreenHeight );

	switch( NewType )
	{
		case WT_Sizeable:
		{
			//
			// Regular sizeable window.
			//
			SetFocus( hWnd );
			SetWindowLong
			(
				hWnd, 
				GWL_STYLE, 
				(WS_OVERLAPPEDWINDOW | GetWindowLong( hWnd, GWL_STYLE )) & ~WS_MAXIMIZE
			);
			SetWindowPos
			( 
				hWnd, 
				HWND_NOTOPMOST, 
				(ScreenWidth-NewWidth)/2, (ScreenHeight-NewHeight)/2, 
				NewWidth, NewHeight, 
				SWP_FRAMECHANGED
			);
			break;
		}
		case WT_Single:
		{
			//
			// Regular non-sizeable window.
			//
			SetFocus( hWnd );
			SetWindowLong
			(
				hWnd, 
				GWL_STYLE, 
				(WS_OVERLAPPEDWINDOW | GetWindowLong( hWnd, GWL_STYLE )) & ~(WS_MAXIMIZE|WS_SIZEBOX|WS_MAXIMIZEBOX)
			);
			SetWindowPos
			( 
				hWnd, 
				HWND_NOTOPMOST, 
				(ScreenWidth-NewWidth)/2, (ScreenHeight-NewHeight)/2, 
				NewWidth, NewHeight, 
				SWP_FRAMECHANGED
			);
			break;
			break;
		}
		case WT_FullScreen:
		{
			//
			// Fullscreen application.
			//
			SetFocus( hWnd );
			SetForegroundWindow( hWnd );
			SetWindowPos
			( 
				hWnd, 
				HWND_TOPMOST, 
				0, 0, 
				0, 0, 
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
			);
			SetWindowLong
			(
				hWnd, 
				GWL_STYLE, 
				WS_MAXIMIZE | WS_VISIBLE
			);
			SetWindowPos
			( 
				hWnd, HWND_TOPMOST, 
				0, 0, 
				ScreenWidth, ScreenHeight, 
				SWP_FRAMECHANGED 
			);
			break;
		}	
	}
}


/*-----------------------------------------------------------------------------
    CResourceStream.
-----------------------------------------------------------------------------*/

//
// Stream to load resource.
//
class CResourceStream: public CSerializer
{
public:
	// CResourceStream interface.
	CResourceStream( HINSTANCE hInstance, LPCTSTR ResID, LPCTSTR ResType )
	{
		hResInfo	= FindResource( hInstance, ResID, ResType );
		assert(hResInfo != 0);
		hGlobal		= LoadResource( hInstance, hResInfo );
		assert(hGlobal != 0);

		Size		= SizeofResource( hInstance, hResInfo );
		Memory		= LockResource(hGlobal);
		Mode		= SM_Load;
		Pos			= 0;
	}
	~CResourceStream()
	{
		UnlockResource(hGlobal);
		FreeResource(hGlobal);
	}

	// CSerializer interface.
	void SerializeData( void* Mem, DWord Count )
	{
		MemCopy( Mem, (Byte*)Memory + Pos, Count );
		Pos	+= Count;
	}
	void SerializeRef( FObject*& Obj )
	{
		error(L"CResourceStream::SerializeRef");
	}
	DWord TotalSize()
	{
		return Size;
	}
	void Seek( DWord NewPos )
	{
		Pos	= NewPos;
	}
	DWord Tell()
	{
		return Pos;
	}

private:
	// Internal.
	HRSRC				hResInfo;
	HGLOBAL				hGlobal;
	void*				Memory;
	Integer				Size, Pos;
};


/*-----------------------------------------------------------------------------
    Resources loading.
-----------------------------------------------------------------------------*/

// 
// Load bitmap.
//
static TStaticBitmap* LoadBitmapFromResource( LPCTSTR ResID )
{
	CResourceStream		Stream( GGame->hInstance, ResID, RT_BITMAP );

	BITMAPINFOHEADER	Info;
	Stream.SerializeData( &Info, sizeof(BITMAPINFOHEADER) );
	assert(Info.biBitCount==8);

	// Preinitialize bitmap.
	TStaticBitmap* Bitmap = new TStaticBitmap();
	Bitmap->Format		= BF_Palette8;
	Bitmap->USize		= Info.biWidth;
	Bitmap->VSize		= Info.biHeight;
	Bitmap->UBits		= IntLog2(Bitmap->USize);
	Bitmap->VBits		= IntLog2(Bitmap->VSize);
	Bitmap->Filter		= BFILTER_Nearest;
	Bitmap->BlendMode	= BLEND_Regular;
	Bitmap->RenderInfo	= -1;
	Bitmap->PanUSpeed	= 0.f;
	Bitmap->PanVSpeed	= 0.f;
	Bitmap->Saturation	= 1.f;
	Bitmap->AnimSpeed	= 0.f;
	Bitmap->bDynamic	= false;
	Bitmap->bRedrawn	= false;
	Bitmap->Data.SetNum( Bitmap->USize*Bitmap->VSize*sizeof(Byte) );

	// Load palette.
	RGBQUAD RawPalette[256];
	Stream.SerializeData( RawPalette, sizeof(RGBQUAD)*Info.biClrUsed );

	Bitmap->Palette.Allocate(Info.biClrUsed);
	for( Integer i=0; i<Info.biClrUsed; i++ )
	{
		TColor Col( RawPalette[i].rgbRed, RawPalette[i].rgbGreen, RawPalette[i].rgbBlue, 0xff );
		if( Col == MASK_COLOR )	Col.A	= 0x00;
		Bitmap->Palette.Colors[i]	= Col;
	}

	// Load data, don't forget V-flip.
	Byte* Data = (Byte*)Bitmap->GetData();
	for( Integer V=0; V<Bitmap->VSize; V++ )
		Stream.SerializeData
		(
			&Data[(Bitmap->VSize-1-V) * Bitmap->USize],
			Bitmap->USize * sizeof(Byte)
		);

	// Return it.
	return Bitmap;
}


//
// Load font and it bitmap.
//
static TStaticFont* LoadFontFromResource( LPCTSTR FontID, LPCTSTR BitmapID )
{
	CResourceStream		Stream( GGame->hInstance, FontID, L"FLUFONT" );

	AnsiChar* Text = (AnsiChar*)MemAlloc(Stream.TotalSize());
	AnsiChar* Walk = Text, *End = Text + Stream.TotalSize(); 
	Stream.SerializeData( Text, Stream.TotalSize() );
	#define to_next { while(*Walk != '\n') Walk++; Walk++; }

	TStaticFont* Font = new TStaticFont();

	// Main line.
	AnsiChar Name[64];
	Integer Height;
	sscanf( Walk, "%d %s\n", &Height, Name );	
	to_next;

	// Read characters.
	Font->Height = -1; 
	Integer NumPages = 0;
	while( Walk < End )
	{
		Char C[2] = { 0, 0 };
		Integer X, Y, W, H, iBitmap;
		C[0] = *Walk++;

		if( (Word)C[0]+1 > Font->Remap.Num() )
			Font->Remap.SetNum( (Word)C[0]+1 );

		sscanf( Walk, "%d %d %d %d %d\n", &iBitmap, &X, &Y, &W, &H );
		to_next;

		NumPages = Max( NumPages, iBitmap+1 );

		TGlyph Glyph;
		Glyph.iBitmap	= iBitmap;
		Glyph.X			= X;
		Glyph.Y			= Y;
		Glyph.W			= W;
		Glyph.H			= H;	

		Font->Height			= Max( Font->Height, H );
		Font->Remap[(Word)C[0]] = Font->Glyphs.Push(Glyph);

#if 0
		log( L"Import Char: %s", C );
#endif
	}

	// Load page.
	assert(NumPages == 1);
	FBitmap* Page = LoadBitmapFromResource(BitmapID);
	Page->BlendMode = BLEND_Translucent;
	Font->Bitmaps.Push( Page );

	MemFree(Text);
	#undef to_next

	return Font;
}


/*-----------------------------------------------------------------------------
    CWinPlatform implementation.
-----------------------------------------------------------------------------*/

//
// Window platform functions.
//
class CWinPlatform: public CPlatformBase
{
public:
	// Setup platform functions.
	CWinPlatform()
	{	
		// Timing.
		LARGE_INTEGER LInt;
		if( !QueryPerformanceFrequency(&LInt) )
			error( L"'QueryPerformanceFrequency' failed." );
		SecsPerCycle = 1.0 / (Double)LInt.QuadPart;
	}

	// Return current time.
	Double TimeStamp()
	{
		LARGE_INTEGER LInt;
		QueryPerformanceCounter( &LInt );
		return (Double)LInt.QuadPart * SecsPerCycle;
	}

	// Return time of the current frame.
	// It's more optimized than TimeStamp().
	Double Now()
	{
		return GOldTime;
	}

	// Return CPU cycles, used for benchmark.
	DWord Cycles()
	{
		LARGE_INTEGER Cyc;
		QueryPerformanceCounter(&Cyc);
		return Cyc.LowPart;
	}

	// Whether file exists?
	Bool FileExists( String FileName )
	{
		FILE* File = _wfopen( *FileName, L"rb" );
		if( File )
		{
			fclose(File);
			return true;
		}
		else
			return false;
	}

	// Whether directory exists?
	Bool DirectoryExists( String Dir )
	{
		DWord Type	= GetFileAttributes(*Dir);
		return	!(Type & INVALID_FILE_ATTRIBUTES) && 
				(Type & FILE_ATTRIBUTE_DIRECTORY);
	}

	// Copy text to clipboard.
	void ClipboardCopy( Char* Str ) 
	{
		if( OpenClipboard(GetActiveWindow()) )
		{
			Integer Size = (wcslen(Str)+1) * sizeof(Char);
			HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE, Size );
			MemCopy( GlobalLock(hMem), Str, Size );
			GlobalUnlock( hMem );
			EmptyClipboard();
			SetClipboardData( CF_UNICODETEXT, hMem );
			CloseClipboard();
		}
	}

	// Paste text from the clipboard.
	String ClipboardPaste()
	{
		String Text;
		if( OpenClipboard(GetActiveWindow()) )
		{
			HGLOBAL hMem = GetClipboardData( CF_UNICODETEXT );

			if( hMem )
			{
				Char* Data = (Char*)GlobalLock(hMem);
				if( Data )
				{
					Text	= Data;
					GlobalUnlock(hMem);
				}
			}
			CloseClipboard();
		}
		return Text;
	}

	// Launch application or url in internet.
	void Launch( const Char* Target, const Char* Parms )
	{
		ShellExecute( nullptr, L"open", Target, Parms?Parms:L"", L"", SW_SHOWNORMAL );
	}

private:
	// Internal variables.
	Double		SecsPerCycle;
};


// Make instance.
static	CWinPlatform	WinPlat;
static	CPlatformBase*	_WinPlatPtr = GPlat = &WinPlat;


/*-----------------------------------------------------------------------------
    CGameDebugOutput.
-----------------------------------------------------------------------------*/

#pragma optimize ( "", off )


//
// Calling stack trace.
//
static Char* StackTrace( LPEXCEPTION_POINTERS InException )	
{
	static Char Text[2048];
	MemZero( Text, sizeof(Text) );
	DWORD64 Offset64 = 0;
	DWORD Offset	 = 0;

	// Prepare.
	HANDLE	Process		= GetCurrentProcess();
	HANDLE	Thread		= GetCurrentThread();

	// Symbol info.
	SYMBOL_INFO* Symbol	= (SYMBOL_INFO*)MemAlloc( sizeof(SYMBOL_INFO) + 1024 );
	Symbol->SizeOfStruct	= sizeof(SYMBOL_INFO);
	Symbol->MaxNameLen		= 1024;
	DWORD SymOptions		= SymGetOptions();
	SymOptions				|= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_EXACT_SYMBOLS;
	SymSetOptions( SymOptions );
	SymInitialize( Process, ".", 1 );

	// Line info.
	IMAGEHLP_LINE64 Line;
	MemZero( &Line, sizeof(IMAGEHLP_LINE64) );
	Line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	// Setup frame info.
	STACKFRAME64 StackFrame;
	MemZero( &StackFrame, sizeof(STACKFRAME64) );

	if( InException )
	{
		StackFrame.AddrStack.Offset	= InException->ContextRecord->Esp;
		StackFrame.AddrFrame.Offset	= InException->ContextRecord->Ebp;
		StackFrame.AddrPC.Offset	= InException->ContextRecord->Eip;
	}
	else
	{
		__asm
		{ 
		Label: 
			mov dword ptr [StackFrame.AddrStack.Offset], esp
			mov dword ptr [StackFrame.AddrFrame.Offset], ebp
			mov eax, [Label]
			mov dword ptr [StackFrame.AddrPC.Offset], eax
		}
	}

	StackFrame.AddrPC.Mode		= AddrModeFlat;
	StackFrame.AddrStack.Mode	= AddrModeFlat;
	StackFrame.AddrFrame.Mode	= AddrModeFlat;
	StackFrame.AddrBStore.Mode	= AddrModeFlat;
	StackFrame.AddrReturn.Mode	= AddrModeFlat;
		
	// Walk the stack.
	for( ; ; )
	{
		if( !StackWalk64
					( 
						IMAGE_FILE_MACHINE_I386, 
						Process, 
						Thread, 
						&StackFrame, 
						InException ? InException->ContextRecord : nullptr,
						nullptr, 
						SymFunctionTableAccess64, 
						SymGetModuleBase64, 
						nullptr ) 
					)
			break;

		if( SymFromAddr( Process, StackFrame.AddrPC.Offset, &Offset64, Symbol ) && 
			SymGetLineFromAddr64( Process, StackFrame.AddrPC.Offset, &Offset, &Line ) )
		{
			Char FileName[1024];
			Char FuncName[256];
			mbstowcs( FileName, Line.FileName, 1024 );
			mbstowcs( FuncName, Symbol->Name, 256 );

			// Add to history.
			wcscat( Text, FuncName );
			wcscat( Text, L" <- " );

			// Output more detailed information into log.
			log( L"%s [File: %s][Line: %d]", FuncName, FileName, Line.LineNumber );
		}
	}

	MemFree( Symbol );
	return Text;
}
#pragma optimize ( "", on )


//
// __try..__except exception handler.
//
static Integer HandleException( LPEXCEPTION_POINTERS InException )
{
	GErrorText = StackTrace( InException );
	return EXCEPTION_EXECUTE_HANDLER;
}


//
// Game debug output.
//
class CGameDebugOutput: public CDebugOutputBase
{
public:
	// Output constructor.
	CGameDebugOutput()
	{
#if FDEBUG_CONSOLE
		// Create console.
		_setmode( _fileno(stdout), _O_U16TEXT );
		AllocConsole();
		freopen( "CONOUT$", "w", stdout );
#endif
#if FDEBUG_LOG
		// Open log file.
		LogFile	= _wfopen( *(String(FLU_NAME)+L".log"), L"w" );
#endif
	}

	// Output destructor.
	~CGameDebugOutput()
	{
#if FDEBUG_CONSOLE
		FreeConsole();
#endif
#if FDEBUG_LOG
		fclose( LogFile );
#endif
	}

	// Output log message.
	void Logf( Char* Text, ... )
	{
		Char Dest[1024] = {};
		va_list ArgPtr;
		va_start( ArgPtr, Text );
		_vsnwprintf( Dest, 1024, Text, ArgPtr );
		va_end( ArgPtr );
#if FDEBUG_CONSOLE
		wprintf( Dest );
		wprintf( L"\n" );	
#endif
#if FDEBUG_LOG
		fwprintf( LogFile, Dest );
		fwprintf( LogFile, L"\n" );
#endif
#ifdef _DEBUG
		if( GGame && GGame->Console )
			GGame->Console->LogCallback( Dest, TCR_Gray );
#endif
	}

	// Show warning message.
	void Warnf( Char* Text, ... )
	{
		Char Dest[1024] = {};
		va_list ArgPtr;
		va_start( ArgPtr, Text );
		_vsnwprintf( Dest, 1024, Text, ArgPtr );
		va_end( ArgPtr );

		log( L"**WARNING: %s", Dest );
#if FDEBUG_LOG
		fflush( LogFile );
#endif
		MessageBox( 0, Dest, L"Warning", MB_OK | MB_ICONWARNING | MB_TASKMODAL );
	}

	// Raise fatal error.
	void Errorf( Char* Text, ... )
	{
		Char Dest[1024] = {};
		va_list ArgPtr;
		va_start( ArgPtr, Text );
		_vsnwprintf( Dest, 1024, Text, ArgPtr );
		va_end( ArgPtr );

		log( L"**CRITICAL ERROR: %s", Dest );
		String FullText = String::Format( L"%s\n\nHistory: %s", Dest, StackTrace(nullptr) );

		MessageBox( 0, *FullText, L"Critical Error", MB_OK | MB_ICONERROR | MB_TASKMODAL );

#if FDEBUG_LOG
		fflush( LogFile );
#endif    
		ExitProcess( 0 );	
	}

	// Raise script error. It's not fatal, but should
	// be abstract at core level.
	void ScriptErrorf( Char* Text, ... ) 
	{
#ifdef _DEBUG
		Char Dest[1024] = {};
		va_list ArgPtr;
		va_start( ArgPtr, Text );
		_vsnwprintf( Dest, 1024, Text, ArgPtr );
		va_end( ArgPtr );

		if( GGame && GGame->Console )
			GGame->Console->LogCallback( Dest, TCR_Red );
#endif
	}


#if FDEBUG_LOG
private:
	// Output internal.
	FILE*	LogFile;
#endif
};


// Initialize debug output.
static	CGameDebugOutput	DebugOutput;
static	CDebugOutputBase*	_DebugOutputPtr = GOutput = &DebugOutput;


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/