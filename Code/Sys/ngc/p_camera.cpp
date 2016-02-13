/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsCamera															*
 *	Description:																*
 *				Functionality to set up cameras and minipulate them.			*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include <math.h>
#include "p_camera.h"
#include "p_prim.h"
#include "p_gx.h"

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/
#define RwV3dDotProductMacro(a, b)                              \
    ((((( (((a)->x) * ((b)->x))) +                          	\
        ( (((a)->y) * ((b)->y))))) +                        	\
        ( (((a)->z) * ((b)->z)))))                          	\

#define RwV3dScaleMacro(o, a, s)                                \
{                                                               \
    (o)->x = (((a)->x) * ( (s)));                           	\
    (o)->y = (((a)->y) * ( (s)));                           	\
    (o)->z = (((a)->z) * ( (s)));                           	\
}                                                               \

#define RwV3dIncrementScaledMacro(o, a, s)                      \
{                                                               \
    (o)->x += (((a)->x) * ( (s)));                          	\
    (o)->y += (((a)->y) * ( (s)));                          	\
    (o)->z += (((a)->z) * ( (s)));                          	\
}                                                               \

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/
NsVector	movement( 0.0f, 0.0f, 0.0f );
NsVector	position( 0.0f, 0.0f, 0.0f );
NsVector	rotation( 0.0f, 0.0f, 0.0f );
NsMatrix	temp0;
NsMatrix	temp1;
NsMatrix	sceneMat;
NsMatrix	skyMat;

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/
extern PADStatus padData[PAD_MAX_CONTROLLERS]; // game pad state
PADStatus * pPad = &padData[0];

/********************************************************************************
 *																				*
 *	Method:																		*
 *				NsCamera														*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Generates a camera object.										*
 *																				*
 ********************************************************************************/
