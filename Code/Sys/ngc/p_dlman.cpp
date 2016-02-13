///********************************************************************************
// *																				*
// *	Module:																		*
// *				NsDLMan															*
// *	Description:																*
// *				Manages a buffer for building display lists to.					*
// *	Written by:																	*
// *				Paul Robinson													*
// *	Copyright:																	*
// *				2001 Neversoft Entertainment - All rights reserved.				*
// *																				*
// ********************************************************************************/
//
///********************************************************************************
// * Includes.																	*
// ********************************************************************************/
//#include <math.h>
//#include "p_dlman.h"
//#include "p_assert.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//#define DEFAULT_DLMAN_SIZE (1024*1024*2)		// 2mb default.
//
///********************************************************************************
// * Structures.																	*
// ********************************************************************************/
//
///********************************************************************************
// * Local variables.																*
// ********************************************************************************/
//
///********************************************************************************
// * Forward references.															*
// ********************************************************************************/
//
///********************************************************************************
// * Externs.																		*
// ********************************************************************************/
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				NsDLMan															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Instances a Display List manager to the default Display List	*
// *				size of 2mb.													*
// *																				*
// ********************************************************************************/
//NsDLMan::NsDLMan()
//{
//	NsDLMan ( DEFAULT_DLMAN_SIZE );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				TexMan															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Instances a Display List manager to a user-specified buffer		*
// *				size.															*
// *																				*
// ********************************************************************************/
//NsDLMan::NsDLMan ( int size )
//{
//	char  * p;
//
//	m_pDLBufferCurrent = m_pDLBufferStart = new unsigned char[size];
//
//	m_bufferSize = size;
//	p = (char *)m_pDLBufferStart;
//	m_pDLBufferEnd = (void *)&p[size];
//}
//
//NsDL * NsDLMan::open ( void )
//{
//	return (NsDL *)(OSRoundUp32B(m_pDLBufferCurrent));
//}
//
//void NsDLMan::close ( NsDL * pDL )
//{
//	assert ( pDL <= m_pDLBufferEnd );
//
//	DCFlushRange ( m_pDLBufferCurrent, (OSRoundUp32B(pDL)) - (OSRoundUp32B(m_pDLBufferCurrent)) );
//
//	m_pDLBufferCurrent = (void *)(OSRoundUp32B(pDL));
//}
//
//void NsDLMan::close ( unsigned int size )
//{
//	char  * p8;
//
//	p8 = (char *)(OSRoundUp32B(m_pDLBufferCurrent));
//	p8 = &p8[(OSRoundUp32B(size))];
//
//	assert ( (void *)p8 <= m_pDLBufferEnd );
//
//	DCFlushRange ( m_pDLBufferCurrent, (OSRoundUp32B(size)) );
//
//	m_pDLBufferCurrent = (void *)p8;
//}
//
//unsigned int NsDLMan::freeSpace ( void )
//{
//	return (unsigned int)m_pDLBufferEnd - (unsigned int)m_pDLBufferCurrent;
//}



