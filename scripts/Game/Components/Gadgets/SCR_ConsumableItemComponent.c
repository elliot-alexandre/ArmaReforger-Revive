[EntityEditorProps(category: "GameScripted/Gadgets", description: "Consumable gadget")]
class SCR_ConsumableItemComponentClass : SCR_GadgetComponentClass
{
};

//------------------------------------------------------------------------------------------------
// Consumable gadget component
class SCR_ConsumableItemComponent : SCR_GadgetComponent
{
		
	[Attribute("", UIWidgets.Object, category: "Consumable" )]
	protected ref SCR_ConsumableEffectBase m_ConsumableEffect;
	
	[Attribute("0", UIWidgets.CheckBox, "Switch model on use, f.e. from packaged version", category: "Consumable")]
	protected bool m_bAlternativeModelOnAction;
	
	protected SCR_CharacterControllerComponent m_CharController;
	
	//------------------------------------------------------------------------------------------------
	//! Get consumable type
	//! \return consumable type
	EConsumableType GetConsumableType()
	{
		if (m_ConsumableEffect)
			return m_ConsumableEffect.m_eConsumableType;
		
		return EConsumableType.None;
	}
		
	//------------------------------------------------------------------------------------------------
	//! Apply consumable effect
	//! \param target is the target entity
	void ApplyItemEffect(IEntity target)
	{
		if (!m_ConsumableEffect)
			return;
		
		m_ConsumableEffect.ApplyEffect(target);
		
		InventoryStorageManagerComponent invManager = InventoryStorageManagerComponent.Cast(m_CharacterOwner.FindComponent(InventoryStorageManagerComponent));
		if (invManager)
		{
			ModeClear(EGadgetMode.IN_HAND);
			RplComponent.DeleteRplEntity(GetOwner(), false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! OnItemUseComplete event from SCR_CharacterControllerComponent
	protected void OnApplyToSelf()
	{
		ApplyItemEffect(m_CharacterOwner);
	}
	
	//------------------------------------------------------------------------------------------------
	//! OnItemUseBegan event from SCR_CharacterControllerComponent
	protected void OnUseBegan()
	{
		if (m_bAlternativeModelOnAction)
			SetAlternativeModel(true);
	}
		
	//-----------------------------------------------------------------------------
	//! Switch item model
	//! \param useAlternative determines whether alternative or base model is used
	void SetAlternativeModel(bool useAlternative)
	{		
		InventoryItemComponent inventoryItemComp = InventoryItemComponent.Cast( GetOwner().FindComponent(InventoryItemComponent));
		if (!inventoryItemComp)
			return;

		ItemAttributeCollection attributeCollection = inventoryItemComp.GetAttributes();
		if (!attributeCollection)
			return;
		
		SCR_AlternativeModel additionalModels = SCR_AlternativeModel.Cast( attributeCollection.FindAttribute(SCR_AlternativeModel));
		if (!additionalModels)
			return;

		ResourceName consumableModel;
		if (useAlternative)
			consumableModel = additionalModels.GetAlternativeModel();
		else
			consumableModel = SCR_Global.GetPrefabAttributeResource(GetOwner(), "MeshObject", "Object");

		Resource resource = Resource.Load(consumableModel);
		if (!resource)
			return;

		BaseResourceObject baseResource = resource.GetResource();
		if (baseResource)
		{
			VObject asset = baseResource.ToVObject();
			if (asset)
				GetOwner().SetObject(asset, "");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void ModeSwitch(EGadgetMode mode, IEntity charOwner)
	{
		super.ModeSwitch(mode, charOwner);
					
		if (mode == EGadgetMode.IN_HAND)
		{			
			m_CharController = SCR_CharacterControllerComponent.Cast(m_CharacterOwner.FindComponent(SCR_CharacterControllerComponent));
			m_CharController.m_OnItemUseCompleteInvoker.Insert(OnApplyToSelf);
			m_CharController.m_OnItemUseBeganInvoker.Insert(OnUseBegan);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void ModeClear(EGadgetMode mode)
	{			
		if (mode == EGadgetMode.IN_HAND)
		{
			if (m_CharController)
			{
				m_CharController.m_OnItemUseCompleteInvoker.Remove(OnApplyToSelf);
				m_CharController.m_OnItemUseBeganInvoker.Remove(OnUseBegan);
				m_CharController = null;
			}
		}
		
		super.ModeClear(mode);
	}
	
	//------------------------------------------------------------------------------------------------
	override void ActivateAction()
	{
		if (!m_ConsumableEffect || !m_ConsumableEffect.CanApplyEffect(m_CharacterOwner))
			return;
		
		if (m_CharController)
			m_CharController.TryUseEquippedItem();
	}
	
	//------------------------------------------------------------------------------------------------
	override EGadgetType GetType()
	{
		return EGadgetType.CONSUMABLE;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool IsSingleHanded()
	{
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeToggled()
	{
		return true;
	}
};
