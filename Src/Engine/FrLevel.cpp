/*=============================================================================
    FrLevel.cpp: FLevel implementation.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FLevel implementation.
-----------------------------------------------------------------------------*/

//
// Level's constructor.
//
FLevel::FLevel()
	:	Original( nullptr ),
		RndFlags( RND_Game ),
		Camera( nullptr ) ,
		Sky( nullptr ),
		bIsPlaying( false ),
		bIsPause( false ),
		GameSpeed( 1.f ),
		Soundtrack( nullptr ),
		ScrollClamp( TVector(0.f, 0.f), WORLD_SIZE ),
		CollHash( nullptr ),
		GFXManager( nullptr ),
		Navigator( nullptr ),
		AmbientLight( COLOR_Black )
{
	Effect[0] = Effect[1] = Effect[2] = 1.f;
	Effect[3] = Effect[4] = Effect[5] = 1.f;
	Effect[6] = Effect[7] = Effect[8] = 0.f;
	Effect[9] = 0.f;
}


//
// Level's destructor.
//
FLevel::~FLevel()
{
	// Test state.
	assert(CollHash == nullptr);
	assert(GFXManager == nullptr);

	// Kill navigator.
	freeandnil(Navigator);

	// Destroy all my entities.
	for( Integer i=0; i<Entities.Num(); i++ )
		if( Entities[i] )
			DestroyObject( Entities[i], true );
}


//
// Level serialization.
//
void FLevel::SerializeThis( CSerializer& S )
{
	FResource::SerializeThis( S );

	Serialize( S, Original );
	Serialize( S, RndFlags );
	Serialize( S, Entities );
	Serialize( S, Camera );
	Serialize( S, Sky );
	Serialize( S, bIsPlaying );
	Serialize( S, bIsPause );
	Serialize( S, GameSpeed );
	Serialize( S, Soundtrack );
	Serialize( S, ScrollClamp );
	Serialize( S, Navigator );
	Serialize( S, AmbientLight );
	S.SerializeData( Effect, sizeof(Effect) );

	// Warning: Don't serialize level databases of
	// entities or components, because it
	// already serialized in the FComponent or
	// FEntity ::SerializeThis.
	
	// For some reasons it's failed.
	// Cleanup entities db to avoid null objects.
	//if( S.GetMode() == SM_Undefined )
	//	CLEANUP_ARR_NULL(Entities);
}


//
// Restore level after game loading.
//
void FLevel::PostLoad()
{
	FResource::PostLoad();
}


//
// Export the level.
//
void FLevel::Export( CExporterBase& Ex )
{
	FResource::Export( Ex );

	EXPORT_INTEGER(RndFlags);
	EXPORT_OBJECT(Camera);
	EXPORT_OBJECT(Sky);
	EXPORT_BOOL(bIsPlaying);
	EXPORT_BOOL(bIsPause);
	EXPORT_FLOAT(GameSpeed);
	EXPORT_OBJECT(Soundtrack);
	EXPORT_AABB(ScrollClamp);
	EXPORT_COLOR(AmbientLight);

	// Export effect as array.
	for( Integer i=0; i<array_length(Effect); i++ )
		Ex.ExportFloat( *String::Format(L"Effect[%i]", i), Effect[i] );

	// All entity databases will be
	// exported in exporter.
}


//
// Import the level.
//
void FLevel::Import( CImporterBase& Im )
{
	FResource::Import( Im );

	IMPORT_INTEGER(RndFlags);
	IMPORT_OBJECT(Camera);
	IMPORT_OBJECT(Sky);
	IMPORT_BOOL(bIsPlaying);
	IMPORT_BOOL(bIsPause);
	IMPORT_FLOAT(GameSpeed);
	IMPORT_OBJECT(Soundtrack);
	IMPORT_AABB(ScrollClamp);
	IMPORT_COLOR(AmbientLight);

	// Import effect as array.
	for( Integer i=0; i<array_length(Effect); i++ )
		Effect[i]	= Im.ImportFloat(*String::Format(L"Effect[%i]", i));

	// All entity databases will be
	// imported in importer.
}


/*-----------------------------------------------------------------------------
    Gameplay!
-----------------------------------------------------------------------------*/

