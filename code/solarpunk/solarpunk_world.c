//-
// SolarPunk world building: push islands, products, requirements and links
//-

internal island *
PushIsland(app_state *App, str8 Name)
{
 island *Island = PushArrayZero(App->IslandsArena, island, 1);
 Island->Name = Name;
 DLLAppend(Island, App->FirstIsland, App->LastIsland);
 return Island;
}

internal product *
PushProduct(app_state *App, island *Island, product_kind Kind, u64 Amount)
{
 product *Product = PushArrayZero(App->ProductsArena, product, 1);
 Product->Kind = Kind;
 Product->Amount = Amount;
 DLLAppend(Product, Island->FirstProduct, Island->LastProduct);
 return Product;
}

internal product *
PushRequirement(app_state *App, island *Island, product_kind Kind, u64 Amount)
{
 product *Product = PushArrayZero(App->ProductsArena, product, 1);
 Product->Kind = Kind;
 Product->Amount = Amount;
 DLLAppend(Product, Island->FirstRequirement, Island->LastRequiremement);
 return Product;
}

// NOTE: Creates a link in both directions, one on each island's list.
internal void
PushLink(app_state *App, island *A, island *B, s32 Cost)
{
 link *AB = PushArrayZero(App->LinksArena, link, 1);
 AB->Start = A;
 AB->Dest = B;
 AB->Cost = Cost;
 DLLAppend(AB, A->FirstLink, A->LastLink);
 
 link *BA = PushArrayZero(App->LinksArena, link, 1);
 BA->Start = B;
 BA->Dest = A;
 BA->Cost = Cost;
 DLLAppend(BA, B->FirstLink, B->LastLink);
}

internal void
BuildHardcodedWorld(app_state *App)
{
 ArenaClear(App->IslandsArena);
 ArenaClear(App->ProductsArena);
 ArenaClear(App->LinksArena);
 App->FirstIsland = App->LastIsland = 0;
 
 island *Verdant  = PushIsland(App, S8("Verdant Atoll"));
 island *Sunspire = PushIsland(App, S8("Sunspire"));
 island *Kelp     = PushIsland(App, S8("Kelp Haven"));
 island *Copper   = PushIsland(App, S8("Copperleaf"));
 island *Windward = PushIsland(App, S8("Windward Reach"));
 
 PushProduct(App, Verdant, ProductKind_Wheat, 120);
 PushProduct(App, Verdant, ProductKind_Apples, 300);
 PushRequirement(App, Verdant, ProductKind_Cabbage, 10);
 PushRequirement(App, Verdant, ProductKind_Fish, 40);
 
 PushProduct(App, Sunspire, ProductKind_Cabbage, 50);
 PushProduct(App, Sunspire, ProductKind_Eggs, 25);
 PushRequirement(App, Sunspire, ProductKind_Meat, 60);
 PushRequirement(App, Sunspire, ProductKind_Apples, 80);
 
 PushProduct(App, Kelp, ProductKind_Fish, 200);
 PushProduct(App, Kelp, ProductKind_Carrots, 150);
 PushRequirement(App, Kelp, ProductKind_Eggs, 8);
 PushRequirement(App, Kelp, ProductKind_Wheat, 30);
 
 PushProduct(App, Copper, ProductKind_Meat, 90);
 PushProduct(App, Copper, ProductKind_Pears, 70);
 PushRequirement(App, Copper, ProductKind_Carrots, 50);
 PushRequirement(App, Copper, ProductKind_Cabbage, 15);
 
 PushProduct(App, Windward, ProductKind_Pears, 40);
 PushProduct(App, Windward, ProductKind_Carrots, 30);
 PushRequirement(App, Windward, ProductKind_Fish, 25);
 PushRequirement(App, Windward, ProductKind_Eggs, 12);
 PushRequirement(App, Windward, ProductKind_Meat, 20);
 
 PushLink(App, Verdant,  Sunspire, 3);
 PushLink(App, Verdant,  Kelp,     2);
 PushLink(App, Sunspire, Copper,   4);
 PushLink(App, Kelp,     Copper,   5);
 PushLink(App, Kelp,     Windward, 3);
 PushLink(App, Copper,   Windward, 2);
}
