enum ESlotSize
{
	SLOT_1x1 = 1,
	SLOT_2x1,
	SLOT_2x2,
	SLOT_3x3
};

enum ESlotID
{
	SLOT_ANY 			= -1,
	SLOT_BACKPACK		= 0,
	SLOT_LBS_BUTTPACK,
	SLOT_VEST,
	SLOT_WEAPONS_STORAGE,
	SLOT_GADGETS_STORAGE,
	SLOT_LOADOUT_STORAGE,
	SLOT_LOOT_STORAGE = 9999
};

//@Note(Leo): Feel free to add as many types as you pleased
// *IMPORTANT* - If you tend to rename this enum please contact inventory programmers, since enum type name (ECommonItemType) is used for lookup in gamecode (refferenced in ItemAttributeCollectionClass.h)
//@Modded(Revive): New value for extended medical supply 
enum ECommonItemType
{
	NONE = 0,
   	BANDAGE = 1,
    REVIVE = 5,
   	AMMO = 10,
	MG_AMMO = 11,
	FOOD = 20,
	BINOCULARS = 50,
	COMPASS = 51,
	FLASHLIGHT = 52,
	RADIO = 53,
	WATCH = 54
};

//TODO: move this to the UI constants (if exists)
const float		BUTTON_OPACITY_DISABLED	= 0.2;
const float		BUTTON_OPACITY_ENABLED	= 1.0;