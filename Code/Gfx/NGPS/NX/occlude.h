#ifndef	__OCCLUDE_H__
#define	__OCCLUDE_H__

//#define	OCCLUDER_USES_VU0_MACROMODE
#define	OCCLUDER_USES_VU0_MICROMODE


namespace NxPs2
{
	void AddOcclusionPoly( Mth::Vector &v0, Mth::Vector &v1, Mth::Vector &v2, Mth::Vector &v3, uint32 checksum = 0 );
	void EnableOcclusionPoly( uint32 checksum, bool available );
	void RemoveAllOcclusionPolys( void );
	void BuildOccluders( Mth::Vector *p_cam_pos );
	bool TestSphereAgainstOccluders( Mth::Vector *p_center, float radius, uint32 meshes = 1 );

	bool OccludeUseVU0();
	void OccludeDisableVU0();

	bool OccludeUseScratchPad();
	void OccludeDisableScratchPad();


const uint32 MAX_NEW_OCCLUSION_POLYS_TO_CHECK_PER_FRAME = 4;
const uint32 MIN_NEW_OCCLUSION_POLYS_TO_CHECK_PER_FRAME = 4;

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// Structure used to store details of a single poly. A list of these will be built at geometry load time.
struct sOcclusionPoly
{
	bool		in_use;
	bool		available;	// Whether the poly is available for selection for occlusion.
	uint32		checksum;	// Name checksum of the occlusion poly.
	Mth::Vector	verts[4];
	Mth::Vector	normal;
};
	
const uint32	MAX_OCCLUDERS				= 12;

struct sOccluder
{
	static uint32		NumOccluders;
	static sOccluder	Occluders[MAX_OCCLUDERS];
	static bool			sUseVU0;
	static bool			sUseScratchPad;

	static void			add_to_stack( sOcclusionPoly *p_poly );
	static void			sort_stack( void );
	static void			tidy_stack( void );

	sOcclusionPoly	*p_poly;
	Mth::Vector		planes[5];
	int				score;			// Current rating on quality of occlusion - based on number of meshes occluded last frame.
};

const uint32	MAX_OCCLUSION_POLYS			= 128; 		// not currently useing that many
extern uint32			NumOcclusionPolys;
extern uint32			NextOcclusionPolyToCheck;
extern sOcclusionPoly	OcclusionPolys[];



}

#endif

