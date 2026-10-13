// NOTE(luca): This program is a Serial Downloader for Linux 
// I use this for my ADUC832v4 board

// Resources used:
// - Serial Programming Guide for POSIX Operating systems
//   https://www.msweet.org/serial/serial.html
// - AN-1074: Understanding the Serial Download Protocol
//   https://www.analog.com/en/resources/app-notes/an-1074.html
// - Data sheet for ADuC832
//   https://www.analog.com/media/en/technical-documentation/data-sheets/ADuC832.pdf

// TODO:
// - Reach optimal transfer speed
//   1. Raw throughput test: Write all the time for x amount of time
//   2. Profile code
//   3. Solve?
// - Abort command boolean that makes it not write the command, in the case of ChangeBaud
//   if it is not supported by the PC.
// x ExpectedReturnBytes value, so we can have 0 for the None command, 25 bytes for interrogate,
//   etc.
// - PacketSize inferred from BufferCount, exception for interrogate
// - B115200 should be checked if defined first.
// - Validate device id 


//~ Libraries
#include <termios.h>

#include "base/base.h"
#include "base/base.c"

#include "lsd/generated/lsd.meta.c"

//#include "compenhance/haversine_processor/profiler.c"

//~ Types
typedef struct command command;
struct command
{
 command_kind Kind;
 
 u64 BufferCount;
 u8 Buffer[512];
 u8 CheckSum;
 s32 Baud;
 
 u8 ExpectedBytesReturned;
 b32 Written;
};

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
};

