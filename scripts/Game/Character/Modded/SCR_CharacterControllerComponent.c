 modded class SCR_CharacterControllerComponent : CharacterControllerComponent
{
	protected override void OnControlledByPlayer(IEntity owner, bool controlled)
	{
		// Do initialization/deinitialization of character that was given/lost control by plyer here

		m_OnControlledByPlayer.Invoke(owner, controlled);

		// diiferentiate the inventory setup for player and AI
		auto pCharInvComponent = SCR_CharacterInventoryStorageComponent.Cast( owner.FindComponent( SCR_CharacterInventoryStorageComponent ) );
		if ( pCharInvComponent )
			pCharInvComponent.InitAsPlayer(owner, controlled);
			SCR_CharacterDamageManagerComponent playerDmg = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
			SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());

			playerDmg.isRevivable = false;

			player.Activate();
			playerDmg.UpdatePlayerController(true, owner);
	}
}