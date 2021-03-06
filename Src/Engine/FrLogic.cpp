/*=============================================================================
    FrLogic.cpp: Logic circuit classes.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    FLogicComponent implementation.
-----------------------------------------------------------------------------*/

//
// Logic element constructor.
//
FLogicComponent::FLogicComponent()
	:	CRenderAddon( this ),
		bEnabled( true ),
		NumPlugs( 0 ),
		NumJacks( 0 )
{
	Color		= COLOR_PaleVioletRed;
	DrawOrder	= 0.02;
}


//
// Logic element destructor.
//
FLogicComponent::~FLogicComponent()
{
	// Remove this logic element from the level's list.
	com_remove(LogicElements);
}


//
// Initialize a logic element for the level.
//
void FLogicComponent::InitForEntity( FEntity* InEntity )
{
	FExtraComponent::InitForEntity( InEntity );
	com_add(RenderObjects);
	com_add(LogicElements);
}


//
// Remove all bad connectors that are referenced
// to the null logic elements.
//
void FLogicComponent::CleanBadConnectors()
{
	for( Integer iPlug=0; iPlug<MAX_LOGIC_PLUGS; iPlug++ )
	{
		for( Integer iConn = 0; iConn<Plugs[iPlug].Num(); )
			if( Plugs[iPlug][iConn].Target == nullptr )
				Plugs[iPlug].Remove( iConn );
			else
				iConn++;
	}
}


//
// Some property has been changed in the logic element.
//
void FLogicComponent::EditChange()
{
	FExtraComponent::EditChange();

	// Count the number of the plugs and jacks.
	NumPlugs	= 0;
	NumJacks	= 0;

	for( Integer i=0; i<MAX_LOGIC_JACKS; i++ )
		if( JacksName[i] )
			NumJacks++;

	for( Integer i=0; i<MAX_LOGIC_PLUGS; i++ )
		if( PlugsName[i] )
			NumPlugs++;
}


//
// Add a new connector from the iPlug-th plug to the logic element
// InTarget to the iJack-th jack.
//
void FLogicComponent::AddConnector( FLogicComponent* InTarget, Integer iPlug, Integer iJack )
{
	TLogicConnector Connector;

	Connector.Target	= InTarget;
	Connector.iJack		= iJack;

	// Prevent duplicates also.
	Plugs[iPlug].AddUnique( Connector );

	// Sometimes cleanup unused refs.
	CleanBadConnectors();
}


//
// Remove all connectors from the iPlug plug.
//
void FLogicComponent::RemoveConnectors( Integer iPlug )
{
	Plugs[iPlug].Empty();
}


//
// Logic element serialization.
//
void FLogicComponent::SerializeThis( CSerializer& S )
{
	FExtraComponent::SerializeThis( S );
	CRenderAddon::SerializeAddon( S );

	Serialize( S, bEnabled );

	for( Integer i=0; i<MAX_LOGIC_PLUGS; i++ )
		Serialize( S, PlugsName[i] );

	for( Integer i=0; i<MAX_LOGIC_JACKS; i++ )
		Serialize( S, JacksName[i] );

	Serialize( S, NumPlugs );
	Serialize( S, NumJacks );

	for( Integer i=0; i<MAX_LOGIC_PLUGS; i++ )
		Serialize( S, Plugs[i] );

	// Here should destroy unused refs.
	if( S.GetMode() == SM_Undefined )
		CleanBadConnectors();
}


//
// Import the logic element.
//
void FLogicComponent::Import( CImporterBase& Im )
{
	FExtraComponent::Import( Im );
	CRenderAddon::ImportAddon( Im );

	IMPORT_BOOL( bEnabled );
	IMPORT_INTEGER( NumPlugs );
	IMPORT_INTEGER( NumJacks );

	for( Integer i=0; i<MAX_LOGIC_PLUGS; i++ )
		PlugsName[i] = Im.ImportString( *String::Format( L"PlugsName[%d]", i ) );

	for( Integer i=0; i<MAX_LOGIC_JACKS; i++ )
		JacksName[i] = Im.ImportString( *String::Format( L"JacksName[%d]", i ) );

	// Import all connections.
	for( Integer i=0; i<MAX_LOGIC_PLUGS; i++ )
		if( PlugsName[i] )
		{
			Plugs[i].SetNum(Im.ImportInteger(*String::Format( L"Plugs[%i].Num" , i )));
			for( Integer j=0; j<Plugs[i].Num(); j++ )
			{
				TLogicConnector& Conn = Plugs[i][j];

				Conn.iJack	= Im.ImportInteger(*String::Format( L"Plugs[%i][%i].iJack" , i, j ));
				Conn.Target	= As<FLogicComponent>(Im.ImportObject(*String::Format( L"Plugs[%i][%i].Target" , i, j )));
			}
		}
}


//
// Export the logic component.
//
void FLogicComponent::Export( CExporterBase& Ex )
{
	FExtraComponent::Export( Ex );
	CRenderAddon::ExportAddon( Ex );

	EXPORT_BOOL( bEnabled );
	EXPORT_INTEGER( NumPlugs );
	EXPORT_INTEGER( NumJacks );

	for( Integer i=0; i<MAX_LOGIC_PLUGS; i++ )
		if( PlugsName[i] )
			Ex.ExportString( *String::Format( L"PlugsName[%d]", i ), PlugsName[i] );

	for( Integer i=0; i<MAX_LOGIC_JACKS; i++ )
		if( JacksName[i] )
			Ex.ExportString( *String::Format( L"JacksName[%d]", i ), JacksName[i] );

	// This way sucks! To store connectors and
	// plugs! But works well.
	for( Integer i=0; i<MAX_LOGIC_PLUGS; i++ )
		if( PlugsName[i] )
		{
			Ex.ExportInteger( *String::Format( L"Plugs[%i].Num" , i ), Plugs[i].Num() );
			for( Integer j=0; j<Plugs[i].Num(); j++ )
			{
				TLogicConnector& Conn = Plugs[i][j];

				Ex.ExportInteger( *String::Format( L"Plugs[%i][%i].iJack" , i, j ), Conn.iJack );
				Ex.ExportObject( *String::Format( L"Plugs[%i][%i].Target" , i, j ), Conn.Target );
			}
		}
}


