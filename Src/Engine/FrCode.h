/*=============================================================================
    FrCode.h: Script execution support structures.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    Code execution structures.
-----------------------------------------------------------------------------*/

//
// A virtual machine register.
//
class TRegister
{
public:
	// Constants.
	enum{ NUM_REGS = 24 };

	// Variables.
	struct
	{
		union 
		{
			Byte	Value[16];
			void*	Addr;
		};
		String	StrValue;
	};

	// Constructor.
	TRegister()
		:	Addr( nullptr ),
			StrValue()
	{}
};


//
// An information about foreach loop.
//
class TForeach
{
public:
	// Variables.
	TArray<FEntity*>	Collection;
	Integer				i;

	// Constructor.
	TForeach()
		:	Collection(),
			i( 0 )
	{}
};


/*-----------------------------------------------------------------------------
    TFrame.
-----------------------------------------------------------------------------*/

//
// Constants.
//
#define MAX_RECURSION_DEPTH		32
#define MAX_ITERATIONS			1000000


//
// A code execution local frame.
//
class CFrame
{
public:
	// Variables.
	FEntity*		This;
	FScript*		Script;
	CBytecode*		Bytecode;
	TRegister		Regs[TRegister::NUM_REGS];	

	// Friends.
	friend CEntityThread;
	friend FEntity;

	// Shared stack memory.
	static CMemPool GLocalsMem;

	// CFrame interface.
	CFrame( FEntity* InThis, CFunction* InFunction, Integer InDepth = 1, CFrame* InPrevFrame = nullptr );
	CFrame( FEntity* InThis, CThreadCode* InThread );
	~CFrame();
	void ScriptError( Char* Fmt, ... );

private:
	// Frame internal.
	CFrame*			PrevFrame;
	Integer			Depth;	
	Byte*			Code;
	Byte*			Locals;
	TForeach		Foreach;

	// Opcodes execution.
	void ProcessCode( TRegister* Result );
	void ExecuteNative( FEntity* Context, EOpCode Code );

	// Misc.
	String StackTrace();

public:
	// Constants readers.
	inline Byte ReadByte();
	inline Bool ReadBool();
	inline Integer ReadInteger();
	inline Float ReadFloat();
	inline TAngle ReadAngle();
	inline TColor ReadColor();
	inline String ReadString();
	inline TVector ReadVector();
	inline TRect ReadAABB();
	inline FResource* ReadResource();
	inline FEntity* ReadEntity();
	inline Word ReadWord();
	inline FScript* ReadScript();
	inline EPropType ReadPropType();
};


/*-----------------------------------------------------------------------------
    CEntityThread.
-----------------------------------------------------------------------------*/

//
// A current tread status.
//
enum EThreadStatus
{
	THR_Run,		// Thread executed well.
	THR_Stopped,	// Thread are stopped.
	THR_Sleep,		// Thread are sleep for now.
	THR_Wait		// Thread are waiting.
};


//
// An entity thread to process scenario.
//
class CEntityThread
{
public:
	// Variables.
	EThreadStatus		Status;
	FEntity*			Entity;
	CFrame				Frame;
	Integer				LabelId;
	union
	{
		Float			SleepTime;	// THR_Sleep.
		Byte*			WaitExpr;	// THR_Wait.
	};

	// CEntityThread interface.
	CEntityThread( FEntity* InEntity, CThreadCode* InThread );
	~CEntityThread();
	void Tick( Float Delta );
};


/*-----------------------------------------------------------------------------
    CFrame implementation.
-----------------------------------------------------------------------------*/

inline Byte CFrame::ReadByte()
{
	Byte R = *Code;
	Code += sizeof(Byte);
	return R;
}

inline Bool CFrame::ReadBool()
{
	Bool R = *(Bool*)Code;
	Code += sizeof(Bool);
	return R;
}

inline Integer CFrame::ReadInteger()
{
	Integer R = *(Integer*)Code;
	Code += sizeof(Integer);
	return R;
}

inline Float CFrame::ReadFloat()
{
	Float R = *(Float*)Code;
	Code += sizeof(Float);
	return R;
}

inline TAngle CFrame::ReadAngle()
{
	TAngle R = *(TAngle*)Code;
	Code += sizeof(TAngle);
	return R;
}

inline TColor CFrame::ReadColor()
{
	TColor R = *(TColor*)Code;
	Code += sizeof(TColor);
	return R;
}

inline String CFrame::ReadString()
{
	Integer Len = Clamp( ReadInteger(), 0, 127 );
	Char Str[128];
	MemCopy( Str, Code, Len*sizeof(Char) );
	Code += Len * sizeof(Char);
	Str[Len] = '\0';
	return Str;
}

inline TVector CFrame::ReadVector()
{
	TVector R = *(TVector*)Code;
	Code += sizeof(TVector);
	return R;
}

inline TRect CFrame::ReadAABB()
{
	TRect R = *(TRect*)Code;
	Code += sizeof(TRect);
	return R;
}

inline Word CFrame::ReadWord()
{
	Word R = *(Word*)Code;
	Code += sizeof(Word);
	return R;
}

inline FResource* CFrame::ReadResource()
{
	Byte iRes = ReadByte();
	return iRes != 0xff ? Script->ResTable[iRes] : nullptr;
}

inline FEntity* CFrame::ReadEntity()
{
	Integer iEntity = ReadInteger();
	return iEntity != -1 ? (FEntity*)GObjectDatabase->GObjects[iEntity] : nullptr;	
}

inline FScript* CFrame::ReadScript()
{
	FObject* Obj = GObjectDatabase->GObjects[ReadInteger()];
	assert(Obj->IsA(FScript::MetaClass));
	return (FScript*)Obj;
}

inline EPropType CFrame::ReadPropType()
{
	EPropType R = (EPropType)*Code;
	Code += sizeof(Byte);
	return R;
}


/*-----------------------------------------------------------------------------
    Stack macro.
-----------------------------------------------------------------------------*/

// Parameters grabbers.
#define POP_BYTE			(*(Byte*)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_BOOL			(*(Bool*)(Frame.Regs[Frame.ReadByte()].Value))		
#define POP_INTEGER			(*(Integer*)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_FLOAT			(*(Float*)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_ANGLE			(*(TAngle*)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_COLOR			(*(TColor*)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_STRING			(Frame.Regs[Frame.ReadByte()].StrValue)
#define POP_VECTOR			(*(TVector*)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_AABB			(*(TRect*)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_RESOURCE		(*(FResource**)(Frame.Regs[Frame.ReadByte()].Value))
#define POP_ENTITY			(*(FEntity**)(Frame.Regs[Frame.ReadByte()].Value))

#define POPA_BYTE			((Byte*)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_BOOL			((Bool*)(Frame.Regs[Frame.ReadByte()].Value))		
#define POPA_INTEGER		((Integer*)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_FLOAT			((Float*)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_ANGLE			((TAngle*)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_COLOR			((TColor*)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_STRING			(&Frame.Regs[Frame.ReadByte()].StrValue)
#define POPA_VECTOR			((TVector*)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_AABB			((TRect*)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_RESOURCE		((FResource**)(Frame.Regs[Frame.ReadByte()].Value))
#define POPA_ENTITY			((FEntity**)(Frame.Regs[Frame.ReadByte()].Value))


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/