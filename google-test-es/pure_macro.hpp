#ifndef pure_macro_H__
#define pure_macro_H__

#define QUOTE(x) #x
#define XQUOTE(x) QUOTE(x)

#ifdef DEBUG
	#define DEBUG_LITERAL 1
#else
	#define	DEBUG_LITERAL 0
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Workaround for macros which are expected to yield constant
// expressions, but somebody defined them as
//		#define MOL
// instead of
//		#define MOL 42
// causing the compiler to accuse you of trying to evaluate
// a non-expression whenever you use such macros as intended.
// Examples of this are buggy extension-advertising macros in
// gl2ext headers. This workaround macro function forces such
// misconstrued macros to evaluate to zero.
////////////////////////////////////////////////////////////////////////////////////////////////////

#define EVAL_CONST(x) (x + 0)

#endif // pure_macro_H_