//
// Begin play level.
//
void FLevel::BeginPlay()
{
	// Allocate collision hash.
	CollHash	= new CCollisionHash( this );

	// Level's GFX.
	GFXManager	= new CGFXManager( this );

	// Notify all entities and their components.
	for( Integer i=0; i<Entities.Num(); i++ )
		Entities[i]->BeginPlay();

	// Mark level as played.
	bIsPlaying		= true;

	// Play music!
	if( Soundtrack )
		GApp->GAudio->PlayMusic( Soundtrack, 2.f );

	// Notify all entities because everything are initialized
	// for playing.
	for( Integer i=0; i<Entities.Num(); i++ )
		if( Entities[i]->Script->bHasText )
			Entities[i]->CallEvent(EVENT_OnBeginPlay);	
}


//
// Stop level playing.
//
void FLevel::EndPlay()
{
	// Mark level as not played.
	bIsPlaying		= false;

	// Call in script OnEndPlay, while all objects are
	// valid.
	for( Integer i=0; i<Entities.Num(); i++ )
		if( Entities[i]->Script->bHasText )
			Entities[i]->CallEvent(EVENT_OnEndPlay);	

	// Notify all entities and their components.
	for( Integer i=0; i<Entities.Num(); i++ )
		Entities[i]->EndPlay();

	// Release the collision hash.
	assert(CollHash);
	delete CollHash;
	CollHash	= nullptr;

	// Release GFX man.
	assert(GFXManager);
	delete GFXManager;
	GFXManager	= nullptr;

	// Flush all level's ambients.
	GApp->GAudio->FlushAmbients();

	// Stop music.
	if( Soundtrack )
		GApp->GAudio->PlayMusic( nullptr, 2.f );
}


//
// Incoming level global variable.
//
TIncomingLevel			GIncomingLevel;
TMap<String, String>	GStaticBuffer;


/*-----------------------------------------------------------------------------
    Level collisions.
-----------------------------------------------------------------------------*/

//
// Figure out is point inside brush. If point is
// outside of any brush return nullptr.
//
FBrushComponent* FLevel::TestPointGeom( const TVector& P )
{
	assert(bIsPlaying && CollHash);

	// Get list of brushes.
	FBrushComponent* Brushes[MAX_COLL_LIST_OBJS];
	Integer NumBrshs;
	CollHash->GetOverlappedByClass
								( 
									TRect( P, 0.1f ),
									FBrushComponent::MetaClass,
									NumBrshs,
									(FBaseComponent**)Brushes
								);

	for( Integer iBrush=0; iBrush<NumBrshs; iBrush++ )
	{
		FBrushComponent* Brush = Brushes[iBrush];

		if( Brush->Type == BRUSH_Solid )
		{
			// Transform test point to the Brush's local coords.
			TVector LP = P - Brush->Location;

			if( IsPointInsidePoly( LP, Brush->Vertices, Brush->NumVerts ) )
				return Brush;
		}
	}

	// Not found.
	return nullptr;
}


//
// Test a line with a level geometry.
//
FBrushComponent* FLevel::TestLineGeom( const TVector& A, const TVector& B, Bool bFast, TVector& Hit, TVector& Normal )
{
	assert(bIsPlaying && CollHash);

	Float BestTime			= 100000.0f;
	FBrushComponent* Result	= nullptr;

	// Get line bounds.
	TRect Bounds;
	Bounds.Min.X	= Min( A.X, B.X );
	Bounds.Min.Y	= Min( A.Y, B.Y );
	Bounds.Max.X	= Max( A.X, B.X );
	Bounds.Max.Y	= Max( A.Y, B.Y );

	// Get list of brushes.
	FBrushComponent* Brushes[MAX_COLL_LIST_OBJS];
	Integer NumBrshs;
	CollHash->GetOverlappedByClass
								( 
									Bounds,
									FBrushComponent::MetaClass,
									NumBrshs,
									(FBaseComponent**)Brushes
								);

	for( Integer iBrush=0; iBrush<NumBrshs; iBrush++ )
	{
		FBrushComponent* Brush = Brushes[iBrush];

		if( Brush->Type != BRUSH_NotSolid )
		{
			// Test collision with solid/semi-solid brush.
			Float TestTime;
			TVector TestHit, TestNormal;
			TVector LA = A - Brush->Location;
			TVector LB = B - Brush->Location;

			if( LineIntersectPoly( LA, LB, Brush->Vertices, Brush->NumVerts, TestHit, TestNormal ) )
			{
				if( Brush->Type==BRUSH_Solid || (Brush->Type==BRUSH_SemiSolid && IsWalkable(TestNormal)) )
				{
					TestTime	= ( TestHit + Brush->Location - A ).SizeSquared();

					if( TestTime < BestTime )
					{
						// Least time.
						Hit			= TestHit + Brush->Location;
						Normal		= TestNormal;
						BestTime	= TestTime;
						Result		= Brush;

						if( bFast ) 
							return Brush;
					}
				}
			}
		}
	}

	return Result;
}