NsCamera::NsCamera()
{
	// Default border settings.
	orthographic( 0, 0, 640, 448 );

	// Default up vector: y is up.
	m_up.set( 0.0f, 1.0f, 0.0f );

	// Default position.
	m_camera.set( 0.0f, 0.0f, 0.0f );

	// No frame.
	m_pFrame = NULL;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				NsCamera														*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Deletes a camera object.										*
 *																				*
 ********************************************************************************/
NsCamera::~NsCamera()
{
//	if ( m_pFrame ) delete m_pFrame;
}

void NsCamera::pos ( float x, float y, float z )
{
	m_camera.set( x, y, z );
}

void NsCamera::up ( float x, float y, float z )
{
	m_up.set( x, y, z );
}

void NsCamera::lookAt ( float x, float y, float z )
{
	NsVector lookat;

	lookat.set( x, y, z );

	m_matrix.lookAt( &m_camera, &m_up, &lookat );
}

float line_x0 = 0.0f;
float line_y0 = 100.0f;
float line_z0 = 0.0f;
float line_x1 = 0.0f;
float line_y1 = 0.0f;
float line_z1 = 0.0f;

float gPosX = 320;
float gPosY = 240;
float gPosZ = 0;

float gUpX = 0;
float gUpY = 1;
float gUpZ = 0;

float gAtX = 320;
float gAtY = 240;
float gAtZ = 1;

void NsCamera::_setCurrent ( NsMatrix * object )
{
	NsMatrix m;
	NsMatrix m2;

	movement.set( 0.0f, 0.0f, 0.0f );

	// Test pad for rotation.		
//	if ( pPad->button & PAD_BUTTON_X ) {
//		// Move line.
//		if ( ( pPad->stickY > 8 ) || ( pPad->stickY < -8 ) ) {
//			line_z0 -= ((float)pPad->stickY)/256.0f;
//			line_z1 -= ((float)pPad->stickY)/256.0f;
//		}
//		if ( ( pPad->stickX > 8 ) || ( pPad->stickX < -8 ) ) {
//			line_x0 += ((float)pPad->stickX)/256.0f;
//			line_x1 += ((float)pPad->stickX)/256.0f;
//		}
//		if ( pPad->button & PAD_BUTTON_Y ) {
//			line_y0 -= 0.25f;
//			line_y1 -= 0.25f;
//		}
//		if ( pPad->button & PAD_BUTTON_B ) {
//			line_y0 += 0.25f;
//			line_y1 += 0.25f;
//		}
//	} else {
//		// Rotation.
//		if ( ( pPad->stickY > 8 ) || ( pPad->stickY < -8 ) ) {
//			rotation.x = rotation.x - ((float)pPad->stickY)/512.0f;
//		}
//		if ( ( pPad->stickX > 8 ) || ( pPad->stickX < -8 ) ) {
//		    rotation.y = rotation.y + ((float)pPad->stickX)/512.0f;
//		}
//	}
//	// Test pad for movement.
//	if ( ( pPad->substickY > 8 ) || ( pPad->substickY < -8 ) ) {
//		movement.z = -((float)pPad->substickY)/64.0f;
//	}
//	if ( ( pPad->substickX > 8 ) || ( pPad->substickX < -8 ) ) {
//	    movement.x = -((float)-pPad->substickX)/64.0f;
//	}
//	if ( pPad->button & PAD_BUTTON_Y ) movement.y -= 0.5f;
//	if ( pPad->button & PAD_BUTTON_B ) movement.y += 0.5f;

	// Generate rotation matrix.
	temp0.rotateX( rotation.x, NsMatrix_Combine_Replace );
	temp1.rotateY( rotation.y, NsMatrix_Combine_Replace );
    skyMat.cat( temp0, temp1 );

    // Generate movement vector by multiplying the desired movement vector
    // by the inverse of the view matrix.
	temp1.invert( skyMat );
	temp1.multiply( &movement, &movement );
	position.add( movement );

    // Generate the translation matrix, and concatonate the rotation matrix.
	sceneMat.translate( &position, NsMatrix_Combine_Replace );
	sceneMat.cat( skyMat, sceneMat );

//	m.identity();
//	m2.identity();
	m.copy( *object );
	m2.copy( *object );

	m.setRight( 1.0f, 0.0f, 0.0f );
	m.setUp( 0.0f, 1.0f, 0.0f );
	m.setAt( 0.0f, 0.0f, 1.0f );
	m.setPos( -m.getPosX(), -m.getPosY(), -m.getPosZ() );
	m2.setPos( 0.0f, 0.0f, 0.0f );

	m.cat( m2, m );
	m.cat( sceneMat, m );

	if ( m_type == GX_ORTHOGRAPHIC ) {
		m2.copy( m_matrix );
		NsVector vs( m_clip_w / viewx, m_clip_h / viewy, 1.0f );
		m2.scale( &vs, NsMatrix_Combine_Post );
		m_current.cat( m2, m );
	
		GX::SetScissorBoxOffset ( ( 320 - ( m_clip_w / 2 ) ) - m_clip_x, ( 224 - ( m_clip_h / 2 ) ) - m_clip_y );
		GX::SetScissor (320 - ( m_clip_w / 2 ), 224 - ( m_clip_h / 2 ), m_clip_w, m_clip_h );
	} else {
		m_current.cat( m_matrix, m );
		GX::SetScissorBoxOffset ( 0, 0 );
	}

    GX::LoadPosMtxImm( m_current.m_matrix, GX_PNMTX0 );

	// This makes the perspective center be at the center of the viewport.
	//GXSetScissorBoxOffset ( ( 320 - ( m_clip_w / 2 ) ) - m_clip_x, ( 224 - ( m_clip_h / 2 ) ) - m_clip_y );
	//GXSetScissor (320 - ( m_clip_w / 2 ), 224 - ( m_clip_h / 2 ), m_clip_w, m_clip_h );
//	GXSetScissor (0, 0, m_clip_w, 480/*m_clip_h*/ );

	// Previously in viewport.
    GX::SetProjection( m_viewmatrix, m_type );
}

void NsCamera::begin ( void )
{
	if ( m_pFrame ) {
		_setCurrent ( m_pFrame->getModelMatrix() );
	} else {
		NsMatrix mId;
		mId.identity();
		_setCurrent ( &mId );
	}
}

void NsCamera::offset ( float x, float y, float z )
{
	NsMatrix offset;
	NsVector v( x, y, z );
	offset.copy( m_current );
	offset.translate( &v, NsMatrix_Combine_Pre );

    GX::LoadPosMtxImm( offset.m_matrix, GX_PNMTX0 );
}

void NsCamera::end ( void )
{
}

// Previously in viewport.
void NsCamera::perspective ( u32 x, u32 y, u32 width, u32 height )
{
    float left	= -0.080F;
    float top	= 0.060F;
    float znear	= 2.0F;
    float zfar	= 40000.0F;
    
    MTXFrustum ( m_viewmatrix, top, -top, left, -left, znear, zfar );

	m_type = GX_PERSPECTIVE;

	clip ( x, y, width, height );

	// Default matrix - just like PS2 - aaaah.
	pos		( 0, 0, 0 );
	up		( 0, 1, 0 );
	lookAt	( 0, 0, 1 );
}

// Previously in viewport.
void NsCamera::perspective ( float width, float height, float znear, float zfar )
{
    MTXFrustum ( m_viewmatrix, height, -height, -width, width, znear, zfar );

	m_type = GX_PERSPECTIVE;

	clip ( 0, 0, 640, 448 );

	// Default matrix - just like PS2 - aaaah.
	pos		( 0, 0, 0 );
	up		( 0, 1, 0 );
	lookAt	( 0, 0, 1 );
}

void NsCamera::perspective ( float fovy, float aspect )
{
    MTXPerspective ( m_viewmatrix, fovy, aspect, 2.0f, 40000.0f );

	m_type = GX_PERSPECTIVE;

	// Default matrix - just like PS2 - aaaah.
	pos		( 0, 0, 0 );
	up		( 0, 1, 0 );
	lookAt	( 0, 0, 1 );
}

void NsCamera::orthographic ( u32 x, u32 y, u32 width, u32 height )
{
    float left	= -320.0f;
    float top	= 224.0f;		//40.0F;
    float znear	= 1.0F;
    float zfar	= 100000.0F;
    
    MTXOrtho ( m_viewmatrix, top, -top, left, -left, znear, zfar );

	m_type = GX_ORTHOGRAPHIC;

	clip ( x, y, width, height );

	m_matrix.setRight( 1.0f, 0.0f, 0.0f );
	m_matrix.setUp( 0.0f, -1.0f, 0.0f );
	m_matrix.setAt( 0.0f, 0.0f, 1.0f );
	m_matrix.setPosX( -320 );
	m_matrix.setPosY( 224 );
	m_matrix.setPosZ( 0 );

	// Default viewplane size.
	setViewWindow( width, height );
}

void NsCamera::clip ( u32 x, u32 y, u32 width, u32 height )
{
	m_clip_x = x;
	m_clip_y = y;
	m_clip_w = width;
	m_clip_h = height;
}

void NsCamera::setViewWindow( float x, float y )
{
	viewx = x;
	viewy = y;
	view1x = 1.0f / x;
	view1y = 1.0f / y;
}

//	// Generate rotation matrix.
////	rotation.x = 32.0f;
//	rotation.y += 0.5f;
//    MTXRotDeg(temp0, 'x', rotation.x );
//    MTXRotDeg(temp1, 'y', rotation.y );
//    MTXConcat(temp0, temp1, skyMat);
//
//    // Generate movement vector by multiplying the desired movement vector
//    // by the inverse of the view matrix.
//    MTXInverse ( skyMat, temp1 );
//    MTXMultVec(temp1, &movement, &movement );
//    position.x += movement.x;
//    position.y += movement.y;
//    position.z += movement.z;
//
//    // Generate the translation matrix, and concatonate the rotation matrix.
//	MTXTrans (sceneMat, position.x, position.y, position.z );
//    MTXConcat(skyMat, sceneMat, sceneMat);

NsMatrix * NsCamera::getViewMatrix ( void )
{
    float			scale;
    NsVector		vVector;
    NsVector		temp;
    NsVector		cpos;
    NsMatrix	  *	cameraLTM;

    cameraLTM = getFrame()->getModelMatrix();
	cpos.set( cameraLTM->getPosX(), cameraLTM->getPosY(), cameraLTM->getPosZ() );

    /*INDENT OFF */

    /*
     * Builds the concatenation of the following matrices:
     *
     *  [ [ -Right_X,    Up_X,      At_X,    0 ]       re-orient
     *    [ -Right_Y,    Up_Y,      At_Y,    0 ]
     *    [ -Right_Z,    Up_Z,      At_Z,    0 ]
     *    [ pos Right, -(pos Up), -(pos At), 1 ] ]
     *  [ [   1,      0,    0, 0 ]                     offset eye
     *    [   0,      1,    0, 0 ]
     *    [ off_x,  off_y,  1, 0 ]
     *    [ -off_x, -off_y, 0, 1 ] ]
     *  [ [ 0.5 / width,      0,       0, 0 ]          scale for view window
     *    [      0,      0.5 / height, 0, 0 ]
     *    [      0,           0,       1, 0 ]
     *    [      0,           0,       0, 1 ] ]
     *  [ [ 1, 0,  0, 0 ]                              project & flip y
     *    [ 0, -1, 0, 0 ]
     *    [ 0, 0,  1, 1 ]                              DIFFERS FROM PARALLEL
     *    [ 0, 0,  0, 0 ] ]
     *  [ [  1,   0,  0, 0 ]                           xform XY
     *    [  0,   1,  0, 0 ]                           from [-0.5..0.5]^2
     *    [  0,   0,  1, 0 ]                           to   [0..1]^2
     *    [ 0.5, 0.5, 0, 1 ] ]
     */

    /*INDENT ON */

    /* At */
    scale = (view1x * ((float) ((-0.5))));
	vVector.scale( *cameraLTM->getRight(), scale );

    scale = ( 0.5f - (scale * 0.0f) );
	temp.scale( *cameraLTM->getAt(), scale );
	vVector.add( temp );

	m_view.setRight( &vVector );

    m_view.setRightX( vVector.x );
    m_view.setUpX( vVector.y );
    m_view.setAtX( vVector.z );
    m_view.setPosX ( 0.5f - ( scale + cpos.dot( vVector ) ) );

    /* Up */
    scale = ( view1y * -0.5f );
	vVector.scale( *cameraLTM->getUp(), scale );

    scale = ( 0.5f + (scale * 0.0f) );
	temp.scale( *cameraLTM->getAt(), scale );
	vVector.add( temp );

    m_view.setRightY( vVector.x );
    m_view.setUpY( vVector.y );
    m_view.setAtY( vVector.z );
    m_view.setPosY ( 0.5f - ( scale + cpos.dot( vVector ) ) );

    /* At */
    m_view.setRightZ(cameraLTM->getAt()->x );
    m_view.setUpZ( cameraLTM->getAt()->y );
    m_view.setAtZ( cameraLTM->getAt()->z );
    m_view.setPosZ ( -cpos.dot( *cameraLTM->getAt() ) );

	return &m_view;
}