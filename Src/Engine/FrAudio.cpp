/*=============================================================================
    FrAudio.cpp: Audio implementation.
    Copyright Aug.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FMusic implementation.
-----------------------------------------------------------------------------*/

//
// Music constructor.
//
FMusic::FMusic()
	:	FResource()
{
}


//
// Music destructor.
//
FMusic::~FMusic()
{
}


//
// Initialize music after loading.
//
void FMusic::PostLoad()
{
	FResource::PostLoad();

	// Track file is exists?
	if( !GPlat->FileExists(GDirectory+L"\\"+FileName) )
		log( L"Music: Track '%s' not found", *FileName );
}


/*-----------------------------------------------------------------------------
    FSound implementation.
-----------------------------------------------------------------------------*/

//
// Sound constructor.
//
FSound::FSound()
	:	FBlockResource(),
		AudioInfo( -1 )
{
}


//
// Sound destructor.
//
FSound::~FSound()
{
}


//
// Sound after loading.
//
void FSound::PostLoad()
{
	FBlockResource::PostLoad();

	// Mark sound as not registered.
	AudioInfo	= -1;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/