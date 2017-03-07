/*=============================================================================
    FrHUD.cpp: HUD rendering component.
    Copyright Nov.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FPainterComponent implementation.
-----------------------------------------------------------------------------*/

//
// HUD component constructor.
//
FPainterComponent::FPainterComponent()
	:	FExtraComponent()
{
	Width			= 0.f;
	Height			= 0.f;
	Canvas			= nullptr;
	Color			= COLOR_White;
	Font			= nullptr;
	Bitmap			= nullptr;
	MemZero( Effect, sizeof(Effect) );
}


//
// HUD component destructor.
//
FPainterComponent::~FPainterComponent()
{
	com_remove(Painters);
}


//
// Initialize HUD for level.
// 
void FPainterComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity(InEntity);
	com_add(Painters);
}


//
// Serialize painter stuff.
//
void FPainterComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis(S);

	// Serialize only references for GC.
	if( S.GetMode() == SM_Undefined )
	{
		Serialize( S, Font );
		Serialize( S, Bitmap );
	}
}


//
// Let's render HUD.
//
void FPainterComponent::RenderHUD( CCanvas* InCanvas )
{
	// Setup fields, for access from script.
	Canvas		= InCanvas;
	Width		= Canvas->View.Width;
	Height		= Canvas->View.Height;
	Effect[0]	= Effect[1]	= Effect[2]	= 1.f;
	Effect[3]	= Effect[4]	= Effect[5]	= 1.f;
	Effect[6]	= Effect[7]	= Effect[8]	= 0.f;
	Effect[9]	= 0.f;

	// Setup view info for valid screen to world
	// transform.
	ViewInfo	= TViewInfo
	(
		Level->Camera->Location,
		Level->Camera->Rotation,
		Level->Camera->GetFitFOV( Width, Height ),
		Level->Camera->Zoom,
		false,
		Canvas->View.X, Canvas->View.Y,
		Width, Height
	);

	// Let's script actually draw HUD.
	Entity->CallEvent( EVENT_OnRender );
}


//
// Draw a colored point on HUD.
//
void FPainterComponent::nativePoint( CFrame& Frame )
{
	TVector P	= POP_VECTOR;
	Float	S	= POP_FLOAT;

	if( Canvas )
		Canvas->DrawPoint( P, S, Color );
}


//
// Draw a colored line on HUD.
//
void FPainterComponent::nativeLine( CFrame& Frame )
{
	TVector	A	= POP_VECTOR,
			B	= POP_VECTOR;

	if( Canvas )
		Canvas->DrawLine( A, B, Color, false );
}


//
// Draw a textured tile.
//
void FPainterComponent::nativeTile( CFrame& Frame )
{
	TVector	P	= POP_VECTOR,
			PL	= POP_VECTOR,
			T	= POP_VECTOR,
			TL	= POP_VECTOR;

	if( Canvas )
	{
		TRenderRect	R;
		R.Bounds.Min	= P;
		R.Bounds.Max	= P + PL;

		R.Color			= Color;
		R.Rotation		= 0;

		if( Bitmap )
		{
			R.Bitmap		= Bitmap;
			R.Flags			= POLY_Unlit;

			// Division table, used to reduce multiple
			// divisions, since gui has many icons to draw.
			static const Float Rescale[] =
			{
				1.f / 1.f,
				1.f / 2.f,
				1.f / 4.f,
				1.f / 8.f,
				1.f / 16.f,
				1.f / 32.f,
				1.f / 64.f,
				1.f / 128.f,
				1.f / 256.f,
				1.f / 512.f,
				1.f / 1024.f,
				1.f / 2048.f,
				1.f / 4096.f,
			};

			// Texture coords.
			R.TexCoords.Min.X	= T.X * Rescale[Bitmap->UBits];
			R.TexCoords.Min.Y	= T.Y * Rescale[Bitmap->VBits];
			R.TexCoords.Max.X	= (T.X+TL.X)  * Rescale[Bitmap->UBits];
			R.TexCoords.Max.Y	= (T.Y+TL.Y) * Rescale[Bitmap->VBits];
		}
		else
		{
			R.Bitmap		= nullptr;
			R.Flags			= POLY_Unlit | POLY_FlatShade;
		}

		Canvas->DrawRect(R);
	}
}


//
// Draw text.
//
void FPainterComponent::nativeTextOut( CFrame& Frame )
{
	TVector P	= POP_VECTOR;
	String	T	= POP_STRING;
	Float	S	= POP_FLOAT;

	if( Canvas && Font )
		Canvas->DrawText( *T, T.Len(), Font, Color, P, TVector( S, S ) );
}


//
// Return text width.
//
void FPainterComponent::nativeTextSize( CFrame& Frame )
{
	String	S	= POP_STRING;
	*POPA_FLOAT	= Font ? Font->TextWidth(*S) : 0.f;
}


//
// Restore level's global effect.
//
void FPainterComponent::nativePopEffect( CFrame& Frame )
{
	Level->GFXManager->PopEffect(POP_FLOAT);
}


//
// Set current effect.
//
void FPainterComponent::nativePushEffect( CFrame& Frame )
{
	Level->GFXManager->PushEffect( Effect, POP_FLOAT );
}


//
// Transform a point in the world's coords to the screen
// coords system.
//
void FPainterComponent::nativeProject( CFrame& Frame )
{
	Float	X, Y;
	ViewInfo.Project( POP_VECTOR, X, Y );
	*POPA_VECTOR	= TVector( X, Y );
}


//
// Transform a point in the screen's coords to the world
// coords system.
//
void FPainterComponent::nativeDeproject( CFrame& Frame )
{
	TVector V		= POP_VECTOR;
	*POPA_VECTOR	= ViewInfo.Deproject( V.X, V.Y );
}


/*-----------------------------------------------------------------------------
    Registration.
-----------------------------------------------------------------------------*/

REGISTER_CLASS_CPP( FPainterComponent, FExtraComponent, CLASS_Sterile )
{
	ADD_PROPERTY( Width,	TYPE_Float,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( Height,	TYPE_Float,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( Color,	TYPE_Color,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( Font,		TYPE_Resource,	1,					PROP_None,		FFont::MetaClass );
	ADD_PROPERTY( Bitmap,	TYPE_Resource,	1,					PROP_None,		FBitmap::MetaClass );
	ADD_PROPERTY( Effect,	TYPE_Float,		10,					PROP_None,		nullptr );

	DECLARE_METHOD( nativePoint,		TYPE_None,		TYPE_Vector,		TYPE_Float,		TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeLine,			TYPE_None,		TYPE_Vector,		TYPE_Vector,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeTextOut,		TYPE_None,		TYPE_Vector,		TYPE_String,	TYPE_Float,	TYPE_None );
	DECLARE_METHOD( nativeTextSize,		TYPE_Float,		TYPE_String,		TYPE_None,		TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeTile,			TYPE_None,		TYPE_Vector,		TYPE_Vector,	TYPE_Vector,TYPE_Vector );
	DECLARE_METHOD( nativePushEffect,	TYPE_None,		TYPE_Float,			TYPE_None,		TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativePopEffect,	TYPE_None,		TYPE_Float,			TYPE_None,		TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeProject,		TYPE_Vector,	TYPE_Vector,		TYPE_None,		TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeDeproject,	TYPE_Vector,	TYPE_Vector,		TYPE_None,		TYPE_None,	TYPE_None );

	return 0;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/