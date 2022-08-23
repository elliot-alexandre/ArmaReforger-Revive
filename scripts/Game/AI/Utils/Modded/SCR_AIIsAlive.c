//------------------------------------------------------------------------------------------------
//! Helper function to find out entity is alive

modded class SCR_AIIsAlive
{
	//------------------------------------------------------------------------------------------------
	override static bool IsAlive(IEntity entity)
	{
		if (!entity)
			return false;
		
		DamageManagerComponent damageManager;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
			damageManager = character.GetDamageManager();
		else
			damageManager = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		
		//if (!damageManager)
		//	return true;
		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));

		if(dmgManager.isRevivable == true || !damageManager){
			return false;
		}else{
			return damageManager.GetState() != EDamageState.DESTROYED;
		}
	}
};