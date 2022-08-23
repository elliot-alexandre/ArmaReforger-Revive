// Script File

[ComponentEditorProps(category: "GameScripted/AI", description: "Component for utility AI system calculations", color: "0 0 255 255")]
class SCR_AICombatComponentClass: ScriptComponentClass
{
};

enum EAISkill
{
	NONE,
	ROOKIE	= 20,
	REGULAR	= 50,
	VETERAN	= 70,
	EXPERT 	= 80,
	CYLON  	= 100
};

enum EAICharacterAimZones
{
	NONE,
	TORSO,
	HEAD,
	LIMBS,
};


// be aware it is used for bitmask
enum EAICombatActions
{
	HOLD_FIRE = 1,
	BURST_FIRE = 2,
	SUPPRESSIVE_FIRE = 4,
	MOVEMENT_WHEN_FIRE = 8,
	MOVEMENT_TO_LAST_SEEN = 16,
};

enum EAICombatType
{
	NONE,
	NORMAL,
	SUPPRESSIVE,
	RETREAT
};

//------------------------------------------------------------------------------------------------
class SCR_AICombatComponent : ScriptComponent
{
	private static const int	ENEMIES_SIMPLIFY_THRESHOLD = 12;
	private static const float	RIFLE_BURST_RANGE_SQ = 2500;
	//has to be bigger than m_fUpdateStep in perception unless it will lose target
	private static const float	LAST_SEEN_TIMEOUT = 0.6;
	
	ref array<BaseTarget> m_Enemies = {};
	//current enemy
	private BaseTarget	m_EnemyTarget;
	private bool		m_bTargetChanged;
	private float		m_fDistanceToEnemySq = float.MAX;
	
	private SCR_InventoryStorageManagerComponent	m_InventoryManager;
	private EventHandlerManagerComponent			m_EventHandlerManagerComponent;
	private BaseWeaponManagerComponent				m_WpnManager;
	private BaseWeaponComponent						m_CurrentWeapon;
	private SCR_CompartmentAccessComponent			m_CompartmentAccess;
	private PerceptionComponent m_Perception;
	
	private EAICombatActions	m_iAllowedActions; //will be initialized with default combat type
	private EAICombatType		m_eCombatType = EAICombatType.NORMAL;
	private EAICombatType		m_eDefaultCombatType = EAICombatType.NORMAL;

	
	[Attribute("50", UIWidgets.ComboBox, "AI skill in combat", "", ParamEnumArray.FromEnum(EAISkill) )]
	private EAISkill m_eAISkillDefault;
	private EAISkill m_eAISkill;
	
	private float	m_fLastEndangerCheckTime;
	private float	m_fRandomizedEndangerDelay = Math.RandomFloat(80,150);
	private BaseTarget	m_EndangeringEnemy;
	
	// Belongs to friendly in aim check
	protected static const float FRIENDLY_AIM_MIN_UPDATE_INTERVAL_MS = 300.0;
	protected static const float FRIENDLY_AIM_SAFE_DISTANCE =  0.8;
	// If IsFriendlyInAim is called more often than once per FRIENDLY_AIM_MIN_UPDATE_INTERVAL_MS, it returns a cached value.
	// Min update interval is fixed, but first update time is randomized to desynchronize the load from many AIs calling that.
	protected float m_fFriendlyAimNextCheckTime_ms;
	protected bool m_bFriendlyAimLastResult;
	protected ref array<BaseTarget> m_FriendlyTargets = {};
	#ifdef WORKBENCH
	protected ref Shape m_FriendlyAimShape;
	#endif
	
	//------------------------------------------------------------------------------------------------
	EAISkill GetAISkill()
	{
		return m_eAISkill;
	}
	
	//------------------------------------------------------------------------------------------------
	// use this to change AIskill dynamically
	void SetAISkill(EAISkill skill)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetAISkill: %1", typename.EnumToString(EAISkill, skill)));
		#endif
		
		m_eAISkill = skill;
	}
	
	//------------------------------------------------------------------------------------------------
	// use this to reset AI skill to default
	void ResetAISkill()
	{
		#ifdef AI_DEBUG
		AddDebugMessage("ResetAISkill");
		#endif
		
		m_eAISkill = m_eAISkillDefault;
	}
	
	//------------------------------------------------------------------------------------------------
	EWeaponType GetCurrentWeaponType()
	{
		if (m_CurrentWeapon)
			return m_CurrentWeapon.GetWeaponType();
		else return EWeaponType.WT_NONE;
	}
	
	//------------------------------------------------------------------------------------------------
	BaseTarget GetCurrentEnemy()
	{
		return m_EnemyTarget;
	}
	
	//------------------------------------------------------------------------------------------------
	BaseTarget GetLastSeenEnemy()
	{
		BaseTarget lastSeenEnemy = null;
		float lastSeen = float.MAX;
		
		foreach (BaseTarget t : m_Enemies)
		{
			if (t)
			{
				float currentLastSeen = t.GetTimeSinceSeen();
				if (currentLastSeen < lastSeen)
				{
					if (currentLastSeen == 0)
						return t;
					
					lastSeen = currentLastSeen;
					lastSeenEnemy = t;
				}
			}
		}
		
		return lastSeenEnemy;
	}	
	
	// Finds out if some of my known enemies is aimng at me. For performance reasons it is reavaluated once ~115ms
	// Also when there is too many enemies we suspect it is difficult to orient and exctract meaningfull information from this.
	// So mostly for performance reason we evaluate just if our current target is aimng at us.
	//------------------------------------------------------------------------------------------------
	BaseTarget GetEndangeringEnemy()
	{
		if (m_fLastEndangerCheckTime + m_fRandomizedEndangerDelay > GetOwner().GetWorld().GetWorldTime())
		{
			return m_EndangeringEnemy;
		}
		m_fLastEndangerCheckTime = GetOwner().GetWorld().GetWorldTime();
		
		m_EndangeringEnemy = null;
		
		if (m_Enemies.Count() > ENEMIES_SIMPLIFY_THRESHOLD)
		{
			if (IsEnemyDanger(m_EnemyTarget))
				m_EndangeringEnemy = m_EnemyTarget;
			
			return m_EndangeringEnemy;
		}
		
		foreach (BaseTarget t : m_Enemies)
		{
			if (IsEnemyDanger(t))
			{
				m_EndangeringEnemy = t;
			}
		}
		return m_EndangeringEnemy;
	}

	//------------------------------------------------------------------------------------------------	
	private bool IsEnemyDanger(BaseTarget enemyBase)
	{	
		if (!enemyBase)
			return false;
		
		//Enemy can't be a danger if I can't see him
		if (enemyBase.GetTimeSinceSeen() > LAST_SEEN_TIMEOUT)
			return false;
		
		IEntity enemy = enemyBase.GetTargetEntity();
		if (!enemy)
			return false;
		
		SCR_CharacterControllerComponent enemyCharacterComponent = SCR_CharacterControllerComponent.Cast(enemy.FindComponent(SCR_CharacterControllerComponent));
		if (!enemyCharacterComponent || !enemyCharacterComponent.IsWeaponRaised())
			return false;

		BaseWeaponManagerComponent enemyWeaponMan = BaseWeaponManagerComponent.Cast(enemy.FindComponent(BaseWeaponManagerComponent));
		if (!enemyWeaponMan)
			return false;
		
		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(enemyBase.GetTargetEntity().FindComponent(SCR_CharacterDamageManagerComponent));
	
		if(dmgManager.isRevivable == true)
			return false;
		
		vector muzzleMatrix[4];
		enemyWeaponMan.GetCurrentMuzzleTransform(muzzleMatrix);
		// muzzleMatrix[3] - position vector on tip of the muzzle
		// muzzleMatrix[2] - vector where weapon is pointing at
		return SCR_AIIsCharacterInCone(ChimeraCharacter.Cast(GetOwner()), muzzleMatrix[3], muzzleMatrix[2]);
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsEnemyKnown(IEntity ent)
	{
		foreach (BaseTarget t : m_Enemies)
		{
			if (t)
			{
				if (t.GetTargetEntity() == ent)
					return true;
			}
		}
			
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsTargetChanged()
	{
		return m_bTargetChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	//right now based just on distance, and 0 is the highest
	float EvaluateTargetDistance(BaseTarget enemy)
	{
		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(enemy.GetTargetEntity().FindComponent(SCR_CharacterDamageManagerComponent));
	
		
		if (!enemy || dmgManager.isRevivable == true)
		{
			#ifdef AI_DEBUG
			AddDebugMessage("    null");
			#endif
			return float.MAX;
		}
		
		if (enemy.GetTimeSinceSeen() > LAST_SEEN_TIMEOUT)
		{
			#ifdef AI_DEBUG
			AddDebugMessage(string.Format("    %1: TimeSinceSeen: %2 > %3 threshold", enemy, enemy.GetTimeSinceSeen(), LAST_SEEN_TIMEOUT));
			#endif
			return float.MAX;
		}
			
		float distance = vector.DistanceSq(GetOwner().GetOrigin(), enemy.GetLastSeenPosition());
		
		//TODO decide if check for friendlyFire, frendlifire can be handled by step aside in attack evnetualy
		
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("    %1: TimeSinceSeen: %2, distance: %3", enemy, enemy.GetTimeSinceSeen(), distance));
		#endif
		
		return distance;
	}
	
	//------------------------------------------------------------------------------------------------
	void EvaluateCurrentTarget()
	{		
		m_bTargetChanged = false;
		
		if (m_Enemies.Count() == 0)
		{
			#ifdef AI_DEBUG
			AddDebugMessage("m_Enemies is empty, m_EnemyTarget: null");
			#endif
			
			m_EnemyTarget = null;
			return;
		}
		
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("Evaluating enemies: %1", m_Enemies.Count()));
		#endif
		
		BaseTarget priorityTarget = null;
		//zero is the most prio target
		float targetDistanceSq = float.MAX;
		for(int i = m_Enemies.Count() - 1; i >= 0; i--)
		{
			if (!m_Enemies[i] || m_Enemies[i].GetTargetCategory() != ETargetCategory.ENEMY || SCR_CharacterDamageManagerComponent.Cast(m_Enemies[i].GetTargetEntity().FindComponent(SCR_CharacterDamageManagerComponent)).isRevivable == true )
			{
				if(m_EnemyTarget == m_Enemies[i])
					m_EnemyTarget = null;
				if(priorityTarget == m_Enemies[i])
					priorityTarget = null;
				m_Enemies.Remove(i);
				ResetCombatType();
				continue;
			}
			float newDistance = EvaluateTargetDistance(m_Enemies[i]);
			if(targetDistanceSq > newDistance)
			{
				targetDistanceSq = newDistance;
				priorityTarget = m_Enemies[i];
			}
		}
		if(m_Enemies.Count() == 0){
			ResetCombatType();
			SetHoldFire(true);
			GetGame().GetCallqueue().CallLater(SetHoldFire, 15000, false, false);
		} else {
			SetHoldFire(false);
		}
		
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("Priority Target: %1", priorityTarget));
		#endif
		
		//we don't want to use burst from rifles on long ranges
		if (GetCurrentWeaponType() == EWeaponType.WT_RIFLE)
		{
			if (m_fDistanceToEnemySq > RIFLE_BURST_RANGE_SQ && targetDistanceSq < RIFLE_BURST_RANGE_SQ)
			{	
				SetActionAllowed(EAICombatActions.BURST_FIRE, UsingWeaponWithBurstMode());			
			}
			else if (m_fDistanceToEnemySq < RIFLE_BURST_RANGE_SQ && targetDistanceSq > RIFLE_BURST_RANGE_SQ)
			{
				SetActionAllowed(EAICombatActions.BURST_FIRE, false);			
			}
		}
		m_fDistanceToEnemySq = targetDistanceSq;
		
		if (priorityTarget != m_EnemyTarget)
		{
			#ifdef AI_DEBUG
			AddDebugMessage(string.Format("Target has changed. New: %1, Previous: %2", priorityTarget, m_EnemyTarget));
			#endif
			
			m_bTargetChanged = true;
			m_EnemyTarget = priorityTarget;
			if(GetCombatType() == EAICombatType.SUPPRESSIVE) // suppressive regime is cleared when target is changed
			 	ResetCombatType();	
		}
	}

	//------------------------------------------------------------------------------------------------	
	EWeaponType GetPreferedWeapon()
	{
		if (!m_WpnManager)
				return 0;
		
		array<WeaponSlotComponent> weapons = new array<WeaponSlotComponent>();
		m_WpnManager.GetWeaponsSlots(weapons);
		//used just for WT_SNIPERRIFLE, WT_RIFLE and WT_HANDGUN
		array<WeaponSlotComponent> weaponsPriorityArray = {null, null, null};

		bool inVehicle = false;
		ChimeraCharacter character;
		if (GetCurrentEnemy())
			character = ChimeraCharacter.Cast(GetCurrentEnemy().GetTargetEntity());
		if (character && character.IsInVehicle())
			inVehicle = true;
		
		// finds out what types of weapons we have
		foreach (WeaponSlotComponent weapon : weapons)
		{
			BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
			if (!muzzle)
				continue;
			if (muzzle.GetAmmoCount() <= 0)
			{
				if (!m_InventoryManager)
					continue;

				if (m_InventoryManager.GetMagazineCountByWeapon(weapon) <= 0)
					continue;
			}
			switch(weapon.GetWeaponType())
			{
				//top prio weapon
				case EWeaponType.WT_MACHINEGUN :
				{
					//has priority, we found it so we know we would return it anyway
					return EWeaponType.WT_MACHINEGUN;
				}
				case EWeaponType.WT_SNIPERRIFLE :
				{
					weaponsPriorityArray[0] = weapon;
					break;
				}
				case EWeaponType.WT_RIFLE :
				{
					weaponsPriorityArray[1] = weapon;
					break;
				}
				case EWeaponType.WT_HANDGUN :
				{
					weaponsPriorityArray[2] = weapon;
					break;
				}
				//if we can use rocket launcher we do so.
				case EWeaponType.WT_ROCKETLAUNCHER :
				{
					if (inVehicle)
						return EWeaponType.WT_ROCKETLAUNCHER;
					if (!GetCurrentEnemy())
						break;
					if (Vehicle.Cast(GetCurrentEnemy().GetTargetEntity()))
						return EWeaponType.WT_ROCKETLAUNCHER;
					break;
				}
			}
		}
		
		//select the firs with most priority
		foreach (WeaponSlotComponent weapon : weaponsPriorityArray)
		{
			if (weapon)
			{
				return weapon.GetWeaponType();
			}
		}
		//no weapon found
		return EWeaponType.WT_NONE;
	}
	
	protected static const float DISTANCE_MAX = 22; 
	protected static const float DISTANCE_MIN = 6; // Minimal distance when movement is allowed
	private static const float NEAR_PROXIMITY = 2;
	
	protected const float m_StopDistance = 30 + Math.RandomFloat(0, 10); 
	// TODO: add possibility to get cover towards custom position
	//------------------------------------------------------------------------------------------------
	vector FindNextCoverPosition()
	{
		if (!m_EnemyTarget)
			return vector.Zero;
		
		vector ownerPos = GetOwner().GetOrigin();
		vector lastSeenPos = m_EnemyTarget.GetLastSeenPosition();
		float distanceToTarget = vector.Distance(ownerPos, lastSeenPos);

		if (m_StopDistance > distanceToTarget)
			return vector.Zero;
		
		// Create randomized position
		SCR_ChimeraAIAgent agent = GetAiAgent();
		SCR_DefendWaypoint defendWp = SCR_DefendWaypoint.Cast(agent.m_GroupWaypoint);
		vector direction;
		bool standardAttack = true;
		float nextCoverDistance;
		
		// If target is outside defend waypoint, run towards center of it
		if (defendWp)
		{
			if (!defendWp.IsWithinCompletionRadius(lastSeenPos) &&
				!defendWp.IsWithinCompletionRadius(ownerPos))
			{
				direction = vector.Direction(ownerPos, defendWp.GetOrigin());	// Direction towards center of defend wp
				
				if (vector.Distance(defendWp.GetOrigin(), ownerPos) < DISTANCE_MIN)
					nextCoverDistance = 0;
				else	
					nextCoverDistance = DISTANCE_MIN;
				
				standardAttack = false;
			}
		}
		
		if (standardAttack)
		{
			nextCoverDistance = Math.RandomFloat(DISTANCE_MIN, DISTANCE_MAX);

			// If close enough, get directly to the target
			if (nextCoverDistance > (distanceToTarget - DISTANCE_MIN))
				nextCoverDistance = distanceToTarget - DISTANCE_MIN;
			
			direction = vector.Direction(ownerPos, m_EnemyTarget.GetLastSeenPosition());
		}
			
		direction.Normalize();
		vector newPositionCenter = direction * nextCoverDistance + ownerPos, newPosition;
		// yes possibly it could lead to end up in target position but lets ignore it for now
		
		newPosition = s_AIRandomGenerator.GenerateRandomPointInRadius(0, NEAR_PROXIMITY, newPositionCenter, true);
		newPosition[1] = newPositionCenter[1];
		return newPosition;
	}
	//------------------------------------------------------------------------------------------------
	// Returns aiming offset of selected hitzone on current enemy 
	vector GetAimingOffsetByHitzone(EAICharacterAimZones hitZone)
	{
		// TODO: get this from some Hitzone manager?!
		return vector.Zero;
	}

	//------------------------------------------------------------------------------------------------
	bool UsingWeaponWithBurstMode()
	{
		if (!m_CurrentWeapon)
			return false;
		
		BaseMuzzleComponent muzzle = m_CurrentWeapon.GetCurrentMuzzle();
		if (!muzzle)
			return false;
		
		array<BaseFireMode> fireModes = new array<BaseFireMode>();
		int count = muzzle.GetFireModesList(fireModes);
		foreach (BaseFireMode mode : fireModes)
		{
			EWeaponFiremodeType modeType = mode.GetFiremodeType();
			if (modeType == EWeaponFiremodeType.Burst || modeType == EWeaponFiremodeType.Auto)
			{
				return true;
			}
		}
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	void Event_OnWeaponChanged(BaseWeaponComponent weapon, BaseWeaponComponent prevWeapon)
	{
		m_CurrentWeapon = weapon;
		
		if (!m_CurrentWeapon)
			return;

		if (m_CurrentWeapon.GetWeaponType() == EWeaponType.WT_RIFLE && m_fDistanceToEnemySq > RIFLE_BURST_RANGE_SQ)
		{
			SetActionAllowed(EAICombatActions.BURST_FIRE, false);
			return;
		}
		
		SetActionAllowed(EAICombatActions.BURST_FIRE, UsingWeaponWithBurstMode());			
	}
	
	//------------------------------------------------------------------------------------------------
	// Checks if unit is allowed to do action from the allowed actions enum
	bool IsActionAllowed(EAICombatActions action)
	{
		return (m_iAllowedActions & action);
	}
	
	//------------------------------------------------------------------------------------------------	
	private void SetActionAllowed(EAICombatActions action, bool isAllowed)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetActionAllowed: %1, %2", typename.EnumToString(EAICombatActions, action), isAllowed));
		#endif
		
		if ((m_iAllowedActions & action) != isAllowed)
		{
			if (isAllowed)
				m_iAllowedActions = m_iAllowedActions | action;
			else
				m_iAllowedActions = m_iAllowedActions & ~action;
		}
	}
	
	//------------------------------------------------------------------------------------------------	
	void SetHoldFire(bool isHoldFire)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetHoldFire: %1", isHoldFire));
		#endif
		
		if (isHoldFire)
		{ 
			SetActionAllowed(EAICombatActions.HOLD_FIRE,true);
			SetActionAllowed(EAICombatActions.BURST_FIRE,false);
			SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
		}
		else
		{
			SetCombatType(m_eCombatType);
		}	
	}
	
	//------------------------------------------------------------------------------------------------
	EAICombatType GetCombatType()
	{
		return m_eCombatType;
	}
	// For current AI combat type can be changed during the behaviors
	// If you want AI to come bac to your state you have to set also default
	//------------------------------------------------------------------------------------------------
	void SetCombatType(EAICombatType combatType)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetCombatType: %1", typename.EnumToString(EAICombatType, combatType)));
		#endif
		
		switch (combatType)
		{
			case EAICombatType.NONE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
			case EAICombatType.NORMAL:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.SUPPRESSIVE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.RETREAT:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
		}
		m_eCombatType = combatType;
