// NOTE(luca): This program is a Serial Downloader for Linux 
// I use this for my ADUC832v4 board

// Resources used:
// - Serial Programming Guide for POSIX Operating systems
//   https://www.msweet.org/serial/serial.html
// - AN-1074: Understanding the Serial Download Protocol
//   https://www.analog.com/en/resources/app-notes/an-1074.html


//~ Libraries
#include <termios.h>

#include "base/base.h"
#include "base/base.c"

//~ Types
enum command_kind
{
 Command_None,
 Command_Interrogate,
 Command_EraseAll,
 Command_Program,
 Command_Run,
};
typedef enum command_kind command_kind;

typedef struct command command;
struct command
{
 command_kind Kind;
 
 u8 AddressH;
 u8 AddressL;
 
 u64 BufferCount;
 u8 Buffer[256];
 
 u8 CheckSum;
 
 b32 Written;
};

//~ Globals

// Constants
read_only global_variable command Cmd_EraseAll = {.Kind = Command_EraseAll, .CheckSum = 0xBE};
read_only global_variable command Cmd_Run = {.Kind = Command_Run, .CheckSum = 0xA7};
read_only global_variable command Cmd_Interrogate = {.Kind = Command_Interrogate};

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
 Result = H*0x10 + L;
 
 return Result;
}

