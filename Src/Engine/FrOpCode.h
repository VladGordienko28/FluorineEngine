/*=============================================================================
    FrOpCode.h: List of OPerations CODEs.
    Copyright Jul.2016 Vlad Gordienko.
=============================================================================*/

//
// All codes.
//
enum EOpCode
{
	// Special marker 'End Of Code'.
	CODE_EOC				= 0x00,

	// Jump operations.
	CODE_Jump				= 0x01,
	CODE_JumpZero			= 0x02,
	CODE_Switch				= 0x03,
	CODE_Foreach			= 0x3d,

	// Constants.
	CODE_ConstByte			= 0x04,
	CODE_ConstBool			= 0x05,
	CODE_ConstInteger		= 0x06,
	CODE_ConstFloat			= 0x07,
	CODE_ConstAngle			= 0x08,
	CODE_ConstColor			= 0x09,
	CODE_ConstString		= 0x0a,
	CODE_ConstVector		= 0x0b,
	CODE_ConstAABB			= 0x0c,
	CODE_ConstResource		= 0x0d,
	CODE_ConstEntity		= 0x0e,

	// Entity relative.
	CODE_This				= 0x0f,
	CODE_EntityCast			= 0x10,
	CODE_FamilyCast			= 0x39,
	CODE_Context			= 0x11,

	// Basic functions.
	CODE_Is					= 0x12,
	CODE_In					= 0x13,
	CODE_New				= 0x14,
	CODE_Delete				= 0x15,
	CODE_Log				= 0x16,
	CODE_Assert				= 0x17,
	CODE_Length				= 0x18,

	// Assignments.
	CODE_Assign				= 0x19,
	CODE_AssignDWord		= 0x1a,
	CODE_AssignString		= 0x1b,

	// Variables.
	CODE_LocalVar			= 0x1c,
	CODE_EntityProperty		= 0x1d,
	CODE_BaseProperty		= 0x3a,
	CODE_ComponentProperty	= 0x1e,
	CODE_ResourceProperty	= 0x1f,
	CODE_ProtoProperty		= 0x3c,

	// L-value to R-value conversion.
	CODE_LToR				= 0x20,
	CODE_LToRDWord			= 0x21,
	CODE_LToRString			= 0x22,

	// Elements functions.
	CODE_ArrayElem			= 0x23,
	CODE_RMember			= 0x24,
	CODE_LMember			= 0x25,

	// Function call.
	CODE_BaseMethod			= 0x3b,
	CODE_ComponentMethod	= 0x26,
	CODE_CallFunction		= 0x27,
	CODE_CallVF				= 0x28,

	// Thread operations.
	CODE_Stop				= 0x29,
	CODE_Sleep				= 0x2a,
	CODE_Goto				= 0x2b,
	CODE_Wait				= 0x2c,
	CODE_Interrupt			= 0x3e,
	CODE_Label				= 0x2d,

	// Comparison.
	CODE_Equal				= 0x2e,
	CODE_NotEqual			= 0x2f,

	// Misc.
	CODE_VectorCnstr		= 0x30,	
	CODE_ConditionalOp		= 0x31,

	// Implicit cast.
	CAST_ByteToInteger		= 0x32,
	CAST_ByteToFloat		= 0x33,
	CAST_ByteToAngle		= 0x34,
	CAST_IntegerToFloat		= 0x35,
	CAST_IntegerToByte		= 0x36,
	CAST_IntegerToAngle		= 0x37,
	CAST_AngleToInteger		= 0x38,
	// Available op-codes: 0x3f-0x3f.

	// Unary operators.
	UN_Inc_Integer			= 0x40,
	UN_Inc_Float			= 0x41,
	UN_Dec_Integer			= 0x42,
	UN_Dec_Float			= 0x43,
	UN_Plus_Integer			= 0x44,
	UN_Plus_Float			= 0x45,
	UN_Plus_Vector			= 0x46,
	UN_Plus_Color			= 0x47,
	UN_Minus_Integer		= 0x48,
	UN_Minus_Float			= 0x49,
	UN_Minus_Vector			= 0x4a,
	UN_Minus_Color			= 0x4b,
	UN_Not_Bool				= 0x4c,
	UN_Not_Integer			= 0x4d,

