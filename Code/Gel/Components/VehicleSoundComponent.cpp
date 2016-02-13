//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VehicleSoundComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  7/2/3
//****************************************************************************

#include <gel/components/vehiclesoundcomponent.h>
#include <gel/components/vehiclecomponent.h>
#include <gel/components/soundcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/soundfx/soundfx.h>

#include <sk/modules/skate/goalmanager.h>
#include <sk/parkeditor2/parked.h>

#define MESSAGE(a) { printf("M:%s:%i: %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPI(a) { printf("D:%s:%i: " #a " = %i\n", __FILE__ + 15, __LINE__, a); }
#define DUMPB(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a ? "true" : "false"); }
#define DUMPF(a) { printf("D:%s:%i: " #a " = %g\n", __FILE__ + 15, __LINE__, a); }
#define DUMPE(a) { printf("D:%s:%i: " #a " = %e\n", __FILE__ + 15, __LINE__, a); }
#define DUMPS(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPP(a) { printf("D:%s:%i: " #a " = %p\n", __FILE__ + 15, __LINE__, a); }
#define DUMPC(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, Script::FindChecksumName(a)); }
#define DUMPV(a) { printf("D:%s:%i: " #a " = %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z]); }
#define DUMP4(a) { printf("D:%s:%i: " #a " = %g, %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z], (a)[W]); }
#define DUMPM(a) { DUMP4((a)[X]); DUMP4((a)[Y]); DUMP4((a)[Z]); DUMP4((a)[W]); }
#define DUMP2(a) { printf("D:%s:%i " #a " = ", __FILE__ + 15, __LINE__); for (int n = 32; n--; ) { printf("%c", ((a) & (1 << n)) ? '1' : '0'); } printf("\n"); }
#define MARK { printf("K:%s:%i: %s\n", __FILE__ + 15, __LINE__, __PRETTY_FUNCTION__); }
#define PERIODIC(n) for (static int p__ = 0; (p__ = ++p__ % (n)) == 0; )

#ifdef __NOPT_ASSERT__
	#define DEBUG_OUTPUT(a) \
		{ \
			if (Script::GetInteger(CRCD(0x54f22205, "DebugPlayerVehicleEngineSound")) \
				|| Script::GetInteger(CRCD(0xdf03ebbf, "DebugPlayerVehicleTireSound"))) \
			{ \
				printf("[DebugEngineSound]: "); \
				printf a; \
				printf("\n"); \
			} \
		}
	#define DEBUG_ENGINE(a) \
		{ \
			if (Script::GetInteger(CRCD(0x54f22205, "DebugPlayerVehicleEngineSound"))) \
			{ \
				printf("[DebugEngineSound]: "); \
				printf a; \
				printf("\n"); \
			} \
		}
	#define DEBUG_TIRES(a) \
		{ \
			if (Script::GetInteger(CRCD(0xdf03ebbf, "DebugPlayerVehicleTireSound"))) \
			{ \
				printf("[DebugTireSound]: "); \
				printf a; \
				printf("\n"); \
			} \
		}
	#define DEBUG_COLLISION(a) \
		{ \
			if (Script::GetInteger(CRCD(0xc2ddef08, "DebugPlayerVehicleCollisionSound"))) \
			{ \
				printf("[DebugCollisionSound]: "); \
				printf a; \
				printf("\n"); \
			} \
		}
#else
	#define DEBUG_OUTPUT(a)
	#define DEBUG_ENGINE(a)
	#define DEBUG_TIRES(a)
	#define DEBUG_COLLISION(a)