#ifdef WORKBENCH
		SCR_AIDebugVisualization.VisualizeMessage(GetOwner(), typename.EnumToString(EAICombatType,m_eCombatType), EAIDebugCategory.COMBAT, 5);
#endif
	}
	
	// When AI use reset e.g. after bounding overwatch it is reseted to default combat type
	// You have to also call SetCombatType to have immediate effect.
	//------------------------------------------------------------------------------------------------
	void SetDefaultCombatType(EAICombatType combatType)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetDefaultCombatType: %1", typename.EnumToString(EAICombatType, combatType)));
		#endif
		
		m_eDefaultCombatType = combatType;
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetCombatType()
	{
		#ifdef AI_DEBUG
		AddDebugMessage("ResetCombatType");
		#endif
		
		SetCombatType(m_eDefaultCombatType);
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsFriendlyInAim()
	{
		float timeCurrent = GetGame().GetWorld().GetWorldTime();
		
		if (timeCurrent < m_fFriendlyAimNextCheckTime_ms)
		{
			//Print(string.Format("%1 Return cached %2", this, timeCurrent));
			return m_bFriendlyAimLastResult;
		}
				
		if (!m_WpnManager || !m_Perception)
			return false;
		
		//Print(string.Format("%1 Update %2", this, timeCurrent));
		
		m_Perception.GetTargetsList(m_FriendlyTargets, ETargetCategory.FRIENDLY);
		IEntity friendlyEntInAim = GetFriendlyFireEntity(m_FriendlyTargets);

#ifdef WORKBENCH
		if (friendlyEntInAim && DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_AI_SHOW_FRIENDLY_IN_AIM))
			m_FriendlyAimShape = Shape.CreateSphere(COLOR_RED, ShapeFlags.NOOUTLINE|ShapeFlags.NOZBUFFER|ShapeFlags.TRANSP, friendlyEntInAim.GetOrigin() + Vector(0, 2, 0), 0.1);	
		else 
			m_FriendlyAimShape = null;
#endif		
		m_bFriendlyAimLastResult = friendlyEntInAim != null;
		
		m_fFriendlyAimNextCheckTime_ms = timeCurrent + FRIENDLY_AIM_MIN_UPDATE_INTERVAL_MS;
		
		return m_bFriendlyAimLastResult;
	}
	
	//------------------------------------------------------------------------------------------------
	protected IEntity GetFriendlyFireEntity(notnull array<BaseTarget> friendlies)
	{
		if (friendlies.IsEmpty())
			return null;
		
		IEntity myVehicleEnt = m_CompartmentAccess.GetVehicle();
		IEntity friendlyEntity;
		
		vector muzzleMatrix[4];
		m_WpnManager.GetCurrentMuzzleTransform(muzzleMatrix);
		// muzzleMatrix[3] - position vector on tip of the muzzle
		// muzzleMatrix[2] - vector where weapon is pointing at	
		
		foreach (BaseTarget friendly : friendlies)
		{
			if (!friendly)
				continue;
			
			friendlyEntity = friendly.GetTargetEntity();
				
			if (!friendlyEntity || friendlyEntity == myVehicleEnt)
				continue;
			
			ChimeraCharacter friendlyCharacterEnt = ChimeraCharacter.Cast(friendlyEntity);
			if (!friendlyCharacterEnt)
				continue;
			
			if (SCR_AIIsCharacterInCone(friendlyCharacterEnt, muzzleMatrix[3], muzzleMatrix[2], FRIENDLY_AIM_SAFE_DISTANCE))
			{
				return friendlyEntity;
			}					
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		m_EventHandlerManagerComponent = EventHandlerManagerComponent.Cast(owner.FindComponent(EventHandlerManagerComponent));
		if (m_EventHandlerManagerComponent)
			m_EventHandlerManagerComponent.RegisterScriptHandler("OnWeaponChanged", this, this.Event_OnWeaponChanged, true);
		m_eAISkill = m_eAISkillDefault;
		
		m_WpnManager = BaseWeaponManagerComponent.Cast(owner.FindComponent(BaseWeaponManagerComponent));
		m_InventoryManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(owner.FindComponent(SCR_CompartmentAccessComponent));
		m_Perception = PerceptionComponent.Cast(owner.FindComponent(PerceptionComponent));
		
		auto world = GetGame().GetWorld();
		if (world)
			m_fFriendlyAimNextCheckTime_ms = world.GetWorldTime() + Math.RandomFloat(200,400);
		
		SetCombatType(m_eDefaultCombatType);
	}
	
	//------------------------------------------------------------------------------------------------
	void ~SCR_AICombatComponent()
	{
		if ( m_Enemies )
			m_Enemies.Clear();
		if (m_EventHandlerManagerComponent)
			m_EventHandlerManagerComponent.RemoveScriptHandler("OnWeaponChanged", this, this.Event_OnWeaponChanged, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Used by AIDebugInfoComponent
	void DebugPrintToWidget(TextWidget w)
	{
		string str = string.Format("\n%1", m_EnemyTarget.ToString());
		if (m_EnemyTarget)
			str = str + string.Format("\n%1", m_EnemyTarget.GetTimeSinceSeen());
		else
			str = str + "\n";
		w.SetText(str);
	}
	
	SCR_ChimeraAIAgent GetAiAgent()
	{
		AIControlComponent controlComp = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		return SCR_ChimeraAIAgent.Cast(controlComp.GetAIAgent());
	}
	
	#ifdef AI_DEBUG
	//--------------------------------------------------------------------------------------------
	protected void AddDebugMessage(string str)
	{
		AIControlComponent controlComp = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		AIAgent agent = controlComp.GetAIAgent();
		if (!agent)
			return;
		SCR_AIInfoBaseComponent infoComp = SCR_AIInfoBaseComponent.Cast(agent.FindComponent(SCR_AIInfoBaseComponent));
		infoComp.AddDebugMessage(str, msgType: EAIDebugMsgType.COMBAT);
	}
	#endif
};