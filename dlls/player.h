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
#ifndef PLAYER_H
#define PLAYER_H


#include "pm_materials.h"


#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define		PFLAG_ONLADDER		( 1<<0 )
#define		PFLAG_ONSWING		( 1<<0 )
#define		PFLAG_ONTRAIN		( 1<<1 )
#define		PFLAG_ONBARNACLE	( 1<<2 )
#define		PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define		PFLAG_USING			( 1<<4 )		// Using a continuous entity
#define		PFLAG_OBSERVER		( 1<<5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time

#define SUIT_GROUP			TRUE
#define	SUIT_SENTENCE		FALSE

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define	SOUND_FLASHLIGHT_ON		"items/flashlight1.wav"
#define	SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"

#define TEAM_NAME_LENGTH	16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_KICK,
	PLAYER_FROZEN,
	PLAYER_PUNCH,
	PLAYER_SLIDE,
	PLAYER_RIGHT_FLIP,
	PLAYER_LEFT_FLIP,
	PLAYER_BACK_FLIP,
	PLAYER_FRONT_FLIP,
	PLAYER_HURRICANE_KICK,
	PLAYER_PULL_UP,
	PLAYER_CELEBRATE,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_ID_TARGETH,
	SBAR_ID_TARGETMAX,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f

#if defined( GRAPPLING_HOOK )
class CHook;
#endif

typedef struct {
	int animation_idx;
	char *text;
	char *sound;
} taunt_map_t;

class CBasePlayer : public CBaseMonster
{
public:
	
	// Spectator camera
	void	Observer_FindNextPlayer( bool bReverse );
	void	Observer_HandleButtons();
	void	Observer_SetMode( int iMode );
	void	Observer_CheckTarget();
	void	Observer_CheckProperties();
	EHANDLE	m_hObserverTarget;
	float	m_flNextObserverInput;
	int		m_iObserverWeapon;	// weapon of current tracked target
	int		m_iObserverLastMode;// last used observer mode
	int		IsObserver() { return pev->iuser1; };

	int					random_seed;    // See that is shared between client & server for shared weapons code

	int					m_iPlayerSound;// the index of the sound list slot reserved for this player
	int					m_iTargetVolume;// ideal sound volume. 
	int					m_iWeaponVolume;// how loud the player's weapon is right now.
	int					m_iExtraSoundTypes;// additional classification for this weapon's sound
	int					m_iWeaponFlash;// brightness of the weapon flash
	float				m_flStopExtraSoundTime;
	
	float				m_flFlashLightTime;	// Time until next battery draw/Recharge
	int					m_iFlashBattery;		// Flashlight Battery Draw

	int					m_afButtonLast;
	int					m_afButtonPressed;
	int					m_afButtonReleased;
	
	edict_t			   *m_pentSndLast;			// last sound entity to modify player room type
	float				m_flSndRoomtype;		// last roomtype set by sound entity
	float				m_flSndRange;			// dist from player to sound entity

	float				m_flFallVelocity;
	
	int					m_rgItems[MAX_ITEMS];
	int					m_fKnownItem;		// True when a new item needs to be added
	int					m_fNewAmmo;			// True when a new item has been added

	unsigned int		m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	float				m_fNextSuicideTime; // the time after which the player can next use the suicide command


// these are time-sensitive things that we keep track of
	float				m_flTimeStepSound;	// when the last stepping sound was made
	float				m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float				m_flSwimTime;		// how long player has been underwater
	float				m_flDuckTime;		// how long we've been ducking
	float				m_flWallJumpTime;	// how long until next walljump

	float				m_flSuitUpdate;					// when to play next suit update
	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int					m_iSuitPlayNext;				// next sentence slot for queue storage;
	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int					m_lastDamageAmount;		// Last damage taken
	float				m_tbdPrev;				// Time-based damage timer

	float				m_flgeigerRange;		// range to nearest radiation source
	float				m_flgeigerDelay;		// delay per update of range msg to client
	int					m_igeigerRangePrev;
	int					m_iStepLeft;			// alternate left/right foot stepping sound
	char				m_szTextureName[CBTEXTURENAMEMAX];	// current texture name we're standing on
	char				m_chTextureType;		// current texture type

