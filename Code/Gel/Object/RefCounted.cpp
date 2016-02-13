#include <core/defines.h>
#include <gel/RefCounted.h>
#include <gel/ObjPtr.h>

namespace Obj
{




CRefCounted::CRefCounted()
{
	mp_smart_ptr_list = NULL;
}




CRefCounted::~CRefCounted()
{
	CSmtPtr<CRefCounted> *p_smt = mp_smart_ptr_list;
	while(p_smt)
	{
		CSmtPtr<CRefCounted> *p_next = p_smt->mp_next_ptr;
		Dbg_Assert(p_smt->mp_object);
		Dbg_Assert(p_smt->mp_object == this);
		*p_smt = NULL;
		p_smt = p_next;
	}
}




void CRefCounted::AddSmartPointer(CSmtPtr<CRefCounted> *pSmtPtr)
{
	//debug_validate_smart_pointers();
	
	Dbg_Assert(!pSmtPtr->mp_prev_ptr);
	Dbg_Assert(!pSmtPtr->mp_next_ptr);
	pSmtPtr->mp_next_ptr = mp_smart_ptr_list;
	if (mp_smart_ptr_list)
		mp_smart_ptr_list->mp_prev_ptr = pSmtPtr;
	mp_smart_ptr_list = pSmtPtr;

	//debug_validate_smart_pointers();
}




void CRefCounted::RemoveSmartPointer(CSmtPtr<CRefCounted> *pSmtPtr)
{
	Dbg_Assert(pSmtPtr->mp_object == this);
	//debug_validate_smart_pointers(pSmtPtr);

	if (pSmtPtr->mp_prev_ptr)
	{
		Dbg_Assert(pSmtPtr->mp_prev_ptr->mp_next_ptr == pSmtPtr);
		pSmtPtr->mp_prev_ptr->mp_next_ptr = pSmtPtr->mp_next_ptr;
	}
	else
		mp_smart_ptr_list = pSmtPtr->mp_next_ptr;

	if (pSmtPtr->mp_next_ptr)
	{
		Dbg_Assert(pSmtPtr->mp_next_ptr->mp_prev_ptr == pSmtPtr);
		pSmtPtr->mp_next_ptr->mp_prev_ptr = pSmtPtr->mp_prev_ptr;
	}

	pSmtPtr->mp_prev_ptr = NULL;
	pSmtPtr->mp_next_ptr = NULL;
	
	//debug_validate_smart_pointers();
}




void CRefCounted::debug_validate_smart_pointers(CSmtPtr<CRefCounted> *pPtrToCheckForInclusion)
{
	bool included = false;
	
	CSmtPtr<CRefCounted> *p_entry = mp_smart_ptr_list;
	while(p_entry)
	{
		Dbg_Assert(p_entry->mp_object == this);
		if (p_entry->mp_prev_ptr)
			Dbg_Assert(p_entry->mp_prev_ptr->mp_next_ptr == p_entry);
		if (p_entry->mp_next_ptr)
			Dbg_Assert(p_entry->mp_next_ptr->mp_prev_ptr == p_entry);

		if (pPtrToCheckForInclusion == p_entry)
			included = true;

		p_entry = p_entry->mp_next_ptr;
	}

	Dbg_MsgAssert(!pPtrToCheckForInclusion || included, ("smart pointer not in list as expected"));
}




}
