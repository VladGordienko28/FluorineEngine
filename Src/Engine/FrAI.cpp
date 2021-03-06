/*=============================================================================
    FrAI.cpp: AI implementation.
    Copyright Oct.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FPuppetComponent implementation.
-----------------------------------------------------------------------------*/

//
// Puppet constructor.
//
FPuppetComponent::FPuppetComponent()
	:	FExtraComponent(),
		CTickAddon( this )
{
	// This variables are vary for all puppets
	// in scene, but here I set some initial 
	// values, that the most often used.
	Health				= 100;
	Clan				= 0;
	MoveSpeed			= 5.f;
	JumpHeight			= 8.f;
	GravityScale		= 10.f;
	Goal				= 
	GoalStart			= TVector( 0.f, 0.f );
	GoalReach			= PATH_None;
	GoalHint			= 0.f;
	iGoalNode			=
	iHoldenNode			= -1;
	Body				= nullptr;
	LookDirection		= LOOK_None;
	LookPeriod			= 0.f;
	LookRadius			= 16.f;
	LookCounter			= 0.f;
	MemZero( LookList, sizeof(LookList) );
}


//
// Puppet destructor.
//
FPuppetComponent::~FPuppetComponent()
{
	com_remove(Puppets);
}


//
// Initialize puppet for entity.
//
void FPuppetComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity(InEntity);

	com_add(TickObjects);
	com_add(Puppets);
}


//
// Serialize the puppet.
//
void FPuppetComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );

	// Serialize only savable/loadable variables.
	Serialize( S, Health );
	Serialize( S, Clan );
	Serialize( S, MoveSpeed );
	Serialize( S, JumpHeight );
	Serialize( S, GravityScale );
	SerializeEnum( S, LookDirection );
	Serialize( S, LookPeriod );
	Serialize( S, LookRadius );

	// Also serialize list of Watched for GC.
	if( S.GetMode() == SM_Undefined )
		for( Integer i=0; i<MAX_WATCHED; i++ )
			Serialize( S, LookList[i] );
}


//
// Export the puppet.
//
void FPuppetComponent::Export( CExporterBase& Ex )
{
	FExtraComponent::Export( Ex );

	EXPORT_INTEGER(Health);
	EXPORT_INTEGER(Clan);
	EXPORT_FLOAT(MoveSpeed);
	EXPORT_FLOAT(JumpHeight);
	EXPORT_FLOAT(GravityScale);
	EXPORT_BYTE(LookDirection);
	EXPORT_FLOAT(LookPeriod);
	EXPORT_FLOAT(LookRadius);
}


//
// Import the puppet.
//
void FPuppetComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );

	IMPORT_INTEGER(Health);
	IMPORT_INTEGER(Clan);
	IMPORT_FLOAT(MoveSpeed);
	IMPORT_FLOAT(JumpHeight);
	IMPORT_FLOAT(GravityScale);
	IMPORT_BYTE(LookDirection);
	IMPORT_FLOAT(LookPeriod);
	IMPORT_FLOAT(LookRadius);
}


//
// Prepare 'warriors' for the battle!
//
void FPuppetComponent::BeginPlay()
{
	FExtraComponent::BeginPlay();

	// Slightly modify a look time counter, to avoid all puppets 
	// watching at the same time.
	LookCounter	= LookPeriod * RandomF();

	// Get arcade body.
	Body	= As<FArcadeBodyComponent>(Base);
	if( !Body )
	{
		// Suicide, if no arcade base.
		log( L"Puppet '%s' has no arcade body base!", *Entity->GetFullName() );
		Level->DestroyEntity( Entity );
	}

	// Reset navi variables.
	iGoalNode	=
	iHoldenNode	= -1;
}


/*-----------------------------------------------------------------------------
	Puppet AI functions.
-----------------------------------------------------------------------------*/

//
// AI tick.
//
void FPuppetComponent::Tick( Float Delta )
{
	if( !Body )
		return;

	// Did we want to watch puppets?
	if( LookPeriod > 0.f && LookRadius > 0.f && LookDirection != LOOK_None )
	{
		// Update timer.
		LookCounter += Delta;

		if( LookCounter > LookPeriod )
		{
			LookCounter	= 0.f;
			LookAtPuppets();
		}
	}
}


