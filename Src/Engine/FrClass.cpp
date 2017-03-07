/*=============================================================================
    FrClass.cpp: Classes system implementation.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    CClassDatabase implementation.
-----------------------------------------------------------------------------*/

//
// Whether use user-friendly aliases instead of
// real properties and enum names.
//
#define USE_ALIASES		1


//
// Global tables.
//
TArray<CClass*>				CClassDatabase::GClasses;
TArray<CEnum*>				CClassDatabase::GEnums;
TArray<CNativeFunction*>	CClassDatabase::GFuncs;


//
// Find a meta class by its name,
// if class not found, return null.
//
CClass* CClassDatabase::StaticFindClass( const Char* InName )
{
	// Pretty sad, O(n), but used seldom,
	// so, it's doesn't critical.
	for( Integer i=0; i<GClasses.Num(); i++ )
		if( GClasses[i]->Name == InName )
			return GClasses[i];

	return nullptr;
}


//
// Find a enumeration by its name,
// if enumeration not found, return null.
//
CEnum* CClassDatabase::StaticFindEnum( const Char* InName )
{
	// Pretty sad, O(n), but used seldom,
	// so, it's doesn't critical.
	for( Integer i=0; i<GEnums.Num(); i++ )
		if( GEnums[i]->Name == InName )
			return GEnums[i];

	return nullptr;
}


//
// Find a function by its name,
// if function not found, return null.
//
CNativeFunction* CClassDatabase::StaticFindFunction( const Char* InName )
{
	// Pretty sad, O(n), but used seldom,
	// so, it's doesn't critical.
	for( Integer i=0; i<GFuncs.Num(); i++ )
		if( GFuncs[i]->Name == InName )
			return GFuncs[i];

	return nullptr;
}


/*-----------------------------------------------------------------------------
    CClass implementation.
-----------------------------------------------------------------------------*/

//
// Class constructor.
//
CClass::CClass( const Char* InName, TConstructor InCnstr, CClass* InSuper, DWord InFlags )
	:	Name( InName ),
		Alt( String::Copy( Name, 1, Name.Len()-1 ) ),	// Friendly name
		Flags( InFlags ),
		Super( InSuper ),
		Constructor( InCnstr ),
		Properties(),
		Methods()
{
	// Test for valid inheritance.
	if( this != FObject::MetaClass )
		assert(Super != nullptr);

	// Notify about old class.
	if( Flags & CLASS_Deprecated )
		log( L"Class '%s' is outdated", *Name );

	// Add to global database, not really
	// good place to do it.
	assert(!CClassDatabase::StaticFindClass(InName));
	CClassDatabase::GClasses.Push(this);
}


//
// Meta-Class destructor.
//
CClass::~CClass()
{
	// Kill properties.
	for( Integer iProp=0; iProp<Properties.Num(); iProp++ )
		delete Properties[iProp];
	Properties.Empty();

	// Kill methods.
	for( Integer iMeth=0; iMeth<Methods.Num(); iMeth++ )
		delete Methods[iMeth];
	Methods.Empty();
}


//
// Add property to class.
//
void CClass::AddProperty( CProperty* InProp )
{
	assert(FindProperty(*InProp->Name) == nullptr);
	Properties.Push( InProp );
}


//
// Add method to class.
//
void CClass::AddMethod( CNativeFunction* InMeth )
{
	assert(FindMethod(*InMeth->Name) == nullptr);
	Methods.Push( InMeth );
}


//
// Find a property by name, if no property
// found return null.
//
CProperty* CClass::FindProperty( const Char* InName ) const
{
	for( Integer iProp=0; iProp<Properties.Num(); iProp++ )
		if( Properties[iProp]->Name == InName )
			return Properties[iProp];

	// Property not found, but maybe it in the super class.
	return Super ? Super->FindProperty(InName) : nullptr;
}


//
// Find a method by name, if no method
// found return null.
//
CNativeFunction* CClass::FindMethod( const Char* InName ) const
{
	for( Integer iMeth=0; iMeth<Methods.Num(); iMeth++ )
		if( Methods[iMeth]->Name == InName )
			return Methods[iMeth];

	// Method not found, but maybe it in the super class.
	return Super ? Super->FindMethod(InName) : nullptr;
}


