//------------------------------------------------------------------------------------------------
class PauseMenuUI: ChimeraMenuBase
{
	InputManager m_InputManager;

	protected Widget m_wEditorOpen;
	protected Widget m_wEditorClose;
	protected TextWidget m_wVersion;
	protected Widget m_wRoot;
	protected Widget m_wFade;
	protected Widget m_wSystemTime;
	protected bool m_bFocused = true;

	//Editor and Photo mode Specific
	protected SCR_ButtonTextComponent m_EditorUnlimitedOpenButton;
	protected SCR_ButtonTextComponent m_EditorUnlimitedCloseButton;
	protected SCR_ButtonTextComponent m_EditorPhotoOpenButton;
	protected SCR_ButtonTextComponent m_EditorPhotoCloseButton;
	
	protected SCR_SaveLoadComponent m_SaveLoadComponent;
	
	const string EXIT_SAVE = "#AR-PauseMenu_ReturnSaveTitle";
	const string EXIT_NO_SAVE = "#AR-PauseMenu_ReturnTitle";
	
	const string EXIT_MESSAGE = "#AR-PauseMenu_ReturnText";
	const string EXIT_TITLE = "#AR-PauseMenu_ReturnTitle";
	const ResourceName EXIT_IMAGESET = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	const string EXIT_IMAGE = "exit";
	
	static ref ScriptInvoker m_OnPauseMenuOpened = new ScriptInvoker();
	static ref ScriptInvoker m_OnPauseMenuClosed = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		m_SaveLoadComponent = SCR_SaveLoadComponent.GetInstance();

		m_wRoot = GetRootWidget();
		m_wFade = m_wRoot.FindAnyWidget("BackgroundFade");
		m_wSystemTime = m_wRoot.FindAnyWidget("SystemTime");
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		SCR_ButtonTextComponent comp;

		// Continue
		comp = SCR_ButtonTextComponent.GetButtonText("Continue", m_wRoot);
		if (comp)
		{
			GetGame().GetWorkspace().SetFocusedWidget(comp.GetRootWidget());
			comp.m_OnClicked.Insert(Close);
		}

		// Restart
		comp = SCR_ButtonTextComponent.GetButtonText("Restart", m_wRoot);
		if (comp)
		{
			bool enabledRestart = !Replication.IsRunning();
			comp.GetRootWidget().SetVisible(enabledRestart);
			comp.m_OnClicked.Insert(OnRestart);
		}

		// Respawn
		comp = SCR_ButtonTextComponent.GetButtonText("Respawn", m_wRoot);
		if (comp)
		{
			bool canRespawn;
			BaseGameMode gameMode = GetGame().GetGameMode();
			if (gameMode)
			{
				RespawnSystemComponent respawn = RespawnSystemComponent.Cast(gameMode.FindComponent(RespawnSystemComponent));
				canRespawn = (respawn != null);
			}

			comp.GetRootWidget().SetVisible(canRespawn);
			comp.m_OnClicked.Insert(OnRespawn);
		}

		// Exit
		comp = SCR_ButtonTextComponent.GetButtonText("Exit", m_wRoot);
		if (comp)
		{
			comp.m_OnClicked.Insert(OnExit);
			if (IsSavingOnExit())
				comp.SetText(EXIT_SAVE);
			else
				comp.SetText(EXIT_NO_SAVE);
		}
		
		// Save
		comp = SCR_ButtonTextComponent.GetButtonText("Save", m_wRoot);
		if (comp)
		{
			comp.m_OnClicked.Insert(OnSave);
			comp.GetRootWidget().SetVisible(CanSave());
			comp.SetEnabled(IsSaveAvailable());
		}

		// Load
		comp = SCR_ButtonTextComponent.GetButtonText("Load", m_wRoot);
		if (comp)
		{
			comp.m_OnClicked.Insert(OnLoad);
			comp.GetRootWidget().SetVisible(CanLoad());
			comp.SetEnabled(IsLoadAvailable());
		}
		
		// Camera
		comp = SCR_ButtonTextComponent.GetButtonText("Camera", m_wRoot);
		if (comp)
		{
			comp.m_OnClicked.Insert(OnCamera);
			comp.GetRootWidget().SetEnabled(editorManager && !editorManager.IsOpened());
			comp.GetRootWidget().SetVisible(Game.IsDev());
		}

		// Settings
		comp = SCR_ButtonTextComponent.GetButtonText("Settings", m_wRoot);
		if (comp)
			comp.m_OnClicked.Insert(OnSettings);

		// Field Manual
		comp = SCR_ButtonTextComponent.GetButtonText("FieldManual", m_wRoot);
		if (comp)
		{
			comp.m_OnClicked.Insert(OnFieldManual);
		}

		// Players
		comp = SCR_ButtonTextComponent.GetButtonText("Players", m_wRoot);
		if (comp)
			comp.m_OnClicked.Insert(OnPlayers);

		// Version
		m_wVersion = TextWidget.Cast(m_wRoot.FindAnyWidget("Version"));
		if (m_wVersion)
			m_wVersion.SetText(GetGame().GetBuildVersion());

