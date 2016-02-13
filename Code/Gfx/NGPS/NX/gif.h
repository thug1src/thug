#ifndef __GIF_H
#define __GIF_H


namespace NxPs2
{


// GIFtag FLG definitions
#define PACKED	0
#define REGLIST	1
#define IMAGE	2

class gif
{

public:

	//------------------------------------------
	//		S T A T I C   F U N C T I O N S
	//------------------------------------------

	static void Tag1(uint32 Regs, uint NReg, uint Flg, uint Prim, uint Pre, uint Eop, uint NLoop, uint Addr);
	static void EndTag1(uint Eop);
	static void BeginTag1(uint32 Regs, uint NReg, uint Flg, uint Prim, uint Pre, uint Addr);
	static void BeginTag1_extended(uint32 Regs, uint NReg, uint Flg, uint Prim, uint Pre, uint Addr, uint Step);
	static void EndTagImmediate(uint Eop);
	static void BeginTagImmediate(uint32 Regs, uint NReg, uint Flg, uint Prim, uint Pre, uint Addr);
	static void Tag2(uint32 Regs, uint NReg, uint Flg, uint Prim, uint Pre, uint Eop, uint NLoop);
	static void EndTag2(uint Eop);
	static void BeginTag2(uint32 Regs, uint NReg, uint Flg, uint Prim, uint Pre);

	//--------------------------------
	//		S T A T I C   D A T A
	//--------------------------------

	static uint8 *pTag;

}; // class gif


} // namespace NxPs2


#endif // __GIF_H