	int					m_idrowndmg;			// track drowning damage taken
	int					m_idrownrestored;		// track drowning damage restored

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
												// the hude via the DAMAGE message
	BOOL				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	BOOL				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	BOOL				m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	int					m_iWeapons2;

	EHANDLE				m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())

	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump; // does this player have the longjump module?

	float       m_tSneaking;
	int			m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV
	// usable player items 
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;  // client version of the active item
	CBasePlayerItem *m_pLastItem;
	CBasePlayerItem *m_pLastRef;
	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_SLOTS];
	int	m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector				m_vecAutoAim;
	BOOL				m_fOnTarget;
	int					m_iDeaths;
	float				m_iRespawnFrames;	// used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	int m_fHasRune;
	float m_flRuneHealTime;
	float m_fOffhandTime;
	float m_fThawTime;
	int m_iFreezeCounter;
	float m_fJumpHeight;
	float m_fGrenadeTime;
	float m_fKickEndTime; // pev->nextthink cannot be used consistently

	void CalculateToSelacoSlide( void );
	void StartSelacoSlide( void );
	void TraceHitOfSelacoSlide( void );
	void EndSelacoSlide( void );
	float m_fSelacoButtonTime;
	float m_fSelacoIncrement;
	int m_fSelacoZ;
	int m_fSelacoCount;
	BOOL m_fSelacoSliding;
	BOOL m_fSelacoHit;
	float m_fSelacoLastX, m_fSelacoLastY;

	int m_iAutoWepSwitch;
	int m_iAutoWepThrow;
	int m_iDisplayInfoMessage;

	BOOL HasDisconnected;
	BOOL IsInArena;
	int m_iRoundWins;
	int m_iRoundPlays;
	float m_flForceToObserverTime;
	void ExitObserver( void );
	BOOL IsSpectator( void ) { return ( m_afPhysicsFlags & PFLAG_OBSERVER ? TRUE : FALSE ); };

	BOOL IsArmoredMan;
	float m_fHallelujahTime;

	void CalculateToFlip( void );
	void StartRightFlip( void );
	void StartLeftFlip( void );
	void StartBackFlip( void );
	void StartFrontFlip( void );
	void StartHurricaneKick( void );
	void EndHurricaneKick( void );
	void TraceHitOfFlip( void );
	int m_iKeyboardAcrobatics;
	float m_fFlipTime;
	float m_fFlipButtonTime;
	int m_fFlipType;
	int m_fKickCount;

	void AutoMelee( void );
	float m_flNextAutoMelee;
	BOOL m_iAutoMelee;

	BOOL m_iHoldingChumtoad;
	int m_iChumtoadCounter;
	int m_iCaptureTime;

	// Ricochet
	EHANDLE m_hLastPlayerToHitMe;
	float m_flLastDiscHit;
	int m_iLastDiscBounces;
	float m_flLastDiscHitTeleport;
	Vector m_vecHitVelocity;
	int m_iFlyingDiscs;

	// Prophunt
	int m_iPropsDeployed;

	BOOL GetHeaviestWeapon( CBasePlayerItem *pCurrentWeapon );

	EHANDLE pHeldItem;
	BOOL m_iHoldingItem;

	float m_flNextSantaSound;
	float m_flNextPropSound;

	float m_fNextScreamSound;

	void StartForceGrab( void );
	void TryGrabAgain( void );
	void EndForceGrab( void );
	float m_fForceGrabTime;
	EHANDLE m_Banana;

	virtual void Spawn( void );
	void Pain( void );

//	virtual void Think( void );
	virtual void Jump( void );
#if defined( GRAPPLING_HOOK )
	CHook *pGrapplingHook;
	float m_flNextHook;
