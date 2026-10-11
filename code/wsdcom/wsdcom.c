/*
THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES INC. ``AS IS'' AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT, ARE 
DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES INC. BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
*/

// NOTE(luca): I have modified this code to fit my needs.
// - Ported to c
// - Added pragma for libraries
// - Change terminal output in favor of line prints

// TODO(luca):
// x Build with cling
// x Find configuration folder as path next to exe
// - Remove standard libraries and replace it with base layer ?

#pragma comment(lib, "User32.lib")

#include <windows.h>
#include <stdio.h>   //for reading hex file
#include <string.h>  //for copying parameter strings
#include <stdlib.h>  //for exit() function
#include <time.h>    //for clock() and clock_t (timeouts)
#include <ctype.h>   //for toupper() function
#include <conio.h>   //for reading/writing the UART hardware ports

#define BASE_NO_ENTRYPOINT 1
#include "base/base.h"
#include "base/base.c"

// NOTE(luca): If you want to log output to the command line
//#define LOGGING_ENABLED 1

#include "wsdcom/wsdcom.h"

//~ Constants            
#define TIMEOUT         1
#define DEFAULTTIMEOUT  250  //about a quarter of a second
#define ERASETIMEOUT    4000   //about 4 seconds
#define VER             " 7.05 "
#define DATE            "03rd September, 2007  "

// The maximum flash size on the ADuC702x parts is 62K
#define MAX_HEX_DATA   0xF800

// Buad Rates
#define ADUC_BAUD_9600    9600
#define ADUC_BAUD_115200  115200

// Intel Hex constants
#define IHEX_DATA_RECORD                        0x000
#define IHEX_END_OF_FILE_RECORD                 0x001
#define IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD    0x002
#define IHEX_START_SEGMENT_ADDRESS_RECORD       0x003
#define IHEX_EXTENDED_LINEAR_ADDRESS_RECORD     0x004
#define IHEX_START_LINEAR_ADDRESS_RECORD        0x005
#define IHEX_COLON_POS                          0x000
#define IHEX_MAX_LENGTH                         0x200

#define IHEX_MAX_WRITE_LINE_DATA                0x10

#define OK                                      0x0

// Responses
#define OP_OK                                   0x0
#define NOT_RESET                               0x1
#define TGT_NOT_COMPT                           0x2
#define FAILED_TO_DO                            0x3
#define FILE_NOT_FOUND                          0x4
#define INVALID_FILE                            0x5
#define OP_FAILED                               0x6
#define INVALID_CSUM                            0x7

#define Success                                         0  
#define Too_many_retries                                1  
#define File_error                                      2  
#define Memory_range_error                              3  
#define Communication_error                             4  
#define Erase_error                                     5  
#define Undefined_error                                 6  
#define Write_error                                     7  
#define Verify_error                                    8  
#define Protection_error                                9  

// Serial Download Protocol Definitions

// Buffer Offsets
#define OFF_ID_0                                0x0
#define OFF_ID_1                                0x1
#define OFF_LEN                                 0x2
#define OFF_CMD                                 0x3

#define PROT_OVHEAD                             0x4 // Two id, len pls CS bytes
#define PROT_START_ID_0                         0x7
#define PROT_START_ID_1                         0xE

#define PROT_ACK                                0x6 // Positive response
#define PROT_NACK                               0x7 // Negative response


/* Function Prototypes*/
int  CommOpen
(
 int               ComPort,
 long              Baudrate
 );
void CommClose
(
 void
 );
b32 ChangeBaudrate
(
 unsigned long     Baudrate,
 TyCmdLineOpts  *  pCmdOpts,
 TyCfgFileOpt   *  pCfg,
 TyBaudRates    *  pBauds
 );
b32 DoBootloadOption
(
 TyCmdLineOpts  *  pCmdOpts
 );
b32 DoSecurity
(
 TyCmdLineOpts  * pCmdOpts
 );
b32 UpdateCfgData
(
 char        *     pszPart,
 TyCfgFileOpt   *  pCfg
 );
b32 DoSendSimpleCmd
(
 unsigned char *  pCmd,
 TyCmdLineOpts  * pCmdOpts
 );
void showInstructions
(
 void
 );
int  validateParameters
(
 TyCmdLineOpts  *  pCmdOpts,
 int            *  pError
 );
void UpdateCS
(
 unsigned char *pucCmd
 );

b32 ReadChar
(
 unsigned char *   ch
 );
int  readRecordFromFile
(
 FILE *            FptrIn, 
 unsigned char *   Record
 );
int  readBlockFromFile
(
 FILE *            FptrIn, 
 unsigned long *   addr,
 int *             len, 
 unsigned char *   data
 );
int  CheckHexFile
(
 char *            program,
 int  *            piNumLines,
 TyCmdLineOpts  *  pCmdOpts
 );
void ReadHexBlock
(
 char *            pcHexLn,
 unsigned long *   addr,
 unsigned int *    len,
 unsigned char *   data
 );
int  sendPacket
(
 unsigned char *   message, 
 int               length,
 TyCmdLineOpts  *  pCmdOpts
 
 );
int  sendLoaderPacket
(
 int               len, 
 char *            data,
 TyCmdLineOpts  *  pCmdOpts
 );
int  waitForAck
(
 int               iTimeoutMS
 );
int  waitForSignature
(
 char *            pszSignature, 
 int *             piMajor, 
 int *             piMinor,
 char *            pcVHi,
 char *            pcVMid,
 char *            pcVLo
 );
b32  EraseFlash
(
 TyCmdLineOpts  *  pCmdOpts
 );
int  downloadRev2
(
 TyCfgFileOpt   *  pCfg,
 TyCmdLineOpts  *  pCmdOpts
 );

int  downloadRev2QuickMode
(
 TyCfgFileOpt   *  pCfg,
 TyCmdLineOpts  *  pCmdOpts
 );
b32  DoDownloadCodeFlash
(
 TyCfgFileOpt   *  pCfg,
 TyCmdLineOpts  *  pCmdOpts
 );

b32 DoDownloadDataFlash
(
 TyCmdLineOpts  *  pCmdOpts
 );

int  DoRun
(
 TyCmdLineOpts  *  pCmdOpts
 );
int  ReadByteBufferTimed
(
 unsigned char *   InputData,
 int               NumBytes,
 int               timeout
 );
int  VerifyAfterDownload
(
 char *            program,
 TyCmdLineOpts *   pCmdOpts
 );
b32  verifyfile
(
 TyCmdLineOpts *   pCmdOpts
 );

void UpdateTxt
(
 char *         pszText,
 SHORT          XOffset,
 SHORT          YOffset,
 SHORT          FieldLen
 );

char * FieldCrop
(
 const char *   pszFileName,
 int      iFldLen
 );

//~ Global Variables
global_variable HANDLE hCom = INVALID_HANDLE_VALUE;        // Handle to the serial port

// Hex file contents
global_variable unsigned char ucHexFileData[MAX_HEX_DATA];
global_variable unsigned long ulNumHexBytes = 0x0;

global_variable char                szCurOpText        [0x80];
global_variable char                szCurStatText      [0x80];
global_variable char                szCurStageText     [0x80];

global_variable char *StageStrings_[] = 
{
 "Connect", "Erase", "Download", "Verify", "Bootload", "Secure", "Run", "Complete"
};

#if 0
0         1         2         3         4         5         6         7
01234567890123456789012345678901234567890123456789012345678901234567890123456789
0 Download.exe: ADuC8xx Program Downloader Ver 4.03
1 03rd May 2006, Analog Devices Inc.
2
3 ==========================================================================
4 <Connect> <Erase> <Download> <Verify> <Bootload> <Secure> <Run> <Complete>
5 ==========================================================================
6 
7 Comms      :                          Bootload   :
8 Part       :                          Erase Data :
9 Crystal    :                          Run Adr.   :
0 %Baud Er.  :                          Verify     :
1 Dwnlod Md. :                          Ver. Files :
2 XMit Size  :
3 Code File  :
4 Data File  :
5
6 Stage      :
7
8 Operation  :
9 Status     :
0 
1 Download Completed Successfully
#endif

// Function     : UpdateTxt
// Description  : Update the field
void UpdateTxt(char *Text,SHORT XOffset,SHORT YOffset,SHORT FieldLen)
{
 // NOTE(luca): If we want to log.
#ifdef LOGGING_ENABLED
 Log("%s\n", Text);
#endif
}

// Function     : SetStage
// Description  : Set the stage indicator
void SetStage(int Stage)
{
 char *String = (char *)Str8Fmt("Stage: %s", StageStrings_[Stage]).Data;
 UpdateTxt(String, 0x0, CUR_STAGE_IND, 0);
}

// Function     : UpdateCurOpTxt
// Description  : Update the field
void UpdateCurOpTxt(char * pszText)
{
 strncpy(szCurOpText,pszText,sizeof(szCurOpText));
 szCurOpText[sizeof(szCurOpText)-1] = '\0';
 UpdateTxt(pszText, COL_1_X_FLD_ST, CUR_OP_Y, 0);
}
// Function     : UpdateCurStatTxt
// Description  : Update the field
void UpdateCurStatTxt(char * pszText)
{
 strncpy(szCurStatText,pszText,sizeof(szCurStatText));
 szCurStatText[sizeof(szCurStatText)-1] = '\0';
 UpdateTxt(pszText, COL_1_X_FLD_ST, CUR_STAT_Y, 0);
}
// Function     : UpdateCurStageTxt
// Description  : Update the field
void UpdateCurStageTxt(char * pszText)
{
 strncpy(szCurStageText,pszText,sizeof(szCurStageText));
 szCurStageText[sizeof(szCurStageText)-1] = '\0';
 UpdateTxt(pszText, COL_1_X_FLD_ST, CUR_STAGE_Y,  0);
}

// Function     : UpdateScreenInfo
// Description  : Update some fields on the screen
void UpdateScreenInfo(TyCmdLineOpts *CmdOpts,TyBaudRates *Bauds)
{
 char           szMessage[0x80];
 unsigned long  ulDiff;
 double         Percentage ;
 
 UpdateTxt(FieldCrop(CmdOpts->szCodeHexF,COL_1_FLD_LEN_EX),COL_1_X_FLD_ST,CODE_Y,COL_1_FLD_LEN_EX);
 UpdateTxt(FieldCrop(CmdOpts->szDataHexF,COL_1_FLD_LEN_EX),COL_1_X_FLD_ST,DATA_Y,COL_1_FLD_LEN_EX);
 
 UpdateTxt(CmdOpts->bBootLoad ? "true":"false",
           COL_2_X_FLD_ST,BOOTLOAD_Y,
           COL_2_FLD_LEN);
 
 UpdateTxt(CmdOpts->bEraseData ? "true":"false",
           COL_2_X_FLD_ST,ERASE_Y,
           COL_2_FLD_LEN);
 
 UpdateTxt(CmdOpts->bVerify ? "true":"false",
           COL_2_X_FLD_ST,VERIFY_Y,
           COL_2_FLD_LEN);
 
 strcpy(szMessage,"");
 
 if(!CmdOpts->SecurityLockMode && !CmdOpts->SecuritySecureMode && !CmdOpts->SecuritySerialSafeMode)
 {
  strcpy(szMessage,"None");
 }
 else
 {
  if(CmdOpts->SecurityLockMode)
   strcat(szMessage," (Lock)");
  
  if(CmdOpts->SecuritySecureMode) 
   strcat(szMessage," (Secure)");
  
  if(CmdOpts->SecuritySerialSafeMode)
   strcat(szMessage," (Serial Safe)");
 }
 
 
 UpdateTxt(szMessage,
           COL_2_X_FLD_ST,SECURITY_Y,
           COL_2_FLD_LEN);
 
 // Run address
 if(CmdOpts->bRun)
 {
  sprintf(szMessage,"0x%04lX",CmdOpts->ulRunAdr);
  UpdateTxt(szMessage,
            COL_2_X_FLD_ST,
            RUN_Y,
            COL_2_FLD_LEN);
 }
 else
  UpdateTxt("------",
            COL_2_X_FLD_ST,
            RUN_Y,
            COL_2_FLD_LEN);
 
 // XMit Sz
 sprintf(szMessage,"%d",CmdOpts->iXMITBytes);
 UpdateTxt(szMessage,
           COL_1_X_FLD_ST,
           XMIT_SZ_Y,
           COL_1_FLD_LEN);
 
 // Download mode
 switch(CmdOpts->DownloadMode)
 {
  case DWN_CODE_DATA_MODE:
  UpdateTxt("Code and Data",
            COL_1_X_FLD_ST,
            DWN_MD_Y,
            COL_1_FLD_LEN);
  break;
  case DWN_CODE_ONLY_MODE:
  UpdateTxt("Code Only",
            COL_1_X_FLD_ST,
            DWN_MD_Y,
            COL_1_FLD_LEN);
  break;
  case DWN_DATA_ONLY_MODE:
  UpdateTxt("Data Only",
            COL_1_X_FLD_ST,
            DWN_MD_Y,
            COL_1_FLD_LEN);
  break;
  default:
  UpdateTxt("Unknown",
            COL_1_X_FLD_ST,
            DWN_MD_Y,
            COL_1_FLD_LEN);
  break;
 }
 
 sprintf(szMessage,"%d Hz",CmdOpts->fcore);
 UpdateTxt(szMessage,COL_1_X_FLD_ST,CRYSTAL_Y,COL_1_FLD_LEN);
 
 // PC is trying to match the ADuC8xx baud. Determine the percentage error
 // and display a warning if > 3% as comms will be unreliable
 if(Bauds->PCSlowBR >  Bauds->ADSlowBR)
  ulDiff = Bauds->PCSlowBR - Bauds->ADSlowBR;
 else
  ulDiff = Bauds->ADSlowBR - Bauds->PCSlowBR;
 Percentage = (double) (ulDiff* 100)/Bauds->ADSlowBR;
 
 sprintf(szMessage,"%.02f%%",Percentage);
 UpdateTxt(szMessage,COL_1_X_FLD_ST,BAUD_ERR_Y,COL_1_FLD_LEN);
 
 
}


