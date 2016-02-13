#ifndef __ENGINE_EXPORTMSG_H__
#define	__ENGINE_EXPORTMSG_H__

#include <core/defines.h>

namespace Net
{

#define	vSERVER_IP_VARIABLE	"VIEWER_IP"
#define vXBOX_SERVER_IP_VARIABLE "XBOX_VIEWER_IP"
#define vNGC_SERVER_IP_VARIABLE "NGC_VIEWER_IP"

enum
{
	vEXPORT_COMM_PORT = 10000,
};	

enum
{	
	vMSG_ID_QUICKVIEW		= 32,	//	32 is the first available user-defined message id
	vMSG_ID_UPDATE_MATERIAL,		//	New material properties
	vMSG_ID_REMOTE_Q,
	vMSG_ID_VIEWOBJ_LOAD_MODEL,
	vMSG_ID_VIEWOBJ_UNLOAD_MODEL,
	vMSG_ID_VIEWOBJ_SET_ANIM,
	vMSG_ID_VIEWOBJ_SET_ANIM_SPEED,
	vMSG_ID_VIEWOBJ_INCREMENT_FRAME,
	vMSG_ID_VIEWOBJ_SET_ANIM_FILE,
	vMSG_ID_VIEWOBJ_SET_CAMANIM_FILE,
	vMSG_ID_INCREMENTAL_UPDATE,
	vMSG_ID_RUN_SCRIPT_COMMAND,
	vMSG_ID_VIEWOBJ_PREVIEW_SEQUENCE,
};

class MsgViewObjLoadModel
{
public:
	char	m_ModelName[128];
	uint32	m_AnimScriptName;
	uint32	m_SkeletonName;
};

class MsgViewObjSetAnimSpeed
{
public:
	float	m_AnimSpeed;
};

class MsgViewObjSetAnim
{
public:
	uint32	m_AnimName;
};

class MsgViewObjIncrementFrame
{
public:
	bool	m_Forwards;
};				   

class MsgViewObjSetAnimFile
{
public:
	char    m_Filename[128];
	uint32  m_checksum;
};

class MsgViewObjSetCamAnimFile
{
public:
	char    m_Filename[128];
	uint32  m_checksum;
};

class MsgQuickview
{
public:
	char	m_Filename[128];
	char	m_UpdateFilename[128];
};

class MsgMaterialUpdate
{
public:
	unsigned long	MaterialChecksum;
	int				m_BlendMode;
	int				m_FixedAlpha;
	int				m_MappingMode;		// Explicit or procedural (eg. environment-mapping)	
	int				m_MinFilteringMode;	// Point/Bi-linear
	int				m_MagFilteringMode;	// Point/Bi-linear/Tri-linear
	bool			m_UVWibbleEnabled;		
	float			m_UVel;
	float			m_VVel;	
	float			m_UAmplitude;
	float			m_VAmplitude;
	float			m_UPhase;
	float			m_VPhase;
	float			m_UFrequency;
	float			m_VFrequency;
	float			m_MipMapK;
	int				m_MipMapL;	
};

}
#endif	// __ENGINE_EXPORTMSG_H__