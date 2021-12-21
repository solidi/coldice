/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_GAUGE_SHOTGUN	Vector( 0.08716, 0.04362, 0.00 )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLEGAUGE_SHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum gauge_shotgun_e {
	GAUGE_SHOTGUN_IDLE = 0,
	GAUGE_SHOTGUN_IDLE4,
	GAUGE_SHOTGUN_DRAW,
	GAUGE_SHOTGUN_FIRE,
	GAUGE_SHOTGUN_START_RELOAD,
	GAUGE_SHOTGUN_RELOAD,
	GAUGE_SHOTGUN_PUMP,
	GAUGE_SHOTGUN_HOLSTER,
	GAUGE_SHOTGUN_FIRE2,
	GAUGE_SHOTGUN_IDLE_DEEP
};

#ifdef _12GAUGE
LINK_ENTITY_TO_CLASS( weapon_12gauge, C12Gauge );
#endif

void C12Gauge::Spawn( )
{
	Precache( );
	m_iId = WEAPON_12GAUGE;
	SET_MODEL(ENT(pev), "models/w_12gauge.mdl");

	m_iDefaultAmmo = _12_GAUGE_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void C12Gauge::Precache( void )
{
	PRECACHE_MODEL("models/v_12gauge.mdl");
	PRECACHE_MODEL("models/w_12gauge.mdl");
	PRECACHE_MODEL("models/p_12gauge.mdl");

	m_iShell = PRECACHE_MODEL ("models/w_shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND ("12gauge_fire.wav");
	PRECACHE_SOUND ("weapons/sbarrel1.wav");//shotgun

	PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload

	PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND ("12gauge_cock.wav");	// cock gun

	PRECACHE_SOUND ("12gauge_jackson.wav");
	PRECACHE_SOUND ("12gauge_jackson_shutup.wav");
	PRECACHE_SOUND ("12gauge_jackson_moneyout.wav");
	PRECACHE_SOUND ("12gauge_jackson_moneyall.wav");
	PRECACHE_SOUND ("12gauge_jackson_comeon.wav");
	PRECACHE_SOUND ("12gauge_jackson_buddy.wav");
	PRECACHE_SOUND ("12gauge_jackson_dontstallme.wav");

	m_usSingleFire = PRECACHE_EVENT( 1, "events/gauge_single.sc" );
}

int C12Gauge::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int C12Gauge::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GAUGE_SHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_12GAUGE;
	p->iWeight = GAUGE_SHOTGUN_WEIGHT;
	p->pszDisplayName = "Samuel L. Jackson's 12 Gauge Shotgun";

	return 1;
}

BOOL C12Gauge::Deploy( )
{
	BOOL result = DefaultDeploy( "models/v_12gauge.mdl", "models/p_12gauge.mdl", GAUGE_SHOTGUN_DRAW, "shotgun" );

#ifndef CLIENT_DLL
	if (result && allowvoiceovers.value) {
		EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson.wav", 1.0, ATTN_NORM );
	}
#endif

	return result;
}

void C12Gauge::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( GAUGE_SHOTGUN_HOLSTER );
}

void C12Gauge::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		vecDir = m_pPlayer->FireBulletsPlayer( 4, vecSrc, vecAiming, VECTOR_CONE_DM_GAUGE_SHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// regular old, untouched spread. 
		vecDir = m_pPlayer->FireBulletsPlayer( 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );


	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = GetNextAttackDelay(0.75);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	m_fInSpecialReload = 0;
}

void C12Gauge::SecondaryAttack( void )
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 1)
	{
		Reload( );
		PlayEmptySound( );
		return;
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	SendWeaponAnim(GAUGE_SHOTGUN_PUMP);

#ifndef CLIENT_DLL
	if (allowvoiceovers.value) {
		switch (RANDOM_LONG(0,6)) {
			case 0: EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson_shutup.wav", 1.0, ATTN_NORM ); break;
			case 1: EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson_moneyout.wav", 1.0, ATTN_NORM ); break;
			case 2: EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson_moneyall.wav", 1.0, ATTN_NORM ); break;
			case 3: EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson_comeon.wav", 1.0, ATTN_NORM ); break;
			case 4: EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson_buddy.wav", 1.0, ATTN_NORM ); break;
			case 5: EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson_dontstallme.wav", 1.0, ATTN_NORM ); break;
			case 6: EMIT_SOUND ( ENT(m_pPlayer->pev), CHAN_VOICE, "12gauge_jackson.wav", 1.0, ATTN_NORM ); break;
		}
	}
#endif

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.25;

	m_flNextPrimaryAttack = GetNextAttackDelay(1.5);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.0;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0;
	else
		m_flTimeWeaponIdle = 1.5;
}

void C12Gauge::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == GAUGE_SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim( GAUGE_SHOTGUN_START_RELOAD );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( GAUGE_SHOTGUN_RELOAD );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}

void C12Gauge::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_pPlayer->pev->button & IN_IRONSIGHT )
		return;

	if ( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "12gauge_cock.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != GAUGE_SHOTGUN_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( GAUGE_SHOTGUN_PUMP );
				
				// play cocking sound
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "12gauge_cock.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if (flRand <= 0.8)
			{
				iAnim = GAUGE_SHOTGUN_IDLE_DEEP;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (60.0/12.0);// * RANDOM_LONG(2, 5);
			}
			else if (flRand <= 0.95)
			{
				iAnim = GAUGE_SHOTGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			else
			{
				iAnim = GAUGE_SHOTGUN_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			}
			SendWeaponAnim( iAnim );
		}
	}
}
