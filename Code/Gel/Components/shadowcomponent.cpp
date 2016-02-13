//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ShadowComponent.cpp
//* OWNER:          gj
//* CREATION DATE:  2/06/03
//****************************************************************************

#include <gel/components/shadowcomponent.h>

#include <gel/components/modelcomponent.h>
#include <gel/components/motioncomponent.h>

#include <gel/object/compositeobject.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>
#include <sk/scripting/cfuncs.h>

#include <gfx/nxmodel.h>
#include <gfx/nxlight.h>
#include <gfx/shadow.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// This static function is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
CBaseComponent* CShadowComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CShadowComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CShadowComponent::CShadowComponent() : CBaseComponent()
{
	SetType( CRC_SHADOW );

	// no shadow object
	mp_shadow = NULL;					
	m_shadowNormal.Set(0.0f,1.0f,0.0f);
	m_shadowType = CRCD(0x806fff30,"none");
	m_shadowPos.Set(0,0,0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CShadowComponent::~CShadowComponent()
{
	if ( mp_shadow )
	{
		delete mp_shadow;
		mp_shadow = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CShadowComponent::Finalize()
{
	mp_model_component = GetModelComponentFromObject( GetObject() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CShadowComponent::Teleport()
{									 
	// GJ:  the shadow component's Update()
	// function doesn't really require finalization
	// right now, but having this assert in here
	// might help catch future errors
	Dbg_MsgAssert( GetObject()->IsFinalized(), ( "Has not been finalized!  Tell Gary!" ) );

	Update();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CShadowComponent::InitFromStructure( Script::CStruct* pParams )
{
	m_shadowType = CRCD(0x3e84c2fd,"simple");
	pParams->GetChecksum( CRCD(0x9ac24b18,"ShadowType"), &m_shadowType, Script::NO_ASSERT );

	if ( mp_shadow )
	{
		// get rid of existing shadow, in case it was the wrong type...
		SwitchOffShadow();
	}

	switch ( m_shadowType )
	{
		case 0x3e84c2fd:	// simple
			{
				if ( !mp_shadow )
				{
					SwitchOnShadow( Gfx::vSIMPLE_SHADOW );
				}
				
				float scale = 1.0f;
				pParams->GetFloat( CRCD(0x6f8cd62f,"ShadowScale"), &scale, Script::NO_ASSERT );
				
				const char *p_shadow_model_name = "Ped_Shadow";
				pParams->GetString( CRCD(0x545f8172,"ShadowModel"), &p_shadow_model_name, Script::NO_ASSERT );
						
				Dbg_MsgAssert( mp_shadow, ("NULL mp_shadow") );
				Gfx::CSimpleShadow* pSimpleShadow = (Gfx::CSimpleShadow*)mp_shadow;
				pSimpleShadow->SetScale( scale );
				pSimpleShadow->SetModel( p_shadow_model_name );

				// GJ:  need to immediately change the shadow's position if Obj_ShadowOn gets called
				// this is because sometimes the shadow component will be suspended, and so
				// update_shadow() won't get called (fixes shadow appearing at the origin in SC2)
				pSimpleShadow->UpdatePosition( m_shadowPos, GetObject()->m_matrix, m_shadowNormal );
			}
			break;

		case 0x76a54cd1:	// detailed
			{
				if ( !mp_shadow )
				{
					SwitchOnShadow( Gfx::vDETAILED_SHADOW );
				}
			}
			break;

		case 0x806fff30:	// none
			{
				if ( mp_shadow )
				{
					SwitchOffShadow();
				}
			}
			break;

		default:
			Dbg_MsgAssert( 0, ( "Unrecognized shadow type %s", Script::FindChecksumName(m_shadowType) ) );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CShadowComponent::update_shadow()
{
	if ( mp_shadow )
	{
//		Dbg_MsgAssert(mp_shadow->GetShadowType()!=Gfx::vDETAILED_SHADOW,("Tried to update a detailed shadow in MovingObj_Update, will need to change code to send a higher-up position"));

		
		Nx::CModel* pModel = NULL;
		if ( mp_model_component )
		{
			pModel = mp_model_component->GetModel();
		}
		
		// if we have a model and the model is not active,then don't need to update the shadwo
		if (pModel)
		{
			if (pModel->GetActive())
			{
				switch( mp_shadow->GetShadowType() )
				{
					case Gfx::vDETAILED_SHADOW:
					{
		//				Mth::Vector shadow_target_pos = pos + ( matrix.GetUp() * 36.0f );
						Mth::Vector shadow_target_pos = GetObject()->m_pos + ( GetObject()->m_matrix.GetUp() * 36.0f );

		#ifdef __PLAT_XBOX__
						// K: Moved this in here cos it was giving an unused-variable compile error on PS2
						Mth::Vector ground_dir( 0.2f, -0.8f, 0.3f );

						// If lights are active, set the ground direction to be that of the primary light.
						if( pModel )
						{
							Nx::CModelLights* p_lights = pModel->GetModelLights();
							if( p_lights )
							{
								ground_dir = p_lights->GetLightDirection( 0 ) * -1.0f;
							}
						}
		//				mp_shadow->UpdateDirection( ground_dir );
		#endif	
						mp_shadow->UpdatePosition( shadow_target_pos );
					}
					break;

					case Gfx::vSIMPLE_SHADOW:
					{
						if ( pModel )
						{
							((Gfx::CSimpleShadow*)mp_shadow)->SetScale( pModel->GetScale().GetX() );
						}	

						/*
						TODO:  Commented this section out, because it 
						references m_jump_start_pos, which isn't accessible
						from the shadow code yet...

						if ( GetMotionComponentFromObject( GetObject() )->m_movingobj_status & MOVINGOBJ_STATUS_JUMPING )
						{
							// If jumping, use the jump's start-y so that the shadow stays on the ground.	
							Mth::Vector p=GetObject()->m_pos;
							p[Y]=m_jump_start_pos[Y];
							mp_shadow->UpdatePosition(p,GetObject()->m_matrix,m_shadowNormal);
						}
						else
						*/
						{
							mp_shadow->UpdatePosition( m_shadowPos, GetObject()->m_matrix, m_shadowNormal );
						}
					}
					break;

					default:
						Dbg_MsgAssert(0,("Bad shadow type: %d",mp_shadow->GetShadowType()));
						break;
				}

				mp_shadow->UnHide();
			}
			else
			{
				// model is not active, so we probably don't want the shadow's model to be active either
				mp_shadow->Hide();
			}
		}
	}			
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CShadowComponent::Hide( bool shouldHide )
{
	HideShadow( shouldHide );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CShadowComponent::Update()
{
//	m_shadowNormal.Set( 0.0f, 1.0f, 0.0f );

	/*
	if ( mp_shadow )
	{
		if (mp_shadow->GetShadowType()==Gfx::vSIMPLE_SHADOW)
		{
			mp_shadow->UpdatePosition(m_shadow_pos,m_matrix,pShadowComponent->m_shadownormal);
		}		
		else
		{
			mp_shadow->UpdatePosition(shadow_target_pos); // at this point m_pos is the same as mp_physics->m_pos
		}
	}
	*/

	if ( GetObject()->GetID() >= 0 && GetObject()->GetID() < Mdl::Skate::vMAX_SKATERS )
	{
		// the skater shadows are handled elsewhere by other components
		// (CSkaterAdjustPhysicsComponent or CWalkComponent)
//		SetShadowPos( GetObject()->GetPos() );
	}
	else
	{
		// the ped shadow is handled here...
		SetShadowPos( GetObject()->GetPos() );
	}

	update_shadow();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::EMemberFunctionResult CShadowComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | Obj_ShadowOff | turn off shadow
		case ( 0xaaf6e513 ): // Obj_ShadowOff
			SwitchOffShadow();
			break;

        // @script | Obj_ShadowOn | turn shadow on
		// @parmopt float | Offset | 0.2 | The y offset of the shadow, > 0 = up
		// @parmopt float | ShadowScale | 1.0 | The scale of the shadow.
		// @parmopt string | ShadowModel | "Ped_Shadow" | The mdl file to use
		// @parmopt name | shadow_type | Either "detailed" or "simple"
		case ( 0xf272c43a ): // Obj_ShadowOn
		{
			// add a default shadow_type
			uint32 shadowType;
			if ( !pParams->GetChecksum( "shadowType", &shadowType, Script::NO_ASSERT ) )
			{
				pParams->AddChecksum( "shadowType", CRCD(0x3e84c2fd,"simple") );
			}

			this->InitFromStructure( pParams );

			if ( mp_shadow )
			{
				if( mp_shadow->GetShadowType() == Gfx::vSIMPLE_SHADOW )
				{
					float offset=0.2f;
					pParams->GetFloat(CRCD(0xa6f5352f,"Offset"),&offset);
					((Gfx::CSimpleShadow*)mp_shadow)->SetOffset(offset);
				}
			}
		}
		break;

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CShadowComponent::SwitchOnShadow( Gfx::EShadowType mode )
{
	
	
	if ( mp_shadow )
	{
		// it's already got a shadow
		return;
	}

// Note we can't use mp_model_component here, as SwitchOnShaodow is called
// from InitFromStructure, which is obviously called before Finalize(); 
	CModelComponent* pModelComponent = GetModelComponentFromObject( GetObject() );
	Nx::CModel* pModel = NULL;
	if ( pModelComponent )
	{
		pModel = pModelComponent->GetModel();
	}


	switch (mode)
	{
		case Gfx::vDETAILED_SHADOW:
		{
			Dbg_MsgAssert( mp_shadow == NULL, ("mp_shadow not NULL?") );
			Dbg_MsgAssert( pModel, ("adding detailed shadow to something with no model?") );
			mp_shadow = new Gfx::CDetailedShadow( pModel );
		}
		break;

		case Gfx::vSIMPLE_SHADOW:
		{
			Dbg_MsgAssert( mp_shadow == NULL, ("mp_shadow not NULL?") );
			Dbg_MsgAssert( pModel, ("adding simple shadow to something with no model?") );
			
			Gfx::CSimpleShadow* pSimpleShadow = new Gfx::CSimpleShadow;
			pSimpleShadow->SetScale( pModel->GetScale().GetX() );
			pSimpleShadow->SetModel( "Ped_Shadow" );
			
			mp_shadow = pSimpleShadow;
			mp_shadow->UpdatePosition( m_shadowPos, GetObject()->m_matrix, m_shadowNormal );
		}	
		break;
		
		default:
			Dbg_Message( "Unrecognized shadow mode %d", mode );
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CShadowComponent::SwitchOffShadow()
{
	if ( mp_shadow )
	{
		if ( mp_model_component )
		{
			Nx::CModel* pModel = mp_model_component->GetModel();
			Dbg_Assert( pModel );
			pModel->EnableShadow( false );
		}

		delete mp_shadow;
		mp_shadow = NULL;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CShadowComponent::HideShadow( bool should_hide )
{
	if ( mp_shadow )
	{
		if ( should_hide )
		{
			mp_shadow->Hide();
		}
		else
		{
			mp_shadow->UnHide();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CShadowComponent::SetShadowPos( const Mth::Vector& pos )
{
	m_shadowPos = pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CShadowComponent::SetShadowNormal( const Mth::Vector& normal )
{
	m_shadowNormal = normal;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CShadowComponent::SetShadowScale( float scale )
{
	if ( m_shadowType == CRCD(0x3e84c2fd,"simple") )
	{
		((Gfx::CSimpleShadow*)mp_shadow)->SetScale( scale );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CShadowComponent::SetShadowDirection( const Mth::Vector& vector )
{
	if( mp_shadow )
	{
		mp_shadow->UpdateDirection( vector );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CShadowComponent::SwitchOnSkaterShadow()
{
	// NOTE: skater specific and should not be here; migrate to somewhere like Mdl::Skate
	
	// only call on shadows attached to skaters
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CShadowComponent::SwitchOnSkaterShadow called on shadow not attached to skater object"));
	
	CSkater* pSkater = static_cast< CSkater* >(GetObject());
	
	Gfx::EShadowType mode = Gfx::vDETAILED_SHADOW;
	
	// put it on the bottom up heap, because we don't want to fragment the skater geom heap...
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterHeap(pSkater->GetHeapIndex()));

	// In Splitscreen games, shadow type for local player is optional
	if (CFuncs::ScriptInSplitScreenGame(NULL, NULL))
	{
		mode = Mdl::Skate::Instance()->GetShadowMode();
	}

	if (!pSkater->IsLocalClient())
	{
		Dbg_Printf( "************************ SWITCHING ON SKATER SHADOW FOR CLIENT\n" );
		mode = Gfx::vSIMPLE_SHADOW;
	}
	
	Script::CStruct* pTempParams = new Script::CStruct;
	pTempParams->AddFloat(CRCD(0x6f8cd62f, "ShadowScale"), 1.0f);
	pTempParams->AddString(CRCD(0x545f8172, "ShadowModel"), "Ped_Shadow");
	InitFromStructure(pTempParams);
	delete pTempParams;

	SwitchOffShadow();
	SwitchOnShadow(mode);

	Mem::Manager::sHandle().PopContext();
}
	
}
