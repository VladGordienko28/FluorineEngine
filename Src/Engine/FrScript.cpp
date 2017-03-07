/*=============================================================================
    FrScript.cpp: Virtual machine implementation.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FScript implementation.
-----------------------------------------------------------------------------*/

//
// Script constructor.
//
FScript::FScript()
	:	FResource()
{
	// Set defaults.
	bHasText		= false;
	iFamily			= -1;
	InstanceSize	= 0;
	InstanceBuffer	= nullptr;
	Thread			= nullptr;
	Base			= nullptr;
}


//
// Script destructor.
//
FScript::~FScript()
{
	// Kill extra components.
	for( Integer e=0; e<Components.Num(); e++ )
		DestroyObject( Components[e], false );

	// Kill the base.
	DestroyObject( Base, false );

	// Destroy the instance buffer.
	if( bHasText )
	{
		assert(InstanceBuffer);
		delete InstanceBuffer;
	}

	// Script objects.
	for( Integer i=0; i<Functions.Num(); i++ )
		delete Functions[i];
	for( Integer i=0; i<Properties.Num(); i++ )
		delete Properties[i];
	for( Integer i=0; i<Enums.Num(); i++ )
		delete Enums[i];

	// kill the thread.
	freeandnil(Thread);

	// Release tables.
	Text.Empty();
	Enums.Empty();
	Properties.Empty();
	Functions.Empty();
	Events.Empty();
	ResTable.Empty();
}


//
// Find an extra component by it's name.
// If component not found return nullptr.
//
FComponent* FScript::FindComponent( String InName )
{
	for( Integer e=0; e<Components.Num(); e++ )
		if( Components[e]->GetName() == InName )
			return Components[e];

	return nullptr;
}


//
// When some field changed in resource.
//
void FScript::EditChange()
{
	FResource::EditChange();
}


//
// Initialize script after loading.
//
void FScript::PostLoad()
{
	FResource::PostLoad();
}


/*-----------------------------------------------------------------------------
    Script import & export.
-----------------------------------------------------------------------------*/

//
// Import the script.
//
void FScript::Import( CImporterBase& Im )
{
	FResource::Import( Im );

	// Components and instance buffer, handled
	// in the file importer code.

	// Script text stored in another file, and
	// loaded by the file importer too.

	IMPORT_BOOL( bHasText );
	//IMPORT_STRING( iFamily );

	// All other fields and script objects
	// database will be restored after full 
	// compilation.
}


//
// Export the script.
//
void FScript::Export( CExporterBase& Ex )
{
	FResource::Export( Ex );

	// Components and instance buffer, handled
	// in the file exporter code.

	// Script text stored in another file, and
	// saved by the file exporter too.

	EXPORT_BOOL( bHasText );
	//EXPORT_STRING( iFamily );

	// All other fields and script objects
	// database will be restored after full 
	// compilation.
}


#if 0
/*-----------------------------------------------------------------------------
    Probability optimization for events searching.
-----------------------------------------------------------------------------*/

//
// Events reordering threshold. After 
// EVENTS_REODER_THRESH events will be sorted by
// popularity.
//
#define EVENTS_REODER_THRESH	20000

//
// Event popularity comparison.
//
Bool EventsCmp( CFunction* const &A, CFunction* const &B )
{
	return A->Popularity > B->Popularity;
}


//
// Sort script events by popularity.
//
void FScript::ReorderEvents()
{
	Events.Sort(EventsCmp);
#if 0
	log( L"Reorder in '%s'", *GetName() );
	for( Integer i=0; i<Events.Num(); i++ )
		log( L" - %s %d", *Events[i]->Name, Events[i]->Popularity );
#endif
}


//
// Find script event by it name.
//
CFunction* FScript::FindEvent( const Char* InName )
{
	for( Integer i=0; i<Events.Num(); i++ )
		if( Events[i]->Name == InName )
		{
			CFunction* E = Events[i];
			E->Popularity++;

			// It is time for reordering?
			if( ++CallCounter > EVENTS_REODER_THRESH )
			{
				CallCounter	= 0;
				ReorderEvents();
			}

			// Return it.
			return E;
		}

	// Not found.
	return nullptr;
}
#endif

/*-----------------------------------------------------------------------------
    Script serialization.
-----------------------------------------------------------------------------*/

// Little hack, here we declare global script variable, its used in
// serialization of script objects, such as variables or functions,
// it's will be cool to use another non-standard signature of 
// 'Serialize' function, but I can't do it, since this functions
// will be used in TArray<T> serialization implicitly.
static FScript*	GScript;


