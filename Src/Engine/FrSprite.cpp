/*=============================================================================
    FrSprite.cpp: Sprite components.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FSpriteComponent implementation.
-----------------------------------------------------------------------------*/
//
// A global division table, used to reduce multiple
// divisions, when convert pixel's coords to texture's.
//
static const Float GRescale[] =
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


//
// Sprite constructor.
//
FSpriteComponent::FSpriteComponent()
	:	FExtraComponent(),
		CBitmapRenderAddon( this )
{
	Offset			= TVector( 0.f, 0.f );
	Scale			= TVector( 1.f, 1.f );
	Rotation		= TAngle( 0 );
	TexCoords.Min	= TVector( 0.f, 0.f );
	TexCoords.Max	= TVector( 16.f, 16.f );
}


//
// Initialize sprite.
//
void FSpriteComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity(InEntity);
	com_add(RenderObjects);
}


//
// Serialize sprite component.
//
void FSpriteComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );
	CBitmapRenderAddon::SerializeAddon( S );

	Serialize( S, Offset );
	Serialize( S, Scale );
	Serialize( S, Rotation );
	Serialize( S, TexCoords );
}


//
// Sprite import.
//
void FSpriteComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );
	CBitmapRenderAddon::ImportAddon( Im );

	IMPORT_VECTOR(Offset);
	IMPORT_VECTOR(Scale);
	IMPORT_ANGLE(Rotation);
	IMPORT_AABB(TexCoords);
}


//
// Sprite export.
//
void FSpriteComponent::Export( CExporterBase& Ex )
{	
	FExtraComponent::Export( Ex );
	CBitmapRenderAddon::ExportAddon( Ex );

	EXPORT_VECTOR(Offset);
	EXPORT_VECTOR(Scale);
	EXPORT_ANGLE(Rotation);
	EXPORT_AABB(TexCoords);
}


//
// Render the sprite.
//
void FSpriteComponent::Render( CCanvas* Canvas )
{
	// Test hidden.
	if( bHidden && !(Level->RndFlags & RND_Hidden) )
		return;

	// Precompute.
	TVector Location	= Offset + Base->Location;
	TVector Size		= TVector( Base->Size.X*Scale.X, Base->Size.Y*Scale.Y );

	// Sprite is visible?
	TRect Bounds = TRect( Location, Size );
	if( !Canvas->View.Bounds.IsOverlap(TRect( Location, FastSqrt(Size.X*Size.X+Size.Y*Size.Y) )) )
		return;

	// Initialize rect.
	TRenderRect Rect;
	Rect.Flags			= POLY_Unlit * (bUnlit | !(Level->RndFlags & RND_Lighting));
	Rect.Rotation		= Base->Rotation + Rotation;		
	Rect.Color			= Base->bSelected ? TColor( 0x80, 0xe6, 0x80, 0xff ) : Color;
	Rect.Bitmap			= Bitmap ? Bitmap : FBitmap::Default;
	Rect.Bounds			= Bounds;
	
	// Integer texture coords to float.
	Rect.TexCoords.Min.X	= TexCoords.Min.X * GRescale[Rect.Bitmap->UBits];
	Rect.TexCoords.Max.X	= TexCoords.Max.X * GRescale[Rect.Bitmap->UBits];
	Rect.TexCoords.Min.Y	= TexCoords.Max.Y * GRescale[Rect.Bitmap->VBits];
	Rect.TexCoords.Max.Y	= TexCoords.Min.Y * GRescale[Rect.Bitmap->VBits];	

	if( Base->bFrozen )
		Rect.Color	= Rect.Color * 0.55f;

	// Apply flipping.
	if( bFlipH )
		Exchange( Rect.TexCoords.Min.X, Rect.TexCoords.Max.X );
	if( bFlipV )
		Exchange( Rect.TexCoords.Min.Y, Rect.TexCoords.Max.Y );

	// Draw it!
	Canvas->DrawRect( Rect );
}


