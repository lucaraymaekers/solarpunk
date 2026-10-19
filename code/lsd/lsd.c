//-
//  Linux Serial Downloader for the ADUC832v4 board
//- 
// Resources used:
// - Serial Programming Guide for POSIX Operating systems
//   https://www.msweet.org/serial/serial.html
// - AN-1074: Understanding the Serial Download Protocol
//   https://www.analog.com/en/resources/app-notes/an-1074.html
// - Data sheet for ADuC832
//   https://www.analog.com/media/en/technical-documentation/data-sheets/ADuC832.pdf
// - UC-007: User Donwload (ULOAD) Mode
//   https://www.analog.com/media/en/technical-documentation/application-notes/uC007___ULOAD_Example_for_ADuC834__ADuC832.pdf?doc=aduc832.pdf
// - Linux USB serial
//   https://docs.kernel.org/usb/usb-serial.html
//-

//~ Libraries

#define BASE_CONSOLE_APPLICATION 1
#define BASE_FORCE_THREADS_COUNT 1
#include "base/base.h"
#include "base/base.c"

#include "rl/rl_profile.h"

#include "lsd/generated/lsd.meta.c"
#include "lsd/lsd.h"
#if OS_LINUX
# include "lsd/lsd_linux.c"
#elif OS_WINDOWS
# include "lsd/lsd_windows.c"
#endif

//~ Globals

// Constants
read_only global_variable command Command_EraseAll = 
{
 .Kind = CommandKind_EraseAll, 
 .CheckSum = 0xBE
};

read_only global_variable command Command_Run = 
{
 .Kind = CommandKind_Run,
 .BufferCount = 3, .Buffer = {0x00, 0x00, 0x00}, 
 .CheckSum = 0xA7
};

read_only global_variable command Command_Interrogate = 
{
 .Kind = CommandKind_Interrogate, 
 .BufferCount = 4, .Buffer = { 0x21, 0x5A, 0x00, 0xA6 }, 
 .ExpectedBytesReturned = 25,
};

// NOTE(luca): See table 44 in datasheet for AduC832

read_only global_variable command Command_SetFastBaud =
{
 .Kind = CommandKind_ChangeBaud, 
 //                           T3CON T3FD
 .BufferCount = 2, .Buffer = {0x81, 0x09},
 .Baud = 230400,
};

#if 0
#if 0
#elif defined(B115200)
{
 .Kind = CommandKind_ChangeBaud, 
 //                           T3CON T3FD
 .BufferCount = 2, .Buffer = {0x82, 0x09},
 .Baud = 115200,
};
#elif defined(B57600)
{
 .Kind = CommandKind_ChangeBaud,
 .BufferCount = 2, .Buffer = {0x83, 0x09},
 .Baud = 57600,
};
#else
{
 .Kind = CommandKind_ChangeBaud, 
 // NOTE(luca): Assumes normal clock speed
 //                           T3CON T3FD
 .BufferCount = 2, .Buffer = {0x85, 0x2D},
 .Baud = 9600,
};
#endif
#endif

read_only global_variable command Command_SetSlowBaud = 
{
 .Kind = CommandKind_ChangeBaud, 
 // NOTE(luca): Assumes normal clock speed
 //                           T3CON T3FD
 .BufferCount = 2, .Buffer = {0x85, 0x2D},
 .Baud = 9600,
};


//~ Functions
internal u8
ParseHexDigit(str8 File, u64 *At, b32 *Success)
{
 u8 Result = 0;
 
 if(*At < File.Size)
 {  
  u8 Char = File.Data[*At];
  
  if(0) {}
  else if(Char >= '0' && Char <= '9')
  {
   Result = Char - '0';
  }
  else if(Char >= 'A' && Char <= 'F')
  {
   Result = Char - 'A' + 0x0A;
  }
  else if(Char >= 'a' && Char <= 'f')
  {
   Result = Char - 'a' + 0x0A;
  }
  else
  {
   *Success = false;
  }
  
  *At += 1;
 }
 else
 {
  *Success = false;
 }
 
 return Result;
}

internal u8
ParseHexDigitPair(str8 File, u64 *At, b32 *Succeeded)
{
 u8 Result = 0;
 
 u8 H = ParseHexDigit(File, At, Succeeded);
 u8 L = ParseHexDigit(File, At, Succeeded);
 Result = (u8)(H*0x10) + L;
 
 return Result;
}