#endif

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CVehicleSoundComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CVehicleSoundComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CVehicleSoundComponent::CVehicleSoundComponent() : CBaseComponent()
{
	SetType( CRC_VEHICLESOUND );
	
	mp_vehicle_component = NULL;
	mp_sound_component = NULL;
	
	m_sound_setup_struct = NULL;
	
	m_use_default_sounds = false;
	
	m_effective_speed = 0.0f;
	m_effective_gear = 0;
	m_gear_shift_time_stamp = 0;
	m_airborne_time_stamp = 0;
	m_effective_tire_slip_vel = 0.0f;
	m_latest_collision_sound_time_stamp = 0;
	
	m_engine_sound_id = 0;
	m_tire_sound_id = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CVehicleSoundComponent::~CVehicleSoundComponent()
{
	if (m_engine_sound_id)
	{
		Sfx::CSfxManager::Instance()->StopSound(m_engine_sound_id);
		m_engine_sound_id = 0;
	}
	if (m_tire_sound_id)
	{
		Sfx::CSfxManager::Instance()->StopSound(m_tire_sound_id);
		m_tire_sound_id = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::InitFromStructure( Script::CStruct* pParams )
{
	m_use_default_sounds = Ed::CParkEditor::Instance()->UsingCustomPark();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::Finalize()
{
	mp_vehicle_component = GetVehicleComponentFromObject(GetObject());
	mp_sound_component = GetSoundComponentFromObject(GetObject());
	
	Dbg_Assert(mp_vehicle_component);
	Dbg_Assert(mp_sound_component);
	
	fetch_script_parameters();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::Update()
{
	DEBUG_OUTPUT(("-- frame start --"));
	
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0xcf5f4e16, "DynamicPlayerVehicleSound")))
	{
		fetch_script_parameters();
	}
	#endif
	
	if (!m_sound_setup_struct) return;
	
	update_engine_sounds();
	
	update_tire_sounds();
	
	update_collision_sounds();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CVehicleSoundComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CVehicleSoundComponent::GetDebugInfo"));
	
	p_info->AddChecksum(CRCD(0x2a19e830, "engine_sound"), m_engine_sound_checksum);
	p_info->AddChecksum(CRCD(0xe798fe80, "tire_sound"), m_tire_sound_checksum);
	p_info->AddStructure(CRCD(0x36fa68d2, "collide_sound"), m_collide_sound_struct);


	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::update_engine_sounds (   )
{
	float frame_length = Tmr::FrameLength();
	
	CVehicleComponent::SWheel* p_wheels = mp_vehicle_component->mp_wheels;
	
	// determine the smallest of all the drive wheels' rotational velocity
	float min_vel = 1e20f;
	for (int n = CVehicleComponent::vVP_NUM_WHEELS; n--; )
	{
		if (!p_wheels[n].drive) continue;
		min_vel = Mth::Min(min_vel, Mth::Abs(p_wheels[n].rotvel) * p_wheels[n].radius);
	}
	float frame_effective_speed = IPS_TO_MPH(min_vel);
	
	// adjust effective speed based on throttle position
	bool throttle = (mp_vehicle_component->m_controls.throttle || mp_vehicle_component->m_controls.reverse) && !mp_vehicle_component->m_controls.brake;
	if (!throttle)
	{
		frame_effective_speed *= get_global_param(CRCD(0xec67bc97, "idle_engine_effective_speed_factor"));
	}
	
	// smooth out frame-to-frame fluxuations
	m_effective_speed = Mth::Lerp(frame_effective_speed, m_effective_speed, get_global_param(CRCD(0x2a7095b, "effective_speed_lerp_rate")) * frame_length);
	if (m_effective_speed < frame_effective_speed)
	{
		m_effective_speed = Mth::ClampMax(m_effective_speed + get_global_param(CRCD(0xca5efd91, "effective_speed_adjust_up_rate")) * frame_length, m_effective_speed);
	}
	else
	{
		m_effective_speed = Mth::ClampMin(m_effective_speed - get_global_param(CRCD(0x77c8e7c8, "effective_speed_adjust_down_rate")) * frame_length, m_effective_speed);
	}
	DEBUG_ENGINE(("speed: %.2f mph", m_effective_speed));
	
	// determine the appropriate gear for this frame
	if (mp_vehicle_component->m_num_wheels_in_contact != 0)
	{
		if (m_effective_gear < m_num_gears - 1 && m_effective_speed > m_gears[m_effective_gear].upshift_point)
		{
			m_effective_gear++;
			m_gear_shift_time_stamp = Tmr::GetTime();
			DEBUG_ENGINE(("upshift to gear: %i", m_effective_gear));
		}
		else if (m_effective_gear > 0 && m_effective_speed < m_gears[m_effective_gear].downshift_point)
		{
			m_effective_gear--;
			m_gear_shift_time_stamp = Tmr::GetTime();
			DEBUG_ENGINE(("downshift to gear: %i", m_effective_gear));
		}
	}
	else if (mp_vehicle_component->m_air_time > get_global_param(CRCD(0xc0f451d4, "engine_airborne_delay")))
	{	 
		// when airborne, drop to lowest gear and use no gear shift velocity damping
		m_effective_gear = 0;
	}
	DEBUG_ENGINE(("gear: %i", m_effective_gear + 1));
	
	// calculate a volume adjustment based on the time since the last gear shift
	float gear_shift_vol_factor = Mth::ClampMax(
		Mth::LinearMap(
			get_global_param(CRCD(0xbfa32092, "gear_shift_min_vol_factor")),
			1.0f,
			Tmr::ElapsedTime(m_gear_shift_time_stamp),
			0.0f,
			get_global_param(CRCD(0xfb6bd20e, "gear_shift_vol_adjustment_duration"))
		),
		1.0f
	);
		
	// calculate this frame's engine rpm based on the current gear and speed
	float frame_engine_rpm = Mth::LinearMap(
		m_gears[m_effective_gear].bottom_rpm,
		m_gears[m_effective_gear].top_rpm,
		m_effective_speed,
		m_effective_gear ? m_gears[m_effective_gear - 1].upshift_point : 0.0f,
		m_gears[m_effective_gear].upshift_point
	);
	// DEBUG_ENGINE(("speed top: %.2f", m_gears[m_effective_gear].upshift_point));
	// DEBUG_ENGINE(("speed bottom: %.2f", m_effective_gear ? m_gears[m_effective_gear - 1].upshift_point : 0.0f));
	// DEBUG_ENGINE(("rpm top: %.2f", m_gears[m_effective_gear].top_rpm));
	// DEBUG_ENGINE(("rpm bottom: %.2f", m_gears[m_effective_gear].bottom_rpm));
	
	// smooth out the engine rpm changes (causes nice shifting effects)
	m_effective_engine_rpm = Mth::Lerp(
		m_effective_engine_rpm,
		frame_engine_rpm,
		get_global_param(CRCD(0x3dd9c040, "engine_rpm_lerp_rate")) * frame_length
	);
	DEBUG_ENGINE(("engine RPM: %.2f", m_effective_engine_rpm));
	
	// calculate a number between 0.0f and 1.0f which will scale the engine sound's pitch and volume
	float sound_factor = Mth::Clamp(
		Mth::LinearMap(
			0.0f,
			1.0f,
			m_effective_engine_rpm,
			get_param(CRCD(0x05ac84a3, "MinEngineRPM")),
			get_param(CRCD(0x95df9449, "MaxEngineRPM"))
		),
		0.0f,
		1.0f
	);
	
	// setup volume
	Sfx::sVolume volume;
	Sfx::CSfxManager::Instance()->SetVolumeFromPos(&volume, GetObject()->GetPos(), Sfx::CSfxManager::Instance()->GetDropoffDist(m_engine_sound_checksum));
	float volume_percent = gear_shift_vol_factor * Mth::Lerp(
		get_param(CRCD(0xb8f81277, "MinEngineVol")),
		get_param(CRCD(0x288b029d, "MaxEngineVol")),
		sound_factor
	);
	volume.PercentageAdjustment(volume_percent);
	DEBUG_ENGINE(("engine volume: %.0f", volume_percent));
	
	// setup pitch
	float pitch = Mth::Lerp(
		get_param(CRCD(0x2660af3b, "MinEnginePitch")),
		get_param(CRCD(0x9f46344a, "MaxEnginePitch")),
		sound_factor
	);
	DEBUG_ENGINE(("engine pitch: %.0f", pitch));
	
	// play or adjust sound
	if (!m_engine_sound_id)
	{
		m_engine_sound_id = Sfx::CSfxManager::Instance()->PlaySound(m_engine_sound_checksum, &volume, pitch, NULL);
	}
	else
	{
		Sfx::CSfxManager::Instance()->UpdateLoopingSound(m_engine_sound_id, &volume, pitch);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::update_tire_sounds (   )
{
	CVehicleComponent::SWheel* p_wheels = mp_vehicle_component->mp_wheels;
	
	// determine the largest of all the wheels' slip velocity
	float max_slip_vel = 0.0f;
	
	if (mp_vehicle_component->m_controls.brake || mp_vehicle_component->m_controls.handbrake)
	{
		for (int n = CVehicleComponent::vVP_NUM_WHEELS; n--; )
		{
			// handbrake skid states do not depend on the tire's slip velocity, so we check the slip velocity ourselves
			if ((p_wheels[n].state == CVehicleComponent::SWheel::SLIPPING || p_wheels[n].state == CVehicleComponent::SWheel::SKIDDING)
				|| ((p_wheels[n].state == CVehicleComponent::SWheel::HANDBRAKE_LOCKED || p_wheels[n].state == CVehicleComponent::SWheel::HANDBRAKE_THROTTLE)
				&& p_wheels[n].slip_vel > p_wheels[n].max_static_velocity))
			{
				max_slip_vel = Mth::Max(max_slip_vel, p_wheels[n].slip_vel);
			}
		}
	}
	
	m_effective_tire_slip_vel = Mth::Lerp(
		m_effective_tire_slip_vel,
		max_slip_vel,
		get_global_param(CRCD(0x95fbe774, "tire_slip_lerp_rate")) * Tmr::FrameLength()
	);
	DEBUG_TIRES(("tire slip velocity: %.2f", m_effective_tire_slip_vel));
	
	float sound_factor = Mth::Clamp(
		m_effective_tire_slip_vel / get_param(CRCD(0x28f1aad0, "FullTireSlipVelocity")),
		0.0f,
		1.0f
	);
	
	// setup volume
	Sfx::sVolume volume;
	Sfx::CSfxManager::Instance()->SetVolumeFromPos(&volume, GetObject()->GetPos(), Sfx::CSfxManager::Instance()->GetDropoffDist(m_tire_sound_checksum));
	float volume_percent = Mth::Lerp(
		get_param(CRCD(0x43e6cdf2, "MinTireVol")),
		get_param(CRCD(0x42ea5746, "MaxTireVol")),
		sound_factor
	);
	volume.PercentageAdjustment(volume_percent);
	DEBUG_TIRES(("tire volume: %.0f", volume_percent));
	
	// setup pitch
	float pitch = Mth::Lerp(
		get_param(CRCD(0x76f8f76e, "MinTirePitch")),
		get_param(CRCD(0xe68be784, "MaxTirePitch")),
		sound_factor
	);
	DEBUG_TIRES(("tire pitch: %.0f", pitch));
	
	if (!volume.IsSilent())
	{
		if (!m_tire_sound_id)
		{
			m_tire_sound_id = Sfx::CSfxManager::Instance()->PlaySound(m_tire_sound_checksum, &volume, pitch, NULL);
		}
		else
		{
			Sfx::CSfxManager::Instance()->UpdateLoopingSound(m_tire_sound_id, &volume, pitch);
		}
	}
	else if (m_tire_sound_id)
	{
		Sfx::CSfxManager::Instance()->StopSound(m_tire_sound_id);
		m_tire_sound_id = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::update_collision_sounds (   )
{
	if (Tmr::ElapsedTime(m_latest_collision_sound_time_stamp) < get_global_param(CRCD(0x71b4b9fe, "collision_mute_delay"))) return;
	
	float sound_factor = mp_vehicle_component->m_max_normal_collision_impulse / get_param(CRCD(0xcea59d0b, "FullCollision"));
	if (sound_factor > get_global_param(CRCD(0xf6981837, "collision_cutoff_factor")))
	{
		DEBUG_COLLISION(("collision: %.2f", mp_vehicle_component->m_max_normal_collision_impulse));
		
		m_collide_sound_struct->AddFloat(CRCD(0x9e497fc6, "Percent"), 100.0f * sound_factor);
		mp_sound_component->PlayScriptedSound(m_collide_sound_struct);
		
		m_latest_collision_sound_time_stamp = Tmr::GetTime();
		
		Script::CStruct* p_params = new Script::CStruct;
		p_params->AddFloat(CRCD(0x29461e27, "Strength"), Mth::Clamp(sound_factor, 0.0f, 1.0f));
		mp_vehicle_component->mp_skater->SelfEvent(CRCD(0xc4a60423, "Vehicle_BodyCollision"), p_params);
		delete p_params;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleSoundComponent::fetch_script_parameters (   )
{
	// get sound setup structure
	Script::CStruct* sound_setups_struct = Script::GetStructure(CRCD(0x3c2da7f5, "PlayerVehicleSounds"), Script::ASSERT);
	sound_setups_struct->GetStructure(mp_vehicle_component->GetSoundSetupChecksum(), &m_sound_setup_struct, Script::ASSERT);
	
	// extract sound checksum
	m_sound_setup_struct->GetChecksum(CRCD(0x306f8a9f, "EngineSound"), &m_engine_sound_checksum);
	m_sound_setup_struct->GetChecksum(CRCD(0x667fa96a, "TireSound"), &m_tire_sound_checksum);
	
	// extract gear information
	
	Script::CArray* gear_array;
	m_sound_setup_struct->GetArray(CRCD(0x83273e6b, "Gears"), &gear_array, Script::ASSERT);
	
	m_num_gears = gear_array->GetSize();
	Dbg_MsgAssert(m_num_gears <= vVS_MAX_NUM_GEARS, ("PlayerVehicleSound '%s' exceeds the maximum number of %i allowed gears", Script::FindChecksumName(mp_vehicle_component->GetSoundSetupChecksum()), vVS_MAX_NUM_GEARS));
	for (int n = 0; n < m_num_gears; n++)
	{
		Script::CStruct* gear_struct = gear_array->GetStructure(n);
		
		gear_struct->GetFloat(CRCD(0xfad20901, "UpshiftPoint"), &m_gears[n].upshift_point, Script::ASSERT);
		gear_struct->GetFloat(CRCD(0xfcf5c8a2, "DownshiftPoint"), &m_gears[n].downshift_point, Script::ASSERT);
        gear_struct->GetFloat(CRCD(0x735b5ce4, "BottomRPM"), &m_gears[n].bottom_rpm, Script::ASSERT);
        gear_struct->GetFloat(CRCD(0xe59a89a6, "TopRPM"), &m_gears[n].top_rpm, Script::ASSERT);
		
		Dbg_MsgAssert(m_gears[n].upshift_point > m_gears[n].downshift_point, ("In gear %i of PlayerVehicleSound '%s,' DownshiftPoint exceeds UpshiftPoint", n + 1, Script::FindChecksumName(mp_vehicle_component->GetSoundSetupChecksum())));
		Dbg_MsgAssert(!n || m_gears[n].downshift_point < m_gears[n - 1].upshift_point, ("In gear %i of PlayerVehicleSound '%s', DownshiftPoint exceeds lower gear's UpshiftPoint", n + 1, Script::FindChecksumName(mp_vehicle_component->GetSoundSetupChecksum())));
	}
	
	// extract collision sound structure
	m_sound_setup_struct->GetStructure(CRCD(0xbf8e0ace, "CollideSound"), &m_collide_sound_struct, Script::ASSERT);
	
	// if we're in a create-a-goal, swap sound checksums for default sound checksums, as the true sounds may not be loaded
	if (m_use_default_sounds)
	{
		Script::CStruct* global_sound_setup = Script::GetStructure(CRCD(0x38ec37ac, "PlayerVehicleSoundGlobalParameters"), Script::ASSERT);
		
		global_sound_setup->GetChecksum(CRCD(0xf5406e1a, "CAGEngineSound"), &m_engine_sound_checksum, Script::ASSERT);
		global_sound_setup->GetChecksum(CRCD(0x60e93a2, "CAGTireSound"), &m_tire_sound_checksum, Script::ASSERT);
		global_sound_setup->GetStructure(CRCD(0x22995285, "CAGCollideSound"), &m_collide_sound_struct, Script::ASSERT);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CVehicleSoundComponent::get_param ( uint32 checksum )
{
	Dbg_Assert(m_sound_setup_struct);
	
	float value;
	m_sound_setup_struct->GetFloat(checksum, &value, Script::ASSERT);
	return value;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CVehicleSoundComponent::get_global_param ( uint32 checksum )
{
	Script::CStruct* global_sound_setup = Script::GetStructure(CRCD(0x38ec37ac, "PlayerVehicleSoundGlobalParameters"), Script::ASSERT);
	
	float value;
	global_sound_setup->GetFloat(checksum, &value, Script::ASSERT);
	return value;
}

}