// NOTE(luca): See table 44 in datasheet for AduC832
read_only global_variable command Command_SetFastBaud = 
{
 .Kind = CommandKind_ChangeBaud, 
 //                           T3CON T3FD
 .BufferCount = 2, .Buffer = {0x82, 0x09},
 .Baud = 115200,
};
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
    PrintErrnoIf(Ret == -1);
    
    // TODO(luca): Baud Rate = 9600 Baud × Crystal Freq/11.0592 MHz
    // What if crystal speed changed?
    
    Ret = cfsetispeed(&Options, B9600);
    PrintErrnoIf(Ret == -1);
    Ret = cfsetospeed(&Options, B9600);
    PrintErrnoIf(Ret == -1);
    
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
     
     // Disable parity checks and strip of it
     Options.c_iflag &= (tcflag_t)~(INPCK | ISTRIP);
     
     // Disable software flow control
     Options.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
     
     // Enable raw output <- Disable processed output
     Options.c_oflag &= (tcflag_t)~OPOST;
    }
    
    Ret = tcsetattr(TerminalHandle, TCSANOW, &Options);
    PrintErrnoIf(Ret == -1);
    
    if(Ret != -1)
    {
     // TODO(luca): Try ReadIdx & WriteIdx and batching commands and batching reads.
     u64 CommandIdx = 0;
     u64 CommandCount = 0;
     
     command Commands_[KB(4)] = {0};
     command *Commands = Commands_;
     
     Commands[CommandCount] = Command_Interrogate;
     CommandCount += 1;
     
     str8 File = {0};
     if(HexFileName)
     {
      File = OS_ReadEntireFileIntoMemory(HexFileName);
      if(File.Size)
      {
       Commands[CommandCount] = Command_EraseAll;
       CommandCount += 1;
       
       Commands[CommandCount] = Command_SetFastBaud;
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
          
          Command->Kind = CommandKind_Program;
          
          Command->Buffer[0] = 0; // Not used, ADuC832 uses 16 bit adresses. 
          Command->Buffer[1] = AddressH;
          Command->Buffer[2] = AddressL;
          Command->BufferCount += 3;
          
          // x data bytes
          for EachIndex(Idx, RecordLength)
          {
           u8 Byte = ParseHexDigitPair(File, &At, &ParseSucceeded);
           Command->Buffer[Command->BufferCount] = Byte;
           Command->BufferCount += 1;
          }
          
          // 2 bytes: checksum
          Command->CheckSum = ParseHexDigitPair(File, &At, &ParseSucceeded);
          
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
       
       Commands[CommandCount] = Command_SetSlowBaud;
       CommandCount += 1;
       
       if(ParseSucceeded)
       {
        Commands[CommandCount] = Command_Run;
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
     }
     
#if 0
     Commands[CommandCount] = Command_EraseAll;
     CommandCount += 1;
     
     Commands[CommandCount] = 
     (command){
      .Kind = CommandKind_Program, 
      .BufferCount = 12,
#if 1
      //                                                    P2    Val
      .Buffer = { 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x75, 0xA0, 0xAA, 0x02, 0x00, 0x03, }
#else
      //                                                    P0    P2
      .Buffer = { 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x85, 0x80, 0xA0, 0x02, 0x00, 0x03, },
#endif
     };
     CommandCount += 1;
#endif
     
     u8 ReadBuffer_[KB(64)] = {0};
     u8 *ReadBuffer = ReadBuffer_;
     s64 ReadBufferIdx = 0;
     
     u8 WriteBuffer_[KB(64)] = {0};
     u8 *WriteBuffer = WriteBuffer_;
     
     str8 Pad = S8("               ");
     
     s64 TotalBytesSent = 0;
     b32 ShouldQuit = false;
     while(!ShouldQuit && (CommandIdx < CommandCount))
     {
      command *Command = Commands + CommandIdx;
      
      //- Writing  
      {
       if(!Command->Written)
       {          
        u8 *Packet = WriteBuffer;
        
        u64 PacketSize = Command->BufferCount;
        u8 CommandByte = 0;
        
        switch(Command->Kind)
        {
         default: NotImplemented(); break;
         
         case CommandKind_Interrogate:
         {
          Command->ExpectedBytesReturned = 25;
          MemoryCopy(Packet, Command->Buffer, Command->BufferCount);
         } break;
         
         case CommandKind_ChangeBaud:   CommandByte = 'B'; break;
         case CommandKind_EraseAll:     CommandByte = 'A'; break;
         case CommandKind_Program:      CommandByte = 'W'; break;
         case CommandKind_Run:          CommandByte = 'U'; break;
        }
        
        if(Command->ExpectedBytesReturned == 0)
        {
         // Default is expecting NAK/ACK
         Command->ExpectedBytesReturned = 1;
        }
        
        b32 IsCommand = (CommandByte != 0); 
        if(IsCommand)
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
         
         u8 CheckSum = 0;
         {
          for(u64 Idx = 2; Idx < PacketSize - 1; Idx += 1)
          {
           CheckSum += Packet[Idx];
          }
          CheckSum = 0 - CheckSum;
         }
         
         Packet[PacketSize - 1] = CheckSum;
         
         //Assert(Command->CheckSum == 0 || Command->CheckSum == CheckSum);
        }
        
        // options 
        
        // Considerations:
        // - Some things you don't want to batch, because you want to receive the ACK 
        //   before proceeding.
        //   For now the only thing I can think of is Run To User. 
        
        // # I. 
        // 1. When adding the commands compute packetsize and checksum
        // 2. When writing if we are a CommandKind_Program
        //    2.1. We loop until het last contiguous CommandKind_Program
        //    2.1. We make up our packet like this
        //
        // + It seems we will need readidx and writeidx since writing will end up way
        //   ahead of our reading
        // - Lots of code changes
        
        // # II.
        // We create a special batch command that we create when parsing the file
        // we then handle this appropriately here
        //
        // + This loop will work the same
        // - We will have to move the checksum logic up 
        
        // # III.
        
        s64 BytesWritten = write(TerminalHandle, Packet, PacketSize);
        if(BytesWritten == (s64)PacketSize)
        {
         Command->Written = true;
         Log("Command(%3lldb): %S\n", BytesWritten, CommandDescriptions[Command->Kind]);
         TotalBytesSent += BytesWritten;
        }
        else
        {
         PrintErrnoIf(true);
        }
       }
      }
      
      //- Reading 
      {
       if(Command->Written)
       {        
        s64 BytesRead = read(TerminalHandle, ReadBuffer + ReadBufferIdx, sizeof(ReadBuffer_));
        if(BytesRead != -1)
        {
         ReadBufferIdx += BytesRead;
        }
        else 
        {
         AssertErrno(errno == EAGAIN);
        }
        
        if(ReadBufferIdx == Command->ExpectedBytesReturned)
        {
         // NOTE(luca): The command response was sucessfully read so we can bump the idx.
         CommandIdx += 1;
         ReadBufferIdx = 0;
         
         switch(Command->Kind)
         {
          default: NotImplemented(); break;
          
          case CommandKind_Interrogate:
          {
           {
            // TODO(luca): Implement verification that this board is the ADuC832 because 
            // other boards are not compatible (yet).
            Log("%SLoader connected.\n", Pad);
           }
           
          } break;
          
          case CommandKind_ChangeBaud:
          case CommandKind_EraseAll:
          case CommandKind_Program:
          case CommandKind_Run:
          {
           if(BytesRead == 1)
           {
            if(ReadBuffer[0] == 7)
            {
             Log("%SFailed: NAK.\n", Pad);
             DebugBreak();
             ShouldQuit = true;
            }
            if(ReadBuffer[0] == 6)
            {
             Log("%SSuccess: ACK.\n", Pad);
             
             if(Command->Kind == CommandKind_ChangeBaud)
             {
              Log("%SSet baud rate to %d.\n", Pad, Command->Baud);
              
              speed_t Speed = {0};
              if(0) {}
              else if(Command->Baud == 9600)   Speed = B9600;
              else if(Command->Baud == 115200) Speed = B115200;
              else InvalidPath();
              
              Ret = cfsetispeed(&Options, Speed);
              PrintErrnoIf(Ret == -1);
              Ret = cfsetospeed(&Options, Speed);
              PrintErrnoIf(Ret == -1);
              Ret = tcsetattr(TerminalHandle, TCSANOW, &Options);
              PrintErrnoIf(Ret == -1);
#if 0
              Ret = tcflush(TerminalHandle, TCIOFLUSH);
              PrintErrnoIf(Ret == -1);
#endif
             }
             
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
        else
        {
         // NOTE(luca): Wait on more bytes
        }
        
       }
      }
      
     }
     
     Log("Total bytes sent: %lld.\n", TotalBytesSent);
     
    }
    else
    {
     PrintErrnoIf(true);
    }
   }
   else
   {
    PrintErrnoIf(true);
   }
  }
  else
  {
   PrintErrnoIf(true);
  }
  
 }
 return 0;
}