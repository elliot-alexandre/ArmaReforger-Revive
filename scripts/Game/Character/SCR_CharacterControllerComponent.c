[ComponentEditorProps(category: "GameScripted/Character", description: "Scripted character controller", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class SCR_CharacterControllerComponentClass : CharacterControllerComponentClass
{
};

class SCR_CharacterControllerComponent : CharacterControllerComponent
{
	// Private members
	private BaseWeaponManagerComponent m_WeaponManager;
	private SCR_CharacterCameraHandlerComponent m_CameraHandler; // Set from the camera handler itself
	private SCR_MeleeComponent m_MeleeComponent;
	private bool m_bOverrideActions = true;

	// Character event invokers
	ref ScriptInvoker m_OnPlayerDeath = new ScriptInvoker();
	ref ScriptInvoker<SCR_CharacterControllerComponent, IEntity> m_OnPlayerDeathWithParam = new ScriptInvoker();
	ref ScriptInvoker<IEntity, bool> m_OnControlledByPlayer = new ScriptInvoker();
	
	// Gadget even invokers
	ref ScriptInvoker<IEntity, bool, bool> m_OnGadgetStateChangedInvoker = new ref ScriptInvoker<IEntity, bool, bool>();
	ref ScriptInvoker<IEntity, bool> m_OnGadgetFocusStateChangedInvoker = new ref ScriptInvoker<IEntity, bool>();
	
	// Item even invokers
	ref ScriptInvoker<IEntity> m_OnItemUseBeganInvoker = new ref ScriptInvoker<IEntity>();
	ref ScriptInvoker<IEntity> m_OnItemUseCompleteInvoker = new ref ScriptInvoker<IEntity>();

	// Diagnostics, debugging
	#ifdef ENABLE_DIAG
	private static bool s_bDiagRegistered;
	private static bool m_bEnableDebugUI;
	private CharacterAnimationComponent m_AnimComponent;
	private Widget m_wDebugRootWidget;
	private int m_bDebugLastStance = ECharacterStance.STAND;
	#endif

	//------------------------------------------------------------------------------------------------
	//! Will be called when gadget taken/removed from hand
	override void OnGadgetStateChanged(IEntity gadget, bool isInHand, bool isOnGround) { m_OnGadgetStateChangedInvoker.Invoke(gadget, isInHand, isOnGround); };
	//! Will be called when gadget fully transitioned to or canceled focus mode
	override void OnGadgetFocusStateChanged(IEntity gadget, bool isFocused) { m_OnGadgetFocusStateChangedInvoker.Invoke(gadget, isFocused); };
	
	//------------------------------------------------------------------------------------------------
	//! Will be called when item use action is started
	override void OnItemUseBegan(IEntity item) { m_OnItemUseBeganInvoker.Invoke(item); };
	//! Will be called when item use action is complete
	override void OnItemUseComplete(IEntity item) { m_OnItemUseCompleteInvoker.Invoke(item); };

	//------------------------------------------------------------------------------------------------
	// handling of melee events. Sends true if melee started, false, when melee ends
	override void OnMeleeDamage(bool started)
	{
		m_MeleeComponent.SetMeleeAttackStarted(started);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDeath(IEntity instigator)
	{
		if (m_OnPlayerDeath != null)
			m_OnPlayerDeath.Invoke();

		if (m_OnPlayerDeathWithParam)
			m_OnPlayerDeathWithParam.Invoke(this, instigator);
		
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc && m_CameraHandler && m_CameraHandler.IsInThirdPerson())
			pc.m_bRetain3PV = true;
		
		// Insert the character and see if it held a weapon, if so, try adding that as well
		GarbageManager garbageManager = GetGame().GetGarbageManager();
		if (garbageManager)
		{
			garbageManager.Insert(GetCharacter());
			
			if (!m_WeaponManager)
				return;
			
			BaseWeaponComponent weaponOrSlot = m_WeaponManager.GetCurrentWeapon();
			if (!weaponOrSlot)
				return;
			
			IEntity weaponEntity;
			WeaponSlotComponent slot = WeaponSlotComponent.Cast(weaponOrSlot);
			if (slot)
				weaponEntity = slot.GetWeaponEntity();
			else
				weaponEntity = weaponOrSlot.GetOwner();
			
			if (!weaponEntity)
				return;
			
			garbageManager.Insert(weaponEntity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override bool GetCanMeleeAttack()
	{
		if (!m_MeleeComponent)
			return false;

		if (IsFalling())
			return false;
		
		if (GetStance() == ECharacterStance.PRONE)
			return false;
		
		if (GetCurrentMovementPhase() > EMovementType.RUN)
			return false;
		
		// TODO: Gadget melee weapon properties in case we want to be able to have melee component, like a shovel?
		if (IsGadgetInHands())
			return false;
		
		//! check presence of MeleeWeaponProperties component to ensure it is an Melee weapon or not
		if (!SCR_WeaponLib.CurrentWeaponHasComponent(m_WeaponManager, SCR_MeleeWeaponProperties))
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Return true to override default behaviour.
	//! Returns false to use default behaviour - Perform Action key will immediately perform the action
	override bool OnPerformAction()
	{
		return m_bOverrideActions;
	}

	//-----------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		
		
		ChimeraCharacter character = GetCharacter();
		if (!character)
			return;
		
		if (!m_WeaponManager)
			m_WeaponManager = BaseWeaponManagerComponent.Cast(character.FindComponent(BaseWeaponManagerComponent));

		if (!m_MeleeComponent)
			m_MeleeComponent = SCR_MeleeComponent.Cast(character.FindComponent(SCR_MeleeComponent));
		if (!m_CameraHandler)
			m_CameraHandler = SCR_CharacterCameraHandlerComponent.Cast(character.FindComponent(SCR_CharacterCameraHandlerComponent));

		#ifdef ENABLE_DIAG
		if (!m_AnimComponent)
			m_AnimComponent = CharacterAnimationComponent.Cast(character.FindComponent(CharacterAnimationComponent));
		#endif
	}

	//-----------------------------------------------------------------------------
	// #ifdef ENABLE_DIAG
	//-----------------------------------------------------------------------------
	#ifdef ENABLE_DIAG
	//-----------------------------------------------------------------------------
		//-----------------------------------------------------------------------------
		override void OnDiag(IEntity owner, float timeslice)
		{
			ChimeraCharacter character = GetCharacter();
			if (!character)
				return;
	
			if (IsDead())
				return;
			
			if (DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_NOBANKING))
				SetBanking(0);
	
			bool diagTransform = DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_TRANSFORMATION);
			bool bIsLocalCharacter = SCR_PlayerController.GetLocalControlledEntity() == character;
	
			// Character Transformation Diag
			if (bIsLocalCharacter && diagTransform)
			{
				DbgUI.Begin("Character Transformation");
				const string CHAR_POS = "X:\t%1\nY:\t%2\nZ:\t%3";
				const string CHAR_ROT = "PITCH:\t%1\nYAW:\t%2\nROLL:\t%3";
				const string CHAR_SCALE = "%1";
				const string CHAR_AIM = "X:\t%1\nY:\t%2\nZ:\t%3";
				vector pos = character.GetOrigin();
				vector rot = character.GetYawPitchRoll();
				float scale = character.GetScale();
				vector aimRot = GetAimingAngles();
				string strPosition = string.Format(CHAR_POS, pos[0].ToString(), pos[1].ToString(), pos[2].ToString());
				string strRotation = string.Format(CHAR_ROT, rot[1].ToString(), rot[0].ToString(), rot[2].ToString());
				string strScale = string.Format(CHAR_SCALE, scale.ToString());
				string strAiming = string.Format(CHAR_AIM, aimRot[0].ToString(), aimRot[1].ToString(), aimRot[2].ToString());
				DbgUI.Text("POSITION:\n" + strPosition);
				DbgUI.Text("ROTATION:\n" + strRotation);
				DbgUI.Text("SCALE:" + strScale);
				DbgUI.Text("AIMING ANGLES:\n" + strAiming);
	
				if (DbgUI.Button("Print plaintext to console"))
					Print("TransformInfo:\nPOSITION:\n"+strPosition + "\nROTATION:\n"+strRotation + "\nSCALE:"+strScale + "\nAIMING ANGLES:\n"+strAiming);
			
				DbgUI.End();
			}
	
			// Character Controller diag (inputs...)
			m_bEnableDebugUI = bIsLocalCharacter && DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_MENU);
			if (m_bEnableDebugUI && !m_wDebugRootWidget) // Currently controlled player
				CreateDebugUI();
			else if (!m_bEnableDebugUI && m_wDebugRootWidget)
				DeleteDebugUI();
	
			if (m_bEnableDebugUI)
				UpdateDebugUI();
		}
		
		//------------------------------------------------------------------------------------------------
		private void ReloadDebugUI()
		{
			if (m_wDebugRootWidget)
				DeleteDebugUI();
	
			CreateDebugUI();
		}
	
		//------------------------------------------------------------------------------------------------
		private void DeleteDebugUI()
		{
			m_wDebugRootWidget.RemoveFromHierarchy();
			m_wDebugRootWidget = null;
		}
	
		//------------------------------------------------------------------------------------------------
		private void UpdateInputCircles()
		{
			if (!m_wDebugRootWidget || !m_AnimComponent)
				return;
	
			Widget wInput = m_wDebugRootWidget.FindAnyWidget("Input");
			if (!wInput)
				return;
	
			WorkspaceWidget workspaceWidget = GetGame().GetWorkspace();
	
			float topSpeed = m_AnimComponent.GetTopSpeed(-1, false);
	
			TextWidget wMaxSpeed = TextWidget.Cast(wInput.FindAnyWidget("MaxSpeed"));
			if (wMaxSpeed)
			{
				float speed = Math.Ceil(topSpeed * 10) * 0.1;
				wMaxSpeed.SetText(speed.ToString() + "m/s");
			}
	
			float ringSize = FrameSlot.GetSizeX(wInput);
			float degSizeDivider = topSpeed * ringSize;
			if (degSizeDivider > 0)
			for (int spd = 0; spd < 3; spd++)
			{
				int spdType;
				if (spd == 0) spdType = EMovementType.WALK;
				if (spd == 1) spdType = EMovementType.RUN;
				if (spd == 2) spdType = EMovementType.SPRINT;
	
				int color;
				if (spd == 0) color = ARGB(255, 150, 255, 150);
				if (spd == 1) color = ARGB(255, 200, 200, 150);
				if (spd == 2) color = ARGB(255, 255, 150, 150);
				for (int deg = 0; deg < 360; deg++)
				{
					vector velInput = GetMovementVelocity();
					float inputForward = velInput[2];
					float inputRight = velInput[0];
	
					float degSize = m_AnimComponent.GetMaxSpeed(inputForward, inputRight, spdType) / degSizeDivider;
					if (spdType == EMovementType.SPRINT && !IsSprinting())
						degSize = 0;
	
					float degOff = (ringSize - degSize) * 0.5;
	
					string iRingname = "iRing_" + spd.ToString() + "_" + deg.ToString();
	
					ImageWidget wImg = ImageWidget.Cast(wInput.FindAnyWidget(iRingname));
					if (!wImg)
					{
						wImg = ImageWidget.Cast(workspaceWidget.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.BLEND | WidgetFlags.VISIBLE | WidgetFlags.STRETCH | WidgetFlags.NOWRAP, Color.FromInt(color), spd + 2, wInput));
						wImg.LoadImageTexture(0, "{90DF4F065D08EF4E}UI/Textures/HUD_obsolete/character/input_360ringslice.edds");
						wImg.SetRotation(deg);
						wImg.SetName(iRingname);
					}
					FrameSlot.SetAnchorMax(wImg, 0, 0);
					FrameSlot.SetAnchorMin(wImg, 0, 0);
					FrameSlot.SetSize(wImg, degSize, degSize);
					FrameSlot.SetPos(wImg, degOff, degOff);
				}
			}
		}
	
		//------------------------------------------------------------------------------------------------
		private void CreateDebugUI()
		{
			m_wDebugRootWidget = GetGame().GetWorkspace().CreateWidgets("{C70DA11469BBAF67}UI/layouts/Debug/HUD_Debug_Character.layout", null);
			UpdateInputCircles();
		}
	
		//------------------------------------------------------------------------------------------------
		private void UpdateDebugBoolWidget(string textWidgetName, bool isTrue, float time = 0)
		{
			TextWidget wTxt = TextWidget.Cast(m_wDebugRootWidget.FindAnyWidget(textWidgetName));
			if (!wTxt)
				return;
	
			time = Math.Ceil(time * 10) * 0.1;
			if (isTrue)
			{
				wTxt.SetColorInt(ARGB(255, 160, 210, 255));
				if (time != 0)
					wTxt.SetText("YES (" + time.ToString() + ")");
				else
					wTxt.SetText("YES");
			}
			else
			{
				wTxt.SetColorInt(ARGB(255, 255, 160, 160));
				if (time != 0)
					wTxt.SetText("NO (" + time.ToString() + ")");
				else
					wTxt.SetText("NO");
			}
		}
	
		//------------------------------------------------------------------------------------------------
		private void UpdateDebugUI()
		{
			ChimeraCharacter character = GetCharacter();
			
			// Reload debug UI for stance change
			if (m_bDebugLastStance != GetStance())
			{
				m_bDebugLastStance = GetStance();
				UpdateInputCircles();
			}
	
			float topSpeed = 0;
			if (m_AnimComponent)
				topSpeed = m_AnimComponent.GetTopSpeed(-1, false);
	
			Widget wCenter = m_wDebugRootWidget.FindAnyWidget("Center");
			Widget wCenter2 = m_wDebugRootWidget.FindAnyWidget("Center2");
			Widget wCenter3 = m_wDebugRootWidget.FindAnyWidget("Center3");
	
			vector movementInput = GetMovementInput();
			if (movementInput != vector.Zero)
			{
				float inputLength = movementInput.Length();
				if (inputLength > 1)
					movementInput *= 1 / inputLength;
			}
	
			if (wCenter && wCenter2 && wCenter3 && topSpeed > 0)
			{
				float inputForward = movementInput[0];
				float inputRight = movementInput[2];
				float maxSpeed = m_AnimComponent.GetMaxSpeed(inputForward, inputRight, GetCurrentMovementPhase());
				float moveScale = maxSpeed / topSpeed;
				float x = movementInput[0] * moveScale * 0.5 + 0.5;
				float y = -movementInput[2] * moveScale * 0.5 + 0.5;
				FrameSlot.SetAnchorMin(wCenter, x, y);
				FrameSlot.SetAnchorMax(wCenter, x, y);
	
				vector moveLocal = m_AnimComponent.GetInertiaSpeed();
				x = (moveLocal[0] / topSpeed) * 0.5 + 0.5;
				y = (-moveLocal[2] / topSpeed) * 0.5 + 0.5;
				FrameSlot.SetAnchorMin(wCenter2, x, y);
				FrameSlot.SetAnchorMax(wCenter2, x, y);
	
				moveLocal = character.VectorToLocal(GetVelocity());
				x = (moveLocal[0] / topSpeed) * 0.5 + 0.5;
				y = (-moveLocal[2] / topSpeed) * 0.5 + 0.5;
				FrameSlot.SetAnchorMin(wCenter3, x, y);
				FrameSlot.SetAnchorMax(wCenter3, x, y);
			}
	
			TextWidget wSpeed = TextWidget.Cast(m_wDebugRootWidget.FindAnyWidget("Speed"));
			TextWidget wSpeed2 = TextWidget.Cast(m_wDebugRootWidget.FindAnyWidget("Speed2"));
			if (wSpeed && wSpeed2)
			{
				float speed = Math.Ceil(m_AnimComponent.GetInertiaSpeed().Length() * 10) * 0.1;
				wSpeed.SetText(speed.ToString() + "m/s");
	
				speed = Math.Ceil(GetVelocity().Length() * 10) * 0.1;
				wSpeed2.SetText(speed.ToString() + "m/s");
			}
		
			TextWidget wAdjustedSpeed = TextWidget.Cast(m_wDebugRootWidget.FindAnyWidget("AdjustedSpeed"));
			if (wAdjustedSpeed)
				wAdjustedSpeed.SetText(GetDynamicSpeed().ToString());
		
			TextWidget wAdjustedStance = TextWidget.Cast(m_wDebugRootWidget.FindAnyWidget("AdjustedStance"));
			if (wAdjustedStance)
				wAdjustedStance.SetText(GetDynamicStance().ToString());
		
	
			UpdateDebugBoolWidget("ToggleSprint", GetIsSprintingToggle());
			UpdateDebugBoolWidget("IsInADS", IsWeaponADS());
			UpdateDebugBoolWidget("IsWeaponHolstered", !IsWeaponRaised());
			UpdateDebugBoolWidget("CanFireWeapon", GetCanFireWeapon());
			UpdateDebugBoolWidget("CanThrow", GetCanThrow());
			UpdateDebugBoolWidget("InFreeLook", IsFreeLookEnabled());
		}
	
	//-----------------------------------------------------------------------------
	// #endif ENABLE_DIAG
	//-----------------------------------------------------------------------------
	#endif
	
		//------------------------------------------------------------------------------------------------
	private void OnMapOpen(MapConfiguration config)
	{
		//! enable/disables player movement when the map is hidden/shown
		// true, disables the controls.
		
		if (SCR_PlayerController.GetLocalControlledEntity() != GetOwner())
			return;

		SetDisableMovementControls(true);
	}
	
	private void OnMapClose(MapConfiguration config)
	{
		//! enable/disables player movement when the map is hidden/shown
		// false, enables the controls.
		
		if (SCR_PlayerController.GetLocalControlledEntity() != GetOwner())
			return;

		SetDisableMovementControls(false);
	}
	
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		if (am.GetActionTriggered("GetOut") && CanGetOutVehicleScript())
		{
			CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(owner.FindComponent(CompartmentAccessComponent));
			BaseCompartmentSlot compartment = compAccess.GetCompartment();
			if (compartment)
			{
				CompartmentUserAction action = compartment.GetGetOutAction();
				if (action && action.CanBePerformed(GetOwner()))
				{
					action.PerformAction(compartment.GetOwner(), GetOwner());
					return;
				}
				
				if (!action && compAccess.CanGetOutVehicle())
				{
					compAccess.GetOutVehicle(-1);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void SCR_CharacterControllerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		#ifdef ENABLE_DIAG
		if (System.IsCLIParam("CharacterDebugUI")) // If the startup parameter is set, enable the debug UI
			m_bEnableDebugUI = true;

		if (!s_bDiagRegistered)
		{
			DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_MENU, "", "Enable Debug UI", "Character");
			DiagMenu.SetValue(SCR_DebugMenuID.DEBUGUI_CHARACTER_MENU, m_bEnableDebugUI);
			DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_NOBANKING, "", "Disable Banking", "Character");
			DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_TRANSFORMATION, "", "Enable transform info", "Character");
			DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_RECOIL_CAMERASHAKE_DISABLE, "", "Disable recoil cam shake", "Character");
			DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_ADDITIONAL_CAMERASHAKE_DISABLE, "", "Disable additional cam shake", "Character");
			DiagMenu.RegisterBool(SCR_DebugMenuID.DEBUGUI_CHARACTER_CAMERASHAKE_TEST_WINDOW, "", "Show shake diag", "Character");

			s_bDiagRegistered = true;
		}
		#endif

		SCR_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
		SCR_MapEntity.GetOnMapClose().Insert(OnMapClose);
	}

	//------------------------------------------------------------------------------------------------
	void ~SCR_CharacterControllerComponent()
	{
		SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
		SCR_MapEntity.GetOnMapClose().Remove(OnMapClose);

		#ifdef ENABLE_DIAG
		if (m_wDebugRootWidget)
			DeleteDebugUI();
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
		If a weapon with sights is equipped, advances to desired sights FOV info.
		\param allowCycling If enabled, selection will cycle from end to start and from start to end, otherwise it will be clamped.
		\param direction If above zero, advances to next info. If below zero, advances to previous info.
	*/
	void SetNextSightsFOVInfo(int direction = 1, bool allowCycling = false)
	{
		if (direction == 0)
			return;
		
		SightsFOVInfo fovInfo = GetSightsFOVInfo();
		if (!fovInfo)
			return;
		
		SCR_BaseVariableSightsFOVInfo variableFovInfo = SCR_BaseVariableSightsFOVInfo.Cast(fovInfo);
		if (!variableFovInfo)
			return;
		
		if (direction > 0)
			variableFovInfo.SetNext(allowCycling);
		else
			variableFovInfo.SetPrevious(allowCycling);
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
		Returns currently used SightsFOVInfo if any, null otherwise.
	*/
	SightsFOVInfo GetSightsFOVInfo()
	{
		if (!m_WeaponManager)
			return null;
		
		BaseWeaponComponent weaponComponent = m_WeaponManager.GetCurrentWeapon();
		if (!weaponComponent)
			return null;
		
		BaseSightsComponent sightsComponent = weaponComponent.GetSights();
		if (!sightsComponent)
			return null;
		
		SightsFOVInfo fovInfo = sightsComponent.GetFOVInfo();
		if (!fovInfo)
			return null;
		
		return fovInfo;
	}

	protected override void OnControlledByPlayer(IEntity owner, bool controlled)
	{
		// Do initialization/deinitialization of character that was given/lost control by plyer here

		m_OnControlledByPlayer.Invoke(owner, controlled);
		
		// diiferentiate the inventory setup for player and AI
		auto pCharInvComponent = SCR_CharacterInventoryStorageComponent.Cast( owner.FindComponent( SCR_CharacterInventoryStorageComponent ) );
		if ( pCharInvComponent )
			pCharInvComponent.InitAsPlayer(owner, controlled);
	}
};

enum ECharacterGestures
{
	NONE = 0,
	POINT_WITH_FINGER = 1
};
