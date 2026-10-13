enum command_kind
{
CommandKind_None,
 CommandKind_Interrogate,
 CommandKind_EraseAll,
 CommandKind_Program,
 CommandKind_ChangeBaud,
 CommandKind_Run,
 CommandKind_BatchProgram,
 CommandKind_Count
};
typedef enum command_kind command_kind;

str8 CommandDescriptions[] = {
{(u8 *)"Nothing", 7},{(u8 *)"Connect to loader", 17},{(u8 *)"Erase Flash/EE program and data memory", 38},{(u8 *)"Progam block of Flash/EE program memory", 39},{(u8 *)"Change Baud Rate", 16},{(u8 *)"Jump to User Code", 17},{(u8 *)"Batch - Program block of Flash/EE program memory", 48},
};

