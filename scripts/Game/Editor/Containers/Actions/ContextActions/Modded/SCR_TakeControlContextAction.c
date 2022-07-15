modded class SCR_TakeControlContextAction : SCR_BaseEditorAction
{
	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		if (!hoveredEntity || !SCR_PossessingManagerComponent.GetInstance())
			return;
			
		GenericEntity owner = hoveredEntity.GetOwner();
		if (!owner) 
			return;
		
		SCR_EditableEntityComponent entityToControl;
		SCR_BaseCompartmentManagerComponent compartmentManager = SCR_BaseCompartmentManagerComponent.Cast(owner.FindComponent(SCR_BaseCompartmentManagerComponent));
		
		if (!compartmentManager)
		{
			entityToControl = hoveredEntity;
		}
		else 
		{			
			array<IEntity> pilots = new array<IEntity>;
			compartmentManager.GetOccupantsOfType(pilots, ECompartmentType.Pilot);
		
			if (pilots.IsEmpty() || !pilots[0])
				return;
			
			entityToControl = SCR_EditableEntityComponent.Cast(pilots[0].FindComponent(SCR_EditableEntityComponent));
		}
		
		if (!entityToControl)
			return;
		
		SCR_EditableEntityComponent aiEntity = entityToControl.GetAIEntity();
		if (!aiEntity)
			return;
		
		//--- Find player controller (param is playerID)
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(param));
		if (!playerController)
			return;

		playerController.SetPossessedEntity(aiEntity.GetOwner());
		playerController.Activate();
		//--- Close the editor (ToDo: Move to lobby's OnPossessed event)
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (core)
		{
			SCR_EditorManagerEntity manager = core.GetEditorManager(param);
			if (manager)
				manager.Close();
		}
	}
};