/*-----------------------------------------------------------------------------
    FDecoComponent implementation.
-----------------------------------------------------------------------------*/

//
// Deco component constructor.
// 
FDecoComponent::FDecoComponent()
	:	FExtraComponent(),
		CBitmapRenderAddon( this ),
		DecoType( DECO_Shrub ),
		Frequency( 1.f ),
		Amplitude( 1.f )		
{
	TexCoords.Min	= TVector( 0.f, 0.f );
	TexCoords.Max	= TVector( 16.f, 16.f );
}


//
// Initialize component.
//
void FDecoComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity( InEntity );
	com_add(RenderObjects);
}


//
// Validate parameters.
//
void FDecoComponent::EditChange()
{	
	FExtraComponent::EditChange();

	// Clamp parameters.
	Amplitude	= Clamp( Amplitude, -100.f, +100.f );
	Frequency	= Clamp( Frequency, -100.f, +100.f );
}


//
// Serialize component.
//
void FDecoComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );
	CBitmapRenderAddon::SerializeAddon( S );
	SerializeEnum( S, DecoType );
	Serialize( S, Frequency );
	Serialize( S, Amplitude );
	Serialize( S, TexCoords );
}


//
// Export component.
//
void FDecoComponent::Export( CExporterBase& Ex )
{
	FExtraComponent::Export( Ex );
	CBitmapRenderAddon::ExportAddon( Ex );
	EXPORT_BYTE(DecoType);
	EXPORT_FLOAT(Frequency);
	EXPORT_FLOAT(Amplitude);
	EXPORT_AABB(TexCoords);
}


//
// Import component.
//
void FDecoComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );
	CBitmapRenderAddon::ImportAddon( Im );
	IMPORT_BYTE(DecoType);
	IMPORT_FLOAT(Frequency);
	IMPORT_FLOAT(Amplitude);
	IMPORT_AABB(TexCoords);
}


//
// Render decoration sprite.
//
void FDecoComponent::Render( CCanvas* Canvas )
{
	// Prepare.
	TVector Scale	= Base->Size;
	Float	Now		= GPlat->Now();

	// Is visible?
	TRect Bounds = TRect( Base->Location, Max(Scale.X, Scale.Y)*2.f );
	if( !Canvas->View.Bounds.IsOverlap(Bounds) )
		return;

	// Initialize polygon.
	TRenderPoly Poly;
	Poly.Bitmap			= Bitmap ? Bitmap : FBitmap::Default;
	Poly.Color			= Base->bSelected ? TColor( 0x80, 0xe6, 0x80, 0xff ) : Color;
	Poly.Flags			= POLY_Unlit * (bUnlit | !(Level->RndFlags & RND_Lighting));
	Poly.NumVerts		= 4;

	// Texture coords.
	Float	U1	= TexCoords.Min.X * GRescale[Poly.Bitmap->UBits],
			U2	= TexCoords.Max.X * GRescale[Poly.Bitmap->UBits],
			V1	= TexCoords.Min.Y * GRescale[Poly.Bitmap->VBits],
			V2	= TexCoords.Max.Y * GRescale[Poly.Bitmap->VBits];

	if( bFlipH )
		Exchange( U1, U2 );
	if( bFlipV )
		Exchange( V1, V2 );

	Poly.TexCoords[0]	= TVector( U1, V2 );
	Poly.TexCoords[1]	= TVector( U1, V1 );
	Poly.TexCoords[2]	= TVector( U2, V1 );
	Poly.TexCoords[3]	= TVector( U2, V2 );

	// Untransformed vertices in local coords.
	Poly.Vertices[0]	= TVector( -Scale.X, -Scale.Y )*0.5;
	Poly.Vertices[1]	= TVector( -Scale.X, +Scale.Y )*0.5;
	Poly.Vertices[2]	= TVector( +Scale.X, +Scale.Y )*0.5;
	Poly.Vertices[3]	= TVector( +Scale.X, -Scale.Y )*0.5;
	  
	// Apply distortion effect.
	switch ( DecoType )
	{
		case DECO_Shrub:
		{
			// Windy shrub.
			Float Sine		= Sin(Now*Frequency) * Amplitude;
			Float Cosine	= Cos(Now*Frequency) * Amplitude;

			Poly.Vertices[1] += TVector( +Cosine, +Sine );
			Poly.Vertices[2] += TVector( -Cosine, +Sine );
			break;
		}
		case DECO_Trunc:
		{
			// Tree trunc.
			Float Phase		= Sin(Now*Frequency) * Amplitude * (PI/8.f);
			TCoords Matrix	= TCoords( TVector( Base->Location.X, Base->Location.Y-Scale.Y*0.5f ), Phase );

			Poly.Vertices[1]	= TransformVectorBy( Poly.Vertices[1], Matrix ); 
			Poly.Vertices[2]	= TransformVectorBy( Poly.Vertices[2], Matrix );
			
			break;
		}
		case DECO_Liana:
		{
			// Waver liana.
			Float Phase		= Sin(Now*Frequency) * Amplitude * (PI/8.f);
			TCoords Matrix	= TCoords( TVector( Base->Location.X, Base->Location.Y+Scale.Y*0.5f ), Phase );

			Poly.Vertices[0]	= TransformVectorBy( Poly.Vertices[0], Matrix ); 
			Poly.Vertices[3]	= TransformVectorBy( Poly.Vertices[3], Matrix );

			break;
		}
	}

	// Transform to world coords.
	Poly.Vertices[0]	+= Base->Location;
	Poly.Vertices[1]	+= Base->Location;
	Poly.Vertices[2]	+= Base->Location;
	Poly.Vertices[3]	+= Base->Location;

	// Draw it!
	Canvas->DrawPoly( Poly );
}