//~ Entry point
raddbg_entry_point(EntryPoint);
ENTRY_POINT(EntryPoint)
{
 if(LaneIndex() == 0)
 {
  int TerminalHandle = 0;
  char *DeviceFileName = "/dev/ttyUSB0";
  char *HexFileName = 0;
  
  cmd_line *CmdLine = &Params->CmdLine;
  
  {  
   cmd_line_entry *Entry = CmdLineGetEntry(CmdLine, S8("DeviceFile"));
   if(!IsNilCmdLineEntry(Entry))
   {
    DeviceFileName = CSS8(Entry->First->V);
   }
   HexFileName = CSS8(CmdLine->FirstFile->V);
  }
  
  TerminalHandle = open(DeviceFileName, O_RDWR | O_NOCTTY | O_NDELAY);
  
  // TODO(luca): We have to check that this file is not owned/opened/being read from by
  // another process.  So we can notify the user.
  
  if(TerminalHandle != -1)
  {
   int Ret = fcntl(TerminalHandle, F_SETFL, 0);
   if(Ret != -1)
   {
    Ret = fcntl(TerminalHandle, F_SETFL, FNDELAY);
    
    // From the docs loader should be in 9600 baud, 8 data bits, and no parity.
    
    struct termios Options;
    Ret = tcgetattr(TerminalHandle, &Options);
    PrintErrnoIf(Ret != -1);
    
    // TODO(luca): Baud Rate = 9600 Baud × Crystal Freq/11.0592 MHz
    // What if crystal speed changed?
    
    Ret = cfsetispeed(&Options, B9600);
    PrintErrnoIf(Ret != -1);
    
    Ret = cfsetospeed(&Options, B9600);
    PrintErrnoIf(Ret != -1);
    
    // NOTE(luca): Never set directly because of compatibility
    {
     // NOCTTY equivalent
     Options.c_cflag |= (CLOCAL | CREAD);
     
     // Set 8 data bits, No parity
     Options.c_cflag &= (tcflag_t)~PARENB;  // Disable Parity
     Options.c_cflag &= (tcflag_t)~CSTOPB;  // 1 stop bit (Not 2 bits)
     Options.c_cflag &= (tcflag_t)~CSIZE;   // Clear data bits size
     Options.c_cflag |= CS8;                // 8 data bits
     
     // NOTE(luca): This was very important and fixed the bug where 25 bits wouldn't be read
     // when activating the loader.  Since the identity packet would send a newline it would
     // Cut right after.
     // Raw input -> Disable canonical, echoing and signals
     Options.c_lflag &= (tcflag_t)~(ICANON | ECHO | ECHOE | ISIG);
     
     // Enable parity checks and strip of parity bit afterwads
     Options.c_iflag |= (INPCK | ISTRIP);
     
     // Disable software flow control
     Options.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
     
     // Enable raw output <- Disable processed output
     Options.c_oflag &= (tcflag_t)~OPOST;
    }
    
    Ret = tcsetattr(TerminalHandle, TCSANOW, &Options);
    PrintErrnoIf(Ret != -1);
    
    if(Ret != -1)
    {
     // TODO(luca): Try ReadIdx & WriteIdx and batching commands and batching reads.
     u64 CommandIdx = 0;
     u64 CommandCount = 0;
     
#if 0     
     command Commands[] =
     {
      Cmd_Interrogate,
      Cmd_EraseAll,
      {
       .Kind = Command_Program, 
       .BufferCount = 0x09,
#if 1
       //                                  P2    Val
       .Buffer = { 0x02, 0x00, 0x03, 0x75, 0xA0, 0xAA, 0x02, 0x00, 0x03, }
#else
       //                                  P0    P2
       .Buffer = { 0x02, 0x00, 0x03, 0x85, 0x80, 0xA0, 0x02, 0x00, 0x03, },
#endif
      },
      Cmd_Run,
     };
     CommandCount = ArrayCount(Commands);
#else
     command Commands[KB(4)] = {0};
     
     str8 File = OS_ReadEntireFileIntoMemory(HexFileName);
     if(File.Size)
     {
      Commands[CommandCount] = Cmd_EraseAll;
      CommandCount += 1;
      
      b32 ParseSucceeded = true;
      
      // Parse hex file and create program commands accordingly
      {
       b32 FinishedParsing = false;
       u64 At = 0;
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
         CommandCount += 1;
         
         Command->Kind = Command_Program;
         
         Command->AddressH = AddressH;
         Command->AddressL = AddressL;
         
         // x data bytes
         for EachIndex(Idx, RecordLength)
         {
          u8 Byte = ParseHexDigitPair(File, &At, &ParseSucceeded);
          Command->Buffer[Command->BufferCount] = Byte;
          Command->BufferCount += 1;
         }
         
         // 2 bytes: checksum
         u8 CheckSum = ParseHexDigitPair(File, &At, &ParseSucceeded);
         
         // TODO(luca): Verify checksum?
         //Command->CheckSum = CheckSum;
         
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
       
      }
      
      if(ParseSucceeded)
      {
       Commands[CommandCount] = (command){.Kind = Command_Run, .CheckSum = 0xA7 };
       CommandCount += 1;
      }
      else
      {
       Log("Error: Invalid hex file '%s'.\n", HexFileName);
       CommandCount = 0;
      }
     }
     else
     {
      Log("Error: Could not read file '%s'\n", HexFileName);
     }
#endif
     
     b32 ShouldQuit = false;
     while(!ShouldQuit && (CommandIdx < CommandCount))
     {
      command *Command = Commands + CommandIdx;
      
      //- Writing  
      {
       if(!Command->Written)
       {          
        u64 PacketSize = 0;
        u8 Packet_[KB(64)] = {0};
        u8 *Packet = Packet_;
        
        b32 DoCheckSum = true;
        u8 HardCodedCheckSum = Command->CheckSum;
        
        switch(Command->Kind)
        {
         default: NotImplemented(); break;
         
         case Command_Interrogate:
         {
          PacketSize = 4;
          
          u8 Buf[] = { 0x21, 0x5A, 0x00, 0xA6 };
          MemoryCopy(Packet, Buf, sizeof(Buf));
          
          DoCheckSum = false;
         } break;
         
         case Command_EraseAll:
         {
          PacketSize = 5;
          Packet[3] = 'A';
          
         } break;
         
         case Command_Program:
         {
          // 3: Adress bytes
          // 1: Command byte
          u64 MaxDataByteCount = 25 - 3 - 1;
          
          // NOTE(luca): We can only copy up to 24 data bytes at a time.
          if(Command->BufferCount <= MaxDataByteCount)
          {
           // 2 id bytes
           // 1 no of data bytes byte
           // 1 command byte
           // 3 adress bytes 
           // 1 checksum byte
           PacketSize = Command->BufferCount + 2 + 1 + 1 + 3 + 1;
           Packet[3] = 'W';
           
           // NOTE(luca): ADR is always 0, because we work with 16 bits and not 24 
           Packet[4] = 0; 
           Packet[5] = Command->AddressH;
           Packet[6] = Command->AddressL;
           
           MemoryCopy(Packet + 7, Command->Buffer, Command->BufferCount);
          }
          
         } break;
         
         case Command_Run:
         {
          PacketSize = 8;
          Packet[3] = 'U';
         } break;
        }
        
        if(DoCheckSum)
        {
         Packet[0] = 0x07;
         Packet[1] = 0x0E;
         // NOTE(luca): 2 ID bytes + 1 No. of data Bytes byte and checksum byte
         Packet[2] = (u8)(PacketSize - 4);
         
         u8 *CheckSum = Packet + (PacketSize - 1);
         u64 Skip = 2;
         u8 Sum = 0;
         for(u64 Idx = Skip; Idx < PacketSize - 1; Idx += 1)
         {
          Sum += Packet[Idx];
         }
         CheckSum[0] = 0 - Sum;
         
         if(HardCodedCheckSum != 0)
         {
          Assert(HardCodedCheckSum == CheckSum[0]);
         }
        }
        
        smm BytesWritten = write(TerminalHandle, Packet, PacketSize);
        if(BytesWritten == (smm)PacketSize)
        {
         Command->Written = true;
         // TODO(luca): Show command name 
         Log("%lld bytes written.\n", BytesWritten);
        }
        else
        {
         PrintErrnoIf(0);
        }
       }
      }
      
      //- Reading 
      {
       if(Command->Written)
       {        
        u8 Buffer[KB(4)] = {0};
        
        smm BytesRead = read(TerminalHandle, Buffer, sizeof(Buffer));
        if(BytesRead > 0)
        {
         Log("%lld bytes read.\n", BytesRead);
         
         // NOTE(luca): Assume the command was read so we can bump the idx.
         CommandIdx += 1;
         
         switch(Command->Kind)
         {
          default: NotImplemented(); break;
          
          case Command_Interrogate:
          {
           if(BytesRead == 25)
           {
            Log("Loader connected.\n");
           }
           else
           {
            Log("Error: Not 25 bytes read.\n");
            ShouldQuit = true;
           }
           
          } break;
          
          case Command_EraseAll:
          case Command_Program:
          case Command_Run:
          {
           if(BytesRead == 1)
           {
            if(Buffer[0] == 7)
            {
             Log("Error: NAK.\n");
             DebugBreak();
             ShouldQuit = true;
            }
            if(Buffer[0] == 6)
            {
             Log("Success: ACK.\n");
            }
           }
           else
           {
            Log("Error: Too many bytes, expected 1.\n");
            ShouldQuit = true;
           }
           
          } break;
         }
         
         
        }
        else if(BytesRead == 0)
        {
         // NOTE(luca): Nothing to read
        }
        else
        {
         //Log(ERRNO_FMT "\n", ERRNO_ARG);
        }
        
       }
      }
      
     }
     
     
    }
    else
    {
     PrintErrnoIf(0);
    }
   }
   else
   {
    PrintErrnoIf(0);
   }
  }
  else
  {
   PrintErrnoIf(0);
  }
  
 }
 return 0;
}