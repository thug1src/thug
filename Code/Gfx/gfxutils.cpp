//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       GfxUtils.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  02/01/2001
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/gfxutils.h>

#include <core/math.h>
#include <core/math/vector.h>

#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>

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
**							   PrivateFunctions								**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{
	float min, max, delta;
	min = Mth::Min3( r, g, b );
	max = Mth::Max3( r, g, b );
	*v = max;				// v
	delta = max - min;
	if( max != 0.0f )
		*s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0.0f;
		*h = -1.0f;
		return;
	}

	// GJ:
	if (delta == 0.0f)
		return;

	if( r == max )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		*h = 2.0f + ( b - r ) / delta;	// between cyan & yellow
	else
		*h = 4.0f + ( r - g ) / delta;	// between magenta & cyan
	*h *= 60.0f;				// degrees
	if( *h < 0.0f )
		*h += 360.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;
	if( s == 0.0f ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}
	h /= 60.0f;			// sector 0 to 5
	i = (int)h;			// basically, the floor
	f = h - i;			// factorial part of h
	p = v * ( 1.0f - s );
	q = v * ( 1.0f - s * f );
	t = v * ( 1.0f - s * ( 1.0f - f ) );
	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float FRAMES_TO_TIME(int frames)
{
	return (frames / 60.0f);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int TIME_TO_FRAMES(float time)
{
	return (int)(time * 60.0f + 0.5f);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void GetModelFromFileName ( char* filename, char* pModelNameBuf )
{
	char* pModelName = NULL;
	while (*filename)
	{
		if (*filename == '\\' || *filename == '/')
		{
			pModelName = filename + 1;
		}
		filename++;
	}

	Dbg_MsgAssert ( pModelName,( "Not full path name" ));

	strcpy(pModelNameBuf, pModelName);

	// strip out extension
	for (unsigned int i = 0; i < strlen(pModelNameBuf); i++)
	{
		if (pModelNameBuf[i] == '.')
		{
			pModelNameBuf[i] = 0;
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool GetScaleFromParams( Mth::Vector* pScaleVector, Script::CStruct* pParams )
{
	if ( pParams->ContainsComponentNamed( CRCD(0x13b9da7b,"scale") ) )
	{
		float scaleValue;
		if ( pParams->GetFloat( CRCD(0x13b9da7b,"scale"), &scaleValue, Script::NO_ASSERT ) )
		{
			*pScaleVector = Mth::Vector( scaleValue, scaleValue, scaleValue );
			return true;
		}
		else if ( pParams->GetVector( CRCD(0x13b9da7b,"scale"), pScaleVector, Script::NO_ASSERT ) )
		{
			return true;
		}
		else
		{
			Dbg_MsgAssert( 0, ( "Scale should be either a float or a vector" ) );
		}
	}
		
	bool xFound = pParams->GetFloat( CRCD(0x7323e97c,"x"), &(*pScaleVector)[X], false );
	bool yFound = pParams->GetFloat( CRCD(0x424d9ea,"y"), &(*pScaleVector)[Y], false );
	bool zFound = pParams->GetFloat( CRCD(0x9d2d8850,"z"), &(*pScaleVector)[Z], false );

	return ( xFound || yFound || zFound );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool GetLoopingTypeFromParams( Gfx::EAnimLoopingType* pLoopingType, Script::CStruct* pParams )
{
	*pLoopingType = Gfx::LOOPING_HOLD;
	
	if ( pParams->ContainsFlag( CRCD(0x4f792e6c,"Cycle") ) )
	{
		*pLoopingType=Gfx::LOOPING_CYCLE;
	}
	else if ( pParams->ContainsFlag( CRCD(0x3153e314,"PingPong") ) )
	{
		*pLoopingType=Gfx::LOOPING_PINGPONG;
	}
	else if ( pParams->ContainsFlag( CRCD(0x6d941203,"Wobble") ) )
	{
		*pLoopingType=Gfx::LOOPING_WOBBLE;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool GetTimeFromParams( float* pStart, float* pEnd, float Current, float Duration, Script::CStruct* pParams, Script::CScript* pScript )
{
	float From = 0.0f;
	float To = Duration;

	uint32 FromChecksum=0;
	if ( pParams->GetChecksum( CRCD(0x46e55e8f,"From"), &FromChecksum ) )
	{
		switch (FromChecksum)
		{
		case 0x6086aa70: // Start
			From=0;
			break;
		case 0xff03cc4e: // End
			From=Duration;
			break;
		case 0x230ccbf4: // Current
			From=Current;
			break;
		case 0x617fe530: // Middle
			From=Duration / 2.0f;
			break;	
		default:
			Dbg_MsgAssert(0,("\n%s\nUnrecognized value '%s' for From in PlayAnim",pScript?pScript->GetScriptInfo():"???",Script::FindChecksumName(FromChecksum)));
			break;
		}
	}		
	
	uint32 ToChecksum=0;
	if (pParams->GetChecksum( CRCD(0x28782d3b,"To"), &ToChecksum) )
	{
		switch (ToChecksum)
		{
		case 0x6086aa70: // Start
			To=0;
			break;
		case 0xff03cc4e: // End
			To=Duration;
			break;
		case 0x230ccbf4: // Current
			To=Current;
			break;	
		case 0x617fe530: // Middle
			To=Duration / 2.0f;
			break;	
		default:
			Dbg_MsgAssert(0,("\n%s\nUnrecognized value '%s' for To in PlayAnim",pScript?pScript->GetScriptInfo():"???",Script::FindChecksumName(ToChecksum)));
			break;
		}
	}		
	
	// also check if From and To were specified as integers, in which case use units of 60ths.
	float FromFrames=0;
	if (pParams->GetFloat( CRCD(0x46e55e8f,"From"), &FromFrames ) )
	{
		if ( pParams->ContainsFlag( CRCD(0xd029f619,"seconds") ) )
		{
			From = FromFrames;
		}
		else
		{
			From = Gfx::FRAMES_TO_TIME((int)FromFrames);
		}
	}
		
	float ToFrames=0;
	if ( pParams->GetFloat( CRCD(0x28782d3b,"To"), &ToFrames ) )
	{
		if ( pParams->ContainsFlag( CRCD(0xd029f619,"seconds") ) )
		{
			To = ToFrames;
		}
		else
		{
			To = Gfx::FRAMES_TO_TIME((int)ToFrames);
		}
	}
	
	if ( pParams->ContainsFlag( CRCD(0xf8cfd515,"Backwards") ) )
	{
		float Temp=From;
		From=To;
		To=Temp;
	}	

	*pStart = From;
	*pEnd = To;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Str::String GetModelFileName(const char* pName, const char* pExtension)
{
	char fullName[512];

	if ( strstr( pName, "/" ) || strstr( pName, "\\" ) )
	{
		Dbg_MsgAssert( strstr( pName, "." ), ( "Filename %s is missing extension", pName ) ); 
		
		sprintf( fullName, "models\\%s", pName ); 
	}
	else
	{
		sprintf( fullName, "models\\%s\\%s%s", pName, pName, pExtension );
	}

#ifdef __NOPT_ASSERT__
	// replace all forward slashes with backslashes
	char temp[512];
	strcpy( temp, fullName );
	char* pString = temp;
	while ( *pString )
	{
		if ( *pString == '/' )
		{
			*pString = '\\';
		}
		pString++;
	}
	// look for double backslashes, which are bad
	Dbg_MsgAssert( !strstr( temp, "\\\\" ), ( "Filename %s has double backslash", temp ) ); 
#endif

	return Str::String( fullName );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void GetModelFileName(const char* pName, const char* pExtension, char* pTarget)
{
	if ( strstr( pName, "/" ) || strstr( pName, "\\" ) )
	{
		Dbg_MsgAssert( strstr( pName, "." ), ( "Filename %s is missing extension", pName ) ); 
		
		Dbg_MsgAssert( pTarget, ( "No target buffer" ) );
		sprintf( pTarget, "models\\%s", pName ); 
	}
	else
	{
		Dbg_MsgAssert( pTarget, ( "No target buffer" ) );
		sprintf( pTarget, "models\\%s\\%s%s", pName, pName, pExtension );
	}
	
#ifdef __NOPT_ASSERT__
	// replace all forward slashes with backslashes
	char temp[512];
	strcpy( temp, pTarget );
	char* pString = temp;
	while ( *pString )
	{
		if ( *pString == '/' )
		{
			*pString = '\\';
		}
		pString++;
	}
	// look for double backslashes, which are bad
	Dbg_MsgAssert( !strstr( temp, "\\\\" ), ( "Filename %s has double backslash", temp ) ); 
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx

