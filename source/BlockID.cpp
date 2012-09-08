
// BlockID.cpp

// Implements the helper functions for converting Block ID string to int etc.

#include "Globals.h"
#include "BlockID.h"
#include "../iniFile/iniFile.h"
#include "cItem.h"





NIBBLETYPE g_BlockLightValue[256];
NIBBLETYPE g_BlockSpreadLightFalloff[256];
bool       g_BlockTransparent[256];
bool       g_BlockOneHitDig[256];
bool       g_BlockPistonBreakable[256];
bool       g_BlockIsSnowable[256];
bool       g_BlockRequiresSpecialTool[256];





class cBlockIDMap
{
	// Making the map case-insensitive:
	struct Comparator
	{
		bool operator()(const AString & a_Item1, const AString & a_Item2)
		{
			return (NoCaseCompare(a_Item1, a_Item2) > 0);
		}
	} ;
	
	typedef std::map<AString, std::pair<short, short>, Comparator> ItemMap;
	
public:
	cBlockIDMap(void)
	{
		cIniFile Ini("items.ini");
		if (!Ini.ReadFile())
		{
			return;
		}
		long KeyID = Ini.FindKey("Items");
		if (KeyID == cIniFile::noID)
		{
			return;
		}
		unsigned NumValues = Ini.GetNumValues(KeyID);
		for (unsigned i = 0; i < NumValues; i++)
		{
			AString Name = Ini.ValueName(KeyID, i);
			if (Name.empty())
			{
				continue;
			}
			AString Value = Ini.GetValue(KeyID, i);
			AddToMap(Name, Value);
		}  // for i - Ini.Values[]
	}
	
	
	int Resolve(const AString & a_ItemName)
	{
		ItemMap::iterator itr = m_Map.find(a_ItemName);
		if (itr == m_Map.end())
		{
			return -1;
		}
		return itr->second.first;
	}
	
	
	bool ResolveItem(const AString & a_ItemName, cItem & a_Item)
	{
		ItemMap::iterator itr = m_Map.find(a_ItemName);
		if (itr != m_Map.end())
		{
			a_Item.m_ItemType = itr->second.first;
			a_Item.m_ItemDamage = itr->second.second;
			return true;
		}

		// Not a resolvable string, try pure numbers: "45:6", "45^6" etc.
		AStringVector Split = StringSplit(a_ItemName, ":");
		if (Split.size() == 1)
		{
			Split = StringSplit(a_ItemName, "^");
		}
		if (Split.empty())
		{
			return false;
		}
		a_Item.m_ItemType = (short)atoi(Split[0].c_str());
		if ((a_Item.m_ItemType == 0) && (Split[0] != "0"))
		{
			// Parsing the number failed
			return false;
		}
		if (Split.size() < 2)
		{
			return true;
		}
		a_Item.m_ItemDamage = atoi(Split[1].c_str());
		if ((a_Item.m_ItemDamage == 0) && (Split[1] != "0"))
		{
			// Parsing the number failed
			return false;
		}

		return true;
	}
	
	
	AString Desolve(short a_ItemType, short a_ItemDamage)
	{
		for (ItemMap::iterator itr = m_Map.begin(), end = m_Map.end(); itr != end; ++itr)
		{
			if ((itr->second.first == a_ItemType) && (itr->second.second == a_ItemDamage))
			{
				return itr->first;
			}
		}  // for itr - m_Map[]
		AString res;
		if (a_ItemDamage == -1)
		{
			Printf(res, "%d", a_ItemType);
		}
		else
		{
			Printf(res, "%d:%d", a_ItemType, a_ItemDamage);
		}
		return res;
	}
	
	
protected:
	ItemMap m_Map;
	
	
	void AddToMap(const AString & a_Name, const AString & a_Value)
	{
		AStringVector Split = StringSplit(a_Value, ":");
		if (Split.size() == 1)
		{
			Split = StringSplit(a_Value, "^");
		}
		if (Split.empty())
		{
			return;
		}
		short ItemType = (short)atoi(Split[0].c_str());
		short ItemDamage = (Split.size() > 1) ? (short)atoi(Split[1].c_str()) : -1;
		m_Map[a_Name] = std::make_pair(ItemType, ItemDamage);
	}
} ;





static cBlockIDMap gsBlockIDMap;





