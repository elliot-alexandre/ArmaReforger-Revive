modded class SCR_PlayerSpawnPoint: SCR_SpawnPoint
{
	protected override void OnPlayerSpawn(int playerID, IEntity player)
	{
		if (playerID != m_iPlayerID)
			return;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		playerManager.GetPlayerController(playerID).Activate();
		
		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		m_CachedFaction = respawnSystem.GetPlayerFaction(m_iPlayerID);
		m_TargetPlayer = player;
		
		ActivateSpawnPoint();
	}
}