/*-----------------------------------------------------------------------------
    FAnimatedSpriteComponent implementation.
-----------------------------------------------------------------------------*/

//
// Animated sprite constructor.
//
FAnimatedSpriteComponent::FAnimatedSpriteComponent()
	:	FExtraComponent(),
		CAnimationRenderAddon(this),
		CTickAddon(this),
		Offset( 0.f, 0.f ),
		Scale( 1.f, 1.f ),
		Rotation( 0 ),
		AnimType( ANIM_Once ),
		iSequence( 0 ),
		bBackward( false ),
		Rate( 0.f ),
		Frame( 0.f )
{
}


//
// Initialize sprite for level.
//
void FAnimatedSpriteComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity( InEntity );
	com_add(TickObjects);
	com_add(RenderObjects);
}


//
// Process animation.
//
void FAnimatedSpriteComponent::Tick( Float Delta )
{
	// Does we can play?
	if( !Animation || iSequence >= Animation->Sequences.Num() )
		return;

	// Prepare.
	TAnimSequence* Seq	= &Animation->Sequences[iSequence];
	Integer	MaxFrames	= Seq->Count-1, iFrame;

	// Process according type.
	switch( AnimType )
	{
		case ANIM_Once:
		{
			// Simple animation, just play it and stop with
			// OnAnimEnd notification.
			Frame		+= Rate * Delta;
			iFrame		= Floor(Frame);

			if( iFrame > MaxFrames )
			{
				// Animation expired.
				Rate	= 0.f;
				Frame	= MaxFrames;
				Entity->CallEvent( EVENT_OnAnimEnd );
			}
			break;
		}
		case ANIM_Loop:
		{
			// Looped animation.
			Frame		+= Rate * Delta;
			iFrame		= Floor(Frame);

			if( iFrame > MaxFrames )
			{
				// Reloop animation.
				Frame	= 0.f;
			}
			break;
		}
		case ANIM_PingPong:
		{
			// Ping-pong animation.
			if( bBackward )
			{
				// Backward animation.
				Frame		+= Rate * Delta;
				iFrame		= Floor(Frame);
				if( iFrame < 0 )
				{
					// Toggle to forward.
					bBackward	= false;
					Frame		= MaxFrames - 0.999f;
				}
			}
			else
			{
				// Forward animation.
				Frame		+= Rate * Delta;
				iFrame		= Floor(Frame);
				if( iFrame > MaxFrames )
				{
					// Toggle to backward.
					bBackward	= true;
					Frame		= MaxFrames - 0.001f;
				}
			}
			break;
		}
	}
}