/*
// Quick self-test:
class Tester
{
public:
	Tester(void)
	{
		cItem Item;
		gsBlockIDMap.ResolveItem("charcoal", Item);
		AString Charcoal = gsBlockIDMap.Desolve(Item.m_ItemType, Item.m_ItemDamage);
		ASSERT(Charcoal == "charcoal");
	}
} test;
//*/





BLOCKTYPE BlockStringToType(const AString & a_BlockTypeString)
{
	int res = atoi(a_BlockTypeString.c_str());
	if ((res != 0) || (a_BlockTypeString.compare("0") == 0))
	{
		// It was a valid number, return that
		return res;
	}
	
	return gsBlockIDMap.Resolve(TrimString(a_BlockTypeString));
}




bool StringToItem(const AString & a_ItemTypeString, cItem & a_Item)
{
	return gsBlockIDMap.ResolveItem(TrimString(a_ItemTypeString), a_Item);
}





AString ItemToString(const cItem & a_Item)
{
	return gsBlockIDMap.Desolve(a_Item.m_ItemType, a_Item.m_ItemDamage);
}





AString ItemTypeToString(short a_ItemType)
{
	return gsBlockIDMap.Desolve(a_ItemType, -1);
}





EMCSBiome StringToBiome(const AString & a_BiomeString)
{
	// If it is a number, return it:
	int res = atoi(a_BiomeString.c_str());
	if ((res != 0) || (a_BiomeString.compare("0") == 0))
	{
		// It was a valid number
		return (EMCSBiome)res;
	}
	
	// Convert using the built-in map:
	static struct {
		EMCSBiome    m_Biome;
		const char * m_String;
	} BiomeMap[] =
	{
		{biOcean,            "Ocean"} ,
		{biPlains,           "Plains"},
		{biDesert,           "Desert"},
		{biExtremeHills,     "ExtremeHills"},
		{biForest,           "Forest"},
		{biTaiga,            "Taiga"},
		{biSwampland,        "Swampland"},
		{biRiver,            "River"},
		{biHell,             "Hell"},
		{biHell,             "Nether"},
		{biSky,              "Sky"},
		{biFrozenOcean,      "FrozenOcean"},
		{biFrozenRiver,      "FrozenRiver"},
		{biIcePlains,        "IcePlains"},
		{biIcePlains,        "Tundra"},
		{biIceMountains,     "IceMountains"},
		{biMushroomIsland,   "MushroomIsland"},
		{biMushroomShore,    "MushroomShore"},
		{biBeach,            "Beach"},
		{biDesertHills,      "DesertHills"},
		{biForestHills,      "ForestHills"},
		{biTaigaHills,       "TaigaHills"},
		{biExtremeHillsEdge, "ExtremeHillsEdge"},
		{biJungle,           "Jungle"},
		{biJungleHills,      "JungleHills"},
	} ;
	
	for (int i = 0; i < ARRAYCOUNT(BiomeMap); i++)
	{
		if (NoCaseCompare(BiomeMap[i].m_String, a_BiomeString) == 0)
		{
			return BiomeMap[i].m_Biome;
		}
	}  // for i - BiomeMap[]
	return (EMCSBiome)-1;
}





