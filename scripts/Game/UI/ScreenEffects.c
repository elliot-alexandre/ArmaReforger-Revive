enum DepthOfFieldTypes
{
	NONE,
	SIMPLE,
	BOKEH,
};

class SCR_ScreenEffects : SCR_InfoDisplayExtended
{
	// Pointer for getting SCR_ScreenEffects
	private static SCR_ScreenEffects s_pScreenEffects;
		
	// PP constants
	//DOFBokeh AddDOFBokehEffect()
	private const int FOCUSDISTANCE_MULTIPIER 							= 15;
	private const int FOCALLENGTH_MAX 									= 5000;
	protected const int FOCALLENGTHNEAR_INTENSITY 						= 225;
	
	//DOFNormal AddDOFEffect()
	protected const float FOCALDISTANCE_INTENSITY 						= 0.3;
	protected const float STANDARD_FOCALCHANGE_NEAR 					= 0.25;
	private const int SIMPLEDOF_FOCALCHANGE_MAX			 				= 10000;
	private const int SIMPLEDOF_EFFECT_MAX								= 1500;

	// Variables connected to a material, need to be static
	static const int COLORS_PP_PRIORITY									= 5;
	static const int RADIAL_BLUR_PRIORITY								= 6;
	static const int GAUSS_BLUR_PRIORITY								= 7;
	static const int DEPTH_OF_FIELD_PRIORITY							= 6;

	// Play Animation of BlackoutEffect()
	protected const float BLACKOUTEFFECT_OPACITY_FADEIN_DURATION		= 20;
	protected const float BLACKOUTEFFECT_PROGRESSION_FADEIN_DURATION	= 10;
	protected const float BLACKOUT_OPACITY_MULTIPLIER					= 0.20;
	private const float BLACKOUT_ALPHAMASK_MULTIPLIER					= 0.25;
	protected const float BLACKOUTEFFECT_OPACITY_FADEOUT_DURATION 		= 2;
	protected const float BLACKOUTEFFECT_PROGRESSION_FADEOUT_DURATION	= 1;
	
	// Play Animation of ClearDOFOutEffect()
	protected const float DOFOUT_OPACITY_FADEOUT_DURATION				= 2;
	protected const float DOFOUT_PROGRESSION_FADEOUT_DURATION			= 1;

	// Play Animation of DeathEffect()
	private const float DEATHEFFECT_START_OPACITY						= 0.7;
	protected const float DEATHEFFECT_FADEIN_OPACITY_DURATION 			= 0.8;
	protected const float DEATHEFFECT_FADEIN_PROGRESSION_DURACTION		= 0.5;
	protected const float DEATHEFFECT_FADEIN_OPACITY_TARGET				= 1;
	protected const float DEATHEFFECT_FADEIN_PROGRESSION_TARGET 		= 1;

	// Play Animation of InstaDeathEffect()
	protected const float INSTADEATHEFFECT_START_OPACITY				= 0.4;
	protected const float INSTADEATHEFFECT_START_PROGRESSION			= 0.4;
	protected const float INSTADEATHEFFECT_FADEIN_OPACITY_DURATION 		= 8.5;
	protected const float INSTADEATHEFFECT_FADEIN_PROGRESSION_DURACTION	= 10;
	protected const float INSTADEATHEFFECT_FADEIN_OPACITY_TARGET		= 1;
	protected const float INSTADEATHEFFECT_FADEIN_PROGRESSION_TARGET 	= 1;

	// Play Animation of StaminaEffects()
	protected const float STAMINAEFFECT_INITIAL_OPACITY_TARGET			= 0.75;
	protected const float STAMINAEFFECT_FADEIN_PROGRESSION_TARGET		= 0.35;
	protected const float STAMINAEFFECT_FADEIN_PROGRESSION_DURATION	 	= 1;
	protected const float STAMINA_CLEAREFFECT_DELAY 					= 2000;
	protected const float STAMINA_EFFECT_THRESHOLD 						= 0.45;
	private	const float STAMINA_BLUR_DIVIDER							= 6;

	// Play Animation of CreateEffectOverTime()
	protected const int   BLEEDING_REPEAT_DELAY 						= 2500;
	protected const float BLEEDINGEFFECT_OPACITY_FADEOUT_1_DURATION 	= 0.2;
	protected const float BLEEDINGEFFECT_PROGRESSION_FADEOUT_1_DURATION = 0.2;
	protected const float BLEEDINGEFFECT_OPACITY_FADEOUT_2_DURATION 	= 0.3;
	protected const float BLEEDINGEFFECT_PROGRESSION_FADEOUT_2_DURATION = 3;
	protected const float BLEEDINGEFFECT_OPACITY_FADEIN_1_DURATION		= 1;
	protected const float BLEEDINGEFFECT_PROGRESSION_FADEIN_1_DURATION  = 4.5;