/*-----------------------------------------------------------------------------
    Tick.
-----------------------------------------------------------------------------*/

//
// Update level.
//
void FLevel::Tick( Float Delta )
{
	// Clamp delta.
	Delta	= Clamp( Delta, 1.f/500.f, 1.f/30.f );

	// Modify delta according to game speed.
	GameSpeed	= Clamp( GameSpeed, 0.01f, 10.f );
	Delta		*= GameSpeed;

	// Are we play now?
	if( bIsPlaying && !bIsPause )
	{
		// Normally play level.
		for( Integer i=0; i<Entities.Num(); i++ )
			if( Entities[i]->Thread )
				Entities[i]->Thread->Tick( Delta );

		for( Integer i=0; i<TickObjects.Num(); i++ )
			TickObjects[i]->PreTick( Delta );

		for( Integer i=0; i<TickObjects.Num(); i++ )
			TickObjects[i]->Tick( Delta );

		// Update GFX interpolation.
		GFXManager->Tick( Delta );
	}
	else
	{
		// We not play, just edit level or in pause.
		for( Integer i=0; i<TickObjects.Num(); i++ )
			TickObjects[i]->TickNonPlay( Delta );
	}

	// Destroy all marked entities.
	for( Integer iEntity=0; iEntity<Entities.Num(); )
		if( Entities[iEntity]->Base->bDestroyed )
		{
			ReleaseEntity( iEntity );
		}
		else
			iEntity++;
}


/*-----------------------------------------------------------------------------
    Level entity functions.
-----------------------------------------------------------------------------*/

//
// Create a new entity. Use only it, because it
// create, register and prepare entity for playing.
//
FEntity* FLevel::CreateEntity( FScript* InScript, String InName, TVector InLocation )
{
	assert(InScript);

	// Select name.
	String EntityName;

	if( InName )
	{
		// Name specified. 
		EntityName	= InName;
	}
	else
	{
		// Generate new unique name.
		for( Integer iUniq=0; ; iUniq++ )
		{
			String TestName = String::Format( L"%s%d", *InScript->GetName(), iUniq );
			if( !GObjectDatabase->FindObject( TestName, FEntity::MetaClass, this ) )
			{
				EntityName = TestName;
				break;
			}
		}
	}

	// Allocate new entity.
	FEntity* Entity = NewObject<FEntity>( EntityName, this );
	Entities.Push( Entity );
	Entity->Init( InScript, this );

	// Initialize fields.
	Entity->Base->Location	= InLocation;
	Entity->Base->Layer		+= RandomRange( -0.01f, +0.01f );

	// Prepare for playing.
	if( bIsPlaying )
	{
		Entity->BeginPlay();
	}

	// Return it!
	return Entity;
}


//
// Destroy the entity. Actually just mark,
// entity will destroyed after entire level tick.
//
void FLevel::DestroyEntity( FEntity* Entity )
{
	assert(Entity);
	assert(Entity->Level==this);

	Entity->Base->bDestroyed = true;
}


//
// Totally release entity, don't invoke it in
// FLevel::Tick, call it after level tick, it's
// cleanup all references also.
//
void FLevel::ReleaseEntity( Integer iEntity )
{
	FEntity* Entity = Entities[iEntity];
	assert(Entity && Entity->Base->bDestroyed);

	// Notify about end of play, if play.
	if( bIsPlaying )
	{
		Entity->CallEvent( EVENT_OnDestroy );
		Entity->EndPlay();
	}

	// Let's CObjectDatabase handle it.
	Entities.Remove(iEntity);
	DestroyObject( Entity, true );
}


/*-----------------------------------------------------------------------------
    Level editor functions.
-----------------------------------------------------------------------------*/

//
// Find entity by name. Too slow, but used
// in editor, so it's well.
//
FEntity* FLevel::FindEntity( String InName )
{
	String UpperName = String::UpperCase(InName);

	for( Integer iEnt=0; iEnt<Entities.Num(); iEnt++ )
		if( UpperName == String::UpperCase(Entities[iEnt]->GetName()) )
			return Entities[iEnt];

	// Not found.
	return nullptr; 
}