	// Binary operators.
	BIN_Mult_Integer		= 0x4e,
	BIN_Mult_Float			= 0x4f,
	BIN_Mult_Color			= 0x50,
	BIN_Mult_Vector			= 0x51,
	BIN_Div_Integer			= 0x52,
	BIN_Div_Float			= 0x53,
	BIN_Mod_Integer			= 0x54,
	BIN_Add_Integer			= 0x55,
	BIN_Add_Float			= 0x56,
	BIN_Add_Color			= 0x57,
	BIN_Add_String			= 0x58,
	BIN_Add_Vector			= 0x59,
	BIN_Sub_Integer			= 0x5a,
	BIN_Sub_Float			= 0x5b,
	BIN_Sub_Color			= 0x5c,
	BIN_Sub_Vector			= 0x5d,
	BIN_Shr_Integer			= 0x5e,
	BIN_Shl_Integer			= 0x5f,
	BIN_Less_Integer		= 0x60,
	BIN_Less_Float			= 0x61,
	BIN_LessEq_Integer		= 0x62,
	BIN_LessEq_Float		= 0x63,
	BIN_Greater_Integer		= 0x64,
	BIN_Greater_Float		= 0x65,
	BIN_GreaterEq_Integer	= 0x66,
	BIN_GreaterEq_Float		= 0x67,
	BIN_And_Integer			= 0x68,
	BIN_Xor_Integer			= 0x69,
	BIN_Cross_Vector		= 0x6a,
	BIN_Or_Integer			= 0x6b,
	BIN_Dot_Vector			= 0x6c,
	BIN_AddEqual_Integer	= 0x6d,
	BIN_AddEqual_Float		= 0x6e,
	BIN_AddEqual_Vector		= 0x6f,
	BIN_AddEqual_String		= 0x70,
	BIN_AddEqual_Color		= 0x71,
	BIN_SubEqual_Integer	= 0x72,
	BIN_SubEqual_Float		= 0x73,
	BIN_SubEqual_Vector		= 0x74,
	BIN_SubEqual_Color		= 0x75,
	BIN_MulEqual_Integer	= 0x76,
	BIN_MulEqual_Float		= 0x77,
	BIN_MulEqual_Color		= 0x78,
	BIN_DivEqual_Integer	= 0x79,
	BIN_DivEqual_Float		= 0x7a,
	BIN_ModEqual_Integer	= 0x7b,
	BIN_ShlEqual_Integer	= 0x7c,
	BIN_ShrEqual_Integer	= 0x7d,
	BIN_AndEqual_Integer	= 0x7e,
	BIN_XorEqual_Integer	= 0x7f,
	BIN_OrEqual_Integer		= 0x80,
	// Available op-codes: 0x81-0x8f.

	// Native core functions.
	OP_Abs					= 0x90,
	OP_ArcTan				= 0x91,
	OP_ArcTan2				= 0x92,
	OP_Cos					= 0x93,
	OP_Sin					= 0x94,
	OP_Sqrt					= 0x95,
	OP_Distance				= 0x96,
	OP_Exp					= 0x97,
	OP_Ln					= 0x98,
	OP_Frac					= 0x99,
	OP_Round				= 0x9a,
	OP_Normalize			= 0x9b,
	OP_Random				= 0x9c,
	OP_RandomF				= 0x9d,
	OP_VectorSize			= 0x9e,
	OP_VectorToAngle		= 0x9f,
	OP_AngleToVector		= 0xa0,
	OP_RGBA					= 0xa1,
	OP_IToS					= 0xa2,
	OP_CharAt				= 0xa3,
	OP_IndexOf				= 0xa4,
	OP_Execute				= 0xa5,
	OP_Now					= 0xa6,
	// Available op-codes: 0xa7-0xbf.

	// Native engine functions.
	OP_PlaySoundFX			= 0xc0,
	OP_PlayMusic			= 0xc1,
	OP_KeyIsPressed			= 0xc2,
	OP_GetCamera			= 0xc4,
	OP_GetScreenCursor		= 0xc5,
	OP_GetWorldCursor		= 0xc6,
	OP_Localize				= 0xc7,
	OP_GetScript			= 0xc8,
	OP_StaticPush			= 0xc9,
	OP_StaticPop			= 0xca,
	OP_TravelTo				= 0xcb,
	OP_FindEntity			= 0xce,
	IT_AllEntities			= 0xc3,
	IT_RectEntities			= 0xcc,
	IT_TouchedEntities		= 0xcd
	// Available op-codes: 0xcf-0xff.
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/