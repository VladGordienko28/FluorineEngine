/*=============================================================================
    FrClass.h: Meta classes system.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    CEnum.
-----------------------------------------------------------------------------*/

//
// Enumeration flags.
//
#define ENUM_None			0x0000
#define ENUM_Native			0x0001


//
// A list of strings.
//
class CEnum
{
public:
	String			Name;
	DWord			Flags;
	TArray<String>	Elements;
	TArray<String>	Aliases;

	// CEnum interface.
	CEnum( const Char* InName );
	CEnum( const Char* InName, FScript* Script );
	~CEnum();
	Integer AddElement( String NewElem );
};


/*-----------------------------------------------------------------------------
    CProperty.
-----------------------------------------------------------------------------*/

// Static array maximum size.
#define STATIC_ARR_MAX		128

//
// Property flags.
//
#define PROP_None			0x0000		// Nothing.
#define PROP_Native			0x0001		// Its a native property.
#define PROP_Deprecated		0x0002		// Property should be killed.
#define PROP_Localized		0x0004		// Property should be localized.
#define PROP_Editable		0x0008		// Property is editable via ObjectInspector.
#define PROP_Const			0x0040		// Value is visible in ObjectInspector, but doesn't editable.
#define PROP_OutParm		0x0080		// Function output parameter.


//
// A property type.
//
enum EPropType
{
	TYPE_None,			// Invalid type.
	TYPE_Byte,			// Simple 1-byte integral or enumeration.
	TYPE_Bool,			// A logical variable.
	TYPE_Integer,		// Signed 32-bit value.
	TYPE_Float,			// IEEE float.
	TYPE_Angle,			// Angle value.
	TYPE_Color,			// Color.
	TYPE_String,		// Dynamic sizable string.
	TYPE_Vector,		// Point or direction in 2d space.
	TYPE_AABB,			// Axis aligned bounding box.
	TYPE_Resource,		// Resource such as FBitmap or FSound.
	TYPE_Entity,		// Entity - object in the level.
	TYPE_MAX
};


//
// An information about property type.
//
class CTypeInfo
{
public:
	// Variables.
	EPropType	Type;
	Integer		ArrayDim;

	// Type addition information.
	union 
	{
		struct{ CEnum*		Enum;				  };		// TYPE_Byte.
		struct{ CClass*		Class;				  };		// TYPE_Resource.
		struct{ FScript*	Script; int iFamily;  };		// TYPE_Entity.	
		struct{ void*		Inner;				  };	
	};

	// Constructors.
	CTypeInfo();
	CTypeInfo( EPropType InType, Integer InArrDim = 1, void* InInner = nullptr );

	// CTypeInfo interface.	
	DWord TypeSize( Bool bNoArray = false ) const;	
	String TypeName() const;
	String ToString( const void* Addr ) const;
	Bool MatchTypes( const CTypeInfo& Other ) const;
	Bool CompareValues( const void* A, const void* B ) const;
	void CopyValues( void* Dst, const void* Src ) const;
	void DestroyValues( const void* Addr ) const;
};


//
// A class or script property.
//
class CProperty: public CTypeInfo
{
public:
	// Variables.
	String		Name;
	String		Alias;
	DWord		Flags;
	Integer		Offset;

	// Constructor.
	CProperty();
	CProperty( CClass* Class, const Char* InName, EPropType PropType, Integer InArrDim, Integer Offset );
	CProperty( CTypeInfo InType, String InName, DWord InFlags, Integer InOffset );
	~CProperty();

	// CProperty interface.
	void Import( const void* Addr, CImporterBase& Im ) const; 
	void Export( const void* Addr, CExporterBase& Ex ) const;
	void SerializeValue( void* Addr, CSerializer& S ) const;
	void SetName( String InName );
};


/*-----------------------------------------------------------------------------
    CVariant.
-----------------------------------------------------------------------------*/

//
// All possible script types. Warning: CVariant doesn't support
// array types, just simple value.
//
class CVariant
{
public:
	// Variables.
	EPropType		Type;
	struct  
	{
		Byte		Value[16];
		String		StringValue;
	};

	// CVariant interface.
	CVariant();
	CVariant( const Byte InByte );
	CVariant( const Bool InBool );
	CVariant( const Integer InInteger );
	CVariant( const Float InFloat );
	CVariant( const TAngle InAngle );
	CVariant( const TColor InColor );
	CVariant( const String InString );
	CVariant( const TVector InVector );
	CVariant( const TRect InRect );
	CVariant( FResource* InResource );
	CVariant( FEntity* InEntity );
};


/*-----------------------------------------------------------------------------
    CNativeFunction.
-----------------------------------------------------------------------------*/

//
// Native function flags.
//
#define NFUN_None				0x0000		// Nothing.
#define NFUN_UnaryOp			0x0001		// Unary operator.
#define NFUN_BinaryOp			0x0002		// Binary operator.
#define NFUN_Method				0x0004		// Native method.
#define NFUN_SuffixOp			0x0008		// Suffix unary operator.
#define NFUN_AssignOp			0x0010		// Assignment operator.
#define NFUN_Foreach			0x0020		// Foreach iteration function.


