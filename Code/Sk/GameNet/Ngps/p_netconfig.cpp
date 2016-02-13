/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			GameNet					 								**
**																			**
**	File name:		p_netconfig.cpp											**
**																			**
**	Created by:		04/17/02	-	spg										**
**																			**
**	Description:	Netcnf wrapper											**
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC p_netconfig
// @module p_netconfig | None
// @subindex Scripting Database
// @index script | p_netconfig
                          
/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#define GLOBAL 1
#include <gamenet/ngps/p_ezmain.h>


#include <sk/gamenet/gamenet.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/array.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

#define sceNETCNF_MAGIC_ERROR		(-15)

namespace GameNet
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
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void sceNetcnfifDataDump(sceNetcnfifData_t *data)
{
    Dbg_Printf("-----------------------\n");
    Dbg_Printf("attach_ifc       : \"%s\"\n", data->attach_ifc);
    Dbg_Printf("attach_dev       : \"%s\"\n", data->attach_dev);
    Dbg_Printf("dhcp_host_name   : \"%s\"\n", data->dhcp_host_name);
    Dbg_Printf("address          : \"%s\"\n", data->address);
    Dbg_Printf("netmask          : \"%s\"\n", data->netmask);
    Dbg_Printf("gateway          : \"%s\"\n", data->gateway);
    Dbg_Printf("dns1_address     : \"%s\"\n", data->dns1_address);
    Dbg_Printf("dns2_address     : \"%s\"\n", data->dns2_address);
    Dbg_Printf("phone_numbers1   : \"%s\"\n", data->phone_numbers1);
    Dbg_Printf("phone_numbers2   : \"%s\"\n", data->phone_numbers2);
    Dbg_Printf("phone_numbers3   : \"%s\"\n", data->phone_numbers3);
    Dbg_Printf("auth_name        : \"%s\"\n", data->auth_name);
    Dbg_Printf("auth_key         : \"%s\"\n", data->auth_key);
    Dbg_Printf("peer_name        : \"%s\"\n", data->peer_name);
    Dbg_Printf("vendor           : \"%s\"\n", data->vendor);
    Dbg_Printf("product          : \"%s\"\n", data->product);
    Dbg_Printf("chat_additional  : \"%s\"\n", data->chat_additional);
    Dbg_Printf("outside_number   : \"%s\"\n", data->outside_number);
    Dbg_Printf("outside_delay    : \"%s\"\n", data->outside_delay);
    Dbg_Printf("ifc_type         : \"%d\"\n", data->ifc_type);
    Dbg_Printf("mtu              : \"%d\"\n", data->mtu);
    Dbg_Printf("ifc_idle_timeout : \"%d\"\n", data->ifc_idle_timeout);
    Dbg_Printf("dev_type         : \"%d\"\n", data->dev_type);
    Dbg_Printf("phy_config       : \"%d\"\n", data->phy_config);
    Dbg_Printf("dialing_type     : \"%d\"\n", data->dialing_type);
    Dbg_Printf("dev_idle_timeout : \"%d\"\n", data->dev_idle_timeout);
    Dbg_Printf("dhcp             : \"%d\"\n", data->dhcp);
    Dbg_Printf("dns1_nego        : \"%d\"\n", data->dns1_nego);
    Dbg_Printf("dns2_nego        : \"%d\"\n", data->dns2_nego);
    Dbg_Printf("f_auth           : \"%d\"\n", data->f_auth);
    Dbg_Printf("auth             : \"%d\"\n", data->auth);
    Dbg_Printf("pppoe            : \"%d\"\n", data->pppoe);
    Dbg_Printf("prc_nego         : \"%d\"\n", data->prc_nego);
    Dbg_Printf("acc_nego         : \"%d\"\n", data->acc_nego);
    Dbg_Printf("accm_nego        : \"%d\"\n", data->accm_nego);
    Dbg_Printf("-----------------------\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
 *  given a combination configuration, display its details.
 */
static void showConfiguration(ezNetCnfCombination_t *p_conf, sceNetcnfifData_t *p_data)
{
	if (p_conf == NULL) 
	{
		Dbg_Printf("showConfiguration: p_conf is NULL\n");
		return;
	}
  
	if (p_conf->isActive) 
	{
		Dbg_Printf("ezNetCnfCombination_t ------------\n");
		Dbg_Printf("net display name\t%s\n", p_conf->combinationName);
		Dbg_Printf("ifc display name\t%s\n", p_conf->ifcName);
		Dbg_Printf("dev display name\t%s\n", p_conf->devName);
		Dbg_Printf("type\t\t%s\n", kConnectionTypeDescs[p_conf->connectionType]);
//#if (SCE_LIBRARY_VERSION >= 0x2500)
		sceNetcnfifDataDump(p_data);
//#endif
	} 
	else 
	{
		Dbg_Printf("\tempty Combination slot\n");
	}

	Dbg_Printf("\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptLoadNetConfigs(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
    ezNetCnfCombination_t *p_combination = NULL;
	int i, err;

	Dbg_Printf( "****************** LOADING NETWORK CONFIG!!!! ************************ \n" );

	static bool once = false;
	if( !once )
	{
		SIO::LoadIRX("eznetcnf" );	// net config stuff
		initEzNetCnf();
	}

    err = ezNetCnfGetCombinationList( g_netdb, &gamenet_man->m_combination_list );
	if( err < 0 )
	{
		if( err != sceNETCNF_MAGIC_ERROR )
		{
			pScript->GetParams()->AddChecksum( NONAME, CRCD(0x492676a6,"corrupt"));
		}
		Dbg_Printf( "ezNetCnfGetCombinationList error = %d\n", err );
		return false;
	}

	for( i = 0; i < gamenet_man->m_combination_list.listLength; i++ )
	{
		Dbg_Printf("getting combo %d\n", i + 1);
		p_combination = ezNetCnfGetCombination(&gamenet_man->m_combination_list, i + 1);
		if (p_combination != NULL && p_combination->isActive && ( p_combination->connectionType != eConnInvalid )) 
		{
			Dbg_Printf("found combo, %s\n", p_combination->combinationName);
			err = ezNetCnfGetEnvData(g_netdb, p_combination->combinationName, &gamenet_man->m_env_data[i] );
			if( err < 0 ) 
			{
				Dbg_Printf("ezNetCnfGetEnvData error = %d\n", err);
			}
			showConfiguration( p_combination, &gamenet_man->m_env_data[i] );
		}
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*ezNetCnfCombination_t*	Manager::GetNetworkConfig( int index )
{
	return mp_combinations[index];
}*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*void	Manager::ExitNetworkConfig( void )
{
	Dbg_Printf( "****************** EXITING NETWORK CONFIG!!!! ************************ \n" );
	// call using non-blocking mode,
	// since eznetcnf unloads and the call cannot return.
	sceSifCallRpc(&g_cd, eFuncEnd, SIF_RPCM_NOWAIT, 
	    		0, 0, 0, 0, 0, 0);
	// not sure if this is necessary.
	sceSifInitRpc(0);
}*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptNoNetConfigFiles(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	ezNetCnfCombination_t *p_combination;

	p_combination = ezNetCnfGetDefault(&gamenet_man->m_combination_list);
	if( p_combination != NULL && p_combination->isActive ) 
	{
		return false;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptFillNetConfigList(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	ezNetCnfCombination_t *p_combination;
	Script::CStruct* p_item_params;
	Script::CStruct* p_script_params;
	int i;
	char config_title[64];
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

	for( i = 0; i < gamenet_man->m_combination_list.listLength; i++ )
	{
		// check the Default Combination
		p_combination = ezNetCnfGetCombination(&gamenet_man->m_combination_list, i + 1);
		if( p_combination != NULL && p_combination->isActive && ( p_combination->connectionType != eConnInvalid ))
		{
			sprintf( config_title, "%d %s", i + 1, p_combination->ifcName );
		
			p_item_params = new Script::CStruct;	
			p_item_params->AddString( "text", config_title );
			p_item_params->AddChecksum( "id", Script::GenerateCRC( "first_net_cfg" ) + i );
			p_item_params->AddChecksum( "centered", CRCD(0x2a434d05,"centered") );
			p_item_params->AddFloat( "scale", 0.8f );
			p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("choose_net_config"));
			p_item_params->AddChecksum( "focus_script",CRCD(0x6100d1f9,"main_theme_focus_noscale"));
			p_item_params->AddChecksum( "unfocus_script",CRCD(0xe9602a90,"main_theme_unfocus_noscale"));
			p_item_params->AddChecksum( "parent_menu_id", Script::GenerateCRC( "network_options_load_config_vmenu" ));
		
			// create the parameters that are passed to the X script
			p_script_params= new Script::CStruct;
			p_script_params->AddInteger( "index", i + 1 );	
			p_item_params->AddStructure("pad_choose_params",p_script_params);			
		
			Script::RunScript("theme_menu_add_item",p_item_params);
			delete p_item_params;
			delete p_script_params;
		}
	}

	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ScriptChooseNetConfig(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Prefs::Preferences* prefs;
	Script::CScriptStructure* pTempStructure;
	ezNetCnfCombination_t *p_combination;
	int index;
    sceNetcnfifData* data;
	char outside_str[64];
	char full_phone[128];
	char full_name[256];
	bool dns_set;

	prefs = gamenet_man->GetNetworkPreferences();
	p_combination = NULL;
	data = NULL;
	if( pParams->GetInteger( "index", &index, false ))
	{
		data = &gamenet_man->m_env_data[index];
		p_combination = ezNetCnfGetCombination( &gamenet_man->m_combination_list, index );
		sprintf( full_name, "%d %s", index, p_combination->ifcName );
	}
	else
	{	
		const char* name;
		int i;
		bool found;

		pParams->GetString( "name", &name, Script::ASSERT );
		found = false;
		for( i = 0; i < gamenet_man->m_combination_list.listLength; i++ )
		{
			// check the Default Combination
			p_combination = ezNetCnfGetCombination(&gamenet_man->m_combination_list, i + 1 );
			
			if(	p_combination != NULL )
			{
				sprintf( full_name, "%d %s", i + 1, p_combination->ifcName );
			
				if(	( p_combination->isActive ) && 
					( strcmp( full_name, name ) == 0 ) && 
					( p_combination->connectionType != eConnInvalid ))
				{   
					index = i;
					found = true;
					data = &gamenet_man->m_env_data[index];
					break;
				}
			}
		}

		if( !found )
		{
			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, Script::GetString( "manual_settings_str" ));
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "config_manual" ));
			prefs->SetPreference( Script::GenerateCRC("config_type"), pTempStructure );
			delete pTempStructure;

			Script::RunScript( "clear_net_options" );
			return false;
		}
	}

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, full_name );
	pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "config_sony" ));
	prefs->SetPreference( Script::GenerateCRC("config_type"), pTempStructure );
	delete pTempStructure;
	
	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->address );
	prefs->SetPreference( Script::GenerateCRC("ip_address"), pTempStructure );
	delete pTempStructure;

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->netmask );
	prefs->SetPreference( Script::GenerateCRC("subnet_mask"), pTempStructure );
	delete pTempStructure;

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->gateway );
	prefs->SetPreference( Script::GenerateCRC("gateway"), pTempStructure );
	delete pTempStructure;

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->dns1_address );
	prefs->SetPreference( Script::GenerateCRC("dns_server"), pTempStructure );
	delete pTempStructure;

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->dns2_address );
	prefs->SetPreference( Script::GenerateCRC("dns_server2"), pTempStructure );
	delete pTempStructure;

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->dhcp_host_name );
	prefs->SetPreference( Script::GenerateCRC("host_name"), pTempStructure );
	delete pTempStructure;

	sprintf( outside_str, "%s%s", data->outside_number, data->outside_delay );
	sprintf( full_phone, "%s%s", outside_str, data->phone_numbers1 );

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, full_phone );
	prefs->SetPreference( Script::GenerateCRC("dialup_number"), pTempStructure );
	delete pTempStructure;

	sprintf( full_phone, "%s%s", outside_str, data->phone_numbers2 );

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, full_phone );
	prefs->SetPreference( Script::GenerateCRC("dialup_number2"), pTempStructure );
	delete pTempStructure;

	sprintf( full_phone, "%s%s", outside_str, data->phone_numbers3 );

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, full_phone );
	prefs->SetPreference( Script::GenerateCRC("dialup_number3"), pTempStructure );
	delete pTempStructure;

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->auth_name );
	prefs->SetPreference( Script::GenerateCRC("dialup_username"), pTempStructure );
	delete pTempStructure;

	pTempStructure = new Script::CScriptStructure;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, data->auth_key );
	prefs->SetPreference( Script::GenerateCRC("dialup_password"), pTempStructure );
	delete pTempStructure;

	dns_set = false;
			
	if( ( strlen( data->dns1_address ) > 0 ) ||
		( strlen( data->dns2_address ) > 0 ))
	{
		dns_set = true;
	}

	switch((int) p_combination->connectionType )
	{
		case eConnStatic:
			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Ethernet (Network Adaptor for PS2)" );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "device_broadband_pc" ));
			prefs->SetPreference( Script::GenerateCRC("device_type"), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Static IP Address" );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "ip_static" ));
			prefs->SetPreference( Script::GenerateCRC("broadband_type"), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "No" );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "boolean_false" ));
			prefs->SetPreference( Script::GenerateCRC("auto_dns"), pTempStructure );
			delete pTempStructure;
			break;
		case eConnDHCP:
		{
			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Ethernet (Network Adaptor for PS2)" );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "device_broadband_pc" ));
			prefs->SetPreference( Script::GenerateCRC("device_type"), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Auto-Detect (DHCP)" );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "ip_dhcp" ));
			prefs->SetPreference( Script::GenerateCRC("broadband_type"), pTempStructure );
			delete pTempStructure;

			if( dns_set )
			{
				pTempStructure = new Script::CScriptStructure;
				pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "No" );
				pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "boolean_false" ));
				prefs->SetPreference( Script::GenerateCRC("auto_dns"), pTempStructure );
				delete pTempStructure;
			}
			else
			{
				pTempStructure = new Script::CScriptStructure;
				pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Yes" );
				pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "boolean_true" ));
				prefs->SetPreference( Script::GenerateCRC("auto_dns"), pTempStructure );
				delete pTempStructure;
			}
			
			break;
		}
		case eConnPPPoE:
			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Ethernet (Network Adaptor for PS2) (PPPoE)" );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "device_broadband_pc_pppoe" ));
			prefs->SetPreference( Script::GenerateCRC("device_type"), pTempStructure );
			delete pTempStructure;

			if( dns_set )
			{
				pTempStructure = new Script::CScriptStructure;
				pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "No" );
				pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "boolean_false" ));
				prefs->SetPreference( Script::GenerateCRC("auto_dns"), pTempStructure );
				delete pTempStructure;
			}
			else
			{
				pTempStructure = new Script::CScriptStructure;
				pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Yes" );
				pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "boolean_true" ));
				prefs->SetPreference( Script::GenerateCRC("auto_dns"), pTempStructure );
				delete pTempStructure;
			}
			break;
		case eConnPPP:
		case eConnAOLPPP:
			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Modem (Network Adaptor for PS2)" );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "device_sony_modem" ));
			prefs->SetPreference( Script::GenerateCRC("device_type"), pTempStructure );
			delete pTempStructure;
			if( dns_set )
			{
				pTempStructure = new Script::CScriptStructure;
				pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "No" );
				pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "boolean_false" ));
				prefs->SetPreference( Script::GenerateCRC("auto_dns"), pTempStructure );
				delete pTempStructure;
			}
			else
			{
				pTempStructure = new Script::CScriptStructure;
				pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, "Yes" );
				pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, Script::GenerateCRC( "boolean_true" ));
				prefs->SetPreference( Script::GenerateCRC("auto_dns"), pTempStructure );
				delete pTempStructure;
			}
			break;
		default:
			Dbg_Printf( "************** GOT %d\n", (int) p_combination->connectionType );
			Dbg_Assert( 0 );
			break;
	}
    
	//Dbg_Printf("product          : \"%s\"\n", data->product);
    //Dbg_Printf("chat_additional  : \"%s\"\n", data->chat_additional);
    //Dbg_Printf("ifc_type         : \"%d\"\n", data->ifc_type);
    //Dbg_Printf("mtu              : \"%d\"\n", data->mtu);
    //Dbg_Printf("ifc_idle_timeout : \"%d\"\n", data->ifc_idle_timeout);
    //Dbg_Printf("dev_type         : \"%d\"\n", data->dev_type);
    //Dbg_Printf("phy_config       : \"%d\"\n", data->phy_config);
    //Dbg_Printf("dialing_type     : \"%d\"\n", data->dialing_type);
    //Dbg_Printf("dev_idle_timeout : \"%d\"\n", data->dev_idle_timeout);
    //Dbg_Printf("dhcp             : \"%d\"\n", data->dhcp);
    //Dbg_Printf("dns1_nego        : \"%d\"\n", data->dns1_nego);
    //Dbg_Printf("dns2_nego        : \"%d\"\n", data->dns2_nego);
    //Dbg_Printf("f_auth           : \"%d\"\n", data->f_auth);
    //Dbg_Printf("auth             : \"%d\"\n", data->auth);
    //Dbg_Printf("pppoe            : \"%d\"\n", data->pppoe);
    //Dbg_Printf("prc_nego         : \"%d\"\n", data->prc_nego);
    //Dbg_Printf("acc_nego         : \"%d\"\n", data->acc_nego);
    //Dbg_Printf("accm_nego        : \"%d\"\n", data->accm_nego);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet




