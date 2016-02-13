//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       ModelAppearance.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  4/2/2000
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/modelappearance.h>

#include <gfx/casutils.h>
#include <gfx/facetexture.h>
#include <gfx/modelbuilder.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/component.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Gfx
{

// NOTES:

// A CModelAppearance contains one script structure, whose
// contents look something like this:
//	{
//		head			= { desc_id=#"Andrew Reynolds" h = 0 s = 50 v = 100 use_default_hsv = 1 }
//		torso			= { desc_id=#"Long Sleeve - Collar" h = 0 s = 50 v = 100 use_default_hsv = 1 }
//		legs			= { desc_id=#"Reynolds's Pants" h = 0 s = 50 v = 100 use_default_hsv = 1 }
//		shoes			= { desc_id=#"Reynolds's" h = 0 s = 50 v = 100 use_default_hsv = 1 }
//		boardup			= { desc_id=#"Solid" }	
//		boarddown		= { desc_id=#"Green Brand Logo" h = 0 s = 50 v = 100 use_default_hsv = 1 }
//		scale			= 1.02
//		weight_scale	= 0.97
//	}

// Basically, it's a bunch of "virtual structures", each 
// assigned to a "part checksum" (head, torso, etc.).  
// Each part checksum corresponds to the name of a global
// array containing a list of read-only "actual structures".
// Given a part checksum and a desc_id in the CModelAppearance, 
// we can lookup the appropriate global array and find the 
// appropriate actual structure.

// CModelAppearance is really just a wrapper around the
// CStruct class, but after some consideration,
// I decided not to subclass from it.  This is so that
// I'd be able to replace the underlying implementation
// while keeping the same interface.

// There shouldn't be anything skater- (or even player-)
// specific hardcoded here.  The intent is that we use
// this same class for any kind of customizable model
// (such as Create-a-Peds, Create-a-Cars, or even
// Create-a-Spaceships for future games).
	
// The CModelAppearance should have no knowledge of the full
// list of possible part checksums;  this list should be
// completely in script.  This implies that the code should
// never have to iterate through a list of part checksums,
// (unlike THPS3, which had many explicit references to
// "editable_cas_options", Cas::GetBodyPartCount(), and
// Cas::GetBodyPartName()).

// One of the reset types used to be "randomized", but
// that is something that is more appropriate at the skater
// profile level (or, even better, in script), which has
// a better understanding of what parts should be disqualified 
// from working with a particular skater instance.  If 
// randomization were to be implemented at this level, it should
// be a purely naive randomization (no weighting, no part
// disqualification).
	
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
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 get_desc_id_from_structure( Script::CStruct* pStructure )
{
	// This function is purely for convenience.  It grabs
	// the "desc_id" field from the supplied structure.
	
	uint32 descId = 0;
	pStructure->GetChecksum( CRCD(0x4bb2084e,"desc_id"), &descId, Script::NO_ASSERT );
	return descId;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelAppearance::resolve_randomized_desc_ids()
{
	// for each structure component, see whether
	// it's got any randomized desc ids.  if so,
	// select one to be used every time this
	// appearance instance is used

	// sometimes the skin tone must be consistent 
	// among all the body parts...  if any structure
	// contains a "random_set" checksum, then
	// make sure all future random parts get
	// this selected as well
	uint32 random_set = 0;

	Script::CComponent* p_comp = m_appearance.GetNextComponent( NULL );
	while ( p_comp )
	{
		Script::CComponent* p_next = m_appearance.GetNextComponent( p_comp );		
		
		if ( p_comp->mType == ESYMBOLTYPE_STRUCTURE )
		{
			uint32 partChecksum = p_comp->mNameChecksum;
			int randomIndex;

			if ( p_comp->mpStructure->ContainsFlag( CRCD(0xd81c03b0,"randomized_desc_id") ) )
			{
				Script::CStruct* pActualStruct = Cas::GetRandomOptionStructure( partChecksum, random_set );
				Dbg_MsgAssert( pActualStruct, ( "Unrecognized part checksum to randomize %s", Script::FindChecksumName(partChecksum) ) );
				
				// remember the random_set, if any
				pActualStruct->GetChecksum( CRCD(0x0d7260fd,"random_set"), &random_set, Script::NO_ASSERT );
				
				p_comp->mpStructure->Clear();
				p_comp->mpStructure->AddChecksum( CRCD(0x4bb2084e,"desc_id"), get_desc_id_from_structure(pActualStruct) );
			}
			else if ( p_comp->mpStructure->GetInteger( CRCD(0x4b833e64,"random_index"), &randomIndex, Script::NO_ASSERT ) )
			{
				Script::CStruct* pActualStruct = Cas::GetRandomOptionStructureByIndex( partChecksum, randomIndex, random_set );
				Dbg_MsgAssert( pActualStruct, ( "Unrecognized part checksum to randomize %s", Script::FindChecksumName(partChecksum) ) );
				
				// remember the random_set, if any
				pActualStruct->GetChecksum( CRCD(0x0d7260fd,"random_set"), &random_set, Script::NO_ASSERT );
				
				p_comp->mpStructure->Clear();
				p_comp->mpStructure->AddChecksum( CRCD(0x4bb2084e,"desc_id"), get_desc_id_from_structure(pActualStruct) );
			}
		}

		p_comp = p_next;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelAppearance::clear_part(uint32 partChecksum)
{
	m_appearance.RemoveComponent(partChecksum);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelAppearance::set_part(uint32 partChecksum, uint32 descID, Script::CStruct* pParams)
{
	Script::CStruct* pStruct;
	
	// if the structure doesn't already exist, then add it...
	if ( !m_appearance.GetStructure(partChecksum, &pStruct) )
	{
		pStruct = new Script::CStruct;
		m_appearance.AddComponent(partChecksum, ESYMBOLTYPE_STRUCTUREPOINTER, (int)pStruct);
	}

	pStruct->Clear();

	// check for extra color parameters
	int use_default_hsv = 1;
	pParams->GetInteger( CRCD(0x97dbdde6,"use_default_hsv"), &use_default_hsv, Script::NO_ASSERT );
	
	if ( !use_default_hsv )
	{
		int h, s, v;
		pParams->GetInteger( CRCD(0x6e94f918,"h"), &h, Script::ASSERT );
		pParams->GetInteger( CRCD(0xe4f130f4,"s"), &s, Script::ASSERT );
		pParams->GetInteger( CRCD(0x949bc47b,"v"), &v, Script::ASSERT );
		
		pStruct->AddInteger( CRCD(0x97dbdde6,"use_default_hsv"), 0 );
		pStruct->AddInteger( CRCD(0x6e94f918,"h"), h );
		pStruct->AddInteger( CRCD(0xe4f130f4,"s"), s );
		pStruct->AddInteger( CRCD(0x949bc47b,"v"), v );
	}

	pStruct->AddChecksum( CRCD(0x4bb2084e,"desc_id"), descID );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelAppearance::set_checksum(uint32 fieldChecksum, uint32 valueChecksum)
{
	m_appearance.AddChecksum( fieldChecksum, valueChecksum );
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CModelAppearance::CModelAppearance( void )
{
	mp_faceTexture = NULL;

	m_willEventuallyHaveFaceTexture = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelAppearance::CModelAppearance( const CModelAppearance& rhs )
{
	mp_faceTexture = NULL;

	// use the overridden assignment operator
	*this = rhs;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CModelAppearance::~CModelAppearance()
{
	if ( mp_faceTexture )
	{
		delete mp_faceTexture;
		mp_faceTexture = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelAppearance& CModelAppearance::operator=( const CModelAppearance& rhs )
{
	if ( &rhs == this )
	{
		return *this;
	}

	// it shouldn't be necessary to define this function, as
	// this is what the default assignment operator is supposed
	// to do.  However, the compiler gives me warnings if I
	// don't define it ("statement with no effect").
	m_appearance = rhs.m_appearance;

	// get rid of old face texture
	if ( mp_faceTexture )
	{
		delete mp_faceTexture;
		mp_faceTexture = NULL;

	}

	if ( rhs.mp_faceTexture )
	{
		CreateFaceTexture();
		Dbg_Assert( mp_faceTexture );
		*mp_faceTexture = *rhs.mp_faceTexture;
	}

	m_willEventuallyHaveFaceTexture = rhs.m_willEventuallyHaveFaceTexture;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelAppearance::Init()
{
	m_appearance.Clear();

	if ( mp_faceTexture )
	{
		// reset the face texture
		mp_faceTexture->SetValid( false );
	}

	// everyone should have certain items defined, 
	// such as sleeves for doing sleeve colors.
	Script::CStruct* pResetStructure = Script::GetStructure( CRCD(0xfe54486d,"appearance_init_structure"), Script::NO_ASSERT );
	if ( pResetStructure )
	{
		m_appearance.AppendStructure( pResetStructure );
	}

	m_willEventuallyHaveFaceTexture = false;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelAppearance::Load( Script::CStruct* pStructure, bool resolve_randoms )
{
	Dbg_Assert( pStructure );

	// In THPS3, we didn't clear the m_appearance first when
	// loading from a memory card, so that if the saved data is 
	// missing anything, it won't matter because the default 
	// will already be in m_appearance.  (This prevents asserts
	// on autoloading when a new component has been added to 
	// m_appearance which is not present on the memory card)
	// However, for THPS4, the appearances should be a lot more
	// flexible, and should no longer fail when there are missing items.
	Init();

#if 1
	// in case there are any global structure names,
	// resolve them
	pStructure->ExpandInto(	&m_appearance, 0 );
#else
	// add the new desired data
	m_appearance.AppendStructure( pStructure );
#endif
	
   // at this point, all the randomized_desc_ids
	// should be resolved
	if ( resolve_randoms )
	{
		resolve_randomized_desc_ids();
	}
		
#ifdef	__NOPT_ASSERT__
	// Make sure that the m_appearance does not contain any flag members,
	// which would potentially cause leaks as we append any structures.
	uint32 dummy = 0;
	Dbg_MsgAssert( !m_appearance.GetChecksum( NONAME, &dummy ), ( "m_appearance contains a flag '%s'", Script::FindChecksumName(dummy) ) );
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelAppearance::Load( uint32 structure_name, bool resolve_randoms )
{
	Script::CStruct* pStructure;

	pStructure = Script::GetStructure( structure_name, Script::ASSERT );

	return Load( pStructure, resolve_randoms );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifndef __PLAT_NGC__
void compress_model_appearance( Script::CStruct* pStruct )
{
	// the worst-case is too big to fit into a network packet
	// so this will remove some of the more unnecessary items
	// from the structure.
	pStruct->RemoveComponent( CRCD(0xb1d19000,"deck_layer1") );
	pStruct->RemoveComponent( CRCD(0x28d8c1ba,"deck_layer2") );
	pStruct->RemoveComponent( CRCD(0x5fdff12c,"deck_layer3") );
	pStruct->RemoveComponent( CRCD(0xc1bb648f,"deck_layer4") );
	pStruct->RemoveComponent( CRCD(0xb6bc5419,"deck_layer5") );

	pStruct->RemoveComponent( CRCD(0x3fa9b96e,"head_tattoo") );
	pStruct->RemoveComponent( CRCD(0x283dea37,"left_bicep_tattoo") );
	pStruct->RemoveComponent( CRCD(0xde55864b,"left_forearm_tattoo") );
	pStruct->RemoveComponent( CRCD(0xd408fa96,"right_bicep_tattoo") );
	pStruct->RemoveComponent( CRCD(0x74fee7b2,"right_forearm_tattoo") );
	pStruct->RemoveComponent( CRCD(0x8eba2bc9,"chest_tattoo") );
	pStruct->RemoveComponent( CRCD(0x233b7bba,"back_tattoo") );
	pStruct->RemoveComponent( CRCD(0x2b359a94,"left_leg_tattoo") );
	pStruct->RemoveComponent( CRCD(0x609432ca,"right_leg_tattoo") );
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CModelAppearance::WriteToBuffer(uint8 *pBuffer, uint32 BufferSize, bool ignoreFaceData )
{
	// for network message building
	// (note that the face texture is not included in this buffer
	// because the buffer is too small...  face textures will
	// be sent in a separate net packet)
	Script::CStruct* pTempStructure = new Script::CStruct;
	pTempStructure->AppendStructure( &m_appearance );
	uint32 size = Script::WriteToBuffer(pTempStructure, pBuffer, BufferSize);

#ifndef __PLAT_NGC__
	// GJ:  Need to compress the model appearances for Xbox and PS2
	// versions, or else the worst-case model appearance will crash
	// the server if he quits his own game (because it overflows the
	// max net packet size).  MAX_MODEL_APPEARANCE_SIZE is a guess 
	// based on the existing model appearance size, but it really
	// depends on how big the info/tricks get...  700 bytes seems safe
	const uint32 MAX_MODEL_APPEARANCE_SIZE = 700;
	if ( size > MAX_MODEL_APPEARANCE_SIZE )
	{
		// if the model appearance is too big, then	strip out
		// some of the cosmetic stuff...
		compress_model_appearance( pTempStructure );
		size = Script::WriteToBuffer(pTempStructure, pBuffer, BufferSize);
	}
#endif

	delete pTempStructure;

	// skip to the next chunk
	pBuffer += size;
	BufferSize -= size;

	// we do, however, still need to send a flag that says it
	// will eventually have a face texture...  so that the model 
	// builder knows to use the face-mapped head rather than
	// the non-facemapped head
	if( ignoreFaceData )
	{
		
		*pBuffer = 0;
	}
	else
	{
		*pBuffer = ( ( GetFaceTexture() && GetFaceTexture()->IsValid() ) || m_willEventuallyHaveFaceTexture ) ? 1 : 0;		
	}
	
	pBuffer += 1;
	BufferSize -= 1;
	size += 1;
	
	return size;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8* CModelAppearance::ReadFromBuffer(uint8 *pBuffer)
{
	Script::CStruct* pTempStructure = new Script::CStruct;
	pBuffer = Script::ReadFromBuffer( pTempStructure, pBuffer );
	m_appearance.Clear();
	m_appearance.AppendStructure( pTempStructure );
	delete pTempStructure;
	
	// we do, however, still need to send a flag that says it
	// will eventually have a face texture...  so that the model 
	// builder knows to use the face-mapped head rather than
	// the non-facemapped head
	m_willEventuallyHaveFaceTexture = *pBuffer;
	pBuffer++;
	
	return pBuffer;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelAppearance::PrintContents( const Script::CStruct* p_structure )
{
#ifdef __NOPT_ASSERT__
	//Script::PrintContents( &m_appearance );

	if(!p_structure)
	{
		p_structure = &m_appearance;
	}
	//const Script::CStruct* p_structure= &m_appearance;

	Dbg_MsgAssert(p_structure,("NULL p_structure"));
	  
	//printf(" ");
	
	
	printf("{");
			
    Script::CComponent *p_comp=p_structure->GetNextComponent(NULL);

    while (p_comp)
    {
		
		if (p_comp->mNameChecksum)
		{
			printf(" %s=",Script::FindChecksumName(p_comp->mNameChecksum));
		}	
            
        switch (p_comp->mType)
        {
        case ESYMBOLTYPE_INTEGER:
            printf("%d",p_comp->mIntegerValue);
            break;
        case ESYMBOLTYPE_FLOAT:
            printf("%f",p_comp->mFloatValue);
            break;
        case ESYMBOLTYPE_STRING:
            printf("#\"%s\"",p_comp->mpString);
            break;
        case ESYMBOLTYPE_LOCALSTRING:
            printf("'%s'",p_comp->mpLocalString);
            break;
		/*case ESYMBOLTYPE_PAIR:
            printf("(%f,%f) ",p_comp->mpPair->mX,p_comp->mpPair->mY);
            break;
        case ESYMBOLTYPE_VECTOR:
            printf("(%f,%f,%f) ",p_comp->mpVector->mX,p_comp->mpVector->mY,p_comp->mpVector->mZ);
            break;*/
        case ESYMBOLTYPE_STRUCTURE:
			//printf(" ");
			CModelAppearance::PrintContents(p_comp->mpStructure);
            break;
        case ESYMBOLTYPE_NAME:
            printf("#\"%s\"",Script::FindChecksumName(p_comp->mChecksum));
			
			#ifdef EXPAND_GLOBAL_STRUCTURE_REFERENCES
			if (p_comp->mNameChecksum==0)
			{
				// It's an un-named name. Maybe it's a global structure ... 
				// If so, print its contents too, which is handy for debugging.
			    CSymbolTableEntry *p_entry=Resolve(p_comp->mChecksum);
				if (p_entry && p_entry->mType==ESYMBOLTYPE_STRUCTURE)
				{
					printf("... Defined in %s ...\n",Script::FindChecksumName(p_entry->mSourceFileNameChecksum));
					Dbg_MsgAssert(p_entry->mpStructure,("NULL p_entry->mpStructure"));
					CModelAppearance::PrintContents(p_entry->mpStructure);
				}
			}		
			#endif
            break;
        case ESYMBOLTYPE_QSCRIPT:
			printf("(A script) "); // TODO
			break;
		case ESYMBOLTYPE_ARRAY:
			//printf(" ");
			Script::PrintContents(p_comp->mpArray,0);
            break;
        default:
			printf("Component of type '%s', value 0x%08x\n",Script::GetTypeName(p_comp->mType),p_comp->mUnion);
            //Dbg_MsgAssert(0,("Bad p_comp->Type"));
            break;
        }
        p_comp=p_structure->GetNextComponent(p_comp);
		
		#ifdef SLOW_DOWN_PRINTCONTENTS
		// A delay to let printf catch up so that it doesn't skip stuff when printing big arrays.		
		for (int i=0; i<1000000; ++i);
		#endif
    }
	
	printf("}\n");


	/*if ( mp_faceTexture && mp_faceTexture->IsValid() )
	{
		mp_faceTexture->PrintContents();
	}*/
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CModelAppearance::GetActualDescStructure( uint32 partChecksum )
{
	Script::CStruct* pStructure;
	if ( m_appearance.GetStructure( partChecksum, &pStructure, Script::NO_ASSERT ) )
	{
		return Cas::GetOptionStructure( partChecksum, get_desc_id_from_structure( pStructure ), false );
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CModelAppearance::GetVirtualDescStructure( uint32 partChecksum )
{
	Script::CStruct* pStructure = NULL;
	m_appearance.GetStructure( partChecksum, &pStructure, Script::NO_ASSERT );
	return pStructure;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelAppearance::CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( checksum )
	{
		case 0x0a23400c:		// ClearPart
		{
			Dbg_MsgAssert(pParams, ("No params supplied to model appearance"));

			uint32 partChecksum;
			pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );
			clear_part(partChecksum);
		}
		return true;
		
		case 0x83339d0b:		// SetPart
		{
			Dbg_MsgAssert(pParams, ("No params supplied to model appearance"));

			uint32 partChecksum;
			pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );
	
			uint32 descChecksum;
			pParams->GetChecksum( CRCD(0x4bb2084e,"desc_id"), &descChecksum, Script::ASSERT );
			
			set_part(partChecksum, descChecksum, pParams);
		}
		return true;

		case 0x10a225d6:		// GetPart
		{
			Dbg_MsgAssert(pParams, ("No params supplied to model appearance"));
		
			uint32 partChecksum;
			pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );
	
			Script::CStruct* pVirtualDescStructure = GetVirtualDescStructure( partChecksum );

			if ( pVirtualDescStructure )
			{
				// return all the parameters, including desc_id, use_default_hsv, h, s, v
				pScript->GetParams()->AppendStructure( pVirtualDescStructure );
				return true;
			}

			return false;
		}
		return true;

		case 0xd27427ff:		// SetChecksum
		{
			Dbg_MsgAssert(pParams, ("No params supplied to model appearance"));

			uint32 fieldChecksum;
			pParams->GetChecksum( CRCD(0xa40abaa7,"field"), &fieldChecksum, Script::ASSERT );
	
			uint32 valueChecksum;
			pParams->GetChecksum( CRCD(0xe288a7cb,"value"), &valueChecksum, Script::ASSERT );
			
			set_checksum(fieldChecksum, valueChecksum);
		}
		return true;

		case 0xb13906b0:		// GotPart
		{
			uint32 partChecksum;
			pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );
			return ( GetActualDescStructure( partChecksum ) );
		}

		case 0xe961bafa:		// PartGotFlag
		{
			uint32 partChecksum;
			pParams->GetChecksum( CRCD(0xb6f08f39,"part"), &partChecksum, Script::ASSERT );
			uint32 flagChecksum;
			pParams->GetChecksum( CRCD(0x2e0b1465,"flag"), &flagChecksum, Script::ASSERT );

			Script::CStruct* pActualDescStructure = GetActualDescStructure( partChecksum );

			if ( pActualDescStructure )
			{
				return pActualDescStructure->ContainsFlag( flagChecksum );
			}

			Dbg_MsgAssert( 0, ( "part %s was not defined (need it for disqualification script)", Script::FindChecksumName(partChecksum) ) );

			return false;
		}
		return true;
	}

	return Obj::CObject::CallMemberFunction(checksum, pParams, pScript);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFaceTexture* CModelAppearance::GetFaceTexture()
{
	return mp_faceTexture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
											  
void CModelAppearance::CreateFaceTexture()
{
	Dbg_MsgAssert( !mp_faceTexture, ( "Model appearance already has a face texture" ) );
	
	// the face texture should always go on the skater info heap
	// these may be permanent, or they might be temporary...
	mp_faceTexture = new CFaceTexture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
											  
void CModelAppearance::DestroyFaceTexture()
{
	if ( mp_faceTexture )
	{
		delete mp_faceTexture;
		mp_faceTexture = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CModelAppearance::WillEventuallyHaveFaceTexture()
{
	return m_willEventuallyHaveFaceTexture;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx
