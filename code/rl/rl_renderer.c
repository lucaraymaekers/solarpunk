//~ Functions
internal void
RenderBuildAtlas(font_atlas *Atlas, u64 FontCount, font **Fonts, f32 HeightPx)
{ 
 arena *Arena = Atlas->Arena;
 s32 CodepointCount = 0;
 
 for EachIndex(Idx, FontCount)
 {
  font *Font = Fonts[Idx];
  
  if(Font->Initialized)
  {  
   // TODO(luca): Unhardcode with asset pipeline.
   Font->FirstPackedCharIdx = CodepointCount;
   Font->HeightPx = HeightPx;
   if(Idx == FontKind_Music)
   {
    Font->HeightPx *= 2;
   }
   Font->Scale = stbtt_ScaleForPixelHeight(&Font->Info, Font->HeightPx);
   
   CodepointCount += Font->CodepointCount;
  }
 }
 
 Atlas->Width = 1024;
 Atlas->Height = 1024;
 u64 Size = (u64)(Atlas->Width*Atlas->Height);
 
 ArenaSetPos(Arena, 0);
 Atlas->Data = PushArrayZero(Arena, u8, Size);
 Atlas->PackedChars = PushArray(Arena, stbtt_packedchar, (u64)CodepointCount);
 Atlas->AlignedQuads = PushArray(Arena, stbtt_aligned_quad, (u64)CodepointCount);
 
 s32 PackedCharStartIdx = 0;
 stbtt_pack_context Ctx = {0};
 {
  stbtt_PackBegin(&Ctx, 
                  Atlas->Data,
                  Atlas->Width, Atlas->Height,
                  0, 1, 0);
  
  for EachIndex(Idx, FontCount)
  {  
   font *Font = Fonts[Idx];
   if(Font->Initialized)
   {    
    stbtt_PackFontRange(&Ctx, Font->Info.data, 0, Font->HeightPx, 
                        Font->FirstCodepoint, Font->CodepointCount, 
                        Atlas->PackedChars + Font->FirstPackedCharIdx);
    
   }
  }
  
  stbtt_PackEnd(&Ctx);
 }
 
 for EachIndex(Idx, CodepointCount)
 {
  float UnusedX, UnusedY;
  stbtt_GetPackedQuad(Atlas->PackedChars, Atlas->Width, Atlas->Height, 
                      Idx,
                      &UnusedX, &UnusedY,
                      &Atlas->AlignedQuads[Idx], 0);
 }
}

internal rect_instance *
DrawRect(v4 Dest, v4 Color, f32 CornerRadius, f32 BorderThickness, f32 Softness)
{
 rect_instance *Result = GlobalRectsInstances + GlobalRectsCount;
 
 b32 RecIsValid = (Dest.Min.X < Dest.Max.X &&
                   Dest.Min.Y < Dest.Max.Y);
 if(RecIsValid)
 {    
  GlobalRectsCount += 1;
  
  MemoryZero(Result);
  
  Result->Dest = Dest;
  Result->Clip = GlobalClipStackTop->V;
  Result->Color0 = Color;
  Result->Color1 = Color;
  Result->Color2 = Color;
  Result->Color3 = Color;
  Result->CornerRadii = V4F32(CornerRadius);
  Result->BorderThickness = BorderThickness;
  Result->Softness = Softness;
 }
 else
 {
  NoOp();
 }
 
 return Result;
}

internal rect_instance *
DrawFilledRect(v2 Pos, v2 Size, v4 Color)
{
 rect_instance *Result = 0;
 
 v4 Dest = RectFromSize(Pos, Size);
 Result = DrawRect(Dest, Color, 0.f, 0.f, 0.f);
 
 return Result;
}

internal rect_instance *
DrawRectChar(font_atlas *Atlas, font *Font, v2 Pos, rune Codepoint, v4 Color)
{
 rect_instance *Result = 0;
 v4 Dest = {0};
 
 // NOTE(luca): Evereything happens in pixel coordinates in here.
 // TODO(luca): Proper glyph cache.
 
 s32 PackedCharIdx = GetPackedCharIdx(Font, Codepoint);
 
 {
  stbtt_packedchar *PackedChar = &Atlas->PackedChars[PackedCharIdx];
  stbtt_aligned_quad *Quad = &Atlas->AlignedQuads[PackedCharIdx];
  // TODO(luca): Investigate why floorf needed.
  f32 Width = FloorF32((f32)(PackedChar->x1 - PackedChar->x0));
  f32 Height = FloorF32((f32)(PackedChar->y1 - PackedChar->y0));
  {    
   f32 Baseline = GetBaseline(Font);
   f32 Descent = Font->Scale*(f32)Font->Descent;
   
   v2 Min = V2(FloorF32(Pos.X + PackedChar->xoff), 
               FloorF32(Pos.Y + PackedChar->yoff + Baseline + Descent));
   v2 Max = V2(Min.X + Width, Min.Y + Height);
   
   Dest = RectV2(Min, Max);
   
   Result = DrawRect(Dest, Color, 0.f, 0.f, 0.f);
   Result->TexSrc = V4(Quad->s0, Quad->t0, Quad->s1, Quad->t1);
  }
  
  Result->HasTexture = true;
  
#if 0
  DrawRect(Result->Dest, V4(0.f, 0.f, 1.f, 1.f), 0.f, 1.f, 0.f);
#endif
 }
 
 return Result;
}

internal void 
GL_DebugCallback(GLenum Source,
                 GLenum Type,
                 GLuint Id,
                 GLenum Severity,
                 GLsizei Length,
                 const GLchar *Message,
                 const void *UserParam)
{
 Log("OpenGL DEBUG: source=%u type=%u id=%u severity=%u\n"
     "    %.*s\n",
     Source,
     Type,
     Id,
     Severity,
     Length,
     Message);
}