internal u8
GetCheckSum(u64 Size, u8 *Data)
{
 u8 CheckSum = 0;
 
 for EachIndex(Idx, Size)
 {
  CheckSum += Data[Idx];
 }
 CheckSum = 0 - CheckSum;
 
 return CheckSum;
}

//~ Entry point
BASE_ENTRY_POINT()
{
 lsd_port_handle TerminalHandle = {0};
 str8 DeviceFileName = S8("");
 char *HexFileName = 0;
 b32 DidWork = false;
 b32 Run = false;
 b32 Udev = false;
 b32 ShowHelp = false;
 
 b32 ShouldQuit = false;
 
 cmd_line *CmdLine = &Params->CmdLine;
 
 arena *Arena = GetScratch();
 
 str8_array Help = PushS8Array(Arena, 256);
 
#if LSD_INTERNAL
 Prof_Profile()
#endif
 {
  GlobalVerbose = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("verbose"), S8("Increase verbosity"));
  Run = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("run"), S8("Run program after download"));
  Udev = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("udev"), S8("Print udev rule for setting usb latency timer automatically"));
  
  
  ShowHelp = (CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("h"), S8("Print out help")) ||
              ShowHelp);
  ShowHelp = (CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("-help"), S8("Print out help")) ||
              ShowHelp);
  if(ShowHelp) ShouldQuit = true;
  
  {  
   cmd_line_entry *Entry = CmdLineGetEntryAndAddHelp(CmdLine, &Help, S8("device"), S8("Select device e.g., (linux:) -device=/dev/ttyUSB1 or (windows:) -device=4 for COM4"));
   if(!IsNilCmdLineEntry(Entry))
   {
    DeviceFileName = Entry->First->V;
   }
   
   HexFileName = CSS8(CmdLine->FirstFile->V);
  }
  
  if(HexFileName)
  {
   DidWork = true;
   
   Log("Uploading %s\n", HexFileName);
   
   Prof_TimeScope("Open terminal")
   {
    TerminalHandle = LSD_OpenPort(DeviceFileName);
    if(TerminalHandle.Valid)
    {
     u64 CommandIdx = 0;
     u64 CommandCount = 0;
     
     command *Commands = PushArray(Arena, command, KB(1));
     
     Commands[CommandCount] = Command_Interrogate;
     CommandCount += 1;
     
     Prof_TimeScope("Hex file parse")
     {      
      str8 File = OS_ReadEntireFileIntoMemory(HexFileName);
      if(File.Size)
      {
       Commands[CommandCount] = Command_SetFastBaud;
       CommandCount += 1;
       
       Commands[CommandCount] = Command_EraseAll;
       CommandCount += 1;
       
       b32 ParseSucceeded = true;
       
       // Parse hex file and create program commands accordingly
       {
        b32 FinishedParsing = false;
        u64 At = 0;
        
        u8 PageIdx = 0;
        u64 BufferIdx = 0;
        
        while(!FinishedParsing)
        {        
         ParseSucceeded = (File.Data[At] == ':');
         At += 1;
         
         // 2 bytes: record length
         u8 RecordLength = ParseHexDigitPair(File, &At, &ParseSucceeded);
         
         // 4 bytes: load address
         u8 AddressH = ParseHexDigitPair(File, &At, &ParseSucceeded);
         u8 AddressL = ParseHexDigitPair(File, &At, &ParseSucceeded);
         
         // 2 bytes: record type
         u8 RecordType = ParseHexDigitPair(File, &At, &ParseSucceeded);
         if(RecordType == 0x00)
         {
          command *Command = Commands + CommandCount;
          
          Command->Kind = CommandKind_PageDownload;
          Command->Buffer[0] = PageIdx;
          
          // 1: PageIdx
          Command->BufferCount = 1 + 256;
          u8 *B = Command->Buffer + BufferIdx + 1;
          
          // x data bytes
          for EachIndex(Idx, RecordLength)
          {
           B[Idx] = ParseHexDigitPair(File, &At, &ParseSucceeded);
          }
          BufferIdx += RecordLength;
          
          if(BufferIdx >= 256)
          {
           Assert(BufferIdx == 256);
           PageIdx += 1;
           CommandCount += 1;
           BufferIdx = 0;
          }
          
          // 2 bytes: checksum
          
          // TODO(luca): Figure out why this checksum is different.
          //Command->CheckSum = 
          ParseHexDigitPair(File, &At, &ParseSucceeded);
          
          // 1 or 2 byte: newline
          if(File.Data[At] == '\r')
          {
           At += 1;
          }
          
          if(File.Data[At] == '\n')
          {
           At += 1;
          }
          else
          {
           ParseSucceeded = false;
          }
         }
         else if(RecordType == 0x01)
         {
          FinishedParsing = true;
         }
         else
         {
          ParseSucceeded = false;
         }
         
         if(!FinishedParsing &&
            (!ParseSucceeded || At >= File.Size))
         {
          FinishedParsing = true;
          ParseSucceeded = false;
         }
        }
        
        if(PageIdx == 0 && BufferIdx > 0)
        {
         CommandCount += 1;
        }
        
       }
       
       Commands[CommandCount] = Command_SetSlowBaud;
       CommandCount += 1;
       
       if(ParseSucceeded)
       {
        if(Run)
        {
         Commands[CommandCount] = Command_Run;
         CommandCount += 1;
        }
       }
       else
       {
        Log("Error: Invalid hex file '%s'.\n", HexFileName);
        ShouldQuit = true;
       }
      }
      else
      {
       Log("Error: Could not read file '%s'.\n", HexFileName);
       ShouldQuit = true;
      }
     }
     
     u8 ReadBuffer_[KB(64)] = {0};
     u8 *ReadBuffer = ReadBuffer_;
     u64 ReadBufferIdx = 0;
     
     u8 WriteBuffer_[KB(64)] = {0};
     u8 *WriteBuffer = WriteBuffer_;
     
     if(!GlobalVerbose) GlobalPad.Size = 1;
     
     u64 TotalBytesSent = 0;
     
     Prof_TimeScope("Commands")
     {
      while(!ShouldQuit && (CommandIdx < CommandCount))
      {
       command *Command = Commands + CommandIdx;
       
       //- Writing  
       {
        Prof_TimeScope("Write")
        {
         if(!Command->Written)
         {          
          u8 *Packet = WriteBuffer;
          
          u64 PacketSize = Command->BufferCount;
          u8 CommandByte = CommandBytes[Command->Kind];
          
          if(Command->ExpectedBytesReturned == 0)
          {
           // Default is expecting NAK/ACK
           Command->ExpectedBytesReturned = 1;
          }
          
          if(0) {}
          else if(Command->Kind == CommandKind_Interrogate)
          {
           MemoryCopy(Packet, Command->Buffer, Command->BufferCount);
          }
          else if(Command->Kind == CommandKind_PageDownload)
          {
           Packet[0] = 0x07;
           Packet[1] = CommandByte;
           MemoryCopy(Packet + 2, Command->Buffer, Command->BufferCount);
           
           // 1: id byte
           // 2: command byte
           // 3: checksum
           PacketSize += 1 + 1 + 1;
           
           u8 CheckSum = GetCheckSum(Command->BufferCount, Packet + 2);
           Packet[PacketSize - 1] = CheckSum;
          }
          else if(Command->Kind != CommandKind_None)
          {
           // 2: id bytes
           // 1: no. data bytes
           // 1: command byte
           // 1: checksum 
           PacketSize += 2 + 1 + 1 + 1;
           
           Packet[0] = 0x07;
           Packet[1] = 0x0E;
           // 1: Command byte
           Packet[2] = (u8)(Command->BufferCount + 1);
           Packet[3] = CommandByte;
           
           MemoryCopy(Packet + 4, Command->Buffer, Command->BufferCount);
           
           u8 CheckSum = GetCheckSum(Command->BufferCount + 2, Packet + 2);
           Packet[PacketSize - 1] = CheckSum;
           
           Assert(Command->CheckSum == 0 || Command->CheckSum == CheckSum);
          }
          
          lsd_rw_result Write = {0};
          Prof_TimeScope("Writing")
          {
           Write = LSD_Write(TerminalHandle, Packet, PacketSize);
          }
          if(Write.Count == PacketSize)
          {
#if 0           
#if OS_LINUX
           tcdrain(TerminalHandle)
#elif OS_WINDOWS
           FlushFileBuffers((HANDLE)TerminalHandle.Device);
#endif
#endif
           
           Command->Written = true;
           if(GlobalVerbose) 
           {
            Log("Command: %S (%lld bytes)\n", CommandDescriptions[Command->Kind], Write.Count);
           }
           else if(Command->Kind == CommandKind_Interrogate)
           {
            Log("Connect to loader.\n");
           }
           TotalBytesSent += Write.Count;
           
           Command->StartTSC = rdtsc();
          }
          else
          {
           Log("%SError: Could not write command.\n", GlobalPad);
#if OS_LINUX
           if(GlobalVerbose) PrintErrnoIf(true);
#endif
           ShouldQuit = true;
          }
         }
        }
       }
       
       //- Reading 
       {
        Prof_TimeScope("Read")
        {
         if(Command->Written)
         {
          lsd_rw_result Read = {0};
          Prof_TimeScope("Reading")
          {
           Read = LSD_Read(TerminalHandle, ReadBuffer + ReadBufferIdx, sizeof(ReadBuffer_));
          }
          ReadBufferIdx += Read.Count;
          
          if(ReadBufferIdx == Command->ExpectedBytesReturned)
          {
           str8 Response = { .Data = ReadBuffer, .Size = Command->ExpectedBytesReturned };
           
           // NOTE(luca): The command response was sucessfully read so we can bump the idx.
           CommandIdx += 1;
           ReadBufferIdx = 0;
           
           switch(Command->Kind)
           {
            case CommandKind_None: 
            case CommandKind_Count:
            break;
            
            case CommandKind_Interrogate:
            {
             str8 Identifier = S8To(Response, 10);
             str8 Version = S8FromTo(Response, 10, 14);
             
             if(GlobalVerbose)
              Log("%SID:        %S\n"
                  "%SVersion:   %S\n"
                  "%SHardware:  0x%x%x\n", 
                  GlobalPad, Identifier,
                  GlobalPad, Version,
                  GlobalPad, Response.Data[16], Response.Data[17]);
             
             u8 ResponseCheckSum = Response.Data[24];
             u8 CheckSum = GetCheckSum(24, Response.Data);
             
             if(!S8Match(Identifier, S8("ADI 832"), true))
             {
              Log("%SError: Board not supported.\n", GlobalPad);
              ShouldQuit = true;
             } 
             else if(CheckSum != ResponseCheckSum)
             {
              Log("%SError: Checksum failed, has: %x wanted: %x\n", GlobalPad, CheckSum, ResponseCheckSum);
              ShouldQuit = true;
             }
             else
             {
              Log("%SLoader connected.\n", GlobalPad);
             }
             
            } break;
            
            case CommandKind_PageDownload:
            case CommandKind_ChangeBaud:
            case CommandKind_EraseAll:
            case CommandKind_Program:
            case CommandKind_Run:
            {
             if(Read.Count == 1)
             {
              if(ReadBuffer[0] == 0x07)
              {
               Log("%SFailed: NAK.\n", GlobalPad);
               ShouldQuit = true;
              }
              if(ReadBuffer[0] == 0x06)
              {
               // NOTE(luca): "ACK"
               
#if LSD_INTERNAL
               u64 TSCElapsed = rdtsc() - Command->StartTSC;
               u64 CPUFreq = 2304001541;
               if(GlobalVerbose) Log("%Stook %fms\n", GlobalPad, 1000.0*(f64)TSCElapsed/(f64)CPUFreq);
#endif
               
               if(0) {}
               else if(Command->Kind == CommandKind_ChangeBaud)
               {
                if(GlobalVerbose) Log("%SSet baud rate to %d.\n", GlobalPad, Command->Baud);
                LSD_SetBaudRate(TerminalHandle, Command->Baud);
               }
              }
             }
             else
             {
              Log("Error: Received %llu, expected 1.\n", Read.Count);
              ShouldQuit = true;
             }
            } break;
           }
          }
          else
          {
           // NOTE(luca): Wait on more bytes
          }
         }
        }
       }
      }
     }
     
     if(TotalBytesSent)
     {
      if(GlobalVerbose) Log("\nCommand count: %llu.\n", CommandCount);
      Log("Total bytes sent: %lld.\n", TotalBytesSent);
     }
    }
   }
  }
  
  if(Udev)
  {
   DidWork = true;
   if(GlobalVerbose)
    Log("E.g., copy to /etc/udev/rules.d/99-usb-serial-latency.rules\n\n");
   Log("ACTION==\"add\", SUBSYSTEM==\"usb-serial\", ATTR{latency_timer}=\"1\"\n");
  }
  
  ShowHelp = (ShowHelp || !DidWork);
  if(ShowHelp)
  {
   Log("Usage: lsd [options] file.hex\n"
       "\n"
       "Options:\n", Params->Args[0]);
   
   str8 HelpText = S8ArrayJoinFrom(Arena, &Help, '\n');
   Log("%S\n", HelpText);
  }
  
 }
 
 return 0;
}