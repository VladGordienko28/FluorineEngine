/*=============================================================================
    FrPhysic.cpp: Physics relative classes.
    Copyright Aug.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FJointComponent implementation.
-----------------------------------------------------------------------------*/

//
// Joint constructor.
//
FJointComponent::FJointComponent()
	:	FRectComponent(),
		CTickAddon( this ),
		Body1( nullptr ),
		Body2( nullptr ),
		Hook1( 0.f, 0.f ),
		Hook2( 0.f, 0.f )
{
}


//
// Initialize joint for level.
//
void FJointComponent::InitForEntity( FEntity* InEntity )
{
	FRectComponent::InitForEntity( InEntity );
	com_add(TickObjects);
}


//
// Serialize joint.
//
void FJointComponent::SerializeThis( CSerializer& S )
{
	FRectComponent::SerializeThis( S );
	Serialize( S, Body1 );
	Serialize( S, Body2 );
	Serialize( S, Hook1 );
	Serialize( S, Hook2 );
}


//
// Import joint.
//
void FJointComponent::Import( CImporterBase& Im )
{
	FRectComponent::Import( Im );
	IMPORT_OBJECT( Body1 );
	IMPORT_OBJECT( Body2 );
	IMPORT_VECTOR( Hook1 );
	IMPORT_VECTOR( Hook2 );
}


//
// Export joint.
//
void FJointComponent::Export( CExporterBase& Ex )
{
	FRectComponent::Export( Ex );
	EXPORT_OBJECT( Body1 );
	EXPORT_OBJECT( Body2 );
	EXPORT_VECTOR( Hook1 );
	EXPORT_VECTOR( Hook2 );
}


//
// Render joint.
//
void FJointComponent::Render( CCanvas* Canvas )
{
	FRectComponent::Render( Canvas );

	// Draw line from Body1 to Body2.
	if( Level->RndFlags & RND_Other )
		if( Body1 && Body2 )
		{
			TVector Point1 = TransformPointBy( Hook1, Body1->Base->ToWorld() );
			TVector Point2 = TransformPointBy( Hook2, Body2->Base->ToWorld() );

			Canvas->DrawLine
						(
							Point1,
							Point2,
							COLOR_LimeGreen,
							false
						);
		}
}


/*-----------------------------------------------------------------------------
    FSpringComponent implementation.
-----------------------------------------------------------------------------*/

//
// Spring constructor.
//
FSpringComponent::FSpringComponent()
	:	FJointComponent(),
		Damping( 5.f ),			
		Spring( 5.f ),
		Length( 8.f ),
		NumSegs( 3 ),
		Segment( nullptr ),
		Width( 1.f )
{
}


//
// Spring serialization.
//
void FSpringComponent::SerializeThis( CSerializer& S )
{
	FJointComponent::SerializeThis( S );
	Serialize( S, Damping );
	Serialize( S, Spring );
	Serialize( S, Length );
	Serialize( S, NumSegs );
	Serialize( S, Segment );
	Serialize( S, Width );
}


//
// Import spring.
//
void FSpringComponent::Import( CImporterBase& Im )
{
	FJointComponent::Import( Im );
	IMPORT_FLOAT( Damping );
	IMPORT_FLOAT( Spring );
	IMPORT_FLOAT( Length );
	IMPORT_INTEGER( NumSegs );
	IMPORT_OBJECT( Segment );
	IMPORT_FLOAT( Width );
}


//
// Export spring.
//
void FSpringComponent::Export( CExporterBase& Ex )
{
	FJointComponent::Export( Ex );
	EXPORT_FLOAT( Damping );
	EXPORT_FLOAT( Spring );
	EXPORT_FLOAT( Length );
	EXPORT_INTEGER( NumSegs );
	EXPORT_OBJECT( Segment );
	EXPORT_FLOAT( Width );
}


