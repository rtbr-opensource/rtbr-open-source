f//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Stun Stick- beating stick with a zappy end
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "c_weapon_stunstick.h"
#include "dlight.h"
#include "r_efx.h"
#include "IEffects.h"
#include "debugoverlay_shared.h"

#ifdef CLIENT_DLL

#include "iviewrender_beams.h"
#include "beam_shared.h"
#include "materialsystem/imaterial.h"
#include "model_types.h"
#include "c_te_effect_dispatch.h"
#include "fx_quad.h"
#include "fx.h"

extern void DrawHalo(IMaterial* pMaterial, const Vector &source, float scale, float const *color, float flHDRColorScale = 1.0f);
extern void FormatViewModelAttachment(Vector &vOrigin, bool bInverse);

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar metropolice_move_and_melee;


#ifdef MAPBASE
ConVar    sk_plr_dmg_stunstick("sk_plr_dmg_stunstick", "0");
ConVar    sk_npc_dmg_stunstick("sk_npc_dmg_stunstick", "0");
#endif

//-----------------------------------------------------------------------------
// CWeaponStunStick
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponStunStick, DT_WeaponStunStick)

BEGIN_NETWORK_TABLE(CWeaponStunStick, DT_WeaponStunStick)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_bActive)),
RecvPropFloat(RECVINFO(m_flChargeLevel)),
#else
SendPropInt(SENDINFO(m_bActive), 1, SPROP_UNSIGNED),
#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponStunStick)
END_PREDICTION_DATA()

BEGIN_DATADESC(CWeaponStunStick)
DEFINE_FIELD(m_flChargeLevel, FIELD_FLOAT),
DEFINE_FIELD(m_bActive, FIELD_BOOLEAN),
DEFINE_FIELD(m_bStunChargeActive, FIELD_BOOLEAN),
END_DATADESC()

LINK_ENTITY_TO_CLASS(weapon_stunstick, CWeaponStunStick);
PRECACHE_WEAPON_REGISTER(weapon_stunstick);

//-----------------------------------------------------------------------------
// Purpose: Fidget Animation Stuff
//-----------------------------------------------------------------------------
void CWeaponStunStick::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();
}

void CWeaponStunStick::OnRestore() {
	ParticleProp()->StopEmissionAndDestroyImmediately(m_hStunstickParticle);

	BaseClass::OnRestore();

	m_bStunChargeActive = false;
}
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponStunStick::CWeaponStunStick(void)
{
	// HACK:  Don't call SetStunState because this tried to Emit a sound before
	//  any players are connected which is a bug
	m_bActive = false;

#ifdef CLIENT_DLL
	m_bSwungLastFrame = false;
	m_flFadeTime = FADE_DURATION;	// Start off past the fade point
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStunStick::Spawn()
{

	Precache();

	BaseClass::Spawn();
	AddSolidFlags(FSOLID_NOT_STANDABLE);
}

void CWeaponStunStick::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound("Weapon_StunStick.Activate");
	PrecacheScriptSound("Weapon_StunStick.Deactivate");
	PrecacheParticleSystem("weapon_stunstick_impact");
	PrecacheParticleSystem("weapon_stunstick_charge");

	PrecacheModel(STUNSTICK_BEAM_MATERIAL);
	PrecacheModel("sprites/light_glow02_add.vmt");
	PrecacheModel("effects/blueflare1.vmt");
	PrecacheModel("sprites/light_glow02_add_noz.vmt");
}
//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponStunStick::GetDamageForActivity(Activity hitActivity)
{
#ifdef MAPBASE
	if ((GetOwner() != NULL) && (GetOwner()->IsPlayer()))
		return sk_plr_dmg_stunstick.GetFloat();

	return sk_npc_dmg_stunstick.GetFloat();
#else
	return 40.0f;
#endif
}

