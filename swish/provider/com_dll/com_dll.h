

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Wed Apr 21 19:34:53 2010
 */
/* Compiler settings for .\com_dll.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __com_dll_h__
#define __com_dll_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __Provider_FWD_DEFINED__
#define __Provider_FWD_DEFINED__

#ifdef __cplusplus
typedef class Provider Provider;
#else
typedef struct Provider Provider;
#endif /* __cplusplus */

#endif 	/* __Provider_FWD_DEFINED__ */


#ifndef __RealDispenser_FWD_DEFINED__
#define __RealDispenser_FWD_DEFINED__

#ifdef __cplusplus
typedef class RealDispenser RealDispenser;
#else
typedef struct RealDispenser RealDispenser;
#endif /* __cplusplus */

#endif 	/* __RealDispenser_FWD_DEFINED__ */


#ifndef __Dispenser_FWD_DEFINED__
#define __Dispenser_FWD_DEFINED__

#ifdef __cplusplus
typedef class Dispenser Dispenser;
#else
typedef struct Dispenser Dispenser;
#endif /* __cplusplus */

#endif 	/* __Dispenser_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __ProviderLib_LIBRARY_DEFINED__
#define __ProviderLib_LIBRARY_DEFINED__

/* library ProviderLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ProviderLib;

EXTERN_C const CLSID CLSID_Provider;

#ifdef __cplusplus

class DECLSPEC_UUID("b816a862-5022-11dc-9153-0090f5284f85")
Provider;
#endif

EXTERN_C const CLSID CLSID_RealDispenser;

#ifdef __cplusplus

class DECLSPEC_UUID("b816a863-5022-11dc-9153-0090f5284f85")
RealDispenser;
#endif

EXTERN_C const CLSID CLSID_Dispenser;

#ifdef __cplusplus

class DECLSPEC_UUID("b816a864-5022-11dc-9153-0090f5284f85")
Dispenser;
#endif
#endif /* __ProviderLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