//
// Spring rendering.
//
void FSpringComponent::Render( CCanvas* Canvas )
{
	FJointComponent::Render( Canvas );

	// Render a spring.
	if( Body1 && Body2 && Segment && NumSegs >= 0 )
	{
		// Get hooks location.
		TVector Point1 = TransformPointBy( Hook1, Body1->Base->ToWorld() );
		TVector Point2 = TransformPointBy( Hook2, Body2->Base->ToWorld() );

		// Setup spring rectangle.
		TRenderRect Rect;
		Rect.Bitmap				= Segment;
		Rect.Color				= bSelected ? TColor( 0x80, 0xe6, 0x80, 0xff ) : Color;
		Rect.Flags				= POLY_Unlit * (bUnlit | !(Level->RndFlags & RND_Lighting));
		Rect.Rotation			= VectorToAngle((Point1-Point2).Cross());
		Rect.TexCoords			= TRect( TVector( 0.5f, 0.f ), TVector( 1.f, NumSegs ) );
		Rect.Bounds				=	TRect
									( 
										(Point1+Point2) * 0.5, 
										TVector( Width, (Point1-Point2).Size() ) 
									);

		// Flipping.
		if( bFlipH )	Exchange( Rect.TexCoords.Min.X, Rect.TexCoords.Max.X );
		if( bFlipV )	Exchange( Rect.TexCoords.Min.Y, Rect.TexCoords.Max.Y );

		// Draw spring.
		Canvas->DrawRect( Rect );
	}
}


//
// Setup spring physics.
//
void FSpringComponent::PreTick( Float Delta )
{
	if( !Body1 || !Body2 )
		return;
	
	FPhysicComponent* Phys1	= As<FPhysicComponent>( Body1->Base );
	FPhysicComponent* Phys2	= As<FPhysicComponent>( Body2->Base );

	TCoords ToWorld1 = Body1->Base->ToWorld(),
			ToWorld2 = Body2->Base->ToWorld();

	TVector	Point1	= TransformPointBy( Hook1, ToWorld1 ),
			Point2	= TransformPointBy( Hook2, ToWorld2 );

	TVector	Rad1	= TransformVectorBy( Hook1, ToWorld1 ),
			Rad2	= TransformVectorBy( Hook2, ToWorld2 );

	TVector Vel1	= Phys1 ? Phys1->Velocity : TVector( 0.f, 0.f );
	TVector Vel2	= Phys2 ? Phys2->Velocity : TVector( 0.f, 0.f );

	TVector RelVel, RelLoc;
	RelVel	= Vel2 - Vel1;
	RelLoc	= Point2 - Point1;

	Float DL, S;
	TVector F;

	DL	= RelLoc.Size() - Length;
	S	= Spring * DL;
	RelLoc.Normalize();

	F	= (RelLoc * S) + RelLoc*(Damping*(RelVel*RelLoc));

	// Test for sleeping.
	FRigidBodyComponent*	Rigid1	= As<FRigidBodyComponent>(Phys1);
	FRigidBodyComponent*	Rigid2	= As<FRigidBodyComponent>(Phys2);

	// Apply impulses.
	if( Phys1 && !(Rigid1 && Rigid1->bSleeping) )
	{
		Phys1->Forces	+= F;
		Phys1->Torque	+= Rad1 / F;
	}
	if( Phys2 && !(Rigid2 && Rigid2->bSleeping) )
	{
		Phys2->Forces	-= F;
		Phys2->Torque	-= Rad2 / F;
	}
}


/*-----------------------------------------------------------------------------
    FHingeComponent implementation.
-----------------------------------------------------------------------------*/

//
// Hinge constructor.
//
FHingeComponent::FHingeComponent()
	:	FJointComponent()
{
}


//
// Hinge serialization.
//
void FHingeComponent::SerializeThis( CSerializer& S )
{
	FJointComponent::SerializeThis(S);
}


//
// Hinge import.
//
void FHingeComponent::Import( CImporterBase& Im )
{
	FJointComponent::Import(Im);
}


//
// Hinge export.
//
void FHingeComponent::Export( CExporterBase& Ex )
{
	FJointComponent::Export(Ex);
}


