/*=============================================================================
    Game.h: Game general include file.
    Copyright Nov.2016 Vlad Gordienko.
=============================================================================*/
#ifndef _FLU_GAME_
#define _FLU_GAME_

// C++ includes.
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
   
#pragma pack( push, 8 )
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#pragma pack( pop ) 

#undef DrawText
#undef LoadBitmap
#undef FindText

// Flu includes.
#include "..\Engine\Engine.h"
#include "..\Render\OpenGL\OpenGLRend.h"
#include "..\Audio\OpenAL\OpenALAud.h"

// Game includes.
#include "FrConsole.h"
#include "FrGame.h"


#endif
/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/