#endif
	virtual void Duck( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual Vector GetGunPosition( void );
	virtual int TakeHealth( float flHealth, int bitsDamageType );
	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual void	Killed( entvars_t *pevAttacker, int iGib );
	virtual void Killed( entvars_t *pevAttacker, entvars_t *lastInflictor, int iGib );
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center( ) + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 ); };		// position to shoot at
	virtual void StartSneaking( void ) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking( void ) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking( void ) { return m_tSneaking <= gpGlobals->time; }
	virtual BOOL IsAlive( void ) { return (pev->deadflag == DEAD_NO || pev->deadflag == DEAD_FAKING) && pev->health > 0; }
	virtual BOOL ShouldFadeOnDeath( void ) { return FALSE; }
	virtual	BOOL IsPlayer( void ) { return TRUE; }			// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

	virtual BOOL IsNetClient( void ) { return TRUE; }		// Bots should return FALSE for this, they can't receive NET messages
															// Spectators should return TRUE for this
	virtual const char *TeamID( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	void RenewItems(void);
	void PackDeadPlayerItems( void );
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem *pWeapon );
	BOOL ShouldWeaponSwitch();
	BOOL ShouldWeaponThrow();

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData( void );
	
	static	TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int		ObjectCaps( void ) { return (CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_PORTAL; }
	virtual void	Precache( void );
	BOOL			IsOnLadder( void );
	BOOL			FlashlightIsOn( void );
	void			FlashlightTurnOn( void );
	void			FlashlightTurnOff( void );
	
	void UpdatePlayerSound ( void );
	void DeathSound ( void );

	int Classify ( void );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands( void );
	void CheatImpulseCommands( int iImpulse, BOOL m_iFromClient );

	void StartDeathCam( void );
	void StartObserver( Vector vecPosition, Vector vecViewAngle );

	void AddPoints( int score, BOOL bAllowNegativeScore );
	void AddPointsToTeam( int score, BOOL bAllowNegativeScore );
	BOOL AddPlayerItem( CBasePlayerItem *pItem );
	BOOL RemovePlayerItem( CBasePlayerItem *pItem );
	void DropPlayerItem ( char *pszItemName, BOOL weaponbox = TRUE, BOOL explode = FALSE );
	BOOL HasPlayerItem( CBasePlayerItem *pCheckItem );
	BOOL HasNamedPlayerItem( const char *pszItemName );
	BOOL HasWeapons( void );// do I have ANY weapons?
	void SelectPrevItem( int iItem );
	void SelectNextItem( int iItem );
	void SelectLastItem(void);
	void SelectItem(const char *pstr);
	void ItemPreFrame( void );
	void ItemPostFrame( void );
	void GiveNamedItem( const char *szName );
	void EnableControl(BOOL fControl);

	void ChangeGun();
	BOOL ReleaseHeldItem(float speed);

	int  GiveAmmo( int iAmount, char *szName, int iMax );
	void SendAmmoUpdate(void);

	void WaterMove( void );
	void EXPORT PlayerDeathThink( void );
	void PlayerUse( void );

	void CheckSuitUpdate();
	void SetSuitUpdate(char *name, int fgroup, int iNoRepeat);
	void UpdateGeigerCounter( void );
	void CheckTimeBasedDamage( void );

	BOOL FBecomeProne ( void );
	void BarnacleVictimBitten ( entvars_t *pevBarnacle );
	void BarnacleVictimReleased ( void );
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory( int iAmmoIndex );
	int Illumination( void );

	void ResetAutoaim( void );
	Vector GetAutoaimVector( float flDelta  );
	Vector AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  );

	void ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void DeathMessage( entvars_t *pevKiller );

	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames( void );

	void TabulateAmmo( void );

	void MakeInvisible( void );
	void MakeVisible( void );

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?
	
	//Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	int m_izSBarState[ SBAR_END ];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[ SBAR_STRING_SIZE ];
	char m_SbarString1[ SBAR_STRING_SIZE ];
	
	float m_flNextChatTime;
	float m_iShownWelcomeMessage = -1;
	float m_iShownDualMessage;
	float m_iShowMutatorMessage;
	float m_iShowGameModeMessage;

	void DisplayHudMessage(char const *message, int channel, float x, float y, int r, int g, int b, int effect, float fadein, float fadeout, float holdtime, float fxtime);

	void SendWeatherInfo(void);

	bool canClimb;
	bool isClimbing;
	TraceResult climbTr1, climbTr2, headTr;
	void ClimbingPhysics( void );
	float m_flNextWallClimb;

	void GiveRandomWeapon(const char *szIgnoreList);
	void GiveMelees();
	void GiveExplosives();

	BOOL HasPlayerItemFromID( int nID );

	float nextburntime;
	byte m_bPlayerOnFire;
	void PlayerBurn(void);

	float m_fVoteCoolDown;

	float m_flEjectBrass;
	float m_flEjectShotShell;

	EHANDLE m_pPortal[2];

	float m_fLastSpawnTime;
	float m_fEffectTime;

	int m_iJumpCount;

	EHANDLE pLastAssist;
	int m_iAssists;

	EHANDLE pFlag;
	float m_fFlagTime;
	float m_fCelebrateTime;
	void Celebrate( void );

	float m_fCreditsTime;
	int m_iCreditMode;

	virtual void InitializeEntities( void );
	BOOL m_bSendMessages;

	float m_fVampireHealth;

	float m_fLastHurtTime;
	int m_iBurstCount;
	int m_iModeCount;
	int m_iAmt;

	BOOL m_iExitObserver;

	float m_fFeignTime;

	void Taunt( void );
	BOOL m_iAutoTaunt;
	float m_fTauntTime; // when to taunt
	float m_fTauntFullTime; // when taunt finished
	float m_fTauntCancelTime; // when taunt can be cancelled
	taunt_map_t m_fTaunts[25] = 
	{
		{0,	"I am proud to serve, and proud to die!", "taunt01.wav"},
		{1,	"Nothing can stop me.", "taunt02.wav"},
		{2,	"You died a horrible death!", "taunt03.wav"},
		{0,	"Add another body to the corpse pile.", "taunt04.wav"},
		{1,	"Nice try, now die.", "taunt05.wav"},
		{2,	"You burn nicer than my firewood.", "taunt01.wav"},
		{0,	"How about a slice?", "taunt01.wav"},
		{1,	"I see other players... Boom, boom! Now I don't.", "taunt01.wav"},
		{2,	"And you think you can play this?", "taunt01.wav"},
		{0,	"Hold my beer, I'll fire the nuke.", "taunt01.wav"},
		{1,	"Inbound... oh wait too late, they are dead.", "taunt01.wav"},
		{2,	"Hey, nice model, I like to... hmm, nevermind.", "taunt01.wav"},
		{0,	"You're not woth the shots, so I'll use the wrench.", "taunt01.wav"},
		{1,	"The ground worms are calling you.", "taunt01.wav"},
		{2,	"Your ancestors are displeased!", "taunt01.wav"},
		{0,	"Was that a fart? A grenade you say?", "taunt01.wav"},
		{1,	"Show me your die face.", "taunt01.wav"},
		{2,	"I'm going to cook, but I'm out of meat... oh wait!", "taunt01.wav"},
		{0,	"Knock, knock on the heavens door... Boom!", "taunt01.wav"},
		{1,	"You should of wore your brown pants.", "taunt01.wav"},
		{2,	"You know, that was a nice flight, assisted one with the rpg blast...", "taunt01.wav"},
		{0,	"I'm supposed to taunt you, I'll just shoot you instead.", "taunt01.wav"},
		{1,	"Shoot first, ask questions later", "taunt01.wav"},
		{2,	"My gun loves your face!", "taunt01.wav"},
		{0,	"Can you pull my finger? Oops that was the grenade pin! Catch!", "taunt01.wav"},
	};

	BOOL m_iPlayMusic;
};

inline int FNullEnt( CBaseEntity *ent ) { return (ent == NULL) || FNullEnt( ent->edict() ); }

#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669


extern int	gmsgHudText;
extern BOOL gInitHUD;

float IceExplode(CBaseEntity *pAttacker, CBaseEntity *pEntity, int bitsDamageType);
void Confuse(CBaseEntity *pAttacker, CBaseEntity *pEntity, int bitsDamageType);

#endif // PLAYER_H