//
// Serialize an enumeration.
//
void Serialize( CSerializer& S, CEnum*& V )
{
	if( S.GetMode() == SM_Load )
	{
		freeandnil(V);
		V	= new CEnum( L"", GScript );
	}

	Serialize( S, V->Name );
	Serialize( S, V->Flags );
	Serialize( S, V->Elements );
	Serialize( S, V->Aliases );
}


//
// Find an enumeration in list by it name.
//
CEnum* FindEnumIn( FScript* Script, const Char* Name )
{
	for( Integer i=0; i<Script->Enums.Num(); i++ )
		if( Script->Enums[i]->Name == Name )
			return Script->Enums[i];

	return nullptr;
}


//
// Serialize a script property.
//
void Serialize( CSerializer& S, CProperty*& V )
{
	if( S.GetMode() == SM_Load )
	{
		freeandnil(V);
		V	= new CProperty();
	}

	Serialize( S, V->Name );
	Serialize( S, V->Alias );
	Serialize( S, V->Flags );
	Serialize( S, V->Offset );
	SerializeEnum( S, V->Type );
	Serialize( S, V->ArrayDim );

	// Save extra information about type.
	if( V->Type == TYPE_Entity )
	{
		// Save also script of entity variable.
		Serialize( S, V->Script );
	} 
	else if( V->Type == TYPE_Byte )
	{
		// Serialize enumeration.
		if( S.GetMode() == SM_Load )
		{
			Byte Status;
			Serialize( S, Status );
			if( Status != 0 )
			{
				String EnumName;
				Serialize( S, EnumName );
				V->Enum		= (Status==1) ? CClassDatabase::StaticFindEnum(*EnumName) : FindEnumIn( GScript, *EnumName );
			}
			else
				V->Enum		= nullptr;
		}
		else
		{
			CEnum*	Enum = V->Enum;
			Byte Status = Enum ? (Enum->Flags&ENUM_Native) ? 1 : 2 : 0;
			Serialize( S, Status );
			if( Status != 0 )
				Serialize( S, Enum->Name );
		}
	}
	else if( V->Type == TYPE_Resource )
	{
		// Serialize also class of resource.
		if( S.GetMode() == SM_Load )
		{
			Byte Status;
			Serialize( S, Status );
			if( Status )
			{
				String ClassName;
				Serialize( S, ClassName );
				V->Class	= CClassDatabase::StaticFindClass(*ClassName);
			}
			else
				V->Class	= nullptr;
		}
		else if( S.GetMode() == SM_Save )
		{
			Byte Status = V->Class ? 1 : 0;
			Serialize( S, Status );
			if( Status )
				Serialize( S, V->Class->Name );
		}
	}
}


//
// Serialize a script function.
//
void Serialize( CSerializer& S, CFunction*& V )
{
	if( S.GetMode() == SM_Load )
	{
		freeandnil(V);
		V	= new CFunction();
	}

	Serialize( S, V->Name );
	Serialize( S, V->Flags ); 
	Serialize( S, V->Locals );
	Serialize( S, V->FrameSize );
	Serialize( S, V->ParmsCount );
	Serialize( S, V->Code );

	// Result variables.
	if( V->Flags & FUNC_HasResult )
	{
		if( S.GetMode() == SM_Load )
		{
			Integer Index;
			Serialize( S, Index );
			V->ResultVar	= (Index!=-1) ? V->Locals[Index] : nullptr;
		}
		else if( S.GetMode() == SM_Save )
		{
			Integer Index = V->ResultVar ? V->Locals.FindItem(V->ResultVar) : -1;
			Serialize( S, Index );
		}
	}
}


//
// Serialize a thread label.
//
void Serialize( CSerializer& S, CThreadCode::TLabel& V )
{
	Serialize( S, V.Address );
	Serialize( S, V.Name );
}


//
// Serialize a script thread.
//
void Serialize( CSerializer& S, CThreadCode*& V )
{
	if( S.GetMode() == SM_Load )
	{
		freeandnil(V);
		Bool bUsed;
		Serialize( S, bUsed );
		if( bUsed )
		{
			V	= new CThreadCode();
			Serialize( S, V->Labels );
			Serialize( S, V->Code );
		}
	}
	else
	{
		Bool bUsed = V != nullptr;
		Serialize( S, bUsed );
		if( bUsed )
		{
			Serialize( S, V->Labels );
			Serialize( S, V->Code );
		}
	}
}


