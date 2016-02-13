///////////////////////////////////////////////////////////////////////////////
// FaceMassage.h

#ifndef	__GFX_FACE_MASSAGE_H__
#define	__GFX_FACE_MASSAGE_H__

#include <core/defines.h>
#include <core/support.h>

#include <gfx/image/imagebasic.h>

namespace Script
{
	class CStruct;
	class CScript;
}

namespace Nx
{

// Forward declarations
class CTexture;

////////////////////////////////////////////////////////////////////////////
// Holds all the face coordinates
struct SFacePoints
{
	// Constants
	enum
	{
		NUM_AXES	= 2,			// Just two axes for 2D coordinates
	};

	// Eye points
	int				m_left_eye[NUM_AXES];
	int				m_right_eye[NUM_AXES];

	// Nose point (between the nostrils)
	int				m_nose[NUM_AXES];

	// Lips point (center)
	int				m_lips[NUM_AXES];

	// Color HSV adjustments
	bool			m_adjust_hsv;
	float			m_h_offset;				// Added to each pixel H value (0 - 360)
	float			m_s_scale;				// Scale applied to each pixel S value (1=no change)
	float			m_v_scale;				// Scale applied to each pixel V value (1=no change)

	// Texture size
	int				m_texture_width;
	int				m_texture_height;

#ifdef __NOPT_ASSERT__
	void			PrintData();
#endif // __NOPT_ASSERT__
};

//////////////////////////////////////////////////////////////////////////////
// Transforms a face texture into one that can be used by a model.
class CFaceTexMassager
{
public:
	// Set data
	static void				sSetModelFacePoints(const SFacePoints &f_points);
	static void				sSetFaceTextureOverlay(CTexture *p_texture);

	// Massage the face texture into a usable face texture for a model (overwrites old texture pixel data)
	static bool				sMassageTexture(CTexture *p_face_texture, const SFacePoints &texture_points, bool palette_gen = true,
											bool use_fill_color = false, Image::RGBA fill_color = Image::RGBA(128, 128, 128, 128));

	// Individual massage steps (mainly for editor).  Note that these functions do not convert to 32-bit or back to
	// 8-bit at the end.  These functions need to be called separately.
	static bool				sAdjustTextureToModel(CTexture *p_face_texture, const SFacePoints &texture_points,
												  bool use_fill_color = false, Image::RGBA fill_color = Image::RGBA(128, 128, 128, 128));
	static bool				sAdjustTextureColors(CTexture *p_face_texture, const SFacePoints &texture_points);
	static bool				sCombineTextureWithOverlay(CTexture *p_face_texture);

protected:

	static SFacePoints		s_model_face_points;		// Face points needed for a model

	static CTexture *		sp_face_texture_overlay;  	// The texture that needs to go over the top of a supplied face texture

};

// Script functions
bool ScriptSetModelFaceTexturePoints(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetFaceMassageTextureOverlay(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptMassageFaceTexture(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAdjustFaceTextureToModel(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAdjustFaceTextureColors(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptCombineFaceTextureWithOverlay(Script::CStruct *pParams, Script::CScript *pScript);

// Utility functions
void SetDefaultFacePoints(SFacePoints* pFacePoints);
bool SetFacePointsStruct(const SFacePoints &face_points, Script::CStruct *p_struct);
bool GetFacePointsStruct(SFacePoints &face_points, Script::CStruct *p_struct);

}

#endif //	__GFX_FACE_MASSAGE_H__