//
// Return true, if SomeClass is a parent class of
// this class.
//
Bool CClass::IsA( CClass* SomeClass )
{
	for(  CClass* C = this; C; C = C->Super )
		if( C == SomeClass )
			return true;

	return false;
}


/*-----------------------------------------------------------------------------
    CNativeFunction implementation.
-----------------------------------------------------------------------------*/

//
// Native function constructor.
//
CNativeFunction::CNativeFunction( const Char* InName, DWord InFlags, Integer IniOpCode )
	:	Name( InName ),
		Flags( InFlags ),
		iOpCode( IniOpCode ),
		Class( nullptr ),
		Priority( 0 )
{
	// Test for duplicates.
	if( !(Flags & (NFUN_UnaryOp | NFUN_BinaryOp)) )
		for( Integer i=0; i<CClassDatabase::GFuncs.Num(); i++ )
		{
			CNativeFunction* Other = CClassDatabase::GFuncs[i];

			if( Class == Other->Class && Name == Other->Name )
				error( L"function '%s' redeclarated", *Name );
		}
}


//
// Native method constructor.
//
CNativeFunction::CNativeFunction( const Char* InName, CClass* InClass, TNativeMethod InMethod )
	:	Flags( NFUN_Method ),
		Class( InClass ),
		ptrMethod( InMethod )
{
	// Make a friendly name.
	// Ugly a little :(
	Name = InName;
	assert(String::Copy( Name, 0, 6 ) == L"native");
	Name = String::Copy( Name, 6, Name.Len()-6 );

	// A 'horrible test'.
	//assert( Class && Class->IsA(FComponent::MetaClass) );
}


//
// Return a function signature as string.
//
String CNativeFunction::GetSignature() const
{
	String Args;
	for( Integer i=0; i<8 && ParamsType[i].Type!=TYPE_None; i++ )
	{
		Args	+= ParamsType[i].TypeName();
		if( i<7 && ParamsType[i+1].Type != TYPE_None )
			Args	+= L", ";
	}

	return	String::Format
	(
		L"%s %s(%s)",
		ResultType.Type != TYPE_None ? *ResultType.TypeName() : L"void",
		*Name,
		*Args 
	);
}


/*-----------------------------------------------------------------------------
    CVariant implementation.
-----------------------------------------------------------------------------*/

CVariant::CVariant()
	:	Type( TYPE_None )
{}
CVariant::CVariant( const Byte InByte )
	:	Type( TYPE_Byte )
{
	*((Byte*)(&Value[0])) = InByte;
}
CVariant::CVariant( const Bool InBool )
	:	Type( TYPE_Bool )
{
	*((Bool*)(&Value[0])) = InBool;
}
CVariant::CVariant( const Integer InInteger )
	:	Type( TYPE_Integer )
{
	*((Integer*)(&Value[0])) = InInteger;
}
CVariant::CVariant( const Float InFloat )
	:	Type( TYPE_Float )
{
	*((Float*)(&Value[0])) = InFloat;
}
CVariant::CVariant( const TAngle InAngle )
	:	Type( TYPE_Angle )
{
	*((TAngle*)(&Value[0])) = InAngle;
}
CVariant::CVariant( const TColor InColor )
	:	Type( TYPE_Color )
{
	*((TColor*)(&Value[0])) = InColor;
}
CVariant::CVariant( const String InString )
	:	Type( TYPE_String )
{
	StringValue = InString;
}
CVariant::CVariant( const TVector InVector )
	:	Type( TYPE_Vector )
{
	*((TVector*)(&Value[0])) = InVector;
}
CVariant::CVariant( const TRect InRect )
	:	Type( TYPE_AABB )
{
	*((TRect*)(&Value[0])) = InRect;
}
CVariant::CVariant( FResource* InResource )
	:	Type( TYPE_Resource )
{
	*((FResource**)(&Value[0])) = InResource;
}
CVariant::CVariant( FEntity* InEntity )
	:	Type( TYPE_Entity )
{
	*((FEntity**)(&Value[0])) = InEntity;
}