//
// Serialize script.
//
void FScript::SerializeThis( CSerializer& S )
{
	// Call parent.
	FResource::SerializeThis( S );

	// Set self.
	GScript		= this;

	// General variables.
	Serialize( S, bHasText );
	Serialize( S, Components );
	Serialize( S, Base );

	if( bHasText )
	{
		Serialize( S, iFamily );
		Serialize( S, ResTable );
		Serialize( S, InstanceSize );

		if( S.GetMode() == SM_Undefined )
			Serialize( S, Text );

		// Enumerations.
		Serialize( S, Enums );

		// Properties.
		Serialize( S, Properties );

		// Functions.
		Serialize( S, Functions );

		// Events and VF table.
		if( S.GetMode() == SM_Load )
		{
			Integer NumEvents, NumVF;
			Serialize( S, NumEvents );
			Serialize( S, NumVF );
			assert(_EVENT_MAX == NumEvents);

			VFTable.Empty();
			Events.Empty();
			VFTable.SetNum(NumVF);
			Events.SetNum(NumEvents);

			for( Integer i=0; i<NumEvents; i++ )
			{
				Integer Tmp;
				Serialize( S, Tmp );
				Events[i]	= Tmp != -1 ? Functions[Tmp] : nullptr;
			}
			for( Integer i=0; i<NumVF; i++ )
			{
				Integer Tmp;
				Serialize( S, Tmp );
				VFTable[i]	= Tmp != -1 ? Functions[Tmp] : nullptr;
			}
		}
		else if( S.GetMode() == SM_Save )
		{
			Integer NumEvents	= Events.Num(),
					NumVF		= VFTable.Num();
			Serialize( S, NumEvents );
			Serialize( S, NumVF );

			for( Integer i=0; i<NumEvents; i++ )
			{
				Integer Tmp	= Functions.FindItem(Events[i]);
				Serialize( S, Tmp );
			}
			for( Integer i=0; i<NumVF; i++ )
			{
				Integer Tmp	= Functions.FindItem(VFTable[i]);
				Serialize( S, Tmp );
			}
		}

		// Serialize thread.
		Serialize( S, Thread );

		// And finally instance buffer.
		if( S.GetMode() == SM_Load )
		{
			freeandnil(InstanceBuffer);
			InstanceBuffer	= new CInstanceBuffer( this );
			InstanceBuffer->Data.SetNum( InstanceSize );
		}
		InstanceBuffer->SerializeValues( S );
	}
}


/*-----------------------------------------------------------------------------
    Script objects implementation.
-----------------------------------------------------------------------------*/

//
// Bytecode constructor.
//
CBytecode::CBytecode()
{
	iLine	= -1;
	iPos	= -1;
}


//
// Bytecode destructor.
//
CBytecode::~CBytecode()
{
	Code.Empty();
}


//
// Thread code constructor.
//
CThreadCode::CThreadCode()
	:	CBytecode()
{
}


//
// Find a label by it's name.
// Returns label index, if not found return -1.
//
Integer CThreadCode::GetLabelId( const Char* InName )
{
	for( Integer i=0; i<Labels.Num(); i++ )
		if( Labels[i].Name == InName )
			return i;

	return -1;
}


//
// Add a new label. If label already exists
// break the app. Return index of the new label.
//
Integer CThreadCode::AddLabel( const Char* InName, Word InAddr )
{
	assert(GetLabelId(InName) == -1);

	TLabel Label;
	Label.Address	= InAddr;
	Label.Name		= InName;

	return Labels.Push( Label );
}


//
// CFunction constructor.
//
CFunction::CFunction()
	:	CBytecode()
{
	Flags		= FUNC_None;
	FrameSize	= 0;
	ParmsCount	= 0;
	Popularity	= 0;
	ResultVar	= nullptr;
}
 

//
// CFunction destructor.
//
CFunction::~CFunction()
{
	// Destroy local variables.
	for( Integer i=0; i<Locals.Num(); i++ )
		delete Locals[i];

	Locals.Empty();
}


//
// Return a function signature as string.
//
String CFunction::GetSignature() const
{
	String Args;
	for( Integer i=0; i<ParmsCount; i++ )
	{
		Args	+= Locals[i]->TypeName();
		if( i < (ParmsCount-1) )
			Args	+= L", ";
	}

	return	String::Format
	(
		L"%s %s(%s)",
		ResultVar ? *ResultVar->TypeName() : L"void",
		*Name,
		*Args 
	);
}