//
// Hinge joint rendering.
//
void FHingeComponent::Render( CCanvas* Canvas )
{
	FJointComponent::Render( Canvas );

	if( Level->RndFlags & RND_Other )		
	{
		// Render Body1.
		if( Body1 )
		{
			TVector Pin = TransformPointBy( Hook1, Body1->Base->ToWorld() );
			Canvas->DrawLineStar( Pin, Body1->Base->Rotation, 2.f, COLOR_LightCoral, false );
			Canvas->DrawPoint( Pin, 5.f, COLOR_LightCoral );
		}

		// Render Body2.
		if( Body2 )
		{
			TVector Pin = TransformPointBy( Hook2, Body2->Base->ToWorld() );
			Canvas->DrawLineStar( Pin, Body2->Base->Rotation, 2.f, COLOR_LightBlue, false );
			Canvas->DrawPoint( Pin, 5.f, COLOR_LightBlue );
		}
	}
}


//
// Setup hinge physics.
//
void FHingeComponent::PreTick( Float Delta )
{
	// Check bodies.
	if( !Body1 || !Body2 )
		return;

	FPhysicComponent* Phys2 = As<FPhysicComponent>(Body2->Base);
	if( !Phys2 )
		return;

	// Unhash body to perform movement.
	if( Phys2->bHashable ) 
		Level->CollHash->RemoveFromHash(Phys2);
	{
		TVector	Pin1	= TransformPointBy( Hook1, Body1->Base->ToWorld() );
		TVector	Pin2	= TransformPointBy( Hook2, Body2->Base->ToWorld() );
		TVector	Fix		= Pin1 - Pin2;

		// Move body to pin, and eliminate movement
		// forces.
		Phys2->Location	+= Fix;
		Phys2->Velocity	= TVector( 0.f, 0.f );
		Phys2->Forces	= TVector( 0.f, 0.f );
	}
	if( Phys2->bHashable )
		Level->CollHash->AddToHash(Phys2);
}


/*-----------------------------------------------------------------------------
    FKeyframeComponent implementation.
-----------------------------------------------------------------------------*/

//
// Keyframe constructor.
//
FKeyframeComponent::FKeyframeComponent()
	:	CTickAddon( this ),
		Points(),
		GlideType( GLIDE_None ),
		iTarget( -1 ),
		bLooped( false ),
		Speed( 0.f ),
		Progress( 0.f ),
		StartLocation( 0.f, 0.f )
{
}


//
// Initialize keyframe for entity.
//
void FKeyframeComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity( InEntity );
	com_add(TickObjects);
}


//
// Game just started.
//
void FKeyframeComponent::BeginPlay()
{
	FExtraComponent::BeginPlay();
}


//
// Tick keyframe.
//
void FKeyframeComponent::Tick( Float Delta )
{
	if( GlideType != GLIDE_None )
		CPhysics::PhysicKeyframe( this, Delta );
}


//
// Keypoint serialization.
//
void Serialize( CSerializer& S, TKeyframePoint& V )
{
	Serialize( S, V.Location );
	Serialize( S, V.Rotation );
	Serialize( S, V.bCCW );
}


//
// Serialize keyframe.
//
void FKeyframeComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );
	Serialize( S, Points );

#if 0
	// Is really should serialize all value, cause
	// them used during play only.
	SerializeEnum( S, GlideType );
	Serialize( S, iTarget );
	Serialize( S, bLooped );
	Serialize( S, Speed );
	Serialize( S, Progress );
	Serialize( S, StartLocation );
#endif
}


//
// Import the keyframe.
//
void FKeyframeComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );

	Points.SetNum( Im.ImportInteger(L"NumPoints") );
	for( Integer i=0; i<Points.Num(); i++ )
	{
		Points[i].Location	= Im.ImportVector( *String::Format( L"Points[%d].Location", i ) );
		Points[i].Rotation	= Im.ImportAngle( *String::Format( L"Points[%d].Rotation", i ) );
		Points[i].bCCW		= Im.ImportBool( *String::Format( L"Points[%d].bCCW", i ) );
	}
}