//
// Render sprite.
//
void FAnimatedSpriteComponent::Render( CCanvas* Canvas )
{
	// Test hidden.
	if( bHidden && !(Level->RndFlags & RND_Hidden) )
		return;

	// Sprite is visible?
	TVector DrawSize/*( Base->Size.X*Scale.X, Base->Size.Y*Scale.Y )*/ = Scale;
	TVector DrawPos( Base->Location.X+Offset.X, Base->Location.Y+Offset.Y );
	TRect Bounds( DrawPos, DrawSize );
	if( !Canvas->View.Bounds.IsOverlap(Bounds) )
		return;

	// Initialize rect struct.
	TRenderRect Rect;

	Rect.Flags			= POLY_Unlit * (bUnlit | !(Level->RndFlags & RND_Lighting));
	Rect.Rotation		= Base->Rotation + Rotation;
	Rect.Color			= Base->bSelected ? TColor( 0x80, 0xe6, 0x80, 0xff ) : Color;
	Rect.Bounds			= Bounds;

	// Is animation valid?
	if( Animation && Animation->Sheet && iSequence < Animation->Sequences.Num() )
	{
		// Draw animation.
		TAnimSequence* Seq	= &Animation->Sequences[iSequence];
		Integer iDrawFrame	= Floor(Frame) + Seq->Start;

		Rect.Bitmap		= Animation->Sheet;
		Rect.TexCoords	= Animation->GetTexCoords( iDrawFrame );
	}
	else
	{
		// Bad animation, draw chess board instead.
		Rect.Bitmap			= nullptr;
		Rect.TexCoords.Min	= TVector( 0.f, 1.f );
		Rect.TexCoords.Max	= TVector( 1.f, 0.f );
	}

	// Apply flipping.
	if( bFlipH )
		Exchange( Rect.TexCoords.Min.X, Rect.TexCoords.Max.X );
	if( bFlipV )
		Exchange( Rect.TexCoords.Min.Y, Rect.TexCoords.Max.Y );

	if( Base->bFrozen )
		Rect.Color	= Rect.Color * 0.55f;

	// Draw it!
	Canvas->DrawRect( Rect );

}


//
// Serialize animation.
//
void FAnimatedSpriteComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );
	CAnimationRenderAddon::SerializeAddon( S );

	Serialize( S, Offset );
	Serialize( S, Scale );
	Serialize( S, Rotation );

	// Don't serialize animation
	// runtime info.
}


//
// Import animation.
//
void FAnimatedSpriteComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );
	CAnimationRenderAddon::ImportAddon( Im );
	IMPORT_VECTOR(Offset);
	IMPORT_VECTOR(Scale);
	IMPORT_ANGLE(Rotation);
}


//
// Export animation.
//
void FAnimatedSpriteComponent::Export( CExporterBase& Ex )
{
	FExtraComponent::Export( Ex );
	CAnimationRenderAddon::ExportAddon( Ex );
	EXPORT_VECTOR(Offset);
	EXPORT_VECTOR(Scale);
	EXPORT_ANGLE(Rotation);
}