/*-----------------------------------------------------------------------------
    CProperty implementation.
-----------------------------------------------------------------------------*/

//
// CProperty default constructor.
// Don't shy to use it, it's not a CTypeInfo::CTypeInfo.
//
CProperty::CProperty()
	:	CTypeInfo( TYPE_None, 0, nullptr ),
		Flags( 0 ),
		Offset( 0 )
{
}


//
// CProperty native constructor.
//
CProperty::CProperty( CClass* Class, const Char* InName, EPropType PropType, Integer InArrDim, Integer InOffset )
	:	CTypeInfo( PropType, InArrDim, nullptr ),
		Flags( PROP_Native ),
		Offset( InOffset )
{
	SetName( InName );
}


//
// CProperty script or function constructor.
//
CProperty::CProperty( CTypeInfo InType, String InName, DWord InFlags, Integer InOffset )
	:	CTypeInfo( InType ),
		Flags( InFlags ),
		Offset( InOffset )
{
	SetName( InName );
}


//
// CProperty destructor.
//
CProperty::~CProperty()
{
	// Nothing to destroy here..
}


//
// Set an property name, and parse alias.
//
void CProperty::SetName( String InName )
{
	Name	= InName;

#ifdef USE_ALIASES
	// Alias.
	if( String::Pos( L"_", InName ) != -1 )
	{
		// Something crazy.
		Alias	= InName;
	}
	else
	{
		// Should be valid property.
		Char Buffer[64] = {}, *Walk = Buffer;
		Bool bBool = InName(0) == 'b';
		Integer i = (Integer)bBool;
		Char PrevChar = '\0';
		while( i < InName.Len() )
		{
			Char ThisChar = InName(i);
			if( (ThisChar>='A'&&ThisChar<='Z')&&(PrevChar>='a'&&PrevChar<='z')&&(InName(i+1)!='\0') )
				*Walk++ = ' ';
			*Walk++ = ThisChar;
			PrevChar = ThisChar;
			i++;
		}
		if( bBool )
			*Walk++ = '?';

		Alias	= Buffer;
	}
#else
	// No aliases.
	Alias	= Name;
#endif
}