//
// Export the keyframe.
//
void FKeyframeComponent::Export( CExporterBase& Ex )
{
	FExtraComponent::Export( Ex );

	Ex.ExportInteger( L"NumPoints", Points.Num() );
	for( Integer i=0; i<Points.Num(); i++ )
	{
		Ex.ExportVector( *String::Format( L"Points[%d].Location", i ), Points[i].Location );
		Ex.ExportAngle( *String::Format( L"Points[%d].Rotation", i ), Points[i].Rotation );
		Ex.ExportBool( *String::Format( L"Points[%d].bCCW", i ), Points[i].bCCW );
	}
}

//
// Start moving.
//
void FKeyframeComponent::nativeStart( CFrame& Frame )
{
	Speed		= POP_FLOAT;
	bLooped		= POP_BOOL;
	GlideType	= GLIDE_Forward;
}


//
// Stop moving.
//
void FKeyframeComponent::nativeStop( CFrame& Frame )
{
	GlideType	= GLIDE_None;
	Speed		= 0.f;
}


//
// Move to the key from current location.
//
void FKeyframeComponent::nativeMoveTo( CFrame& Frame )
{
	Speed			= POP_FLOAT;
	iTarget			= Clamp( POP_INTEGER, 0, Points.Num()-1 );
	StartLocation	= Base->Location;
	GlideType		= GLIDE_Target;
	Progress		= 0.f;
}


/*-----------------------------------------------------------------------------
    FRigidBodyComponent implementation.
-----------------------------------------------------------------------------*/

// Physics per frame iterations count.		
enum{ NUM_PHYS_ITERS = 3 };


//
// Initialize rigid body.
//
FRigidBodyComponent::FRigidBodyComponent()
	:	FPhysicComponent(),
		bSleeping( false ),
		bCanSleep( true )
{
}


//
// Pre-tick physics.
//
void FRigidBodyComponent::PreTick( Float Delta )
{
	if( !bCanSleep || !bSleeping )
	{
		// Let's script setup forces.
		Entity->CallEvent( EVENT_OnPreTick, Delta );

		// Let's physics engine prepare.
		CPhysics::SetupPhysics( this, Delta );
	}
}


//
// Tick rigid body.
//
void FRigidBodyComponent::Tick( Float Delta )
{
	if( !bCanSleep || !bSleeping )
	{
		// Process physics.
		for( Integer i=0; i<NUM_PHYS_ITERS; i++ )
			CPhysics::PhysicComplex( this, Delta*(1.f/NUM_PHYS_ITERS) );

		// Notify script.
		Entity->CallEvent( EVENT_OnTick, Delta );
	}
}


//
// Serialize body.
//
void FRigidBodyComponent::SerializeThis( CSerializer& S )
{
	FPhysicComponent::SerializeThis( S );
	SerializeEnum( S, Material );
	Serialize( S, bCanSleep );
	Serialize( S, bSleeping );
	Serialize( S, Inertia );
	Serialize( S, Forces );
	Serialize( S, Torque );
}


//
// Import body properties.
//
void FRigidBodyComponent::Import( CImporterBase& Im )
{
	FPhysicComponent::Import( Im );

	IMPORT_BYTE( Material );
	IMPORT_BOOL( bCanSleep );
	IMPORT_BOOL( bSleeping );
	IMPORT_FLOAT( Inertia );
}


//
// Export body properties.
//
void FRigidBodyComponent::Export( CExporterBase& Ex )
{
	FPhysicComponent::Export( Ex );

	EXPORT_BYTE( Material );
	EXPORT_BOOL( bCanSleep );
	EXPORT_BOOL( bSleeping );
	EXPORT_FLOAT( Inertia );
}