//
// Pointer to the native method.
//
typedef void (FComponent::*TNativeMethod)( CFrame& Frame );


//
// A native function signature.
//
class CNativeFunction
{
public:
	// Variables.
	DWord			Flags;
	String			Name;
	CClass*			Class;
	CTypeInfo		ResultType;
	CTypeInfo		ParamsType[8];
	DWord			Priority;

	union
	{
		Byte			iOpCode;
		TNativeMethod	ptrMethod;	
	};

	// CNativeFunction interface.
	CNativeFunction( const Char* InName, DWord InFlags, Integer IniOpCode );
	CNativeFunction( const Char* InName, CClass* InClass, TNativeMethod InMethod );
	String GetSignature() const; 
};


/*-----------------------------------------------------------------------------
    CClass.
-----------------------------------------------------------------------------*/

//
// A Class flags.
//
#define CLASS_None			0x0000		// Nothing.
#define CLASS_Abstract		0x0001		// Class is abstract.
#define CLASS_Sterile		0x0002		// Class is 'final', no inheritance are allowed.
#define CLASS_Deprecated	0x0004		// Class marked as outdated.
#define CLASS_Highlight		0x0008		// Class has special marker.

//
// An object constructor.
//
typedef	FObject*(*TConstructor)();


//
// A meta class.
//
class CClass
{
public:
	// Variables.
	String				Name;
	String				Alt;
	DWord				Flags;
	CClass*				Super;
	TConstructor		Constructor;

	// In script stuff.
	TArray<CProperty*>			Properties;
	TArray<CNativeFunction*>	Methods;

	// Constructors.
	CClass( const Char* InName, TConstructor InCnstr, CClass* InSuper, DWord InFlags );
	~CClass();

	// CClass interface.
	void AddProperty( CProperty* InProp );
	void AddMethod( CNativeFunction* InMeth );
	CProperty* FindProperty( const Char* InName ) const;
	CNativeFunction* FindMethod( const Char* InName ) const;
	Bool IsA( CClass* SomeClass );
};


/*-----------------------------------------------------------------------------
    CClassDatabase.
-----------------------------------------------------------------------------*/

//
// A classes subsystem.
//
class CClassDatabase
{
public:
	// Static tables.
	static TArray<CClass*>			GClasses;
	static TArray<CEnum*>			GEnums;
	static TArray<CNativeFunction*>	GFuncs;

	// Static functions.
	static CClass* StaticFindClass( const Char* InName );
	static CEnum* StaticFindEnum( const Char* InName );
	static CNativeFunction* StaticFindFunction( const Char* InName );
};


/*-----------------------------------------------------------------------------
    Native macro.
-----------------------------------------------------------------------------*/

// Macro to figure out property offset.
#define PROPERTY_OFFSET(object, field) (Integer)((Byte*)&((object*)nullptr)->field - (Byte*)nullptr) 

// A variant parameter.
#define VARIANT_PARM(name) CVariant name = CVariant()

