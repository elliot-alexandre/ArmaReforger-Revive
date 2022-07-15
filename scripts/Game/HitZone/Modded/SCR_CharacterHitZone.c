modded class SCR_CharacterHitZone : ScriptedHitZone
{

	override void OnDamage(EDamageType type, float damage, HitZone pOriginalHitzone, IEntity instigator, inout vector hitTransform[3], float speed, int colliderID, int nodeID)
	{
		super.OnDamage(type, damage, pOriginalHitzone, instigator, hitTransform, speed, colliderID, nodeID);
		
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(GetHitZoneContainer());
		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));

		
		if (this != pOriginalHitzone)
			return;

 		// Only serious hits should cause bleeding
		if (damage < GetCriticalDamageThreshold()*GetMaxHealth())
			return;
		
		if(dmg.isRevivable == true && damage < GetCriticalDamageThreshold()*GetMaxHealth()){
			damageManager.Kill();
		}
		
		// Adding immediately some blood to the clothes - currently it's based on the damage dealt.
		AddBloodToClothes(Math.Clamp(damage * DAMAGE_TO_BLOOD_MULTIPLIER, 0, 255));
		AddBleeding(colliderID);
	}
};

