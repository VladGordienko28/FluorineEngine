/*=============================================================================
    FrComponent.h: Basic components classes.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    FComponent.
-----------------------------------------------------------------------------*/

//
// An abstract component class.
//
class FComponent: public FObject
{
REGISTER_CLASS_H(FComponent);
public:
	// Variables.
	FScript*		Script;
	FEntity*		Entity;
	FLevel*			Level;

	// FComponent interface.
	FComponent();
	~FComponent();
	virtual void InitForEntity( FEntity* InEntity );
	virtual void InitForScript( FScript* InScript );
	virtual Float GetLayer() const;
	virtual void BeginPlay();
	virtual void EndPlay();

	// FObject interface.
	void SerializeThis( CSerializer& S );
	void EditChange();
	void PostLoad();
	void Import( CImporterBase& Im );
	void Export( CExporterBase& Ex );
};


/*-----------------------------------------------------------------------------
    FExtraComponent.
-----------------------------------------------------------------------------*/

//
// An addition component for entity.
//
class FExtraComponent: public FComponent
{
REGISTER_CLASS_H(FExtraComponent)
public:
	// Variables.
	Float				DrawOrder;
	FBaseComponent*		Base;

	// FExtraComponent interface.
	FExtraComponent();

	// FComponent interface.
	void InitForEntity( FEntity* InEntity );
	void InitForScript( FScript* InScript );
	Float GetLayer() const;

	// FObject interface.
	void PostLoad();
};


/*-----------------------------------------------------------------------------
    FBaseComponent.
-----------------------------------------------------------------------------*/

//
// A base entity component.
//
class FBaseComponent: public FComponent
{
REGISTER_CLASS_H(FBaseComponent)
public:
	// Flags.
	Bool			bDestroyed;
	Bool			bFixedAngle;
	Bool			bFixedSize;
	Bool			bHashable;
	
	// Editor flags.
	Bool			bSelected;
	Bool			bFrozen;

	// World info.
	TVector			Location;
	TAngle			Rotation;
	TVector			Size;
	Float			Layer;

	// FBaseComponent interface.
	FBaseComponent();
	virtual TRect GetAABB();

	// FComponent interface.
	void InitForEntity( FEntity* InEntity );
	void InitForScript( FScript* InScript );
	Float GetLayer() const;
	void BeginPlay();
	void EndPlay();

	// FObject interface.
	void SerializeThis( CSerializer& S );
	void Import( CImporterBase& Im );
	void Export( CExporterBase& Ex );
	void EditChange();

	// Accessors.
	inline TCoords ToWorld() const
	{
		return TCoords( Location, Rotation ).Transpose();
	}
	inline TCoords ToLocal() const
	{
		return TCoords( Location, Rotation );
	}
	inline Bool IsHashed() const
	{
		return bHashed;
	}


private:
	// Collision hash internal.
	friend CCollisionHash;
	Bool		bHashed;
	DWord		HashMark;
	TRect		HashAABB;

	// Natives.
	void nativeSetLocation( CFrame& Frame );
	void nativeSetSize( CFrame& Frame );
	void nativeSetRotation( CFrame& Frame );
	void nativeMove( CFrame& Frame );
	void nativePlayAmbient( CFrame& Frame );
	void nativeStopAmbient( CFrame& Frame );
	void nativeIsBasedOn( CFrame& Frame );
	void nativeGetAABB( CFrame& Frame );
};


/*-----------------------------------------------------------------------------
    Addon classes.
-----------------------------------------------------------------------------*/

//
// CTickAddon - provide component tick functions.
//
class CTickAddon
{
public:
	// Tick methods.
	virtual void PreTick( Float Delta ){}
	virtual void Tick( Float Delta ){}
	virtual void TickNonPlay( Float Delta ){}

	// CTickAddon interface.
	CTickAddon( FComponent* InOwner )
		:	AddonOwner( InOwner )
	{}
	virtual ~CTickAddon();
private:
	// Internal.
	FComponent*			AddonOwner;
};


//
// CRenderAddon - provide component render functions.
//
class CRenderAddon
{
public:
	// Variables.
	Bool			bHidden;
	TColor			Color;