		// Unlimited editor (Game Master)
		m_EditorUnlimitedOpenButton = SCR_ButtonTextComponent.GetButtonText("EditorUnlimitedOpen",m_wRoot);
		if (m_EditorUnlimitedOpenButton)		
			m_EditorUnlimitedOpenButton.m_OnClicked.Insert(OnEditorUnlimited);

		m_EditorUnlimitedCloseButton = SCR_ButtonTextComponent.GetButtonText("EditorUnlimitedClose",m_wRoot);
		if (m_EditorUnlimitedCloseButton)		
			m_EditorUnlimitedCloseButton.m_OnClicked.Insert(OnEditorUnlimited);
		
		//--- Photo mode
		m_EditorPhotoOpenButton = SCR_ButtonTextComponent.GetButtonText("EditorPhotoOpen",m_wRoot);
		if (m_EditorPhotoOpenButton)
			m_EditorPhotoOpenButton.m_OnClicked.Insert(OnEditorPhoto);
		m_EditorPhotoCloseButton = SCR_ButtonTextComponent.GetButtonText("EditorPhotoClose",m_wRoot);
		if (m_EditorPhotoCloseButton)
			m_EditorPhotoCloseButton.m_OnClicked.Insert(OnEditorPhoto);
		
		SetEditorUnlimitedButton(editorManager);
		SetEditorPhotoButton(editorManager);
		
		if (editorManager)
		{
			editorManager.GetOnModeAdd().Insert(OnEditorModeChanged);
			editorManager.GetOnModeRemove().Insert(OnEditorModeChanged);
		}

		comp = SCR_ButtonTextComponent.GetButtonText("Feedback", m_wRoot);
		if (comp)
		{
			comp.m_OnClicked.Insert(OnFeedback);
		}

		m_InputManager = GetGame().GetInputManager();
		
		SCR_HUDManagerComponent hud = GetGame().GetHUDManager();
		if (hud)
			hud.SetVisible(false);
		
		m_OnPauseMenuOpened.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuShow()
	{
		//--- Close pause menu when editor is opened or closed
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager)
		{
			editorManager.GetOnOpened().Insert(Close);
			editorManager.GetOnClosed().Insert(Close);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuHide()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager)
		{
			editorManager.GetOnOpened().Insert(Close);
			editorManager.GetOnClosed().Insert(Close);
		}
		
		if (m_wFade)
			m_wFade.SetVisible(false);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		SCR_HUDManagerComponent hud = GetGame().GetHUDManager();
		if (hud)
			hud.SetVisible(true);
		
