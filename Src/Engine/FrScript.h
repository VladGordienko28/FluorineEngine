/*=============================================================================
    FrScript.h: FScript class.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    Script objects.
-----------------------------------------------------------------------------*/

//
// An abstract code stream.
//
class CBytecode
{
public:
	// Variables.
	TArray<Byte>		Code;
	Integer				iLine	: 20;
	Integer				iPos	: 12;

	// CBytecode interface.
	CBytecode();
	virtual ~CBytecode();
};


//
// An entity thread code.
//
class CThreadCode: public CBytecode
{
public:
	// A label in the code.
	struct TLabel
	{
	public:
		Word		Address;
		String		Name;
	};

	// Variables.
	TArray<TLabel>		Labels;

	// CThreadCode interface.
	CThreadCode();
	Integer GetLabelId( const Char* InName );	
	Integer AddLabel( const Char* InName, Word InAddr );		
};


//
// Function flags.
//
#define FUNC_None			0x0000
#define FUNC_HasResult		0x0001
#define FUNC_Event			0x0002
#define FUNC_Unified		0x0004


//
// A script function.
//
class CFunction: public CBytecode
{
public:
	// Variables.
	String				Name;
	DWord				Flags;
	DWord				Popularity; 
	TArray<CProperty*>	Locals;
	DWord				FrameSize;
	Integer				ParmsCount;
	CProperty*			ResultVar;
 
	// CFunction interface.
	CFunction();
	~CFunction();
	String GetSignature() const;
};


/*-----------------------------------------------------------------------------
    CInstanceBuffer.
-----------------------------------------------------------------------------*/

//
// A script properties values holder class.
//
class CInstanceBuffer
{
public:
	// Variables.
	FScript*		Script;
	TArray<Byte>	Data;

	// CInstanceBuffer interface.
	CInstanceBuffer( FScript* InScript );
	~CInstanceBuffer();
	void DestroyValues();
	void CopyValues( void* Source );
	void ImportValues( CImporterBase& Im );
	void ExportValues( CExporterBase& Ex );
	void SerializeValues( CSerializer& S );
};


/*-----------------------------------------------------------------------------
    FScript.
-----------------------------------------------------------------------------*/

//
// A Script.
//
class FScript: public FResource
{
REGISTER_CLASS_H(FScript);
public:
	// Components, same as in FEntity.
	TArray<FExtraComponent*>	Components;
	FBaseComponent*				Base;
	CInstanceBuffer*			InstanceBuffer;

	// Script variables.
	Bool						bHasText;
	Integer						iFamily;
	TArray<String>				Text;
	TArray<CEnum*>				Enums;
	TArray<CProperty*>			Properties;
	TArray<CFunction*>			Functions;
	TArray<CFunction*>			Events;
	TArray<CFunction*>			VFTable;
	CThreadCode*				Thread;
	DWord						InstanceSize;

	// Table of resources uses in bytecode.
	TArray<FResource*>			ResTable;

	// FScript interface.
	FScript();
	~FScript();
	FComponent* FindComponent( String InName );

	// FObject interface.
	void SerializeThis( CSerializer& S );
	void EditChange();
	void PostLoad();
	void Import( CImporterBase& Im );
	void Export( CExporterBase& Ex );

	// FluScript functions.
	CFunction* FindEvent( const Char* InName );
	void ReorderEvents();
};


/*-----------------------------------------------------------------------------
    List of all events.
-----------------------------------------------------------------------------*/

//
// Make a list of events as enum.
//
#define SCRIPT_EVENT(name)	EVENT_##name,
enum EEventName
{
#include "FrEvent.h"
	_EVENT_MAX
};
#undef SCRIPT_EVENT


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/