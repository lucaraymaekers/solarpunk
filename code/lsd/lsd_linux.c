#include <sys/ioctl.h>
#include <termios.h>

internal lsd_port_handle
LSD_OpenPort(str8 DeviceFileName)
{
 lsd_port_handle Result = {0};
 
 s32 TerminalHandle = -1;
 
 // NOTE(luca): Sometimes it will open up on /dev/ttyUSB1 if /dev/ttyUSB0 was in use
 // while plugging in the board.  In those cases, we search for ttyUSB devices in /dev.
 // NOTE(luca): Since there is a hard limit on max serial devices we could also enumerate them (https://docs.kernel.org/usb/usb-serial.html)
 if(DeviceFileName.Size == 0)
 {
  os_dir *Dir = OS_WalkDirPush(S8("/dev"), 0);
  os_dir_entry *Entry;
  while((Entry = OS_WalkDirGetNext(Dir)))
  {
   if(S8Match(Entry->Name, S8("ttyUSB"), true))
   {
    DeviceFileName = Str8Fmt("/dev/%S", Entry->Name);
    
    break;
   }
  }
  
  // NOTE(luca): This is for better error message
  if(DeviceFileName.Size == 0)
  {
   DeviceFileName = S8("/dev/ttyUSB0");
  }
  
  TerminalHandle = open(CSS8(DeviceFileName), O_RDWR | O_NOCTTY | O_NDELAY);
 }
 else
 {
  TerminalHandle = open(CSS8(DeviceFileName), O_RDWR | O_NOCTTY | O_NDELAY);
 }
 
 if(TerminalHandle != -1)
 {
  int Ret;
  
  arena *Arena = GetScratch();
  
  struct termios *Options = PushArrayZero(Arena, struct termios, 1);;
  
  Prof_TimeScope("Terminal options")
  {
   {
    // NOTE(luca): goto here acts as a scope exit/early return
    Ret = fcntl(TerminalHandle, F_SETFL, 0);
    if(Ret == -1) goto TerminalOpenFail;
    
    Ret = fcntl(TerminalHandle, F_SETFL, FNDELAY);
    if(Ret == -1) goto TerminalOpenFail;
    
    Ret = tcgetattr(TerminalHandle, Options);
    if(Ret == -1) goto TerminalOpenFail;
    
    // NOTE(luca): Never set directly because of compatibility
    // From the docs loader should be in 9600 baud, 8 data bits, and no parity.
    {
     // NOCTTY equivalent
     Options->c_cflag |= (CLOCAL | CREAD);
     
     // Set 8 data bits, No parity
     Options->c_cflag &= (tcflag_t)~PARENB;  // Disable Parity
     Options->c_cflag &= (tcflag_t)~CSTOPB;  // 1 stop bit (Not 2 bits)
     Options->c_cflag &= (tcflag_t)~CSIZE;   // Clear data bits size
     Options->c_cflag |= CS8;                // 8 data bits
     
     // NOTE(luca): This was very important and fixed the bug where 25 bits wouldn't be read
     // when activating the loader.  Since the identity packet would send a newline it would
     // Cut right after.
     // Raw input -> Disable canonical, echoing and signals
     Options->c_lflag &= (tcflag_t)~(ICANON | ECHO | ECHOE | ISIG);
     
     // Disable parity checks and strip of it
     Options->c_iflag &= (tcflag_t)~(INPCK | ISTRIP);
     
     // Disable software flow control
     Options->c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
     
     // NOTE(luca): When interrogating the ADuC board it will send /r/n in its response,
     // its important for the checksum that this does not get translated.
     Options->c_iflag &= (tcflag_t)~(ICRNL | INLCR | IGNCR);
     
     // Enable raw output <- Disable processed output
     Options->c_oflag &= (tcflag_t)~OPOST;
     
     Options->c_cc[VMIN]  = 1;
     Options->c_cc[VTIME] = 0;
    }
    
    Ret = cfsetispeed(Options, B9600);
    if(Ret == -1) goto TerminalOpenFail;
    
    Ret = cfsetospeed(Options, B9600);
    if(Ret == -1) goto TerminalOpenFail;
    
    Ret = tcsetattr(TerminalHandle, TCSANOW, Options);
    if(Ret == -1) goto TerminalOpenFail;
    
    // NOTE(luca): All options have been (sucessfully ?) set, now try to see if it is opened
    // already
    
    Ret = ioctl(TerminalHandle, TIOCEXCL);
    if(Ret == -1) goto TerminalOpenFail;
    
    Result.Valid = true;
    Result.Device = (u64)TerminalHandle;
    Result.Options = (u64)Options;
   } TerminalOpenFail:;
   
   if(Ret == -1)
   {
    Log("Could not set options for port.\n");
    PrintErrnoIf(GlobalVerbose);
   }
  }
 }
 else
 {
  PrintErrnoIf(GlobalVerbose);
  str8 HelpText = S8("make sure device is present");
  if(0) {}
  if(errno == EACCES) HelpText = S8("make sure you have access permission");
  if(errno == EBUSY)  HelpText = S8("device is already opened by another program.");
  
  Log("Could not open '%S', %S.\n", DeviceFileName, HelpText);
 }
 
 return Result;
}


internal lsd_rw_result
LSD_Read(lsd_port_handle Handle, u8 *Buffer, u64 BufferSize)
{
 lsd_rw_result Result = {0};
 
 s32 TerminalHandle = (s32)Handle.Device;
 
 s64 BytesRead = read(TerminalHandle, Buffer, BufferSize);
 
 if(BytesRead == -1)
 {
  AssertErrno(errno == EAGAIN);
 }
 else
 {
  Result.Valid = true;
  Result.Count = (u64)BytesRead;
 }
 
 return Result;
}

internal lsd_rw_result
LSD_Write(lsd_port_handle Handle, u8 *Buffer, u64 BufferSize)
{
 lsd_rw_result Result = {0};
 
 s32 TerminalHandle = (s32)Handle.Device;
 
 s64 BytesWritten = write(TerminalHandle, Buffer, BufferSize);
 if(BytesWritten != -1)
 {
  Result.Valid = true;
  Result.Count = (u64)BytesWritten;
 }
 
 return Result;
}

internal void
LSD_SetBaudRate(lsd_port_handle Handle, s32 Baud)
{
 s32 TerminalHandle = (s32)Handle.Device;
 struct termios *Options = (struct termios *)Handle.Options;
 
 speed_t Speed = {0};
 if(0) {}
 else if(Baud == 9600)   Speed = B9600;
 else if(Baud == 230400) Speed = B230400;
 else if(Baud == 115200) Speed = B115200;
 else if(Baud == 57600)  Speed = B57600;
 else InvalidPath();
 
 int Ret = 0;
 
 Ret = cfsetispeed(Options, Speed);
 if(Ret == -1) goto OptionSetFail;
 Ret = cfsetospeed(Options, Speed);
 if(Ret == -1) goto OptionSetFail;
 Ret = tcsetattr(TerminalHandle, TCSANOW, Options);
 if(Ret == -1) goto OptionSetFail;
 
 OptionSetFail:
 
 if(Ret == -1)
 {
  Log("Error: Could not set baud rate.\n");
  PrintErrnoIf(GlobalVerbose);
 }
}