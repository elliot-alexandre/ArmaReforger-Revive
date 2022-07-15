# ArmaReforger-Revive (WIP)

## Adding revivable Player

The goal here was to change the initial way the player dies to be changed for an "unconscious" state.

- When The player receives enough damage to his resilence hit zone it calls the override function Kill().
- It set a timer of 30s to make the Player bleed out if not bandaged or revive.

## TODO

- Disable the AI attack behaviour when someone is unconscious.
- Add functionality to pull out a Player unconscious inside of a vehicle.
- Add ragdoll effect without destroying the entity.
- Set a limit or threshold of the amount a player can be revived.
- Add that headshot neutralized a Player immediately.
