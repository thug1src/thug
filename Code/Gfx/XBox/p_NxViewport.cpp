///////////////////////////////////////////////////////////////////////////////
// p_NxViewport.cpp

#include 	"Gfx/NxViewMan.h"
#include 	"Gfx/Xbox/p_NxViewport.h"

namespace Nx
{

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CViewport

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxViewport::CXboxViewport()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxViewport::CXboxViewport( const Mth::Rect *rect, Gfx::Camera *cam) : CViewport( rect, cam )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxViewport::~CXboxViewport()
{
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float CXboxViewport::plat_transform_to_screen_coord( const Mth::Vector & world_pos, float & screen_pos_x, float & screen_pos_y, ZBufferValue & screen_pos_z )
{
	D3DXVECTOR3	in( world_pos[X], world_pos[Y], world_pos[Z] );
	D3DXVECTOR4	mid;
	D3DXVECTOR3	out;
	D3DXVec3Transform( &mid, &in, (D3DXMATRIX*)&NxXbox::EngineGlobals.view_matrix );

	in.x = mid.x;
	in.y = mid.y;
	in.z = mid.z;
	D3DXVec3TransformCoord( &out, &in, (D3DXMATRIX*)&NxXbox::EngineGlobals.projection_matrix );

	// Convert from homogenous cube (-1.0, 1.0) to pixel coordinates. Have to be careful here, the initial
	// feeling is to use the backbuffer width and height as multipliers, but since this value is being used
	// for text which has it's position re-converted at draw time, we actually want to use the fixed Ps2 screen
	// values here.
	screen_pos_x = (( out.x + 1.0f ) * 0.5f ) * 640.0f;
	screen_pos_y = (( -out.y + 1.0f ) * 0.5f ) * 448.0f;
	
	float scale = -1.0f;
	
	if( out.z < 1.0f )
	{
		// Text is not clipped. Calculate scale factor.
		float d_noscale = Nx::CViewportManager::sGet2DIn3DSpaceNoscaleDistance();
		float s_min		= Nx::CViewportManager::sGet2DIn3DSpaceMinScale();
		float s_max		= Nx::CViewportManager::sGet2DIn3DSpaceMaxScale();
		
		scale = d_noscale / -mid.z;

		// Clamp scale.
		if( scale > s_max )
		{
			scale = s_max;
		}
		else if( scale < s_min )
		{
			scale = s_min;
		}
		
		// The z value is passed in as a uint32, so set the value here based on fixed point 16:16.
		screen_pos_z = (uint32)( out.z * 65536.0f );
	}
		
	// This is the scale factor. Returning a value < 0 will cause the text to be hidden.
	return scale;
}



/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

} // Namespace Nx  			
				
				