//
// Return index of the given entity in the level's
// database, if entity not found return -1.
//
Integer FLevel::GetEntityIndex( FEntity* Entity )
{ 
	for( Integer i=0; i<Entities.Num(); i++ )
		if( Entities[i] == Entity )
			return i;

	return -1;
}


/*-----------------------------------------------------------------------------
    FEntity implementation.
-----------------------------------------------------------------------------*/

//
// Entity constructor.
//
FEntity::FEntity()
	:	Level( nullptr ),
		Script( nullptr ),
		Thread( nullptr ),
		InstanceBuffer(nullptr),
		Base( nullptr ),
		Components()
{
}


//
// Entity destructor.
//
FEntity::~FEntity()
{
	assert(Thread == nullptr);

	for( Integer i=0; i<Components.Num(); i++ )
		DestroyObject( Components[i], true );

	DestroyObject( Base, true );

	// Release instance buffer.
	if( InstanceBuffer )
		delete InstanceBuffer;
}


//
// Entity initialization.
//
void FEntity::Init( FScript* InScript, FLevel* InLevel )	
{
	// Initialize fields.
	Script		= InScript;
	Level		= InLevel;

	// Base component.
	FBaseComponent* BasCom = (FBaseComponent*)GObjectDatabase->CopyObject
																	( 
																		Script->Base, 
																		Script->Base->GetName(), 
																		this 
																	);
	BasCom->InitForEntity( this );

	// Extra components.
	for( Integer i=0; i<Script->Components.Num(); i++ )
	{
		FExtraComponent* Source = Script->Components[i];
		FExtraComponent* Com = (FExtraComponent*)GObjectDatabase->CopyObject
																		( 
																			Source, 
																			Source->GetName(), 
																			this 
																		);
		Com->InitForEntity( this );
	}

	// Initialize instance buffer.
	if( Script->InstanceBuffer )
	{
		InstanceBuffer = new CInstanceBuffer( Script );
		InstanceBuffer->Data.SetNum( Script->InstanceSize );

		if( Script->Properties.Num() )
			InstanceBuffer->CopyValues( &Script->InstanceBuffer->Data[0] );
	}
}


//
// Entity serialization.
//
void FEntity::SerializeThis( CSerializer& S )
{
	FObject::SerializeThis( S );

	Serialize( S, Level );
	Serialize( S, Script );	

	Serialize( S, Base );
	Serialize( S, Components );

	// Instance buffer.
	if( S.GetMode() == SM_Load )
	{
		freeandnil(InstanceBuffer);
		InstanceBuffer	= new CInstanceBuffer( Script );
		InstanceBuffer->Data.SetNum( Script->InstanceSize );
		InstanceBuffer->SerializeValues( S );	
	}
	else
	{
		if( InstanceBuffer )
			InstanceBuffer->SerializeValues( S );	
	}

	// Thread shouldn't be serialized since it's in-game
	// only used, and initialized after FLevel::BeginPlay.
}


//
// Entity after loading initialization.
//
void FEntity::PostLoad()
{
	FObject::PostLoad();
}


//
// Entity import.
//
void FEntity::Import( CImporterBase& Im )
{
	FObject::Import( Im );
}


//
// Entity export.
//
void FEntity::Export( CExporterBase& Ex )
{
	FObject::Export( Ex );
}


//
// Notify entity about game start.
// This event prepare entity for playing!
//
void FEntity::BeginPlay()
{
	if( Script->bHasText )
	{
		// Allocate an entity thread if any.
		if( Script->Thread )
			Thread	= new CEntityThread( this, Script->Thread );
	}

	// Notify all components.
	Base->BeginPlay();
	for( Integer e=0; e<Components.Num(); e++ )
		Components[e]->BeginPlay();
}


//
// Notify entity about the end of game.
//
void FEntity::EndPlay()
{
	// Notify all components.
	Base->EndPlay();
	for( Integer e=0; e<Components.Num(); e++ )
		Components[e]->EndPlay();

	// Destroy thread if any.
	if( Thread )
	{
		delete Thread;
		Thread	= nullptr;
	}
}


/*-----------------------------------------------------------------------------
    Registration.
-----------------------------------------------------------------------------*/

