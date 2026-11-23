
enum product_kind
{
ProductKind_None,
 ProductKind_Apples,
 ProductKind_Pears,
 ProductKind_Cabbage,
 ProductKind_Carrots,
 ProductKind_Wheat,
 ProductKind_Fish,
 ProductKind_Meat,
 ProductKind_Eggs,
 ProductKind_Berries,
 ProductKind_Coconuts,
 ProductKind_Crabs,
 ProductKind_Potatoes,
 ProductKind_Count
};
typedef enum product_kind product_kind;

str8 ProductNames[] =
{
{(u8 *)"None", 4},
 {(u8 *)"Apples", 6},
 {(u8 *)"Pears", 5},
 {(u8 *)"Cabbage", 7},
 {(u8 *)"Carrots", 7},
 {(u8 *)"Wheat", 5},
 {(u8 *)"Fish", 4},
 {(u8 *)"Meat", 4},
 {(u8 *)"Eggs", 4},
 {(u8 *)"Berries", 7},
 {(u8 *)"Coconuts", 8},
 {(u8 *)"Crabs", 5},
 {(u8 *)"Potatoes", 8},
 
};

u32 ProductIcons[] =
{
0,0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
};