//
// Look to the puppets.
//
void FPuppetComponent::LookAtPuppets()
{
	// Prepare.
	Integer	iLookee = 0;
	Float	Dist2	= LookRadius*LookRadius; 
	MemZero( LookList, sizeof(LookList) );

	for( Integer i=0; i<Level->Puppets.Num(); i++ )
	{
		FPuppetComponent*	Other	= Level->Puppets[i];
		if( Other == this )
			continue;

		// Look direction testing.
		TVector	Dir	= Other->Base->Location - Base->Location;
		if( Dir.X>0.f && LookDirection==LOOK_Left )		continue;
		if( Dir.X<0.f && LookDirection==LOOK_Right )	continue;

		// Distance and LOS testing.
		TVector UnusedHit, UnusedNormal;
		if( Dir.SizeSquared() > Dist2 ) continue;
		if	( Level->TestLineGeom
				(
					Base->Location + TVector( 0.f, Base->Size.Y*0.5f ),
					Other->Base->Location + TVector( 0.f, Other->Base->Size.Y*0.5f ),
					true,
					UnusedHit, UnusedNormal
				)
			)
				continue;

		// Yes! Other is visible for this.
		if( iLookee >= MAX_WATCHED )
		{
			LookList[iLookee] = Other;
			iLookee++;
		}
		else
			break;

		Entity->CallEvent( EVENT_OnLookAt, Other->Entity );
	}
}


/*-----------------------------------------------------------------------------
	Puppet movement functions.
-----------------------------------------------------------------------------*/

//
// Moves puppet to the goal, just set velocity to reach it. Returns true, if
// puppet hits the goal. If goal is unreachabled for now returns false. 
//
Bool FPuppetComponent::MoveToGoal()
{
	if( !Body )
	{
		GOutput->ScriptErrorf( L"Puppet used without appropriate arcade body in '%s'", *Entity->Script->GetName() );
		return false;
	}

	// Did we have something to move?
	if( GoalReach == PATH_None )
		return false;

	if( GoalReach == PATH_Walk )
	{
		//
		// Walking.
		//
		if( Goal.X > GoalStart.X )
		{
			// Walk rightward.
			if( Body->Location.X < Goal.X+Body->Size.X*0.25f )
			{
				Body->Velocity.X	= MoveSpeed;
				return false;
			}
			else
			{
				Body->Velocity.X	= 0.f;
				return true;
			}
		}
		else
		{
			// Walk leftward.
			if( Body->Location.X > Goal.X-Body->Size.X*0.25f )
			{
				Body->Velocity.X	= -MoveSpeed;
				return false;
			}
			else
			{
				Body->Velocity.X	= 0.f;
				return true;
			}
		}
	}
	else if( GoalReach == PATH_Jump )
	{
		//
		// Jumping.
		//
		Bool bXOk	= false;
		if( Goal.X > GoalStart.X )
		{
			// X move rightward.
			Body->Velocity.X	= MoveSpeed;
			bXOk				= Body->Location.X > Goal.X+Body->Size.X*0.25f;
		}
		else
		{
			// X move leftward.
			Body->Velocity.X	= -MoveSpeed;	
			bXOk				= Body->Location.X < Goal.X-Body->Size.X*0.25f;
		}

		if( Body->Floor && !bXOk )
			Body->Velocity.Y	= GoalHint * 1.f;

		if( bXOk )
			Body->Velocity.X	= 0.f;

		return bXOk && Abs(Goal.Y - Body->Location.Y)<Body->Size.Y*0.5f;
	}
	else
	{
		//
		// Miscellaneous, handled by script, here we detect only ovelap.
		//
		return Body->GetAABB().IsInside(Goal);
	}
}


//
// Returns true, if figure can make a jump from the 'From' to 'To' spot.
// SuggestedSpeed - Recommended initial speed for jumping.
//
Bool FPuppetComponent::CanJumpTo( TVector From, TVector To, Float& SuggestedSpeed )
{
	Float Speed	= SpeedForJump( From, To, MoveSpeed, GravityScale );

	if( Speed < SuggestJumpSpeed( JumpHeight, GravityScale ) )
	{
		// Possible.
		SuggestedSpeed	= Speed;
		return true;
	}
	else
	{
		// Impossible.
		SuggestedSpeed	= 0.f;
		return false;
	}
}


//
// Computes a maximum reached jump height with an initial velocity JumpSpeed
// and Gravity.
//
Float FPuppetComponent::SuggestJumpHeight( Float JumpSpeed, Float Gravity )
{
	Float Time	= JumpSpeed / Gravity;
	return JumpSpeed*Time - 0.5f*Time*Time*Gravity;
}


