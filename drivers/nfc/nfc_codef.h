#ifndef _NFC_CODEF_H_
#define _NFC_CODEF_H_

#ifndef BOOL
#define BOOL unsigned char
#endif
#ifndef TRUE
#define TRUE (BOOL) 1
#endif
#ifndef FALSE
#define FALSE (BOOL) 0
#endif

#ifndef SUCCESS
#define SUCCESS (0)
#endif
#ifndef FAILURE
#define FAILURE (-1)
#endif

typedef signed char		S8;
typedef unsigned char		U8;
typedef signed short		S16;
typedef unsigned short		U16;
typedef int32_t			S32;
typedef u_int32_t		U32;

typedef signed char *		PS8;
typedef unsigned char *		PU8;
typedef signed short *		PS16;
typedef unsigned short *	PU16;
typedef int32_t *		PS32;
typedef u_int32_t *		PU32;

#endif
