/*=============================================================================
    FrKeyword.h: List of fluscript keywords.
    Copyright Jun.2016 Vlad Gordienko.
=============================================================================*/

// It's messy a little. Store source macro.
// Its here since some keywords are C++ macro also.
#pragma push_macro("log")
#pragma push_macro("assert")
#undef log
#undef assert

// List of keywords.
KEYWORD(null)
KEYWORD(false)
KEYWORD(true)
KEYWORD(undefined)
KEYWORD(family)
KEYWORD(script)
KEYWORD(private)
KEYWORD(public)
KEYWORD(const)
KEYWORD(enum)
KEYWORD(byte)
KEYWORD(bool)
KEYWORD(integer)
KEYWORD(float)
KEYWORD(angle)
KEYWORD(color)
KEYWORD(string)
KEYWORD(vector)
KEYWORD(aabb)
KEYWORD(entity)
KEYWORD(fn)
KEYWORD(event)
KEYWORD(thread)
KEYWORD(result)
KEYWORD(if)
KEYWORD(for)
KEYWORD(do)
KEYWORD(switch)
KEYWORD(while)
KEYWORD(break)
KEYWORD(return)
KEYWORD(continue)
KEYWORD(stop)
KEYWORD(goto)
KEYWORD(sleep)
KEYWORD(wait)
KEYWORD(this)
KEYWORD(log)
KEYWORD(assert)
KEYWORD(else)
KEYWORD(is)
KEYWORD(base)
KEYWORD(length)
KEYWORD(label)
KEYWORD(new)
KEYWORD(delete)
KEYWORD(default)
KEYWORD(case)
KEYWORD(unified)
KEYWORD(in)
KEYWORD(out)
KEYWORD(proto)
KEYWORD(foreach)
KEYWORD(interrupt)

// Restore source macro.
#pragma pop_macro("assert")
#pragma pop_macro("log")

/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/