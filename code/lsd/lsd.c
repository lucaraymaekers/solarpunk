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
 
 u8 AddressL;
 u8 AddressH;
 
 u64 BufferCount;
 u8 Buffer[256];
 
 b32 Written;
};

//~ Entry point
raddbg_entry_point(EntryPoint);
ENTRY_POINT(EntryPoint)
{
 if(LaneIndex() == 0)
 {
  int File = 0;
  char *FileName = "/dev/ttyUSB0";
  
  File = open(FileName, O_RDWR | O_NOCTTY | O_NDELAY);
  
  // TODO(luca): We have to check that this file is not owned/opened/being read from by
  // another process.  So we can notify the user.
  
  if(File != -1)
  {
   int Ret = fcntl(File, F_SETFL, 0);
   if(Ret != -1)
   {
    Ret = fcntl(File, F_SETFL, FNDELAY);
    
    // From the docs loader should be in 9600 baud, 8 data bits, and no parity.
    
    struct termios Options;
    Ret = tcgetattr(File, &Options);
    AssertErrno(Ret != -1);
    
    // TODO(luca): Baud Rate = 9600 Baud × Crystal Freq/11.0592 MHz
    // What if crystal speed changed?
    
    Ret = cfsetispeed(&Options, B9600);
    AssertErrno(Ret != -1);
    
    Ret = cfsetospeed(&Options, B9600);
    AssertErrno(Ret != -1);
    
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
    
    Ret = tcsetattr(File, TCSANOW, &Options);
    AssertErrno(Ret != -1);
    
    if(Ret != -1)
    {
     // TODO(luca): Try ReadIdx & WriteIdx and batching commands and batching reads.
     u64 CommandIdx = 0;
     command Commands[] =
     {
      //{Command_Interrogate},
      {Command_EraseAll},
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
      {Command_Run},
     };
     u64 CommandCount = ArrayCount(Commands);
     
     b32 ShouldQuit = false;
     while(!ShouldQuit)
     {
      command *Command = Commands + CommandIdx;
      if(CommandIdx < CommandCount)
      {
       //- Writing  
       {
        if(!Command->Written)
        {          
         u64 PacketSize = 0;
         u8 Packet_[KB(64)] = {0};
         u8 *Packet = Packet_;
         
         b32 DoCheckSum = true;
         u8 HardCodedCheckSum = 0;
         
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
           HardCodedCheckSum = 0xBE;
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
            Packet[5] = Command->AddressL;
            Packet[6] = Command->AddressH;
            
            MemoryCopy(Packet + 7, Command->Buffer, Command->BufferCount);
           }
           
          } break;
          
          case Command_Run:
          {
           PacketSize = 8;
           Packet[3] = 'U';
           HardCodedCheckSum = 0xA7;
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
         
         smm BytesWritten = write(File, Packet, PacketSize);
         if(BytesWritten == (smm)PacketSize)
         {
          Command->Written = true;
          // TODO(luca): Show command name 
          Log("%lld bytes written.\n", BytesWritten);
         }
         else
         {
          AssertErrno(0);
         }
        }
       }
       
       //- Reading 
       {
        if(Command->Written)
        {        
         u8 Buffer[KB(4)] = {0};
         
         smm BytesRead = read(File, Buffer, sizeof(Buffer));
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
      
      // NOTE(luca): This is a delay because I don't know if it's a good idea to have none.
     }
     
     
    }
    else
    {
     AssertErrno(0);
    }
   }
   else
   {
    AssertErrno(0);
   }
  }
  else
  {
   AssertErrno(0);
  }
  
  close(File);
  
 }
 return 0;
}