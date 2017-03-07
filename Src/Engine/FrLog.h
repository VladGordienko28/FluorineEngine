/*=============================================================================
    FrLog.h: An abstract output class.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    CDebugOutputBase.
-----------------------------------------------------------------------------*/

//
// An abstract output.
//
class CDebugOutputBase
{
public:
	// CDebugOutputBase interface.
	virtual void Logf( Char* Text, ... ) = 0;
	virtual void Errorf( Char* Text, ... ) = 0;
	virtual void Warnf( Char* Text, ... ) = 0;
	virtual void ScriptErrorf( Char* Text, ... ) = 0;
};

// Global instance.
extern CDebugOutputBase* GOutput;


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/