// Function     : InitSessionStructs
// Description  : Initialise variables used for comms and info
void InitSessionStructs(TyCfgFileOpt *CfgOpts,TyCmdLineOpts  *CmdOpts)
{
 
 memset(CfgOpts,0x0,sizeof(CfgOpts));
 memset(CmdOpts,0x0,sizeof(CmdOpts));
 
 CmdOpts->XTAL                     = 0;
 CmdOpts->szCodeHexF[0x0]          = '\0';
 CmdOpts->szDataHexF[0x0]          = '\0';
 CmdOpts->iComPort                 = 0x1;
 CmdOpts->bEraseData               = true;
 CmdOpts->bRun                     = false;
 
 CmdOpts->ulRunAdr                 = 0x00;
 
 CmdOpts->bVerify                  = false;
 CmdOpts->bConfirmSerialSafe       = true;
 CmdOpts->fcore                    = 11059200;
 CmdOpts->bBootLoad                = false;
 CmdOpts->DownloadMode             = DWN_CODE_ONLY_MODE;
 CmdOpts->SecurityLockMode         = false;       
 CmdOpts->SecuritySerialSafeMode   = false; 
 CmdOpts->SecuritySecureMode       = false;     
 CmdOpts->ulTimeOpen               = 0;
 CmdOpts->iXMITBytes               = 0x8; 
 CmdOpts->bPromptForFiles          = false;
 CmdOpts->ulHexCSUM                = 0x0;
 CmdOpts->ulActualCodeFCSum        = 0x0;
 CmdOpts->bVerifyCSUM              = false;
 CmdOpts->fcoreLimit               = 20000000;
 
 CfgOpts->BootloadOption              = true;
 CfgOpts->SecurityBitsOption          = true;
 CfgOpts->VerifyAfterDownloadOption   = true;
 CfgOpts->FastBROption                = true;
 CfgOpts->CodeSize                    = 63488;
}

// Function     : main
// Description  : Serial Downloader for Analog Devices ADuC8xx parts
int main(int argc, char *argv[])
{
 TyCfgFileOpt   CfgOpts        = {0x0}; // Part Configuration File Options
 TyCmdLineOpts  CmdOpts        = {0x0}; // Command line options
 TyBaudRates    Bauds     = {ADUC_BAUD_115200,ADUC_BAUD_9600,ADUC_BAUD_9600};
 b32           bSuccess = false;
 
 SetStringsScratch(ArenaAlloc());
 
 SetStage(CONNECT_STAGE);
 
 InitSessionStructs(&CfgOpts,&CmdOpts);
 
 if(argc == 0x1)
  showInstructions();
 
 UpdateCurStageTxt("Decoding command line");
 if(decodeCommandLine(argc,argv, &CmdOpts))
 {
  fprintf(stderr, "Uploading %s\n", CmdOpts.szCodeHexF);
  
  //bSuccess = (DoActions(&CmdOpts, &CfgOpts, &Bauds) == 0);
  {
   char           szMessage[0x80];
   b32           bSuccess = false;
   int            rMajor         = 0;
   int            rMinor         = 0;    // rev number of the target firmware
   char           sanatizedSig[100];
   DWORD          dwStartTime    = 0;
   char           cVHi = 0, cVMid = 0, cVLo = 0;
   DWORD          dwEndTime      = 0;
   DWORD          dwDnwldTime    = 0;
   DWORD          dwVerifyTime   = 0;
   int            iRetVal        = Undefined_error;
   
   UpdateCurStageTxt("Validating command line");
   if(validateParameters(&CmdOpts,&iRetVal))
   {
    UpdateCurStageTxt("Calculating baudrates to be used");
    Bauds = getCorrectBauds(&CmdOpts,0);
    
    UpdateScreenInfo(&CmdOpts, &Bauds);
    
    UpdateCurStageTxt("Connecting to ADuC8xx");
    // Reset the target device (hopefully into loader mode)
    iRetVal = Communication_error;
    bSuccess = reset(&rMajor,
                     &rMinor,
                     sanatizedSig,
                     Bauds.PCSlowBR,
                     &CmdOpts,
                     &cVHi,
                     &cVMid,
                     &cVLo,
                     2,
                     &Bauds);
    
    if(bSuccess)
    {
     bSuccess=(CommOpen(CmdOpts.iComPort, Bauds.PCSlowBR)==1);
    }
    
    if(bSuccess)
    {
     sprintf(szMessage," %s V%c.%c.%c",sanatizedSig,cVHi,cVMid,cVLo);
     UpdateTxt(szMessage,COL_1_X_FLD_ST,PART_Y,COL_1_FLD_LEN);
     UpdateCurStageTxt("Reading configuration information");
     iRetVal = File_error;
     bSuccess = bSuccess && UpdateCfgData(sanatizedSig,&CfgOpts);
     
     // A quick check to see if the user has specified security bits
     // and the connected processor does not support it
     if(bSuccess && !CfgOpts.SecurityBitsOption)
     {
      if(CmdOpts.SecurityLockMode || CmdOpts.SecuritySecureMode || CmdOpts.SecuritySerialSafeMode)
      {
       UpdateCurOpTxt("Checking security feature available");
       UpdateCurStatTxt("Security specified but unavailable.");
       iRetVal = Undefined_error;
       bSuccess = false;
      }
     }
     
     if(bSuccess)
     {
      // Now that we know which part we are targetting it may be
      // possible to switch to using faster baud rates if the ADuC8xx
      // can support this
      Bauds = getCorrectBauds(&CmdOpts,&CfgOpts);
      
      // Update crystal frequency now that we actually know
      // what it is
      sprintf(szMessage,"%d Hz",CmdOpts.fcore);
      UpdateTxt(szMessage,COL_1_X_FLD_ST,CRYSTAL_Y,COL_1_FLD_LEN);
      
      
      // Use the fastest baud rate that is supported to speed up download
      if(CfgOpts.FastBROption)
      {
       UpdateCurStageTxt("Changing to fast baudrate");
       iRetVal = Communication_error;
       bSuccess = ChangeBaudrate(Bauds.FastBR,
                                 &CmdOpts,
                                 &CfgOpts,&Bauds);
       
       // We have just changed the baud rate so it is possible that
       // we now have lost comms with the ADuC8xx. 
       
       UpdateCurStageTxt("Erasing Flash");
       SetStage(ERASE_STAGE);
       // 
       // First operation is to erase the code flash. The data flash may
       // need to be erased as well
       bSuccess = bSuccess && EraseFlash(&CmdOpts);
       
       if(!bSuccess)
       {
        // Assume now that it was due to the comms
        UpdateCurStageTxt("Changing to fast baudrate");
        UpdateCurStatTxt("Lost contact after changing the baudrate");
       }
       
      }
      else
      {
       UpdateCurStageTxt("Erasing Flash");
       SetStage(ERASE_STAGE);
       // First operation is to erase the code flash. The data flash may
       // need to be erased as well
       bSuccess = bSuccess && EraseFlash(&CmdOpts);
       iRetVal = Erase_error;
      }
     }
     
     iRetVal = Undefined_error;
     SetStage(DOWNLOAD_STAGE);
     
     // Do we need to download to code flash?
     if(bSuccess && (CmdOpts.DownloadMode != DWN_DATA_ONLY_MODE))
     {
      UpdateCurStageTxt("Downloading bytes to code flash");
      dwStartTime = GetTickCount();
      bSuccess = bSuccess && DoDownloadCodeFlash(&CfgOpts,
                                                 &CmdOpts) ;
      
      dwEndTime = GetTickCount();
      
      dwDnwldTime    = dwEndTime-dwStartTime;
      dwVerifyTime   = 0;
      
      if(bSuccess)
      {
       sprintf(szMessage,
               "Download completed in %d seconds",
               (dwEndTime-dwStartTime)/1000);
       UpdateCurStatTxt(szMessage);
       
       sprintf(szMessage,
               "%s (%d\\%d) [Csum:%d]",
               FieldCrop(CmdOpts.szCodeHexF,COL_1_FLD_LEN_EX - 25),
               dwDnwldTime/1000,
               dwVerifyTime/1000,
               CmdOpts.ulActualCodeFCSum);
       UpdateTxt(szMessage,COL_1_X_FLD_ST,CODE_Y,COL_1_FLD_LEN_EX);
       
       // Should we verify now?
       if(CmdOpts.bVerify && CfgOpts.VerifyAfterDownloadOption)
       {
        UpdateCurStageTxt("Verifying Code Flash");
        SetStage(VERIFY_STAGE);
        
        dwStartTime = GetTickCount();
        bSuccess = bSuccess && verifyfile(&CmdOpts);
        dwEndTime = GetTickCount();
        
        if(bSuccess)
        {
         dwVerifyTime    = dwEndTime-dwStartTime;
         
         sprintf(szMessage,
                 "Verify completed in %d seconds",
                 (dwEndTime-dwStartTime)/1000);
         UpdateCurStatTxt(szMessage);
         sprintf(szMessage,
                 "%s (%d\\%d) [Csum:%d]",
                 FieldCrop(CmdOpts.szCodeHexF,COL_1_FLD_LEN_EX - 25),
                 dwDnwldTime/1000,
                 dwVerifyTime/1000,
                 CmdOpts.ulActualCodeFCSum);
         UpdateTxt(szMessage,COL_1_X_FLD_ST,CODE_Y,COL_1_FLD_LEN_EX);
        }
       }
      }
     }
     
     // Do we need to download to data flash?
     if(bSuccess && (CmdOpts.DownloadMode != DWN_CODE_ONLY_MODE))
     {
      SetStage(DOWNLOAD_STAGE);
      UpdateCurStageTxt("Downloading bytes to data flash");
      bSuccess = bSuccess && DoDownloadDataFlash(&CmdOpts);
     }
     
     // Do we need to send down the security bits
     if(bSuccess && CfgOpts.SecurityBitsOption)
     {
      if(CmdOpts.SecurityLockMode || CmdOpts.SecuritySecureMode || CmdOpts.SecuritySerialSafeMode)
      {
       SetStage(SECURE_STAGE);
       UpdateCurStageTxt("Downloading Security Bits");
       bSuccess = bSuccess && DoSecurity(&CmdOpts);
      }
     }
     
     // Do we need to enable the bootload option
     // Bootload Option
     if(bSuccess && CfgOpts.BootloadOption)
     {
      SetStage(BOOTLOAD_STAGE);
      UpdateCurStageTxt("Configuring Bootload Options");
      bSuccess = bSuccess && DoBootloadOption(&CmdOpts);
     }
     
     // Change back to the standard baud rate now if we had changed it
     if(bSuccess && CfgOpts.FastBROption)
     {
      UpdateCurStageTxt("Changing back to original baudrate");
      bSuccess = ChangeBaudrate(Bauds.PCSlowBR,&CmdOpts,&CfgOpts,&Bauds);
     }
     else
     {
      if(&CfgOpts.FastBROption)
      {
       // An error has happened but we may stil be able to change
       // the uC baud rate back to the default ready for the next
       // attempt. Try this now but don't lose the existing status
       
       // Take a copy of the existing strings and restore them
       // at the end 
       char szCurOpTextBKUp   [sizeof(szCurOpText   )];
       char szCurStatTextBKUp [sizeof(szCurStatText)];
       char szCurStageTextBKUp[sizeof(szCurStageText)];
       
       strcpy(szCurOpTextBKUp   ,szCurOpText   );
       strcpy(szCurStatTextBKUp ,szCurStatText );
       strcpy(szCurStageTextBKUp,szCurStageText);
       
       UpdateCurStageTxt("Changing back to original baudrate");
       (void)ChangeBaudrate(Bauds.PCSlowBR,&CmdOpts,&CfgOpts,&Bauds);
       
       UpdateCurStatTxt (szCurStatTextBKUp   );
       UpdateCurStageTxt(szCurStageTextBKUp );
       UpdateCurOpTxt   (szCurOpTextBKUp);
      }
      
      
     }
     
     
     // Set the ADuC8xx executing now if required
     if(bSuccess && CmdOpts.bRun)
     {
      SetStage(RUN_STAGE);
      UpdateCurStageTxt("Starting execution of ADuC8xx");
      bSuccess = bSuccess && run(&CmdOpts,&Bauds);
     }
    }
    CommClose();
   }
   SetStage(COMPLETE_STAGE);
   
   if(bSuccess)
   {
    iRetVal = Success;
    fprintf(stderr, "SUCCESS\n");
   }
   else
    fprintf(stderr, "FAILED\n");
   
   bSuccess = iRetVal;
  }
  
 }
 
 if(!bSuccess)
 {
  // Completion Line
  return 0x1;
 }
 else
 {
  // Completion Line
  // And clear the other status lines as well
  
  UpdateCurOpTxt("Complete");
  UpdateCurStatTxt("Success");
  UpdateCurStageTxt("Complete");
  return 0x0;
 }
}

// Function     : decodeCommandLine
// Description  : extract the filename etc. from the command line parameters: 
int  decodeCommandLine(int             argc,
                       char   *        argv[],
                       TyCmdLineOpts  *pCmdOpts)