	//Saturation
	static float s_fSaturation 											= 1;

	//Blurriness
	private static float s_fBlurriness;
	private static float s_fBlurrinessSize;
	private static float s_fBlurFactor;
	private static bool s_bEnableRadialBlur;

	//GaussBlurriness
	private const float MOMENTARY_DAMAGE_BLUR_DURATION					= 0.5;	// MomentaryDamageEffect() duration in seconds
	private float m_fMomentaryDamage;
	private static float s_fGaussBlurriness;
	private static bool s_bRemoveGaussBlur 								= true;
	private float m_fGaussBlurReduction 								= 1;

	//DepthOfField
	private static float s_fFocalChange									= 10000;
	private static float s_fFocalDistance;
	private static float s_fFocalChangeNear;
	private static bool s_bSkipFar;

	//DepthOfFieldBOKEH
	private static float s_fFocalLength									= 0.1;	// Blur originates from horizon, higher is closer to camera
	private static float s_fFocusDistance;
	private static float s_fFocalLengthNear;									// Blur originates from camera, higher is further
	
	private int m_iDesiredDofType;

	// Widgets
	private ImageWidget 							m_wSupression;
	protected ImageWidget							m_wDeath;
	private ImageWidget								m_wBlackOut;
	private ImageWidget								m_wDOFOut;
	protected ImageWidget 							m_wBloodEffect1;
	protected ImageWidget 							m_wBloodEffect2;
	private ImageWidget 							m_wFadeInOut;
	private int m_iEffectNo 						= 1;

	// Owner data
	protected ChimeraCharacter 						m_pCharacterEntity;
	protected SignalsManagerComponent 				m_pSignalsManager;

	// Character
	protected EDamageState			 				m_eLifeState;
	protected ScriptedHitZone 						m_pHealthHitZone;
	protected ScriptedHitZone 						m_pHeadHitZone;
	protected HitZone 								m_pBloodHZ;
	protected SCR_CharacterDamageManagerComponent	m_pDamageManager;

	protected bool m_bPlayerOutsideCharacter;
	protected bool m_bBleedingEffect;
	protected bool m_bIsBleeding;

	// Stamina
	protected int m_iStaminaSignal 					= -1;
	protected bool m_bStaminaEffectActive;

	//Sound effects
	protected const string SOUND_DEATH_SLOW 		= "SOUND_DEATH_SLOW";
	protected const string SOUND_DEATH_FAST 		= "SOUND_DEATH_FAST";

	// Enabling/Disabling of PP fx
	private static bool s_bNearDofEffect;
	private static bool s_bEnableDOFBokeh;
	private static bool s_bEnableDOF;
	private static bool s_bEnableGaussBlur;
	private static bool s_bEnableSaturation;

	//------------------------------------------------------------------------------------------------
	override void DisplayStartDraw(IEntity owner)
	{
		super.DisplayStartDraw(owner);

		SettingsChanged();
		CharacterChanged();

		m_wDeath = ImageWidget.Cast(m_wRoot.FindAnyWidget("DeathOverlay"));
		m_wSupression = ImageWidget.Cast(m_wRoot.FindAnyWidget("SuppressionVignette"));
		m_wBloodEffect1 = ImageWidget.Cast(m_wRoot.FindAnyWidget("BloodVignette1"));
		m_wBloodEffect2 = ImageWidget.Cast(m_wRoot.FindAnyWidget("BloodVignette2"));
		m_wBlackOut = ImageWidget.Cast(m_wRoot.FindAnyWidget("BlackOut"));
		m_wDOFOut = ImageWidget.Cast(m_wRoot.FindAnyWidget("DOFOut"));
		
		GetGame().OnUserSettingsChangedInvoker().Insert(SettingsChanged);

		// Clear all effects that are applied otherwise
		ClearEffects();

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return;

		playerController.m_OnControlledEntityChanged.Insert(CharacterChanged);

		// Register to debug menu
		DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_SCREEN_EFFECTS,"","Hit Effect Debug","Character");
		DiagMenu.SetValue(SCR_DebugMenuID.DEBUGUI_CHARACTER_SCREEN_EFFECTS,0);

		DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_POSTPROCESSING_EFFECTS,"","Postprocessing Effect Debug","Character");
		DiagMenu.SetValue(SCR_DebugMenuID.DEBUGUI_CHARACTER_POSTPROCESSING_EFFECTS,0);
	}

	//------------------------------------------------------------------------------------------------
	void SettingsChanged()
	{
		// Get desired type of DOF
		BaseContainer m_VideoSettings = GetGame().GetGameUserSettings().GetModule("SCR_VideoSettings");
		if (m_VideoSettings)
		{
			m_VideoSettings.Get("m_iDofType", m_iDesiredDofType);
			m_VideoSettings.Get("m_bNearDofEffect", s_bNearDofEffect);
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		int camId = world.GetCurrentCameraId();
	 	world.SetCameraPostProcessEffect(camId, COLORS_PP_PRIORITY, PostProcessEffectType.Colors, "{7C202A913EB8B1A9}UI/Materials/ScreenEffects_ColorPP.emat");
		world.SetCameraPostProcessEffect(camId, RADIAL_BLUR_PRIORITY,PostProcessEffectType.RadialBlur, "{B011FE0AD21E2447}UI/Materials/ScreenEffects_BlurPP.emat");
		world.SetCameraPostProcessEffect(camId, GAUSS_BLUR_PRIORITY,PostProcessEffectType.GaussFilter, "{790527EE96732730}UI/Materials/ScreenEffects_GaussBlurPP.emat");

		if (m_iDesiredDofType == DepthOfFieldTypes.SIMPLE)
		{
			world.SetCameraPostProcessEffect(camId,DEPTH_OF_FIELD_PRIORITY,PostProcessEffectType.DepthOfFieldBokeh, "");
			world.SetCameraPostProcessEffect(camId,DEPTH_OF_FIELD_PRIORITY,PostProcessEffectType.DepthOfField,"{403795B9349EA61C}UI/Materials/ScreenEffects_DepthOfFieldPP.emat");
			s_bEnableDOFBokeh = false;
			s_bEnableDOF = true;
		}
		else if (m_iDesiredDofType == DepthOfFieldTypes.BOKEH)
		{
			world.SetCameraPostProcessEffect(camId,DEPTH_OF_FIELD_PRIORITY,PostProcessEffectType.DepthOfField, "");
			world.SetCameraPostProcessEffect(camId,DEPTH_OF_FIELD_PRIORITY,PostProcessEffectType.DepthOfFieldBokeh,"{5CFBB3297D669D9C}UI/Materials/ScreenEffects_DepthOfFieldBokehPP.emat");
			s_bEnableDOFBokeh = true;
			s_bEnableDOF = false;
		}
	}

	//------------------------------------------------------------------------------------------------
 	void CharacterChanged(IEntity from = null, IEntity to = null)
	{
		UnregisterEffects();

		//Getting essential components for the script to function
		if (!m_wRoot)
			return;

		if (to)
			m_pCharacterEntity = ChimeraCharacter.Cast(to);
		else if (GetGame().GetPlayerController())
			m_pCharacterEntity = ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());

		if (!m_pCharacterEntity)
			return;

		// Clear effects only when new controlled character is aqcuired so death effects persist
		ClearEffects();
		
		m_pDamageManager = SCR_CharacterDamageManagerComponent.Cast(m_pCharacterEntity.GetDamageManager());
		if (!m_pDamageManager)
			Debug.Error("Null pointer to SCR_CharacterDamageManagerComponent");

		// define hitzones for later getting
		m_pHealthHitZone = ScriptedHitZone.Cast(m_pDamageManager.GetDefaultHitZone());
		m_pHeadHitZone = ScriptedHitZone.Cast(m_pDamageManager.GetHeadHitZone());

		// Invoker for momentary damage events and DOT damage events
		m_pDamageManager.GetOnDamageStateChanged().Insert(OnDamageStateChanged);
		m_pDamageManager.GetOnDamageOverTimeAdded().Insert(OnDamageOverTimeAdded);
		m_pDamageManager.GetOnDamageOverTimeRemoved().Insert(OnDamageOverTimeRemoved);
		m_pDamageManager.GetOnDamage().Insert(OnDamage);
		if (m_pHeadHitZone)
			m_pHeadHitZone.GetOnDamageStateChanged().Insert(InstaDeathEffect);

		// Establish essential pointers to simplify getting them in the future
		m_eLifeState = m_pDamageManager.GetState();
		m_pBloodHZ = m_pDamageManager.GetBloodHitZone();
		
		if (!m_pBloodHZ)
			Debug.Error("No BloodHZ inside damagemanager!");

		// In case player started bleeding before invokers were established, check if already bleeding
		if (m_pDamageManager.IsDamagedOverTime(EDamageType.BLEEDING))
			OnDamageOverTimeAdded(EDamageType.BLEEDING, m_pDamageManager.GetDamageOverTime(EDamageType.BLEEDING));

		// Audio components for damage-related audio effects
		m_pSignalsManager = SignalsManagerComponent.Cast(m_pCharacterEntity.FindComponent(SignalsManagerComponent));
		if (m_pSignalsManager)
			m_iStaminaSignal = m_pSignalsManager.FindSignal("Exhaustion");
	}

	//------------------------------------------------------------------------------------------------
	override event void DisplayUpdate(IEntity owner, float timeSlice)
	{
		if (!GetGame().GetCameraManager())
			return;

		#ifdef ENABLE_DIAG
		//	TODO: Move to OnDebug Event
			ProcessDebug();
		#endif

		CameraBase currentCamera = GetGame().GetCameraManager().CurrentCamera();
		PlayerCamera playerCamera = GetGame().GetPlayerController().GetPlayerCamera();
		if (!playerCamera || currentCamera != playerCamera)
		{
			m_wRoot.SetVisible(false);
			m_bPlayerOutsideCharacter = true;
			s_bEnableDOFBokeh = false;
			s_bEnableDOF = false;
			s_bEnableGaussBlur = false;
			s_bEnableSaturation = false;
			return;
		}

		m_bPlayerOutsideCharacter = false;
		m_wRoot.SetVisible(true);

		if (m_pSignalsManager)
			FindStaminaValues();

		if (m_fGaussBlurReduction > 0)
			MomentaryDamageEffect(timeSlice);
		else
			s_fGaussBlurriness = 0;

		AddDesaturationEffect();
		
		bool allowed = IsNearDOFAllowed();
		if (m_iDesiredDofType == DepthOfFieldTypes.SIMPLE)
			AddDOFEffect(timeSlice, allowed);
		else if (m_iDesiredDofType == DepthOfFieldTypes.BOKEH)
			AddDOFBokehEffect(allowed);
	}

	//------------------------------------------------------------------------------------------------
	bool IsNearDOFAllowed()
	{
		if (!s_bNearDofEffect || !m_pCharacterEntity)
			return false;

		CharacterControllerComponent controller = m_pCharacterEntity.GetCharacterController();

		if (!controller.IsWeaponRaised() || controller.IsGadgetInHands() || controller.GetIsInspectionMode())
			return false;

		if (!controller.IsWeaponADS())
			return true;

		//When ADS'sing and current sights are using PIP, disable nearDOF
		BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(m_pCharacterEntity.FindComponent(BaseWeaponManagerComponent));
		if (weaponManager)
		{
			SCR_2DPIPSightsComponent sights = SCR_2DPIPSightsComponent.Cast(weaponManager.GetCurrentSights());
			if (sights && sights.IsPIPEnabled())
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void FindStaminaValues()
	{
		if (!m_pSignalsManager)
			return;
		
		if (m_iStaminaSignal == -1)
			return;

		float stamina = m_pSignalsManager.GetSignalValue(m_iStaminaSignal);
		s_fBlurriness = (m_wSupression.GetMaskProgress() / STAMINA_BLUR_DIVIDER);

		if (stamina > STAMINA_EFFECT_THRESHOLD && !m_bStaminaEffectActive)
		{
			s_bEnableRadialBlur = true;
			StaminaEffects(true);
		}
		else if (m_bStaminaEffectActive && stamina < STAMINA_EFFECT_THRESHOLD)
		{
			s_bEnableRadialBlur = false;
			GetGame().GetCallqueue().Remove(StaminaEffects);
			GetGame().GetCallqueue().Remove(ClearStaminaEffect);
			ClearStaminaEffect(false);
			m_bStaminaEffectActive = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnDamage(EDamageType type,
				  float damage,
				  HitZone pHitZone,
				  IEntity instigator,
				  inout vector hitTransform[3],
				  float speed,
				  int colliderID,
				  int nodeID)
	{
		m_fGaussBlurReduction = Math.Clamp(++m_fGaussBlurReduction, 0, 2);
	}

	void OnDamageOverTimeAdded(EDamageType dType, float dps, HitZone hz = null)
	{
		m_bIsBleeding = m_pDamageManager.IsDamagedOverTime(EDamageType.BLEEDING);
		if (!m_bBleedingEffect && m_bIsBleeding && m_eLifeState != EDamageState.DESTROYED)
			CreateEffectOverTime(true);

		m_bBleedingEffect = m_bIsBleeding;
	}

	void OnDamageOverTimeRemoved(EDamageType dType, HitZone hz = null)
	{
		m_bIsBleeding = m_pDamageManager.IsDamagedOverTime(EDamageType.BLEEDING);
		if (m_bIsBleeding)
			return;

		ClearEffectOverTime(false);
		GetGame().GetCallqueue().Remove(CreateEffectOverTime);
		GetGame().GetCallqueue().Remove(ClearEffectOverTime);

		m_bBleedingEffect = false;
	}

	//------------------------------------------------------------------------------------------------
	void MomentaryDamageEffect(float timeslice)
	{
		// enable the gaussblur ematerial
		s_bEnableGaussBlur = true;

		// gaussblur is set to max, then drained back to 0
		if (!s_bRemoveGaussBlur)
		{
			s_fGaussBlurriness = 1;
			s_bRemoveGaussBlur = true;
		}
		else
		{
			m_fGaussBlurReduction -= timeslice / MOMENTARY_DAMAGE_BLUR_DURATION;
			s_fGaussBlurriness = Math.Lerp(0, 1, m_fGaussBlurReduction);

			if (s_fGaussBlurriness <= 0)
			{
				s_bRemoveGaussBlur = false;
				s_fGaussBlurriness = 0;
				m_fGaussBlurReduction = 0;
				s_bEnableGaussBlur = false;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void BlackoutEffect(float effectStrength)
	{
		if (!m_wBlackOut)
			return;
		
		m_wBlackOut.SetOpacity(1);

		if (effectStrength > 0)
		{
			effectStrength = effectStrength * BLACKOUT_OPACITY_MULTIPLIER;
			WidgetAnimator.PlayAnimation(m_wBlackOut, WidgetAnimationType.AlphaMask, effectStrength, BLEEDINGEFFECT_PROGRESSION_FADEIN_1_DURATION);
		}
		else
		{
			WidgetAnimator.PlayAnimation(m_wBlackOut, WidgetAnimationType.AlphaMask, 0, BLEEDINGEFFECT_OPACITY_FADEOUT_1_DURATION);
		}
	}

	//------------------------------------------------------------------------------------------------
	void CreateEffectOverTime(bool repeat)
	{
		if (!m_wBloodEffect1 || !m_wBloodEffect2)
			return;
		
		float effectStrength1 = 1;
	
		m_wBloodEffect1.SetSaturation(Math.Clamp(s_fSaturation, 0.3, 1));
		m_wBloodEffect2.SetSaturation(Math.Clamp(s_fSaturation, 0.3, 1));
		
		if (m_iEffectNo == 1)
		{
			WidgetAnimator.PlayAnimation(m_wBloodEffect2, WidgetAnimationType.Opacity, 0, BLEEDINGEFFECT_OPACITY_FADEOUT_2_DURATION);
			WidgetAnimator.PlayAnimation(m_wBloodEffect2, WidgetAnimationType.AlphaMask, 0, BLEEDINGEFFECT_PROGRESSION_FADEOUT_2_DURATION);
			WidgetAnimator.PlayAnimation(m_wBloodEffect1, WidgetAnimationType.Opacity, effectStrength1 , BLEEDINGEFFECT_OPACITY_FADEIN_1_DURATION);
			WidgetAnimator.PlayAnimation(m_wBloodEffect1, WidgetAnimationType.AlphaMask, effectStrength1 * 0.5, BLEEDINGEFFECT_PROGRESSION_FADEIN_1_DURATION);
		}
		else if (m_iEffectNo == 2)
		{
			WidgetAnimator.PlayAnimation(m_wBloodEffect1, WidgetAnimationType.Opacity, 0, BLEEDINGEFFECT_OPACITY_FADEOUT_2_DURATION);
			WidgetAnimator.PlayAnimation(m_wBloodEffect1, WidgetAnimationType.AlphaMask, 0, BLEEDINGEFFECT_PROGRESSION_FADEOUT_2_DURATION);
			WidgetAnimator.PlayAnimation(m_wBloodEffect2, WidgetAnimationType.Opacity, effectStrength1  , BLEEDINGEFFECT_OPACITY_FADEIN_1_DURATION);
			WidgetAnimator.PlayAnimation(m_wBloodEffect2, WidgetAnimationType.AlphaMask, effectStrength1 * 0.5, BLEEDINGEFFECT_PROGRESSION_FADEIN_1_DURATION);
		}

		BlackoutEffect(1);

		// Play heartbeat sound
		if (!m_bPlayerOutsideCharacter)
		{
			SCR_UISoundEntity.SetSignalValueStr("BloodLoss", 1 - m_pBloodHZ.GetHealthScaled());
			SCR_UISoundEntity.SoundEvent("SOUND_INJURED_PLAYERCHARACTER");
		}

		GetGame().GetCallqueue().CallLater(ClearEffectOverTime, 1000, false, repeat, m_iEffectNo);
	}
	

	//------------------------------------------------------------------------------------------------
	void ClearEffectOverTime(bool repeat)
	{
		Widget w;

		if (m_iEffectNo == 1)
		{
			m_iEffectNo = 2;
			w = m_wBloodEffect1;
		}
		else
		{
			m_iEffectNo = 1;
			w = m_wBloodEffect2;
		}

		WidgetAnimator.PlayAnimation(m_wBloodEffect1, WidgetAnimationType.Opacity, 0, BLEEDINGEFFECT_PROGRESSION_FADEOUT_1_DURATION);
		WidgetAnimator.PlayAnimation(m_wBloodEffect1, WidgetAnimationType.AlphaMask, 0, BLEEDINGEFFECT_OPACITY_FADEOUT_1_DURATION);
		WidgetAnimator.PlayAnimation(m_wBloodEffect2, WidgetAnimationType.Opacity, 0, BLEEDINGEFFECT_PROGRESSION_FADEOUT_1_DURATION);
		WidgetAnimator.PlayAnimation(m_wBloodEffect2, WidgetAnimationType.AlphaMask, 0, BLEEDINGEFFECT_OPACITY_FADEOUT_1_DURATION);
		
		BlackoutEffect(0);

		if (repeat && m_bBleedingEffect)
			GetGame().GetCallqueue().CallLater(CreateEffectOverTime, BLEEDING_REPEAT_DELAY, false, repeat);	
	}
	

	//------------------------------------------------------------------------------------------------
	void AddDesaturationEffect()
	{
		if (!m_pBloodHZ)
			return;

		float desaturationStart = m_pBloodHZ.GetDamageStateThreshold(ECharacterBloodState.FAINTING);
		float desaturationEnd = m_pBloodHZ.GetDamageStateThreshold(ECharacterBloodState.UNCONSCIOUS);
		float bloodLevel = Math.InverseLerp(desaturationEnd, desaturationStart, m_pBloodHZ.GetHealthScaled());
		float deathDesat = m_wDeath.GetMaskProgress();

		s_fSaturation = Math.Clamp(bloodLevel * bloodLevel, 0, 1);
		s_fSaturation = Math.Clamp(s_fSaturation - deathDesat, 0, 1);

		s_bEnableSaturation = s_fSaturation < 1;
	}

	//------------------------------------------------------------------------------------------------
	void AddDOFBokehEffect(bool nearDofAllowed)
	{
		if (m_wDeath.GetOpacity() > 0.1)
			s_fFocalLength = s_fFocalLengthNear + (FOCALLENGTH_MAX * (m_wDeath.GetOpacity() - DEATHEFFECT_START_OPACITY) / (DEATHEFFECT_FADEIN_OPACITY_TARGET - DEATHEFFECT_START_OPACITY));
		
		s_fFocusDistance = FOCUSDISTANCE_MULTIPIER;

		//s_fFocalLength cannot be 0, so it is disabled when it is within a 0.1 margin of the lowest permitted value
		if (s_fFocalLength > 0.2 || nearDofAllowed)
		{
			s_bEnableDOFBokeh = true;
			s_fFocalLengthNear = FOCALLENGTHNEAR_INTENSITY;
		}
		else
			s_bEnableDOFBokeh = false;
	}

	void AddDOFEffect(float timeslice, bool nearDofAllowed)
	{
		
		s_fFocalDistance = FOCALDISTANCE_INTENSITY;

		// if s_bNearDofEffect is allowed, simpleDOF always must be enabled. If not, it must be disabled when inactive (i.e. focalchange > max)
		if (s_fFocalChange < SIMPLEDOF_FOCALCHANGE_MAX)
		{
			s_bEnableDOF = true;
			s_bSkipFar = false;
			s_fFocalChangeNear = SIMPLEDOF_FOCALCHANGE_MAX;
		}
		else if (nearDofAllowed)
		{
			s_fFocalChangeNear = STANDARD_FOCALCHANGE_NEAR;
			s_bEnableDOF = true;
			s_bSkipFar = true;
		}
		else
			s_bEnableDOF = false;

		if (m_eLifeState == EDamageState.DESTROYED)
			s_fFocalChange = 100 - 100 * (m_wDeath.GetOpacity() - DEATHEFFECT_START_OPACITY) / (DEATHEFFECT_FADEIN_OPACITY_TARGET - DEATHEFFECT_START_OPACITY);
		else
			s_fFocalChange = SIMPLEDOF_FOCALCHANGE_MAX;
	}

	//------------------------------------------------------------------------------------------------
	void OnDamageStateChanged()
	{
		m_eLifeState = m_pDamageManager.GetState();
		if (m_eLifeState == EDamageState.DESTROYED)
		{
			if (!m_pHeadHitZone.GetDamageState() != EDamageState.DESTROYED)
				DeathEffect();

			GetGame().GetCallqueue().Remove(CreateEffectOverTime);
			GetGame().GetCallqueue().Remove(ClearEffectOverTime);
			ClearEffectOverTime(false);
		}
	} 

	//------------------------------------------------------------------------------------------------
	void DeathEffect()
	{
		if (!m_wDeath || (m_wDeath.GetOpacity() > 0))
			return;

		WidgetAnimator.StopAnimation(m_wDeath);
		m_wDeath.SetMaskProgress(0);

		SCR_UISoundEntity.SoundEvent(SOUND_DEATH_SLOW);

		m_wDeath.SetOpacity(DEATHEFFECT_START_OPACITY);
		WidgetAnimator.PlayAnimation(m_wDeath,WidgetAnimationType.AlphaMask, DEATHEFFECT_FADEIN_PROGRESSION_TARGET, DEATHEFFECT_FADEIN_PROGRESSION_DURACTION);
		WidgetAnimator.PlayAnimation(m_wDeath,WidgetAnimationType.Opacity, DEATHEFFECT_FADEIN_OPACITY_TARGET, DEATHEFFECT_FADEIN_OPACITY_DURATION);
	}

	void InstaDeathEffect()
	{
		if (m_wDeath.GetOpacity() >= 0.99 || m_pHeadHitZone.GetDamageState() != EDamageState.DESTROYED || m_pHealthHitZone.GetDamageState() != EDamageState.DESTROYED)
			return;

		SCR_UISoundEntity.SoundEvent(SOUND_DEATH_FAST);
		
		m_wDeath.SetMaskProgress(INSTADEATHEFFECT_START_PROGRESSION);
		m_wDeath.SetOpacity(INSTADEATHEFFECT_START_OPACITY);
		WidgetAnimator.PlayAnimation(m_wDeath,WidgetAnimationType.AlphaMask, INSTADEATHEFFECT_FADEIN_PROGRESSION_TARGET, INSTADEATHEFFECT_FADEIN_PROGRESSION_DURACTION);
		WidgetAnimator.PlayAnimation(m_wDeath,WidgetAnimationType.Opacity, INSTADEATHEFFECT_FADEIN_OPACITY_TARGET, INSTADEATHEFFECT_FADEIN_OPACITY_DURATION);
	}

	//------------------------------------------------------------------------------------------------
	void StaminaEffects(bool repeat)
	{
		if (!m_wSupression)
			return;

		m_bStaminaEffectActive = true;
		m_wSupression.SetOpacity(STAMINAEFFECT_INITIAL_OPACITY_TARGET);
		WidgetAnimator.PlayAnimation(m_wSupression, WidgetAnimationType.AlphaMask, STAMINAEFFECT_FADEIN_PROGRESSION_TARGET, STAMINAEFFECT_FADEIN_PROGRESSION_DURATION);
		GetGame().GetCallqueue().CallLater(ClearStaminaEffect, STAMINA_CLEAREFFECT_DELAY, false, true);
	}

	void ClearStaminaEffect(bool repeat)
	{
		if (!m_wSupression)
			return;

		WidgetAnimator.StopAnimation(m_wSupression);
		WidgetAnimator.PlayAnimation(m_wSupression, WidgetAnimationType.AlphaMask, 0, 1);
		if (repeat && m_bStaminaEffectActive && m_wSupression && !m_wSupression.GetOpacity() == 0)
			GetGame().GetCallqueue().CallLater(StaminaEffects, STAMINA_CLEAREFFECT_DELAY, false, repeat);
	}

	//------------------------------------------------------------------------------------------------
	void ClearEffects()
	{
		m_bBleedingEffect 					= 0;
		s_fGaussBlurriness					= 0;
		s_fFocalLength 						= 0.1;

		if (m_wBloodEffect1)
		{
			if (WidgetAnimator.IsAnimating(m_wBloodEffect1))
				WidgetAnimator.StopAnimation(m_wBloodEffect1);
			m_wBloodEffect1.SetOpacity(0);
			m_wBloodEffect1.SetMaskProgress(0);
		}

		if (m_wBloodEffect2)
		{
			if (WidgetAnimator.IsAnimating(m_wBloodEffect2))
				WidgetAnimator.StopAnimation(m_wBloodEffect2);
			m_wBloodEffect2.SetOpacity(0);
			m_wBloodEffect2.SetMaskProgress(0);
		}

		if (m_wBlackOut)
		{
			if (WidgetAnimator.IsAnimating(m_wBlackOut))
				WidgetAnimator.StopAnimation(m_wBlackOut);
			m_wBlackOut.SetOpacity(0);
			m_wBlackOut.SetMaskProgress(0);
		}

		if (m_wDeath)
		{
			if (WidgetAnimator.IsAnimating(m_wDeath))
				WidgetAnimator.StopAnimation(m_wDeath);
			m_wDeath.SetOpacity(0);
			m_wDeath.SetMaskProgress(0);
		}

		if (m_wSupression)
		{
			if (WidgetAnimator.IsAnimating(m_wDeath))
				WidgetAnimator.StopAnimation(m_wDeath);
			m_wSupression.SetOpacity(0);
			m_wSupression.SetMaskProgress(0);
		}

		GetGame().GetCallqueue().Remove(CreateEffectOverTime);
		GetGame().GetCallqueue().Remove(ClearEffectOverTime);
	}

	//------------------------------------------------------------------------------------------------
	void ProcessDebug()
	{
#ifdef ENABLE_DIAG
		if (DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_POSTPROCESSING_EFFECTS))
		{
			DbgUI.Text("U to increase postprocesssing effects");
			DbgUI.Text("K to reset postprocessing effects");
			DbgUI.Text("O to decrease postprocesssing effects");

			// Replace the effect in this debug with any other postprocessing effect to test it incrementally			
			if (Debug.KeyState(KeyCode.KC_U))
			{
				Debug.ClearKey(KeyCode.KC_U);
				s_fFocalLength += 100;
				PrintFormat("Focal Length: %1", s_fFocalLength);
			}
			if (Debug.KeyState(KeyCode.KC_K))
			{
				Debug.ClearKey(KeyCode.KC_K);
				s_fFocalLength = FOCALLENGTHNEAR_INTENSITY;
			}
			if (Debug.KeyState(KeyCode.KC_O))
			{
				Debug.ClearKey(KeyCode.KC_O);
				s_fFocalLength -= 100;
				PrintFormat("Focal length: %1", s_fFocalLength);
			}
		}

		if (DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_SCREEN_EFFECTS))
		{
			DbgUI.Text("Health: " + m_pDamageManager.GetHealth());
			DbgUI.Text("Y to raise Head damage effect by 60");
			DbgUI.Text("U to raise Chest damage effect by 60");
			DbgUI.Text("I to raise damage effect by 30");
			DbgUI.Text("O to raise damage effect by 10");
			DbgUI.Text("P to raise damage effect by 1");
			DbgUI.Text("K to reset damage effect");
			DbgUI.Text("T to add damage over time effect");
		
			vector hitPosDirNorm[3];
			
			if (Debug.KeyState(KeyCode.KC_P))
			{
				Debug.ClearKey(KeyCode.KC_P);
				m_pDamageManager.HandleDamage(EDamageType.TRUE, 1, hitPosDirNorm, GetGame().GetPlayerController().GetControlledEntity(), m_pDamageManager.GetDefaultHitZone(), GetGame().GetPlayerController().GetControlledEntity(), null, -1, -1);
			}
			if (Debug.KeyState(KeyCode.KC_O))
			{
				Debug.ClearKey(KeyCode.KC_O);
				m_pDamageManager.HandleDamage(EDamageType.TRUE, 10, hitPosDirNorm, GetGame().GetPlayerController().GetControlledEntity(), m_pDamageManager.GetDefaultHitZone(), GetGame().GetPlayerController().GetControlledEntity(), null, -1, -1);
			}
			if (Debug.KeyState(KeyCode.KC_I))
			{
				Debug.ClearKey(KeyCode.KC_I);
				m_pDamageManager.HandleDamage(EDamageType.TRUE, 30, hitPosDirNorm, GetGame().GetPlayerController().GetControlledEntity(), m_pDamageManager.GetDefaultHitZone(), GetGame().GetPlayerController().GetControlledEntity(), null, -1, -1);
			}
			if (Debug.KeyState(KeyCode.KC_U))
			{
				Debug.ClearKey(KeyCode.KC_U);
				m_pDamageManager.HandleDamage(EDamageType.TRUE, 60, hitPosDirNorm, GetGame().GetPlayerController().GetControlledEntity(), m_pDamageManager.GetDefaultHitZone(), GetGame().GetPlayerController().GetControlledEntity(), null, -1, -1);
			}
			if (Debug.KeyState(KeyCode.KC_Y))
			{
				Debug.ClearKey(KeyCode.KC_Y);
				m_pHeadHitZone.HandleDamage(60, EDamageType.TRUE, null);
			}
			if (Debug.KeyState(KeyCode.KC_K))
			{
				Debug.ClearKey(KeyCode.KC_K);
				m_pDamageManager.SetHealthScaled(1);
			}
			if (Debug.KeyState(KeyCode.KC_T))
			{
				Debug.ClearKey(KeyCode.KC_T);
				m_pDamageManager.AddRandomBleeding();
			}
		}
#endif
	}

	void UnregisterEffects()
	{
		if (!m_pDamageManager)
			return;

		m_pDamageManager.GetOnDamageStateChanged().Remove(OnDamageStateChanged);
		m_pDamageManager.GetOnDamageOverTimeAdded().Remove(OnDamageOverTimeAdded);
		m_pDamageManager.GetOnDamageOverTimeRemoved().Remove(OnDamageOverTimeRemoved);
		m_pDamageManager.GetOnDamage().Remove(OnDamage);

		if (m_pHeadHitZone)
			m_pHeadHitZone.GetOnDamageStateChanged().Remove(InstaDeathEffect);

		m_pHealthHitZone = null;
		m_pHeadHitZone = null;
		m_pCharacterEntity = null;
		m_pDamageManager = null;
	}

	//------------------------------------------------------------------------------------------------
	override event void DisplayStopDraw(IEntity owner)
	{
		ClearEffects();
		UnregisterEffects();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.m_OnControlledEntityChanged.Remove(CharacterChanged);
		GetGame().OnUserSettingsChangedInvoker().Remove(SettingsChanged);
	}
};