//
// Return the world location of the iPlug (right-side) socket.
//
TVector FLogicComponent::GetPlugPos( Integer iPlug )
{
	iPlug	= Clamp( iPlug, 0, NumPlugs );

	TVector V;

	V.X	= Base->Location.X + Base->Size.X * 0.5f;
	V.Y	= Base->Location.Y + Base->Size.Y * 0.5f - (iPlug+1)*Base->Size.Y/(NumPlugs+1);

	return V;
}


//
// Return the world location of the iJack (left-side) socket.
//
TVector FLogicComponent::GetJackPos( Integer iJack )
{
	iJack	= Clamp( iJack, 0, NumJacks );

	TVector V;

	V.X	= Base->Location.X - Base->Size.X * 0.5f;
	V.Y	= Base->Location.Y + Base->Size.Y * 0.5f - (iJack+1)*Base->Size.Y/(NumJacks+1);

	return V;
}


//
// Render the logic element.
//
void FLogicComponent::Render( CCanvas* Canvas )
{
	// Is visible or not?
	if( (Level->RndFlags & RND_Logic) == 0 )
			return;

	TRect Rect = Base->GetAABB();

	// Pick a colors.
	TColor WireColor = bEnabled ? Color : COLOR_LightSlateGray;
	if( Base->bSelected )
		WireColor *= 1.5f;

	// Draw a gray pad.
	TRenderRect Pad;
	Pad.Flags		= POLY_Unlit | POLY_FlatShade | POLY_Ghost;
	Pad.Color		= bEnabled ? TColor( 0x20, 0x20, 0x20, 0xff ) : TColor( 0x40, 0x40, 0x40, 0xff );
	Pad.Bounds		= Rect;
	Pad.Bitmap		= nullptr;
	Pad.Rotation	= 0;
	Canvas->DrawRect( Pad );

	// Wire bounds.
	Canvas->DrawLineRect( Base->Location, Base->Size, 0, WireColor, false );

	// Draw the plugs sockets.
	for( Integer i=0; i<NumPlugs; i++ )
		Canvas->DrawPoint( GetPlugPos(i), 7.f, WireColor );

	// Draw the jacks sockets.
	for( Integer i=0; i<NumJacks; i++ )
		Canvas->DrawPoint( GetJackPos(i), 7.f, WireColor );

	// Draw connectors.
	for( Integer iPlug=0; iPlug<NumPlugs; iPlug++ )
		for( Integer iConn=0; iConn<Plugs[iPlug].Num(); iConn++ )
		{
			TLogicConnector& Conn = Plugs[iPlug][iConn];

			if( Conn.Target )
				Canvas->DrawSmoothLine
							( 
								GetPlugPos(iPlug), 
								Conn.Target->GetJackPos(Conn.iJack), 
								WireColor, 
								false 
							);
		}
}


//
// Induce the signal and send it from the appropriate plug.
//
void FLogicComponent::nativeInduceSignal( CFrame& Frame )
{
	// Pop parameters.
	FEntity*	Creator		= POP_ENTITY;
	String		PlugName	= POP_STRING;

	// Don't induce if disabled.
	if( !bEnabled )
		return;
	   
	// Find the plug by it name.
	Integer iPlug = -1;
	for( Integer i=0; i<NumPlugs; i++ )
		if( PlugsName[i] == PlugName )
		{
			iPlug	= i;
			break;
		}

	if( iPlug == -1 )
	{
		log( L"Logic: Plug '%s' not found in '%s'", *PlugName, *Entity->GetFullName() );
		return;
	}

	// Send the signal over the wires.
	for( Integer i=0; i<Plugs[iPlug].Num(); i++ )
	{
		TLogicConnector& Conn = Plugs[iPlug][i];

		if( Conn.Target )
			Conn.Target->Entity->CallEvent
							( 
								EVENT_OnReceiveSignal,
								Creator,
								Entity,
								Conn.Target->JacksName[Conn.iJack]
							);
	}
}


/*-----------------------------------------------------------------------------
    TLogicConnector implementation.
-----------------------------------------------------------------------------*/

//
// Connector serialization.
//
void Serialize( CSerializer& S, TLogicConnector& V )
{
	Serialize( S, V.Target );
	Serialize( S, V.iJack );
}


//
// Logic connector comparison.
//
Bool TLogicConnector::operator==( const TLogicConnector& LC ) const
{
	return (Target == LC.Target) && (iJack == LC.iJack);
}


/*-----------------------------------------------------------------------------
    Registration.
-----------------------------------------------------------------------------*/

REGISTER_CLASS_CPP( FLogicComponent, FExtraComponent, CLASS_None )
{
	ADD_PROPERTY( bEnabled,		TYPE_Bool,		1,					PROP_Editable,	nullptr );
	ADD_PROPERTY( JacksName,	TYPE_String,	MAX_LOGIC_JACKS,	PROP_None,		nullptr );	
	ADD_PROPERTY( PlugsName,	TYPE_String,	MAX_LOGIC_PLUGS,	PROP_None,		nullptr );

	DECLARE_METHOD( nativeInduceSignal,	TYPE_None,	TYPE_Entity,	TYPE_String,	TYPE_None, TYPE_None );

	return 0;
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/