//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
extern ConVar sk_crowbar_lead_time;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStunStick::ImpactEffect(trace_t &traceHit)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	CEffectData	data;

	data.m_vNormal = traceHit.plane.normal;
	data.m_vOrigin = traceHit.endpos + (data.m_vNormal * 4.0f);

	Vector vecDir;

	AngleVectors(pOwner->EyeAngles(), &vecDir);

	QAngle EyeAngle = pOwner->EyeAngles();
	Vector vecAbsStart = pOwner->EyePosition();
	Vector vecAbsEnd = vecAbsStart + (vecDir * MAX_TRACE_LENGTH);

	trace_t tr;
	UTIL_TraceLine(vecAbsStart, vecAbsEnd, MASK_ALL, pOwner, COLLISION_GROUP_NONE, &tr);

	cplane_t impactPlane = tr.plane;
	Vector impactNormal = impactPlane.normal;
	QAngle impactAngles;
	VectorAngles(impactNormal, impactAngles);

	//DispatchParticleEffect("weapon_stunstick_impact", PATTACH_POINT_FOLLOW, pOwner->GetViewModel(), "1", true);
	DispatchParticleEffect("weapon_stunstick_impact", tr.endpos, impactAngles, this);


	UTIL_ImpactTrace(&traceHit, DMG_CLUB);
}

