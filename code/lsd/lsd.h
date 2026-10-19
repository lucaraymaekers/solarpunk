/* date = October 19th 2026 11:07 am */

#ifndef LSD_H
#define LSD_H

//~ Types
typedef struct command command;
struct command
{
 command_kind Kind;
 
 u64 BufferCount;
 u8 Buffer[256];
 u8 CheckSum;
 s32 Baud;
 
 u64 StartTSC;
 
 u8 ExpectedBytesReturned;
 b32 Written;
};

typedef struct lsd_port_handle lsd_port_handle;
struct lsd_port_handle
{
 b32 Valid;
 u64 Options;
 u64 Device;
};

typedef struct lsd_rw_result lsd_rw_result;
struct lsd_rw_result
{
 b32 Valid;
 u64 Count;
};

//~ Globals
global_variable str8 GlobalPad =
{
 .Data =  (u8 *)"          ",
 .Size = sizeof("          ") - 1,
};

global_variable b32 GlobalVerbose;


//~ API
internal lsd_port_handle LSD_OpenPort(str8 DeviceFileName);
internal lsd_rw_result   LSD_Read (lsd_port_handle Handle, u8 *Buffer, u64 BufferSize);
internal lsd_rw_result   LSD_Write(lsd_port_handle Handle, u8 *Buffer, u64 BufferSize);
internal void            LSD_SetBaudRate(lsd_port_handle Handle, s32 Baud);

#endif //LSD_H