//
// Play an animation.
//
void FAnimatedSpriteComponent::nativePlayAnim( CFrame& Frame )
{
	// Pop arguments.
	String		ASeqName	= POP_STRING;
	Float		ARate		= POP_FLOAT;
	EAnimType	AType		= (EAnimType)POP_BYTE;

	if( !Animation )
	{
		log( L"Anim: Error in '%s': No animation to play", *Entity->GetFullName() );
		return;
	}

	Integer iSeq = Animation->FindSequence( ASeqName );
	if( iSeq == -1 )
	{
		log( L"Anim: Sequence '%s' not found in '%s'", *ASeqName, *Animation->GetName() );
		return;
	}

	if( iSequence<Animation->Sequences.Num() && iSeq == iSequence )
	{
		// Modify current play.
		if( Rate != ARate )
			this->Frame	= 0.f;

		Rate		= ARate;
		AnimType	= AType;
	}
	else
	{
		// Start not sequence.
		AnimType	= AType;
		iSequence	= iSeq;
		bBackward	= false;
		Rate		= ARate;
		this->Frame	= 0.f;
	}
}


//
// Return the name of current played sequence.
//
void FAnimatedSpriteComponent::nativeGetAnimName( CFrame& Frame )
{
	if( Animation && Rate != 0.f && iSequence < Animation->Sequences.Num() )
	{
		*POPA_STRING	= Animation->Sequences[iSequence].Name;
	}
	else
		*POPA_STRING	= L"";
}


//
// Pause current animation.
//
void FAnimatedSpriteComponent::nativePauseAnim( CFrame& Frame )
{
	Rate	= 0.f;
}


/*-----------------------------------------------------------------------------
    FParallaxLayerComponent implementation.
-----------------------------------------------------------------------------*/

//
// Parallax layer component.
//
FParallaxLayerComponent::FParallaxLayerComponent()
	:	FExtraComponent(),
		CBitmapRenderAddon( this )
{
	Scale			= TVector( 20.f, 20.f );
	Parallax		= TVector( 0.05f, 0.05f );
	Gap				= TVector( 5.f, 5.f );
	Offset			= TVector( 0.f, 0.f );
	TexCoords.Min	= TVector( 0.f, 0.f );
	TexCoords.Max	= TVector( 16.f, 16.f );
}


//
// Initialize parallax for level.
//
void FParallaxLayerComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity(InEntity);
	com_add(RenderObjects);
}


//
// Parallax layer serialization.
//
void FParallaxLayerComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );
	CBitmapRenderAddon::SerializeAddon( S );
	
	Serialize( S, Scale );
	Serialize( S, Parallax );
	Serialize( S, Gap );
	Serialize( S, Offset );
	Serialize( S, TexCoords );
}


//
// Parallax layer import.
//
void FParallaxLayerComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );
	CBitmapRenderAddon::ImportAddon( Im );

	IMPORT_VECTOR( Scale );
	IMPORT_VECTOR( Parallax );
	IMPORT_VECTOR( Gap );
	IMPORT_VECTOR( Offset );
	IMPORT_AABB( TexCoords );
}


//
// Parallax layer export.
//
void FParallaxLayerComponent::Export( CExporterBase& Ex )
{
	FExtraComponent::Export( Ex );
	CBitmapRenderAddon::ExportAddon( Ex );

	EXPORT_VECTOR( Scale );
	EXPORT_VECTOR( Parallax );
	EXPORT_VECTOR( Gap );
	EXPORT_VECTOR( Offset );
	EXPORT_AABB( TexCoords );
}

//
// When some field changed in parallax layer.
//
void FParallaxLayerComponent::EditChange()
{
	FExtraComponent::EditChange();
}


