/* date = January 26th 2026 11:00 am */
#pragma once

//~ Types
typedef uint gl_uint;
typedef uint gl_enum;
typedef int gl_int;

typedef struct gl_render_state gl_render_state;
struct gl_render_state
{
 gl_uint VAOs[1];
 gl_uint VBOs[1];
 gl_uint Textures[2];
 gl_uint RectShader;
 b32 ShadersCompiled;
};

//~ Globals
global_variable arena *GL_FrameArena = 0;

//~ Functions 
internal void 
FatalError(const char* message)
{
 fprintf(stderr, "%s\n", message);
 
 char cmd[1024];
 snprintf(cmd, sizeof(cmd), "zenity --error --no-wrap --text=\"%s\"", message);
 system(cmd);
 
 exit(0);
}

internal void  
DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
              GLsizei length, const GLchar* message, const void* user)
{
 fprintf(stderr, "%s\n", message);
 if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
 {
  Assert(!"OpenGL API usage error! Use debugger to examine call stack!");
 }
}

internal void
GL_ErrorStatus(gl_uint Handle, b32 IsShader)
{
 b32 Valid = true;
 
 char InfoLog[KB(2)] = {0};
 if(IsShader)
 {
  glGetShaderiv(Handle, GL_COMPILE_STATUS, &Valid);
  glGetShaderInfoLog(Handle, sizeof(InfoLog), NULL, InfoLog);
 }
 else
 {
  glGetProgramiv(Handle, GL_LINK_STATUS, &Valid);
  glGetProgramInfoLog(Handle, sizeof(InfoLog), NULL, InfoLog);
 }
 
 if(!Valid)
 {
  ErrorLog("%s\n", InfoLog);
 }
}

internal gl_uint
GL_CompileShaderFromString(str8 File, u32 Type)
{
 gl_uint Shader = glCreateShader(Type);
 
 glShaderSource(Shader, 1, (const char **)&File.Data, 0);
 glCompileShader(Shader);
 GL_ErrorStatus(Shader, true);
 
 return Shader;
}

internal gl_uint
GL_CompileShaderFromFile(str8 FileNameAfterExe, u32 Type)
{
 gl_uint Shader = 0;
 
 char *FilePath = PathFromExe(GL_FrameArena, FileNameAfterExe);
 str8 File = OS_ReadEntireFileIntoMemory(FilePath);
 
 if(File.Size)
 {    
  Shader = GL_CompileShaderFromString(File, Type);
  OS_FreeFileMemory(File);
 }
 
 return Shader;
}

internal gl_uint
GL_ProgramFromShaders(gl_uint VertexShader, gl_uint FragmentShader)
{
 gl_uint Program = 0;
 
 Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
 glLinkProgram(Program);
 GL_ErrorStatus(Program, false);
 
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader); 
 
 return Program;
}

internal void
GL_LoadFloatsIntoBuffer(gl_uint BufferHandle, gl_uint ShaderHandle, char *AttributeName, s32 Count, s32 VecSize, void *Buffer)
{
 gl_int AttribHandle;
 
 s32 SizeOfVec = (smm)(sizeof(f32))*VecSize;
 
 glBindBuffer(GL_ARRAY_BUFFER, BufferHandle);
 
 AttribHandle = glGetAttribLocation(ShaderHandle, AttributeName);
 Assert(AttribHandle != -1);
 glEnableVertexAttribArray((gl_uint)AttribHandle);
 glVertexAttribPointer((gl_uint)AttribHandle, VecSize, GL_FLOAT, GL_FALSE, SizeOfVec, 0);
 
 glBufferData(GL_ARRAY_BUFFER, (SizeOfVec*Count), Buffer, GL_STATIC_DRAW);
}

internal void
GL_LoadTextureFromImage(gl_uint Texture, s32 Width, s32 Height, u8 *Image, gl_enum Format, gl_uint ShaderProgram, char *TextureHandle)
{
 glUseProgram(ShaderProgram);
 
 glActiveTexture(GL_TEXTURE0);
 glBindTexture(GL_TEXTURE_2D, Texture);
 
 s32 InternalFormat = GL_RGBA;
 
 glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Image);
 
 // NOTE(luca): Border might show up sometimes, so we use it to debug, but it isn't really desired.
#if 1
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
 f32 Color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
 glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, Color);
#endif
 
 // TODO(luca): Use mipmap
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 
 gl_int UTexture = glGetUniformLocation(ShaderProgram, TextureHandle); 
 glUniform1i(UTexture, 0);
}