	// Render functions.
	virtual void Render( CCanvas* Canvas ){}

	// CRenderAddon interface.
	CRenderAddon( FComponent* InOwner )
		:	AddonOwner( InOwner ),
			bHidden( false ),
			Color( COLOR_White )
	{}
	virtual ~CRenderAddon();
	virtual void SerializeAddon( CSerializer& S )
	{
		Serialize( S, bHidden );
		Serialize( S, Color );
	}
	virtual void ExportAddon( CExporterBase& Ex )
	{
		EXPORT_BOOL(bHidden);
		EXPORT_COLOR(Color);
	}
	virtual void ImportAddon( CImporterBase& Im )
	{
		IMPORT_BOOL(bHidden);
		IMPORT_COLOR(Color);
	}

	// Accessors.
	inline Float GetLayer() const
	{
		return AddonOwner->GetLayer();
	}
private:
	// Internal.
	FComponent*		AddonOwner;
};


//
// CBitmapRenderAddon.
//
class CBitmapRenderAddon: public CRenderAddon
{
public:
	// Variables.
	Bool			bUnlit;
	Bool			bFlipH;
	Bool			bFlipV;
	FBitmap*		Bitmap;

	// CBitmapRenderAddon interface.
	CBitmapRenderAddon( FComponent* InOwner )
		:	CRenderAddon( InOwner ),
			bUnlit( true ),
			bFlipH( false ),
			bFlipV( false ),
			Bitmap( nullptr )
	{}
	void SerializeAddon( CSerializer& S )
	{
		CRenderAddon::SerializeAddon( S );
		Serialize( S, bUnlit );
		Serialize( S, bFlipH );
		Serialize( S, bFlipV );
		Serialize( S, *(FObject**)&Bitmap );
	}
	void ExportAddon( CExporterBase& Ex )
	{
		CRenderAddon::ExportAddon( Ex );
		EXPORT_BOOL(bUnlit);
		EXPORT_BOOL(bFlipH);
		EXPORT_BOOL(bFlipV);
		EXPORT_OBJECT(Bitmap);
	}
	void ImportAddon( CImporterBase& Im )
	{
		CRenderAddon::ImportAddon( Im );
		IMPORT_BOOL(bUnlit);
		IMPORT_BOOL(bFlipH);
		IMPORT_BOOL(bFlipV);
		IMPORT_OBJECT(Bitmap);
	}
};


//
// CAnimationRenderAddon - allow object render with animation.
//
class CAnimationRenderAddon: public CRenderAddon
{
public:
	// Variables.
	Bool			bUnlit;
	Bool			bFlipH;
	Bool			bFlipV;
	FAnimation*		Animation;

	// CAnimationRenderAddon interface.
	CAnimationRenderAddon( FComponent* InOwner )
		:	CRenderAddon( InOwner ),
			bUnlit( true ),
			bFlipH( false ),
			bFlipV( false ),
			Animation( nullptr )
	{}
	void SerializeAddon( CSerializer& S )
	{
		CRenderAddon::SerializeAddon( S );
		Serialize( S, bUnlit );
		Serialize( S, bFlipH );
		Serialize( S, bFlipV );
		Serialize( S, *(FObject**)&Animation );
	}
	void ExportAddon( CExporterBase& Ex )
	{
		CRenderAddon::ExportAddon( Ex );
		EXPORT_BOOL(bUnlit);
		EXPORT_BOOL(bFlipH);
		EXPORT_BOOL(bFlipV);
		EXPORT_OBJECT(Animation);
	}
	void ImportAddon( CImporterBase& Im )
	{
		CRenderAddon::ImportAddon( Im );
		IMPORT_BOOL(bUnlit);
		IMPORT_BOOL(bFlipH);
		IMPORT_BOOL(bFlipV);
		IMPORT_OBJECT(Animation);
	}
};


/*-----------------------------------------------------------------------------
    Component macro.
-----------------------------------------------------------------------------*/

// Add component to list of components.
#define com_add( arr )	{ Level->arr.Push(this); }

// Remove component from the level's list.
#define com_remove( arr ){ if(Level){ Integer i=Level->arr.FindItem(this); if(i != -1) Level->arr.Remove(i); }}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/