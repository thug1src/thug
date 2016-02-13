#include <sk/objects/followob.h>
#include <gel/object.h>
#include <sk/objects/movingobject.h>
#include <core/math.h>

namespace Obj
{

CFollowOb::CFollowOb()
{
	Reset();
}

CFollowOb::~CFollowOb()
{
}

void CFollowOb::Reset()
{
	m_num_path_points=0;
	m_most_recent_path_point=0;
	m_leader_name=0;
	m_leader_pos.Set();
	m_distance=0.0f;
	m_orient_y=false;
}

// Adds a new position taken from the leader object, if that position is suitably interesting.
// (Ie, it is not too close to the last position)
// FOLLOW_RESOLUTION should be kept as big as possible. That way, the array of positions does
// not need to be so big, and calculating the position will be faster because it won't have to
// step over so many segments.
// If it is too big though the object won't follow the leader's path so accurately and 
// will start cutting corners noticeably.
#define FOLLOW_RESOLUTION 40.0f
void CFollowOb::GetNewPathPointFromObjectBeingFollowed()
{
	//CMovingObject *p_obj=CMovingObject::m_hash_table.GetItem(m_leader_name);
	CMovingObject *p_obj=(CMovingObject*)Obj::ResolveToObject(m_leader_name);
	
	if (!p_obj)
	{
		// Can't find the object, oh well.
		return;
	}	

	m_leader_pos=p_obj->m_pos;
	
	if (m_num_path_points==0)
	{
		// Insert the first point.
		m_most_recent_path_point=0;
		mp_path_points[0]=m_leader_pos;
		++m_num_path_points;
		return;
	}	
	
	if (Mth::DistanceSqr(mp_path_points[m_most_recent_path_point],m_leader_pos) > 
		FOLLOW_RESOLUTION*FOLLOW_RESOLUTION)
	{
		// The new point is sufficiently far from the last, so insert it into the cyclic buffer.
		++m_most_recent_path_point;
		if (m_most_recent_path_point>=MAX_FOLLOWOB_PATH_POINTS)
		{
			m_most_recent_path_point=0;
		}
		
		mp_path_points[m_most_recent_path_point]=m_leader_pos;
		
		// m_num_path_points increases till it hits MAX_FOLLOWOB_PATH_POINTS then stays there.
		if (m_num_path_points<MAX_FOLLOWOB_PATH_POINTS)
		{
			++m_num_path_points;
		}	
	}
}

void CFollowOb::CalculatePositionAndOrientation(Mth::Vector *p_pos, Mth::Matrix *p_orientation)
{
	Dbg_MsgAssert(p_pos,("NULL p_pos"));
	Dbg_MsgAssert(p_orientation,("NULL p_orientation"));
	
	if (m_num_path_points<1)
	{
		// Need at least 1 path point before the interpolated position can be calculated.
		return;
	}	
	
	Mth::Vector seg_start;
	Mth::Vector seg_end;
	Mth::Vector seg_vec;
	float seg_length;

	int num_points_left=m_num_path_points;
	float dist_remaining=m_distance;

	seg_start=m_leader_pos;
	int i=m_most_recent_path_point;
	seg_end=mp_path_points[i];	
	num_points_left-=1;
	
	
	while (true)
	{
		seg_vec=seg_end-seg_start;
		seg_length=seg_vec.Length();
		if (dist_remaining > seg_length)
		{
			dist_remaining-=seg_length;
			seg_start=seg_end;
			if (!num_points_left)
			{
				break;
			}	
			--i;
			if (i < 0)
			{
				i=MAX_FOLLOWOB_PATH_POINTS-1;
			}
			seg_end=mp_path_points[i];	
			--num_points_left;
		}
		else
		{
			break;
		}
	}
	
	if (seg_length>0.0f)
	{
		Mth::Vector seg_dir=seg_vec/-seg_length;
		
		*p_pos=seg_start-seg_dir*dist_remaining;
		
		if (m_orient_y)
		{
			// Instead of setting z to seg_dir directly, use some of the previous value
			// so that the orientation gradually moves towards the new dir rather than snapping.
			(*p_orientation)[Z]=10.0f*(*p_orientation)[Z]+seg_dir;
			(*p_orientation)[Z].Normalize();
			
			Mth::Vector Up(0.0f,1.0f,0.0f);
			(*p_orientation)[X]=Mth::CrossProduct(Up,(*p_orientation)[Z]);
			(*p_orientation)[X].Normalize();
			(*p_orientation)[Y]=Mth::CrossProduct((*p_orientation)[Z],(*p_orientation)[X]);
		}
		else
		{
			(*p_orientation)[Z]=10.0f*(*p_orientation)[Z]+seg_dir;
			(*p_orientation)[Z][Y]=0;
			(*p_orientation)[Z].Normalize();
			(*p_orientation)[Y].Set(0.0f,1.0f,0.0f);
			(*p_orientation)[X]=Mth::CrossProduct((*p_orientation)[Y],(*p_orientation)[Z]);
		}
	}	
}

}

