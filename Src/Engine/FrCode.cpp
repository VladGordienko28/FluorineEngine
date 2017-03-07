/*=============================================================================
    FrCode.cpp: Flu script virtual machine.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

#include "Engine.h"

/*-----------------------------------------------------------------------------
    CFrame implementation.
-----------------------------------------------------------------------------*/

//
// Static memory for script calling stack.
// 512 kB should be enough.
//
CMemPool CFrame::GLocalsMem( L"Locals", 512 * 1024 );


//
// Initialize frame for the function call.
//
CFrame::CFrame( FEntity* InThis, CFunction* InFunction, Integer InDepth, CFrame* InPrevFrame )
	:	This( InThis ),
		Script( InThis->Script ),
		Bytecode( InFunction ),
		PrevFrame( InPrevFrame ),
		Depth( InDepth ),
		Code( &InFunction->Code[0] ),
		Locals( nullptr )
{
	// Test recursion depth.
	if( InDepth > MAX_RECURSION_DEPTH )
		ScriptError( L"Stack overflow" );

	// Allocate locals.
	Locals	= (Byte*)GLocalsMem.Push0( InFunction->FrameSize );
}


//
// Initialize frame for
// the entity thread execution.
//
CFrame::CFrame( FEntity* InThis, CThreadCode* InThread )
	:	Depth( 0 ),
		Bytecode( InThread ),
		PrevFrame( nullptr ),
		Script( InThis->Script ),
		This( InThis ),
		Locals( nullptr ),
		Code( &InThread->Code[0] )
		
{
}


//
// Trace the script calling stack, start from
// the current frame.
//
String CFrame::StackTrace()
{
	String Result;
	CFrame*	Walk	= this;

	while( Walk )
	{
		Result += String::Format
							( 
								L"<- %s::%s", 
								*Walk->Script->GetName(),
								Walk->Bytecode == Walk->Script->Thread ?
													L"Thread" :
													*((CFunction*)Walk->Bytecode)->Name
							);

		Walk = Walk->PrevFrame;
	}

	return Result;
}


//
// Raise a script exception.
//
void CFrame::ScriptError( Char* Fmt, ... )
{
	Char Dest[2048] = {};
	va_list ArgPtr;
	va_start( ArgPtr, Fmt );
	_vsnwprintf( Dest, 1024, Fmt, ArgPtr );
	va_end( ArgPtr );

	GOutput->ScriptErrorf
		( 
			L"Runtime error at %s(%s::%s). Message='%s'. History: %s.", 
			*This->GetFullName(),
			*Script->GetName(),
			Script->Thread == Bytecode ? L"Thread" : *((CFunction*)Bytecode)->Name,
			Dest,
			*StackTrace()
		);

	// Interrupt execution.
	throw nullptr;
}


//
// Frame destructor.
//
CFrame::~CFrame()
{
	if( Locals )
	{
		CFunction* Func = (CFunction*)Bytecode;

		for( Integer i=0; i<Func->Locals.Num(); i++ )
		{
			CProperty* L = Func->Locals[i];
			L->DestroyValues( Locals + L->Offset );
		}

		GLocalsMem.Pop(Locals);
	}  
}


/*-----------------------------------------------------------------------------
    FEntity implementation.
-----------------------------------------------------------------------------*/