//
// When some rigid body property changed via
// Object inspector.
//
void FRigidBodyComponent::EditChange()
{
	FPhysicComponent::EditChange();

	if( Material != PM_Custom )
		CPhysics::ComputeRigidMaterial( this );
}


/*-----------------------------------------------------------------------------
    FArcadeBodyComponent implementation.
-----------------------------------------------------------------------------*/

//
// Initialize arcade body.
//
FArcadeBodyComponent::FArcadeBodyComponent()
	:	FPhysicComponent()
{
}


//
// Pre-tick arcade physics.
//
void FArcadeBodyComponent::PreTick( Float Delta )
{
	// Let's script setup forces.
	Entity->CallEvent( EVENT_OnPreTick, Delta );

	// Let's physics engine prepare.
	CPhysics::SetupPhysics( this, Delta );
}


//
// Tick arcade body.
//
void FArcadeBodyComponent::Tick( Float Delta )
{
	// Process physics.
	CPhysics::PhysicArcade( this, Delta );

	// Notify script.
	Entity->CallEvent( EVENT_OnTick, Delta );
}


/*-----------------------------------------------------------------------------
    FPhysicComponent implementation.
-----------------------------------------------------------------------------*/

//
// Physic body constructor.
//
FPhysicComponent::FPhysicComponent()
	:	FRectComponent(),
		CTickAddon( this ),
		Material( PM_Custom ),
		Velocity( 0.f, 0.f ),
		Mass( 1.f ),
		Inertia( 0.f ),
		Forces( 0.f, 0.f ),
		AngVelocity( 0.f ),
		Torque( 0.f ),
		Floor( nullptr ),
		Zone( nullptr )
{
	MemZero( Touched, sizeof(Touched) );
	bHashable	= true;
}


//
// Initialize body for entity.
//
void FPhysicComponent::InitForEntity( FEntity* InEntity )
{
	FRectComponent::InitForEntity( InEntity );
	com_add(TickObjects);
}


//
// Serialize physics fields.
//
void FPhysicComponent::SerializeThis( CSerializer& S )
{
	FRectComponent::SerializeThis( S );
	
	// It's just serialize common physics
	// properties.
	Serialize( S, Velocity );
	Serialize( S, AngVelocity );
	Serialize( S, Mass );
	Serialize( S, Floor );
	Serialize( S, Zone );

	for( Integer i=0; i<array_length(Touched); i++ )
		Serialize( S, Touched[i] );
}


//
// Export all properties.
//
void FPhysicComponent::Export( CExporterBase& Ex )
{
	FRectComponent::Export( Ex );

	// Just this properties, other will stored in
	// subclasses or used only in run-time.
	EXPORT_VECTOR( Velocity );
	EXPORT_FLOAT( AngVelocity );
	EXPORT_FLOAT( Mass );
}


//
// Import all properties.
//
void FPhysicComponent::Import( CImporterBase& Im )
{
	FRectComponent::Import( Im );

	// Just this properties, other will restored in
	// subclasses or used only in run-time.
	IMPORT_VECTOR( Velocity );
	IMPORT_FLOAT( AngVelocity );
	IMPORT_FLOAT( Mass );
}


//
// Solve solid collision hit.
//
void FPhysicComponent::nativeSolveSolid( CFrame& Frame )
{
	CPhysics::bBrake	= POP_BOOL || CPhysics::bBrake;
	CPhysics::Solution	= Max( CPhysics::Solution, HSOL_Solid );
}


//
// Solve oneway collision hit. Very useful
// for platformer stuff.
//
void FPhysicComponent::nativeSolveOneway( CFrame& Frame )
{
	CPhysics::bBrake	= POP_BOOL || CPhysics::bBrake;
	CPhysics::Solution	= Max( CPhysics::Solution, HSOL_Oneway );
}


//
// Return true if object touching other.
//
void FPhysicComponent::nativeIsTouching( CFrame& Frame )
{
	FEntity*	Other	= POP_ENTITY;
	Bool		Result	= false;

	if( Other )
		for( Integer i=0; i<array_length(Touched); i++ )
			if( Touched[i] == Other )
			{
				Result	= true;
				break;
			}

	*POPA_BOOL	= Result;
}


