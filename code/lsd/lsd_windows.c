internal lsd_port_handle
LSD_OpenPort(str8 DeviceFileName)
{
 lsd_port_handle Result = {0};
 
 HANDLE Port = INVALID_HANDLE_VALUE;
 
 if(DeviceFileName.Size == 0)
 {
  // NOTE(luca): If no device name was provided, enumerate from high port number backwards
  // and use the first valid one found.
  s32 Idx = 16; // arbitrary
  while(Port == INVALID_HANDLE_VALUE &&
        Idx > 0)
  {
   DeviceFileName = Str8Fmt("\\\\.\\COM%d", Idx);
   Port = CreateFile(CSS8(DeviceFileName),
                     GENERIC_READ|GENERIC_WRITE,
                     0,
                     0,
                     OPEN_EXISTING,
                     0,
                     NULL);
   Idx -= 1;
  }
 }
 else
 {
  Port = CreateFile(CSS8(Str8Fmt("\\\\.\\COM%S", DeviceFileName)),
                    GENERIC_READ|GENERIC_WRITE,
                    0,
                    0,
                    OPEN_EXISTING,
                    0,
                    NULL);
 }
 
 if(Port != INVALID_HANDLE_VALUE)
 {
  arena *Arena = GetScratch();
  DCB *Options = PushArrayZero(Arena, DCB, 1);
  
  *Options = 
  (DCB){
   .DCBlength = sizeof(*Options),
   .BaudRate = CBR_9600,
   .ByteSize = 8,
   .Parity = NOPARITY,
   .fBinary = true,
   .StopBits = ONESTOPBIT,
   .fDsrSensitivity = false,
   .fOutxCtsFlow = false,
   .fOutxDsrFlow = false,
   .fDtrControl = DTR_CONTROL_DISABLE,
   .fRtsControl = RTS_CONTROL_DISABLE
  };
  
  COMMTIMEOUTS Timeouts = 
  {
   .ReadIntervalTimeout = MAXDWORD,
   .ReadTotalTimeoutMultiplier = 0,
   .ReadTotalTimeoutConstant = 0,
   .WriteTotalTimeoutMultiplier = 0,
   .WriteTotalTimeoutConstant = 0,
  };
  
  if(SetCommState(Port, Options))
  {
   if(SetCommTimeouts(Port, &Timeouts))
   {
    PurgeComm(Port, PURGE_RXCLEAR | PURGE_TXCLEAR);
    
    Result.Device = (u64)Port;
    Result.Options = (u64)Options;
    Result.Valid = true;
   }
   else
   {
    Log("Error: Could not configure timeouts.\n");
   }
  }
  else
  {
   Log("Error: Could not set terminal options.\n");
  }
 }
 else
 {
  if(GlobalVerbose) Win32Log();
  Log("Error: Could not open '%S', make sure device is present.\n", DeviceFileName);
 }
 
 return Result;
}

internal void
LSD_SetBaudRate(lsd_port_handle Handle, s32 Baud)
{
 HANDLE Port = (HANDLE)Handle.Device;
 DCB *Options = (DCB *)Handle.Options;
 
 Options->BaudRate = Baud;
 if(!SetCommState(Port, Options))
 {
  Log("Error: Failed to set baud rate.\n");
  if(GlobalVerbose) Win32Log();
 }
}

internal lsd_rw_result
LSD_Read(lsd_port_handle Handle, u8 *Buffer, u64 BufferSize)
{
 lsd_rw_result Result = {0};
 HANDLE Port = (HANDLE)Handle.Device;
 
 DWORD BytesRead = 0;
 if(ReadFile(Port, Buffer, (DWORD)BufferSize, &BytesRead, 0))
 {
  Result.Count = (u64)BytesRead;
  Result.Valid = true;
 }
 
 return Result;
}

internal lsd_rw_result
LSD_Write(lsd_port_handle Handle, u8 *Buffer, u64 BufferSize)
{
 lsd_rw_result Result = {0};
 HANDLE Port = (HANDLE)Handle.Device;
 
 DWORD BytesWritten = 0;
 if(WriteFile(Port, Buffer, (DWORD)BufferSize, &BytesWritten, 0))
 {
  Result.Count = (u64)BytesWritten;
  Result.Valid = true;
 }
 
 return Result;
}