//
// Call a script event.
//
void FEntity::CallEvent( EEventName EventName, CVariant A1, CVariant A2, CVariant A3, CVariant A4 )
{
	// Don't call if no text.
	if( !Script->bHasText || Script->Events.Num()==0 )
		return;

	// Try to find event.
	CFunction* Event = Script->Events[EventName];
	if( !Event )
		return;

	// Execute code!
	try
	{
		// Create new frame frame.
		CFrame Frame( this, Event, 1, nullptr );

		// Parse parameters.
		CVariant* Args[4] = { &A1, &A2, &A3, &A4 };
		for( Integer iArg=0; iArg<4 && iArg<Event->ParmsCount; iArg++ )
		{
			// Everything parsed.
			if( Args[iArg]->Type == TYPE_None )
				break;

			CProperty* Parm = Event->Locals[iArg];

			// Match types.
			if( Parm->Type != Args[iArg]->Type )
			{
				// Mismatched.
				log
				( 
					L"Script: Exception in FEntity::CallEvent(\"%s\"), types are mismatched in %d argument", 
					EventName, 
					iArg 
				);
				GOutput->ScriptErrorf
				(
					L"Event call %s(%s::%s) from C++ failed. Argument %d types mismatched '%s' and '%s'",
					*GetFullName(),
					*Script->GetName(),
					EventName,
					iArg+1,
					*Parm->TypeName(),
					*CTypeInfo(Args[iArg]->Type).TypeName()
				);
				throw nullptr;
			}

			// Copy arguments.
			if( Parm->Type != TYPE_String )
				Parm->CopyValues( &Frame.Locals[Parm->Offset], Args[iArg]->Value );
			else
				*(String*)&Frame.Locals[Parm->Offset] = Args[iArg]->StringValue;
		}

		// Execute the code!
		Frame.ProcessCode( nullptr );
	}
	catch( ... )
	{
		// Script crashes :(
		// But abort only current frame.
		log( L"Script: Runtime error" );
	}
}


/*-----------------------------------------------------------------------------
    Script execution.
-----------------------------------------------------------------------------*/

