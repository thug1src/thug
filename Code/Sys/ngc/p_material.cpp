///********************************************************************************
// *																				*
// *	Module:																		*
// *				Material														*
// *	Description:																*
// *				A Material.														*
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
//#include "p_material.h"
//#include "p_model.h"
//#include "p_clump.h"
//#include "p_scene.h"
//#include "p_light.h"
//#include <gfx/gfxman.h>
//#include <gfx/nxflags.h>
//#include "p_display.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//#define rpWORLDUVS			( rpWORLDTEXTURED2 | rpWORLDTEXTURED )
//#define MAX_VC_WIBBLE_VERTS	2048
//static const float pi_over_180 = 3.1415926535f / 180.0f;
//
//// Treats the VC wibbling as a multiplier from the original PreLitLum, rather than replacing it outright.
//#define NS_SECTOR_FLAGS_VC_PRESERVE_PRELIT	0x400
//
//// whether the VC wibbling needs to be continually refreshed (if static, then VC doesn't get refreshed unless the mesh's dirty flag is set)
//#define NS_SECTOR_FLAGS_VC_STATIC			0x800
//
//enum
//{
//	mTWO_SIDED			= 0x01,
//	mINVISIBLE			= 0x02,
//	mFRONT_FACING		= 0x04,
//	mTRANSPARENT		= 0x08,
//	mDECAL				= 0x10,
//	mUV_WIBBLE_SUPPORT	= 0x20,	// If these two values change, notify Steve as Rw references 
//	mVC_WIBBLE_SUPPORT	= 0x40, // these two flags by value.
//};
//
//enum RpWorldFlag
//{
//    rpWORLDTRISTRIP = 0x01,     /**<This world's meshes can be rendered
//                                   as tri strips */
//    rpWORLDTEXTURED = 0x04,     /**<This world has one set of texture coordinates */
//    rpWORLDPRELIT = 0x08,       /**<This world has luminance values */
//    rpWORLDNORMALS = 0x10,      /**<This world has normals */
//    rpWORLDLIGHT = 0x20,        /**<This world will be lit */
//    rpWORLDMODULATEMATERIALCOLOR = 0x40, /**<Modulate material color with
//                                            vertex colors (pre-lit + lit) */
//    rpWORLDTEXTURED2 = 0x80, /**<This world has 2 set of texture coordinates */
//
//    /*
//     * These flags are stored in the flags field in an RwObject, the flag field is 8bit.
//     */
//
//    rpWORLDSECTORSOVERLAP = 0x40000000,
//};
//typedef enum RpWorldFlag RpWorldFlag;
//
//enum
//{
//	MAX_NUM_SEQUENCES	= 8
//};
//
///********************************************************************************
// * Structures.																	*
// ********************************************************************************/
//
///********************************************************************************
// * Local variables.																*
// ********************************************************************************/
//
//static NsMaterial* lastUVWibbleMaterial = NULL;
//
///********************************************************************************
// * Forward references.															*
// ********************************************************************************/
//
///********************************************************************************
// * Externs.																		*
// ********************************************************************************/
//
//float env_map_cam_u_factor = 0.0f;
//float env_map_cam_v_factor = 0.0f;
//
//void NsMaterial::deleteDLs( void )
//{
////	NsDL	  * pDL;
////	NsDL	  * pKillDL;
////	int			count = 0;
////
////	NsDisplay::flush();
////	
////	// Remove pools from display lists.
//// 	pDL = m_pDL;
////	while ( pDL ) {
////		pKillDL = pDL;
////		pDL = pDL->m_pNext;
////
//////		if ( pKillDL->m_pParent->m_pVertexPool ) {
//////			delete pKillDL->m_pParent->m_pVertexPool;
//////			pKillDL->m_pParent->m_pVertexPool = NULL;
//////		}
////		if ( pKillDL->m_pParent->m_pFaceFlags ) {
////			delete pKillDL->m_pParent->m_pFaceFlags;
////			pKillDL->m_pParent->m_pFaceFlags = NULL;
////		}
////		if ( pKillDL->m_pParent->m_pFaceMaterial ) {
////			delete pKillDL->m_pParent->m_pFaceMaterial;
////			pKillDL->m_pParent->m_pFaceMaterial = NULL;
////		}
////	}
////	// Remove all display lists.
//// 	pDL = m_pDL;
////	while ( pDL ) {
////		pKillDL = pDL;
////		pDL = pDL->m_pNext;
////		delete pKillDL;
////		count++;
////	}
////
////	OSReport( "Killed &%d display lists\n", count );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				init															*
// *	Inputs:																		*
// *				number	The material number to create.							*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Creates a material object.										*
// *																				*
// ********************************************************************************/
//void NsMaterial::init(unsigned int number)
//{
//	// Reset material to blank.
//	m_id			=  *((unsigned int *)"GCMT");
//	m_version		= 0;
//	m_number		= number;
//	m_pTexture	= NULL;
//	m_nDL			= 0;
//	m_pDL			= NULL;
//	m_priority	= 0;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				addDL															*
// *	Inputs:																		*
// *				pDL	The display list to add.									*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Adds a display list to this material.							*
// *																				*
// ********************************************************************************/
//void NsMaterial::addDL ( NsDL * pDLToAdd )
//{
//	// Make this DL point to the current DL.
//	pDLToAdd->m_pNext = m_pDL;
//	// Point the material at this DL.
//	m_pDL = pDLToAdd;
//	// Increment number of DLs.
//	m_nDL++;
//}
//
//NsBlendMode	gBlendMode = (NsBlendMode)-1;
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				draw															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Draws all display lists assigned to this material.				*
// *																				*
// ********************************************************************************/
//void NsMaterial::draw ( void )
//{
//	NsDL	  * pDLToDraw;
//	GXLightID	light_id;
//	NsZMode		z_mode;
//	int			z_compare;
//	int			z_update;
//	int			z_reject_before;
//
//	if ( m_flags & mINVISIBLE ) return;
//	if( !m_pDL ) return;
//
//	// See if we're actually going to draw something.
////	int we_want_to_draw_something = 0;
//// 	pDLToDraw = m_pDL;
////	while ( pDLToDraw ) {
////		if (	(pDLToDraw->m_pParent->m_cull.visible) &&
////				!(pDLToDraw->m_pParent->m_rwflags & (mSD_INVISIBLE|mSD_KILLED|mSD_NON_COLLIDABLE)) ) {
////			we_want_to_draw_something = 1;
////			break;
////		}
////		pDLToDraw = pDLToDraw->m_pNext;
////	}
////	if ( !we_want_to_draw_something ) return;
//
//	// Set default texture generation matrix.
//	GXSetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_TEXCOORD0, GX_IDENTITY );
//
//	if ( m_blendMode == gBlendMode ) {
//		return;
//	}
//
////	if ( m_pTexture ) {
////		if ( strcmp( m_pTexture->m_name, "cw_ss_floor_decal_32.png" ) == 0 ) {
////			OSReport( "Suspect material: %s\n", m_pTexture->m_name );
////			OSReport( "Dimensions: %dx%d (%d bit)\n", m_pTexture->m_width, m_pTexture->m_height, m_pTexture->m_depth );
//////			{
//////				unsigned char * p8 = (unsigned char *)m_pTexture->m_pImage;
//////			}
////			{
////				unsigned short * p16 = (unsigned short *)m_pTexture->m_pPalette;
////				for ( int lp = 0; lp < 16; lp++ ) {
////					OSReport( "Palette %2d: 0x%04x\n", lp, p16[lp] );
////				}
////			}
////		}
////	}
//
////	OSReport ( "Draw material: %d %s\n", number, pTexture ? pTexture->name : "<no texture" );
//
//	// Clear material flag for uv wibble.
//	lastUVWibbleMaterial = NULL;
//
//	// Amend z-buffer state for decal materials.
//	if( m_flags & mDECAL )
//	{
//		NsRender::getZMode( &z_mode, &z_compare, &z_update, &z_reject_before );
//		NsRender::setZMode( z_mode, z_compare, 0, z_reject_before );
//	}
//
//	// Determine whether lighting is required, based on whether the geometry contains normals.
//	if( m_pDL->m_pParent->m_flags & rpWORLDNORMALS )
//	{
//		light_id = NsLight::getLightID();
//	}
//	else
//	{
//		light_id = GX_LIGHT_NULL;
//	}
//
//	GX::SetChanCtrl(
//        GX_ALPHA0,       // color channel 0
//        GX_ENABLE,       // enable channel (use lighting)
//        GX_SRC_VTX,      // use the register as ambient color source
//        GX_SRC_REG,      // use the register as material color source
//        GX_LIGHT_NULL,	 // use GX_LIGHT0, GX_LIGHT1 etc.
//        GX_DF_CLAMP,     // diffuse function (CLAMP = normal setting)
//        GX_AF_NONE );    // attenuation function (this case doesn't use)
//	// Set alpha operation to scale colors by 2 to match PS2 hardware.
//	GXSetTevAlphaOp(
//	    GX_TEVSTAGE0,
//	    GX_TEV_ADD,		// For modulation, this is irrelevent.
//	    GX_TB_ZERO,
//	    GX_CS_SCALE_2,	// This is the bit that makes it like a PS2.
//	    GX_ENABLE,
//	    GX_TEVPREV );
//	GXSetAlphaCompare(	// This means only pass the alpha if it is > 0.
//	    GX_GREATER,		// Which allows cut-out shapes to work, even when
//	    0,				// rendered out of order.
//	    GX_AOP_AND,
//	    GX_GREATER,
//	    0 );
//	GXSetTevColorOp(
//	    GX_TEVSTAGE0,
//	    GX_TEV_ADD,		// For modulation, this is irrelevent.
//	    GX_TB_ZERO,
//	    GX_CS_SCALE_2,	// This is the bit that makes it like a PS2.
//	    GX_ENABLE,
//	    GX_TEVPREV );
//
//	// Set color control to combine material color with vertex colors.
//	if( light_id == GX_LIGHT_NULL )
//	{
//		GX::SetChanCtrl(
//			GX_COLOR0,       // color channel 0
//			GX_ENABLE,       // enable channel (use lighting)
//			GX_SRC_VTX,      // use the register as ambient color source
//			GX_SRC_REG,      // use the register as material color source
//			light_id,		 // use GX_LIGHT0, GX_LIGHT1 etc.
//			GX_DF_NONE,		 // diffuse function (this case doesn't use)
//			GX_AF_NONE );    // attenuation function (this case doesn't use)
//	}
//	else
//	{
//		GX::SetChanCtrl(
//			GX_COLOR0,       // color channel 0
//			GX_ENABLE,       // enable channel (use lighting)
//			GX_SRC_REG,      // use the register as ambient color source
//			GX_SRC_REG,      // use the register as material color source
//			light_id,		 // use GX_LIGHT0, GX_LIGHT1 etc.
//			GX_DF_CLAMP,     // diffuse function (CLAMP = normal setting)
//			GX_AF_NONE );    // attenuation function (this case doesn't use)
//
//			// The primitive code may have changed the ambient register, so ensure it is set here.
//			NsLight::loadupAmbient();
//	}
//
//	GXSetNumTexGens(1);
//	GXSetNumTevStages(1);
//
//	// We don't deal with untextured materials.
////	if ( !pMat->pTexture ) return;
//
///*	GX::SetChanCtrl(
//        GX_ALPHA0,       // color channel 0
//        GX_ENABLE,       // enable channel (use lighting)
//        GX_SRC_REG,      // use the register as ambient color source
//        GX_SRC_VTX,      // use the register as material color source
//        GX_LIGHT_NULL,   // use GX_LIGHT0
//        GX_DF_CLAMP,     // diffuse function (CLAMP = normal setting)
//        GX_AF_NONE );    // attenuation function (this case doesn't use)
//	// Set alpha operation to scale colors by 2 to match PS2 hardware.
//	GXSetTevAlphaOp(
//	    GX_TEVSTAGE0,
//	    GX_TEV_ADD,		// For modulation, this is irrelevent.
//	    GX_TB_ZERO,
//	    GX_CS_SCALE_2,	// This is the bit that makes it like a PS2.
//	    GX_ENABLE,
//	    GX_TEVPREV );
//	GXSetAlphaCompare(	// This means only pass the alpha if it is > 0.
//	    GX_GREATER,		// Which allows cut-out shapes to work, even when
//	    0,				// rendered out of order.
//	    GX_AOP_AND,
//	    GX_GREATER,
//	    0 );
//*/
//    // Set format of position, color and tex coordinates.
///*    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
//	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);*/
//	
//	// Lighting channel control
///*	GXSetNumChans(1);    // number of active color channels
//
//	// Sets up vertex descriptors
//	GXClearVtxDesc();
//	GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
//	GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
//   	GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
//	//GXSetVtxDesc(GX_VA_NRM, GX_INDEX8);*/
//
//	// Upload texture if required.
////	if ( !pMat->pTexture->uploaded ) {
////		TexMan_Upload ( pTex );
////	}
////	// Set hardware to this texture.
////	GXSetNumTexGens(1);
////	GXSetNumTevStages(1);
////	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, /*pTex->texID*/GX_TEXMAP0, GX_COLOR0A0);
//
//	GX::SetChanMatColor( GX_COLOR0A0, m_color );
//
//	// Textured blend modes.
//	if ( m_pTexture ) {
//		if ( !m_pTexture->m_uploaded ) {
//			m_pTexture->upload( m_wrap );
//		}
//	}
//
//	// Draw all display lists for this material.
// 	pDLToDraw = m_pDL;
//	while ( pDLToDraw ) {
////		if ( (pDLToDraw->cull.visible) && !(pDLToDraw->rwflags & mSD_INVISIBLE)  ) {
////		if ( (pDLToDraw->cull.visible) && ((pDLToDraw->pParent->rwflags & (mSD_INVISIBLE|mSD_KILLED|mSD_NON_COLLIDABLE)) == 0) ) {
//		if (	(pDLToDraw->m_pParent->m_cull.visible) &&
//				!(pDLToDraw->m_pParent->m_rwflags & (mSD_INVISIBLE|mSD_KILLED|mSD_NON_COLLIDABLE)) ) {
//			if ( ( pDLToDraw->m_flags & rpWORLDUVS ) && m_pTexture ) {
//				NsRender::setBlendMode ( m_blendMode, m_pTexture, m_alpha );
//			} else {
//				NsRender::setBlendMode ( m_blendMode, NULL, m_alpha );
//			}
////			// If model is straddling z, turn on clipping.
////			if ( pDLToDraw->cull.clipCodeOR ) {
////				GXSetClipMode(GX_CLIP_ENABLE);
////			} else {
////				GXSetClipMode(GX_CLIP_DISABLE);
////			}
//
////			if ( pDLToDraw->m_vpoolFlags & (1<<5) ) {
//				// It's a tri-strip.
//				NsRender::setCullMode( NsCullMode_Never );
//				//GXSetCullMode ( GX_CULL_BACK );
////			} else {
////				// It's a tri-list.
////				NsRender::setCullMode( NsCullMode_Back );
////			}
//
//			if( m_flags & mUV_WIBBLE_SUPPORT )
//			{
//				wibbleUV( pDLToDraw );
//			}
//			if( m_flags & mVC_WIBBLE_SUPPORT )
//			{
////				OSReport( "vc wibble: %x\n", pDLToDraw );
//				wibbleVC( pDLToDraw );
//			}
//
//			if ( pDLToDraw->m_size ) GXCallDisplayList ( &pDLToDraw[1], pDLToDraw->m_size );
//		}
//		pDLToDraw = pDLToDraw->m_pNext;
//	}
//
//	// Restore z-buffer state to previous.
//	if( m_flags & mDECAL )
//	{
//		NsRender::setZMode( z_mode, z_compare, z_update, z_reject_before );
//	}
//
//	// Clear dirty flag for all display lists for this material.
// 	pDLToDraw = m_pDL;
//	while ( pDLToDraw ) {
//		pDLToDraw->m_pParent->m_flags &= ~0x200;
//		pDLToDraw = pDLToDraw->m_pNext;
//	}
//}
//
//
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				getVCWibbleParameters											*
// *	Inputs:																		*
// *																				*
// *	Output:																		*
// *																				*
// *	Description:																*
// *																				*
// *																				*
// ********************************************************************************/
//GXColor* NsMaterial::getVCWibbleParameters( NsDL* p_dl, char** change_mask )
//{
////	MaterialPluginData* mat_plugin_data;
////	WorldSectorPluginData* ws_plugin_data;
//
////	mat_plugin_data = GetMaterialPluginData( material );
////	if( mat_plugin_data->m_pWibble == NULL )
////	{
////		return NULL;
////	}
//	if( p_dl->m_pParent->getWibbleColor() == NULL )
//	{
//		return NULL;
//	}
//
////	ws_plugin_data = GetWorldSectorPluginData( sector );
////	if( ( ws_plugin_data->m_pWibbleSeqIndices == NULL ) || ( ws_plugin_data->m_pWibbleSeqOffsets == NULL ))
////	{
////		return NULL;
////	}
//
//	Spt::SingletonPtr< Gfx::Manager > gfx_man;
////	static RwRGBA		s_new_colors[MAX_VC_WIBBLE_VERTS];
//	static GXColor		s_new_colors[MAX_VC_WIBBLE_VERTS];
//	static char			s_change_mask[MAX_VC_WIBBLE_VERTS];
//	int					i, j, seq_num, offset, bounded_time;
////	WibbleSequenceData*	sequence;
//	NsWibbleSequence*	sequence;
//	Tmr::Time			time, last_time;
//	
//	time = gfx_man->GetGfxTime();
////	Dbg_MsgAssert( MAX_VC_WIBBLE_VERTS >= sector->numVertices, ("Fire somebody -- too many vertices (%d) in sector (max %d).", sector->numVertices, MAX_VC_WIBBLE_VERTS ) );
//	Dbg_Assert( MAX_VC_WIBBLE_VERTS >= p_dl->m_numVertex );
//
//	for( i = p_dl->m_vertBase; i < ( p_dl->m_vertBase + p_dl->m_vertRange ); ++i )
//	{
//		int num_frames;
//	
////		seq_num = ws_plugin_data->m_pWibbleSeqIndices[i];
//		seq_num = (int)((char)( p_dl->m_pParent->getWibbleIdx()[i] ));
//			
//		Dbg_MsgAssert( seq_num >= -1 && seq_num <= MAX_NUM_SEQUENCES, ( "Out of range seq_num %d", seq_num ));
//
//		// A sequence of -1 signifies that we should restore the original vertex color.
//		if( seq_num == -1 )
//		{
//			// Set the flag to refresh the color pool entry.
//			// Xbox needs to differentiate between setting the original vertex color, and setting a wibbled color.
//			// This is because we are writing these values back to the instanced data, which expects the vertex
//			// color to have already been modulated by the material color, where appropriate.
//			s_change_mask[i] = 2;
//				
//			// Restore the original vertex color.
//			s_new_colors[i].r = ( p_dl->m_pParent->getWibbleColor()[i] ) & 0xFF;
//			s_new_colors[i].g = ( p_dl->m_pParent->getWibbleColor()[i] >> 8 ) & 0xFF;
//			s_new_colors[i].b = ( p_dl->m_pParent->getWibbleColor()[i] >> 16 ) & 0xFF;
//
//			// As a test, just make it green.
////			s_new_colors[i].r = 0;
////			s_new_colors[i].g = 255;
////			s_new_colors[i].b = 0;
//
//			// Clear the seq index so that we don't wibble on the next frame...
////			ws_plugin_data->m_pWibbleSeqIndices[i] = 0;
//				
//			// If we clear to 0 now, then the rest of the meshes in this world sector will not get colored.  so...
//			// leave it at -1...  you might think this would slow things down to have to go through the vertex
//			// list over and over, but the "dirty flag" already reduces the number of times that this function
//			// gets called...
//			continue;
//		}
//
//		// A sequence of 0 signifies no wibble intentions.
//		else if( seq_num == 0 )
//		{
//			s_change_mask[i] = 0;
//			continue;
//		}
//		else
//		{
//			s_change_mask[i] = 1;
//		}
//
////		sequence	= &mat_plugin_data->m_pWibble->m_pSequences[seq_num - 1];	// sequences are 1-based
//		sequence	= m_pWibbleData + seq_num - 1;	// sequences are 1-based
//		num_frames	= sequence->numKeys;
//
//		// If a vert has only one frame of anim, don't wibble it. The artist should have just used vertex colors instead of vertex wibble.
//		if( num_frames <= 1 )
//		{   
//			s_change_mask[i] = 0;
//			continue;
//		}
//			
////		offset = ws_plugin_data->m_pWibbleSeqOffsets[i];
//		offset = p_dl->m_pParent->getWibbleOff()[i];
//
//		// Range check.
////		Dbg_MsgAssert( offset >= 0 && offset < sequence->m_NumKeyframes,( "VC wibbling offset (%d) out of range (0<=x<%d) for %s", offset, sequence->m_NumKeyframes, Script::FindChecksumName( ws_plugin_data->m_TriggerNode )));
//		Dbg_MsgAssert( offset >= 0 && offset < sequence->numKeys,( "VC wibbling offset (%d) out of range (0<=x<%d)", offset, sequence->numKeys ));
//
////		last_time = sequence->m_Keyframes[num_frames - 1].m_Time;
//		last_time = sequence->pKeys[num_frames - 1].time;
////		Dbg_MsgAssert( last_time, ( "Trying to mod by 0 in world sector %s", Script::FindChecksumName(ws_plugin_data->m_TriggerNode )));
//		Dbg_MsgAssert( last_time, ( "Trying to mod by 0 in world sector" ));
//
////		bounded_time = (int)( time + sequence->m_Keyframes[offset].m_Time ) % last_time;
//		bounded_time = (int)( time + sequence->pKeys[offset].time ) % last_time;
//		for( j = num_frames - 1; j >= 0; j-- )
//		{
////			if( bounded_time >= sequence->m_Keyframes[j].m_Time )
//			if( bounded_time >= (int)sequence->pKeys[j].time )
//			{
////				float ratio =	(float)( bounded_time - sequence->m_Keyframes[j].m_Time ) / (float)( sequence->m_Keyframes[j + 1].m_Time - sequence->m_Keyframes[j].m_Time );
//				float ratio =	(float)( bounded_time - sequence->pKeys[j].time ) / (float)( sequence->pKeys[j + 1].time - sequence->pKeys[j].time );
//	
////				s_new_colors[i].r	= (unsigned char)( sequence->m_Keyframes[j].m_Color.r + ( ratio * ( sequence->m_Keyframes[j+1].m_Color.r - sequence->m_Keyframes[j].m_Color.r )));
//				s_new_colors[i].r	= (unsigned char)( sequence->pKeys[j].color.r + ( ratio * ( sequence->pKeys[j+1].color.r - sequence->pKeys[j].color.r )));
////				s_new_colors[i].g	= (unsigned char)( sequence->m_Keyframes[j].m_Color.g + ( ratio * ( sequence->m_Keyframes[j+1].m_Color.g - sequence->m_Keyframes[j].m_Color.g )));
//				s_new_colors[i].g	= (unsigned char)( sequence->pKeys[j].color.g + ( ratio * ( sequence->pKeys[j+1].color.g - sequence->pKeys[j].color.g )));
////				s_new_colors[i].b	= (unsigned char)( sequence->m_Keyframes[j].m_Color.b + ( ratio * ( sequence->m_Keyframes[j+1].m_Color.b - sequence->m_Keyframes[j].m_Color.b )));
//				s_new_colors[i].b	= (unsigned char)( sequence->pKeys[j].color.b + ( ratio * ( sequence->pKeys[j+1].color.b - sequence->pKeys[j].color.b )));
//
////				if( sector->flags & NS_SECTOR_FLAGS_VC_PRESERVE_PRELIT )
//				if( p_dl->m_pParent->m_flags & NS_SECTOR_FLAGS_VC_PRESERVE_PRELIT )
//				{
//					// Scale the colors by the original pre-lit colors.
//					s_new_colors[i].r = (unsigned char)((( p_dl->m_pParent->getWibbleColor()[i] & 0xFF ) * s_new_colors[i].r ) >> 8 );
//					s_new_colors[i].g = (unsigned char)(((( p_dl->m_pParent->getWibbleColor()[i] >> 8 ) & 0xFF ) * s_new_colors[i].g ) >> 8 );
//					s_new_colors[i].b = (unsigned char)(((( p_dl->m_pParent->getWibbleColor()[i] >> 16 ) & 0xFF ) * s_new_colors[i].b ) >> 8 );
//				}
//				break;
//			}								 
//		}
//	}
//	*change_mask = s_change_mask;
//	return s_new_colors;
//}
//
//
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				wibbleVC														*
// *	Inputs:																		*
// *																				*
// *	Output:																		*
// *																				*
// *	Description:																*
// *																				*
// *																				*
// ********************************************************************************/
//bool NsMaterial::wibbleVC( NsDL* p_dl )
//{
//	char*			change_mask;
//
//	// These next two MUST be signed or they'll wrap around causing infinite loops!!
//	int				i;
//	GXColor*		new_colors;
//
//	// Dirty flag optimization.
//	bool			vc_mesh_is_dirty;
//
//	// Check the dirty flag to see whether we need to continue.
////	vc_mesh_is_dirty = ( !( p_dl->m_pParent->m_flags & NS_SECTOR_FLAGS_VC_STATIC )) || ( vc_mesh_header_dirty );
//	vc_mesh_is_dirty = ( !( p_dl->m_pParent->m_flags & NS_SECTOR_FLAGS_VC_STATIC )) || ( p_dl->m_pParent->m_flags & 0x200 );
//	if( !vc_mesh_is_dirty )
//	{
//		return false;
//	}
//
//	new_colors = getVCWibbleParameters( p_dl, &change_mask );
//	if( new_colors == NULL )
//	{
//		return false;
//	}
//
//	// Basic algorithm appears to be (inefficient!!) scan through all *indices*, updating vertex buffer with color information.
//	for( i = p_dl->m_vertBase; i < ( p_dl->m_vertBase + p_dl->m_vertRange ); ++i )
//	{
//		if( change_mask[i] )
//		{
//			// Have to be careful here. Although we are getting a RwRGBA pointer, the actual format of the
//			// instanced color data is (bgra), so we need to switch red and blue when writing to the locked
//			// vertex buffer.
//			GXColor* p_color = (GXColor*)( p_dl->m_pParent->getColorPool() + i );
//
//			if( change_mask[i] == 1 )
//			{
//				// Setting a wibbled color, so don't worry about material color modulation.
//				p_color->b	= new_colors[i].r;
//				p_color->g	= new_colors[i].g;
//				p_color->r	= new_colors[i].b;
//			}
//			else
//			{
//				// Restoring an 'original' color, so consider material color modulation.
////				if( flags & rxGEOMETRY_MODULATE )
////				{
////					p_color->blue	= new_colors[i].red * colorScale.red;
////					p_color->green	= new_colors[i].green * colorScale.green;
////					p_color->red	= new_colors[i].blue * colorScale.blue;
////				}
////				else
////				{
//					p_color->b	= new_colors[i].r;
//					p_color->g	= new_colors[i].g;
//					p_color->r	= new_colors[i].b;
////				}
//			}
//		}
//	}
//	return true;
//}
//
//
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				getUVWibbleParameters											*
// *	Inputs:																		*
// *																				*
// *	Output:																		*
// *																				*
// *	Description:																*
// *																				*
// *																				*
// ********************************************************************************/
//bool NsMaterial::getUVWibbleParameters( float* u_offset, float* v_offset, int pass )
//{
//	float uoff, voff;
//
//	// Need to decide whether to use time based, or camera based, wibble.
//	if( m_UVWibbleEnabled & 0x80 )
//	{
//		uoff = env_map_cam_u_factor * m_uvel;
//		voff = env_map_cam_v_factor * m_vvel;
//	}
//	else
//	{
//		float elapsed_seconds, vel, amp, phase, freq;
//		int whole_offset;
//		Spt::SingletonPtr< Gfx::Manager > gfx_man;
//		elapsed_seconds = (float)gfx_man->GetGfxTime() * 0.001f;
//
//		vel		= m_uvel;
//		amp		= m_uamp;
//		phase	= m_uphase;
//		freq	= m_ufreq;
//		uoff	= elapsed_seconds * vel + ( amp * sinf( pi_over_180 * ( freq * (( elapsed_seconds + phase ) * 360.0f ))));
//
//		if( fabs( uoff ) > 1.0f )
//		{
//			whole_offset = (int)uoff;
//			uoff -= whole_offset;
//		}
//	
//		vel		= m_vvel;
//		amp		= m_vamp;
//		phase	= m_vphase;
//		freq	= m_vfreq;
//		voff	= elapsed_seconds * vel + ( amp * sinf( pi_over_180 * ( freq * (( elapsed_seconds + phase ) * 360.0f ))));
//		
//		if( fabs( voff ) > 1.0f )
//		{
//			whole_offset = (int)voff;
//			voff -= whole_offset;
//		}
//	}
//
//	*u_offset = uoff;
//	*v_offset = voff;
//
//	return true;
//}
//
//
//
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				wibbleUV														*
// *	Inputs:																		*
// *																				*
// *	Output:																		*
// *																				*
// *	Description:																*
// *																				*
// *																				*
// ********************************************************************************/
//bool NsMaterial::wibbleUV( NsDL* p_dl )
//{
//	static float	u_offset[2]	= { 0.0f, 0.0f };
//	static float	v_offset[2]	= { 0.0f, 0.0f };
//	static bool		do_wibble[2];
//
//	if( this != lastUVWibbleMaterial )
//	{
//		lastUVWibbleMaterial = this;
//
//		do_wibble[0] = getUVWibbleParameters( &u_offset[0], &v_offset[0], 0 );
//	}
//
////	if( !( do_wibble[0] || do_wibble[1] ))
//	if( !do_wibble[0] )
//	{
//		return false;
//	}
//	
//	// Set up the texture generation matrix here.
//	Mtx	m;
//	MTXTrans( m, u_offset[0], v_offset[0], 0.0f );
//	GX::LoadTexMtxImm( m, GX_TEXMTX0, GX_MTX2x4 );
//	GXSetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEXCOORD0, GX_TEXMTX0 );
//	return true;
//}







