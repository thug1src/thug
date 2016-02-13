///////////////////////////////////////////////////////////////////////////////
// FaceMassage.cpp

#include <core/math.h>

#include <gel/Scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>

#include <gfx/NxTexture.h>
#include <gfx/NxTexMan.h>
#include <gfx/FaceMassage.h>

namespace	Nx
{

#ifdef __NOPT_ASSERT__
void			SFacePoints::PrintData()
{
	Dbg_Message("FacePoints left_eye (%d, %d)", m_left_eye[X], m_left_eye[Y]);
	Dbg_Message("FacePoints right_eye (%d, %d)", m_right_eye[X], m_right_eye[Y]);
	Dbg_Message("FacePoints nose (%d, %d)", m_nose[X], m_nose[Y]);
	Dbg_Message("FacePoints lips (%d, %d)", m_lips[X], m_lips[Y]);
	Dbg_Message("FacePoints adjust HSV (%s)", m_adjust_hsv ? "true" : "false");
	if (m_adjust_hsv)
	{
		Dbg_Message("FacePoints H offset (%f)", m_h_offset);
		Dbg_Message("FacePoints S scale (%f)", m_s_scale);
		Dbg_Message("FacePoints V scale (%f)", m_v_scale);
	}
	Dbg_Message("FacePoints texture size (%d, %d)", m_texture_width, m_texture_height);
}
#endif // __NOPT_ASSERT__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SFacePoints s_default_face_points = {
	{  49,  54 },		// Left eye
	{  76,  54 },		// Right eye
	{  64,  75 },		// Nose
	{  64,  89 },		// Lips
	false,				// Adjust HSV
	0.0f,				// H offset
	1.0f,				// S scale
	1.0f,				// V scale
	128,				// Texture width
	128					// Texture height
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Utility functions
void SetDefaultFacePoints(SFacePoints* pFacePoints)
{
	Dbg_MsgAssert( pFacePoints, ( "No face points" ) );

	*pFacePoints = s_default_face_points;
}  

///////////////////////////////////////////////////////////////////////////////
// CFaceTexMassager

// copy over default face points
SFacePoints		CFaceTexMassager::s_model_face_points = s_default_face_points;

CTexture *		CFaceTexMassager::sp_face_texture_overlay = NULL;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CFaceTexMassager::sSetModelFacePoints(const SFacePoints &f_points)
{
	s_model_face_points = f_points;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CFaceTexMassager::sSetFaceTextureOverlay(CTexture *p_texture)
{
	if (p_texture)
	{
		Dbg_MsgAssert(p_texture->GetWidth() == s_model_face_points.m_texture_width, ("Face texture overlay width is not equal to the face points width"));
		Dbg_MsgAssert(p_texture->GetHeight() == s_model_face_points.m_texture_height, ("Face texture overlay height is not equal to the face points height"));
	}

	sp_face_texture_overlay = p_texture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#define PRINT_TIMES 0

bool			CFaceTexMassager::sMassageTexture(CTexture *p_face_texture, const SFacePoints &c_texture_points, bool palette_gen,
												  bool use_fill_color, Image::RGBA fill_color)
{
	Dbg_Assert(p_face_texture);

#if PRINT_TIMES
	uint32 start_time = Tmr::GetTimeInUSeconds();
#endif

	// Convert texture to 32-bit, if not already (speeds conversion)
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	p_face_texture->Generate32BitImage();
	Mem::Manager::sHandle().PopContext();

	// Adjust the texture based on the model face points
	sAdjustTextureToModel(p_face_texture, c_texture_points, use_fill_color, fill_color);

	// Adjust the texture colors
	sAdjustTextureColors(p_face_texture, c_texture_points);

	// And put overlay on top, if any
	if (sp_face_texture_overlay)
	{
		p_face_texture->CombineTextures(sp_face_texture_overlay, palette_gen);
	}

	// Put 32-bit image into texture
	p_face_texture->Put32BitImageIntoTexture(palette_gen);

#if PRINT_TIMES
	uint32 end_time = Tmr::GetTimeInUSeconds();
	Dbg_Message("sMassageTexture Update time %d us", end_time - start_time);
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CFaceTexMassager::sAdjustTextureToModel(CTexture *p_face_texture, const SFacePoints &c_texture_points,
														bool use_fill_color, Image::RGBA fill_color)
{
	Dbg_Assert(p_face_texture);
	Dbg_MsgAssert(p_face_texture->GetWidth() == s_model_face_points.m_texture_width, ("Face texture width is not equal to the face points width"));
	Dbg_MsgAssert(p_face_texture->GetHeight() == s_model_face_points.m_texture_height, ("Face texture height is not equal to the face points height"));

	// Make a local copy of the texture points, as we might need to modify it to make it safe											 
	SFacePoints texture_points = c_texture_points;
											 
// Make the values "safe" (should also do this at the user interface stage)


// if eyes are flipped, then flip them back
	int t;
	if (texture_points.m_left_eye[X] > texture_points.m_right_eye[X])
	{
		t = texture_points.m_left_eye[X];
		texture_points.m_left_eye[X] = texture_points.m_right_eye[X];
		texture_points.m_right_eye[X] = t;
	}
	
	
// If Left eye is below the nose then put it above it.	
	if (texture_points.m_left_eye[Y] > texture_points.m_nose[Y])
	{
		texture_points.m_left_eye[Y] = texture_points.m_nose[Y]-1;
	}


// If Right eye is below the nose then put it above it.	
	if (texture_points.m_right_eye[Y] >= texture_points.m_nose[Y])
	{
		texture_points.m_right_eye[Y] = texture_points.m_nose[Y]-2;
	}

// if Lips is above the nose, then put it below
	if (texture_points.m_lips[Y] <= texture_points.m_nose[Y])
	{
		texture_points.m_lips[Y] = texture_points.m_nose[Y]+2;
	}

	// All stretching and shrinking will be done at the nose point
	int x_axis_point = s_model_face_points.m_nose[X];
	int y_axis_point = s_model_face_points.m_nose[Y];

	// First, offset the texture by the difference in nose points
	int x_offset = s_model_face_points.m_nose[X] - texture_points.m_nose[X];
	int y_offset = s_model_face_points.m_nose[Y] - texture_points.m_nose[Y];
	p_face_texture->Offset(x_offset, y_offset);

	int model_eyeline_y = (s_model_face_points.m_left_eye[Y] + s_model_face_points.m_right_eye[Y]) >> 1;
	int texture_eyeline_y = ((texture_points.m_left_eye[Y] + texture_points.m_right_eye[Y]) >> 1) + y_offset;

	// Calculate the ratios to find how much we need to pull/push a texture section
	float left_eye_to_nose_width = texture_points.m_nose[X] - texture_points.m_left_eye[X];
	float left_nose_width = texture_points.m_nose[X] + x_offset;
	float left_eye_ratio = left_nose_width / left_eye_to_nose_width;

	float right_eye_to_nose_width = texture_points.m_right_eye[X] - texture_points.m_nose[X];
	float right_nose_width = p_face_texture->GetWidth() - (texture_points.m_nose[X] + x_offset);
	float right_eye_ratio = right_nose_width / right_eye_to_nose_width;

	float eyeline_to_nose_height = (texture_points.m_nose[Y] + y_offset) - texture_eyeline_y;
	float top_nose_height = texture_points.m_nose[Y] + y_offset;
	float eyeline_ratio = top_nose_height / eyeline_to_nose_height;

	float lips_to_nose_height = texture_points.m_lips[Y] - texture_points.m_nose[Y];
	float bottom_nose_height = p_face_texture->GetHeight() - (texture_points.m_nose[Y] + y_offset);
	float lips_ratio = bottom_nose_height / lips_to_nose_height;

	// Move the left eye along the X axis
	int left_eye_pixels = s_model_face_points.m_left_eye[X] - (texture_points.m_left_eye[X] + x_offset);
	//Dbg_Message("Left eye pixels before %d", left_eye_pixels);
	//Dbg_Message("Left eye to_nose %f width %f", left_eye_to_nose_width, left_nose_width);
	left_eye_pixels = (int) ((left_eye_pixels * left_eye_ratio) + 0.5f);
	//Dbg_Message("Left eye pixels after %d", left_eye_pixels);
	if (left_eye_pixels < 0)
	{
		// Stretch out
		p_face_texture->PullToEdge(x_axis_point, X, left_eye_pixels);
	}
	else
	{
		// Shrink
		p_face_texture->PushToPoint(x_axis_point, X, left_eye_pixels, use_fill_color, fill_color);
	}

	// Move the right eye along the X axis
	int right_eye_pixels = s_model_face_points.m_right_eye[X] - (texture_points.m_right_eye[X] + x_offset);
	//Dbg_Message("Right eye pixels before %d", right_eye_pixels);
	//Dbg_Message("Right eye to_nose %f width %f", right_eye_to_nose_width, right_nose_width);
	right_eye_pixels = (int) ((right_eye_pixels * right_eye_ratio) + 0.5f);
	//Dbg_Message("Right eye pixels after %d", right_eye_pixels);
	if (right_eye_pixels >= 0)
	{
		// Stretch out
		p_face_texture->PullToEdge(x_axis_point, X, right_eye_pixels);
	}
	else
	{
		// Shrink
		p_face_texture->PushToPoint(x_axis_point, X, right_eye_pixels, use_fill_color, fill_color);
	}

	// Move the eyeline along the Y axis
	int eyeline_pixels = model_eyeline_y - texture_eyeline_y;
	eyeline_pixels = (int) ((eyeline_pixels * eyeline_ratio) + 0.5f);
	if (eyeline_pixels < 0)
	{
		// Stretch out
		p_face_texture->PullToEdge(y_axis_point, Y, eyeline_pixels);
	}
	else
	{
		// Shrink
		p_face_texture->PushToPoint(y_axis_point, Y, eyeline_pixels, use_fill_color, fill_color);
	}

	// Move the lips along the Y axis
	int lips_pixels = s_model_face_points.m_lips[Y] - (texture_points.m_lips[Y] + y_offset);
	lips_pixels = (int) ((lips_pixels * lips_ratio) + 0.5f);
	if (lips_pixels >= 0)
	{
		// Stretch out
		p_face_texture->PullToEdge(y_axis_point, Y, lips_pixels);
	}
	else
	{
		// Shrink
		p_face_texture->PushToPoint(y_axis_point, Y, lips_pixels, use_fill_color, fill_color);
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CFaceTexMassager::sAdjustTextureColors(CTexture *p_face_texture, const SFacePoints &texture_points)
{
	Dbg_Assert(p_face_texture);

	if (texture_points.m_adjust_hsv)
	{
		p_face_texture->AdjustHSV(texture_points.m_h_offset, texture_points.m_s_scale, texture_points.m_v_scale, false);
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CFaceTexMassager::sCombineTextureWithOverlay(CTexture *p_face_texture)
{
	Dbg_Assert(p_face_texture);

	if (sp_face_texture_overlay)
	{
		p_face_texture->CombineTextures(sp_face_texture_overlay, false);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
// Script functions

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool SetFacePointsStruct(const SFacePoints& face_points, Script::CStruct *p_struct)
{
	Dbg_Assert(p_struct);

	Script::CArray* p_left_eye = new Script::CArray;
	p_left_eye->SetSizeAndType( 2, ESYMBOLTYPE_INTEGER );
	p_left_eye->SetInteger( 0, face_points.m_left_eye[X] );
	p_left_eye->SetInteger( 1, face_points.m_left_eye[Y] );
	
	Script::CArray* p_right_eye = new Script::CArray;
	p_right_eye->SetSizeAndType( 2, ESYMBOLTYPE_INTEGER );
	p_right_eye->SetInteger( 0, face_points.m_right_eye[X] );
	p_right_eye->SetInteger( 1, face_points.m_right_eye[Y] );
	
	Script::CArray* p_nose = new Script::CArray;
	p_nose->SetSizeAndType( 2, ESYMBOLTYPE_INTEGER );
	p_nose->SetInteger( 0, face_points.m_nose[X] );
	p_nose->SetInteger( 1, face_points.m_nose[Y] );
	
	Script::CArray* p_lips = new Script::CArray;
	p_lips->SetSizeAndType( 2, ESYMBOLTYPE_INTEGER );
	p_lips->SetInteger( 0, face_points.m_lips[X] );
	p_lips->SetInteger( 1, face_points.m_lips[Y] );

	p_struct->AddArrayPointer( CRCD(0x08bf1d3f,"left_eye"), p_left_eye );
	p_struct->AddArrayPointer( CRCD(0xb0b44396,"right_eye"), p_right_eye );
	p_struct->AddArrayPointer( CRCD(0x7f03932c,"nose"), p_nose );
	p_struct->AddArrayPointer( CRCD(0x0e7ec187,"lips"), p_lips );

	if (face_points.m_adjust_hsv)
	{
		p_struct->AddFloat(CRCD(0x6e94f918,"h"), face_points.m_h_offset);
		p_struct->AddFloat(CRCD(0xe4f130f4,"s"), face_points.m_s_scale);
		p_struct->AddFloat(CRCD(0x949bc47b,"v"), face_points.m_v_scale);
	}

	p_struct->AddInteger( CRCD(0x73e5bad0,"width"), face_points.m_texture_width );
	p_struct->AddInteger( CRCD(0x0ab21af0,"height"), face_points.m_texture_height );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool GetFacePointsStruct(SFacePoints &face_points, Script::CStruct *p_struct)
{
	Dbg_Assert(p_struct);

	Script::CArray *p_left_eye = NULL;
	Script::CArray *p_right_eye = NULL;
	Script::CArray *p_nose = NULL;
	Script::CArray *p_lips = NULL;

	// Left eye
	p_struct->GetArray( CRCD(0x08bf1d3f,"left_eye"), &p_left_eye );
	if (p_left_eye)
	{
		face_points.m_left_eye[X] = p_left_eye->GetInteger(0);
		face_points.m_left_eye[Y] = p_left_eye->GetInteger(1);
	}
	else
	{
		Dbg_MsgAssert(0, ("Couldn't find member left_eye"))
		return false;	
	}

	// Right eye
	p_struct->GetArray( CRCD(0xb0b44396,"right_eye"), &p_right_eye );
	if (p_right_eye)
	{
		face_points.m_right_eye[X] = p_right_eye->GetInteger(0);
		face_points.m_right_eye[Y] = p_right_eye->GetInteger(1);
	}
	else
	{
		Dbg_MsgAssert(0, ("Couldn't find member right_eye"))
		return false;	
	}

	// Nose
	p_struct->GetArray( CRCD(0x7f03932c,"nose"), &p_nose );
	if (p_nose)
	{
		face_points.m_nose[X] = p_nose->GetInteger(0);
		face_points.m_nose[Y] = p_nose->GetInteger(1);
	}
	else
	{
		Dbg_MsgAssert(0, ("Couldn't find member nose"))
		return false;	
	}

	// Lips
	p_struct->GetArray( CRCD(0x0e7ec187,"lips"), &p_lips );
	if (p_lips)
	{
		face_points.m_lips[X] = p_lips->GetInteger(0);
		face_points.m_lips[Y] = p_lips->GetInteger(1);
	}
	else
	{
		Dbg_MsgAssert(0, ("Couldn't find member lips"))
		return false;	
	}

	// Grab color adjustment values, if any
	face_points.m_adjust_hsv = p_struct->GetFloat(CRCD(0x6e94f918,"h"), &face_points.m_h_offset);
	face_points.m_adjust_hsv = p_struct->GetFloat(CRCD(0xe4f130f4,"s"), &face_points.m_s_scale) && face_points.m_adjust_hsv;
	face_points.m_adjust_hsv = p_struct->GetFloat(CRCD(0x949bc47b,"v"), &face_points.m_v_scale) && face_points.m_adjust_hsv;

	if (face_points.m_adjust_hsv)
	{
		Dbg_MsgAssert((face_points.m_h_offset >= 0.0f) && (face_points.m_h_offset <= 360.0f), ("h must be in the range of 0-360"));
		Dbg_MsgAssert(face_points.m_s_scale >= 0.0f, ("s cannot be negative"));
		Dbg_MsgAssert(face_points.m_v_scale >= 0.0f, ("v cannot be negative"));
	}

	// Width and height (some functions may not care, so init them to 0)
	face_points.m_texture_width = 0;
	face_points.m_texture_height = 0;
	p_struct->GetInteger( CRCD(0x73e5bad0,"width"), &face_points.m_texture_width );
	p_struct->GetInteger( CRCD(0x0ab21af0,"height"), &face_points.m_texture_height );

	return true;
}

// @script | SetModelFaceTexturePoints | Sets the face points for the face texture of the model
// @parm struct | face_points | face points structure
bool ScriptSetModelFaceTexturePoints(Script::CStruct *pParams, Script::CScript *pScript)
{
	SFacePoints face_points;

	Script::CStruct *pFacePointsStruct = NULL;
	pParams->GetStructure("face_points", &pFacePointsStruct);

	if (pFacePointsStruct)
	{
		GetFacePointsStruct(face_points, pFacePointsStruct);

		Dbg_MsgAssert(face_points.m_texture_width != 0, ("Must set texture width"));
		Dbg_MsgAssert(face_points.m_texture_height != 0, ("Must set texture height"));

		CFaceTexMassager::sSetModelFacePoints(face_points);
		//face_points.PrintData();
	}
	else
	{
		Dbg_MsgAssert(0, ("The face_points structure needs to be supplied"));
	}

	return true;
}

// @script | SetFaceMassageTextureOverlay | Sets the texture that is used as an overlay to the massaged face texture 
// @parm name |  | name of texture
bool ScriptSetFaceMassageTextureOverlay(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		CFaceTexMassager::sSetFaceTextureOverlay(p_texture);
	} else {
		Dbg_MsgAssert(0, ("Can't find texture %s to adjust", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | MassageFaceTexture | Transform the supplied face texture to one that can be used with a model
// @parm name | texture | name of texture
// @parm struct | face_points | face points structure
// @parmopt flag | no_palette_gen | | use original palette (faster)
bool ScriptMassageFaceTexture(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(CRCD(0x7d99f28d,"texture"), &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		bool palette_gen = true;
		SFacePoints face_points;

		Script::CStruct *pFacePointsStruct = NULL;
		pParams->GetStructure(CRCD(0xac3cd84c,"face_points"), &pFacePointsStruct);

		if (pParams->ContainsFlag(CRCD(0x5905256b,"no_palette_gen")))
		{
			palette_gen = false;
		}

		if (pFacePointsStruct)
		{
			GetFacePointsStruct(face_points, pFacePointsStruct);

			CFaceTexMassager::sMassageTexture(p_texture, face_points, palette_gen);
			//CFaceTexMassager::sMassageTexture(p_texture, face_points, palette_gen, true, Image::RGBA(255, 0, 0, 128));
		}
		else
		{
			Dbg_MsgAssert(0, ("The face_points structure needs to be supplied"));
		}

	} else {
		Dbg_MsgAssert(0, ("Can't find texture %s to massage", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | AdjustFaceTextureToModel | Pulls portions of the face texture from the supplied points to the model points
// @parm name | texture | name of texture
// @parm struct | face_points | face points structure
bool ScriptAdjustFaceTextureToModel(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(CRCD(0x7d99f28d,"texture"), &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));

	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		SFacePoints face_points;

		Script::CStruct *pFacePointsStruct = NULL;
		pParams->GetStructure(CRCD(0xac3cd84c,"face_points"), &pFacePointsStruct);

		if (pFacePointsStruct)
		{
			GetFacePointsStruct(face_points, pFacePointsStruct);

			CFaceTexMassager::sAdjustTextureToModel(p_texture, face_points);
		}
		else
		{
			Dbg_MsgAssert(0, ("The face_points structure needs to be supplied"));
		}
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to adjust", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | AdjustFaceTextureColors | Does all the requested color adjustments to the face texture.
// @parm name | texture | name of texture
// @parm struct | face_points | face points structure
bool ScriptAdjustFaceTextureColors(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(CRCD(0x7d99f28d,"texture"), &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));

	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		SFacePoints face_points;

		Script::CStruct *pFacePointsStruct = NULL;
		pParams->GetStructure(CRCD(0xac3cd84c,"face_points"), &pFacePointsStruct);

		if (pFacePointsStruct)
		{
			GetFacePointsStruct(face_points, pFacePointsStruct);

			CFaceTexMassager::sAdjustTextureColors(p_texture, face_points);
		}
		else
		{
			Dbg_MsgAssert(0, ("The face_points structure needs to be supplied"));
		}
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to adjust", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | CombineFaceTextureWithOverlay | Puts overlay texture on top of face texture and combines them.
// @parm name | texture | name of texture
bool ScriptCombineFaceTextureWithOverlay(Script::CStruct *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(CRCD(0x7d99f28d,"texture"), &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));

	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		CFaceTexMassager::sCombineTextureWithOverlay(p_texture);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to combine", Script::FindChecksumName(checksum)));
	}

	return true;
}

} // namespace Nx