//
// Execute script function code.
//
void CFrame::ProcessCode( TRegister* Result )
{
	// Infinity loop detection variables.
	Integer LoopCounter = 0;

	// Current execution context.
	FEntity* Context = This;

	// Execute it!
	while( *Code != CODE_EOC )
	{
		EOpCode Op = (EOpCode)*Code++;

		switch( Op )
		{
			case CODE_Jump:
			{
				// Immediately jump.
				Code = &Bytecode->Code[ReadWord()];
				if( LoopCounter++ > MAX_ITERATIONS )
					ScriptError( L"Infinity loop" );
				break;
			}
			case CODE_JumpZero:
			{
				// Conditional jump.
				Word Dst = ReadWord();
				if( !*(Bool*)(Regs[ReadByte()].Value) )
					Code = &Bytecode->Code[Dst];

				if( LoopCounter++ > MAX_ITERATIONS )
					ScriptError( L"Infinity loop" );
				break;
			}
			case CODE_LToR:
			{
				// General purpose l to r.
				Byte iReg = ReadByte();
				MemCopy( Regs[iReg].Value, Regs[iReg].Addr, ReadByte() );
				break;
			}
			case CODE_LToRDWord:
			{
				// DWord l to r.
				Byte iReg = ReadByte();
				*((DWord*)Regs[iReg].Value) = *(DWord*)Regs[iReg].Addr;
				break;
			}
			case CODE_LToRString:
			{
				// String l to r.
				Byte iReg = ReadByte();
				Regs[iReg].StrValue = *(String*)Regs[iReg].Addr;
				break;
			}
			case CODE_Assign:
			{
				// General purpose assignment.
				Byte iDst = ReadByte();
				Byte iSrc = ReadByte();
				MemCopy( Regs[iDst].Addr, Regs[iSrc].Value, ReadByte() );
				break;
			}
			case CODE_AssignDWord:
			{
				// Assign DWord sized value.
				Byte iDst = ReadByte();
				*((DWord*)Regs[iDst].Addr) = *((DWord*)Regs[ReadByte()].Value);
				break;
			}
			case CODE_AssignString:
			{
				// String assignment.
				Byte iDst = ReadByte();
				*((String*)Regs[iDst].Addr) = Regs[ReadByte()].StrValue;
				break;
			}
			case CODE_LocalVar:
			{
				// Local variable.
				Byte iReg = ReadByte();
				Regs[iReg].Addr	= Locals + ReadWord();
				break;
			}
			case CODE_EntityProperty:
			{
				// Get an entity property.
				Byte iReg = ReadByte();
				Regs[iReg].Addr = &Context->InstanceBuffer->Data[ReadWord()];
				break;
			}
			case CODE_BaseProperty:
			{
				// Get an base component property.
				Byte* Base = (Byte*)Context->Base;
				Byte iReg = ReadByte();
				Regs[iReg].Addr = Base + ReadWord();
				break;
			}
			case CODE_ComponentProperty:
			{
				// Get an extra component property.
				Byte*	Component =	(Byte*)Context->Components[ReadByte()];
				Byte iReg = ReadByte();
				Regs[iReg].Addr = Component + ReadWord();
				break;
			}
			case CODE_ResourceProperty:
			{
				// Get an resource property.
				Byte iReg = ReadByte();
				FResource* Res = *(FResource**)Regs[iReg].Value;
				if( !Res )
					ScriptError( L"Access to null resource" );
				Regs[iReg].Addr	= (Byte*)Res + ReadWord();
				break;
			}
			case CODE_ProtoProperty:
			{
				// Get a prototype property.
				FScript*	Prototype	= ReadScript();
				Byte		iSource		= ReadByte();
				Byte		iReg		= ReadByte();
				Byte*		BaseAddr	=	iSource == 0xff ? (Byte*)&Prototype->InstanceBuffer->Data[0] :
											iSource == 0xfe ? (Byte*)Prototype->Base : (Byte*)Prototype->Components[iSource];
				Regs[iReg].Addr	= BaseAddr + ReadWord();
				break;
			}
			case CODE_ArrayElem:
			{
				// Get an array element.
				Byte iReg = ReadByte();
				Integer Index = *(Integer*)Regs[ReadByte()].Value;
				
				Regs[iReg].Addr = (Byte*)Regs[iReg].Addr + Index * ReadByte();

				Integer Max = ReadByte();
				if( Index < 0 || Index >= Max )
					ScriptError( L"Array violates bounds %i/%i", Index, Max );
				break;
			}
			case CODE_LMember:
			{
				// Get an l-value member.
				Byte iReg = ReadByte();
				Regs[iReg].Addr = (Byte*)Regs[iReg].Addr + ReadByte();
				break;
			}
			case CODE_RMember:
			{
				// Get an r-value member.
				Byte iReg = ReadByte();
				Byte Offset = ReadByte();
				MemCopy( &Regs[iReg].Value[0], &Regs[iReg].Value[Offset], 16-Offset );
				break;
			}
			case CODE_This:
			{
				// This reference.
				*(FEntity**)(Regs[ReadByte()].Value) = This;
				break;
			}
			case CODE_ConstByte:
			{
				// Byte constant.
				Byte Value = ReadByte();
				*(Byte*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstBool:
			{
				// Bool constant.
				Bool Value = ReadBool();
				*(Bool*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstInteger:
			{
				// Integer constant.
				Integer Value = ReadInteger();
				*(Integer*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstFloat:
			{
				// Float constant.
				Float Value = ReadFloat();
				*(Float*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstAngle:
			{
				// Angle constant.
				TAngle Value = ReadAngle();
				*(TAngle*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstColor:
			{
				// Color constant.
				TColor Value = ReadColor();
				*(TColor*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstString:
			{
				// String constant.
				String Value = ReadString();
				Regs[ReadByte()].StrValue = Value;
				break;
			}
			case CODE_ConstVector:
			{
				// Vector constant.
				TVector Value = ReadVector();
				*(TVector*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstAABB:
			{
				// TRect constant.
				TRect Value = ReadAABB();
				*(TRect*)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstResource:
			{
				// FResource constant.
				FResource* Value = ReadResource();
				*(FResource**)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_ConstEntity:
			{
				// FEntity constant.
				FEntity* Value = ReadEntity();
				*(FEntity**)(Regs[ReadByte()].Value) = Value;
				break;
			}
			case CODE_Assert:
			{
				// Assertion.
				Word Line = ReadWord();
				if( !*(Bool*)(Regs[ReadByte()].Value) )
					ScriptError( L"Assertion failed! Line: %d", Line );
				break;
			}
			case CODE_Log:
			{
				// C-style output function, its really SLOW!!
				String Fmt = ReadString();
				Char Out[256], *Str = Out;

				for( Integer i=0; i<Fmt.Len(); i++ )
				{
					if( Fmt[i] != L'%' )
					{
						*Str++ = Fmt[i];
					}
					else
					{
						CTypeInfo Info = CTypeInfo( ReadPropType() );
						Byte iReg = ReadByte();
						String Value = Info.Type == TYPE_String ? Regs[iReg].StrValue : Info.ToString( Regs[iReg].Value );

						for( Integer j=0; j<Value.Len(); j++ )
							*Str++ = Value[j];
						i++;
					}
				}

				*Str = 0;
				log( L"Script: %s", Out );
				break;
			}
			case CODE_EntityCast:
			{
				// Entity explicit cast.
				Byte iReg			= ReadByte();
				FScript* DstType	= ReadScript();
				FEntity* Value		= *(FEntity**)Regs[iReg].Value;
				if( Value && Value->Script != DstType )
					ScriptError
							( 
								L"Invalid script typecast '%s' to '%s'", 
								*Value->Script->GetName(), 
								*DstType->GetName() 
							);
				break;
			}
			case CODE_FamilyCast:
			{
				// Entity explicit family cast.
				Byte	iReg		= ReadByte();
				Integer	iFamily		= ReadInteger();
				FEntity* Value		= *(FEntity**)Regs[iReg].Value;

				if( Value && Value->Script->iFamily != iFamily )
					ScriptError
							( 
								L"Invalid family typecast '%d' to '%d'", 
								Value->Script->iFamily, 
								iFamily
							);
				break;
			}
			case CODE_Length:
			{
				// String length.
				Byte iStr = ReadByte();
				*(Integer*)(Regs[ReadByte()].Value) = Regs[iStr].StrValue.Len();
				break;
			}
			case CODE_New:
			{
				// Create a new entity.
				static String TempName = L"Temp";
				FLevel* Level		= This->Level;
				FScript* Script		= As<FScript>(*(FObject**)Regs[ReadByte()].Value);
				if( !Script )
					ScriptError( L"Failed create entity, meta-script is not specified" );

				*(FEntity**)(Regs[ReadByte()].Value) = Level->CreateEntity( Script, TempName, This->Base->Location );
				break;
			}
			case CODE_Delete:
			{
				// Delete an entity.
				FEntity* Poor = *(FEntity**)(Regs[ReadByte()].Value);
				if( Poor )
					Poor->Base->bDestroyed	= true;
				else
					ScriptError( L"An attempt to delete undefined entity" );
				break;
			}
			case CODE_VectorCnstr:
			{
				// Vector constructor.
				TVector* VectorPtr = (TVector*)Regs[ReadByte()].Value;
				VectorPtr->X = *(Float*)Regs[ReadByte()].Value;	
				VectorPtr->Y = *(Float*)Regs[ReadByte()].Value;	
				break;
			}
			case CODE_Label:
			{
				// Current label id.
				*(Integer*)Regs[ReadByte()].Value = This->Thread->LabelId;
				break;
			}
			case CODE_Is:
			{
				// Test entity script.
				Byte iReg = ReadByte();
				FEntity* Entity = *(FEntity**)(Regs[iReg].Value);
				FScript* Test	= *(FScript**)(Regs[ReadByte()].Value);
				if( !Test )
					ScriptError( L"'is' failure, meta-script is null" );

				*(Bool*)(Regs[iReg].Value) = Entity ? Entity->Script == Test : false;
				break;
			}
			case CODE_In:
			{
				// Test entity family.
				Byte iReg = ReadByte();
				FEntity* Entity = *(FEntity**)(Regs[iReg].Value);
				Integer	iFamily	= ReadInteger();
				*(Bool*)(Regs[iReg].Value) = Entity ? Entity->Script->iFamily == iFamily : false;
				break;
			}
			case CODE_Equal:
			{
				// Comparison operator "==".
				Byte i1	= ReadByte();
				Byte i2 = ReadByte();	
				Byte Size = ReadByte();
				*(Bool*)(Regs[i1].Value) = Size != 0 ? MemCmp( Regs[i1].Value, Regs[i2].Value, Size ) : Regs[i1].StrValue == Regs[i2].StrValue;
				break;
			}
			case CODE_NotEqual:
			{
				// Comparison operator "!=".
				Byte i1	= ReadByte();
				Byte i2 = ReadByte();	
				Byte Size = ReadByte();
				*(Bool*)(Regs[i1].Value) = Size != 0 ? !MemCmp( Regs[i1].Value, Regs[i2].Value, Size ) : Regs[i1].StrValue != Regs[i2].StrValue;
				break;
			}
			case CODE_ConditionalOp:
			{
				// Ternary if.
				Byte iRes		= ReadByte();
				Byte iFirst		= ReadByte();
				Byte iSecond	= ReadByte();
				Regs[iRes]		= Regs[*(Bool*)(Regs[ReadByte()].Value) ? iFirst : iSecond];
				break;
			}
			case CAST_ByteToInteger:
			{
				// Byte to integer cast.
				Byte iReg = ReadByte();
				*(Integer*)Regs[iReg].Value = *(Byte*)Regs[iReg].Value;
				break;
			}
			case CAST_ByteToFloat:
			{
				// Byte to float cast.
				Byte iReg = ReadByte();
				*(Float*)Regs[iReg].Value = *(Byte*)Regs[iReg].Value;
				break;
			}
			case CAST_ByteToAngle:
			{
				// Byte to angle cast.
				Byte iReg = ReadByte();
				*(TAngle*)Regs[iReg].Value = *(Byte*)Regs[iReg].Value;
				break;
			}
			case CAST_IntegerToFloat:
			{
				// Integer to float cast.
				Byte iReg = ReadByte();
				*(Float*)Regs[iReg].Value = *(Integer*)Regs[iReg].Value;
				break;
			}
			case CAST_IntegerToByte:
			{
				// Integer to byte cast.
				Byte iReg = ReadByte();
				*(Byte*)Regs[iReg].Value = *(Integer*)Regs[iReg].Value;
				break;
			}
			case CAST_IntegerToAngle:
			{
				// Integer to angle cast.
				Byte iReg = ReadByte();
				*(TAngle*)Regs[iReg].Value = *(Integer*)Regs[iReg].Value;
				break;
			}
			case CAST_AngleToInteger:
			{
				// Angle to integer cast.
				Byte iReg = ReadByte();
				*(Integer*)Regs[iReg].Value = *(TAngle*)Regs[iReg].Value;
				break;
			}
			case CODE_Context:
			{
				// Change current context.
				FEntity* NewContext = *(FEntity**)Regs[ReadByte()].Value;
				if( !NewContext )
					ScriptError( L"Access to undefined entity" );
				Context	= NewContext;
				break;
			}
			case CODE_Switch:
			{
				// Perform switch statement.
				Integer Expr = 0;
				Byte Size = ReadByte();
				MemCopy( &Expr, Regs[ReadByte()].Value, Size );
				Byte* AddrDef = &Bytecode->Code[ReadWord()];
				Code = &Bytecode->Code[ReadWord()];
				Integer NumLabs = ReadByte();

				for( Integer i=0; i<NumLabs; i++ )
				{
					Integer Label	= 0;
					MemCopy( &Label, Code, Size );
					Code += Size;
					Word Addr		= ReadWord();

					if( Label == Expr )
					{
						Code		= &Bytecode->Code[Addr];
						goto LabFound;
					}
				}

				// No labels found, goto default.
				Code	= AddrDef;
				LabFound:
				break;
			}
			case CODE_Foreach:
			{
				// Perform foreach statement.
				FEntity**	Value	= (FEntity**)&Locals[ReadWord()];	
				Word		EndAddr	= ReadWord();

				if( Foreach.i < Foreach.Collection.Num() )
				{
					*Value		= Foreach.Collection[Foreach.i];
					Foreach.i++;
				}
				else
				{
					*Value		= nullptr;
					Code		= &Bytecode->Code[EndAddr];
					Foreach.i	= 0;
					Foreach.Collection.Empty();
				}
				break;
			}
			case CODE_CallFunction:
			{
				// Call script function.
				CFunction* Func = Context->Script->Functions[ReadByte()];
				void* OutParms[16];
				assert(Func->ParmsCount<=array_length(OutParms));
				CFrame NewFrame( Context, Func, Depth+1, this );

				for( Integer i=0; i<Func->ParmsCount; i++ )
				{
					CProperty* Arg = Func->Locals[i];
					Byte iReg = ReadByte();
					if( Arg->Flags & PROP_OutParm )
					{
						// Out param.
						OutParms[i]	= Regs[iReg].Addr;
						Arg->CopyValues
									(	
										NewFrame.Locals + Arg->Offset,
										OutParms[i]
									);
					}
					else
					{
						// Regular param.
						Arg->CopyValues
									(	
										NewFrame.Locals + Arg->Offset,
										Arg->Type == TYPE_String ? (Byte*)&Regs[iReg].StrValue : (Byte*)Regs[iReg].Value
									);
					}
				}

				if( Func->ResultVar )
				{
					// With result.
					Byte iRes = ReadByte();
					NewFrame.ProcessCode( &Regs[iRes] );
				}
				else
				{
					// Without result.
					NewFrame.ProcessCode( nullptr );
				}

				// Copy out parameters back.
				for( Integer i=0; i<Func->ParmsCount; i++ )
				{
					CProperty* Arg = Func->Locals[i];
					if( Arg->Flags & PROP_OutParm )
						Arg->CopyValues( OutParms[i], NewFrame.Locals + Arg->Offset );
				}

				break;
			}
			case CODE_CallVF:
			{
				// Call virtual function.
				CFunction* Func = Context->Script->VFTable[ReadByte()];
				if( !Func )
					ScriptError( L"Attempt call abstract method from '%s'", *Context->Script->GetName() );		

				CFrame NewFrame( Context, Func, Depth+1, this );

				for( Integer i=0; i<Func->ParmsCount; i++ )
				{
					CProperty* Arg = Func->Locals[i];
					Byte iReg = ReadByte();
					Arg->CopyValues
								(	
									NewFrame.Locals + Arg->Offset,
									Arg->Type == TYPE_String ? (Byte*)&Regs[iReg].StrValue : (Byte*)Regs[iReg].Value
								);
				}

				if( Func->ResultVar )
				{
					// With result.
					Byte iRes = ReadByte();
					NewFrame.ProcessCode( &Regs[iRes] );
				}
				else
				{
					// Without result.
					NewFrame.ProcessCode( nullptr );
				}
				break;
			}
			case CODE_BaseMethod:
			{
				// Base method call.
				CNativeFunction* Native = CClassDatabase::GFuncs[ReadWord()];
				((Context->Base)->*(Native->ptrMethod))( *this );
				break;
			}
			case CODE_ComponentMethod:
			{
				// Component method call.
				CNativeFunction* Native = CClassDatabase::GFuncs[ReadWord()];
				((Context->Components[ReadByte()])->*(Native->ptrMethod))( *this );
				break;
			}
			case CODE_Stop:
			{
				// Stop thread execution.
				This->Thread->Status	= THR_Stopped;
				goto LeaveCode;
			}
			case CODE_Sleep:
			{
				// Make thread sleep.
				This->Thread->SleepTime	= *(Float*)(Regs[ReadByte()].Value);
				This->Thread->Status	= THR_Sleep;
				goto LeaveCode;
			}
			case CODE_Wait:
			{
				// Force the thread to wait.
				This->Thread->WaitExpr	= &Bytecode->Code[ReadWord()];

				if( *(Bool*)(Regs[ReadByte()].Value) )
				{
					// AWake.
					This->Thread->Status	= THR_Run;
					This->Thread->WaitExpr	= nullptr;
				}
				else
				{
					// Still wait.
					This->Thread->Status	= THR_Wait;
				}
				goto LeaveCode;
			}
			case CODE_Goto:
			{
				// Goto label in thread.
				Integer iLabel = *(Integer*)(Regs[ReadByte()].Value);

				if( iLabel >= 0 && iLabel < Script->Thread->Labels.Num() )
				{
					// Goto label and restart execution.
					Word LabAddr				= Script->Thread->Labels[iLabel].Address;
					This->Thread->Frame.Code	= &Script->Thread->Code[LabAddr];
					This->Thread->Status		= THR_Run;
					This->Thread->LabelId		= iLabel;
				}
				else
					ScriptError( L"Bad label %d in 'goto'", iLabel );

				// Interrupt thread, if in running in thread.
				if( Bytecode == Script->Thread )
					goto LeaveCode;

				break;
			}
			case CODE_Interrupt:
			{
				// Interrupt thread execution.
				goto LeaveCode;
				break;
			}
			default:
			{
				// Delegate execution to native functions.
				ExecuteNative( Context, Op );
				break;
			}
		}
	}

LeaveCode:;

	// Copy result if required.
	if( Result && (Bytecode != Script->Thread) && ((CFunction*)Bytecode)->ResultVar )
	{
		CProperty* ResProp = ((CFunction*)Bytecode)->ResultVar;
		ResProp->CopyValues
						( 
							ResProp->Type == TYPE_String ? (Byte*)&Result->StrValue : Result->Value,
							Locals + ResProp->Offset
						);
	}
}


/*-----------------------------------------------------------------------------
    CEntityThread implementation.
-----------------------------------------------------------------------------*/

//
// Thread constructor. Creates frame for execution.
// Warning: Use only while playing, since cause
// problems with GC and Undo/Redo.
//
CEntityThread::CEntityThread( FEntity* InEntity, CThreadCode* InThread )
	:	Frame( InEntity, InThread ),
		Status( THR_Run ),
		Entity( InEntity ),
		SleepTime( 0.f ),
		WaitExpr( nullptr ),
		LabelId( -1 )
{
}


//
// Entity thread destructor. Call only
// when entity die, or game over.
//
CEntityThread::~CEntityThread()
{
}


//
// Execute entity's thread code.
//
void CEntityThread::Tick( Float Delta )
{
	// Execute code!
	try
	{
		switch( Status )
		{
			case THR_Run:
			{
				// Normally process the code.
				Frame.ProcessCode( nullptr );
				break;
			}
			case THR_Stopped:
			{
				// Thread are stopped.
				// Nothing to do.
				break;
			}
			case THR_Sleep:
			{
				// Sleep until time not passed.
				SleepTime -= Delta;

				if( SleepTime <= 0.f )
				{
					// Awake.
					SleepTime	= 0.f;
					Status		= THR_Run;
				}
				break;
			}
			case THR_Wait:
			{
				// Wait expression became true.
				assert(WaitExpr != nullptr);

				Frame.Code = WaitExpr;
				Frame.ProcessCode( nullptr );		
				break;
			}
			default:
				error( L"Bad thread '%s' status '%d'", *Entity->GetFullName(), (Byte)Status );
		}
	}
	catch( ... )
	{
		// Damn, something horrible happened.
		log( L"Entity: Thread error in entity '%s'", *Entity->GetFullName() );
	}
}


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/