		m_OnPauseMenuClosed.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusLost()
	{
		m_bFocused = false;
		m_InputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, Close);
		m_InputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, Close);
			m_InputManager.RemoveActionListener("MenuBackWB", EActionTrigger.DOWN, Close);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusGained()
	{
		m_bFocused = true;
		m_InputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, Close);
		m_InputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, Close);
			m_InputManager.AddActionListener("MenuBackWB", EActionTrigger.DOWN, Close);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void FadeBackground(bool fade, bool animate = true)
	{
		if (!m_wFade)
			return;
		
		m_wFade.SetVisible(fade);
		m_wFade.SetOpacity(0);
		if (fade && animate)
			WidgetAnimator.PlayAnimation(m_wFade, WidgetAnimationType.Opacity, 1, WidgetAnimator.FADE_RATE_FAST, false, true);
	}

	//------------------------------------------------------------------------------------------------
	private void OnSettings()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.SettingsSuperMenu);
	}

	//------------------------------------------------------------------------------------------------
	private void OnFieldManual()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.FieldManualDialog);
	}

	//------------------------------------------------------------------------------------------------
	private void OnSave()
	{
		// TODO
	}
	
	//------------------------------------------------------------------------------------------------
	private void OnLoad()
	{
		// TODO
	}
	
	//------------------------------------------------------------------------------------------------
	private void OnExit()
	{
		// Create exit dialog
		DialogUI dialog = DialogUI.CreateOkCancelDialog();
		if (!dialog)
			return;
		
		dialog.SetMessage(EXIT_MESSAGE);
		dialog.SetTitle(EXIT_TITLE);
		dialog.SetTitleIcon(EXIT_IMAGESET, EXIT_IMAGE);
		dialog.m_OnConfirm.Insert(OnExitConfirm);
	}
	
	//------------------------------------------------------------------------------------------------
	private void OnExitConfirm()
	{
		if (m_SaveLoadComponent && IsSavingOnExit())
			m_SaveLoadComponent.Save();
		
		Close();
		GameStateTransitions.RequestGameplayEndTransition();
	}

	//------------------------------------------------------------------------------------------------
	private void OnEditorUnlimited()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager)
		{
			if (!editorManager.IsOpened() || editorManager.GetCurrentModeEntity().IsLimited())
			{
				editorManager.SetCurrentMode(false);
				editorManager.Open();
			}
			else
			{
				editorManager.Close();
			}
		}
		Close();
	}

	//Update Editor Mode Button text and if Enabled
	//------------------------------------------------------------------------------------------------
	private void SetEditorUnlimitedButton(SCR_EditorManagerEntity editorManager)
	{
		if (!m_EditorUnlimitedOpenButton || !m_EditorUnlimitedCloseButton) return;
		
		if (!editorManager || editorManager.IsLimited())
		{
			m_EditorUnlimitedOpenButton.GetRootWidget().SetVisible(true);
			m_EditorUnlimitedOpenButton.SetEnabled(false);
			m_EditorUnlimitedCloseButton.GetRootWidget().SetVisible(false);
			m_wSystemTime.SetVisible(false);
		}
		else
		{
			if (!editorManager.IsOpened() || editorManager.GetCurrentModeEntity().IsLimited())
			{
				m_EditorUnlimitedOpenButton.GetRootWidget().SetVisible(true);
				m_EditorUnlimitedCloseButton.GetRootWidget().SetVisible(false);
			}
			else
			{
				m_EditorUnlimitedOpenButton.GetRootWidget().SetVisible(false);
				m_EditorUnlimitedCloseButton.GetRootWidget().SetVisible(true);
				m_EditorUnlimitedCloseButton.SetEnabled(editorManager.CanClose());
			}
			m_wSystemTime.SetVisible(true);
		}
	}

	//Updates Editor and Photomode button if Rights changed
	//------------------------------------------------------------------------------------------------
	protected void OnEditorModeChanged(SCR_EditorModeEntity modeEntity)
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		SetEditorUnlimitedButton(editorManager);
		SetEditorPhotoButton(editorManager);
	}

	//------------------------------------------------------------------------------------------------
	private void OnEditorPhoto()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager)
		{
			if (!editorManager.IsOpened() || editorManager.GetCurrentMode() != EEditorMode.PHOTO)
			{
				editorManager.SetCurrentMode(EEditorMode.PHOTO);
				editorManager.Open();
			}
			else
			{
				editorManager.Close();
			}
		}
		Close();
	}

	//Update Photo Mode Button text and if Enabled
	//------------------------------------------------------------------------------------------------
	private void SetEditorPhotoButton(SCR_EditorManagerEntity editorManager)
	{		
		if (!m_EditorPhotoOpenButton || !m_EditorPhotoCloseButton) return;
		
		if (!editorManager || !editorManager.HasMode(EEditorMode.PHOTO))
		{
			m_EditorPhotoOpenButton.GetRootWidget().SetVisible(true);
			m_EditorPhotoOpenButton.SetEnabled(false);
			m_EditorPhotoCloseButton.GetRootWidget().SetVisible(false);
		}
		else
		{			
			if (!editorManager.IsOpened() || editorManager.GetCurrentMode() != EEditorMode.PHOTO)
			{
				m_EditorPhotoOpenButton.GetRootWidget().SetVisible(true);
				m_EditorPhotoCloseButton.GetRootWidget().SetVisible(false);
				
				//Set enabled
				m_EditorPhotoOpenButton.SetEnabled(!editorManager.IsLimited() || GetGame().GetPlayerController().GetControlledEntity());
			}
			else
			{
				m_EditorPhotoOpenButton.GetRootWidget().SetVisible(false);
				m_EditorPhotoCloseButton.GetRootWidget().SetVisible(true);
				m_EditorPhotoCloseButton.SetEnabled(editorManager.CanClose());
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	private void OnFeedback()
	{
		OpenFeedbackDialog();
	}

	//------------------------------------------------------------------------------------------------
	private void OnRestart()
	{
		GetGame().GetMenuManager().CloseAllMenus();
		ChimeraMenuBase.ReloadCurrentWorld();
	}

	//------------------------------------------------------------------------------------------------
	private void OnPlayers()
	{
		ArmaReforgerScripted.OpenPlayerList();
	}

	//------------------------------------------------------------------------------------------------
	private void OnRespawn()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		playerController.Activate();

		SCR_RespawnComponent respawn = SCR_RespawnComponent.Cast(playerController.FindComponent(SCR_RespawnComponent));
		if (!respawn)
			return;

		respawn.RequestPlayerSuicide();
		Close();
	}

	//------------------------------------------------------------------------------------------------
	private void OnCamera()
	{
		SCR_DebugCameraCore cameraCore = SCR_DebugCameraCore.Cast(SCR_DebugCameraCore.GetInstance(SCR_DebugCameraCore));
		if (cameraCore)
			cameraCore.ToggleCamera();
		Close();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);

		//Remove Editor modes listener
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		
		if (editorManager)
		{
			editorManager.GetOnModeAdd().Remove(OnEditorModeChanged);
			editorManager.GetOnModeRemove().Remove(OnEditorModeChanged);
		}
	}
	
	// TODO: Implement incomming API
	
	//------------------------------------------------------------------------------------------------
	protected bool CanSave()
	{
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CanLoad()
	{
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsLoadAvailable()
	{
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsSaveAvailable()
	{
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsSavingOnExit()
	{
		return !Replication.IsRunning() && m_SaveLoadComponent && m_SaveLoadComponent.CanSaveOnExit();
	}
};