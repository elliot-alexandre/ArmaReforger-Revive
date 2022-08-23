
//------------------------------------------------------------------------------------------------
modded class SCR_CharacterDamageManagerComponent : ScriptedDamageManagerComponent
{
	[Attribute("", UIWidgets.CheckBox, desc: "Revivable", category: "Revive", params: "ptc")]
    bool isRevivable;
	
	
		
	//-----------------------------------------------------------------------------------------------------------
	//!@modded(revive) Remove or enable controll
	void UpdatePlayerController(bool State, IEntity Owner){

		PlayerManager new_PlayerManager = GetGame().GetPlayerManager();
		
		if (!Owner)
			return;
		
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(Owner.FindComponent(CharacterControllerComponent));
		if(!controller)
			return;

		
		if(State == false){
			if(new_PlayerManager.GetPlayerController(new_PlayerManager.GetPlayerIdFromControlledEntity(Owner)) != null){
				new_PlayerManager.GetPlayerController(new_PlayerManager.GetPlayerIdFromControlledEntity(Owner)).Deactivate();
			}
			
			vector v1 = Vector(0,0,0);
			
			if(controller.CanChangeStance(3) == true){
				controller.SetStanceChange(3);
			}
			
			if(controller.CanChangeStance(3) == false){
				controller.SetStanceChange(2);
			}
			
			controller.SetMovement(0, v1);
			controller.SetDisableMovementControls(true);
			controller.SetDisableViewControls(true);
			controller.SetDisableWeaponControls(true);
			controller.SetInThirdPersonView(true);
			controller.RemoveGadgetFromHand(true);
		} 
		if(State == true) {
			if(new_PlayerManager.GetPlayerController(new_PlayerManager.GetPlayerIdFromControlledEntity(Owner)) != null){
				new_PlayerManager.GetPlayerController(new_PlayerManager.GetPlayerIdFromControlledEntity(Owner)).Activate();
			}
			controller.SetDisableMovementControls(false);
			controller.SetDisableViewControls(false);
			controller.SetDisableWeaponControls(false);
			controller.SetStanceChange(2);
		}
		
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//!@modded(revive) Stop death
	void RemoveDeath()
	{		
		if(isRevivable == true){
	        isRevivable = false;			

			IEntity newOwner = GetOwner();

			UpdatePlayerController(true, newOwner);

			// @Modded Revive1.0.0 @Description - Healing the player that was previously downed.
			// FullHeal();
		} else {
			return;
		}
		
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//!@modded(revive) Add death
	 void AddDeath()
	{
	
        if(isRevivable == false){
            isRevivable = true;
			IEntity newOwner = GetOwner();
			UpdatePlayerController(false, newOwner);
			OnDamageStateChanged(EDamageState.STATE1);
        }
		
	}
	
	override void RemoveBleeding()
	{		
		if (m_aBleedingHitZones)
		{
			HitZone hitZone;
			for (int i = m_aBleedingHitZones.Count() - 1; i >= 0; i--)
			{
				hitZone = m_aBleedingHitZones.Get(i);
				if (hitZone)
					RemoveBleedingHitZone(hitZone);
			}
		}
		
		if (m_pBloodHitZone)
			m_pBloodHitZone.SetDamageOverTime(EDamageType.BLEEDING, 0);


		RemoveDeath();
		UpdateBloodyFace();
	}
	
	
	
	//-----------------------------------------------------------------------------------------------------------
	void ~SCR_CharacterDamageManagerComponent()
	{
        RemoveDeath();
	}
	
};