//
// Import a property values.
//
void CProperty::Import( const void* Addr, CImporterBase& Im ) const
{
	switch( Type )
	{
		case TYPE_Byte:
			// Import byte.
			if( ArrayDim == 1 )
				*(Byte*)Addr = Im.ImportByte( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((Byte*)Addr)[i] = Im.ImportByte( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_Bool:
			// Import bool.
			if( ArrayDim == 1 )
				*(Bool*)Addr = Im.ImportBool( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((Bool*)Addr)[i] = Im.ImportBool( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_Integer:
			// Import integer.
			if( ArrayDim == 1 )
				*(Integer*)Addr = Im.ImportInteger( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((Integer*)Addr)[i] = Im.ImportInteger( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_Float:
			// Import float.
			if( ArrayDim == 1 )
				*(Float*)Addr = Im.ImportFloat( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((Float*)Addr)[i] = Im.ImportFloat( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_Angle:
			// Import angle.
			if( ArrayDim == 1 )
				*(TAngle*)Addr = Im.ImportAngle( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((TAngle*)Addr)[i] = Im.ImportAngle( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_Color:
			// Import color.
			if( ArrayDim == 1 )
				*(TColor*)Addr = Im.ImportColor( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((TColor*)Addr)[i] = Im.ImportColor( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_String:
			// Import string.
			if( ArrayDim == 1 )
				*(String*)Addr = Im.ImportString( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((String*)Addr)[i] = Im.ImportString( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_Vector:
			// Import vector.
			if( ArrayDim == 1 )
				*(TVector*)Addr = Im.ImportVector( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((TVector*)Addr)[i] = Im.ImportVector( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_AABB:
			// Import rect.
			if( ArrayDim == 1 )
				*(TRect*)Addr = Im.ImportAABB( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((TRect*)Addr)[i] = Im.ImportAABB( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		case TYPE_Resource:
		case TYPE_Entity:
			// Import object.
			if( ArrayDim == 1 )
				*(FObject**)Addr = Im.ImportObject( *Name );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					((FObject**)Addr)[i] = Im.ImportObject( *String::Format( L"%s[%d]", *Name, i ) );
			break;

		default:
			break;
	}
}


//
// Export a property values.
//
void CProperty::Export( const void* Addr, CExporterBase& Ex ) const
{
	switch( Type )
	{
		case TYPE_Byte:
			// Export byte.
			if( ArrayDim == 1 )
				Ex.ExportByte( *Name, *(Byte*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportByte( *String::Format( L"%s[%d]", *Name, i ), ((Byte*)Addr)[i] );
			break;

		case TYPE_Bool:
			// Export bool.
			if( ArrayDim == 1 )
				Ex.ExportBool( *Name, *(Bool*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportBool( *String::Format( L"%s[%d]", *Name, i ), ((Bool*)Addr)[i] );
			break;

		case TYPE_Integer:
			// Export integer.
			if( ArrayDim == 1 )
				Ex.ExportInteger( *Name, *(Integer*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportInteger( *String::Format( L"%s[%d]", *Name, i ), ((Integer*)Addr)[i] );
			break;

		case TYPE_Float:
			// Export float.
			if( ArrayDim == 1 )
				Ex.ExportFloat( *Name, *(Float*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportFloat( *String::Format( L"%s[%d]", *Name, i ), ((Float*)Addr)[i] );
			break;

		case TYPE_Angle:
			// Export angle.
			if( ArrayDim == 1 )
				Ex.ExportAngle( *Name, *(TAngle*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportAngle( *String::Format( L"%s[%d]", *Name, i ), ((TAngle*)Addr)[i] );
			break;

		case TYPE_Color:
			// Export color.
			if( ArrayDim == 1 )
				Ex.ExportColor( *Name, *(TColor*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportColor( *String::Format( L"%s[%d]", *Name, i ), ((TColor*)Addr)[i] );
			break;

		case TYPE_String:
			// Export string.
			if( ArrayDim == 1 )
				Ex.ExportString( *Name, *(String*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportString( *String::Format( L"%s[%d]", *Name, i ), ((String*)Addr)[i] );
			break;

		case TYPE_Vector:
			// Export vector.
			if( ArrayDim == 1 )
				Ex.ExportVector( *Name, *(TVector*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportVector( *String::Format( L"%s[%d]", *Name, i ), ((TVector*)Addr)[i] );
			break;

		case TYPE_AABB:
			// Export rect.
			if( ArrayDim == 1 )
				Ex.ExportAABB( *Name, *(TRect*)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportAABB( *String::Format( L"%s[%d]", *Name, i ), ((TRect*)Addr)[i] );
			break;

		case TYPE_Resource:
		case TYPE_Entity:
			// Export object.
			if( ArrayDim == 1 )
				Ex.ExportObject( *Name, *(FObject**)Addr );
			else
				for( Integer i=0; i<ArrayDim; i++ )
					Ex.ExportObject( *String::Format( L"%s[%d]", *Name, i ), ((FObject**)Addr)[i] );
			break;

		default:
			break;
	}
}


//
// Serialize property value.
//
void CProperty::SerializeValue( void* Addr, CSerializer& S ) const
{
	switch( Type )
	{
		case TYPE_Byte:
		case TYPE_Bool:
		case TYPE_Integer:
		case TYPE_Float:
		case TYPE_Angle:
		case TYPE_Color:
		case TYPE_Vector:
		case TYPE_AABB:
			// Simple values.
			S.SerializeData( Addr, TypeSize(false) );
			break;

		case TYPE_String:
			// List of strings.
			for( Integer i=0; i<ArrayDim; i++ )
				Serialize( S, ((String*)Addr)[i] );
			break;

		case TYPE_Entity:
			// Entities.
			for( Integer i=0; i<ArrayDim; i++ )
				Serialize( S, ((FEntity**)Addr)[i] );
			break;

		case TYPE_Resource:
			// Entities.
			for( Integer i=0; i<ArrayDim; i++ )
				Serialize( S, ((FResource**)Addr)[i] );
			break;

		default:
			break;
	}
}


/*-----------------------------------------------------------------------------
    CTypeInfo implementation.
-----------------------------------------------------------------------------*/

//
// Default constructor.
// Avoid to use it.
//
CTypeInfo::CTypeInfo()
	:	Type( TYPE_None ),
		ArrayDim( 1 ),
		Inner( nullptr ),
		iFamily( -1 )
{
}	


//
// Type info regular constructor.
//
CTypeInfo::CTypeInfo( EPropType InType, Integer InArrDim, void* InInner )
	:	Type( InType ),
		ArrayDim( InArrDim ),
		Inner( InInner ),
		iFamily( -1 )
{
}


//
// Return size of this type.
// bNoArray - size of the array inner only.
//
DWord CTypeInfo::TypeSize( Bool bNoArray ) const
{
	static const DWord TypeSizes[TYPE_MAX] =
	{
		0,						//	TYPE_None
		sizeof( Byte ),			//	TYPE_Byte
		sizeof( Bool ),			//	TYPE_Bool
		sizeof( Integer ),		//	TYPE_Integer
		sizeof( Float ),		//	TYPE_Float
		sizeof( TAngle ),		//	TYPE_Angle
		sizeof( TColor ),		//	TYPE_Color	
		sizeof( String ),		//	TYPE_String
		sizeof( TVector ),		//	TYPE_Vector
		sizeof( TRect ),		//	TYPE_AABB
		sizeof( FResource* ),	//	TYPE_Resource
		sizeof( FEntity* )		//	TYPE_Entity
	};

	// Is should count in array.
	return bNoArray ? TypeSizes[Type] : TypeSizes[Type] * ArrayDim;
}


//
// Return the name of the type.
//
String CTypeInfo::TypeName() const
{
	static const Char* Names[TYPE_MAX]	=
	{
		L"none",
		L"byte",
		L"bool",
		L"integer",
		L"float",
		L"angle",
		L"color",
		L"string",
		L"vector",
		L"aabb",
		L"resource",
		L"entity"
	};

	// Make name, based on it addition info, if any.
	String Base =	( Type == TYPE_Entity && Script ) ? Script->GetName() :
					( Type == TYPE_Resource && Class ) ? Class->Alt :
					( Type == TYPE_Byte && Enum ) ? Enum->Name :	Names[Type];

	return ArrayDim == 1 ? Base : String::Format( L"%s[%d]", *Base, ArrayDim );
}


//
// Destroy a values at the given address.
//
void CTypeInfo::DestroyValues( const void* Addr ) const
{
	// Only strings should be destroyed.
	if( Type == TYPE_String )
		for( Integer i=0; i<ArrayDim; i++ )
			(&((String*)Addr)[i])->~String();
}


//
// Copy a value from Src to Dst, this function is
// optimized, via MemCopy for speed.
//
void CTypeInfo::CopyValues( void* Dst, const void* Src ) const
{
	if( Type == TYPE_String )
	{
		// Slow string copy.
		for( Integer i=0; i<ArrayDim; i++ )
			((String*)Dst)[i] = ((String*)Src)[i];
	}
	else
	{
		// Fast simple types copy.
		MemCopy( Dst, Src, TypeSize(false) );
	}
}


//
// Compare two values, return true, if they are matched.
// Pass entire array to test.
//
Bool CTypeInfo::CompareValues( const void* A, const void* B ) const
{
	if( Type == TYPE_String )
	{
		// Slow string comparison.
		for( Integer i=0; i<ArrayDim; i++ )
			if( ((String*)A)[i] != ((String*)B)[i] )
				return false;

		return true;
	}
	else
	{
		// Fast simple types comparison.
		return MemCmp( A, B, TypeSize(false) );
	}
}


//
// Match types, return true if
// they are identical.
//
Bool CTypeInfo::MatchTypes( const CTypeInfo& Other ) const
{
	if( ArrayDim != Other.ArrayDim )
		return false;

	if( Type == TYPE_Entity )
	{
		// Compare entity by it script.
		return Other.Type == TYPE_Entity && ((Other.Script && Script) ? Other.Script == Script : true);
	}
	else if( Type == TYPE_Resource )
	{
		// Compare resource by it class.
		return Other.Type == TYPE_Resource && ( Class && Other.Class ? Other.Class->IsA( Class ) : true );
	}
	else
	{
		// Just compare types.
		return Type == Other.Type;
	}
}


//
// Convert a value to the string, used by ObjectInspector.
//
String CTypeInfo::ToString( const void* Addr ) const
{
	// Don't show entire array.
	if( ArrayDim > 1 )
		return L"[...]";

	// Single value.
	switch( Type )
	{
		case TYPE_Byte:
		{
			Byte Value = *(Byte*)Addr;
			if( Enum )
				return String::Format( L"%s", Value < Enum->Aliases.Num() ? *Enum->Aliases[Value] : L"BAD_INDEX" );
			else
				return String::Format( L"%d", Value );
		}
		case TYPE_Bool:
		{
			return *(Bool*)Addr ? L"True" : L"False";
		}
		case TYPE_Integer:
		{
			return String::Format( L"%d", *(Integer*)Addr );
		}
		case TYPE_Float:
		{
			return String::Format( L"%.4f", *(Float*)Addr );
		}
		case TYPE_Angle:
		{
			return String::Format( L"%.2f deg.", (*(TAngle*)Addr).ToDegs() );
		}
		case TYPE_Color:
		{
			TColor Value = *(TColor*)Addr;
			return String::Format( L"#%02x%02x%02x", Value.R, Value.G, Value.B );
		}
		case TYPE_String:
		{
			return *(String*)Addr;
		}	
		case TYPE_Vector:
		{
			TVector Value = *(TVector*)Addr;
			return String::Format( L"[%.2f, %.2f]", Value.X, Value.Y );
		}
		case TYPE_AABB:
		{
			TRect Value = *(TRect*)Addr;
			return String::Format( L"(%2.f, %2.f, %2.f, %2.f )", Value.Min.X, Value.Min.Y, Value.Max.X, Value.Max.Y );
		}
		case TYPE_Resource:
		{
			FResource* Value = *(FResource**)Addr;
			return Value ? Value->GetName() : L"???";
		}
		case TYPE_Entity:
		{
			FEntity* Value = *(FEntity**)Addr;
			return Value ? Value->GetName() : L"???";
		}
		default:
		{
			return String::Format( L"Bad property type %d", Type );
		}
	}
}


/*-----------------------------------------------------------------------------
    CEnum implementation.
-----------------------------------------------------------------------------*/

//
// Enumeration native constructor.
//
CEnum::CEnum( const Char* InName )
	:	Name( InName ),
		Flags( ENUM_Native ),
		Elements(),
		Aliases()
{
}


//
// Enumeration script constructor.
//
CEnum::CEnum( const Char* InName, FScript* Script )
	:	Name( InName ),
		Flags( ENUM_None ),
		Elements(),
		Aliases()
{
}	


//
// Enumeration destructor.
//
CEnum::~CEnum()
{
	Elements.Empty();
	Aliases.Empty();
}


//
// Add new item to enumeration. Also parse user-friendly
// alias for element. Return index of new element.
//
Integer CEnum::AddElement( String NewElem )
{
	assert(Elements.Num()==Aliases.Num());
	assert(Elements.FindItem(NewElem)==-1);

	// Add to list.
	Elements.Push(NewElem);

#if USE_ALIASES
	// Parse name.
	if( String::Pos( L"_", NewElem ) != -1 )
	{
		// Regular enum name.
		Integer i = 0;
		Char Buffer[64] = {}, *Walk = Buffer;
		while( NewElem(i) != '_' )
			i++;
		i++;
		Char PrevChar = '\0';
		while( i < NewElem.Len() )
		{
			Char ThisChar = NewElem(i);
			if( (ThisChar>='A'&&ThisChar<='Z')&&(PrevChar>='a'&&PrevChar<='z') )
				*Walk++ = ' ';
			*Walk++ = ThisChar;
			PrevChar = ThisChar;
			i++;
		}
		Aliases.Push(Buffer);
	}
	else
	{
		// Something strange.
		Aliases.Push(NewElem);
	}
#else
	// No alias.
	Aliases.Push(NewElem);
#endif

	return Elements.Num()-1;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/