{
 int            i;
 char           option;
 char           szMessage[80];
 float          value;
 
 sprintf(szMessage,"Decoding command line options");
 UpdateCurOpTxt(szMessage);
 UpdateCurStatTxt("Reading...");
 
 
 for(i=1;i<argc;i++)                   //loop thru all arguments on cmd line
 {
  if(  (argv[i][0]=='-')
     ||(argv[i][0]=='\\')
     ||(argv[i][0]=='/'))
  {
   option=toupper(argv[i][1]);
   if(argv[i][2]==':')             // 3rd char in param string must be ';'
   {
    switch(option)               // check the second char to see what
     // parameter it is
    {
     case 'C':
     //comport parameter
     sscanf(&argv[i][3],"%d",&pCmdOpts->iComPort);
     break;
     case 'U':
     // Transit packet size
     sscanf(&argv[i][3],"%d",&pCmdOpts->iXMITBytes);
     break;
     case 'R': // Run start address
     pCmdOpts->bRun = true;
     sscanf(&argv[i][3],"%X",&pCmdOpts->ulRunAdr);
     break;
     case 'X'://crystal frequency
     sscanf(&argv[i][3],"%f",&value);   
     pCmdOpts->fcore = (unsigned long)(value * 1000000);
     if(pCmdOpts->fcore == 11059200)
      pCmdOpts->XTAL = 0x0;
     else
      pCmdOpts->XTAL = 0x2;
     break;
     case 'L'://Kernel frequency threshold for alternate behaviour
     sscanf(&argv[i][3],"%f",&value);   
     pCmdOpts->fcoreLimit = (unsigned long)(value * 1000000);
     break;
     case 'H'://Hex file checksum
     pCmdOpts->bVerifyCSUM = true;
     sscanf(&argv[i][3],"%d",&pCmdOpts->ulHexCSUM);
     break;
     case 'M':// Download mode
     sscanf(&argv[i][3],"%d",&pCmdOpts->DownloadMode);
     break;
     case 'S':// Security Mode
     {
      unsigned long ulBits;
      sscanf(&argv[i][3],"%d",&ulBits);
      
      if(ulBits > 0x3)
      {
       UpdateCurStatTxt("Invalid security mode specified.");
       return 0;
      }
      
      pCmdOpts->SecurityLockMode         = false;
      pCmdOpts->SecuritySecureMode       = false;
      pCmdOpts->SecuritySerialSafeMode   = false;
      
      switch(ulBits)
      {
       case 0x0:
       pCmdOpts->SecurityLockMode         = true;
       break;
       case 0x1:
       pCmdOpts->SecuritySecureMode       = true;
       break;
       case 0x2:
       pCmdOpts->SecuritySerialSafeMode   = true;
       break;
       case 0x3:
       pCmdOpts->SecurityLockMode         = true;
       pCmdOpts->SecuritySecureMode       = true;
       pCmdOpts->SecuritySerialSafeMode   = true;
       break;
       default:
       break;
      }
     }
     break;
     case 'Y':// Extended Security Mode
     {
      unsigned long ulBits;
      sscanf(&argv[i][3],"%d",&ulBits);
      
      if(ulBits > 0x7)
      {
       UpdateCurStatTxt("Invalid security mode specified.");
       return 0;
      }
      pCmdOpts->SecurityLockMode         = false;
      pCmdOpts->SecuritySecureMode       = false;
      pCmdOpts->SecuritySerialSafeMode   = false;
      
      if((ulBits & 0x1) == 0x0)
       pCmdOpts->SecurityLockMode         = true;
      if((ulBits & 0x2) == 0x0)
       pCmdOpts->SecuritySecureMode       = true;
      if((ulBits & 0x4) == 0x0)
       pCmdOpts->SecuritySerialSafeMode   = true;
      
     }
     break;
     case 'T': // Time out
     //                  sscanf(&argv[i][3],"%d",&pCmdOpts->ulTimeOpen);
     UpdateCurStatTxt("Option [/T] not implemented yet and ignored");
     return 0;
     break;
     default:
     sprintf(szMessage,"Unrecognised Option [%s]",argv[i]);
     UpdateCurStatTxt(szMessage);
     return 0;
     break;
    }
   } else
    if((option=='H')||(option=='?'))
    showInstructions();
   else
    if(option=='D')
    pCmdOpts->bEraseData = false;
   else
    if(option=='R')
    pCmdOpts->bRun = true;
   else
    if(option=='V')
    pCmdOpts->bVerify = true;
   else
    if(option=='N')
    pCmdOpts->bConfirmSerialSafe = false;
   else
    if(option=='B')
    pCmdOpts->bBootLoad = true;
  } else
  {
   // First unrecognised param is the code hex file
   // Second unrecognised param is the data file
   if(strlen((const char *)pCmdOpts->szCodeHexF))
   {
    strncpy(pCmdOpts->szDataHexF,argv[i],sizeof(pCmdOpts->szDataHexF));
    pCmdOpts->szDataHexF[sizeof(pCmdOpts->szDataHexF)-1] = '\0';
   }
   else
   {
    strncpy(pCmdOpts->szCodeHexF,argv[i],sizeof(pCmdOpts->szCodeHexF));
    pCmdOpts->szCodeHexF[sizeof(pCmdOpts->szCodeHexF)-1] = '\0';
   }
  }
 }
 
 return 1;
}

// Function     : validateParameters
// Description  : check that the specified parameters are valid
int validateParameters(TyCmdLineOpts  *pCmdOpts,int *pError)
{  
 int            iRetVal = 0x1;
 char           szMessage[MAX_PATH*2];
 
 UpdateCurOpTxt("Validating command line paramters");
 UpdateCurStatTxt("Validating...");
 
 
 if(iRetVal)
 {
  // A code filename must always be specified
  if(strlen(pCmdOpts->szCodeHexF)==0)              //If no string is entered
  {
   UpdateCurStatTxt("No Program file specified");
   *pError = File_error;
   iRetVal = 0;   // Program will terminate after displaying help
  }
  else
  {
   FILE * pFile=fopen(pCmdOpts->szCodeHexF,"r");     //Open the file
   
   if(pFile == 0)
   {
    sprintf(szMessage,"Code File \"%s\" not found",FieldCrop(pCmdOpts->szCodeHexF,40));
    UpdateCurStatTxt(szMessage);
    iRetVal = 0;   // Program will terminate after displaying help
    *pError = File_error;
   }
   else
    fclose(pFile);
  }
 }
 
 
 if(iRetVal)
 {
  // bDontEraseData controls the valid download modes
  // You can "erase code" only or "erase code and data" together.
  // Do not allow data download if data is not selected for erasure
  // Validate the download mode and szDataHexF
  switch(pCmdOpts->DownloadMode)
  {
   case DWN_CODE_ONLY_MODE:
   // If a data file specified then discard it. Won't be downloaded
   if(strlen(pCmdOpts->szDataHexF))
    pCmdOpts->szDataHexF[0] = '\0';
   break;
   case DWN_CODE_DATA_MODE:
   case DWN_DATA_ONLY_MODE:
   if(!pCmdOpts->bEraseData)
   {
    sprintf(szMessage,"Invalid download mode when \"Don't erase data\" option selected");
    UpdateCurStatTxt(szMessage);
    *pError = Undefined_error;
    iRetVal = 0;   // Program will terminate after displaying help
   }
   // A data file MUST be specified!
   if(strlen(pCmdOpts->szDataHexF)==0)        //If no string is entered
   {
    UpdateCurStatTxt("No data file specified for data download");
    *pError = Undefined_error;
    iRetVal = 0;   // Program will terminate after displaying help
   }
   break;
   default:
   sprintf(szMessage,"Invalid download mode specified - %d",
           pCmdOpts->DownloadMode);
   UpdateCurStatTxt(szMessage);
   iRetVal = 0;   // Program will terminate after displaying help
   *pError = Undefined_error;
   break;
  }
  
  if(strlen(pCmdOpts->szDataHexF))              //If no string is entered
  {
   FILE * pFile=fopen(pCmdOpts->szDataHexF,"r");     //Open the file
   
   if(pFile == 0)
   {
    sprintf(szMessage,"Data file \"%s\" not found",FieldCrop(pCmdOpts->szDataHexF,20));
    UpdateCurStatTxt(szMessage);
    iRetVal = 0;   // Program will terminate after displaying help
    *pError = File_error;
   }
   else
    fclose(pFile);
  }
 }
 
 
 if(iRetVal)
 {
  // Validate Timeout
  if(pCmdOpts->ulTimeOpen > 9)
  {
   sprintf(szMessage,"Invalid timeout specified - %d",
           pCmdOpts->ulTimeOpen);
   UpdateCurStatTxt(szMessage);
   iRetVal = 0;   // Program will terminate after displaying help
   *pError = Undefined_error;
  }
  // Validate Timeout
  if(pCmdOpts->ulTimeOpen != 0)
  {
   sprintf(szMessage,"Invalid timeout specified - %d",
           pCmdOpts->ulTimeOpen);
   UpdateCurStatTxt(szMessage);
   iRetVal = 0;   // Program will terminate after displaying help
   *pError = Undefined_error;
  }
 }
 
 if(iRetVal)
 {
  if((pCmdOpts->fcore<2000000) || (pCmdOpts->fcore>70000000))
  {
   sprintf(szMessage,"Invalid target crystal frequency");
   UpdateCurStatTxt(szMessage);
   iRetVal = 0;   // Program will terminate after displaying help
   *pError = Undefined_error;
  }
 }
 
 if(iRetVal)
 {
  if(pCmdOpts->ulRunAdr>0xFFFF)
  {
   sprintf(szMessage,"Invalid target crystal frequency");
   UpdateCurStatTxt(szMessage);
   iRetVal = 0;   // Program will terminate after displaying help
   *pError = Undefined_error;
  }
 }
 
 return iRetVal;                              
}
// Function     : showInstructions
// Description  : Show the tool command line syntax
void showInstructions(void)
{
 printf("\nProgram options:\n");
 printf("\n  <code.hex> a .hex file must be specified");
 printf("\n  <code.hex> <data.hex> a .hex file must be specified");
 printf("\n  /C:n\t\tselect the com port (default is 1)");
 printf("\n  /D\t\tdo not erase Flash/EE data space");
 printf("\n  /R\t\trun the program from the beginning (00 hex)");
 printf("\n  /R:xxxx\trun the program from the specified start address");
 printf("\n  /V\t\tselect verify (default is off)");
 printf("\n  /X:n.n\tselect the target crystal freq (MHz) (default 11.0592)");
 printf("\n  /B\t\tBootload Option, runs from E000h");
 printf("\n  /M:n\t\tDownload Mode 0=Code+Data,1=Code,2=Data (default 1)");
 printf("\n  /U:n\t\tTransmit packet size - default is 1");
 printf("\n  /S:n\t\tSecurity Mode (0=Lock,1=Secure,2=Serial Safe,3=All");
 printf("\n  /H:n\t\tSelect hexfile checksum verification (default is off)");
 printf("\n  /N:n\t\tSuppress serial safe setting confirmation");
 //   printf("\n  /T:n\t\tTime program remains open for in second. (0-9 seconds)");
 printf("\n  Example:");
 printf("\n  wsdcom filename.hex /c:1 /r");
}

/****************************************************************** */


