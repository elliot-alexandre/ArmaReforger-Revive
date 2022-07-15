modded class SCR_RespawnComponent : RespawnComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		m_PlayerController = PlayerController.Cast(owner);
		if (!m_PlayerController)
		{
			Print("SCR_RespawnComponent must be attached to PlayerController!", LogLevel.ERROR);
			return;
		}
		
		m_PlayerController.Activate();
		
		m_RplComponent = RplComponent.Cast(m_PlayerController.FindComponent(RplComponent));


		m_pFactionLock.GetInvoker().Insert(OnFactionLockChanged);
		m_pLoadoutLock.GetInvoker().Insert(OnLoadoutLockChanged);
		m_pSpawnPointLock.GetInvoker().Insert(OnSpawnPointLockChanged);

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
		{
			Print("No game mode found in the world, SCR_RespawnComponent will not function correctly!", LogLevel.ERROR);
			return;
		}

		m_RespawnSystemComponent = SCR_RespawnSystemComponent.Cast(gameMode.FindComponent(SCR_RespawnSystemComponent));

		gameMode.GetOnPlayerSpawned().Insert(NotifyOnPlayerSpawned);
		gameMode.GetOnPlayerKilled().Insert(NotifyOnPlayerSpawned);
	}
}