modded class SCR_DamageManagerComponent: DamageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	/*! Neutralize the entity with a specific damage type, registering the killer entity.
	\param instigator Source of the damage
	\param damageType Damage type
	*/

	
	override void Kill(IEntity instigator = null)
	{
		IEntity owner = GetOwner();

		if (!owner)
			return;
		
		if (!instigator)
			instigator = owner;
		
		HitZone hitZone = GetDefaultHitZone();
		
		if (!hitZone)
			return;
		
		hitZone.SetHealth(5);
		
		vector hitPosDirNorm[3];
		
		PlayerManager new_PlayerManager = GetGame().GetPlayerManager();	
		
		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));
	
		if(dmg.isRevivable == true){
			HandleDamage(EDamageType.TRUE, hitZone.GetMaxHealth(), hitPosDirNorm, owner, hitZone, instigator, null, -1, -1);
		}
		
		if(dmg.isRevivable == false && new_PlayerManager.GetPlayerIdFromControlledEntity(owner) != 0){
			dmg.AddDeath();
			OnDamageStateChanged(EDamageState.STATE3);
			GetGame().GetCallqueue().CallLater(BleedingOut, 30000, false, instigator);
		} 
		
		if(new_PlayerManager.GetPlayerIdFromControlledEntity(owner) == 0){
			HandleDamage(EDamageType.TRUE, hitZone.GetMaxHealth(), hitPosDirNorm, owner, hitZone, instigator, null, -1, -1);
		}
	}
	
	
	void BleedingOut(IEntity instigator = null)
	{
		IEntity owner = GetOwner();

		if (!owner)
			return;
		
		if (!instigator)
			instigator = owner;
		
		HitZone hitZone = GetDefaultHitZone();
		
		if (!hitZone)
			return;
		
		vector hitPosDirNorm[3];
		
		PlayerManager new_PlayerManager = GetGame().GetPlayerManager();	
		
		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));
	
		if(dmg.isRevivable == true && new_PlayerManager.GetPlayerIdFromControlledEntity(owner) != 0){
			// dmg.UpdatePlayerController(true, owner);
			HandleDamage(EDamageType.TRUE, hitZone.GetMaxHealth(), hitPosDirNorm, owner, hitZone, instigator, null, -1, -1);
		} else {
			return;
		}
	}
}