// Function     : reset
// Description  : attempt to perform a software reset on the target
b32  reset(int *piMajor,int *piMinor ,char * pszSignature,
           long Baudrate,TyCmdLineOpts  * pCmdOpts,
           char *pcVHi,char *pcVMid,char *pcVLo,int iNumAttempts,TyBaudRates *Bauds)
{
 unsigned char  szPacket  [4];
 unsigned long  ulDiff;
 double         Percentage ;
 char szMessage[0x100] ;
 *piMajor=0;
 *piMinor=0;
 
 /*now send the reset packet*/
 /*the first byte, the '!' */
 szPacket[0]='!';
 szPacket[1]='Z';
 szPacket[2]= (unsigned char)(0-'Z');
 szPacket[3]=0;
 
 sprintf(szMessage,"%d Hz",pCmdOpts->fcore);
 UpdateTxt(szMessage,COL_1_X_FLD_ST,CRYSTAL_Y,COL_1_FLD_LEN);
 
 if(Bauds->PCSlowBR >  Bauds->ADSlowBR)
  ulDiff = Bauds->PCSlowBR - Bauds->ADSlowBR;
 else
  ulDiff = Bauds->ADSlowBR - Bauds->PCSlowBR;
 Percentage = (double) (ulDiff* 100)/Bauds->ADSlowBR;
 
 sprintf(szMessage,"%.02f%%",Percentage);
 UpdateTxt(szMessage,COL_1_X_FLD_ST,BAUD_ERR_Y,COL_1_FLD_LEN);
 
 for(int iLoop = 0x0;iLoop < iNumAttempts;iLoop++)
 {
  
  // Open the comport at the selected baudrate
  if(!CommOpen(pCmdOpts->iComPort,Baudrate))
   return false;
  
  sprintf(szMessage,"Resetting the target device (%d)",iLoop+1);
  UpdateCurOpTxt(szMessage);
  UpdateCurStatTxt("Waiting for response");
  
  if(sendPacket(szPacket,4,pCmdOpts))
   if(waitForSignature(pszSignature,piMajor,piMinor,pcVHi,pcVMid,pcVLo))
  {
   sprintf(szMessage,"Reset OK! %s device detected",pszSignature);
   UpdateCurStatTxt(szMessage);
   sprintf(szMessage,"%s V%c.%c.%c",pszSignature,*pcVHi,*pcVMid,*pcVLo);
   UpdateTxt(szMessage,COL_1_X_FLD_ST,PART_Y,COL_1_FLD_LEN);
   UpdateScreenInfo(pCmdOpts,Bauds);
   PurgeComm(hCom,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
   CommClose();
   UpdateTxt("SUCCESS",COL_2_X_FLD_ST,SUCCESS_Y,COL_2_FLD_LEN);
   return true;
  }
 }
 
 sprintf(szMessage,"Failed to connect to the ADuC8xx after %d attempts",iNumAttempts);
 UpdateCurStatTxt(szMessage);
 CommClose();
 UpdateTxt("FAILED",COL_2_X_FLD_ST,SUCCESS_Y,COL_2_FLD_LEN);
 return false;
}

// Function     : verifyfile
// Description  : download the program to the target device
b32 verifyfile(TyCmdLineOpts *   pCmdOpts)
{
 int   result;
 char  szMsg[0x200];
 result=VerifyAfterDownload(pCmdOpts->szCodeHexF,pCmdOpts);
 switch(result)                 //print the status messages
 {
  case OP_OK:
  sprintf(szMsg,"Verified OK!");
  break;
  case NOT_RESET:
  sprintf(szMsg,"Error: Target was not reset");
  break;
  case TGT_NOT_COMPT:
  sprintf(szMsg,"Error: This version of WSD is not compatible with this target");
  break;
  case FAILED_TO_DO:
  sprintf(szMsg,"Error: Failed to read.");
  break;
  case FILE_NOT_FOUND:
  sprintf(szMsg,"Error: File not found");
  break;
  case INVALID_FILE:
  sprintf(szMsg,"Error: %s is not a valid 8xx hex file",FieldCrop(pCmdOpts->szCodeHexF,20));
  break;
  case OP_FAILED:
  default:
  sprintf(szMsg,"Error: %s did not verify",FieldCrop(pCmdOpts->szCodeHexF,20));
  break;
 }
 
 UpdateCurStatTxt(szMsg);
 
 return(result==0);
}

// Function     : DoDownloadCodeFlash
// Description  : 
b32 DoDownloadCodeFlash(TyCfgFileOpt * pCfg,TyCmdLineOpts * pCmdOpts)
{
 int            result = 0x1;
 
 if(pCfg->QuickDownloadOption)
  result=downloadRev2QuickMode(pCfg,pCmdOpts);
 
 if(result != 0x0)
  result=downloadRev2(pCfg,pCmdOpts);
 
 return(result==0);
}


// Function     : run
// Description  : send the target device a command to commence user code 
// execution from a specified address
b32 run(TyCmdLineOpts  * pCmdOpts,TyBaudRates    *  pBauds)
{
 int            result;
 char           szMsg[0x200];
 b32           bCommAlreadyOpen = true;
 b32           bSuccess = true;
 
 if(hCom == INVALID_HANDLE_VALUE)
 {
  bCommAlreadyOpen  = false;
  *pBauds             = getCorrectBauds(pCmdOpts,0);
  bSuccess=(CommOpen(pCmdOpts->iComPort,pBauds->PCSlowBR)==1);
 }
 
 if(bSuccess)
 {
  sprintf(szMsg,
          "Running program from address 0x%04lx: "
          ,pCmdOpts->ulRunAdr);
  
  UpdateCurOpTxt(szMsg);
  
  result=DoRun(pCmdOpts);
  
  if(!bCommAlreadyOpen)
   CommClose();
  
  switch(result)
  {
   case OP_OK:
   sprintf(szMsg,"Run OK!");
   break;
   case NOT_RESET:
   sprintf(szMsg,"Error: Target was not reset");
   break;
   case TGT_NOT_COMPT:
   sprintf(szMsg,"Error: This version of WSD is not compatible with this target");
   break;
   case FAILED_TO_DO:
   sprintf(szMsg,"Error: Failed to run.");
   break;
  }
  
  UpdateCurStatTxt(szMsg);
 }
 else
  result=OP_FAILED;
 
 return(result==OP_OK);
}
/****************************************************************** */
/*Revision 2 loader routines */

// Function     : waitForSignature
// Description  : Wait for signiture from the ADuC8xx
int waitForSignature(char *pszSignature,
                     int  *piMajor,
                     int  *piMinor,
                     char *pcVHi,
                     char *pcVMid,
                     char *pcVLo)
{
 int            i        = 0;
 int            state    = 0;
 unsigned char  checksum = 0;
 DWORD          dwStartTime;    //clock variable for determining timeouts
 unsigned char  ch;
 unsigned char  data[30];
 
 strcpy(pszSignature,"none");      
 dwStartTime=GetTickCount();          //get start clock value
 while(1)                    //keep trying....
 {
  // get the data
  if(ReadChar(&ch))    //get the data
  {
   data[state]=ch;
   checksum+=ch;
   switch(state)  //see what we are doing....
   {
    //watch for fallthroughs in this switch statement
    case 0:        //waiting for start character ("A")
    if(ch=='A')
     state++;
    else
     return 0;
    // checksum=0;
    break;
    case 1:
    if(ch=='D')  //waiting for second character ("D")
     state++;
    else
     state=0;
    break;
    case 2:               
    if(ch=='I')  //waiting for third character ("I")
     state++;
    else
     state=0;
    break;
    case 3: //waiting for device partname
    /*Since we scrap the "ADI" part code header,
      replace it with "ADuC"*/              
    strcpy(pszSignature,"ADuC");
    i=4;                
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    /*if the character in the device partname is a printable
      character, other than capital "X" or space " ", then
 add it to the signature string */
    if((isprint(ch)) && (ch!='X') && (ch!=' '))
     pszSignature[i++]=ch;
    pszSignature[i]='\0';
    state++;
    break;
    case 10:
    if(ch=='V')
     state++;
    else
     state=0;
    break;
    case 11:
    *pcVHi = ch;
    if(isdigit(ch) || isalpha(ch))
    {
     *piMajor=ch-'0';
     state++;
    } else
     state=0;
    break;
    case 12:
    *pcVMid = ch;
    if(isdigit(ch) || isalpha(ch))
    {
     *piMinor=(ch-'0')*10;
     state++;
    } else
     state=0;
    break;
    case 13:
    *pcVLo = ch;
    if(isdigit(ch) || isalpha(ch))
    {
     *piMinor+=(ch-'0');
     state++;
    } else
     state++;
    break;
    case 14:
    if(ch=='\r')
     state++;
    else
     state=0;
    break;
    case 15:
    if(ch=='\n')
     state++;
    else
     state=0;
    break;
    case 16:     //waiting for hardware id hi byte: ignore
    case 17:     //waiting for hardware id lo byte: ignore
    case 18:     //waiting for firmware id hi byte: ignore
    case 19:     //waiting for firmware id lo byte: ignore
    case 20:     //waiting for age hi byte:         ignore
    case 21:    //waiting for age lo byte:         ignore
    case 22:     //waiting for spare byte:          ignore  
    case 23:     //waiting for spare byte:          ignore 
    state++;
    break;
    case 24:     
    return(checksum==0);
   }
  }
  if(GetTickCount()>(dwStartTime+500))  //see if timeout has elapsed ,try 50
   return 0;         //timeout failure
 }
}
// Function     : EraseFlash
// Description  : Erase flash
b32 EraseFlash(TyCmdLineOpts  * pCmdOpts)
{
 int            result=0;
 b32           bOK;
 
 if(!pCmdOpts->bEraseData)
 {
  UpdateCurOpTxt("Erasing code memory . . ");
  if(sendLoaderPacket(1,"C",pCmdOpts))
   result=waitForAck(ERASETIMEOUT);
 } else
 {
  UpdateCurOpTxt("Erasing code and data memory . . ");
  if(sendLoaderPacket(1,"A",pCmdOpts))
   result=waitForAck(DEFAULTTIMEOUT);
 }
 if(result)
 {
  bOK = true;
  UpdateCurStatTxt("OK");
 } else
 {
  bOK = false;
  UpdateCurStatTxt("Failed");
 }
 
 return bOK;
}
// Function     : IHEX_WriteFile
// Description  : Write an intel hex file
int IHEX_WriteFile(FILE          *hOutputFile, 
                   unsigned char *pszDataBuffer, 
                   unsigned long  ulDataAddr,
                   unsigned long  ulDataCount)
{
 unsigned char ucChecksum;
 unsigned long i;
 unsigned long ulNewBaseAddress,m_ulBaseAddress=0x0;
 unsigned long ulDataCountThisTime = ulDataCount;
 
 while(ulDataCount)
 {
  ulDataCountThisTime = ulDataCount;
  
  if(ulDataCountThisTime > 0x10)
   ulDataCountThisTime = 0x10;
  
  ulNewBaseAddress = ulDataAddr & 0xFFFF0000;
  if(ulNewBaseAddress != m_ulBaseAddress)
  {
   m_ulBaseAddress = ulNewBaseAddress;
   if(m_ulBaseAddress <= 0xF0000)
   {
    ucChecksum =  0x02;
    ucChecksum += 0x00;
    ucChecksum += 0x00;
    ucChecksum += 0x02;
    ucChecksum += (unsigned char)(m_ulBaseAddress >> 4);
    ucChecksum += (unsigned char)(m_ulBaseAddress >> 12);
    fprintf(hOutputFile, ":02000002");
    fprintf(hOutputFile, 
            "%4.4X", 
            (unsigned short)(m_ulBaseAddress >> 4));
    fprintf(hOutputFile, "%2.2X\n", (unsigned char)(0 - ucChecksum));
   } else
   {
    ucChecksum =  0x02;
    ucChecksum += 0x00;
    ucChecksum += 0x00;
    ucChecksum += 0x04;
    ucChecksum += (unsigned char)(m_ulBaseAddress >> 16);
    ucChecksum += (unsigned char)(m_ulBaseAddress >> 24);
    fprintf(hOutputFile, ":02000004");
    fprintf(hOutputFile, 
            "%4.4X", 
            (unsigned short)(m_ulBaseAddress >> 16));
    fprintf(hOutputFile, "%2.2X\n", (unsigned char)(0 - ucChecksum));
   }
  }
  
  ucChecksum =  (unsigned char)ulDataCountThisTime;
  ucChecksum += (unsigned char)(ulDataAddr - m_ulBaseAddress);
  ucChecksum += (unsigned char)((ulDataAddr - m_ulBaseAddress) >> 8);
  fprintf(hOutputFile, ":%2.2X%4.4X00",(unsigned char)ulDataCountThisTime,
          (unsigned short)(ulDataAddr - m_ulBaseAddress));
  for(i=0; i < ulDataCountThisTime; i++)
  {
   ucChecksum += pszDataBuffer[i];
   fprintf(hOutputFile, "%2.2X", pszDataBuffer[i]);
  }
  fprintf(hOutputFile, "%2.2X\n", (unsigned char)(0 - ucChecksum));
  
  ulDataAddr    += ulDataCountThisTime;
  pszDataBuffer += ulDataCountThisTime;
  ulDataCount   -= ulDataCountThisTime;
 }
 
 fprintf(hOutputFile, ":00000001FF\n");
 
 return 0x0;
}

// Function    : VerifyAfterDownload
// Decription  : Verify that program was downloaded correctly
//               Store read back data to hex file
//               Store read back data to binary file
int VerifyAfterDownload(char *program,TyCmdLineOpts *   pCmdOpts)
{
 FILE *         pFile;                           //Old C pointer to file
 unsigned long  progAddr;                        //address to download to
 int            blockLen;                        //number of bytes 
 int            PageAddress;
 int            file_size;                       // No. of lines in a file
 int            err;                             // File error
 double         percentile        = 0;
 double         percentage        = 0;
 int            result            = 0;           // succes
 unsigned char  verifyData        [0xF800];      // Data read back from part
 unsigned char  InputData         [257];         // Data read back from part
 unsigned char  verifyfilename    [_MAX_PATH];
 unsigned char  verifyfilenamebin [_MAX_PATH];
 char           drive             [_MAX_DRIVE];
 char           dir               [_MAX_DIR];
 char           fname             [_MAX_FNAME];
 char           ext               [_MAX_EXT];
 char           packet            [128];         //temp storage packet
 unsigned char  progData          [128];         //block of data to download
 char           szMsg             [_MAX_PATH*2];                          
 
 
 
 // BIN and HEX files will be created in the same directory
 // as the source file with names called verify.bin and verify.hex
 // _splitpath/_makepath not available on all systems
 _splitpath( program, drive, dir, fname, ext );
 strcpy(fname,"verify");
 strcpy(ext,"hex");
 _makepath((char *)verifyfilename,
           (char *)drive,
           (char *)dir,
           (char *)fname, 
           (char *)ext );
 strcpy(ext,"bin");
 _makepath((char *)verifyfilenamebin,
           (char *)drive,
           (char *)dir,
           (char *)fname,
           (char *)ext);
 
 memset(verifyData,0xFF,sizeof(verifyData));
 
 // Read back the data from the part
 result=0;
 percentile = ((double)100 / 248);
 
 UpdateCurOpTxt("Reading target memory");
 
 for(PageAddress = 0x0;PageAddress < 248;PageAddress++)
 {
  int iTry = 20;
  
  while(iTry--)
  {
   packet[0]=0x07;
   packet[1]=0x0E;
   packet[2]=0x2;
   packet[3]='V';   // <-- Non-documented verify command. Can only be sent
   // after an erase/program sequence.
   packet[4]= PageAddress;
   packet[5] = (512 - 2 - 86 - PageAddress) % 256;
   
   if(sendPacket((unsigned char *)packet,6,pCmdOpts))
   {
    //wait for 500ms or till 257 bytes received
    if(ReadByteBufferTimed(InputData, 257, 500))
    {
     int i;
     unsigned char checksum=0;
     
     for(i=0;i<256;i++)
      checksum+= InputData[i];
     
     if((unsigned char)(0x100 - checksum) == InputData[i])
     {
      memcpy(verifyData+(PageAddress*256),InputData,256);
      break;
     }
    } 
    else
    {
     iTry = 0;
     break;
    }
   }
  }
  
  if(iTry == 0)
  {
   // Failed to successfully read.
   return 3;
  }
  
  sprintf(szMsg,"%.0lf%%",percentage);
  UpdateCurStatTxt(szMsg);
  
  percentage += percentile;
 }
 
 if(result)
  return result;
 
 UpdateCurOpTxt("Verifying against file data");
 
 if((err = CheckHexFile(program,&file_size,pCmdOpts)) != OP_OK)
  return err;
 percentile = (100 / (double)file_size);
 percentage = 0;
 
 /*try and open the file*/
 pFile=fopen(program,"r");     //Open the file
 /*check if it opened ok*/
 if(pFile!=0)
 {           //while not end of file
  while(1)
  {               //keep going until all downloaded
   /*read a block of data from the file, along with its destination addr*/
   if(readBlockFromFile(pFile,&progAddr,&blockLen,progData))
   {
    if(blockLen>0)
    {      //if data exists
     if((progAddr < 0xF800) && ((progAddr+blockLen-1) < 0xF800))
     {
      if(memcmp(progData,verifyData+progAddr,blockLen))
      {
       result=6;
       break;
      }
     } else
     {
      result=6; 
      break;
     }
     //if not 100%
     sprintf(szMsg,"%.0lf%%",percentage);
     UpdateCurStatTxt(szMsg);
     percentage += percentile;
     result=0;
    }
   } else
    break;
  }
  if(percentage<=100)
  {
   sprintf(szMsg,"%.0lf%%",percentage);
   UpdateCurStatTxt(szMsg);
  }
  fclose(pFile);          //close the intel hex file
 } else
  result=4;
 
 return result;
}


// Function     : downloadRev2
// Description  : 
int downloadRev2(TyCfgFileOpt *   pCfg,TyCmdLineOpts  * pCmdOpts)
{
 FILE *         pFile;               // Old C pointer to file
 unsigned char  progData[128];       // block of data to download
 unsigned long  progAddr;            // address to download to
 int            blockLen;            // number of bytes in the block
 int            result=0;            // succes
 char           packet[128];         // temp storage packet
 int            i;
 int            file_size;           // No. of lines in a file
 double         percentile = 0;
 double         percentage = 0;
 int            err;                 // File error
 char           szStatus[0x80];                          
 
 
 UpdateCurOpTxt("Downloading using standard download protocol");
 if((err = CheckHexFile(pCmdOpts->szCodeHexF,&file_size,pCmdOpts)) != OP_OK)
  return err;
 
 percentile = (100 / (double)file_size);
 
 /*try and open the file*/
 pFile=fopen(pCmdOpts->szCodeHexF,"r");     //Open the file
 /*check if it opened ok*/
 if(pFile!=0)
 {           //while not end of file
  while(1)
  {               //keep going until all downloaded
   /*read a block of data from the file, along with its destination addr*/
   if(readBlockFromFile(pFile,&progAddr,&blockLen,progData))
   {
    if(blockLen>0)
    {      //if data exits
     result=FAILED_TO_DO;      //assume the worst...
     packet[0]='W'; //build up the program packet
     packet[1]=(progAddr>>16)%256; //write code memory command
     packet[2]=(progAddr>>8) %256; //set up the 24bit address
     packet[3]= progAddr     %256;
     for(i=0;i<blockLen;i++) //copy the program bytes into the packet
      packet[i+4]=progData[i];   
     if(sendLoaderPacket(blockLen+4,packet,pCmdOpts))
     {  //send the command as a 'loader' packet*/
      if(waitForAck(DEFAULTTIMEOUT))
      {        //check to see if the target ack's the command*/
       if(percentage<100)
       {          //if not 100%
        sprintf(szStatus,"%.0lf%%",percentage);
        UpdateCurStatTxt(szStatus);
       }
       percentage += percentile;
       result=OP_OK;
      } else
      {
       sprintf(szStatus,
               "Failure writing 0x%04lX length 0x%04lX",
               progAddr,
               blockLen);
       UpdateCurStatTxt(szStatus);
       break;
      }
     } else
     {
      UpdateCurStatTxt("Error: Failed to send packet");
      break;
     }
    }
   } else
    break;
   if(percentage<=100)
   {
    sprintf(szStatus,"%.0lf%%",percentage);
    UpdateCurStatTxt(szStatus);
   }
  }
  fclose(pFile);          //close the intel hex file
 } 
 else
 {
  result=FILE_NOT_FOUND;
  UpdateCurStatTxt("Error: File not found");
 }
 
 
 if(result == OP_OK)
  UpdateCurStatTxt("Downloaded OK!");
 
 
 return result;
}

// Function     : downloadRev2QuickMode
// Description  : 
int downloadRev2QuickMode(TyCfgFileOpt* pCfg,TyCmdLineOpts  * pCmdOpts)
{
 FILE *         pFile;               // Old C pointer to file
 unsigned char  progData[0xF800];    // block of data to download
 unsigned long  progAddr;            // address to download to
 int            blockLen,NumBlocks;  // number of bytes in the block
 int            result=0;            // succes
 char           packet[260];         // temp storage packet
 double         percentile = 0;
 double         percentage = 0;
 unsigned char  ErasedPage[0x100];   // temp storage packet
 char           szStatus[0x80];                          
 int            file_size;                       // No. of lines in a file
 
 // Initialise to erased state
 memset(progData,0xFF,sizeof(progData));
 memset(ErasedPage,0xFF,sizeof(ErasedPage));
 
 UpdateCurOpTxt("Downloading using quick protocol");
 if((result = CheckHexFile(pCmdOpts->szCodeHexF,&file_size,pCmdOpts)) != OP_OK)
  return result;
 
 // Read from hex file into array
 pFile=fopen(pCmdOpts->szCodeHexF,"r");     //Open the file
 /*check if it opened ok*/
 if(pFile!=0)
 {           //while not end of file
  while(1)
  {               //keep going until all downloaded
   /*read a block of data from the file, along with its destination addr*/
   if(readBlockFromFile(pFile,&progAddr,&blockLen,ErasedPage))
   {
    if(blockLen>0)
    {
     progAddr &= 0xFFFF;
     if((progAddr < 0xF800) && ((progAddr + blockLen - 0x1) < 0xF800))
      memcpy(progData+progAddr,ErasedPage,blockLen) ;
    }
   } else
    break;
  }
  fclose(pFile);          //close the intel hex file
 }
 else
 {
  result=FILE_NOT_FOUND;
  UpdateCurStatTxt("Error: File not found");
 }
 
 memset(ErasedPage,0xFF,sizeof(ErasedPage));
 
 NumBlocks = 0x0;
 // Count how many pages have to be downloaded
 for(progAddr=0x0;progAddr<0xF800;progAddr+=0x100)
 {
  if(memcmp(progData+progAddr,ErasedPage,sizeof(ErasedPage)))
   NumBlocks++;
 }
 
 percentile = (((double)100) / NumBlocks);
 
 
 for(progAddr=0x0;progAddr<0xF800;progAddr+=0x100)
 {
  if(memcmp(progData+progAddr,ErasedPage,sizeof(ErasedPage)))
  {
   result=FAILED_TO_DO;      //assume the worst...
   packet[0]=7; //build up the program packet
   packet[1]='Q'; //build up the program packet
   packet[2]=(progAddr>>8); //set up the 24bit address
   memcpy(packet+3,progData+progAddr,0x100);
   
   int xsum=0;
   for(int i=0;i<0x100;i++)
    xsum+=progData[progAddr+i];
   
   xsum += (progAddr>>8);
   packet[259]=0-xsum;
   
   if(sendPacket((unsigned char *)packet,sizeof(packet),pCmdOpts))
   {  //send the command as a 'loader' packet*/
    if(waitForAck(DEFAULTTIMEOUT))
    {        //check to see if the target ack's the command*/
     if(percentage<100)
     {          //if not 100%
      sprintf(szStatus,"%.0lf%%",percentage);
      UpdateCurStatTxt(szStatus);
     }
     percentage += percentile;
     
     result=OP_OK;
    } else
    {
     sprintf(szStatus,
             "Failure writing page 0x%04lX",
             progAddr);
     UpdateCurStatTxt(szStatus);
     break;
    }
   } else
   {
    UpdateCurStatTxt("Error: Failed to send packet");
    break;
   }
   if(percentage<=100)
   {
    sprintf(szStatus,"%.0lf%%",percentage);
    UpdateCurStatTxt(szStatus);
   }
  }
 }
 
 
 if(result == OP_OK)
  UpdateCurStatTxt("Downloaded OK!");
 
 return result;
}


// Function     : DoRun
// Description  : Run from specified address
int DoRun(TyCmdLineOpts  * pCmdOpts)
{
 char data[4];
 /*build the 'run' command packet*/
 data[0]='U';
 data[1]=(pCmdOpts->ulRunAdr>>16)%256;
 data[2]=(pCmdOpts->ulRunAdr>>8)%256;
 data[3]= pCmdOpts->ulRunAdr%256;
 /*send the command as a 'loader' packet*/
 if(sendLoaderPacket(4,data,pCmdOpts))
 {
  /*check to see if the target ack's the command*/
  if(waitForAck(DEFAULTTIMEOUT))
   return OP_OK;
 }
 return FAILED_TO_DO;
}

// Function     : sendPacket
// Description  : Send data to the ADuC8xx
int sendPacket(unsigned char *message,int length,TyCmdLineOpts  * pCmdOpts)
{
 DWORD iBytesWritten  = 0x0;
 int   iBytesThisTime = 0x1;
 int   iBytesLeft     = length;
 
 // There is no handshaking of bytes going down so we have to be careful 
 // that we don't send it too fast
 while(iBytesLeft)
 {
  iBytesThisTime = min(pCmdOpts->iXMITBytes,iBytesLeft);
  if(WriteFile(hCom, message,iBytesThisTime,&iBytesWritten,0))
  {
   if(iBytesWritten == iBytesThisTime)
   {
    iBytesLeft -= iBytesThisTime;
    message    += iBytesThisTime;
   } else
    return 0x0;
  } else
   return 0x0;
 }
 return 0x1;
}

// Function     : sendLoaderPacket
// Description  : sends a correctly formated loader command packet to the target
int sendLoaderPacket(int len, char *data,TyCmdLineOpts  * pCmdOpts)
{
 unsigned char packet[263];  //the loader packet
 unsigned char xsum;        //temp var to calculate the checksum
 int            i;
 
 /*set up the 070E header*/
 packet[0]=0x07;
 packet[1]=0x0E;
 /*set up the number of data bytes in the loader packet*/
 packet[2]=len;
 /*copy the data into the packet*/
 for(i=0;i<len;i++)
  packet[3+i]=data[i];
 
 /*now calculate the checksum of the packet*/
 /*ignore the header, but include the length byte*/
 
 xsum=len;
 for(i=0;i<len;i++)
  xsum+=packet[i+3];
 packet[len+3]=0-xsum;
 /*send the packet to the target device*/
 return sendPacket(packet,len+4,pCmdOpts);
}

// Function     : waitForAck
// Description  : wait for acknowledge from the target device
int waitForAck(int iTimeoutMS)
{
 int            i=0;
 unsigned char  ch;
 DWORD          dwStartTime;  //clock variable for determining the timeouts
 
 dwStartTime=GetTickCount();  //get start clock value
 while(1)             //keep trying...
 {
  if(ReadChar(&ch))   //see if any data from the target
  {
   i++;
   //see if it is an ACK
   if(ch==PROT_ACK)
    return 1;
   if(i > 30)
    return 0;
  } else
   if(GetTickCount()>dwStartTime+iTimeoutMS) //see if timeout has elapsed
   return 0;                      /*failed (due to timeout)*/
 }
}

// Function     : ReadByteBufferTimed
// Description  : Read data from target with a timeout
int ReadByteBufferTimed(unsigned char * InputData,int NumBytes,int timeout)
{
 int            i=0;
 DWORD          dwStartTime;  //clock variable for determining the timeouts
 DWORD          iBytesRead;
 
 dwStartTime=GetTickCount();  //get start clock value
 while(1)             //keep trying...
 {
  if(ReadFile(hCom, InputData+i, NumBytes-i, &iBytesRead, 0))
  {
   i+=iBytesRead;
   if(i == NumBytes)
    return 1;
  }
  if(GetTickCount()>dwStartTime+timeout)     //see if timeout has elapsed
   return 0;                      /*failed (due to timeout)*/
 }
}

// Function     : readRecordFromFile
// Description  : 
int readRecordFromFile(FILE *FptrIn,unsigned char *Record)
{
 int Index=0;
 unsigned char InChar;
 InChar=fgetc(FptrIn);
 while((InChar!='\n') && ((char)InChar!=EOF))
 {
  Record[Index++]=InChar;
  InChar=fgetc(FptrIn);
 }
 Record[Index] = '\0';
 if(Index==0) return 0;        /*nothing read*/
 if(Record[0]!=':') return 0; /*not a intel hex record*/
 return 1;
}


// Function     : readBlockFromFile
// Description  : 
int readBlockFromFile (FILE *FptrIn,unsigned long *addr,
                       int *len, unsigned char *data)
{
 char           Record[128];
 int            bytecount;
 unsigned char  xsum=0;
 int            rectype;
 int            packetxsum;
 int            i;
 unsigned long  DataByte=0;
 
 
 if(readRecordFromFile(FptrIn,(unsigned char*)Record))
 {
  /*get the number of data bytes in the record*/
  if(sscanf(Record+1,"%2X",&bytecount)!=1) return 0;
  xsum+=bytecount;
  /*get the block start address*/
  if(sscanf(Record+3,"%4lX",addr)!=1) return 0;
  xsum+=(*addr)/256;
  xsum+=(*addr)%256;
  /*get the record type*/
  if(sscanf(Record+7,"%2X",&rectype)!=1) return 0;
  xsum+=rectype;
  /*get the data from the record*/
  for(i=0;i<bytecount;i++)
  {
   if(sscanf(Record+9+i*2,"%2X",&DataByte)!=1) return 0;
   data[i] = (unsigned char)DataByte;
   xsum+=data[i];
  }
  /*get the checksum from the record*/
  if(sscanf(Record+9+bytecount*2,"%2X",&packetxsum)!=1) return 0;
  xsum+=packetxsum;
  /*validate the checksum*/
  if(xsum!=0)
   return 0;
  /*make sure it's a type of records that we can handle*/
  if(rectype==0)         //only return type 0 records
   *len=bytecount;
  else if(rectype==1)    //ignore type 1 records
   *len=0;
  else
   return 0;            //fail on any other type of records
  return 1;
 }
 return 0;
}

// Function     : ReadChar
// Description  : Read one byte from the uart
b32 ReadChar(unsigned char *   pcCh)
{
 b32 bRet = false;
 DWORD iBytesRead;
 if(ReadFile(hCom, pcCh, 1, &iBytesRead, 0))
  if(iBytesRead == 0x1)
  bRet = true;
 
 return bRet;
}


// Function     : CheckHexFile
// Description  : 
//checks if the file is valid, and checks the amount of lines in it
// 
// Calculate the checksum of the total file. Note that the previous implementation
// made an assumption that the hex files contained 16 data bytes per line. This is
// not a valid assumption but to keep the calculated checksums consistent between
// versions we will mirror that assumption again.
int CheckHexFile(char * program,int  * piNumLines,TyCmdLineOpts  *  pCmdOpts)
{
 int            iValid   = OP_OK;
 char           pcHexTxt [520];
 FILE  *        pFile;
 char           szStatus[0x80];                          
 int            iLoop;
 char           csHexVal[3];
 unsigned long  HexVal;
 char *         pcStr;
 
 pCmdOpts->ulActualCodeFCSum        = 0x0;
 
 
 *piNumLines = 0x0;
 
 pFile = fopen(program,"rt");
 
 if(pFile)
 {
  while(!feof(pFile) && !ferror(pFile) && (iValid   == OP_OK))
  {
   if(fgets(pcHexTxt,sizeof(pcHexTxt),pFile))
   {
    (*piNumLines)++;
    
    while((pcStr = strchr(pcHexTxt,'\n')))
     *pcStr = '\0';
    while((pcStr = strchr(pcHexTxt,'\r')))
     *pcStr = '\0';
    if(strlen(pcHexTxt) == 0x0)
     continue;
    
    if(pcHexTxt[0] == ':')
    {
     if(strncmp(pcHexTxt,":00000001FF",11)==0)
      break;
     
     for(iLoop=0x1;iLoop < strlen(pcHexTxt);iLoop+=0x2)
     {
      strncpy(csHexVal,pcHexTxt+iLoop,0x2);
      csHexVal[0x2]= 0x0;
      if(sscanf(csHexVal,"%X",&HexVal) == 0x1)
       pCmdOpts->ulActualCodeFCSum+= HexVal;
      else
       iValid = INVALID_FILE;
     }
    }
    else
     iValid = INVALID_FILE;
   }
  }
  fclose(pFile);          //close the intel hex file
 }
 else
  iValid = FILE_NOT_FOUND;
 
 // Was the checksum ok?
 if(pCmdOpts->bVerifyCSUM && (iValid == OP_OK))
 {
  if(pCmdOpts->ulActualCodeFCSum != pCmdOpts->ulHexCSUM)
   iValid = INVALID_CSUM;
 }
 
 
 switch(iValid)
 {
  default:
  case OP_OK:
  break;
  case INVALID_CSUM:
  sprintf(szStatus,"Invalid file checksum, expected 0x%X but calculated 0x%X",pCmdOpts->ulHexCSUM,pCmdOpts->ulActualCodeFCSum);
  UpdateCurStatTxt(szStatus);
  break;
  case FILE_NOT_FOUND:
  strcpy(szStatus,"File not found");
  UpdateCurStatTxt(szStatus);
  break;
  case INVALID_FILE:
  strcpy(szStatus,"Invalid hex file");
  UpdateCurStatTxt(szStatus);
  break;
 }
 
 
 
 return iValid;
}

// Function     : ReadHexBlock
// Description  : 
void ReadHexBlock(char *         pcHexLn,
                  unsigned long *addr,
                  unsigned int * len,
                  unsigned char *data)
{
 int            iBytecount;
 unsigned char  cCSum=0;        //Checksum.
 int            iRectype;
 int            packetxsum;
 int            i1;
 unsigned long  ulTmp;
 
 *len = 0;
 if(sscanf(pcHexLn+1,"%2x",&iBytecount)==1)
 {
  cCSum += iBytecount;
  if(sscanf(pcHexLn+3,"%4lx",addr)==1)
  {
   cCSum += (*addr)/256;
   cCSum += (*addr)%256;
   if(sscanf(pcHexLn+7,"%2x",&iRectype)==1)
   {
    cCSum += iRectype;
    for(i1=0; i1<iBytecount; i1++)
    {
     sscanf(pcHexLn+9+i1*2,"%2x",&ulTmp);
     data[i1] = (unsigned char)ulTmp;
     cCSum += data[i1];
    }
    if(sscanf(pcHexLn+9+iBytecount*2,"%2x",&ulTmp)==1)
    {
     packetxsum = (int)ulTmp;
     cCSum += packetxsum;
     if((cCSum==0) && (iRectype==0))
      *len = iBytecount;   //Ignore other rectypes.
     
    }
   }
  }
 }
}

// Function     : CommClose
// Description  : Close the serial port (if open
// Revision     : initial 
void CommClose(void)
{
 if(hCom != INVALID_HANDLE_VALUE)
 {
  // Release RTS\DTR 
  EscapeCommFunction(hCom,CLRDTR);
  EscapeCommFunction(hCom,CLRRTS);
  CloseHandle(hCom);
 }
 hCom = INVALID_HANDLE_VALUE;
}

// Function     : CommOpen
// Description  : Open the specified serial port for commns
// Revision     : initial 
int CommOpen(int ComPort,long Baudrate)
{  
 COMMTIMEOUTS      CommTimeouts;
 DCB               dcb;
 BOOL              fSuccess;
 char              pcCommPort[0x20];
 DWORD             dw;
 char              szMessage[80];
 
 if(hCom != INVALID_HANDLE_VALUE)
  CommClose();
 
 
 sprintf(pcCommPort,"//./COM%d",ComPort);
 
 sprintf(szMessage,"Opening %s",pcCommPort);
 UpdateCurOpTxt(szMessage);
 UpdateCurStatTxt("Opening");
 
 hCom = CreateFile( pcCommPort,
                   GENERIC_READ | GENERIC_WRITE,
                   0,               // must be opened with exclusive-access
                   0,            // no security attributes
                   OPEN_EXISTING,   // must use OPEN_EXISTING
                   0,               // not overlapped I/O
                   0             // hTemplate must be 0
                   );
 
 if(hCom == INVALID_HANDLE_VALUE)
 {
  // Handle the error.
  sprintf(szMessage,"%s,%ld baud,failed to open!",pcCommPort,Baudrate);
  UpdateTxt(szMessage,COL_1_X_FLD_ST,COMM_Y,COL_1_FLD_LEN);
  UpdateCurStatTxt("Failed");
  return(0);     //failed
 }
 
 dw = SetCommMask(hCom,EV_RXCHAR);
 dw = SetupComm(hCom,1024,1024);
 PurgeComm(hCom,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
 
 
 // Build on the current configuration, and skip setting the size
 // of the input and output buffers with SetupComm.
 fSuccess = GetCommState(hCom, &dcb);
 
 // Fill in DCB: 9600 bps, 8 data bits, no parity, and 1 stop bit.
 dcb.BaudRate         = Baudrate;
 dcb.ByteSize         = 8;             // data size, xmit, and rcv
 dcb.Parity           = NOPARITY;      // no parity bit
 dcb.StopBits         = ONESTOPBIT;    // one stop bit
 dcb.fBinary          = TRUE;             //binary mode     
 dcb.fParity          = FALSE;       //enable parity checking
 dcb.fOutxCtsFlow     = FALSE;    //!CTS output flow control 
 dcb.fOutxDsrFlow     = FALSE;    //!DSR output flow control 
 dcb.fDtrControl      = DTR_CONTROL_ENABLE; 
 dcb.fDsrSensitivity  = FALSE;   
 dcb.fTXContinueOnXoff= TRUE;     
 dcb.fOutX            = FALSE;          //!XON/XOFF out flow control           
 dcb.fInX             = FALSE;          //!XON/XOFF in flow control   
 dcb.fErrorChar       = FALSE;           
 dcb.fNull            = FALSE;                
 dcb.fRtsControl      = RTS_CONTROL_ENABLE; 
 dcb.wReserved        = FALSE;
 dcb.fAbortOnError    = FALSE;
 
 fSuccess = SetCommState(hCom, &dcb);
 if(!fSuccess)
 {
  sprintf(szMessage,"%s,%ld baud,configuration failed!",pcCommPort,Baudrate);
  UpdateTxt(szMessage,COL_1_X_FLD_ST,COMM_Y,COL_1_FLD_LEN);
  UpdateCurStatTxt("Failed");
  return(0);
 }
 GetCommTimeouts (hCom, &CommTimeouts);
 
 // Change the COMMTIMEOUTS structure settings.
 CommTimeouts.ReadIntervalTimeout          = 50;
 CommTimeouts.ReadTotalTimeoutMultiplier   = 5;
 CommTimeouts.ReadTotalTimeoutConstant     = 50;   
 SetCommTimeouts(hCom, &CommTimeouts);
 
 sprintf(szMessage,"%s,%ld baud,ok",pcCommPort,Baudrate);
 UpdateTxt(szMessage,COL_1_X_FLD_ST,COMM_Y,COL_1_FLD_LEN);
 UpdateCurStatTxt("Success");
 
 return 1;             //success
 
}
// Function     : ChangeBaudrate
// Description  : Change the baudrate for communicating with the part.
// 
//                A note on baudrates
//                ~~~~~~~~~~~~~~~~~~~
//                All the ADuC8xx parts UARTs are setup to run at 9600
//                assuming that they are being clocked by an 11.0592MhZ
//                crystal.
// 
//                The PC must match the initial baud rate of the ADuC8xx
//                to establish the initial communication.
// 
//                If the ADuC8xx is using a different clock frequency than
//                11.0592MhZ then the baud rate that it will initially
//                communicate at will be scaled accordingly. The PC must
//                match this as best it can.
// 
//                Once communication is established, the PC can instruct
//                the ADuC8xx part to write to its timer registers effectively
//                allowing the PC to change the baud rate that the ADuC8xx
// 
// Revision     : initial 
b32 ChangeBaudrate(unsigned long      Baudrate,
                   TyCmdLineOpts   *  pCmdOpts,
                   TyCfgFileOpt    *  pCfg,
                   TyBaudRates     *  pBauds)
{
 char             Cmd   [128];         //temp storage packet
 b32             retVal = true; 
 unsigned char    InputData         [257];         // Data read back from part
 
 // Two baud rates possible - slow (the 9600 matched baud) and fast ( the
 // maximum PC baud that the 8051 can manage)
 
 UpdateCurOpTxt("Changing baudrate on 8051");
 UpdateCurStatTxt("Pending");
 
 if(Baudrate == pBauds->FastBR)
 {
  //change baudrate to MaxKernelBaudrate if fast baudrate available
  if(pCfg->FastBROption)
  {
   //Change Baudrate Command <07h><0Eh><03h><'B'><T3CON><T3FD><CS>
   Cmd[0] = 7  ;
   Cmd[1] = 14 ;
   Cmd[2] = 3  ;
   Cmd[3] = 66 ;
   Cmd[4] = pCfg->T3CON;
   Cmd[5] = pCfg->T3FD ;
   Cmd[6] = (512 - 3 - 66 - pCfg->T3CON - pCfg->T3FD) % 256;
   
   sendPacket((unsigned char *)Cmd,7,pCmdOpts);
   
   //wait for 500ms or till 257 bytes received
   if(ReadByteBufferTimed(InputData, 1, 500))
   {
    if( InputData[0] == 6)
     CommOpen(pCmdOpts->iComPort,Baudrate);
    else
    {
     retVal = false;
     UpdateCurStatTxt("Error encountered while changing baudrate");
    }
   } else
   {
    retVal = false;
    UpdateCurStatTxt("Error encountered while changing baudrate");
   }
  } else
   retVal = false;
 }
 if(Baudrate ==pBauds->PCSlowBR)
 {
  //change baudrate to default baudrate if fast baudrate available
  if(pCfg->FastBROption)
  {
   //Change Baudrate Command <07h><0Eh><03h><'B'><T3CON><T3FD><CS>
   Cmd[0] = 7 ;
   Cmd[1] = 14;
   Cmd[2] = 3 ;
   Cmd[3] = 66;
   Cmd[4] = pCfg->T3CON9600;
   Cmd[5] = pCfg->T3FD9600 ;
   Cmd[6] = (512-3-66-pCfg->T3CON9600-pCfg->T3FD9600)%256;
   
   sendPacket((unsigned char *)Cmd,7,pCmdOpts);
   
   //wait for 500ms or till 257 bytes received
   if(ReadByteBufferTimed(InputData, 1, 500))
   {
    if( InputData[0] == 6)
     CommOpen(pCmdOpts->iComPort,Baudrate);
    else
    {
     retVal = false;
     UpdateCurStatTxt("Error encountered while changing baudrate");
    }
   } else
   {
    retVal = false;
    UpdateCurStatTxt("Error encountered while changing baudrate");
   }
  } else
   retVal = false;
 }
 return retVal;
}


// Function     : UpdateCfgData
// Description  : Each supported part will have an associated
//                configuration file containing part specific
//                information. If it is not present, return
//                information from the default file. If that
//                id not present, return an error (false)
// Revision     : initial 
b32 UpdateCfgData(char * pszPart,TyCfgFileOpt *pCfg)
{
 b32   bSuccess    = true;
 FILE * pFile       = 0;
 char   szBuffer     [MAX_PATH*2];
 char   szLineData   [MAX_PATH];
 DWORD  LastError   = ERROR_INSUFFICIENT_BUFFER;
 int    iLine       = 0x0;
 char   szMessage[0x80];
 
 
 sprintf(szMessage,"Opening Configuraton file for %s",pszPart);
 UpdateCurOpTxt(szMessage);
 UpdateCurStatTxt("Determining file location");
 
 if(GetModuleFileName(0,szBuffer,sizeof(szBuffer)) == 0x0)
  bSuccess = false;
 
 if(bSuccess)
 {
  // Generate the config file name
  char drive        [_MAX_DRIVE];
  char dir          [_MAX_DIR];
  char fname        [_MAX_FNAME];
  char ext          [_MAX_EXT];
  
  _splitpath( szBuffer, drive, dir, fname, ext );
  strcat(dir,"Configuration");
  _makepath( szBuffer, drive, dir, pszPart, "txt" );
  
  sprintf(szMessage,"Opening %s",szBuffer);
  UpdateCurOpTxt(szMessage);
  UpdateCurStatTxt("Opening");
  
  pFile = fopen(szBuffer,"rt");
  if(pFile == 0)
  {
   _makepath( szBuffer, drive, dir, "aduc8xx", "txt" );
   sprintf(szMessage,"Opening %s",szBuffer);
   UpdateCurOpTxt(szMessage);
   UpdateCurStatTxt("Opening");
   pFile = fopen(szBuffer,"rt");
  }
 }
 
 
 if(pFile != 0)
 {
  // We have the file - process it
  while(!feof(pFile) && !ferror(pFile))
  {
   if(fgets(szLineData,sizeof(szLineData),pFile))
   {
    iLine++;
    // Lines must follow this format EXACTLY to be valid
    // <ID> = <VALUE>                   '< COMMENT>
    if(strncmp(szLineData,"CodeSize",strlen("CodeSize"))== 0x0)
     sscanf(szLineData,"CodeSize = %d",&pCfg->CodeSize);
    else if(strncmp(szLineData,"DataSize",strlen("DataSize"))== 0x0)
     sscanf(szLineData,"DataSize = %d",&pCfg->DataSize);
    else if(strncmp(szLineData,"T3CON9600",strlen("T3CON9600"))== 0x0)
     sscanf(szLineData,"T3CON9600 = %d",&pCfg->T3CON9600);
    else if(strncmp(szLineData,"T3FD9600",strlen("T3FD9600"))== 0x0)
     sscanf(szLineData,"T3FD9600 = %d",&pCfg->T3FD9600);
    else if(strncmp(szLineData,"T3CON",strlen("T3CON"))== 0x0)
     sscanf(szLineData,"T3CON = %d",&pCfg->T3CON);
    else if(strncmp(szLineData,"T3FD",strlen("T3FD"))== 0x0)
     sscanf(szLineData,"T3FD = %d",&pCfg->T3FD);
    else if(strncmp(szLineData,"XTAL",strlen("XTAL"))== 0x0)
     sscanf(szLineData,"XTAL = %d",&pCfg->XTAL);
    else if(strncmp(szLineData,"fcore",strlen("fcore"))== 0x0)
     sscanf(szLineData,"fcore = %d",&pCfg->fcore);
    else if(strncmp(szLineData,"ClkAdjust",strlen("ClkAdjust"))== 0x0)
     sscanf(szLineData,"ClkAdjust = %d",&pCfg->ClkAdjust);
    else if(strncmp(szLineData,
                    "QuickDownloadOption = True",
                    strlen("QuickDownloadOption = True"))== 0x0)
     pCfg->QuickDownloadOption = true;
    else if(strncmp(szLineData,
                    "QuickDownloadOption = False",
                    strlen("QuickDownloadOption = False"))== 0x0)
     pCfg->QuickDownloadOption = false;
    else if(strncmp(szLineData,
                    "FastBaudrateOption = True",
                    strlen("FastBaudrateOption = True"))== 0x0)
     pCfg->FastBROption = true;
    else if(strncmp(szLineData,
                    "FastBaudrateOption = False",
                    strlen("FastBaudrateOption = False"))== 0x0)
     pCfg->FastBROption = false;
    else if(strncmp(szLineData,
                    "SecurityBitsOption = True",
                    strlen("SecurityBitsOption = True"))== 0x0)
     pCfg->SecurityBitsOption = true;
    else if(strncmp(szLineData,
                    "SecurityBitsOption = False",
                    strlen("SecurityBitsOption = False"))== 0x0)
     pCfg->SecurityBitsOption = false;
    else if(strncmp(szLineData,
                    "BootloadOption = True",
                    strlen("BootloadOption = True"))== 0x0)
     pCfg->BootloadOption = true;
    else if(strncmp(szLineData,
                    "BootloadOption = False",
                    strlen("BootloadOption = False"))== 0x0)
     pCfg->BootloadOption = false;
    else if(strncmp(szLineData,
                    "VerifyAfterDownloadOption = True",
                    strlen("VerifyAfterDownloadOption = True"))== 0x0)
     pCfg->VerifyAfterDownloadOption = true;
    else if(strncmp(szLineData,
                    "VerifyAfterDownloadOption = False",
                    strlen("VerifyAfterDownloadOption = False"))== 0x0)
     pCfg->VerifyAfterDownloadOption = false;
    //else
    //   fprintf(stderr,"Error in line %d of %s\n",iLine,pszBuffer);
   }
  }
 } 
 else
 {
  sprintf(szMessage,"Opening %s",szBuffer);
  UpdateCurOpTxt(szMessage);
  sprintf(szMessage,"Error while opening \"%s\"",szBuffer);
  UpdateCurStatTxt(szMessage);
  bSuccess = false;
 }
 
 if(pFile)
 {
  if(ferror(pFile))
   bSuccess = false;
  fclose(pFile);
 }
 
 return bSuccess;
}

// Function     : getCorrectBauds
// Description  : The ADuC8xx baud rate depends on the crystal used. ADuC8xx
// configures itself for 9600 baud at it's default frequency. 
// If the default frequency is not used then it is up to the PC side to 
// match the ADuC8xxx resultant baud rate. 
// 
// The PC UART can only generate certain fixed baud rates (e.g. 9600, 14400)
// but the ADuC8xx can generate baudrates that can lie between these
// fixed baudrates. PC should select the one with the least error.
// 
// Be careful of rounding errors when calculating baudrates. The PC UART
// will generate the next baudrate up if the specified baudrate is
// above a standard baudrate level. For example, configuring the PC
// UART for 9601 baud will result in possiblly 14400 rather than 9600
// 
// There are certain crystal frequencies that the PC will not be able
// to generate accurately enough. Communication with an ADuC8xx using
// these frequencies will not be possible without specialised PC UART
// hardware that can generate these bauds.
// 
// Once we have established communications with the ADuC8xx, we can change
// his baud rate up to a faster frequency for faster downloads. 
// We must determine the maximum baud that a part supports and calculate 
// the timer values neccessary to generate these bauds
// 
// Note 1: If the Crystal frequency is greater than 16MHZ the 8051 will do things 
// differently. It will then assume that the frequency is 20MZ and not the 
// default frequency. It will set up its baudrate generator for this base
// frequncy instead and try for 9600 baud
// 
TyBaudRates getCorrectBauds(TyCmdLineOpts *pCmdOpts,TyCfgFileOpt *pCfgOpts)
{
 TyBaudRates    RetBauds={0x0};
 unsigned long  MaxBaud = ADUC_BAUD_115200;
 unsigned long  FreqBand;
 
 
 if((pCmdOpts->XTAL == 0x0) || (pCmdOpts->XTAL == 0x1))
 {
  // Using defaults so 9600 is the correct baud
  RetBauds.PCSlowBR     = ADUC_BAUD_9600;
  RetBauds.ADSlowBR     = ADUC_BAUD_9600;
  // Assume that the default frequency is 11059200 until we find out
  // for sure
  pCmdOpts->fcore       = 11059200;
 }
 else
 {
  double dTmp = ((ADUC_BAUD_115200 * 1152) / (double)pCmdOpts->fcore);
  // Calculate the baud that the ADuC8xx is using after a reset
  RetBauds.ADSlowBR = ADUC_BAUD_115200 / dTmp;
  
  if(RetBauds.ADSlowBR > ADUC_BAUD_115200)
  {
   // This is unlikely and probably the user has entered an invalid 
   // crystal frequency
   RetBauds.PCSlowBR = ADUC_BAUD_115200;
  }
  else
  {
   // Calculate the best match the PC has for this baud rate
   for(FreqBand = 1; FreqBand <= 250; FreqBand++)
   {
    if(RetBauds.ADSlowBR > (ADUC_BAUD_115200 / FreqBand))
    {
     // Set the baud rate
     unsigned long ulDiffLwrBaud = RetBauds.ADSlowBR - (ADUC_BAUD_115200 / FreqBand);
     unsigned long ulDiffUprBaud = (ADUC_BAUD_115200 / (FreqBand-1)) - RetBauds.ADSlowBR;
     if(ulDiffLwrBaud > ulDiffUprBaud)
      RetBauds.PCSlowBR = ADUC_BAUD_115200 / (FreqBand - 1);
     else
      RetBauds.PCSlowBR = ADUC_BAUD_115200 / FreqBand;
     break ;
    }
   }
  }
  
 }
 
 // Can only be an ADuC841 so
 // See Note 1 above
 if(pCmdOpts->fcore >= pCmdOpts->fcoreLimit)
 {
  // If the kernel is above a certain frequency then it will configure the
  // UART for 9600 operation assuming a frequency of 20 MhZ rather then its
  // default frequency. This will have to be adjusted then for the actual
  // frequency
  double dTmp =  (double)20000000 / (double)pCmdOpts->fcore;
  
  RetBauds.ADSlowBR     = ADUC_BAUD_9600 / dTmp;
  
  // Now we have to calculate the best PC match
  // Calculate the best match the PC has for this baud rate
  for(FreqBand = 1; FreqBand <= 250; FreqBand++)
  {
   if(RetBauds.ADSlowBR > (ADUC_BAUD_115200 / FreqBand))
   {
    // Set the baud rate
    unsigned long ulDiffLwrBaud = RetBauds.ADSlowBR - (ADUC_BAUD_115200 / FreqBand);
    unsigned long ulDiffUprBaud = (ADUC_BAUD_115200 / (FreqBand-1)) - RetBauds.ADSlowBR;
    if(ulDiffLwrBaud > ulDiffUprBaud)
     RetBauds.PCSlowBR = ADUC_BAUD_115200 / (FreqBand - 1);
    else
     RetBauds.PCSlowBR = ADUC_BAUD_115200 / FreqBand;
    break ;
   }
  }
 }
 
 
 // Do we have enough info to calculate the fast frequencies yet?
 if(pCfgOpts)
 {
  if((pCfgOpts->XTAL == 0x1) || (pCfgOpts->XTAL == 0x0) )
  {
   // Fixed frequency defined by the configuration file
   pCmdOpts->fcore = pCfgOpts->fcore;
  } else
  {
   // Can only be an ADuC812,ADuC831 or ADuC841 so
   // See Note 1 above
   
   if((pCmdOpts->XTAL == 0x0) || (pCmdOpts->XTAL == 0x1))
   {
    // Processor is clocked from an external crystal but user has
    // specified that it is the default so take the value from the
    // configuration file also
    pCmdOpts->fcore = pCfgOpts->fcore;
   }
   
   if(pCmdOpts->fcore >= pCmdOpts->fcoreLimit)
   {
    pCfgOpts->T3CON9600   = 0x86;
    pCfgOpts->T3FD9600    = 1;
    RetBauds.PCSlowBR     = ADUC_BAUD_9600;
    RetBauds.ADSlowBR     = ADUC_BAUD_9600;
   }
  }
  
  // Calclate the fastest baud that the ADuC8xx can achieve ...
  RetBauds.FastBR = (pCmdOpts->fcore * 2) / (64 * pCfgOpts->ClkAdjust);
  // ... and determine the closest baud below that that the PC
  // can manage
  for(FreqBand = 0x1;FreqBand <= 250;FreqBand++)
  {
   if(RetBauds.FastBR >= (MaxBaud / FreqBand))
   {
    RetBauds.FastBR = (MaxBaud / FreqBand);
    break;
   }
  }
  
  // if FastBRoption =true then calc T3CON/T3FD for fast baudrates
  if(pCfgOpts->FastBROption)
  {
   //         if(((pCfgOpts->T3CON != 0) || (pCfgOpts->T3FD != 0x0)) && (RetBauds.FastBR == 115200))
   //            {
   //            // These values in the file override calculated ones 
   //
   //            }
   //         else
   //            {
   // Calculate them instead
   unsigned long DIV;
   DIV = (log(pCmdOpts->fcore / (RetBauds.FastBR * 32))) / log(2);
   // add enable bit
   pCfgOpts->T3CON = 128 + DIV;
   pCfgOpts->T3FD = (2 * pCmdOpts->fcore) / ((pow(2, DIV)) * RetBauds.FastBR) - 64;
   //            }
  }
 }
 
 return RetBauds;
}

// Function     : DoBootloadOption
// Description  : Enable the bootload option
// Revision     : initial 
b32 DoBootloadOption(TyCmdLineOpts  * pCmdOpts)
{
 char             BootloadCommand   [7];         //temp storage packet
 b32             retVal = false; 
 unsigned char    Response;                 
 unsigned char    Data;
 char             szMsg[0x200];
 if(pCmdOpts->bBootLoad)
  Data = 0xFE;
 else
  Data = 0xFF;
 
 sprintf(szMsg,"Setting Bootload option to %s",pCmdOpts->bBootLoad ? "TRUE":"FALSE");
 UpdateCurOpTxt(szMsg);
 
 //'<07h><0Eh><02h><46h><FFh or FEh><B9h or BAh>   FE=>bootload enable
 BootloadCommand[0] = 0x7  ;
 BootloadCommand[1] = 0xE ;
 BootloadCommand[2] = 0x2  ;
 BootloadCommand[3] = 0x46 ;
 BootloadCommand[4] = Data;
 BootloadCommand[5] = (512 - 2 - 0x46 - Data) % 256;
 
 sendPacket((unsigned char *)BootloadCommand,6,pCmdOpts);
 
 //wait for 500ms or till 1 bytes received
 if(ReadByteBufferTimed(&Response, 1, 500))
 {
  if( Response == 6)
  {
   UpdateCurStatTxt("Ok!");
   retVal = true;
  }
  else
   UpdateCurStatTxt("Did not receive ACK after sending bootload command");
 }
 else
  UpdateCurStatTxt("Lost contact after sending bootload command");
 
 return retVal;
}

// Function     : DoSecurity
// Description  : Specify security bits
// Revision     : initial 
b32 DoSecurity(TyCmdLineOpts  * pCmdOpts)
{
 unsigned char    SecurityBitsCommand   [6];         //temp storage packet
 unsigned char    DataCmd   [12];         //temp storage packet
 b32             retVal = false; 
 unsigned char    Response;            
 char             szMsg[0x200];
 unsigned char    SecurityBits = 0x7;
 
 
 if(pCmdOpts->SecurityLockMode)
  SecurityBits -= 0x1;
 
 if(pCmdOpts->SecuritySecureMode)
  SecurityBits -= 0x2;
 
 if(pCmdOpts->SecuritySerialSafeMode)
  SecurityBits -= 0x4;
 
 sprintf(szMsg,"Setting Security option to %d",SecurityBits);
 UpdateCurOpTxt(szMsg);
 UpdateCurStatTxt("");
 
 if(pCmdOpts->bConfirmSerialSafe && ((SecurityBits & 0x4) == 0x0))
 {
  // They have enabled serial safe mode...they need to be very sure that
  // this is what they want to do. Should add a command line option to
  // suppress this check
  if(MessageBox(0,
                "Are you sure you want to set the 'Serial Safe Mode' and disable the serial downloader? ",
                "Serial Safe Mode",MB_YESNO) == IDNO)
  {
   UpdateCurStatTxt("Serial Safe mode aborted");
   return false;
  }
 }
 
 
 // firstly program FF's tp page 0 of dataflash.....set EADRH/L to 0
 // <07><0E><08><69><ADDRU><ADDRH><ADDRL><DATA1><DATA2><DATA3><DATA4><CS>
 DataCmd[0]   = 7  ;
 DataCmd[1]  = 14 ;
 DataCmd[2]  = 8  ;
 DataCmd[3]  = 69 ;
 DataCmd[4]  = 0  ;        //ADDRU
 DataCmd[5]  = 0  ;        //ADDRH
 DataCmd[6]  = 0  ;        //ADDRL
 DataCmd[7]  = 0xFF;       //EDATA1
 DataCmd[8]  = 0xFF;       //EDATA2
 DataCmd[9]  = 0xFF;       //EDATA3
 DataCmd[10] = 0xFF;       //EDATA4
 DataCmd[11] = 183;        //CS
 
 sendPacket((unsigned char *)DataCmd,12,pCmdOpts);
 
 //wait for 500ms or till 1 bytes received
 if(ReadByteBufferTimed(&Response, 1, 500))
 {
  // Ignore the NAK/ACK response here. The flash may be protected
  // or already written. We do not care, all we wanted to do 
  // was set the address pointer to ensure the security
  // command works
  
  // program security bits
  //<07><0E><02><'S'><DATA><CS>
  SecurityBitsCommand[0] = 7 ;
  SecurityBitsCommand[1] = 14;
  SecurityBitsCommand[2] = 2 ;
  SecurityBitsCommand[3] = 83;
  SecurityBitsCommand[4] = SecurityBits;
  SecurityBitsCommand[5] = 256 - 83 - 2 - SecurityBits;
  
  sendPacket((unsigned char *)SecurityBitsCommand,6,pCmdOpts);
  //wait for 500ms or till 1 bytes received
  if(ReadByteBufferTimed(&Response, 1, 500))
  {
   if( Response == 6)
   {
    UpdateCurStatTxt("Ok!");
    retVal = true;
   }
   else
    UpdateCurStatTxt("Did not receive ACK after sending security command");
  }
  else
   UpdateCurStatTxt("Lost contact after sending security command");
  
 }
 else
  UpdateCurStatTxt("Lost contact after clearing flash");
 
 
 return retVal;
}


// Function     : DoSendNAK_ACKCmd
// Description  : Send a command that should respond with a single byte 
//                (NAK or ACK) to indicate success
// Revision     : initial 
b32 DoSendSimpleCmd(unsigned char * pCmd,TyCmdLineOpts  * pCmdOpts)
{
 b32              bRet       = false;
 unsigned char     ucResp;
 
 // Make sure that the checksum is correct and that the
 // fixed protocol characters are correct
 pCmd[OFF_ID_0] = PROT_START_ID_0;
 pCmd[OFF_ID_1] = PROT_START_ID_1 ;
 UpdateCS(pCmd+OFF_LEN);
 
 if(sendPacket(pCmd,pCmd[OFF_LEN]+PROT_OVHEAD,pCmdOpts))
 {
  //wait for 500ms or till 1 byte received
  if(ReadByteBufferTimed(&ucResp, 1, 500))
  {
   if(ucResp == PROT_ACK)
    bRet = true;
  }
 }
 return bRet;
}

// Function     : PreprocessLine
// Description  : Remove any unwanted characters from the line
b32 PreprocessLine(char * pszLine)
{
 char * pSrc = pszLine;
 char * pDst = pszLine;
 
 // Only alphanumeric and ':' and ',' should be in the line
 do
 {
  if(isalnum(*pSrc) || (*pSrc == '\0') || (*pSrc == ':') || (*pSrc == ','))
   *pDst++ = *pSrc;
  *pSrc++;
 }
 while(*pSrc != '\0');
 *pDst = '\0';
 
 return (pszLine[0x0] != '\0');
}


// Function     : DoDataDownload
// Description  : Download data to the ADuC8xx data flash memory
// Serial Protocol Format:
//    '<07><0E><08><69><ADDRU><ADDRH><ADDRL><DATA1><DATA2><DATA3><DATA4><CS>
// Data File format: (example)
//    0,10,99,10,99
//    1,FF,F8,11,98
//    2,12,9B,C2,97
//    3,13,AC,13,9C
//    4,A4,95,1D,95
//    5,A5,94,15,94
// Revision     : initial 
b32 DoDownloadDataFlash(TyCmdLineOpts  * pCmdOpts)
{
 b32              RetVal     = true;
 FILE *            pFile;
 char              szBuffer [0x20];
 unsigned char     Cmd      [12];
 unsigned long     Data     [5];
 char             szMsg[0x200];
 
 sprintf(szMsg,"Downloading %s to data flash",FieldCrop(pCmdOpts->szDataHexF,20));
 UpdateCurOpTxt(szMsg);
 
 pFile=fopen(pCmdOpts->szDataHexF,"r"); 
 if(pFile)
 {
  while(!feof(pFile) && !ferror(pFile) && RetVal)
  {
   if(fgets(szBuffer,sizeof(szBuffer),pFile))
   {
    if(PreprocessLine(szBuffer))
    {
     Cmd[OFF_ID_0] = PROT_START_ID_0;
     Cmd[OFF_ID_1] = PROT_START_ID_1 ;
     Cmd[OFF_LEN]  = 8;
     Cmd[OFF_CMD]  = 'E';
     Cmd[4]        = 0;                        // ADDRU
     
     if(sscanf(szBuffer,
               "%d,%X,%X,%X,%X",
               Data+0,
               Data+1,
               Data+2,
               Data+3,
               Data+4)==0x5)
     {
      Cmd[5]  = (unsigned char)(Data[0]>>8); // ADDRH
      Cmd[6]  = (unsigned char)(Data[0]);    // ADDRL
      Cmd[7]  = (unsigned char)(Data[1]);    // DATA1
      Cmd[8]  = (unsigned char)(Data[2]);    // DATA2
      Cmd[9]  = (unsigned char)(Data[3]);    // DATA3
      Cmd[10] = (unsigned char)(Data[4]);    // DATA4
     } else
     {
      // Invalid format
      sprintf(szMsg,
              "Data flash file \"%s\" contains invalid line (%s)",
              FieldCrop(pCmdOpts->szDataHexF,20),
              szBuffer);
      UpdateCurStatTxt(szMsg);
      
      RetVal = false;
     }
     if(!DoSendSimpleCmd(Cmd,pCmdOpts))
     {
      // Error in sending command
      UpdateCurStatTxt("Data flash write FAILED");
      RetVal = false;
     }
    }
   } 
   else
   {
    // Error reading from file. If not just the end of the file
    // then it's an unexpected error
    if(!feof(pFile))
    {
     UpdateCurStatTxt("Error reading from data flash file");
     RetVal = false;
    }
   }
  }
 } else
 {
  UpdateCurStatTxt("Unable to open data flash file");
  RetVal = false;
 }
 
 // Cleanup and return
 if(pFile)
  fclose(pFile);
 
 if(RetVal)
  UpdateCurStatTxt("Download ok.");
 
 return RetVal;
}

// Function     : UpdateCS
// Description  : Update the checksum in a command packet
//                pucCmd should point to the length byte
//                in the command
void UpdateCS(unsigned char *pucCmd)
{
 int               iCS   = 0x0;
 unsigned char     ucLen = pucCmd[0x0];
 unsigned char     i;
 
 iCS = 0x0;
 for(i=0;i<=ucLen;i++)
  iCS += pucCmd[i];
 pucCmd[i] = 0x100 - (iCS % 0x100);  // CS
}

// Function     : FieldCrop
// Description  : We want a filename (including path) to fit in a field length
//                so truncate it if neccessary while still retaining the 
//                important start and end bits if the field
// 
#define INITIAL_FILENAME_STRING "...\\"
char szFieldString[DEFAULT_CONSOLE_LEN];
char * FieldCrop(const char * pszFileName,int iFldLen)
{
 int         iStrLen           = 0x0;
 
 strncpy(szFieldString,pszFileName,sizeof(szFieldString));
 szFieldString[sizeof(szFieldString)-1] = '\0';
 
 iStrLen  = (int)strlen(szFieldString);
 
 if((iStrLen  > iFldLen) && (iFldLen > 0x0))
 {
  int iNumChars     = iFldLen;
  
  if (iNumChars > 0x2)
  {
   // Give ourselves a safety margin of two characters
   iNumChars -= 0x2;
  }
  if (iNumChars > strlen(INITIAL_FILENAME_STRING))
  {
   // We need at least room for "...\" to indicate that part of
   // the filename is being omitted
   iNumChars -= (int)strlen(INITIAL_FILENAME_STRING);
   
   // Now do we have enough room for the filename itself
   char  szFileName      [MAX_PATH];
   char * pCharPtr   =     0;
   char * pLastSlash =     0;
   
   strcpy(szFileName, pszFileName);
   
   // Point at the last character in the string
   pCharPtr = szFileName + strlen(szFileName) - 0x1;
   
   while ((pCharPtr != szFileName) && (iNumChars != 0x0))
   {
    pCharPtr    --;
    iNumChars   --;
    if (*pCharPtr == '\\')
     pLastSlash = pCharPtr;
   }
   
   if (pCharPtr == szFileName)
   {
    // Looks like we can fit the entire string after all
    // Do nothing
   }
   else
   {
    if (pLastSlash != 0)
    {
     strcpy(szFieldString,INITIAL_FILENAME_STRING);
     strcat(szFieldString,pLastSlash+1);
    }
    else
    {
     strncpy(szFieldString,INITIAL_FILENAME_STRING,sizeof(INITIAL_FILENAME_STRING)-2);
     szFieldString[sizeof(INITIAL_FILENAME_STRING)-2] = 0x0;
     strcat(szFieldString,pCharPtr);
    }
   }
  }
 }
 
 return szFieldString;
}

