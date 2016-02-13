//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       RibbonComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  5/20/3
//****************************************************************************

#include <gel/components/ribboncomponent.h>
#include <gel/components/skeletoncomponent.h>

#include <gel/collision/collcache.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/engine/feeler.h>

#define MESSAGE(a) { printf("M:%s:%i: %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPI(a) { printf("D:%s:%i: " #a " = %i\n", __FILE__ + 15, __LINE__, a); }
#define DUMPB(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a ? "true" : "false"); }
#define DUMPF(a) { printf("D:%s:%i: " #a " = %g\n", __FILE__ + 15, __LINE__, a); }
#define DUMPE(a) { printf("D:%s:%i: " #a " = %e\n", __FILE__ + 15, __LINE__, a); }
#define DUMPS(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPP(a) { printf("D:%s:%i: " #a " = %p\n", __FILE__ + 15, __LINE__, a); }
#define DUMPV(a) { printf("D:%s:%i: " #a " = %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z]); }
#define DUMP4(a) { printf("D:%s:%i: " #a " = %g, %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z], (a)[W]); }
#define DUMPM(a) { DUMP4(a[X]); DUMP4(a[Y]); DUMP4(a[Z]); DUMP4(a[W]); }
#define DUMP2(a) { printf("D:%s:%i " #a " = ", __FILE__ + 15, __LINE__); for (int n = 32; n--; ) { printf("%c", ((a) & (1 << n)) ? '1' : '0'); } printf("\n"); }
#define MARK { printf("K:%s:%i: %s\n", __FILE__ + 15, __LINE__, __PRETTY_FUNCTION__); }
#define PERIODIC(n) for (static int p__ = 0; (p__ = ++p__ % (n)) == 0; )

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CRibbonComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CRibbonComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CRibbonComponent::CRibbonComponent() : CBaseComponent()
{
	SetType( CRC_RIBBON );
	
	mp_links = NULL;
	
	m_last_frame_length = 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CRibbonComponent::~CRibbonComponent()
{
	if (mp_links)
	{
		delete mp_links;
		mp_links = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRibbonComponent::InitFromStructure( Script::CStruct* pParams )
{
	pParams->GetChecksum(CRCD(0xcab94088, "bone"), &m_bone_name, Script::ASSERT);
	pParams->GetInteger(CRCD(0x69feef91, "num_links"), &m_num_links, Script::ASSERT);
	pParams->GetFloat(CRCD(0x9f4625c2, "link_length"), &m_link_length, Script::ASSERT);
	pParams->GetChecksum(CRCD(0x99a9b716, "color"), &m_color, Script::ASSERT);
	
	mp_links = new SLink [ m_num_links ];
	for (int n = m_num_links; n--; )
	{
		mp_links[n].p.Set();
		mp_links[n].last_p.Set();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRibbonComponent::Finalize()
{
	mp_skeleton_component = GetSkeletonComponentFromObject(GetObject());
	
	Dbg_Assert(mp_skeleton_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRibbonComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRibbonComponent::Update()
{
	float frame_length = Tmr::FrameLength();
	
	// update link positions based on velocity
	for (int n = m_num_links; n--; )
	{
		Mth::Vector vel = mp_links[n].p - mp_links[n].last_p;
		vel *= 1.0f / m_last_frame_length;
		
		// damp
		vel -= 10.0f * frame_length * vel;
		
		// momentum
		mp_links[n].next_p = mp_links[n].p + frame_length * vel;
		
		// gravity
		mp_links[n].next_p[Y] -= 800.0f * frame_length * frame_length;
	}
	
	// fetch bone position
	Mth::Vector bone_p;
	mp_skeleton_component->GetBonePosition(m_bone_name, &bone_p);
	bone_p = GetObject()->GetMatrix().Rotate(bone_p);
	bone_p += GetObject()->GetPos();
	
	// update link constraints
	for (int c = 20; c--; )
	{
		// first link special case
		float length = (mp_links[0].next_p - bone_p).Length();
		Mth::Vector axis = (mp_links[0].next_p - bone_p) * (1.0f / length);
		float adjustment = m_link_length - length;
		mp_links[0].next_p += adjustment * axis;
		
		// second and subsequent links
		for (int n = 1; n < m_num_links; n++)
		{
			float length = (mp_links[n].next_p - mp_links[n - 1].next_p).Length();
			Mth::Vector axis = (mp_links[n].next_p - mp_links[n - 1].next_p) * (1.0f / length);
			
			float adjustment = 0.5f * (m_link_length - length);
			
			// hack
			if (length > 20.0f * m_link_length)
			{
				CallMemberFunction(CRCD(0x796084f4, "Ribbon_Reset"), NULL, NULL);
				return;
			}
			
			mp_links[n].next_p += adjustment * axis;
			mp_links[n - 1].next_p -= adjustment * axis;
		}
	}
	
	#if 0
	
	// cheap collision detection
	float min_y = GetObject()->GetPos()[Y] + 0.1f;
	for (int n = m_num_links; n--; )
	{
		mp_links[n].next_p[Y] = Mth::ClampMin(mp_links[n].next_p[Y], min_y);
	}
	
	#else
	
	Mth::CBBox bbox;
	Nx::CCollCache* p_coll_cache = Nx::CCollCacheManager::sCreateCollCache();
	for (int n = m_num_links; n--; )
	{
		bbox.AddPoint(mp_links[n].p);
		bbox.AddPoint(mp_links[n].next_p);
	}
	p_coll_cache->Update(bbox);
	
	CFeeler feeler;
	feeler.SetCache(p_coll_cache);
	// real collision detection
	for (int n = m_num_links; n--; )
	{
		feeler.m_start = mp_links[n].p;
		feeler.m_end = mp_links[n].next_p;
		if (!feeler.GetCollision()) continue;
		
		Mth::Vector movement = mp_links[n].next_p - mp_links[n].p;
		
		mp_links[n].next_p = feeler.GetPoint() + 0.1f * feeler.GetNormal();
		
		// assume no additional collision
		movement *= 1.0f - feeler.GetDist();
		movement.ProjectToPlane(feeler.GetNormal());
		
		mp_links[n].next_p += movement;
	}
	
	Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
	
	#endif
	
	// update state
	for (int n = m_num_links; n--; )
	{
		mp_links[n].last_p = mp_links[n].p;
		mp_links[n].p = mp_links[n].next_p;
	}
	m_last_frame_length = frame_length;
	
	// debug draw
	Gfx::AddDebugLine(bone_p, mp_links[0].p, m_color, 0, 1);
	for (int n = 1; n < m_num_links; n++)
	{
		Gfx::AddDebugLine(mp_links[n].p, mp_links[n - 1].p, m_color, 0, 1);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CRibbonComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case CRCC(0x796084f4, "Ribbon_Reset"):
		{
			MESSAGE("Resetting");
			
			Mth::Vector bone_p;
			mp_skeleton_component->GetBonePosition(m_bone_name, &bone_p);
			bone_p = GetObject()->GetMatrix().Rotate(bone_p);
			bone_p += GetObject()->GetPos();
			
			for (int n = m_num_links; n--; )
			{
				mp_links[n].p = bone_p;
				mp_links[n].last_p = bone_p;
				mp_links[n].p[Y] += 0.1f * n;
				mp_links[n].last_p[Y] += 0.1f * n;
			}
			break;
		}
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRibbonComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CRibbonComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

}
