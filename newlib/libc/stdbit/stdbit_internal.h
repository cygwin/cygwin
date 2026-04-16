/*
 * Newlib specific addition to ensure the C23 *_WIDTH constants
 * used in the stdbits implementation are available even when
 * compiling using another version of the C Programming Language.
 */

/*
 *  Written by Joel Sherrill <joel.sherrill@OARcorp.com>.
 *
 *  COPYRIGHT (c) 2026.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  Permission to use, copy, modify, and distribute this software for any
 *  purpose without fee is hereby granted, provided that this entire notice
 *  is included in all copies of any software which is or includes a copy
 *  or modification of this software.
 *
 *  THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 *  WARRANTY.  IN PARTICULAR,  THE AUTHOR MAKES NO REPRESENTATION
 *  OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
 *  SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#ifndef STDBIT_INTERNAL_h
#define STDBIT_INTERNAL_h

#ifndef UCHAR_WIDTH
#define UCHAR_WIDTH __SCHAR_WIDTH__
#endif

#ifndef USHRT_WIDTH
#define USHRT_WIDTH __SHRT_WIDTH__
#endif

#ifndef UINT_WIDTH
#define UINT_WIDTH __INT_WIDTH__
#endif

#ifndef ULONG_WIDTH
#define ULONG_WIDTH __LONG_LONG_WIDTH__
#endif

#ifndef ULLONG_WIDTH
#define ULLONG_WIDTH __LONG_LONG_WIDTH__
#endif

#endif
