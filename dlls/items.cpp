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
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "shake.h"
#include "game.h"

extern int gmsgItemPickup;
extern int gmsgStatusIcon;

class CWorldItem : public CBaseEntity
{
public:
	void	KeyValue(KeyValueData *pkvd ); 
	void	Spawn( void );
	int		m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

void CWorldItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch (m_iType) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if (!pEntity)
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}


void CItem::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	pev->sequence = floatingweapons.value;
	pev->framerate = 1.0;

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch( pPlayer ))
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink ( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
}

#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_suit.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_suit.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->weapons & (1<<WEAPON_SUIT) )
			return FALSE;

		if ( pev->spawnflags & SF_SUIT_SHORTLOGON )
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A0");		// short version of suit logon,
		else
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_AAx");	// long version of suit logon

		pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);



class CItemBattery : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_battery.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_battery.mdl");
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			(pPlayer->pev->weapons & (1<<WEAPON_SUIT)))
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			
			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)( (float)(pPlayer->pev->armorvalue * 100.0) * (1.0/MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;
		
			sprintf( szcharge,"!HEV_%1dP", pct );
			
			//EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);


class CItemAntidote : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_antidote.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->SetSuitUpdate("!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN);
		
		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);


class CItemSecurity : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_security.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_security.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

class CItemLongJump : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_longjump.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_longjump.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if ( ( pPlayer->pev->weapons & (1<<WEAPON_SUIT) ) )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump );

//===================================================================
//===================================================================

void CRune::Spawn( void )
{
	pev->angles.x = 0;
	pev->angles.z = 0;
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	SetTouch( &CRune::RuneTouch );
	SetThink( &CBaseEntity::SUB_StartFadeOut );
	pev->nextthink = gpGlobals->time + 25.0;

	//animate
	pev->sequence = 0;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
}

void CRune::RuneTouch( CBaseEntity *pOther )
{
	if ( !(pev->flags & FL_ONGROUND ) )
	{
		return;
	}

	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (MyTouch( pPlayer ))
	{
		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "rune_pickup.wav", 1, ATTN_NORM );

		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
		MESSAGE_END();

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );

		UTIL_Remove( this );
	}
}

void CRune::ShowStatus(CBasePlayer *pPlayer, int r, int g, int b) {
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev );
		WRITE_BYTE(1);
		WRITE_STRING("dmg_cold");
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
	MESSAGE_END();
	UTIL_ScreenFade( pPlayer, Vector(r, g, b), 1, 1, 64, FFADE_IN);
}

//===================================================================
//===================================================================

class CFragRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_FRAG - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 106;
		pev->rendercolor.y = 13;
		pev->rendercolor.z = 173;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)13);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_FRAG;

			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->renderamt = 5;
			pPlayer->pev->rendercolor.x = 106;
			pPlayer->pev->rendercolor.y = 13;
			pPlayer->pev->rendercolor.z = 173;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)13);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 106, 13, 173);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_frag, CFragRune );

class CVampireRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_VAMPIRE - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 200;
		pev->rendercolor.y = 0;
		pev->rendercolor.z = 0;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)73);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_VAMPIRE;

			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->renderamt = 5;
			pPlayer->pev->rendercolor.x = 200;
			pPlayer->pev->rendercolor.y = 0;
			pPlayer->pev->rendercolor.z = 0;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)73);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 200, 0, 0);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_vampire, CVampireRune );

class CProtectRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_PROTECT - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 0;
		pev->rendercolor.y = 200;
		pev->rendercolor.z = 0;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)178);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_PROTECT;

			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->renderamt = 5;
			pPlayer->pev->rendercolor.x = 0;
			pPlayer->pev->rendercolor.y = 200;
			pPlayer->pev->rendercolor.z = 0;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)178);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 0, 200, 0);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_protect, CProtectRune );

class CRegenRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_REGEN - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 200;
		pev->rendercolor.y = 0;
		pev->rendercolor.z = 200;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)144);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_REGEN;

			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->renderamt = 5;
			pPlayer->pev->rendercolor.x = 200;
			pPlayer->pev->rendercolor.y = 0;
			pPlayer->pev->rendercolor.z = 200;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)144);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 200, 0, 200);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_regen, CRegenRune );

class CHasteRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_HASTE - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 200;
		pev->rendercolor.y = 128;
		pev->rendercolor.z = 0;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)107);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_HASTE;

			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->renderamt = 5;
			pPlayer->pev->rendercolor.x = 200;
			pPlayer->pev->rendercolor.y = 128;
			pPlayer->pev->rendercolor.z = 0;

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "haste", "1" );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)107);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 200, 128, 0);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_haste, CHasteRune );

class CGravityRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_GRAVITY - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 0;
		pev->rendercolor.y = 115;
		pev->rendercolor.z = 230;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)212);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_GRAVITY;

			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->renderamt = 5;
			pPlayer->pev->rendercolor.x = 0;
			pPlayer->pev->rendercolor.y = 115;
			pPlayer->pev->rendercolor.z = 230;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)212);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 0, 115, 230);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_gravity, CGravityRune );

class CStrengthRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_STRENGTH - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 200;
		pev->rendercolor.y = 200;
		pev->rendercolor.z = 0;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)194);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_STRENGTH;

			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->renderamt = 5;
			pPlayer->pev->rendercolor.x = 200;
			pPlayer->pev->rendercolor.y = 200;
			pPlayer->pev->rendercolor.z = 0;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)194);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 200, 200, 0);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_strength, CStrengthRune );

class CCloakRune : public CRune
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_runes.mdl");
		CRune::Spawn( );

		pev->body = RUNE_CLOAK - 1;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 5;
		pev->rendercolor.x = 200;
		pev->rendercolor.y = 200;
		pev->rendercolor.z = 200;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_PARTICLEBURST );
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT( 50 );
			WRITE_BYTE((unsigned short)194);
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_runes.mdl");
		PRECACHE_SOUND ("rune_pickup.wav");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_fHasRune )
		{
			pPlayer->m_fHasRune = RUNE_CLOAK;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_PARTICLEBURST );
				WRITE_COORD(pPlayer->pev->origin.x);
				WRITE_COORD(pPlayer->pev->origin.y);
				WRITE_COORD(pPlayer->pev->origin.z);
				WRITE_SHORT( 50 );
				WRITE_BYTE((unsigned short)194);
				WRITE_BYTE( 5 );
			MESSAGE_END();

			ShowStatus(pPlayer, 200, 200, 200);

			return TRUE;
		}

		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( rune_cloak, CCloakRune );

//===================================================================
//===================================================================

void CWorldRunes::Spawn( )
{
	m_pSpot = NULL;
	Precache( );

	SetThink( &CWorldRunes::SpawnRunes );
	pev->nextthink = gpGlobals->time + 5.0;
}

void CWorldRunes::Precache( )
{
	UTIL_PrecacheOther("rune_frag");
	UTIL_PrecacheOther("rune_vampire");
	UTIL_PrecacheOther("rune_protect");
	UTIL_PrecacheOther("rune_regen");
	UTIL_PrecacheOther("rune_haste");
	UTIL_PrecacheOther("rune_gravity");
	UTIL_PrecacheOther("rune_strength");
	UTIL_PrecacheOther("rune_cloak");
}

CBaseEntity *CWorldRunes::SelectSpawnPoint(CBaseEntity *pSpot)
{
	for ( int i = RANDOM_LONG(1,8); i > 0; i-- )
		pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );

	return pSpot;
}

void CWorldRunes::DropRune(CBasePlayer *pPlayer) {
	UTIL_MakeVectors ( pPlayer->pev->angles );

	char *sz_Rune;
	switch (pPlayer->m_fHasRune)
	{
		case RUNE_FRAG:
			sz_Rune = "rune_frag";
			break;
		case RUNE_VAMPIRE:
			sz_Rune = "rune_vampire";
			break;
		case RUNE_PROTECT:
			sz_Rune = "rune_protect";
			break;
		case RUNE_REGEN:
			sz_Rune = "rune_regen";
			break;
		case RUNE_HASTE:
			sz_Rune = "rune_haste";
			break;
		case RUNE_GRAVITY:
			sz_Rune = "rune_gravity";
			break;
		case RUNE_CLOAK:
			sz_Rune = "rune_cloak";
			break;
		default:
			sz_Rune = "rune_strength";
	}
	CRune *rune = (CRune *)CBaseEntity::Create(sz_Rune, pPlayer->pev->origin + gpGlobals->v_forward * 10, pPlayer->pev->angles, pPlayer->edict());
	rune->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;
}

void CWorldRunes::CreateRune(char *sz_RuneClass)
{
	m_pSpot = SelectSpawnPoint( m_pSpot );

	if ( m_pSpot == NULL )
	{
		ALERT ( at_console, "Error Creating Runes\n" );
		return;
	}

	CBaseEntity *rune = CBaseEntity::Create(sz_RuneClass, m_pSpot->pev->origin, Vector(0, 0, 0), NULL );
	rune->pev->velocity.x = RANDOM_FLOAT( -300, 300 );
	rune->pev->velocity.y = RANDOM_FLOAT( -300, 300 );
	rune->pev->velocity.z = RANDOM_FLOAT( 0, 300 );
}

void CWorldRunes::SpawnRunes( )
{
	ALERT ( at_console, "Creating Runes\n" );
	CreateRune( "rune_frag" );
	CreateRune( "rune_vampire" );
	CreateRune( "rune_protect" );
	CreateRune( "rune_regen" );
	CreateRune( "rune_haste" );
	CreateRune( "rune_gravity" );
	CreateRune( "rune_strength" );
	CreateRune( "rune_cloak" );

	SetThink( &CWorldRunes::SpawnRunes );
	pev->nextthink = gpGlobals->time + 30.0;
}

void CWorldRunes::Create( )
{
	CWorldRunes* WorldRunes = GetClassPtr( (CWorldRunes*)NULL );
	WorldRunes->Spawn();
}

void CWorldRunes::ResetPlayer(CBasePlayer *pPlayer)
{
	pPlayer->m_fHasRune = 0;
	pPlayer->m_flRuneHealTime = 0;
	g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "haste", "0" );
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev );
		WRITE_BYTE(0);
		WRITE_STRING("dmg_cold");
	MESSAGE_END();
	pPlayer->pev->rendermode = kRenderNormal;
	pPlayer->pev->renderfx = kRenderFxNone;
	pPlayer->pev->renderamt = 0;
}
