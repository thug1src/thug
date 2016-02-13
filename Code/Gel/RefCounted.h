#ifndef __GEL_REFCOUNTED_H
#define __GEL_REFCOUNTED_H

namespace Obj
{

template<class T> class CSmtPtr;




class CRefCounted : public Spt::Class
{
public:
							CRefCounted();
	virtual					~CRefCounted();

	void					AddSmartPointer(CSmtPtr<CRefCounted> *pSmtPtr);
	void					RemoveSmartPointer(CSmtPtr<CRefCounted> *pSmtPtr);

	void 					debug_validate_smart_pointers(CSmtPtr<CRefCounted> *pPtrToCheckForInclusion);

protected:

	CSmtPtr<CRefCounted> *	mp_smart_ptr_list;
};




}
#endif
