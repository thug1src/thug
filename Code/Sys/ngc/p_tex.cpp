///********************************************************************************
// *																				*
// *	Module:																		*
// *				Prim															*
// *	Description:																*
// *				Provides functionality for drawing various types of single		*
// *				primitives to the display.										*
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
//#include "p_tex.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//#define NUM_TEX_SLOTS 1
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
//NsTexture::NsTexture()
//{
//}
//
//NsTexture::~NsTexture()
//{
//	if ( m_pImage != &this[1] ) {
//		// Image and palette are separate from header.
//		if ( m_pImage ) delete (unsigned char *)m_pImage;
//	}
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				upload															*
// *	Inputs:																		*
// *				pTex	Pointer to the texture to upload.						*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Uploads the specified texture.									*
// *																				*
// ********************************************************************************/
//void NsTexture::upload ( NsTexture_Wrap wrap )
//{
//	GXTexObj	texObj;
//	GXTlutObj	palObj;
//
//	// Set up texture based on depth.
//	switch ( m_depth ) {
//		case 4:
//			GXInitTexObjCI(	&texObj, m_pImage, m_width, m_height,
//						    GX_TF_C4, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE, GX_TLUT0 );
//			GXInitTlutObj( &palObj, m_pPalette, GX_TL_RGB5A3, 16 );
//			GXLoadTlut ( &palObj, GX_TLUT0 );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// Set this texture as uploaded, and add to our list.
//			m_texID = GX_TEXMAP0;
//			break;
//		case 8:
//			GXInitTexObjCI(	&texObj, m_pImage, m_width, m_height,
//						    GX_TF_C8, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE, GX_TLUT0 );
//			GXInitTlutObj( &palObj, m_pPalette, GX_TL_RGB5A3, 256 );
//			GXLoadTlut ( &palObj, GX_TLUT0 );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// Set this texture as uploaded, and add to our list.
//			m_texID = GX_TEXMAP0;
//			break;
//		case 16:
//			GXInitTexObj(	&texObj, m_pImage, m_width, m_height,
//						    GX_TF_RGB5A3, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// Set this texture as uploaded, and add to our list.
//			m_texID = GX_TEXMAP0;
//			break;
//		case 32:
//			GXInitTexObj(	&texObj, /*genTex32, 64, 64,*/	m_pImage, m_width, m_height,
//						    GX_TF_RGBA8, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// Set this texture as uploaded, and add to our list.
//			m_texID = GX_TEXMAP0;
//			break;
//		default:
//			break;
//	}
////	GXInitTexObjLOD( &pTex->texObj, GX_NEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_TRUE, GX_TRUE, GX_ANISO_4 );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				upload															*
// *	Inputs:																		*
// *				pTex	Pointer to the texture to upload.						*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Uploads the specified texture.									*
// *																				*
// ********************************************************************************/
//void NsTexture::upload ( NsTexture_Wrap wrap, void * force_pImage, unsigned short force_width, unsigned short force_height )
//{
//	GXTexObj	texObj;
//	GXTlutObj	palObj;
//
//	// Set up texture based on depth.
//	switch ( m_depth ) {
//		case 4:
//			GXInitTexObjCI(	&texObj, force_pImage, force_width, force_height,
//						    GX_TF_C4, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE, GX_TLUT0 );
//			GXInitTlutObj( &palObj, m_pPalette, GX_TL_RGB5A3, 16 );
//			GXLoadTlut ( &palObj, GX_TLUT0 );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// Set this texture as uploaded, and add to our list.
//			m_texID = GX_TEXMAP0;
//			break;
//		case 8:
//			GXInitTexObjCI(	&texObj, force_pImage, force_width, force_height,
//						    GX_TF_C8, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE, GX_TLUT0 );
//			GXInitTlutObj( &palObj, m_pPalette, GX_TL_RGB5A3, 256 );
//			GXLoadTlut ( &palObj, GX_TLUT0 );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// Set this texture as uploaded, and add to our list.
//			m_texID = GX_TEXMAP0;
//			break;
//		case 16:
//			GXInitTexObj(	&texObj, force_pImage, force_width, force_height,
//						    GX_TF_RGB5A3, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// If current texture in this slot, flag that it is no longer uploaded.
//			m_texID = GX_TEXMAP0;
//			break;
//		case 32:
//			GXInitTexObj(	&texObj, /*genTex32, 64, 64,*/	force_pImage, force_width, force_height,
//						    GX_TF_RGBA8, (GXTexWrapMode)wrap, (GXTexWrapMode)wrap, GX_FALSE );
//			GXLoadTexObj ( &texObj, GX_TEXMAP0 );
//			// Set this texture as uploaded, and add to our list.
//			m_texID = GX_TEXMAP0;
//			break;
//		default:
//			break;
//	}
////	GXInitTexObjLOD( &pTex->texObj, GX_NEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_TRUE, GX_TRUE, GX_ANISO_4 );
//}