/*-----------------------------------------------------------------------------
    FMovingPlatformComponent implementation.
-----------------------------------------------------------------------------*/

//
// Mover constructor.
//
FMoverComponent::FMoverComponent()
	:	FRectComponent(),
		CTickAddon( this ),
		NumRds( 0 ),
		OldLocation( 0.f, 0.f )
{
	MemZero( Riders, sizeof(Riders) );
	bHashable	= true;
}


//
// Initialize mover for game.
//
void FMoverComponent::BeginPlay()
{
	FRectComponent::BeginPlay();
	
	OldLocation		= Location;
	Reset();
}


//
// Initialize mover for entity.
//
void FMoverComponent::InitForEntity( FEntity* InEntity )
{
	FRectComponent::InitForEntity( InEntity );
	com_add(TickObjects);
}


//
// Reset the mover.
//
void FMoverComponent::Reset()
{
	NumRds	= 0;
}


//
// Add another passenger to this mover.
//
Bool FMoverComponent::AddRider( FPhysicComponent* InRider )
{
	if( NumRds < MAX_RIDERS )
	{
		Riders[NumRds++]	= InRider;
		return true;
	}
	else
	{
		log( L"Phys: Mover '%s' exceeded %d riders", *GetFullName(), MAX_RIDERS );
		return false;
	}
}


//
// Serialize a mover.
//
void FMoverComponent::SerializeThis( CSerializer& S )
{
	FRectComponent::SerializeThis( S );

	// Don't save or load properties.
	if( S.GetMode() == SM_Undefined )
	{
		Serialize( S, OldLocation );
		Serialize( S, NumRds );
		for( Integer iRd=0; iRd<NumRds; iRd++ )
			Serialize( S, Riders[iRd] );
	}
}


//
// Step moving platform in current frame.
//
void FMoverComponent::PreTick( Float Delta )
{
	Level->CollHash->RemoveFromHash( this );
	{
		// Handle the manual script moving.
		// Let's script modify location.
		Entity->CallEvent( EVENT_OnStep, Delta );

		// Compute delta.
		TVector Shift	= Location - OldLocation;
		OldLocation		= Location;					

		// Move each rider.
		{
#if 1
			// Drown riders a little, this is a little hack 
			// but works well.
			Shift	-= TVector( 0.f, 4.f ) * Delta;
#endif
			for( Integer iRd=0; iRd<NumRds; iRd++ )
			{
				Level->CollHash->RemoveFromHash( Riders[iRd] );
				{
					Riders[iRd]->Location	+= Shift;
				}
				Level->CollHash->AddToHash( Riders[iRd] );
			}
		}
	}
	Level->CollHash->AddToHash( this );

	// Reset mover until new time pass.
	Reset();
}


/*-----------------------------------------------------------------------------
    Registration.
-----------------------------------------------------------------------------*/

