/*=============================================================================
    FrEntity.h: Entity class.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    FEntity.
-----------------------------------------------------------------------------*/

//
// An level entity.
//
class FEntity: public FObject
{
REGISTER_CLASS_H(FEntity);
public:
	// General info.
	FLevel*						Level;
	FScript*					Script;
	CInstanceBuffer*			InstanceBuffer;
	CEntityThread*				Thread;

	// Components.
	FBaseComponent*				Base;
	TArray<FExtraComponent*>	Components;

	// FEntity interface.
	FEntity();
	~FEntity();
	void Init( FScript* InScript, FLevel* InLevel );
	void BeginPlay();
	void EndPlay();

	// FObject interface.
	void SerializeThis( CSerializer& S );
	void PostLoad();
	void Import( CImporterBase& Im );
	void Export( CExporterBase& Ex );

	// FluScript functions.
	void CallEvent
				( 
					EEventName EventName, 
					VARIANT_PARM(A1), 
					VARIANT_PARM(A2), 
					VARIANT_PARM(A3), 
					VARIANT_PARM(A4) 
				);
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/