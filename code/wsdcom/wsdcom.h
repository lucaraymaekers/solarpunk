/*
THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES INC. ``AS IS'' AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT, ARE 
DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES INC. BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
*/

// Module       : Download.h
// Description  : Download to AduCxxx parts
// Date         : 06-Mar-2006

// Download Modes
#define DWN_CODE_DATA_MODE 0x0
#define DWN_CODE_ONLY_MODE 0x1
#define DWN_DATA_ONLY_MODE 0x2

#define COL_1_X_LBL_ST          0   // Start of a column 1 label
#define COL_1_X_FLD_ST          13  // Start of a column 1 field
#define COL_1_FLD_LEN           (40 - COL_1_X_FLD_ST -1)
#define COL_1_FLD_LEN_EX        (80 - COL_1_X_FLD_ST -1)

#define COL_2_X_LBL_ST          40 // Start of a column 1 label
#define COL_2_X_FLD_ST          53 // Start of a column 1 field
#define COL_2_FLD_LEN           (80 - COL_2_X_FLD_ST -1)

#define PROG_TITLE_1_Y 0  // Y Co-ordinate of Program Title
#define PROG_TITLE_2_Y 1  // Y Co-ordinate of Program Title

// Stage Bar
#define STAGE_BAR      "================================================================================"
#define STAGE_BAR_1_Y  3  
#define CUR_STAGE_IND  4  
#define STAGE_BAR_2_Y  5  


// Comms 
#define COMMS_LABEL     "Comms      :"
#define COMM_Y          7 
// Part 
#define PART_LABEL      "Part       :"
#define PART_Y         8 
// Crystal
#define CRYSTAL_LABEL   "Crystal    :"
#define CRYSTAL_Y      9 
// Baud Error
#define BAUD_ERR_LABEL  "%Baud Er.  :"
#define BAUD_ERR_Y     10 
// Download Mode
#define DWN_MD_LABEL    "Dwnlod Md. :"
#define DWN_MD_Y         11 

// XMit Sz
#define XMIT_SZ_LABEL    "XMit Size  :"
#define XMIT_SZ_Y         12

// Code File
#define CODE_LABEL      "Code File  :"
#define CODE_Y         13 
// Data File
#define DATA_LABEL      "Data File  :"
#define DATA_Y         14 
// Boot load
#define BOOTLOAD_LABEL  "Bootload   :"
#define BOOTLOAD_Y     7 
// Erase data
#define ERASE_LABEL     "Erase Data :"
#define ERASE_Y        8
// Run
#define RUN_LABEL       "Run Adr.   :"
#define RUN_Y          9
// Verify
#define VERIFY_LABEL    "Verify     :"
#define VERIFY_Y       10

// Security
#define SECURITY_LABEL  "Security   :"
#define SECURITY_Y       11

// Success
#define SUCCESS_LABEL     "Success    :"
#define SUCCESS_Y         12

// Stage
#define STAGE_LABEL     "Stage      :"
#define CUR_STAGE_Y    16 
// Operation
#define OP_LABEL        "Operation  :"
#define CUR_OP_Y       18 
// Status
#define STAT_LABEL      "Status     :"
#define CUR_STAT_Y     19 

#define COMPLETE_Y     21  // Y Co-ordinate of completion message

#define CONSOLE_END    22  // Last modifiable area


#define DEFAULT_CONSOLE_LEN 80

// Stages
#define CONNECT_STAGE   0x0
#define ERASE_STAGE     0x1
#define DOWNLOAD_STAGE  0x2
#define VERIFY_STAGE    0x3
#define BOOTLOAD_STAGE  0x4
#define SECURE_STAGE    0x5
#define RUN_STAGE       0x6
#define COMPLETE_STAGE  0x7

/* Complex Types */
typedef struct
{
 // All of these options are taken from the Configuration File
 unsigned long  XTAL;                 // default = 11.0592MHz crystal
 unsigned long  CodeSize;             // size of code memory
 unsigned long  DataSize;             // size of the data memory
 unsigned long  T3CON;                // baudrate cfg 115200 (if allowed)
 unsigned long  T3FD;                 // baudrate cfg 115200 (if allowed)
 unsigned long  T3CON9600;            // baudrate cfg 9600
 unsigned long  T3FD9600;             // baudrate cfg 9600
 unsigned long  fcore;                // default
 unsigned long  ClkAdjust;            // For checking of max. kernel bauds
 b32           QuickDownloadOption; // Allow Quick Download Protocol
 b32           FastBROption;  // Allow change to Fast Baudrate
 b32           SecurityBitsOption;  // Allow Download to Security Bits
 b32           BootloadOption;      // Allow Bootload Mode
 b32           VerifyAfterDownloadOption;  // Allow Verify Code Command
} TyCfgFileOpt;  // Configuration File Options

typedef struct
{
 unsigned long  XTAL;                 // default = 11.0592MHz crystal
 char           szCodeHexF[MAX_PATH];   // hex file for code flash
 char           szDataHexF[MAX_PATH];   // hex file for data flash
 unsigned int   iComPort;               // COM Port required
 b32           bEraseData;             // Erase the data flash ?
 b32           bRun;                   // Run the program after downloading
 unsigned long  ulRunAdr;               // Run Address
 b32           bVerify;                // Verify after download
 unsigned long  fcore;                  // Crystal frequency in Hz
 b32           bBootLoad;              // Bootload Option, runs from 0xE000
 unsigned long  DownloadMode;           // Download mode
 b32           SecurityLockMode;       // Security Mode
 b32           SecuritySerialSafeMode; // Security Mode
 b32           SecuritySecureMode;     // Security Mode
 unsigned long  ulTimeOpen;             // Time Program Remains Open for
 int            iXMITBytes;             // Number of bytes to send at a time
 b32           bPromptForFiles;        // Do I need to prompt for the files to download?
 b32           bVerifyCSUM;            // Do I need to verify the hex file checksum
 unsigned long  ulHexCSUM;              // Expected Hex file checksum
 unsigned long  ulActualCodeFCSum;      // Actual hex file checksum
 unsigned long  fcoreLimit;             // Threshold above which kernel will detect fast clock
 b32           bConfirmSerialSafe;     // Do I need to confirm serial safe mode
} TyCmdLineOpts;  // Options specified on the invocation line

typedef struct
{
 unsigned long  FastBR;          // Baudrate for fast download
 unsigned long  ADSlowBR;      // ADuC baud rate (slow mode)
 unsigned long  PCSlowBR;        // PC baud rate (slow mode)
}  TyBaudRates;

int  decodeCommandLine
(
 int               argc,
 char   *          argv[],
 TyCmdLineOpts  *  pCmdOpts
 );

TyBaudRates getCorrectBauds
(
 TyCmdLineOpts *   pCmdOpts,
 TyCfgFileOpt  *   pCfgOpts
 );

int DoActions
(
 TyCmdLineOpts *CmdOpts,
 TyCfgFileOpt  *CfgOpts,
 TyBaudRates   *Bauds
 );
b32  reset
(
 int  *            major, 
 int  *            minor,
 char *            sanatizedSig,
 long              Baudrate,
 TyCmdLineOpts  *  pCmdOpts,
 char *            pcVHi,
 char *            pcVMid,
 char *            pcVLo,
 int               iNumAttempts,
 TyBaudRates       *Bauds
 );
b32  run
(
 TyCmdLineOpts  *  pCmdOpts,
 TyBaudRates    *  pBauds
 );
void SetStage
(
 int iStage
 );
void InitSessionStructs
(
 TyCfgFileOpt *CfgOpts,
 TyCmdLineOpts  *CmdOpts
 );

