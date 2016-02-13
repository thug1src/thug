//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       CustomAnimKey.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  02/11/2002
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/customanimkey.h>
								   
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/utils.h>
#include <sys/file/filesys.h>

#include <core/math/quat.h>
#include <core/math/vector.h>
#include <gfx/camera.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Gfx
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

// all custom keys must have 32-bit data, for the animconv's
// byte-swapping algorithm to work properly

struct SIntermediateCustomAnimKeyHeader
{
	uint32	timeStamp;
	uint32	keyType;
	uint32	size;
};

typedef enum
{
	vUNUSED1 = 0,
	vCHANGE_FOCAL_LENGTH,
	vCHANGE_CAMERA_RT,
	vCHANGE_CAMERA_RT_IGNORE,
	vRUN_SCRIPT,
	vCREATE_OBJECT_FROM_STRUCT,
	vKILL_OBJECT_FROM_STRUCT,
	vCHANGE_CAMERA_RT_ENDKEY,
};

// class CChangeFOVKey
class CChangeFOVKey : public CCustomAnimKey
{
public:
	CChangeFOVKey( int frame, float fov );

public:
	virtual	bool	process_key( Obj::CObject* pObject );

protected:
	float	m_fov;
};

// class CRunScriptKey
class CRunScriptKey : public CCustomAnimKey
{
public:
	CRunScriptKey( int frame, uint32 scriptName );

public:
	virtual	bool	process_key( Obj::CObject* pObject );

protected:
	uint32	m_scriptName;
};

// class CChangeCameraRTKey
class CChangeCameraRTKey : public CCustomAnimKey
{
public:
	CChangeCameraRTKey( int frame, const Mth::Quat& theQuat, const Mth::Vector& theVector, bool endKey );

public:
	virtual	bool	process_key( Obj::CObject* pObject );
	virtual bool	WithinRange( float startFrame, float endFrame, bool inclusive );

protected:
	Mth::Quat	m_rot;
	Mth::Vector	m_trans;
	bool		m_endKey;
};

// class CEmptyKey
class CEmptyKey : public CCustomAnimKey
{
public:
	CEmptyKey( int frame );

public:
	virtual	bool	process_key( Obj::CObject* pObject );
};

// class CCreateObjectFromStructKey
class CCreateObjectFromStructKey : public CCustomAnimKey
{
public:
	CCreateObjectFromStructKey( int frame, uint32 objectStructName );

public:
	virtual	bool	process_key( Obj::CObject* pObject );

protected:
	uint32	m_objectStructName;
};

