modded class SCR_ConsumableItemComponent : SCR_GadgetComponent
{
	//------------------------------------------------------------------------------------------------
	//! Apply consumable effect
	//! \param target is the target entity
	override void ApplyItemEffect(IEntity target)
	{
		if (!m_ConsumableEffect)
			return;
		
		m_ConsumableEffect.ApplyEffect(target);
		
		InventoryStorageManagerComponent invManager = InventoryStorageManagerComponent.Cast(m_CharacterOwner.FindComponent(InventoryStorageManagerComponent));
		if (invManager && m_ConsumableEffect.m_eConsumableType != EConsumableType.Revive)
		{
			ModeClear(EGadgetMode.IN_HAND);
			RplComponent.DeleteRplEntity(GetOwner(), false);
		}
	}
}