REGISTER_CLASS_CPP( FEntity, FObject, CLASS_Sterile )
{
	BEGIN_ENUM(EInputKey);
		ENUM_ELEM(KEY_None);			ENUM_ELEM(KEY_LButton);		
		ENUM_ELEM(KEY_RButton);			ENUM_ELEM(KEY_DblClick);		
		ENUM_ELEM(KEY_MButton);			ENUM_ELEM(KEY_WheelUp);		
		ENUM_ELEM(KEY_WheelDown);		ENUM_ELEM(KEY_AV0x07);		
		ENUM_ELEM(KEY_Backspace);		ENUM_ELEM(KEY_Tab);		
		ENUM_ELEM(KEY_AV0x0a);			ENUM_ELEM(KEY_AV0x0b);		
		ENUM_ELEM(KEY_AV0x0c);			ENUM_ELEM(KEY_Return);		
		ENUM_ELEM(KEY_AV0x0e);			ENUM_ELEM(KEY_AV0x0f);		
		ENUM_ELEM(KEY_Shift);			ENUM_ELEM(KEY_Ctrl);		
		ENUM_ELEM(KEY_Alt);				ENUM_ELEM(KEY_Pause);		
		ENUM_ELEM(KEY_CapsLock);		ENUM_ELEM(KEY_AV0x15);		
		ENUM_ELEM(KEY_AV0x16);			ENUM_ELEM(KEY_AV0x17);	
		ENUM_ELEM(KEY_AV0x18);			ENUM_ELEM(KEY_AV0x19);		
		ENUM_ELEM(KEY_AV0x1a);			ENUM_ELEM(KEY_Escape);		
		ENUM_ELEM(KEY_AV0x1c);			ENUM_ELEM(KEY_AV0x1d);		
		ENUM_ELEM(KEY_AV0x1e);			ENUM_ELEM(KEY_AV0x1f);	
		ENUM_ELEM(KEY_Space);			ENUM_ELEM(KEY_PageUp);		
		ENUM_ELEM(KEY_PageDown);		ENUM_ELEM(KEY_End);		
		ENUM_ELEM(KEY_Home);			ENUM_ELEM(KEY_Left);		
		ENUM_ELEM(KEY_Up);				ENUM_ELEM(KEY_Right);	
		ENUM_ELEM(KEY_Down);			ENUM_ELEM(KEY_Select);		
		ENUM_ELEM(KEY_Print);			ENUM_ELEM(KEY_Execute);		
		ENUM_ELEM(KEY_PrintScrn);		ENUM_ELEM(KEY_Insert);		
		ENUM_ELEM(KEY_Delete);			ENUM_ELEM(KEY_Help);	
		ENUM_ELEM(KEY_0);				ENUM_ELEM(KEY_1);		
		ENUM_ELEM(KEY_2);				ENUM_ELEM(KEY_3);		
		ENUM_ELEM(KEY_4);				ENUM_ELEM(KEY_5);		
		ENUM_ELEM(KEY_6);				ENUM_ELEM(KEY_7);	
		ENUM_ELEM(KEY_8);				ENUM_ELEM(KEY_9);		
		ENUM_ELEM(KEY_AV0x3a);			ENUM_ELEM(KEY_AV0x3b);		
		ENUM_ELEM(KEY_AV0x3c);			ENUM_ELEM(KEY_AV0x3d);		
		ENUM_ELEM(KEY_AV0x3e);			ENUM_ELEM(KEY_AV0x3f);	
		ENUM_ELEM(KEY_AV0x40);			ENUM_ELEM(KEY_A);		
		ENUM_ELEM(KEY_B);				ENUM_ELEM(KEY_C);		
		ENUM_ELEM(KEY_D);				ENUM_ELEM(KEY_E);		
		ENUM_ELEM(KEY_F);				ENUM_ELEM(KEY_G);	
		ENUM_ELEM(KEY_H);				ENUM_ELEM(KEY_I);		
		ENUM_ELEM(KEY_J);				ENUM_ELEM(KEY_K);		
		ENUM_ELEM(KEY_L);				ENUM_ELEM(KEY_M);		
		ENUM_ELEM(KEY_N);				ENUM_ELEM(KEY_O);	
		ENUM_ELEM(KEY_P);				ENUM_ELEM(KEY_Q);		
		ENUM_ELEM(KEY_R);				ENUM_ELEM(KEY_S);		
		ENUM_ELEM(KEY_T);				ENUM_ELEM(KEY_U);		
		ENUM_ELEM(KEY_V);				ENUM_ELEM(KEY_W);	
		ENUM_ELEM(KEY_X);				ENUM_ELEM(KEY_Y);		
		ENUM_ELEM(KEY_Z);				ENUM_ELEM(KEY_AV0x5b);		
		ENUM_ELEM(KEY_AV0x5c);			ENUM_ELEM(KEY_AV0x5d);		
		ENUM_ELEM(KEY_AV0x5e);			ENUM_ELEM(KEY_AV0x5f);	
		ENUM_ELEM(KEY_NumPad0);			ENUM_ELEM(KEY_NumPad1);		
		ENUM_ELEM(KEY_NumPad2);			ENUM_ELEM(KEY_NumPad3);		
		ENUM_ELEM(KEY_NumPad4);			ENUM_ELEM(KEY_NumPad5);		
		ENUM_ELEM(KEY_NumPad6);			ENUM_ELEM(KEY_NumPad7);	
		ENUM_ELEM(KEY_NumPad8);			ENUM_ELEM(KEY_NumPad9);		
		ENUM_ELEM(KEY_Multiply);		ENUM_ELEM(KEY_Add);		
		ENUM_ELEM(KEY_Separator);		ENUM_ELEM(KEY_Subtract);		
		ENUM_ELEM(KEY_Decimal);			ENUM_ELEM(KEY_Divide);	
		ENUM_ELEM(KEY_F1);				ENUM_ELEM(KEY_F2);		
		ENUM_ELEM(KEY_F3);				ENUM_ELEM(KEY_F4);		
		ENUM_ELEM(KEY_F5);				ENUM_ELEM(KEY_F6);		
		ENUM_ELEM(KEY_F7);				ENUM_ELEM(KEY_F8);	
		ENUM_ELEM(KEY_F9);				ENUM_ELEM(KEY_F10);		
		ENUM_ELEM(KEY_F11);				ENUM_ELEM(KEY_F12);		
		ENUM_ELEM(KEY_F13);				ENUM_ELEM(KEY_F14);		
		ENUM_ELEM(KEY_F15);				ENUM_ELEM(KEY_F16);	
		ENUM_ELEM(KEY_F17);				ENUM_ELEM(KEY_F18);		
		ENUM_ELEM(KEY_F19);				ENUM_ELEM(KEY_F20);		
		ENUM_ELEM(KEY_F21);				ENUM_ELEM(KEY_F22);		
		ENUM_ELEM(KEY_F23);				ENUM_ELEM(KEY_F24);	
		ENUM_ELEM(KEY_AV0x88);			ENUM_ELEM(KEY_AV0x89);		
		ENUM_ELEM(KEY_AV0x8a);			ENUM_ELEM(KEY_AV0x8b);		
		ENUM_ELEM(KEY_AV0x8c);			ENUM_ELEM(KEY_AV0x8d);		
		ENUM_ELEM(KEY_AV0x8e);			ENUM_ELEM(KEY_AV0x8f);	
		ENUM_ELEM(KEY_NumLock);			ENUM_ELEM(KEY_ScrollLock);		
		ENUM_ELEM(KEY_AV0x92);			ENUM_ELEM(KEY_AV0x93);		
		ENUM_ELEM(KEY_AV0x94);			ENUM_ELEM(KEY_AV0x95);		
		ENUM_ELEM(KEY_AV0x96);			ENUM_ELEM(KEY_AV0x97);	
		ENUM_ELEM(KEY_AV0x98);			ENUM_ELEM(KEY_AV0x99);		
		ENUM_ELEM(KEY_AV0x9a);			ENUM_ELEM(KEY_AV0x9b);		
		ENUM_ELEM(KEY_AV0x9c);			ENUM_ELEM(KEY_AV0x9d);		
		ENUM_ELEM(KEY_AV0x9e);			ENUM_ELEM(KEY_AV0x9);	
		ENUM_ELEM(KEY_LShift);			ENUM_ELEM(KEY_RShift);		
		ENUM_ELEM(KEY_LControl);		ENUM_ELEM(KEY_RControl);		
		ENUM_ELEM(KEY_JoyUp);			ENUM_ELEM(KEY_JoyDown);		
		ENUM_ELEM(KEY_JoyLeft);			ENUM_ELEM(KEY_JoyRight);	
		ENUM_ELEM(KEY_JoySelect);		ENUM_ELEM(KEY_JoyStart);		
		ENUM_ELEM(KEY_JoyA);			ENUM_ELEM(KEY_JoyB);		
		ENUM_ELEM(KEY_JoyC);			ENUM_ELEM(KEY_JoyX);		
		ENUM_ELEM(KEY_JoyY);			ENUM_ELEM(KEY_JoyZ);	
		ENUM_ELEM(KEY_AV0xb0);			ENUM_ELEM(KEY_AV0xb1);		
		ENUM_ELEM(KEY_AV0xb2);			ENUM_ELEM(KEY_AV0xb3);		
		ENUM_ELEM(KEY_AV0xb4);			ENUM_ELEM(KEY_AV0xb5);		
		ENUM_ELEM(KEY_AV0xb6);			ENUM_ELEM(KEY_AV0xb7);	
		ENUM_ELEM(KEY_AV0xb8);			ENUM_ELEM(KEY_AV0xb9);		
		ENUM_ELEM(KEY_Semicolon);		ENUM_ELEM(KEY_Equals);		
		ENUM_ELEM(KEY_Comma);			ENUM_ELEM(KEY_Minus);		
		ENUM_ELEM(KEY_Period);			ENUM_ELEM(KEY_Slash);	
		ENUM_ELEM(KEY_Tilde);			ENUM_ELEM(KEY_AV0xc1);		
		ENUM_ELEM(KEY_AV0xc2);			ENUM_ELEM(KEY_AV0xc3);		
		ENUM_ELEM(KEY_AV0xc4);			ENUM_ELEM(KEY_AV0xc5);		
		ENUM_ELEM(KEY_AV0xc6);			ENUM_ELEM(KEY_AV0xc7);	
		ENUM_ELEM(KEY_AV0xc8);			ENUM_ELEM(KEY_AV0xc9);		
		ENUM_ELEM(KEY_AV0xca);			ENUM_ELEM(KEY_AV0xcb);		
		ENUM_ELEM(KEY_AV0xcc);			ENUM_ELEM(KEY_AV0xcd);		
		ENUM_ELEM(KEY_AV0xce);			ENUM_ELEM(KEY_AV0xcf);	
		ENUM_ELEM(KEY_AV0xd0);			ENUM_ELEM(KEY_AV0xd1);		
		ENUM_ELEM(KEY_AV0xd2);			ENUM_ELEM(KEY_AV0xd3);		
		ENUM_ELEM(KEY_AV0xd4);			ENUM_ELEM(KEY_AV0xd5);		
		ENUM_ELEM(KEY_AV0xd6);			ENUM_ELEM(KEY_AV0xd7);	
		ENUM_ELEM(KEY_AV0xd8);			ENUM_ELEM(KEY_AV0xd9);		
		ENUM_ELEM(KEY_AV0xda);			ENUM_ELEM(KEY_LeftBracket);		
		ENUM_ELEM(KEY_Backslash);		ENUM_ELEM(KEY_RightBracket);		
		ENUM_ELEM(KEY_SingleQuote);		ENUM_ELEM(KEY_AV0xdf);	
		ENUM_ELEM(KEY_AV0xe0);			ENUM_ELEM(KEY_AV0xe1);		
		ENUM_ELEM(KEY_AV0xe2);			ENUM_ELEM(KEY_AV0xe3);		
		ENUM_ELEM(KEY_AV0xe4);			ENUM_ELEM(KEY_AV0xe5);		
		ENUM_ELEM(KEY_AV0xe6);			ENUM_ELEM(KEY_AV0xe7);	
		ENUM_ELEM(KEY_AV0xe8);			ENUM_ELEM(KEY_AV0xe9);		
		ENUM_ELEM(KEY_AV0xea);			ENUM_ELEM(KEY_AV0xeb);		
		ENUM_ELEM(KEY_AV0xec);			ENUM_ELEM(KEY_AV0xed);		
		ENUM_ELEM(KEY_AV0xee);			ENUM_ELEM(KEY_AV0xef);	
		ENUM_ELEM(KEY_AV0xf0);			ENUM_ELEM(KEY_AV0xf1);		
		ENUM_ELEM(KEY_AV0xf2);			ENUM_ELEM(KEY_AV0xf3);		
		ENUM_ELEM(KEY_AV0xf4);			ENUM_ELEM(KEY_AV0xf5);		
		ENUM_ELEM(KEY_Attn);			ENUM_ELEM(KEY_CrSel);	
		ENUM_ELEM(KEY_ExSel);			ENUM_ELEM(KEY_ErEof);		
		ENUM_ELEM(KEY_Play);			ENUM_ELEM(KEY_Zoom);		
		ENUM_ELEM(KEY_NoName);			ENUM_ELEM(KEY_PA1);		
		ENUM_ELEM(KEY_OEMClear);		
	END_ENUM;

	return 0;
}