//
// Computes a jump speed to reach Height height with
// an given Gravity.
//
Float FPuppetComponent::SuggestJumpSpeed( Float DesiredHeight, Float Gravity )
{
	return Sqrt(2.f * Gravity * DesiredHeight);
}


//
// Returns the vertical speed to jump from the 'From' location to 
// the 'To' location.
//
Float FPuppetComponent::SpeedForJump( TVector From, TVector To, Float XSpeed, Float Gravity )
{
	Float XTime	= Abs(From.X - To.X) / XSpeed;
	Float Speed = (To.Y + 0.5f*XTime*XTime*Gravity - From.Y) / XTime;

	// Slightly modify speed to make unpredictable results.
	return Max( 0.f, Speed ) * 1.1f;
}


/*-----------------------------------------------------------------------------
    In-Script AI functions.
-----------------------------------------------------------------------------*/

//
// Emit a noise, from the puppet.
//
void FPuppetComponent::nativeMakeNoise( CFrame& Frame )
{
	Float Radius2 = Sqr(POP_FLOAT);
	for( Integer i=0; i<Level->Puppets.Num(); i++ )
	{
		FPuppetComponent* Other = Level->Puppets[i];

		if	( 
				(Base->Location-Other->Base->Location).SizeSquared() < Radius2 && 
				Other != this 
			)
		{
			Other->Entity->CallEvent( EVENT_OnHearNoise, this->Entity );
		}
	}
}


//
// Suggest a jump height using initial jump speed.
//
void FPuppetComponent::nativeSuggestJumpHeight( CFrame& Frame )
{
	Float Speed = POP_FLOAT;
	*POPA_FLOAT	= FPuppetComponent::SuggestJumpHeight
	(
		Speed,
		GravityScale
	);
}


//
// Suggest a jump speed to reach height.
//
void FPuppetComponent::nativeSuggestJumpSpeed( CFrame& Frame )
{
	Float Height = POP_FLOAT;
	*POPA_FLOAT	= FPuppetComponent::SuggestJumpSpeed
	(
		Height,
		GravityScale
	);
}


//
// Send an order to puppet's teammates in radius.
//
void FPuppetComponent::nativeSendOrder( CFrame& Frame )
{
	Integer	NumRecipients	= 0;
	String	Order			= POP_STRING;
	Float	Radius2			= Sqr(POP_FLOAT);

	for( Integer p=0; p<Level->Puppets.Num(); p++ )
	{
		FPuppetComponent* Testee = Level->Puppets[p];

		if	(
				Testee->Clan == Clan &&
				(Base->Location-Testee->Base->Location).SizeSquared() < Radius2 &&
				Testee != this
			)
		{
			Testee->Entity->CallEvent( EVENT_OnGetOrder, this->Entity, Order );
			NumRecipients++;
		}
	}

	*POPA_INTEGER	= NumRecipients;
}


//
// Return true, if other is visible.
//
void FPuppetComponent::nativeIsVisible( CFrame& Frame )
{
	FEntity* Other	= POP_ENTITY;
	if( !Other )
	{
		*POPA_BOOL	= false;
		return;
	}

	for( Integer i=0; i<MAX_WATCHED && LookList[i]; i++ )
		if( LookList[i]->Entity == Other )
		{
			*POPA_BOOL	= true;
			return;
		}

	*POPA_BOOL	= false;
}


//
// Move puppet to the goal.
//
void FPuppetComponent::nativeMoveToGoal( CFrame& Frame )
{
	*POPA_BOOL	= MoveToGoal();
}


//
// Tries to create a random path.
//
void FPuppetComponent::nativeCreateRandomPath( CFrame& Frame )
{
	if( Level->Navigator )
	{
		*POPA_BOOL	= Level->Navigator->MakeRandomPath( this );
	}
	else
	{
		log( L"AI: Level has no navigator" );
		*POPA_BOOL	= false;
	}
}


//
// Tries to create a path to specified location, returns true
// if path was successfully created, otherwise returns false.
//
void FPuppetComponent::nativeCreatePathTo( CFrame& Frame )
{
	TVector	Dest	= POP_VECTOR;
	if( Level->Navigator )
	{
		*POPA_BOOL	= Level->Navigator->MakePathTo( this, Dest );
	}
	else
	{
		log( L"AI: Level has no navigator" );
		*POPA_BOOL	= false;
	}
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/