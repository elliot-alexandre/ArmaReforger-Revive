[EntityEditorProps(category: "GameScripted/GameMode", description: "")]
class SCR_PlayerSpawnPointClass: SCR_SpawnPointClass
{
};
class SCR_PlayerSpawnPoint: SCR_SpawnPoint
{
	[Attribute("1", desc: "How often will the spawn's position be updated to match assigned player's position (in seconds).", category: "Player Spawn Point")]
	protected float m_fUpdateInterval;
	
	[Attribute(desc: "Spawn point visualization. Original 'Info' attribute will be ignored.", category: "Player Spawn Point")]
	protected ref SCR_PlayerUIInfo m_PlayerInfo;
	
	[RplProp(onRplName: "OnSetPlayerID")]
	protected int m_iPlayerID;
	
	protected Faction m_CachedFaction;
	protected IEntity m_TargetPlayer;
	
	//------------------------------------------------------------------------------------------------
	IEntity GetTargetPlayer()
	{
		return m_TargetPlayer;
	}
	
	/*!
	Assign player ID to this respawn point.
	It will then present itself as the player, and spawning on it will actually spawn the new player on position of assignd player.
	\param playerID Target player ID
	*/
	void SetPlayerID(int playerID)
	{
		if (playerID == m_iPlayerID || !Replication.IsServer())
			return;
		
		//--- Set and broadcast new player ID
		m_iPlayerID = playerID;
		OnSetPlayerID();
		Replication.BumpMe();
		
		//--- Assign player faction only when player entity exists
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawn);
		gameMode.GetOnPlayerKilled().Insert(OnPlayerDeath);
		gameMode.GetOnPlayerDeleted().Insert(OnPlayerDeath);
		
		IEntity player = SCR_PossessingManagerComponent.GetPlayerMainEntity(m_iPlayerID);
		if (player)
			OnPlayerSpawn(m_iPlayerID, player);
		else
			OnPlayerDeath(m_iPlayerID, null);
	}
	/*!
	Get ID of the player this spawn point is assigned to.
	\return Target player ID
	*/
	int GetPlayerID()
	{
		return m_iPlayerID;
	}
	
	protected void OnSetPlayerID()
	{
		//--- Link player info
		if (!m_PlayerInfo)
			m_PlayerInfo = new SCR_PlayerUIInfo();
		
		m_PlayerInfo.SetPlayerID(m_iPlayerID);
		LinkInfo(m_PlayerInfo);
	}
	
	protected void OnPlayerSpawn(int playerID, IEntity player)
	{
		if (playerID != m_iPlayerID)
			return;
		
		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		m_CachedFaction = respawnSystem.GetPlayerFaction(m_iPlayerID);
		m_TargetPlayer = player;
		
		ActivateSpawnPoint();
	}
	protected void OnPlayerDeath(int playerID, IEntity player)
	{
		if (playerID != m_iPlayerID)
			return;
		
		DeactivateSpawnPoint();
		
		m_CachedFaction = null;
		m_TargetPlayer = null;
	}
	protected void ActivateSpawnPoint()
	{
		SetFaction(m_CachedFaction);
		
		//--- Periodically refresh spawn's position
		//--- Clients cannot access another player's entity directly, because it may not be streamed for them
		ClearFlags(EntityFlags.STATIC, false);
		GetGame().GetCallqueue().CallLater(UpdateSpawnPos, m_fUpdateInterval * 1000, true);
	}
	protected void DeactivateSpawnPoint()
	{
		SetFaction(null);
		
		//--- Stop periodic refresh
		SetFlags(EntityFlags.STATIC, false);
		GetGame().GetCallqueue().Remove(UpdateSpawnPos);
	}
	
	protected void UpdateSpawnPos()
	{
		if (!m_TargetPlayer)
			return;
		
		vector pos = m_TargetPlayer.GetOrigin();
		UpdateSpawnPosBroadcast(pos);
		Rpc(UpdateSpawnPosBroadcast, pos);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void UpdateSpawnPosBroadcast(vector pos)
	{
		SetOrigin(pos);
	}
	
	override void GetPositionAndRotation(out vector pos, out vector rot)
	{
		super.GetPositionAndRotation(pos, rot);
		
		if (m_TargetPlayer)
		{
			SCR_CompartmentAccessComponent compartmentAccessTarget = SCR_CompartmentAccessComponent.Cast(m_TargetPlayer.FindComponent(SCR_CompartmentAccessComponent));
			IEntity vehicle = compartmentAccessTarget.GetVehicle();
			if (vehicle)
				rot = vehicle.GetAngles();
			else
				rot = m_TargetPlayer.GetAngles();
		}
	}
	override void EOnPlayerSpawn(IEntity entity)
	{
		//--- If spawn point target is sitting in a vehicle, move spawned player inside as well
		SCR_CompartmentAccessComponent compartmentAccessTarget = SCR_CompartmentAccessComponent.Cast(m_TargetPlayer.FindComponent(SCR_CompartmentAccessComponent));
		IEntity vehicle = compartmentAccessTarget.GetVehicle();
		if (vehicle)
		{
			SCR_CompartmentAccessComponent compartmentAccessPlayer = SCR_CompartmentAccessComponent.Cast(entity.FindComponent(SCR_CompartmentAccessComponent));
			GetGame().GetCallqueue().CallLater(compartmentAccessPlayer.MoveInVehicleAny, 1, false, vehicle); //--- Wait for character ownership to be transferred to client
		}
	}
	void ~SCR_PlayerSpawnPoint()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
		{
			gameMode.GetOnPlayerSpawned().Remove(OnPlayerSpawn);
			gameMode.GetOnPlayerKilled().Remove(OnPlayerDeath);
			gameMode.GetOnPlayerDeleted().Remove(OnPlayerDeath);
		}
		GetGame().GetCallqueue().Remove(UpdateSpawnPos);
	}
};