// Enumeration header.
#define BEGIN_ENUM( name )	\
	CEnum* _##name = new CEnum( L#name );	\
	{	\
		CEnum* Enum = _##name;	\
		assert(!CClassDatabase::StaticFindEnum(*Enum->Name));	\
		CClassDatabase::GEnums.Push(Enum);

// Enumeration footer.
#define END_ENUM	}

// Enumeration element.
#define ENUM_ELEM( element ) Enum->AddElement(L#element);	

// Class .h registration.
#define REGISTER_CLASS_H( cls )	\
private:	\
	static Byte cls##_Initializator();	\
	static CClass _This##cls;	\
	static Byte _InitByte;\
	typedef cls ClassType;	\
public:	\
	static CClass* MetaClass;	\
	inline friend void Serialize( CSerializer& S, cls*& V )	\
	{	\
		S.SerializeRef( *(FObject**)&V );	\
	}	\

// Base class .cpp registration.
#define REGISTER_BASE_CLASS_CPP( cls, flags )	\
FObject* Cntor##cls()\
{ return new cls(); }	\
CClass cls::_This##cls( L#cls, Cntor##cls, nullptr, flags );	\
CClass* cls::MetaClass = &cls::_This##cls;\
Byte cls::_InitByte = 0;	\
Byte cls::cls##_Initializator(){return 0;}\

// Class .cpp registration.
#define REGISTER_CLASS_CPP( cls, super, flags )	\
FObject* Cntor##cls()\
{ return new cls(); }	\
CClass cls::_This##cls( L#cls, Cntor##cls, super::MetaClass, flags );	\
CClass* cls::MetaClass = &cls::_This##cls;\
Byte cls::_InitByte = cls::cls##_Initializator();	\
Byte cls::cls##_Initializator()	\

// Add a new property to class.
#define ADD_PROPERTY( name, type, arr, flags, ptr )	\
{	\
	CProperty* prop = new CProperty( MetaClass, L#name, type, arr, PROPERTY_OFFSET(ClassType, name) );	\
	prop->Flags |= flags;	\
	prop->Inner = (void*)ptr;	\
	MetaClass->AddProperty( prop );	\
}	\

// Declare a native unary operator.
#define DECLARE_UNARY_OP( iopcode, name, arg1type, resulttype )	\
{	\
	CNativeFunction* Func	= new CNativeFunction( L#name, NFUN_UnaryOp, iopcode );	\
	Func->ParamsType[0]		= arg1type;	\
	Func->ResultType		= resulttype;	\
	CClassDatabase::GFuncs.Push(Func);	\
}	\

// Declare a native binary operator.
#define DECLARE_BIN_OP( iopcode, name, priority, arg1type, arg2type, resulttype )	\
{	\
	CNativeFunction* Func	= new CNativeFunction( L#name, NFUN_BinaryOp, iopcode );	\
	Func->Priority			= priority;	\
	Func->ParamsType[0]		= arg1type;	\
	Func->ParamsType[1]		= arg2type;	\
	Func->ResultType		= resulttype;	\
	CClassDatabase::GFuncs.Push(Func);	\
}	\

// Declare a native suffix operator.
#define DECLARE_SUFFIX_OP( iopcode, name, arg1type, resulttype )	\
{	\
	CNativeFunction* Func	= new CNativeFunction( L#name, NFUN_UnaryOp | NFUN_SuffixOp, iopcode );	\
	Func->ParamsType[0]		= arg1type;	\
	Func->ResultType		= resulttype;	\
	CClassDatabase::GFuncs.Push(Func);	\
}	\

// Declare a native binary assignment operator
#define DECLARE_BIN_ASS_OP( iopcode, name, priority, arg1type, arg2type, resulttype )	\
{	\
	CNativeFunction* Func	= new CNativeFunction( L#name, NFUN_BinaryOp | NFUN_AssignOp, iopcode );	\
	Func->Priority			= priority;	\
	Func->ParamsType[0]		= arg1type;	\
	Func->ParamsType[1]		= arg2type;	\
	Func->ResultType		= resulttype;	\
	CClassDatabase::GFuncs.Push(Func);	\
}	\

// Declare a native function.
#define DECLARE_FUNCTION( iopcode, name, resulttype, arg1type, arg2type, arg3type, arg4type )\
{	\
	CNativeFunction* Func	= new CNativeFunction( L#name, NFUN_None, iopcode );	\
	Func->ParamsType[0]		= arg1type;	\
	Func->ParamsType[1]		= arg2type;	\
	Func->ParamsType[2]		= arg3type;	\
	Func->ParamsType[3]		= arg4type;	\
	Func->ResultType		= resulttype;	\
	CClassDatabase::GFuncs.Push(Func);	\
}	\


// Declare a native iteration function.
#define DECLARE_ITERATOR( iopcode, name, resulttype, arg1type, arg2type, arg3type, arg4type )\
{	\
	CNativeFunction* Func	= new CNativeFunction( L#name, NFUN_Foreach, iopcode );	\
	Func->ParamsType[0]		= arg1type;	\
	Func->ParamsType[1]		= arg2type;	\
	Func->ParamsType[2]		= arg3type;	\
	Func->ParamsType[3]		= arg4type;	\
	Func->ResultType		= resulttype;	\
	CClassDatabase::GFuncs.Push(Func);	\
}	\


// Declare a native method.
#define DECLARE_METHOD( name, resulttype, arg1type, arg2type, arg3type, arg4type )\
{	\
	CNativeFunction* Func	= new CNativeFunction( L#name, ClassType::MetaClass, (TNativeMethod)(&ClassType::name) );	\
	Func->ParamsType[0]		= arg1type;	\
	Func->ParamsType[1]		= arg2type;	\
	Func->ParamsType[2]		= arg3type;	\
	Func->ParamsType[3]		= arg4type;	\
	Func->ResultType		= resulttype;	\
	ClassType::MetaClass->AddMethod( Func );\
	CClassDatabase::GFuncs.Push(Func);	\
}\


// Resources property type.
#define TYPE_SOUND		CTypeInfo( TYPE_Resource,	1,	FSound::MetaClass )
#define TYPE_BITMAP		CTypeInfo( TYPE_Resource,	1,	FBitmap::MetaClass )
#define TYPE_MUSIC		CTypeInfo( TYPE_Resource,	1,	FMusic::MetaClass )
#define TYPE_ANIMATION	CTypeInfo( TYPE_Resource,	1,	FAnimation::MetaClass )
#define TYPE_SCRIPT		CTypeInfo( TYPE_Resource,	1,	FScript::MetaClass )
#define TYPE_LEVEL		CTypeInfo( TYPE_Resource,	1,	FLevel::MetaClass )
#define TYPE_FONT		CTypeInfo( TYPE_Resource,	1,	FFont::MetaClass )


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/