//
// Parallax layer rendering.
//
void FParallaxLayerComponent::Render( CCanvas* Canvas )
{
	// Don't draw parallax layers in mirage!
	if( Canvas->View.bMirage || !(Level->RndFlags & RND_Backdrop) )
		return;

	// How much tiles draw.
	TVector Period = Scale + Gap;
	TVector Area = Canvas->View.Bounds.Size();
	Integer NumX = Ceil(Area.X / Period.X) + 1;
	Integer NumY = Ceil(Area.Y / Period.Y) + 1;

	// Compute parallax parameters.
	TVector Bias;
	Bias.X = Canvas->View.Coords.Origin.X * Parallax.X + Offset.X;
	Bias.Y = Canvas->View.Coords.Origin.Y * Parallax.Y + Offset.Y;
	
	Bias.X = fmod( Bias.X, Period.X );
	Bias.Y = fmod( Bias.Y, Period.Y );

	// Setup render rect.
	TRenderRect Rect;
	Rect.Bitmap			= Bitmap ? Bitmap : FBitmap::Default;
	Rect.Color			= Base->bSelected ? TColor( 0x80, 0xe6, 0x80, 0xff ) : Color;
	Rect.Flags			= POLY_Unlit * (bUnlit | !(Level->RndFlags & RND_Lighting));
	Rect.Rotation		= 0;

	Rect.TexCoords.Min.X	= TexCoords.Min.X * GRescale[Rect.Bitmap->UBits];
	Rect.TexCoords.Max.X	= TexCoords.Max.X * GRescale[Rect.Bitmap->UBits];
	Rect.TexCoords.Min.Y	= TexCoords.Max.Y * GRescale[Rect.Bitmap->VBits];
	Rect.TexCoords.Max.Y	= TexCoords.Min.Y * GRescale[Rect.Bitmap->VBits];	

	// Apply flipping.
	if( bFlipH )
		Exchange( Rect.TexCoords.Min.X, Rect.TexCoords.Max.X );
	if( bFlipV )
		Exchange( Rect.TexCoords.Min.Y, Rect.TexCoords.Max.Y );

	// Draw all rectangles.
	for( Integer Y=0; Y<NumY; Y++ )
	for( Integer X=0; X<NumX; X++ )		
	{
		TVector Origin = Canvas->View.Bounds.Min - Bias + TVector( X*Period.X, Y*Period.Y ) - Scale*0.5f;
		Rect.Bounds	= TRect( Origin, Scale );
		Canvas->DrawRect( Rect );
	}
}


/*-----------------------------------------------------------------------------
    FLabelComponent implementation.
-----------------------------------------------------------------------------*/

//
// Label component constructor.
//
FLabelComponent::FLabelComponent()
	:	FExtraComponent(),
		CRenderAddon(this)
{
	bHidden		= false;
	Color		= COLOR_White;
	Font		= nullptr;
	Scale		= 1.f;
	Text		= L"Text";
}


//
// Label addon rendering.
//
void FLabelComponent::Render( CCanvas* Canvas )
{
	// Reject if any.
	if( Canvas->View.bMirage || !Font || !Text )
		return;

	TViewInfo OldView = Canvas->View;
	Canvas->PushTransform(TViewInfo( OldView.X, OldView.Y, OldView.Width, OldView.Height ));
	{
		TVector	P;
		Float	W = Font->TextWidth(*Text)*Scale, H = Font->Glyphs[0].H*Scale;
		OldView.Project( Base->Location, P.X, P.Y );
		
		Canvas->DrawText
		(
			Text,
			Font,
			Color,
			TVector( P.X-W*0.5f, P.Y-H*0.5f ),
			TVector( Scale, Scale )
		);
	}
	Canvas->PopTransform();
}


//
// Initialize parallax for level.
//
void FLabelComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity(InEntity);
	com_add(RenderObjects);
}


//
// Label serialization.
//
void FLabelComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );
	CRenderAddon::SerializeAddon( S );
	Serialize( S, Text );
	Serialize( S, Font );
	Serialize( S, Scale );
}


//
// Import label.
//
void FLabelComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );
	CRenderAddon::ImportAddon( Im );
	IMPORT_STRING(Text);
	IMPORT_FLOAT(Scale);
	IMPORT_OBJECT(Font);
}


//
// Export label.
//
void FLabelComponent::Export( CExporterBase& Ex )
{
	FExtraComponent::Export( Ex );
	CRenderAddon::ExportAddon( Ex );
	EXPORT_STRING(Text);
	EXPORT_FLOAT(Scale);
	EXPORT_OBJECT(Font);
}


/*-----------------------------------------------------------------------------
    Registration.
-----------------------------------------------------------------------------*/

