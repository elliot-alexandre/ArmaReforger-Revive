[EntityEditorProps(category: "GameScripted/Triggers", description: "")]
class SCR_BaseTriggerEntityClass: ScriptedGameTriggerEntityClass
{
};
/*!
Basic scripted trigger which offers external invoekrs for major events like activation or deactivation.
*/
class SCR_BaseTriggerEntity: ScriptedGameTriggerEntity
{
	protected ref ScriptInvoker m_OnActivate = new ScriptInvoker();
	protected ref ScriptInvoker m_OnDeactivate = new ScriptInvoker();
	
	protected bool IsAlive(IEntity entity)
	{
		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));

		DamageManagerComponent damageManager = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));

		if(dmgManager.isRevivable == true){
			return false;
		}else{
			return damageManager.GetState() != EDamageState.DESTROYED;
		};
	}
	
	/*!
	\return Invoker called on server when the trigger is activated
	*/
	ScriptInvoker GetOnActivate()
	{
		return m_OnActivate;
	}
	/*!
	\return Invoker called on server when the trigger is deactivated
	*/
	ScriptInvoker GetOnDeactivate()
	{
		return m_OnDeactivate;
	}
	
	override protected event void OnActivate(IEntity ent)
	{
		m_OnActivate.Invoke();
	}
	override protected event void OnDeactivate(IEntity ent)
	{
		m_OnDeactivate.Invoke();
	}
};