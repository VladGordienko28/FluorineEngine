/*=============================================================================
    FrRes.cpp: Base resource classes.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FResource implementation.
-----------------------------------------------------------------------------*/

//
// Resource constructor.
//
FResource::FResource()
	:	FObject(),
		FileName(),
		Group()
{
}


//
// Resource serialization.
//
void FResource::SerializeThis( CSerializer& S )
{
	FObject::SerializeThis(S);

	Serialize( S, FileName );
	Serialize( S, Group );
}


//
// Resource after loading initialization.
//
void FResource::PostLoad()
{
	FObject::PostLoad();
}


//
// Resource import.
//
void FResource::Import( CImporterBase& Im )
{
	FObject::Import(Im);

	IMPORT_OBJECT( Owner );
	IMPORT_STRING( FileName );
	IMPORT_STRING( Group );
}


//
// Resource export.
//
void FResource::Export( CExporterBase& Ex )
{
	FObject::Export(Ex);

	EXPORT_OBJECT( Owner );
	EXPORT_STRING( FileName );
	EXPORT_STRING( Group );
}


/*-----------------------------------------------------------------------------
    FBlockResource implementation.
-----------------------------------------------------------------------------*/

//
// Block resource constructor.
//
FBlockResource::FBlockResource()
	:	FResource(),
		iBlock( -1 )
{
}


//
// Block resource destructor.
//
FBlockResource::~FBlockResource()
{
	// Release data block if was allocated.
	if( IsValidBlock() )
		ReleaseBlock();
}


//
// Initialize resource data block, return true if
// block created successfully, otherwise return false.
//
Bool FBlockResource::AllocateBlock( Integer NumBytes, DWord ExtraFlags )
{
	// Maybe block already allocated.
	if( IsValidBlock() )
		return false;

	// Create new one.
	iBlock	= GProject->BlockMan->AllocateBlock( NumBytes, ExtraFlags );
	return true;
}


//
// Return the size of block of data.
// If block not allocated, return 0.
//
DWord FBlockResource::GetBlockSize()
{
	return IsValidBlock() ? GProject->BlockMan->GetBlockSize(iBlock) : 0;
}


//
// Return block data.
//
void* FBlockResource::GetData()
{
	assert(IsValidBlock());
	return GProject->BlockMan->GetBlock( iBlock );
}


//
// Release data block data. Return true if
// released successfully, otherwise return false.
//
Bool FBlockResource::ReleaseBlock()
{
	if( !IsValidBlock() )
		return false;

	if( GIsEditor )
	{
		GProject->BlockMan->ReleaseBlock( iBlock );
		iBlock	= -1;
		return true;
	}
	else
		return false;
}


//
// Block resource serialization.
//
void FBlockResource::SerializeThis( CSerializer& S )
{
	FResource::SerializeThis( S );
	Serialize( S, iBlock );
}


/*-----------------------------------------------------------------------------
    Registration.
-----------------------------------------------------------------------------*/

REGISTER_CLASS_CPP( FResource, FObject, CLASS_Abstract )
{
	ADD_PROPERTY( Group,	TYPE_String,	1,					PROP_Editable,	nullptr );

	return 0;
}


REGISTER_CLASS_CPP( FBlockResource, FResource, CLASS_Abstract )
{
	return 0;
}


// Its here due fucked VisualStudio linker, and cpp linking order.
REGISTER_CLASS_CPP( FBitmap, FBlockResource, CLASS_None )
{
	BEGIN_ENUM(EBitmapFormat)
		ENUM_ELEM(BF_Palette8);
		ENUM_ELEM(BF_RGBA);
	END_ENUM;

	BEGIN_ENUM(EBitmapFilter)
		ENUM_ELEM(BFILTER_Nearest);
		ENUM_ELEM(BFILTER_Bilinear);
		ENUM_ELEM(BFILTER_Dither);
	END_ENUM;

	BEGIN_ENUM(EBitmapBlend)
		ENUM_ELEM(BLEND_Regular);
		ENUM_ELEM(BLEND_Masked);
		ENUM_ELEM(BLEND_Translucent);
		ENUM_ELEM(BLEND_Modulated);
		ENUM_ELEM(BLEND_Alpha);
		ENUM_ELEM(BLEND_Darken);
		ENUM_ELEM(BLEND_Brighten);
		ENUM_ELEM(BLEND_FastOpaque);
	END_ENUM;

	ADD_PROPERTY( Format,		TYPE_Byte,		1, PROP_Editable | PROP_Const,	_EBitmapFormat );
	ADD_PROPERTY( Filter,		TYPE_Byte,		1, PROP_Editable,				_EBitmapFilter );
	ADD_PROPERTY( BlendMode,	TYPE_Byte,		1, PROP_Editable,				_EBitmapBlend );
	ADD_PROPERTY( USize,		TYPE_Integer,	1, PROP_Editable | PROP_Const,	nullptr );
	ADD_PROPERTY( VSize,		TYPE_Integer,	1, PROP_Editable | PROP_Const,	nullptr );
	ADD_PROPERTY( PanUSpeed,	TYPE_Float,		1, PROP_Editable,				nullptr );
	ADD_PROPERTY( PanVSpeed,	TYPE_Float,		1, PROP_Editable,				nullptr );
	ADD_PROPERTY( Saturation,	TYPE_Float,		1, PROP_Editable,				nullptr );
	ADD_PROPERTY( AnimSpeed,	TYPE_Float,		1, PROP_Editable,				nullptr );
	ADD_PROPERTY( FileName,		TYPE_String,	1, PROP_Editable | PROP_Const,	 nullptr );

	return 0;
}


REGISTER_CLASS_CPP( FAnimation, FResource, CLASS_Sterile )
{
	ADD_PROPERTY( Sheet,		TYPE_Resource,	1, PROP_Editable,				FBitmap::MetaClass );
	ADD_PROPERTY( FrameW,		TYPE_Integer,	1, PROP_Editable,				nullptr );
	ADD_PROPERTY( FrameH,		TYPE_Integer,	1, PROP_Editable,				nullptr );
	ADD_PROPERTY( SpaceX,		TYPE_Integer,	1, PROP_Editable,				nullptr );
	ADD_PROPERTY( SpaceY,		TYPE_Integer,	1, PROP_Editable,				nullptr );

	return 0;
}


REGISTER_CLASS_CPP( FMusic, FResource, CLASS_Sterile )
{
	ADD_PROPERTY( FileName,		TYPE_String,	1, PROP_Editable | PROP_Const,	 nullptr );

	return 0;
}

REGISTER_CLASS_CPP( FSound, FBlockResource, CLASS_Sterile )
{
	ADD_PROPERTY( FileName,		TYPE_String,	1, PROP_Editable | PROP_Const,	 nullptr );

	return 0;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/