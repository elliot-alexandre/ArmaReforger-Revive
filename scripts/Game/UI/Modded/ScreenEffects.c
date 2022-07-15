modded class SCR_ScreenEffects : SCR_InfoDisplayExtended
{

	//------------------------------------------------------------------------------------------------
	override void OnDamageStateChanged()
	{
		super.OnDamageStateChanged();
	
		// @Modded @todo Revive - Should be used for adding a new visual when the character is on the ground.
		/**
		if (m_eLifeState == EDamageState.UNDAMAGED){
			Print("Add new effect");
		}
		**/
	} 

};