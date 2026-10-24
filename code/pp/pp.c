#define BASE_FORCE_THREADS_COUNT 1
#include "base/base.h"
#include "base/base.c"

BASE_ENTRY_POINT()
{
    cmd_line *CmdLine = &Params->CmdLine;
    
    b32 ShowHelp = true;
    b32 ShouldQuit = false;
    str8 OutFileName = {0};
    str8 InFileName = {0};
    
    arena *Arena = GetScratch();
    str8_array Help = PushS8Array(Arena, 256);
    
    //- Parse command line arguments 
{
    InFileName = CmdLine->FirstFile->V;
}

{    
    cmd_line_entry *Entry = CmdLineGetEntryAndAddHelp(CmdLine, &Help, S8("o"), S8("output file"));
        OutFileName = Entry->First->V;
        if(!OutFileName.Size)
        {
            str8 FileNameWithoutExt = S8CutFromChar(InFileName, '.', false);
            OutFileName = Str8Fmt("%S.pp", FileNameWithoutExt);
        }
    }
    
    //- Preprocess 
    {
            
        str8 File = OS_ReadEntireFileIntoMemory(CSS8(InFileName));
        str8 FileDir = S8CutFromChar(InFileName, '/', false);
        
        str8 String;
        S8OverPush(Arena, String, KB(512), String.Size)
        {
            String.Size = 0;
            for(u64 Idx = 0; Idx < File.Size && !ShouldQuit; Idx += 1)
            {
                ShowHelp = false;
                
                u8 Char = File.Data[Idx];
                
                if(0) {}
                else if(Char == ';')
                {
                    // Skip comments
                    for(Idx += 1;
                        Idx < File.Size && File.Data[Idx] != '\n';
                        Idx += 1);
                    String.Data[String.Size] = '\n';
                    String.Size += 1;
                }
                else if(Char == '.')
                {
                    // Skip
                    // TODO(luca): Only if next is org or equ
                    // makes .org -> org ; .equ ->equ
                }
                else if(Char == '$')
                {
                    str8 Name = {.Data = File.Data + Idx + 1};
                    for(u64 TokenIdx = Idx + 1;
                        TokenIdx < File.Size && IsAlphaNum(File.Data[TokenIdx]);
                        TokenIdx += 1, Name.Size += 1);
                    
                    if(0) {}
                    else if(S8Match(Name, S8("NOMOD51"), S8MatchFlag_CaseInsensitive))
{
                        Idx += Name.Size;
                    }
                    else if(S8Match(Name, S8("include"), S8MatchFlag_CaseInsensitive))
                    {
                        Idx += Name.Size;
                        
                        for(;
                            Idx < File.Size && (File.Data[Idx] != '"');
                            Idx += 1);
                        
                        if(File.Data[Idx] == '"')
                        {
                            Idx += 1;
                            str8 FileName = {0};
                            FileName.Data = File.Data + Idx;
                            for(;
                                Idx < File.Size && File.Data[Idx] != '"';
                                Idx += 1, FileName.Size += 1);
                            if(File.Data[Idx] == '"')
                            {
                                str8 FilePath = Str8Fmt("%S" SLASH "%S", FileDir, FileName);
                                str8 IncludeFile = OS_ReadEntireFileIntoMemory(CSS8(FilePath));
                                
                                MemoryCopy(String.Data + String.Size, IncludeFile.Data, IncludeFile.Size);
                                String.Size += IncludeFile.Size;
                            }
                            else
                            {
                                Log("Error: Missing '\"' for string.\n");
                            }
                            
                        }
                            else
                        {
                            Log("Error: Expected string after include.\n");
                            ShouldQuit = true;
                        }
                            
                    }
                }
                else
{
                String.Data[String.Size] = Char;
                String.Size += 1;
}
        }
        }
        
        OS_WriteEntireFile(CSS8(OutFileName), String);
    }

    if(ShowHelp)
    {
        str8 HelpText = S8ArrayJoinFrom(Arena, &Help, '\n');
        Log("Usage: pp [options] file1.asm\n"
            "%S\n", HelpText);
    }
    
    return 0;
}