REGISTER_CLASS_CPP( FKeyframeComponent, FExtraComponent, CLASS_None )
{
	ADD_PROPERTY( iTarget,		TYPE_Byte,		1,	PROP_None,		nullptr );
	ADD_PROPERTY( bLooped,		TYPE_Bool,		1,	PROP_None,		nullptr );
	ADD_PROPERTY( Speed,		TYPE_Float,		1,	PROP_None,		nullptr );
	ADD_PROPERTY( Progress,		TYPE_Float,		1,	PROP_None,		nullptr );

	DECLARE_METHOD( nativeStart,	TYPE_None,		TYPE_Float,		TYPE_Bool,		TYPE_None, TYPE_None );
	DECLARE_METHOD( nativeStop,		TYPE_None,		TYPE_None,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_METHOD( nativeMoveTo,	TYPE_None,		TYPE_Float,		TYPE_Integer,	TYPE_None, TYPE_None );
	return 0;
}
    

REGISTER_CLASS_CPP( FRigidBodyComponent, FPhysicComponent, CLASS_None )
{
	BEGIN_ENUM(EPhysMaterial)
		ENUM_ELEM(PM_Manual);
		ENUM_ELEM(PM_Wood);
		ENUM_ELEM(PM_Rock);
		ENUM_ELEM(PM_Metal);
		ENUM_ELEM(PM_Ice);
		ENUM_ELEM(PM_Glass);
		ENUM_ELEM(PM_BouncyBall);
	END_ENUM;

	ADD_PROPERTY( bCanSleep,	TYPE_Bool,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( bSleeping,	TYPE_Bool,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Material,		TYPE_Byte,		1,	PROP_Editable,	_EPhysMaterial );

	return 0;
}


REGISTER_CLASS_CPP( FArcadeBodyComponent, FPhysicComponent, CLASS_None )
{
	return 0;
}


REGISTER_CLASS_CPP( FMoverComponent, FRectComponent, CLASS_None )
{
	ADD_PROPERTY( OldLocation,	TYPE_Vector,	1,	PROP_None,		nullptr );
	return 0;
}


REGISTER_CLASS_CPP( FPhysicComponent, FRectComponent, CLASS_Abstract )
{
	BEGIN_ENUM(EHitSolution)
		ENUM_ELEM(HSOL_None);
		ENUM_ELEM(HSOL_Oneway);
		ENUM_ELEM(HSOL_Solid);
	END_ENUM;

	BEGIN_ENUM(EHitSide)
		ENUM_ELEM(HSIDE_Top);
		ENUM_ELEM(HSIDE_Bottom);
		ENUM_ELEM(HSIDE_Left);
		ENUM_ELEM(HSIDE_Right);
	END_ENUM;

	ADD_PROPERTY( Velocity,		TYPE_Vector,	1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Mass,			TYPE_Float,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Inertia,		TYPE_Float,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Forces,		TYPE_Vector,	1,	PROP_None,		nullptr );
	ADD_PROPERTY( AngVelocity,	TYPE_Float,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Torque,		TYPE_Float,		1,	PROP_None,		nullptr );
	ADD_PROPERTY( Floor,		TYPE_Entity,	1,	PROP_None,		nullptr );
	ADD_PROPERTY( Zone,			TYPE_Entity,	1,	PROP_None,		nullptr );
	ADD_PROPERTY( Touched,		TYPE_Entity,	4,	PROP_None,		nullptr );

	DECLARE_METHOD( nativeSolveSolid,	TYPE_None,		TYPE_Bool,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_METHOD( nativeSolveOneway,	TYPE_None,		TYPE_Bool,		TYPE_None,		TYPE_None, TYPE_None );
	DECLARE_METHOD( nativeIsTouching,	TYPE_Bool,		TYPE_Entity,	TYPE_None,		TYPE_None, TYPE_None );

	return 0;
}


REGISTER_CLASS_CPP( FJointComponent, FRectComponent, CLASS_Abstract )
{
	ADD_PROPERTY( Body1,		TYPE_Entity,	1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Body2,		TYPE_Entity,	1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Hook1,		TYPE_Vector,	1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Hook2,		TYPE_Vector,	1,	PROP_Editable,	nullptr );

	return 0;
}


REGISTER_CLASS_CPP( FSpringComponent, FJointComponent, CLASS_None )
{
	ADD_PROPERTY( Damping,		TYPE_Float,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Spring,		TYPE_Float,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Length,		TYPE_Float,		1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( NumSegs,		TYPE_Integer,	1,	PROP_Editable,	nullptr );
	ADD_PROPERTY( Segment,		TYPE_Resource,	1,	PROP_Editable,	FBitmap::MetaClass );
	ADD_PROPERTY( Width,		TYPE_Float,		1,	PROP_Editable,	nullptr );

	return 0;
}


REGISTER_CLASS_CPP( FHingeComponent, FJointComponent, CLASS_None )
{
	return 0;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/