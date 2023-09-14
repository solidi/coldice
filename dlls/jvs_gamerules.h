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

class CHalfLifeJesusVsSanta : public CHalfLifeMultiplay
{
public:
	CHalfLifeJesusVsSanta();
	virtual const char *GetGameDescription( void ) { return "Cold Ice Remastered Jesus vs. Santa"; }
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	virtual void Think ( void );
	virtual BOOL CheckGameTimer( void );
	virtual void FPlayerTookDamage( float flDamage, CBasePlayer *pVictim, CBaseEntity *pKiller);
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL IsArmoredMan( CBasePlayer *pPlayer ) { return pArmoredMan == pPlayer; }
};