/*-----------------------------------------------------------------------------
    CInstanceBuffer implementation.
-----------------------------------------------------------------------------*/

//
// Instance buffer constructor.
//
CInstanceBuffer::CInstanceBuffer( FScript* InScript )
	:	Script( InScript ),
		Data()
{
	assert(Script);
}


//
// Instance buffer destructor.
//
CInstanceBuffer::~CInstanceBuffer()
{
	// Destroy own values here.
	DestroyValues();
}


//
// Destroy all own values.
//
void CInstanceBuffer::DestroyValues()
{
	for( Integer i=0; i<Script->Properties.Num(); i++ )
	{
		CProperty* P = Script->Properties[i];
		P->DestroyValues( &Data[P->Offset] );
	}
}


//
// Copy values from Source.
// Procedure doesn't reallocate buffer size.
// So set proper size somewhere else.
//
void CInstanceBuffer::CopyValues( void* Source )
{
	for( Integer i=0; i<Script->Properties.Num(); i++ )
	{
		CProperty* P = Script->Properties[i];
		P->CopyValues( &Data[P->Offset], &((Byte*)Source)[P->Offset] );
	}
}


//
// Export entire instance buffer values.
//
void CInstanceBuffer::ExportValues( CExporterBase& Ex )
{
	for( Integer i=0; i<Script->Properties.Num(); i++ )
	{
		CProperty* P = Script->Properties[i];
		P->Export( &Data[P->Offset], Ex );
	}
}


//
// Import entire instance buffer values.
//
void CInstanceBuffer::ImportValues( CImporterBase& Im )
{
	for( Integer i=0; i<Script->Properties.Num(); i++ )
	{
		CProperty* P = Script->Properties[i];
		P->Import( &Data[P->Offset], Im );
	}
}


//
// Serialize all values in buffer.
//
void CInstanceBuffer::SerializeValues( CSerializer& S )
{
	for( Integer i=0; i<Script->Properties.Num(); i++ )
	{
		CProperty* P = Script->Properties[i];
		P->SerializeValue( &Data[P->Offset], S );
	}
}


/*-----------------------------------------------------------------------------
    Registration.
-----------------------------------------------------------------------------*/	


