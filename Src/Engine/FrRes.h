/*=============================================================================
    FrRes.h: FResource base class.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    FResource.
-----------------------------------------------------------------------------*/

//
// An abstract resource.
//
class FResource: public FObject
{
REGISTER_CLASS_H(FResource);
public:
	// Variables.
	String		FileName;
	String		Group;

	// FResource interface.
	FResource();

	// FObject interface.
	void SerializeThis( CSerializer& S );
	void PostLoad();
	void Import( CImporterBase& Im );
	void Export( CExporterBase& Ex );
};


/*-----------------------------------------------------------------------------
    FBlockResource.
-----------------------------------------------------------------------------*/

//
// A large data resource, needed to
// be dynamic loaded/unloaded during playing.
//
class FBlockResource: public FResource		
{
REGISTER_CLASS_H(FBlockResource);
public:
	// Friends.
	friend CBlockManager;

	// FBlockResource interface.
	FBlockResource();
	~FBlockResource();
	virtual Bool AllocateBlock( Integer NumBytes, DWord ExtraFlags = BLOCK_None );
	virtual Bool ReleaseBlock();
	virtual DWord GetBlockSize();
	virtual void* GetData();

	// FObject interface.
	void SerializeThis( CSerializer& S );

	// Accessors.
	inline Integer GetBlockId() const
	{
		return iBlock;
	}
	inline Bool IsValidBlock() const
	{
		return iBlock != -1;
	}

private:
	// Resource internal.
	Integer		iBlock;
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/