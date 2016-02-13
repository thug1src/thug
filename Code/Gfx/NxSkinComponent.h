//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxSkinComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/21/2001
//****************************************************************************

#ifndef	__GFX_NXSKINCOMPONENT_H__
#define	__GFX_NXSKINCOMPONENT_H__

#include <core/defines.h>

namespace Nx
{

class CSkinComponent
{

public:
    // The basic interface to the skin component
    // this is the machine independent part	
    // machine independent range checking, etc can go here
    CSkinComponent();
	virtual					~CSkinComponent();

public:
	void					SetColor(uint8 r, uint8 g, uint8 b, uint8 a);
	void					SetVisibility(uint32 mask);
	void					SetScale(float scaleFactor);
    void                    ReplaceTexture(char* p_srcFileName, char* p_dstFileName);

protected:
    uint32                  m_componentName;

private:
    // The virtual functions will have a stub implementation
    // in p_nxskincomponent.cpp
	virtual	void			plat_set_color(uint8 r, uint8 g, uint8 b, uint8 a);
	virtual	void			plat_set_visibility(uint32 mask);
    virtual void            plat_set_scale(float scaleFactor);
    virtual void            plat_replace_texture(char* p_srcFileName, char* p_dstFileName);
};

}

#endif // __GFX_NXSKINCOMPONENT_H__