REGISTER_CLASS_CPP( FLevel, FResource, CLASS_Sterile )
{
	ADD_PROPERTY( bIsPlaying,	TYPE_Bool,		1,	PROP_Const,					nullptr );
	ADD_PROPERTY( bIsPause,		TYPE_Bool,		1,	PROP_Editable,				nullptr );
	ADD_PROPERTY( Original,		TYPE_Resource,	1,	PROP_Const|PROP_Editable,	FLevel::MetaClass );
	ADD_PROPERTY( GameSpeed,	TYPE_Float,		1,	PROP_Editable,				nullptr );
	ADD_PROPERTY( Soundtrack,	TYPE_Resource,	1,	PROP_Editable,				FMusic::MetaClass );
	ADD_PROPERTY( ScrollClamp,	TYPE_AABB,		1,	PROP_Editable,				nullptr );
	ADD_PROPERTY( Effect,		TYPE_Float,		10,	PROP_Editable,				nullptr );
	ADD_PROPERTY( AmbientLight,	TYPE_Color,		1,	PROP_Editable,				nullptr );

	return 0;
}


REGISTER_CLASS_CPP( FPuppetComponent, FExtraComponent, CLASS_None )
{
	BEGIN_ENUM(ELookDirection);
		ENUM_ELEM(LOOK_None);
		ENUM_ELEM(LOOK_Left);
		ENUM_ELEM(LOOK_Right);
		ENUM_ELEM(LOOK_Both);
	END_ENUM;

	BEGIN_ENUM(EPathType)
		ENUM_ELEM(PATH_None);
		ENUM_ELEM(PATH_Walk);
		ENUM_ELEM(PATH_Jump);
		ENUM_ELEM(PATH_Ladder);
		ENUM_ELEM(PATH_Teleport);
		ENUM_ELEM(PATH_Other);
	END_ENUM;

	ADD_PROPERTY( Health,			TYPE_Integer,	1,		PROP_Editable,	nullptr );
	ADD_PROPERTY( Clan,				TYPE_Integer,	1,		PROP_Editable,	nullptr );
	ADD_PROPERTY( MoveSpeed,		TYPE_Float,		1,		PROP_None,		nullptr );
	ADD_PROPERTY( JumpHeight,		TYPE_Float,		1,		PROP_None,		nullptr );
	ADD_PROPERTY( GravityScale,		TYPE_Float,		1,		PROP_None,		nullptr );
	ADD_PROPERTY( GoalReach,		TYPE_Byte,		1,		PROP_None,		_EPathType );
	ADD_PROPERTY( Goal,				TYPE_Vector,	1,		PROP_None,		nullptr );
	ADD_PROPERTY( GoalHint,			TYPE_Float,		1,		PROP_None,		nullptr );
	ADD_PROPERTY( iGoalNode,		TYPE_Integer,	1,		PROP_None,		nullptr );
	ADD_PROPERTY( iHoldenNode,		TYPE_Integer,	1,		PROP_None,		nullptr );
	ADD_PROPERTY( LookDirection,	TYPE_Byte,		1,		PROP_None,		_ELookDirection	);
	ADD_PROPERTY( LookRadius,		TYPE_Float,		1,		PROP_None,		nullptr );
	ADD_PROPERTY( LookPeriod,		TYPE_Float,		1,		PROP_None,		nullptr );

	DECLARE_METHOD( nativeSendOrder,			TYPE_Integer,	TYPE_String,	TYPE_Float,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeSuggestJumpSpeed,		TYPE_Float,		TYPE_Float,		TYPE_None,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeSuggestJumpHeight,	TYPE_Float,		TYPE_Float,		TYPE_None,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeMakeNoise,			TYPE_None,		TYPE_Float,		TYPE_None,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeIsVisible,			TYPE_Bool,		TYPE_Entity,	TYPE_None,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeCreatePathTo,			TYPE_Bool,		TYPE_Vector,	TYPE_None,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeCreateRandomPath,		TYPE_Bool,		TYPE_None,		TYPE_None,	TYPE_None,	TYPE_None );
	DECLARE_METHOD( nativeMoveToGoal,			TYPE_Bool,		TYPE_None,		TYPE_None,	TYPE_None,	TYPE_None );

	return 0;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/