// class CKillObjectFromStructKey
class CKillObjectFromStructKey : public CCustomAnimKey
{
public:
	CKillObjectFromStructKey( int frame, uint32 objectStructName );

public:
	virtual	bool	process_key( Obj::CObject* pObject );

protected:
	uint32	m_objectStructName;
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCustomAnimKey::CCustomAnimKey( int frame ) : Lst::Node<CCustomAnimKey>( this ), m_frame( frame )
{
	// defaults to inactive
	m_active = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCustomAnimKey::WithinRange( float startFrame, float endFrame, bool inclusive )
{
	if ( inclusive )
	{
		if ( startFrame < endFrame )
		{
			// forwards
			return ( m_frame >= startFrame && m_frame <= endFrame );
		}
		else
		{
			// backwards
			return ( m_frame <= startFrame && m_frame >= endFrame );
		}
	}
	else
	{
		if ( startFrame < endFrame )
		{
			// forwards
			return ( m_frame >= startFrame && m_frame < endFrame );
		}
		else
		{
			// backwards
			return ( m_frame <= startFrame && m_frame > endFrame );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCustomAnimKey::SetActive( bool active )
{
	m_active = active;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCustomAnimKey::ProcessKey( Obj::CObject* pObject )
{
	if ( !m_active )
	{
		Dbg_Message( "Key at %d was not active...  ignored!", m_frame );

		return false;
	}

	// once it's run, don't run it again...
	// (this is in case you pause a movie camera
	// on the same frame that it's running a key)
	this->SetActive( false );

	return process_key( pObject );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CChangeFOVKey::CChangeFOVKey( int frame, float fov ) : CCustomAnimKey( frame ), m_fov( fov )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CChangeFOVKey::process_key( Obj::CObject* pObject )
{
	if ( Script::GetInt( CRCD(0x2f510f45,"moviecam_debug"), false ) )
	{
		Dbg_Message( "Processing Change FOV key (fov = %f, %f) (%d)!", m_fov, Mth::RadToDeg(m_fov), m_frame );
	}

	Script::CScriptStructure* pParams = new Script::CScriptStructure;
	pParams->AddFloat( NONAME, m_fov );
	Script::RunScript( "ChangeCameraFov", pParams, pObject );
	delete pParams;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/				

CRunScriptKey::CRunScriptKey( int frame, uint32 scriptName ) : CCustomAnimKey( frame ), m_scriptName( scriptName )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CRunScriptKey::process_key( Obj::CObject* pObject )
{
	if ( Script::GetInt( CRCD(0x2f510f45,"moviecam_debug"), false ) )
	{
		Dbg_Message( "Processing Run Script key (script = %s) @ %d!", Script::FindChecksumName( m_scriptName ), m_frame );
	}
	Script::RunScript( m_scriptName, NULL, pObject );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CChangeCameraRTKey::CChangeCameraRTKey( int frame, const Mth::Quat& theQuat, const Mth::Vector& theVector, bool isEndKey ) : CCustomAnimKey( frame )
{
	// rotate by 90 degrees,
	// eventually, we should do this in the build tools
	
	Mth::Quat rotQuat( Mth::Vector(1,0,0), Mth::DegToRad(90.0f) );
	m_rot = theQuat * rotQuat;

	m_trans[X] = theVector[X];
	m_trans[Y] = theVector[Z];
	m_trans[Z] = -theVector[Y];
	m_trans[W] = 1.0f;

	m_endKey = isEndKey;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CChangeCameraRTKey::process_key( Obj::CObject* pObject )
{
	// GJ:  This key is a kludge to fix the problem with camera
	// double-cuts in the cutscene (where we're trying to
	// interpolate from one camera to the next, rather
	// than just jumping)...  the way it works is that
	// 2 of these keys need to be inserted right before
	// a camera change, which will override the interpolated
	// camera values.  we need 2 because we're guaranteed
	// that the minimum time increment is 1/60th of a second
	// so we need to stick these keys in every 1/60th of
	// a second between the two cameras.  note that this
	// won't work properly if we do anim speed changes
	// or setslomo, because then the minimum time increment
	// is actually less than 1/60th of a second, then it's
	// possible that there will be a camera update, without
	// an accompanying CChangeCamera key found
	// i'd eventually like to look at how WithinRange() works, 
	// to see if there's a better way to do this.
	
	if ( m_frame == 0 )
	{
		// GJ KLUDGE:  there's an exporter bug that writes out extra camera RT
		// keys at the beginning of the cutscene.  this skips
		// those extra keys
		Dbg_Message( "Skipping Change Camera RT key @ %d!", m_frame );
		return false;
	}

	if ( Script::GetInt( CRCD(0x2f510f45,"moviecam_debug"), false ) )
	{
		Dbg_Message( "Processing Change Camera RT key @ %d!", m_frame );
		Dbg_Message( "Quat: %f %f %f %f", m_rot[X], m_rot[Y], m_rot[Z], m_rot[W] );
		Dbg_Message( "Trans: %f %f %f %f", m_trans[X], m_trans[Y], m_trans[Z], m_trans[W] );
	}

	Gfx::Camera* pCamera = (Gfx::Camera*)pObject;

	Mth::Vector& camPos = pCamera->GetPos();
	Mth::Matrix& camMatrix = pCamera->GetMatrix();
	
	// update the camera position
	camPos = m_trans;

	// update the camera orientation
	Mth::Vector nullTrans;
	nullTrans.Set(0.0f,0.0f,0.0f,1.0f);		// Mick: added separate initialization
	
	// QuatVecToMatrix destroys the input data, so need to pass in a temp variable
	Mth::Quat theQuat = m_rot;
	Mth::QuatVecToMatrix( &theQuat, &nullTrans, &camMatrix );

	// need this because of our 1-frame range check
	this->SetActive( true );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CChangeCameraRTKey::WithinRange( float startFrame, float endFrame, bool inclusive )
{
	// good for 1 frame...			  1/60th of a frame

	if ( inclusive )
	{
		if ( startFrame < endFrame )
		{
			// this end key business is used to fix a cutscene glitch
			// during camera cuts.  the problem is that the keys come
			// in sets of 3:
			// KEY[-2]    KEY[-1]    KEY[0]
			// old camera old camera new camera
			// we basically take the current frame (stored
			// in endFrame) and figure out which of the
			// three keys should handle it...  formerly,
			// all three keys handled it, effectively
			// causing the last key's camera data to be 
			// used.  to fix the glitch, we need to only
			// run the correct key...
			if ( m_endKey )
			{
				if ( endFrame > m_frame )
				{
					// let it interpolate
					return false;
				}
			}
			else
			{
				if ( m_frame < ( endFrame - 1.0f ) )
				{
					// there's an end key coming, so let someone else handle it
					return false;
				}
			}
  			
			// forwards
			return ( m_frame >= ( startFrame - 1.0f ) && m_frame <= endFrame );
		}
		else
		{
			// backwards
			return ( m_frame <= ( startFrame + 1.0f ) && m_frame >= endFrame );
		}
	}
	else
	{
		if ( startFrame < endFrame )
		{
			if ( m_endKey )
			{
				if ( endFrame > m_frame )
				{
					// let it interpolate
					return false;
				}
			}
			else
			{
				if ( m_frame < ( endFrame - 1.0f ) )
				{
					// there's an end key coming, so let someone else handle it
					return false;
				}
			}
  			
			// forwards
			return ( m_frame >= ( startFrame - 1.0f ) && m_frame < endFrame );
		}
		else
		{
			// backwards
			return ( m_frame <= ( startFrame + 1.0f ) && m_frame > endFrame );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CEmptyKey::CEmptyKey( int frame ) : CCustomAnimKey( frame )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CEmptyKey::process_key( Obj::CObject* pObject )
{
	// do nothing

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CEventKey::CEventKey( int frame, uint32 eventType, Script::CStruct* pEventParams ) : CCustomAnimKey( frame )
{
	m_eventType = eventType;
	mp_eventParams = new Script::CStruct;
	mp_eventParams->AppendStructure( pEventParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CEventKey::~CEventKey()
{
	if ( mp_eventParams )
	{
		delete mp_eventParams;
		mp_eventParams = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CEventKey::process_key( Obj::CObject* pObject )
{
	pObject->SelfEvent( m_eventType, mp_eventParams );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/				

CCreateObjectFromStructKey::CCreateObjectFromStructKey( int frame, uint32 objectStructName ) : CCustomAnimKey( frame ), m_objectStructName( objectStructName )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCreateObjectFromStructKey::process_key( Obj::CObject* pObject )
{
	if ( Script::GetInt( CRCD(0x2f510f45,"moviecam_debug"), false ) )
	{
		Dbg_Message( "Processing Create Object From Struct key (struct = %s) @ %d!", Script::FindChecksumName( m_objectStructName ), m_frame );
	}

	Script::CStruct* pObjectStruct = Script::GetStructure( m_objectStructName, Script::ASSERT );
	if ( pObjectStruct )
	{
		Script::RunScript( CRCD(0xbdf7f843,"CreateFromStructure"), pObjectStruct );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/				

CKillObjectFromStructKey::CKillObjectFromStructKey( int frame, uint32 objectStructName ) : CCustomAnimKey( frame ), m_objectStructName( objectStructName )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CKillObjectFromStructKey::process_key( Obj::CObject* pObject )
{
	if ( Script::GetInt( CRCD(0x2f510f45,"moviecam_debug"), false ) )
	{
		Dbg_Message( "Processing Kill Object From Struct key (script = %s) @ %d!", Script::FindChecksumName( m_objectStructName ), m_frame );
	}
	
	Script::CStruct* pObjectStruct = Script::GetStructure( m_objectStructName, Script::ASSERT );
	if ( pObjectStruct )
	{
		uint32 objectName;
		pObjectStruct->GetChecksum( CRCD(0xa1dc81f9,"name"), &objectName, Script::ASSERT );
		
		Obj::CObject* pObject = (Obj::CObject*)Obj::ResolveToObject( objectName );
		pObject->MarkAsDead();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCustomAnimKey* ReadCustomAnimKey( uint8** pData )
{
	// factory function for generating a custom key...

	// TODO:  might need to push the heap context

	SIntermediateCustomAnimKeyHeader theIntermediateHeader;
	memcpy(&theIntermediateHeader, *pData, sizeof(SIntermediateCustomAnimKeyHeader));
	*pData += sizeof(SIntermediateCustomAnimKeyHeader);

	switch ( theIntermediateHeader.keyType )
	{
		case vCHANGE_FOCAL_LENGTH:
			{
				float fov;
				memcpy(&fov, *pData, sizeof(float));				// May not be word-aligned in memory
				*pData += sizeof(float);
				return new CChangeFOVKey( theIntermediateHeader.timeStamp, fov );
			}
			break;
		case vRUN_SCRIPT:
			{
				uint32 script_name;
				memcpy(&script_name, *pData, sizeof(uint32));		// May not be word-aligned in memory
				*pData += sizeof(uint32);
				return new CRunScriptKey( theIntermediateHeader.timeStamp, script_name );
			}
			break;
		case vCHANGE_CAMERA_RT:
			{
				Mth::Quat theQuat;
				Mth::Vector theVector;
				memcpy(&theVector, *pData, sizeof(Mth::Vector));
				*pData += sizeof(Mth::Vector);
				memcpy(&theQuat, *pData, sizeof(Mth::Quat));
				*pData += sizeof(Mth::Quat);
				return new CChangeCameraRTKey( theIntermediateHeader.timeStamp, theQuat, theVector, false );
			}
			break;
		case vCHANGE_CAMERA_RT_ENDKEY:
			{
				Mth::Quat theQuat;
				Mth::Vector theVector;
				memcpy(&theVector, *pData, sizeof(Mth::Vector));
				*pData += sizeof(Mth::Vector);
				memcpy(&theQuat, *pData, sizeof(Mth::Quat));
				*pData += sizeof(Mth::Quat);
				return new CChangeCameraRTKey( theIntermediateHeader.timeStamp, theQuat, theVector, true );
			}
			break;
		case vCHANGE_CAMERA_RT_IGNORE:
			{
           		// GJ:  There's a bug in the exporter where
				// 2 camera RT keys with different positions
				// are listed for the same time...  this causes
				// a glitch during certain camera transitions.
				// To fix, I will replace the second key with one
				// that does nothing...
				*pData += sizeof(Mth::Vector);
				*pData += sizeof(Mth::Quat);

				return new CEmptyKey( theIntermediateHeader.timeStamp );
			}
			break;
		case vCREATE_OBJECT_FROM_STRUCT:
			{
				uint32 object_struct_name;
				memcpy(&object_struct_name, *pData, sizeof(uint32));		// May not be word-aligned in memory
				*pData += sizeof(uint32);
				return new CCreateObjectFromStructKey( theIntermediateHeader.timeStamp, object_struct_name );
			}
			break;
		case vKILL_OBJECT_FROM_STRUCT:
			{
				uint32 object_struct_name;
				memcpy(&object_struct_name, *pData, sizeof(uint32));		// May not be word-aligned in memory
				*pData += sizeof(uint32);
				return new CKillObjectFromStructKey( theIntermediateHeader.timeStamp, object_struct_name );
			}
			break;
		default:
			{
				Dbg_Message( "Warning:  Ignoring custom anim key (type %08x is currently unsupported).", theIntermediateHeader.keyType );

				// just skip past the size
				Dbg_Assert( theIntermediateHeader.size - sizeof(SIntermediateCustomAnimKeyHeader) < 512 );
				*pData += theIntermediateHeader.size;
				Dbg_Assert(!((uint) *pData & 0x3));

				return NULL;
			}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx
