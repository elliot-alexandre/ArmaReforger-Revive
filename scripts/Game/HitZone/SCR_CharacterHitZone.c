enum ECharacterDamageState: EDamageState
{
	WOUNDED = 3
};

enum ECharacterHealthState: EDamageState
{
	MODERATE = 3,
	SERIOUS = 4,
	CRITICAL = 5
};

enum ECharacterBloodState: EDamageState
{
	WEAKENED = 3,
	FAINTING = 4,
	UNCONSCIOUS = 5
};

//------------------------------------------------------------------------------------------------
class SCR_CharacterHitZone : ScriptedHitZone
{
	protected static const float DAMAGE_TO_BLOOD_MULTIPLIER = 1.3; //Const value is based on previous testing by caliber
	
	[Attribute("1", UIWidgets.Auto, "Multiplier of hitzone health that will be applied as bleeding damage over time when damaged")]
	private float m_fBleedingRateScale;
	
	[Attribute()]
	ref array<string> m_aDamageSubmeshes;
	
	[Attribute(ELoadoutArea.ELA_None.ToString(), UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(ELoadoutArea))]
	ref array<ELoadoutArea> m_aBleedingAreas;
	
	protected bool m_bIsWounded;
	
	//-----------------------------------------------------------------------------------------------------------
	/*!
	Called after damage multipliers and thresholds are applied to received impacts and damage is applied to hitzone.
	This is also called when transmitting the damage to parent hitzones!
	\param type Type of damage
	\param damage Amount of damage received
	\param pOriginalHitzone Original hitzone that got dealt damage, as this might be transmitted damage.
	\param instigator Damage source parent entity (soldier, vehicle, ...)
	\param hitTransform [hitPosition, hitDirection, hitNormal]
	\param speed Projectile speed in time of impact
	\param colliderID ID of the collider receiving damage
	\param nodeID ID of the node of the collider receiving damage
	*/
	override void OnDamage(EDamageType type, float damage, HitZone pOriginalHitzone, IEntity instigator, inout vector hitTransform[3], float speed, int colliderID, int nodeID)
	{
		super.OnDamage(type, damage, pOriginalHitzone, instigator, hitTransform, speed, colliderID, nodeID);
		
		if (this != pOriginalHitzone)
			return;

 		// Only serious hits should cause bleeding
		if (damage < GetCriticalDamageThreshold()*GetMaxHealth())
			return;
		
		// Adding immediately some blood to the clothes - currently it's based on the damage dealt.
		AddBloodToClothes(Math.Clamp(damage * DAMAGE_TO_BLOOD_MULTIPLIER, 0, 255));
		AddBleeding(colliderID);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Whether hitzone submeshes are hidden with clothing
	bool IsCovered()
	{
		CharacterIdentityComponent identity = CharacterIdentityComponent.Cast(GetOwner().FindComponent(CharacterIdentityComponent));
		if (!identity)
			return false;
		
		foreach (string submesh: m_aDamageSubmeshes)
		{
			if (identity.IsCovered(submesh))
				return true;
		}
		
		return false;
	}
	
	//-----------------------------------------------------------------------------------------------------------
	/*! Add bleeding to this hitzone provided with collider ID
	This should be called from RPC from server on all clients and the host
	\param colliderID ID of the collider to attach particle effects to Default: -1 (random)
	\param nodeID ID of the animation node to attach particle effects to. Default: 0 (automatic)
	*/
	void AddBleeding(int colliderID = -1)
	{
		if (!HasColliderNodes())
			return;
		
		int colliderDescriptorIndex = GetColliderDescriptorIndex(colliderID);
		if (colliderDescriptorIndex < 0)
		{
			int currentHealth = GetHealth();
			colliderDescriptorIndex = currentHealth % GetNumColliderDescriptors();
		}
		
		AddBleedingToCollider(colliderDescriptorIndex);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	/*! Add bleeding to this hitzone provided with collider descriptor index
	This should be called from RPC from server on all clients and the host
	\param colliderDescriptorIndex Index off the colldier descriptor in this hitzone
	*/
	void AddBleedingToCollider(int colliderDescriptorIndex = -1)
	{
		if (!HasColliderNodes())
			return;
		
		if (GetDamageOverTime(EDamageType.BLEEDING) > 0)
			return;
		
		// Currently we only have one bleeding per hitzone
		SCR_CharacterDamageManagerComponent manager = SCR_CharacterDamageManagerComponent.Cast(GetHitZoneContainer());
		if (manager)
			manager.AddBleedingHitZone(this, colliderDescriptorIndex);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	override void OnDamageStateChanged()
	{
		super.OnDamageStateChanged();
		
		UpdateSubmeshes();
	}
	
	//-----------------------------------------------------------------------------------------------------------
	void AddBloodToClothes(float immediateBloodEffect = 0)
	{
		BaseLoadoutManagerComponent loadoutManager = BaseLoadoutManagerComponent.Cast(GetOwner().FindComponent(BaseLoadoutManagerComponent));
		if (!loadoutManager)
			return;
		
		IEntity clothEntity;
		ParametricMaterialInstanceComponent materialComponent;
		for (int i = m_aBleedingAreas.Count() - 1; i >= 0; i--)
		{
			clothEntity = loadoutManager.GetClothByArea(m_aBleedingAreas[i]);
			if (!clothEntity)
				continue;
			
			materialComponent = ParametricMaterialInstanceComponent.Cast(clothEntity.FindComponent(ParametricMaterialInstanceComponent));
			if (!materialComponent)
				continue;
			
			if (immediateBloodEffect > 0)
			{
				materialComponent.SetUserParam2(Math.Clamp(materialComponent.GetUserParam2() + immediateBloodEffect, 1, 255));
				continue;
			}
			
			materialComponent.SetUserParam2(Math.Clamp(materialComponent.GetUserParam2() + GetMaxBleedingRate() * 0.1, 1, 255));
		}
	}
	
	//-----------------------------------------------------------------------------------------------------------
	void SetWoundedSubmesh(bool wounded)
	{
		m_bIsWounded = wounded;
		
		CharacterIdentityComponent identity = CharacterIdentityComponent.Cast(GetOwner().FindComponent(CharacterIdentityComponent));
		if (!identity)
			return;
		
		foreach (string submesh: m_aDamageSubmeshes)
			identity.SetWoundState(submesh, wounded);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Manage wounds submeshes
	void UpdateSubmeshes()
	{
		bool isWounded = GetDamageStateThreshold(GetDamageState()) <= GetDamageStateThreshold(ECharacterDamageState.WOUNDED);
		if (m_bIsWounded != isWounded)
			SetWoundedSubmesh(isWounded);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	/*! Maximum bleeding rate that this hitzone may apply to Blood HitZone
	\return bleedingRate Maximum bleeding rate that this hitzone may apply to Blood HitZone
	*/
	float GetMaxBleedingRate()
	{
		float bleedingRate = m_fBleedingRateScale * GetMaxHealth();
		
		SCR_CharacterDamageManagerComponent manager = SCR_CharacterDamageManagerComponent.Cast(GetHitZoneContainer());
		if (manager)
			bleedingRate *=  manager.GetBleedingRateScale();
		
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			bleedingRate *= gameMode.GetBleedingRate();
		
		return bleedingRate;
	}
};

//-----------------------------------------------------------------------------------------------------------
class SCR_RegeneratingHitZone : ScriptedHitZone
{
	[Attribute("10", UIWidgets.Auto, "Time without receiving damage or bleeding to start regeneration [s]")]
	private float m_fRegenerationDelay;
	[Attribute("200", UIWidgets.Auto, "Time to fully regenerate resilience hitzone [s]")]
	private float m_fFullRegenerationTime;

	private float m_fHealthPrevious;

	//-----------------------------------------------------------------------------------------------------------
	/*!
	Called when damage multipliers and thresholds are applied to received impacts and damage is applied to hitzone. This is also called when 
	transmitting the damage to parent hitzones!
	\param damage Amount of damage received
	\param type Type of damage
	\param pHitEntity Entity that was damaged
	\param pOriginalHitzone Original hitzone that got dealt damage, as this might be transmitted damage.
	\param instigator Damage instigator
	\param hitTransform [hitPosition, hitDirection, hitNormal]
	\param colliderID ID of the collider receiving damage
	\param speed Projectile speed in time of impact
	*/
	override void OnDamage(EDamageType type, float damage, HitZone pOriginalHitzone, IEntity instigator, inout vector hitTransform[3], float speed, int colliderID, int nodeID)
	{
		super.OnDamage(type, damage, pOriginalHitzone, instigator, hitTransform, speed, colliderID, nodeID);
		
		if (IsProxy())
			return;
		
		RemoveRegeneration();
		ScheduleRegeneration();
	}
	
	//-----------------------------------------------------------------------------------------------------------
	override void OnHealthSet()
	{
		super.OnHealthSet();
		
		if (IsProxy())
			return;
		
		RemoveRegeneration();
		ScheduleRegeneration();
	}
	
	// Schedule healing over time, delay current scheduled regeneration
	void ScheduleRegeneration()
	{
		GetGame().GetCallqueue().Remove(UpdateRegenerationOverTime);
		GetGame().GetCallqueue().CallLater(UpdateRegenerationOverTime, 1000 * m_fRegenerationDelay);
	}
	
	// Remove current healing over time and any scheduled regeneration
	void RemoveRegeneration()
	{
		GetGame().GetCallqueue().Remove(UpdateRegenerationOverTime);
		if (GetDamageOverTime(EDamageType.REGENERATION))
			SetDamageOverTime(EDamageType.REGENERATION, 0);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	protected void UpdateRegenerationOverTime()
	{
		if (IsProxy())
			return;
		
		if (GetDamageState() == ECharacterDamageState.UNDAMAGED)
			return;
		
		array<EDamageType> damageTypes = {};
		SCR_Enum.GetEnumValues(EDamageType, damageTypes);
		foreach (EDamageType damageType: damageTypes)
		{
			if (damageType == EDamageType.REGENERATION)
				continue;
			
			if (GetDamageOverTime(damageType) != 0)
				return;
		}
		
		float regenerationSpeed = 0;
		if (m_fFullRegenerationTime > 0)
			regenerationSpeed = -GetMaxHealth() / m_fFullRegenerationTime;
		
		SetDamageOverTime(EDamageType.REGENERATION, regenerationSpeed);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	override void OnDamageStateChanged()
	{
		super.OnDamageStateChanged();
		
		if (GetDamageState() == ECharacterDamageState.UNDAMAGED)
			RemoveRegeneration();
	}
};

//-----------------------------------------------------------------------------------------------------------
//! Resilience - incapacitation or death, depending on game mode settings
class SCR_CharacterResilienceHitZone : SCR_RegeneratingHitZone
{
	//-----------------------------------------------------------------------------------------------------------
	override void OnDamageStateChanged()
	{
		super.OnDamageStateChanged();
		
		if (GetDamageState() != EDamageState.DESTROYED)
			return;
		
		// TODO: Set character to unconscious if game mode allows so
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(GetHitZoneContainer());
		if (damageManager)
			damageManager.Kill();
	}
	
	//-----------------------------------------------------------------------------------------------------------
	override void OnInit(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.OnInit(pOwnerEntity, pManagerComponent);
		SCR_CharacterDamageManagerComponent characterDamageManager = SCR_CharacterDamageManagerComponent.Cast(pManagerComponent);
		if (characterDamageManager)
			characterDamageManager.SetResilienceHitZone(this);
	}
};

//-----------------------------------------------------------------------------------------------------------
//! Blood - does not receive damage directly, only via scripted events.
class SCR_CharacterBloodHitZone : SCR_RegeneratingHitZone
{
	//-----------------------------------------------------------------------------------------------------------
	override void OnDamageStateChanged()
	{
		super.OnDamageStateChanged();
		
		if (GetDamageStateThreshold(GetDamageState()) > GetDamageStateThreshold(ECharacterBloodState.UNCONSCIOUS))
			return;
		
		// TODO: Unconsciousness if blood level remains under unconscious for over 60 seconds
		// TODO: Death if blood level remains under lethal for over 300 seconds
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(GetHitZoneContainer());
		if (damageManager)
			damageManager.Kill();
	}

	//-----------------------------------------------------------------------------------------------------------
	override void OnInit(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.OnInit(pOwnerEntity, pManagerComponent);
		
		SCR_CharacterDamageManagerComponent manager = SCR_CharacterDamageManagerComponent.Cast(GetHitZoneContainer());
		if (manager)
			manager.SetBloodHitZone(this);
	}
};

//-----------------------------------------------------------------------------------------------------------
class SCR_CharacterHandsHitZone : ScriptedHitZone
{
	//-----------------------------------------------------------------------------------------------------------
	override void OnDamageStateChanged()
	{
		super.OnDamageStateChanged();
		
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(GetHitZoneContainer());
		if (damageManager)
			damageManager.SetAimingDamage(1 - GetDamageStateThreshold(GetDamageState()));
	}
};

//-----------------------------------------------------------------------------------------------------------
class SCR_CharacterLegsHitZone : ScriptedHitZone
{
	//-----------------------------------------------------------------------------------------------------------
	override void OnDamageStateChanged()
	{
		super.OnDamageStateChanged();
		
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(GetHitZoneContainer());
		if (damageManager)
			damageManager.SetMovementDamage(1 - GetDamageStateThreshold(GetDamageState()));
	}
};

//-----------------------------------------------------------------------------------------------------------
class SCR_CharacterHeadHitZone : SCR_CharacterHitZone
{
	//-----------------------------------------------------------------------------------------------------------
	override void OnInit(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.OnInit(pOwnerEntity, pManagerComponent);
		
		SCR_CharacterDamageManagerComponent manager = SCR_CharacterDamageManagerComponent.Cast(pManagerComponent);
		if (manager)
			manager.SetHeadHitZone(this);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Manage wounds submeshes
	override void UpdateSubmeshes()
	{
		// Head wounds indicate general bleeding and character damage as well
		if (!m_bIsWounded)
			super.UpdateSubmeshes();
	}
};