REGISTER_CLASS_CPP( FSpriteComponent, FExtraComponent, CLASS_None )
{
	ADD_PROPERTY( Offset,		TYPE_Vector,	1,					PROP_None,		nullptr );
	ADD_PROPERTY( Scale,		TYPE_Vector,	1,					PROP_None,		nullptr );
	ADD_PROPERTY( Rotation,		TYPE_Angle,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( TexCoords,	TYPE_AABB,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bHidden,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bUnlit,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipH,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipV,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Color,		TYPE_Color,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Bitmap,		TYPE_Resource,	1,					PROP_Editable,	FBitmap::MetaClass );

	return 0;
}


REGISTER_CLASS_CPP( FDecoComponent, FExtraComponent, CLASS_None )
{
	BEGIN_ENUM(EDecoType)
		ENUM_ELEM(DECO_None);
		ENUM_ELEM(DECO_Trunc);
		ENUM_ELEM(DECO_Shrub);
		ENUM_ELEM(DECO_Liana);
	END_ENUM;

	ADD_PROPERTY( DecoType,		TYPE_Byte,		1,					PROP_Editable,	_EDecoType );
	ADD_PROPERTY( Frequency,	TYPE_Float,		1,					PROP_Editable,	nullptr );	
	ADD_PROPERTY( Amplitude,	TYPE_Float,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( TexCoords,	TYPE_AABB,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bUnlit,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipH,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipV,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Color,		TYPE_Color,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Bitmap,		TYPE_Resource,	1,					PROP_Editable,	FBitmap::MetaClass );

	return 0;
}


REGISTER_CLASS_CPP( FAnimatedSpriteComponent, FExtraComponent, CLASS_None )
{
	BEGIN_ENUM(EAnimType)
		ENUM_ELEM(ANIM_Once);
		ENUM_ELEM(ANIM_Loop);
		ENUM_ELEM(ANIM_PingPong);
	END_ENUM;

	ADD_PROPERTY( bHidden,		TYPE_Bool,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( Color,		TYPE_Color,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( bUnlit,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipH,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipV,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Animation,	TYPE_Resource,	1,					PROP_Editable,	FAnimation::MetaClass );
	ADD_PROPERTY( Offset,		TYPE_Vector,	1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Scale,		TYPE_Vector,	1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Rotation,		TYPE_Angle,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( iSequence,	TYPE_Byte,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( Frame,		TYPE_Float,		1,					PROP_None,		nullptr );

	DECLARE_METHOD( nativePlayAnim,		TYPE_None,		TYPE_String,	TYPE_Float,	TYPE_Byte,	TYPE_None );
	DECLARE_METHOD( nativeGetAnimName,	TYPE_String,	TYPE_None,		TYPE_None,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativePauseAnim,	TYPE_None,		TYPE_None,		TYPE_None,	TYPE_None,	TYPE_None );

	return 0;
}


REGISTER_CLASS_CPP( FParallaxLayerComponent, FExtraComponent, CLASS_None )
{
	ADD_PROPERTY( Parallax,		TYPE_Vector,	1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Scale,		TYPE_Vector,	1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Offset,		TYPE_Vector,	1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Gap,			TYPE_Vector,	1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( TexCoords,	TYPE_AABB,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bUnlit,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipH,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( bFlipV,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Color,		TYPE_Color,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Bitmap,		TYPE_Resource,	1,					PROP_Editable,	FBitmap::MetaClass );

	return 0;
}


REGISTER_CLASS_CPP( FLabelComponent, FExtraComponent, CLASS_None )
{
	ADD_PROPERTY( bHidden,		TYPE_Bool,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( Color,		TYPE_Color,		1,					PROP_None,		nullptr );
	ADD_PROPERTY( Font,			TYPE_Resource,	1,					PROP_None,		FFont::MetaClass );
	ADD_PROPERTY( Text,			TYPE_String,	1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( Scale,		TYPE_Float,		1,					PROP_Editable,	nullptr );

	return 0;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/