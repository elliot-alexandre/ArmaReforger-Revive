//------------------------------------------------------------------------------------------------
//! Type of consumable gadget
//@Modded(Revive): New value for Revive Feature 
enum EConsumableType
{
	None,
	Bandage,
    Revive,
	Health
};

//------------------------------------------------------------------------------------------------
//! Effect assigned to the consumable gadget
[BaseContainerProps()]
class SCR_ConsumableEffectBase : Managed
{		
	EConsumableType m_eConsumableType;
	
	//------------------------------------------------------------------------------------------------
	//! Apply consumable effect
	//! /param target is the character who is having the effect applied
	void ApplyEffect(IEntity target)
	{}
	
	//------------------------------------------------------------------------------------------------
	//! Condition whther this effect can be applied
	//! /param target is the character who is having the effect applied
	bool CanApplyEffect(IEntity target)
	{}
};

//------------------------------------------------------------------------------------------------
//! Bandage effect
[BaseContainerProps()]
class SCR_ConsumableBandage : SCR_ConsumableEffectBase
{

	//------------------------------------------------------------------------------------------------
	override void ApplyEffect(IEntity target)
	{
		SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(target.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageMgr)
			damageMgr.RemoveBleeding();
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanApplyEffect(IEntity target)
	{
		SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(target.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageMgr)
			return damageMgr.IsDamagedOverTime(EDamageType.BLEEDING);
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	void SCR_ConsumableBandage()
	{
		m_eConsumableType = EConsumableType.Bandage;
	}
};
//@Modded(Revive): ! Revive kit effect
[BaseContainerProps()]
class SCR_ConsumableRevive : SCR_ConsumableEffectBase
{

	//------------------------------------------------------------------------------------------------
	override void ApplyEffect(IEntity target)
	{
		SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(target.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageMgr) {
			damageMgr.RemoveBleeding();
            damageMgr.RemoveDeath();
        }
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanApplyEffect(IEntity target)
	{
		SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(target.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageMgr)
			return damageMgr.isRevivable;
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	void SCR_ConsumableRevive()
	{
		m_eConsumableType = EConsumableType.Revive;
	}
};