// This is actually just some code that needs to run at program startup, so it is wrapped into a global var's constructor:
class cBlockPropertiesInitializer
{
public:
	cBlockPropertiesInitializer(void)
	{
		memset(g_BlockLightValue,          0x00, sizeof(g_BlockLightValue));
		memset(g_BlockSpreadLightFalloff,  0x0f, sizeof(g_BlockSpreadLightFalloff)); // 0x0f means total falloff
		memset(g_BlockTransparent,         0x00, sizeof(g_BlockTransparent));
		memset(g_BlockOneHitDig,           0x00, sizeof(g_BlockOneHitDig));
		memset(g_BlockPistonBreakable,     0x00, sizeof(g_BlockPistonBreakable));
		memset(g_BlockIsSnowable,          0xff, sizeof(g_BlockIsSnowable));  // Set all blocks' snowable to true
		memset(g_BlockRequiresSpecialTool, 0x00, sizeof(g_BlockRequiresSpecialTool));  // Set all blocks to false

		// Emissive blocks
		g_BlockLightValue[E_BLOCK_FIRE]                 = 15;
		g_BlockLightValue[E_BLOCK_GLOWSTONE]            = 15;
		g_BlockLightValue[E_BLOCK_JACK_O_LANTERN]       = 15;
		g_BlockLightValue[E_BLOCK_LAVA]                 = 15;
		g_BlockLightValue[E_BLOCK_STATIONARY_LAVA]      = 15;
		g_BlockLightValue[E_BLOCK_END_PORTAL]           = 15;
		g_BlockLightValue[E_BLOCK_REDSTONE_LAMP_ON]     = 15;
		g_BlockLightValue[E_BLOCK_TORCH]                = 14;
		g_BlockLightValue[E_BLOCK_BURNING_FURNACE]      = 13;
		g_BlockLightValue[E_BLOCK_NETHER_PORTAL]        = 11;
		g_BlockLightValue[E_BLOCK_REDSTONE_ORE_GLOWING] = 9;
		g_BlockLightValue[E_BLOCK_REDSTONE_REPEATER_ON] = 9;
		g_BlockLightValue[E_BLOCK_REDSTONE_TORCH_ON]    = 7;
		g_BlockLightValue[E_BLOCK_BREWING_STAND]        = 1;
		g_BlockLightValue[E_BLOCK_BROWN_MUSHROOM]       = 1;
		g_BlockLightValue[E_BLOCK_DRAGON_EGG]           = 1;

		// Spread blocks
		g_BlockSpreadLightFalloff[E_BLOCK_AIR]              = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_CAKE]             = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_CHEST]            = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_CROPS]            = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_FENCE]            = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_FENCE_GATE]       = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_FIRE]             = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_GLASS]            = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_GLASS_PLANE]      = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_GLOWSTONE]        = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_IRON_BARS]        = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_IRON_DOOR]        = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_LEAVES]           = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_SIGN_POST]        = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_TORCH]            = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_VINES]            = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_WALLSIGN]         = 1;
		g_BlockSpreadLightFalloff[E_BLOCK_WOODEN_DOOR]      = 1;
		// Light in water and lava dissapears faster:
		g_BlockSpreadLightFalloff[E_BLOCK_LAVA]             = 2;
		g_BlockSpreadLightFalloff[E_BLOCK_STATIONARY_LAVA]  = 2;
		g_BlockSpreadLightFalloff[E_BLOCK_STATIONARY_WATER] = 2;
		g_BlockSpreadLightFalloff[E_BLOCK_WATER]            = 2;

		// Transparent blocks
		g_BlockTransparent[E_BLOCK_AIR]            = true;
		g_BlockTransparent[E_BLOCK_BROWN_MUSHROOM] = true;
		g_BlockTransparent[E_BLOCK_CHEST]          = true;
		g_BlockTransparent[E_BLOCK_COBWEB]         = true;
		g_BlockTransparent[E_BLOCK_CROPS]          = true;
		g_BlockTransparent[E_BLOCK_FIRE]           = true;
		g_BlockTransparent[E_BLOCK_GLASS]          = true;
		g_BlockTransparent[E_BLOCK_ICE]            = true;
		g_BlockTransparent[E_BLOCK_RED_MUSHROOM]   = true;
		g_BlockTransparent[E_BLOCK_RED_ROSE]       = true;
		g_BlockTransparent[E_BLOCK_SIGN_POST]      = true;
		g_BlockTransparent[E_BLOCK_SNOW]           = true;
		g_BlockTransparent[E_BLOCK_TALL_GRASS]     = true;
		g_BlockTransparent[E_BLOCK_TORCH]          = true;
		g_BlockTransparent[E_BLOCK_VINES]          = true;
		g_BlockTransparent[E_BLOCK_WALLSIGN]       = true;
		g_BlockTransparent[E_BLOCK_YELLOW_FLOWER]  = true;

		// TODO: Any other transparent blocks?

		// One hit break blocks
		g_BlockOneHitDig[E_BLOCK_BROWN_MUSHROOM]        = true;
		g_BlockOneHitDig[E_BLOCK_CROPS]                 = true;
		g_BlockOneHitDig[E_BLOCK_FIRE]                  = true;
		g_BlockOneHitDig[E_BLOCK_LOCKED_CHEST]          = true;
		g_BlockOneHitDig[E_BLOCK_REDSTONE_REPEATER_OFF] = true;
		g_BlockOneHitDig[E_BLOCK_REDSTONE_REPEATER_ON]  = true;
		g_BlockOneHitDig[E_BLOCK_REDSTONE_TORCH_OFF]    = true;
		g_BlockOneHitDig[E_BLOCK_REDSTONE_TORCH_ON]     = true;
		g_BlockOneHitDig[E_BLOCK_REDSTONE_WIRE]         = true;
		g_BlockOneHitDig[E_BLOCK_REDSTONE_WIRE]         = true;
		g_BlockOneHitDig[E_BLOCK_RED_MUSHROOM]          = true;
		g_BlockOneHitDig[E_BLOCK_RED_ROSE]              = true;
		g_BlockOneHitDig[E_BLOCK_REEDS]                 = true;
		g_BlockOneHitDig[E_BLOCK_SAPLING]               = true;
		g_BlockOneHitDig[E_BLOCK_TNT]                   = true;
		g_BlockOneHitDig[E_BLOCK_TALL_GRASS]            = true;
		g_BlockOneHitDig[E_BLOCK_TORCH]                 = true;
		g_BlockOneHitDig[E_BLOCK_YELLOW_FLOWER]         = true;

		// Blocks that breaks when pushed by piston
		g_BlockPistonBreakable[E_BLOCK_AIR]                   = true;
		g_BlockPistonBreakable[E_BLOCK_BED]                   = true;
		g_BlockPistonBreakable[E_BLOCK_BROWN_MUSHROOM]        = true;
		g_BlockPistonBreakable[E_BLOCK_COBWEB]                = true;
		g_BlockPistonBreakable[E_BLOCK_CROPS]                 = true;
		g_BlockPistonBreakable[E_BLOCK_DEAD_BUSH]             = true;
		g_BlockPistonBreakable[E_BLOCK_FIRE]                  = true;
		g_BlockPistonBreakable[E_BLOCK_IRON_DOOR]             = true;
		g_BlockPistonBreakable[E_BLOCK_JACK_O_LANTERN]        = true;
		g_BlockPistonBreakable[E_BLOCK_LADDER]                = true;
		g_BlockPistonBreakable[E_BLOCK_LAVA]                  = false;
		g_BlockPistonBreakable[E_BLOCK_LEVER]                 = true;
		g_BlockPistonBreakable[E_BLOCK_MELON]                 = true;
		g_BlockPistonBreakable[E_BLOCK_MELON_STEM]            = true;
		g_BlockPistonBreakable[E_BLOCK_PUMPKIN]               = true;
		g_BlockPistonBreakable[E_BLOCK_PUMPKIN_STEM]          = true;
		g_BlockPistonBreakable[E_BLOCK_REDSTONE_TORCH_OFF]    = true;
		g_BlockPistonBreakable[E_BLOCK_REDSTONE_TORCH_ON]     = true;
		g_BlockPistonBreakable[E_BLOCK_REDSTONE_WIRE]         = true;
		g_BlockPistonBreakable[E_BLOCK_RED_MUSHROOM]          = true;
		g_BlockPistonBreakable[E_BLOCK_RED_ROSE]              = true;
		g_BlockPistonBreakable[E_BLOCK_REEDS]                 = true;
		g_BlockPistonBreakable[E_BLOCK_SNOW]                  = true;
		g_BlockPistonBreakable[E_BLOCK_STATIONARY_LAVA]       = false;
		g_BlockPistonBreakable[E_BLOCK_STATIONARY_WATER]      = false;  //This gave pistons the ability to drop water :D
		g_BlockPistonBreakable[E_BLOCK_STONE_BUTTON]          = true;
		g_BlockPistonBreakable[E_BLOCK_STONE_PRESSURE_PLATE]  = true;
		g_BlockPistonBreakable[E_BLOCK_TALL_GRASS]            = true;
		g_BlockPistonBreakable[E_BLOCK_TORCH]                 = true;
		g_BlockPistonBreakable[E_BLOCK_VINES]                 = true;
		g_BlockPistonBreakable[E_BLOCK_WATER]                 = false;
		g_BlockPistonBreakable[E_BLOCK_WOODEN_DOOR]           = true;
		g_BlockPistonBreakable[E_BLOCK_WOODEN_PRESSURE_PLATE] = true;
		g_BlockPistonBreakable[E_BLOCK_YELLOW_FLOWER]         = true;

		// Blocks that can be snowed over:
		g_BlockIsSnowable[E_BLOCK_BROWN_MUSHROOM]        = false;
		g_BlockIsSnowable[E_BLOCK_CACTUS]                = false;
		g_BlockIsSnowable[E_BLOCK_CHEST]                 = false;
		g_BlockIsSnowable[E_BLOCK_CROPS]                 = false;
		g_BlockIsSnowable[E_BLOCK_FIRE]                  = false;
		g_BlockIsSnowable[E_BLOCK_FIRE]                  = false;
		g_BlockIsSnowable[E_BLOCK_GLASS]                 = false;
		g_BlockIsSnowable[E_BLOCK_ICE]                   = false;
		g_BlockIsSnowable[E_BLOCK_LAVA]                  = false;
		g_BlockIsSnowable[E_BLOCK_LOCKED_CHEST]          = false;
		g_BlockIsSnowable[E_BLOCK_REDSTONE_REPEATER_OFF] = false;
		g_BlockIsSnowable[E_BLOCK_REDSTONE_REPEATER_ON]  = false;
		g_BlockIsSnowable[E_BLOCK_REDSTONE_TORCH_OFF]    = false;
		g_BlockIsSnowable[E_BLOCK_REDSTONE_TORCH_ON]     = false;
		g_BlockIsSnowable[E_BLOCK_REDSTONE_WIRE]         = false;
		g_BlockIsSnowable[E_BLOCK_RED_MUSHROOM]          = false;
		g_BlockIsSnowable[E_BLOCK_RED_ROSE]              = false;
		g_BlockIsSnowable[E_BLOCK_REEDS]                 = false;
		g_BlockIsSnowable[E_BLOCK_SAPLING]               = false;
		g_BlockIsSnowable[E_BLOCK_SIGN_POST]             = false;
		g_BlockIsSnowable[E_BLOCK_SNOW]                  = false;
		g_BlockIsSnowable[E_BLOCK_STATIONARY_LAVA]       = false;
		g_BlockIsSnowable[E_BLOCK_STATIONARY_WATER]      = false;
		g_BlockIsSnowable[E_BLOCK_TALL_GRASS]            = false;
		g_BlockIsSnowable[E_BLOCK_TNT]                   = false;
		g_BlockIsSnowable[E_BLOCK_TORCH]                 = false;
		g_BlockIsSnowable[E_BLOCK_VINES]                 = false;
		g_BlockIsSnowable[E_BLOCK_WALLSIGN]              = false;
		g_BlockIsSnowable[E_BLOCK_WATER]                 = false;
		g_BlockIsSnowable[E_BLOCK_YELLOW_FLOWER]         = false;

		//Blocks that don�t drop without a special tool
		g_BlockRequiresSpecialTool[E_BLOCK_DIAMOND_BLOCK]	= true;
		g_BlockRequiresSpecialTool[E_BLOCK_DIAMOND_ORE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_GOLD_BLOCK]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_GOLD_ORE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_IRON_BLOCK]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_IRON_ORE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_REDSTONE_ORE]	= true;
		g_BlockRequiresSpecialTool[E_BLOCK_REDSTONE_ORE_GLOWING]= true;
		g_BlockRequiresSpecialTool[E_BLOCK_LAPIS_ORE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_LAPIS_BLOCK]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_EMERALD_ORE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_COAL_ORE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_OBSIDIAN]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_STONE]			= true;
		g_BlockRequiresSpecialTool[E_BLOCK_COBBLESTONE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_END_STONE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_MOSSY_COBBLESTONE]	= true;
		g_BlockRequiresSpecialTool[E_BLOCK_SANDSTONE]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_STONE_BRICKS]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_COBWEB]			= true;
		g_BlockRequiresSpecialTool[E_BLOCK_SNOW]			= true;
		g_BlockRequiresSpecialTool[E_BLOCK_NETHER_BRICK]	= true;
		g_BlockRequiresSpecialTool[E_BLOCK_NETHERRACK]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_STONE_SLAB]		= true;
		g_BlockRequiresSpecialTool[E_BLOCK_DOUBLE_STONE_SLAB]	= true;
		g_BlockRequiresSpecialTool[E_BLOCK_STONE_PRESSURE_PLATE]= true;
		g_BlockRequiresSpecialTool[E_BLOCK_BRICK]			= true;
		g_BlockRequiresSpecialTool[E_BLOCK_COBBLESTONE_STAIRS] = true;
		g_BlockRequiresSpecialTool[E_BLOCK_STONE_BRICK_STAIRS] = true;
		g_BlockRequiresSpecialTool[E_BLOCK_SANDSTONE_STAIRS] = true;
		g_BlockRequiresSpecialTool[E_BLOCK_NETHER_BRICK_STAIRS] = true;
		g_BlockRequiresSpecialTool[E_BLOCK_VINES]			= true;


	}	
} BlockPropertiesInitializer;
		



