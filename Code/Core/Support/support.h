/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Support (SPT)											**
**																			**
**	Created:		06/10/99	mjb											**
**																			**
**	File name:		core/support/support.h									**
**																			**
**	Description:	template class for tracking class hierarchy				**
**																			**
*****************************************************************************/

#ifndef __CORE_SUPPORT_SUPPORT_H
#define __CORE_SUPPORT_SUPPORT_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/
#ifdef __PLAT_XBOX__
#include <typeinfo.h>
#else
#include <typeinfo>
#endif
#include <core/support/class.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

//#define class _c : public Spt::Class					class _c : public Spt::Class
//#define class _c : public _b				class _c : public _b

#define nTemplateBaseClass(_t,_c)				\
	template< class _t >						\
	class _c : public Spt::Class								\

#define nTemplateBaseClass2(_t1,_t2,_c)			\
	template< class _t1, class _t2 >			\
	class _c : public Spt::Class								\

#define nTemplateBaseClass3(_t1,_t2,_t3,_c)		\
	template< class _t1, class _t2, class _t3 >	\
	class _c : public Spt::Class								\

#define nTemplateSubClass(_t,_c,_b)				\
	template< class _t >						\
	class _c : public _b							\

#define nTemplateSubClass2(_t1,_t2,_c,_b)		\
	template< class _t1, class _t2 >			\
	class _c : public _b							\

#define nTemplateSubClass3(_t1,_t2,_t3,_c,_b)	\
	template< class _t1, class _t2, class _t3 >	\
	class _c : public _b							\


#define Dbg_TemplateBaseClass(_t1,_c)
#define Dbg_TemplateSubClass(_t1,_c,_b)	

#define Dbg_TemplateBaseClass(_t1,_c)
#define Dbg_TemplateSubClass(_t1,_c,_b)	

#define Dbg_TemplateBaseClass2(_t1,_t2,_c)
#define Dbg_TemplateSubClass2(_t1,_t2,_c,_b)	


#endif // __CORE_SUPPORT_SUPPORT_H