internal void
RenderInit(gl_render_state *Render)
{
 glGenVertexArrays(ArrayCount(Render->VAOs), &Render->VAOs[0]);
 glGenTextures(ArrayCount(Render->Textures), &Render->Textures[0]);
 glGenBuffers(ArrayCount(Render->VBOs), &Render->VBOs[0]);
 glBindVertexArray(Render->VAOs[0]);
 glBindBuffer(GL_ARRAY_BUFFER, Render->VBOs[0]);
 glActiveTexture(GL_TEXTURE0);
 
 glEnable(GL_DEBUG_OUTPUT);
 glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
 
 glDebugMessageCallback(GL_DebugCallback, 0);
}

internal void
RenderCleanup(gl_render_state *Render)
{
 glDeleteVertexArrays(ArrayCount(Render->VAOs), &Render->VAOs[0]);
 glDeleteTextures(ArrayCount(Render->Textures), &Render->Textures[0]);
 glDeleteBuffers(ArrayCount(Render->VBOs), &Render->VBOs[0]);
 glDeleteProgram(Render->RectShader);
}

internal void
RenderBeginFrame(arena *Arena, s32 X, s32 Y, s32 Width, s32 Height)
{
 glViewport(X, Y, Width, Height);
 
 GlobalRectsCount = 0;
 GlobalRectsInstances = PushArray(Arena, rect_instance, MaxRectsCount);
 GL_FrameArena = Arena;
 GlobalClipStackTop = 0;
 
 v4 Clip = RectV2(V2S32(X, Y), V2S32(Width, Height));
 RenderPushClip(Clip);
 
 glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 glEnable(GL_BLEND);
 glDisable(GL_DEPTH_TEST);
}

internal void 
RenderClear(void)
{
 glClearColor(U32ToV4Arg(0xFF2E3440));
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

internal void
RenderDrawAllRectangles(gl_render_state *Render, v2 BufferDim, font_atlas *Atlas)
{
 
 if(!Render->ShadersCompiled)
 {
  gl_uint RectShader = Render->RectShader;
  
#if RL_PLATFORM_HOT_RELOAD_SHADERS
  str8 VertPath = S8("../code/rl/generated/rect_vert.glsl");
  str8 FragPath = S8("../code/rl/generated/rect_frag.glsl");
  gl_uint VertexShader = GL_CompileShaderFromFile(VertPath, GL_VERTEX_SHADER);
  gl_uint FragmentShader = GL_CompileShaderFromFile(FragPath, GL_FRAGMENT_SHADER);
#else
  gl_uint VertexShader = GL_CompileShaderFromString(VSCode, GL_VERTEX_SHADER);
  gl_uint FragmentShader = GL_CompileShaderFromString(FSCode, GL_FRAGMENT_SHADER);
#endif
  
  // NOTE(luca): Could be 0 if there are errors.
  if(VertexShader && FragmentShader)
  {
   glDeleteProgram(RectShader);
   
   RectShader = GL_ProgramFromShaders(VertexShader, FragmentShader);
   glUseProgram(RectShader);
   
   Render->RectShader = RectShader;
   Render->ShadersCompiled = true;
  }
 }
 
 // Uniforms
 {
  gl_int UViewport = glGetUniformLocation(Render->RectShader, "U_Viewport");
  glUniform2f(UViewport, V2Arg(BufferDim));
 }
 
 // NOTE(luca): This cannot be shared between platform and app.  For now it is used for fonts
 // so we just make sure that the platform (which is called first) generates the right atlas
 // font.
 GL_LoadTextureFromImage(Render->Textures[0], Atlas->Width, Atlas->Height, Atlas->Data, GL_RED, Render->RectShader, "Texture");
 
#if RL_PLATFORM_HOT_RELOAD_SHADERS
 Render->ShadersCompiled = false;
#endif
 
 Assert(GlobalRectsCount < MaxRectsCount);
 u64 Size = GlobalRectsCount*sizeof(rect_instance);
 glBufferData(GL_ARRAY_BUFFER, (s32)Size, GlobalRectsInstances, GL_STATIC_DRAW);
 
 u64 Offset = 0;
 
 s32 TotalSize = 0;
 for EachElement(Idx, RectVSAttributeOffsets)
 {            
  u64 Count = (u64)RectVSAttributeOffsets[Idx];
  
  glEnableVertexAttribArray((gl_uint)Idx);
  glVertexAttribDivisor((gl_uint)Idx, 1);
  glVertexAttribPointer((gl_uint)Idx, (gl_int)Count, GL_FLOAT, false, sizeof(rect_instance), (void *)((Offset)*sizeof(f32)));
  Offset += Count;
  
  TotalSize += RectVSAttributeOffsets[Idx];
 }
 Assert(TotalSize == (sizeof(rect_instance)/sizeof(f32)));
 
 glEnable(GL_MULTISAMPLE);
 
 glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (gl_int)GlobalRectsCount);
}

internal void
RenderRectSetColor(rect_instance *Instance, v4 Color)
{
 Instance->Color0 = Color;
 Instance->Color1 = Color;
 Instance->Color2 = Color;
 Instance->Color3 = Color;
}

internal void 
RenderPushClip(v4 Value)
{
 clip_stack *Node = PushArrayZero(FrameArena, clip_stack, 1); 
 Node->V = Value;
 
 Node->Prev = GlobalClipStackTop;
 GlobalClipStackTop = Node;
}

internal void RenderPopClip(void)
{
 GlobalClipStackTop = GlobalClipStackTop->Prev;
}