REGISTER_CLASS_CPP( FScript, FResource, CLASS_Sterile )
{
	// All suffix operators.
	DECLARE_SUFFIX_OP( UN_Inc_Integer,	++,	TYPE_Integer,	TYPE_Integer );
	DECLARE_SUFFIX_OP( UN_Inc_Float,	++,	TYPE_Float,		TYPE_Float );
	DECLARE_SUFFIX_OP( UN_Dec_Integer,	--,	TYPE_Integer,	TYPE_Integer );
	DECLARE_SUFFIX_OP( UN_Dec_Float,	--, TYPE_Float,		TYPE_Float );

	// All unary operators.
	DECLARE_UNARY_OP( UN_Plus_Integer,	+,	TYPE_Integer,	TYPE_Integer );
	DECLARE_UNARY_OP( UN_Plus_Float,	+,	TYPE_Float,		TYPE_Float );
	DECLARE_UNARY_OP( UN_Plus_Vector,	+,	TYPE_Vector,	TYPE_Vector );
	DECLARE_UNARY_OP( UN_Plus_Color,	+,	TYPE_Color,		TYPE_Color );
	DECLARE_UNARY_OP( UN_Minus_Integer, -,	TYPE_Integer,	TYPE_Integer );
	DECLARE_UNARY_OP( UN_Minus_Float,	-,	TYPE_Float,		TYPE_Float );
	DECLARE_UNARY_OP( UN_Minus_Vector,	-,	TYPE_Vector,	TYPE_Vector );
	DECLARE_UNARY_OP( UN_Minus_Color,	-,	TYPE_Color,		TYPE_Color );
	DECLARE_UNARY_OP( UN_Not_Bool,		!,	TYPE_Bool,		TYPE_Bool );
	DECLARE_UNARY_OP( UN_Not_Integer,	~,	TYPE_Integer,	TYPE_Integer );

	// All binary operators.
	DECLARE_BIN_OP( BIN_Mult_Integer,		*,	11,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Mult_Float,			*,	11,		TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_OP( BIN_Mult_Color,			*,	11,		TYPE_Color,		TYPE_Color,		TYPE_Color );
	DECLARE_BIN_OP( BIN_Mult_Vector,		*,	11,		TYPE_Vector,	TYPE_Float,		TYPE_Vector );
	DECLARE_BIN_OP( BIN_Div_Integer,		/,	11,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Div_Float,			/,	11,		TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_OP( BIN_Mod_Integer,		%,	11,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Add_Integer,		+,	10,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Add_Float,			+,	10,		TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_OP( BIN_Add_Color,			+,	10,		TYPE_Color,		TYPE_Color,		TYPE_Color );
	DECLARE_BIN_OP( BIN_Add_String,			+,	10,		TYPE_String,	TYPE_String,	TYPE_String );
	DECLARE_BIN_OP( BIN_Add_Vector,			+,	10,		TYPE_Vector,	TYPE_Vector,	TYPE_Vector );
	DECLARE_BIN_OP( BIN_Sub_Integer,		-,	10,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Sub_Float,			-,	10,		TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_OP( BIN_Sub_Color,			-,	10,		TYPE_Color,		TYPE_Color,		TYPE_Color );
	DECLARE_BIN_OP( BIN_Sub_Vector,			-,	10,		TYPE_Vector,	TYPE_Vector,	TYPE_Vector );
	DECLARE_BIN_OP( BIN_Shr_Integer,		>>,	9,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Shl_Integer,		<<,	9,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Less_Integer,		<,	8,		TYPE_Integer,	TYPE_Integer,	TYPE_Bool );
	DECLARE_BIN_OP( BIN_Less_Float,			<,	8,		TYPE_Float,		TYPE_Float,		TYPE_Bool );
	DECLARE_BIN_OP( BIN_LessEq_Integer,		<=,	8,		TYPE_Integer,	TYPE_Integer,	TYPE_Bool );
	DECLARE_BIN_OP( BIN_LessEq_Float,		<=,	8,		TYPE_Float,		TYPE_Float,		TYPE_Bool );
	DECLARE_BIN_OP( BIN_Greater_Integer,	>,	8,		TYPE_Integer,	TYPE_Integer,	TYPE_Bool );
	DECLARE_BIN_OP( BIN_Greater_Float,		>,	8,		TYPE_Float,		TYPE_Float,		TYPE_Bool );
	DECLARE_BIN_OP( BIN_GreaterEq_Integer,	>=,	8,		TYPE_Integer,	TYPE_Integer,	TYPE_Bool );
	DECLARE_BIN_OP( BIN_GreaterEq_Float,	>=,	8,		TYPE_Float,		TYPE_Float,		TYPE_Bool );
	DECLARE_BIN_OP( BIN_And_Integer,		&,	6,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Xor_Integer,		^,	5,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Cross_Vector,		^,	11,		TYPE_Vector,	TYPE_Vector,	TYPE_Float );
	DECLARE_BIN_OP( BIN_Or_Integer,			|,	4,		TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_OP( BIN_Dot_Vector,			|,	11,		TYPE_Vector,	TYPE_Vector,	TYPE_Float );

	// All assignment operators.
	DECLARE_BIN_ASS_OP( BIN_AddEqual_Integer,	+=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_AddEqual_Float,		+=,	1,	TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_ASS_OP( BIN_AddEqual_Vector,	+=,	1,	TYPE_Vector,	TYPE_Vector,	TYPE_Vector );
	DECLARE_BIN_ASS_OP( BIN_AddEqual_String,	+=,	1,	TYPE_String,	TYPE_String,	TYPE_String );
	DECLARE_BIN_ASS_OP( BIN_AddEqual_Color,		+=,	1,	TYPE_Color,		TYPE_Color,		TYPE_Color );
	DECLARE_BIN_ASS_OP( BIN_SubEqual_Integer,	-=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_SubEqual_Float,		-=,	1,	TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_ASS_OP( BIN_SubEqual_Vector,	-=,	1,	TYPE_Vector,	TYPE_Vector,	TYPE_Vector );
	DECLARE_BIN_ASS_OP( BIN_SubEqual_Color,		-=,	1,	TYPE_Color,		TYPE_Color,		TYPE_Color );
	DECLARE_BIN_ASS_OP( BIN_MulEqual_Integer,	*=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_MulEqual_Float,		*=,	1,	TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_ASS_OP( BIN_MulEqual_Color,		*=,	1,	TYPE_Color,		TYPE_Color,		TYPE_Color );
	DECLARE_BIN_ASS_OP( BIN_DivEqual_Integer,	/=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_DivEqual_Float,		/=,	1,	TYPE_Float,		TYPE_Float,		TYPE_Float );
	DECLARE_BIN_ASS_OP( BIN_ModEqual_Integer,	%=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_ShlEqual_Integer,	<<=,1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_ShrEqual_Integer,	>>=,1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_AndEqual_Integer,	&=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_XorEqual_Integer,	^=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	DECLARE_BIN_ASS_OP( BIN_OrEqual_Integer,	|=,	1,	TYPE_Integer,	TYPE_Integer,	TYPE_Integer );
	
	// Standard native functions.
	DECLARE_FUNCTION( OP_Abs,				abs,			TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_ArcTan,			arctan,			TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_ArcTan2,			arctan2,		TYPE_Float,		TYPE_Float,		TYPE_Float,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Cos,				cos,			TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Sin,				sin,			TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Sqrt,				sqrt,			TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Distance,			distance,		TYPE_Float,		TYPE_Vector,	TYPE_Vector,	TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Ln,				ln,				TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Exp,				exp,			TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Frac,				frac,			TYPE_Float,		TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Round,				round,			TYPE_Integer,	TYPE_Float,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Normalize,			normalize,		TYPE_Vector,	TYPE_Vector,	TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Random,			random,			TYPE_Integer,	TYPE_Integer,	TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_RandomF,			randomf,		TYPE_Float,		TYPE_None,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_VectorSize,		vsize,			TYPE_Float,		TYPE_Vector,	TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_VectorToAngle,		vtoa,			TYPE_Angle,		TYPE_Vector,	TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_AngleToVector,		atov,			TYPE_Vector,	TYPE_Angle,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_RGBA,				rgba,			TYPE_Color,		TYPE_Byte,		TYPE_Byte,		TYPE_Byte, TYPE_Byte );
	DECLARE_FUNCTION( OP_IToS,				itos,			TYPE_String,	TYPE_Integer,	TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_CharAt,			charAt,			TYPE_String,	TYPE_String,	TYPE_Integer,	TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_IndexOf,			indexOf,		TYPE_Integer,	TYPE_String,	TYPE_String,	TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Execute,			execute,		TYPE_None,		TYPE_String,	TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_FUNCTION( OP_Now,				now,			TYPE_Float,		TYPE_None,		TYPE_None,		TYPE_None, TYPE_None );

	// Engine native functions.
	DECLARE_FUNCTION( OP_PlaySoundFX,		PlaySoundFX,	TYPE_None,		TYPE_SOUND,		TYPE_Float,		TYPE_Float,		TYPE_None );
	DECLARE_FUNCTION( OP_PlayMusic,			PlayMusic,		TYPE_None,		TYPE_MUSIC,		TYPE_Float,		TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_KeyIsPressed,		KeyIsPressed,	TYPE_Bool,		TYPE_Integer,	TYPE_None,		TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_GetCamera,			GetCamera,		TYPE_Entity,	TYPE_None,		TYPE_None,		TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_GetScreenCursor,	GetScreenCursor,TYPE_Vector,	TYPE_None,		TYPE_None,		TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_GetWorldCursor,	GetWorldCursor,	TYPE_Vector,	TYPE_None,		TYPE_None,		TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_Localize,			Localize,		TYPE_String,	TYPE_String,	TYPE_String,	TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_GetScript,			GetScript,		TYPE_SCRIPT,	TYPE_Entity,	TYPE_None,		TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_StaticPush,		StaticPush,		TYPE_None,		TYPE_String,	TYPE_String,	TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_StaticPop,			StaticPop,		TYPE_String,	TYPE_String,	TYPE_String,	TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_TravelTo,			TravelTo,		TYPE_None,		TYPE_LEVEL,		TYPE_Bool,		TYPE_None,		TYPE_None );
	DECLARE_FUNCTION( OP_FindEntity,		FindEntity,		TYPE_Entity,	TYPE_String,	TYPE_None,		TYPE_None,		TYPE_None );

	// Iterators.
	DECLARE_ITERATOR( IT_AllEntities,		AllEntities,		TYPE_None,		TYPE_SCRIPT,	TYPE_None,		TYPE_None,		TYPE_None );
	DECLARE_ITERATOR( IT_RectEntities,		RectEntities,		TYPE_None,		TYPE_SCRIPT,	TYPE_AABB,		TYPE_None,		TYPE_None );
	DECLARE_ITERATOR( IT_TouchedEntities,	TouchedEntities,	TYPE_None,		TYPE_None,		TYPE_None,		TYPE_None,		TYPE_None );

	return 0;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/