void CWeaponStunStick::OnPickedUp(CBaseCombatCharacter *pNewOwner)
{
	BaseClass::OnPickedUp(pNewOwner);
	//SendWeaponAnim(ACT_VM_FIRSTDRAW);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the state of the stun stick
//-----------------------------------------------------------------------------
void CWeaponStunStick::SetStunState(bool state)
{
	m_bActive = state;
	CBaseCombatCharacter *pOwner = GetOwner();
	if (m_bActive)
	{
		//FIXME: START - Move to client-side
		//if (pOwner->IsNPC()) {
		Vector vecAttachment;
		QAngle vecAttachmentAngles;
		GetAttachment("1", vecAttachment, vecAttachmentAngles);
		if (pOwner->IsNPC()) {
			g_pEffects->Sparks(vecAttachment);
		}

		//}
		//FIXME: END - Move to client-side

		EmitSound("Weapon_StunStick.Activate");
	}
	else
	{
		EmitSound("Weapon_StunStick.Deactivate");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Deploy(void)
{
	SetStunState(true);
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponStunStick::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	//DevMsg("EA SPORTS");
	SendWeaponAnim(ACT_VM_HOLSTER);
	if (BaseClass::Holster(pSwitchingTo) == false)
		return false;

	SetStunState(false);
	//SetWeaponVisible( false );

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecVelocity - 
//-----------------------------------------------------------------------------
void CWeaponStunStick::Drop(const Vector &vecVelocity)
{
	SetStunState(false);

}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStunStick::GetStunState(void)
{
	return m_bActive;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Get the attachment point on a viewmodel that a base weapon is using
//-----------------------------------------------------------------------------
bool UTIL_GetWeaponAttachment(C_BaseCombatWeapon *pWeapon, int attachmentID, Vector &absOrigin, QAngle &absAngles)
{
	// This is already correct in third-person
	if (pWeapon && pWeapon->ShouldDrawUsingViewModel() == false)
	{
		return pWeapon->GetAttachment(attachmentID, absOrigin, absAngles);
	}

	// Otherwise we need to translate the attachment to the viewmodel's version and reformat it
	CBasePlayer *pOwner = ToBasePlayer(pWeapon->GetOwner());

	if (pOwner != NULL)
	{
		int ret = pOwner->GetViewModel()->GetAttachment(attachmentID, absOrigin, absAngles);
		FormatViewModelAttachment(absOrigin, true);

		return ret;
	}

	// Wasn't found
	return false;
}

#define	BEAM_ATTACH_CORE_NAME	"sparkrear"

//-----------------------------------------------------------------------------
// Purpose: Sets up the attachment point lookup for the model
//-----------------------------------------------------------------------------
void C_WeaponStunStick::SetupAttachmentPoints(void)
{
	// Setup points for both types of views
	if (ShouldDrawUsingViewModel())
	{
		const char *szBeamAttachNamesTop[NUM_BEAM_ATTACHMENTS] =
		{
			"spark1a", "spark2a", "spark3a", "spark4a",
			"spark5a", "spark6a", "spark7a", "spark8a",
			"spark9a",
		};

		const char *szBeamAttachNamesBottom[NUM_BEAM_ATTACHMENTS] =
		{
			"spark1b", "spark2b", "spark3b", "spark4b",
			"spark5b", "spark6b", "spark7b", "spark8b",
			"spark9b",
		};

		// Lookup and store all connections
		for (int i = 0; i < NUM_BEAM_ATTACHMENTS; i++)
		{
			m_BeamAttachments[i].IDs[0] = LookupAttachment(szBeamAttachNamesTop[i]);
			m_BeamAttachments[i].IDs[1] = LookupAttachment(szBeamAttachNamesBottom[i]);
		}

		// Setup the center beam point
		m_BeamCenterAttachment = LookupAttachment(BEAM_ATTACH_CORE_NAME);
	}
	else
	{
		// Setup the center beam point
		m_BeamCenterAttachment = 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws the stunstick model (with extra effects)
//-----------------------------------------------------------------------------
int C_WeaponStunStick::DrawModel(int flags)
{
	if (ShouldDraw() == false)
		return 0;

	// Only render these on the transparent pass
#ifdef MAPBASE
	if (m_bActive && flags & STUDIO_TRANSPARENCY)
#else
	if (flags & STUDIO_TRANSPARENCY)
#endif
	{
		DrawEffects();
		return 1;
	}

	return BaseClass::DrawModel(flags);
}

//-----------------------------------------------------------------------------
// Purpose: Randomly adds extra effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::ClientThink(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!GetStunState() || m_flChargeLevel == 0.0f) {
		if (m_bStunChargeActive) {
			m_bStunChargeActive = false;
			ParticleProp()->StopEmissionAndDestroyImmediately(m_hStunstickParticle);
		}
	}
	if (GetStunState()) {
		if (!m_bStunChargeActive && m_flChargeLevel > 0.0f) {
			m_bStunChargeActive = true;

			CNewParticleEffect *pChargedParticle = ParticleProp()->Create("weapon_stunstick_charge", PATTACH_ABSORIGIN_FOLLOW);

			m_hStunstickParticle = pChargedParticle;
		}
		if (m_bStunChargeActive) {
			Vector vecAttachment;
			QAngle vecAttachmentAngles;

			GetAttachment("1", vecAttachment, vecAttachmentAngles);

			dlight_t* dl = effects->CL_AllocDlight(LIGHT_INDEX_MUZZLEFLASH + index);
			dl->radius = 72 * m_flChargeLevel;
			dl->color.exponent = 4 * m_flChargeLevel;
			dl->origin = vecAttachment + Vector(0, 0, 16);
			dl->decay = dl->radius / 0.1f;
			dl->die = gpGlobals->curtime + 1.0f;
			dl->color.r = 163;
			dl->color.g = 98;
			dl->color.b = 34;

			ParticleProp()->AddControlPoint(m_hStunstickParticle, 0, ToBasePlayer(pOwner)->GetViewModel(), PATTACH_POINT_FOLLOW, "2");
			ParticleProp()->AddControlPoint(m_hStunstickParticle, 1, null, PATTACH_WORLDORIGIN, 0, Vector(m_flChargeLevel, 0, 0));
		}



		if (pOwner->IsNPC()) {
			Vector vecAttachment;
			QAngle vecAttachmentAngles;

			GetAttachment("1", vecAttachment, vecAttachmentAngles);

			dlight_t* el = effects->CL_AllocElight(LIGHT_INDEX_MUZZLEFLASH + index);
			el->radius = random->RandomInt(32, 64);
			el->color.exponent = 3;
			el->origin = vecAttachment;
			el->decay = el->radius / 0.1f;
			el->die = gpGlobals->curtime + 1.0f;
			el->color.r = 163;
			el->color.g = 98;
			el->color.b = 34;
		}
	}
	if (InSwing() == false)
	{
		if (m_bSwungLastFrame)
		{
			// Start fading
			m_flFadeTime = gpGlobals->curtime;
			m_bSwungLastFrame = false;
		}

		return;
	}

	// Remember if we were swinging last frame
	m_bSwungLastFrame = InSwing();

	if (IsEffectActive(EF_NODRAW))
		return;

	if (ShouldDrawUsingViewModel())
	{
		// Update our effects
		if (gpGlobals->frametime != 0.0f && (random->RandomInt(0, 3) == 0))
		{
			Vector	vecOrigin;
			QAngle	vecAngles;

			// Inner beams
			BeamInfo_t beamInfo;

			int attachment = random->RandomInt(0, 15);

			UTIL_GetWeaponAttachment(this, attachment, vecOrigin, vecAngles);
			::FormatViewModelAttachment(vecOrigin, false);

			CBasePlayer *pOwner = ToBasePlayer(GetOwner());
			CBaseEntity *pBeamEnt = pOwner->GetViewModel();

			beamInfo.m_vecStart = vec3_origin;
			beamInfo.m_pStartEnt = pBeamEnt;
			beamInfo.m_nStartAttachment = attachment;

			beamInfo.m_pEndEnt = NULL;
			beamInfo.m_nEndAttachment = -1;
			beamInfo.m_vecEnd = vecOrigin + RandomVector(-8, 8);

			beamInfo.m_pszModelName = STUNSTICK_BEAM_MATERIAL;
			beamInfo.m_flHaloScale = 0.0f;
			beamInfo.m_flLife = 0.05f;
			beamInfo.m_flWidth = random->RandomFloat(1.0f, 2.0f);
			beamInfo.m_flEndWidth = 0;
			beamInfo.m_flFadeLength = 0.0f;
			beamInfo.m_flAmplitude = random->RandomFloat(16, 32);
			beamInfo.m_flBrightness = 255.0;
			beamInfo.m_flSpeed = 0.0;
			beamInfo.m_nStartFrame = 0.0;
			beamInfo.m_flFrameRate = 1.0f;
			beamInfo.m_flRed = 255.0f;;
			beamInfo.m_flGreen = 255.0f;
			beamInfo.m_flBlue = 255.0f;
			beamInfo.m_nSegments = 16;
			beamInfo.m_bRenderable = true;
			beamInfo.m_nFlags = 0;

			beams->CreateBeamEntPoint(beamInfo);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Starts the client-side version thinking
//-----------------------------------------------------------------------------
void C_WeaponStunStick::OnDataChanged(DataUpdateType_t updateType)
{

	BaseClass::OnDataChanged(updateType);
	if (updateType == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
		SetupAttachmentPoints();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells us we're always a translucent entity
//-----------------------------------------------------------------------------
RenderGroup_t C_WeaponStunStick::GetRenderGroup(void)
{
	return RENDER_GROUP_TWOPASS;
}

//-----------------------------------------------------------------------------
// Purpose: Tells us we're always a translucent entity
//-----------------------------------------------------------------------------
bool C_WeaponStunStick::InSwing(void)
{
	int activity = GetActivity();

	// FIXME: This is needed until the actual animation works
	if (ShouldDrawUsingViewModel() == false)
		return true;

	// These are the swing activities this weapon can play
	if (activity == GetPrimaryAttackActivity() ||
		activity == GetSecondaryAttackActivity() ||
		activity == ACT_VM_MISSCENTER ||
		activity == ACT_VM_MISSCENTER2)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Draw our special effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::DrawThirdPersonEffects(void)
{
	Vector	vecOrigin;
	QAngle	vecAngles;
	float	color[3];
	float	scale;

	CMatRenderContextPtr pRenderContext(materials);
	IMaterial *pMaterial = materials->FindMaterial(STUNSTICK_GLOW_MATERIAL, NULL, false);
	pRenderContext->Bind(pMaterial);

	// Get bright when swung
	if (InSwing())
	{
		color[0] = color[1] = color[2] = 0.4f;
		scale = 22.0f;
	}
	else
	{
		color[0] = color[1] = color[2] = 0.1f;
		scale = 20.0f;
	}

	// Draw an all encompassing glow around the entire head
	UTIL_GetWeaponAttachment(this, m_BeamCenterAttachment, vecOrigin, vecAngles);
	DrawHalo(pMaterial, vecOrigin, scale, color);

	if (InSwing())
	{
		pMaterial = materials->FindMaterial(STUNSTICK_GLOW_MATERIAL2, NULL, false);
		pRenderContext->Bind(pMaterial);

		color[0] = color[1] = color[2] = random->RandomFloat(0.6f, 0.8f);
		scale = random->RandomFloat(4.0f, 6.0f);

		// Draw an all encompassing glow around the entire head
		UTIL_GetWeaponAttachment(this, m_BeamCenterAttachment, vecOrigin, vecAngles);
		DrawHalo(pMaterial, vecOrigin, scale, color);

		// Update our effects
		if (gpGlobals->frametime != 0.0f && (random->RandomInt(0, 5) == 0))
		{
			Vector	vecOrigin;
			QAngle	vecAngles;

			GetAttachment(1, vecOrigin, vecAngles);

			Vector	vForward;
			AngleVectors(vecAngles, &vForward);

			Vector vEnd = vecOrigin - vForward * 1.0f;

			// Inner beams
			BeamInfo_t beamInfo;

			beamInfo.m_vecStart = vEnd;
			Vector	offset = RandomVector(-12, 8);

			offset += Vector(4, 4, 4);
			beamInfo.m_vecEnd = vecOrigin + offset;

			beamInfo.m_pStartEnt = cl_entitylist->GetEnt(BEAMENT_ENTITY(entindex()));
			beamInfo.m_pEndEnt = cl_entitylist->GetEnt(BEAMENT_ENTITY(entindex()));
			beamInfo.m_nStartAttachment = 1;
			beamInfo.m_nEndAttachment = -1;

			beamInfo.m_nType = TE_BEAMTESLA;
			beamInfo.m_pszModelName = STUNSTICK_BEAM_MATERIAL;
			beamInfo.m_flHaloScale = 0.0f;
			beamInfo.m_flLife = 0.01f;
			beamInfo.m_flWidth = random->RandomFloat(1.0f, 3.0f);
			beamInfo.m_flEndWidth = 0;
			beamInfo.m_flFadeLength = 0.0f;
			beamInfo.m_flAmplitude = random->RandomFloat(1, 2);
			beamInfo.m_flBrightness = 255.0;
			beamInfo.m_flSpeed = 0.0;
			beamInfo.m_nStartFrame = 0.0;
			beamInfo.m_flFrameRate = 1.0f;
			beamInfo.m_flRed = 255.0f;;
			beamInfo.m_flGreen = 255.0f;
			beamInfo.m_flBlue = 255.0f;
			beamInfo.m_nSegments = 16;
			beamInfo.m_bRenderable = true;
			beamInfo.m_nFlags = FBEAM_SHADEOUT;

			beams->CreateBeamPoints(beamInfo);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw our special effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::DrawFirstPersonEffects(void)
{
	Vector	vecOrigin;
	QAngle	vecAngles;
	float	color[3];
	float	scale;

	CMatRenderContextPtr pRenderContext(materials);
	IMaterial *pMaterial = materials->FindMaterial(STUNSTICK_GLOW_MATERIAL_NOZ, NULL, false);
	// FIXME: Needs to work with new IMaterial system!
	pRenderContext->Bind(pMaterial);

	// Find where we are in the fade
	float fadeAmount = RemapValClamped(gpGlobals->curtime, m_flFadeTime, m_flFadeTime + FADE_DURATION, 1.0f, 0.1f);

	// Get bright when swung
	if (InSwing())
	{
		color[0] = color[1] = color[2] = 0.4f;
		scale = 22.0f;
	}
	else
	{
		color[0] = color[1] = color[2] = 0.4f * fadeAmount;
		scale = 20.0f;
	}

	if (color[0] > 0.0f)
	{
		// Draw an all encompassing glow around the entire head
		UTIL_GetWeaponAttachment(this, m_BeamCenterAttachment, vecOrigin, vecAngles);
		DrawHalo(pMaterial, vecOrigin, scale, color);
	}

	// Draw bright points at each attachment location
	for (int i = 0; i < (NUM_BEAM_ATTACHMENTS * 2) + 1; i++)
	{
		if (InSwing())
		{
			color[0] = color[1] = color[2] = random->RandomFloat(0.05f, 0.5f);
			scale = random->RandomFloat(4.0f, 5.0f);
		}
		else
		{
			color[0] = color[1] = color[2] = random->RandomFloat(0.05f, 0.5f) * fadeAmount;
			scale = random->RandomFloat(4.0f, 5.0f) * fadeAmount;
		}

		if (color[0] > 0.0f)
		{
			UTIL_GetWeaponAttachment(this, i, vecOrigin, vecAngles);
			DrawHalo(pMaterial, vecOrigin, scale, color);
		}
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Draw our special effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::DrawNPCEffects(void)
{
	if (m_bActive)
	{
		Vector	vecOrigin;
		QAngle	vecAngles;
		float	color[3];

		color[0] = color[1] = color[2] = random->RandomFloat(0.1f, 0.2f);

		GetAttachment(1, vecOrigin, vecAngles);

		Vector	vForward;
		AngleVectors(vecAngles, &vForward);

		Vector vEnd = vecOrigin - vForward * 1.0f;

		IMaterial *pMaterial = materials->FindMaterial("effects/stunstick", NULL, false);

		CMatRenderContextPtr pRenderContext(materials);
		pRenderContext->Bind(pMaterial);
		DrawHalo(pMaterial, vEnd, random->RandomFloat(4.0f, 6.0f), color);

		color[0] = color[1] = color[2] = random->RandomFloat(0.9f, 1.0f);

		DrawHalo(pMaterial, vEnd, random->RandomFloat(2.0f, 3.0f), color);
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Draw our special effects
//-----------------------------------------------------------------------------
void C_WeaponStunStick::DrawEffects(void)
{
	if (ShouldDrawUsingViewModel())
	{
		DrawFirstPersonEffects();
	}
#ifdef MAPBASE
	else if (GetOwner() && GetOwner()->IsNPC())
	{
		// Original HL2 stunstick FX
		DrawNPCEffects();
	}
#endif
	else
	{
		DrawThirdPersonEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Viewmodel was drawn
//-----------------------------------------------------------------------------
void C_WeaponStunStick::ViewModelDrawn(C_BaseViewModel *pBaseViewModel)
{
	// Don't bother when we're not deployed
	if (IsWeaponVisible())
	{
		// Do all our special effects
		DrawEffects();
	}

	BaseClass::ViewModelDrawn(pBaseViewModel);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a cheap glow quad at our impact point (with sparks)
//-----------------------------------------------------------------------------
void StunstickImpactCallback(const CEffectData &data)
{
	float scale = random->RandomFloat(16, 32);

	FX_AddQuad(data.m_vOrigin,
		data.m_vNormal,
		scale,
		scale*2.0f,
		1.0f,
		1.0f,
		0.0f,
		0.0f,
		random->RandomInt(0, 360),
		0,
		Vector(1.0f, 1.0f, 1.0f),
		0.1f,
		"sprites/light_glow02_add",
		0);

	FX_Sparks(data.m_vOrigin, 1, 2, data.m_vNormal, 6, 64, 256);
}

DECLARE_CLIENT_EFFECT("StunstickImpact", StunstickImpactCallback);

#endif