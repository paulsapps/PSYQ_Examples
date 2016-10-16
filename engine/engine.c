/*
 * Written By: John Convertino
 * 
 * Primitive engine, simple methods for getting primitives on screen, along with some other neat features.
 * 
 * Can Do:
 * 	-Memory Card Access
 * 	-Texture From CD
 * 	-Sprites, Tiles, and all polys.
 *	-Add textures to correct type 
 * 	-Game Pad
 * 
 * Can NOT Do:
 * 	-3D
 * 	-Sound
 * 
 * Version: 1.0
 * 
*/

#include "engine.h"

u_long __ramsize = 0x00200000;  //force 2 megabytes of RAM
u_long __stacksize = 0x00004000; //force 16 kilobytes of stack

//utility functions
void swapBuffers(struct s_environment *p_env)
{
  p_env->p_currBuffer = (p_env->p_currBuffer == p_env->buffer ? p_env->buffer + 1 : p_env->buffer);
}

//available functions
void initEnv(struct s_environment *p_env, int numPrim)
{
  int index;
  
  p_env->bufSize = DOUBLE_BUF;
  p_env->otSize = (numPrim < 1 ? 1 : numPrim);
  p_env->primSize = p_env->otSize;
  p_env->primCur = 0;
  p_env->prevTime = 0;
  
  for(index = 0; index < p_env->bufSize; index++)
  {
    p_env->buffer[index].p_primitive = calloc(p_env->otSize, sizeof(struct s_primitive));
    p_env->buffer[index].p_ot = calloc(p_env->otSize, sizeof(unsigned long));
  }
  
  p_env->p_primParam = calloc(p_env->otSize, sizeof(struct s_primParam));
  
  // within the BIOS, if the address 0xBFC7FF52 equals 'E', set it as PAL (1). Otherwise, set it as NTSC (0)
  switch(*(char *)0xbfc7ff52=='E')
  {
    case 'E':
      SetVideoMode(MODE_PAL); 
      break;
    default:
      SetVideoMode(MODE_NTSC); 
      break;	
  }
  
  ResetGraph(0);

  for(index = 0; index < p_env->bufSize; index += 2) 
  {
    SetDefDispEnv(&p_env->buffer[index].disp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDrawEnv(&p_env->buffer[index].draw, 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
  }

  for(index = 1; index < p_env->bufSize; index += 2)
  {
    SetDefDispEnv(&p_env->buffer[index].disp, 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDrawEnv(&p_env->buffer[index].draw, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  }

  for(index = 0; index < p_env->bufSize; index++)
  {
    p_env->buffer[index].draw.isbg = 1;
    p_env->buffer[index].draw.r0 = 0;
    p_env->buffer[index].draw.g0 = 0;
    p_env->buffer[index].draw.b0 = 0;
    
    ClearOTag(p_env->buffer[index].p_ot, p_env->otSize);
  }
  
  p_env->p_currBuffer = p_env->buffer;
  
  FntLoad(960, 256); // load the font from the BIOS into VRAM/SGRAM
  SetDumpFnt(FntOpen(5, 20, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 512)); // screen X,Y | max text length X,Y | autmatic background clear 0,1 | max characters
  
  PadInitDirect((u_char *)&p_env->gamePad.one, (u_char *)&p_env->gamePad.two);
  PadStartCom();
  
  SetDispMask(1); 
}


void display(struct s_environment *p_env)
{
  //avoid issues with delayed execution
  while(DrawSync(1));
  VSync(0);
  
  PutDrawEnv(&p_env->p_currBuffer->draw);
  PutDispEnv(&p_env->p_currBuffer->disp);
  
  DrawOTag(p_env->p_currBuffer->p_ot);
  
  //exchange reg and draw buffer, so newly registered ot will be drawn, and used draw buffer can now be used for registration.
  swapBuffers(p_env);
  
  FntPrint("%s\n%s\n%X", p_env->envMessage.p_title, p_env->envMessage.p_message, *p_env->envMessage.p_data);
  FntFlush(-1);
}

u_long *loadTIMfromCD(char *p_path)
{
  int sizeSectors = 0;
  int numRemain = 0;
  int prevNumRemain = 0;
  u_char result = 0;
  
  DslFILE fileInfo;
  u_long *image = NULL;
  
  //CD init 
  DsInit();
  
  if(DsSearchFile(&fileInfo, p_path) <= 0)
  {
    printf("\nFILE SEARCH FAILED\n");
  }
  else
  {
    printf("\nFILE SEARCH SUCCESS\n");
  }
  
  sizeSectors = (fileInfo.size + 2047) / 2048;

  printf("\nSECTOR SIZE: %d %d", sizeSectors, fileInfo.size);
  
  image = malloc(sizeSectors * 2048);
  
  if(image == NULL)
  {
    printf("\nALLOCATION FAILED\n");
  }
  else
  {
    printf("\nMEMORY ALLOCATED\n");
  }
  
  DsRead(&fileInfo.pos, sizeSectors, image, DslModeSpeed);
  
  do
  {
    numRemain = DsReadSync(&result);
    
    if(numRemain != prevNumRemain)
    {
      printf("\nNUM REMAIN: %d\n", numRemain);
      prevNumRemain = numRemain;
    }
  }
  while(numRemain);

  printf("\nREAD COMPLETE\n");
  
  DsClose();
  
  return image;
}

struct s_textureInfo getTIMinfo(u_long *p_address)
{
  struct s_textureInfo textureInfo;
  GsIMAGE timData;
  
  GsGetTimInfo(p_address+1, &timData);
  
  textureInfo.textureID = LoadTPage(timData.pixel, timData.pmode, 0, timData.px, timData.py, timData.pw, timData.ph);
  textureInfo.clutID = LoadClut(timData.clut, timData.cx, timData.cy);
  
  return textureInfo;
}

void populateTPage(struct s_environment *p_env, u_long *p_address[], int len)
{
  int index;
  int buffIndex;
  
  for(index = 0; (index < len) && (index < p_env->otSize); index++)
  {
    if(p_address[index] != NULL)
    {
      p_env->p_primParam[index].textureInfo = getTIMinfo(p_address[index]);
    }
  }
  
  for(buffIndex = 0; buffIndex < p_env->bufSize; buffIndex++)
  {
    for(index = 0; (index < len) && (index < p_env->otSize); index++)
    {
      p_env->buffer[buffIndex].p_primitive[index].type = p_env->p_primParam[index].type;
      
      switch(p_env->buffer[buffIndex].p_primitive[index].type)
      {
	case TYPE_FT4:
	  ((POLY_FT4 *)p_env->buffer[buffIndex].p_primitive[index].data)->tpage = p_env->p_primParam[index].textureInfo.textureID;
	  ((POLY_FT4 *)p_env->buffer[buffIndex].p_primitive[index].data)->clut = p_env->p_primParam[index].textureInfo.clutID;
	  break;
	case TYPE_GT4:
	  ((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data)->tpage = p_env->p_primParam[index].textureInfo.textureID;
	  ((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data)->clut = p_env->p_primParam[index].textureInfo.clutID;
	  break;
	case TYPE_SPRITE:
	  SetDrawTPage(&p_env->p_primParam[index].tpage, 1, 0, p_env->p_primParam[index].textureInfo.textureID);
	  AddPrim(&(p_env->buffer[buffIndex].p_ot[index]), &p_env->p_primParam[index].tpage);
	  break;
	default:
	  printf("\nNon Texture Type at index %d\n", index);
	  break;
      }
    }
  }
}

void populateOT(struct s_environment *p_env)
{
  int index;
  int buffIndex;
  
  for(index = 0; index < p_env->otSize; index++)
  {    
    for(buffIndex = 0; buffIndex < p_env->bufSize; buffIndex++)
    {
      p_env->buffer[buffIndex].p_primitive[index].type = p_env->p_primParam[index].type;
      
      switch(p_env->buffer[buffIndex].p_primitive[index].type)
      {
	case TYPE_SPRITE:
	  SetSprt((SPRT *)p_env->buffer[buffIndex].p_primitive[index].data);
	  setXY0((SPRT *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y);
	  setWH((SPRT *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].primSize.w,  p_env->p_primParam[index].primSize.h);
	  setUV0((SPRT *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].textureVertex0.x, p_env->p_primParam[index].textureVertex0.y);
	  setRGB0((SPRT *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	  break;
	case TYPE_TILE:
	  setTile((TILE *)p_env->buffer[buffIndex].p_primitive[index].data);
	  setXY0((TILE *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y);
	  setWH((TILE *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].primSize.w,  p_env->p_primParam[index].primSize.h);
	  setRGB0((TILE *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	  break;
	case TYPE_F4:
	  SetPolyF4((POLY_F4 *)p_env->buffer[buffIndex].p_primitive[index].data);
	  setXYWH((POLY_F4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);
	  setRGB0((POLY_F4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	  break;
	case TYPE_FT4:
	  SetPolyFT4((POLY_FT4 *)p_env->buffer[buffIndex].p_primitive[index].data);
	  setUVWH((POLY_FT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].textureVertex0.x, p_env->p_primParam[index].textureVertex0.y, p_env->p_primParam[index].textureSize.w, p_env->p_primParam[index].textureSize.h);
	  setXYWH((POLY_FT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);
	  setRGB0((POLY_FT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	  break;
	case TYPE_G4:
	  SetPolyG4((POLY_G4 *)p_env->buffer[buffIndex].p_primitive[index].data);
	  setXYWH((POLY_G4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);       
	  setRGB0((POLY_G4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	  setRGB1((POLY_G4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color1.r, p_env->p_primParam[index].color1.g, p_env->p_primParam[index].color1.b);
	  setRGB2((POLY_G4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color2.r, p_env->p_primParam[index].color2.g, p_env->p_primParam[index].color2.b);
	  setRGB3((POLY_G4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color3.r, p_env->p_primParam[index].color3.g, p_env->p_primParam[index].color3.b);
	  break;
	case TYPE_GT4:
	  SetPolyGT4((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data);
	  setUVWH((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].textureVertex0.x, p_env->p_primParam[index].textureVertex0.y, p_env->p_primParam[index].textureSize.w, p_env->p_primParam[index].textureSize.h);
	  setXYWH((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);      
	  setRGB0((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	  setRGB1((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color1.r, p_env->p_primParam[index].color1.g, p_env->p_primParam[index].color1.b);
	  setRGB2((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color2.r, p_env->p_primParam[index].color2.g, p_env->p_primParam[index].color2.b);
	  setRGB3((POLY_GT4 *)p_env->buffer[buffIndex].p_primitive[index].data, p_env->p_primParam[index].color3.r, p_env->p_primParam[index].color3.g, p_env->p_primParam[index].color3.b);
	  break;
	default:
	  printf("\nERROR, NO TYPE DEFINED AT INDEX %d\n", index);
	  break;
      }
      
      AddPrim(&(p_env->buffer[buffIndex].p_ot[index]), p_env->buffer[buffIndex].p_primitive[index].data);
    }
  }
}

void updatePrim(struct s_environment *p_env)
{
  int index;
  
  for(index = 0; index < p_env->otSize; index++)
  {
    switch(p_env->p_currBuffer->p_primitive[index].type)
    {
      case TYPE_SPRITE:
	setXY0((SPRT *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y);
	setWH((SPRT *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].primSize.w,  p_env->p_primParam[index].primSize.h);
	setUV0((SPRT *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].textureVertex0.x, p_env->p_primParam[index].textureVertex0.y);
	setRGB0((SPRT *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	break;
      case TYPE_TILE:
	setXY0((TILE *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y);
	setWH((TILE *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].primSize.w,  p_env->p_primParam[index].primSize.h);
	setRGB0((TILE *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	break;
      case TYPE_F4:
	setXYWH((POLY_F4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);
	setRGB0((POLY_F4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	break;
      case TYPE_FT4:
	setUVWH((POLY_FT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].textureVertex0.x, p_env->p_primParam[index].textureVertex0.y, p_env->p_primParam[index].textureSize.w, p_env->p_primParam[index].textureSize.h);
	setXYWH((POLY_FT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);
	setRGB0((POLY_FT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	break;
      case TYPE_G4:
	setXYWH((POLY_G4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);       
	setRGB0((POLY_G4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	setRGB1((POLY_G4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color1.r, p_env->p_primParam[index].color1.g, p_env->p_primParam[index].color1.b);
	setRGB2((POLY_G4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color2.r, p_env->p_primParam[index].color2.g, p_env->p_primParam[index].color2.b);
	setRGB3((POLY_G4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color3.r, p_env->p_primParam[index].color3.g, p_env->p_primParam[index].color3.b);
	break;
      case TYPE_GT4:
	setUVWH((POLY_GT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].textureVertex0.x, p_env->p_primParam[index].textureVertex0.y, p_env->p_primParam[index].textureSize.w, p_env->p_primParam[index].textureSize.h);
	setXYWH((POLY_GT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].vertex0.x, p_env->p_primParam[index].vertex0.y, p_env->p_primParam[index].primSize.w, p_env->p_primParam[index].primSize.h);      
	setRGB0((POLY_GT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color0.r, p_env->p_primParam[index].color0.g, p_env->p_primParam[index].color0.b);
	setRGB1((POLY_GT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color1.r, p_env->p_primParam[index].color1.g, p_env->p_primParam[index].color1.b);
	setRGB2((POLY_GT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color2.r, p_env->p_primParam[index].color2.g, p_env->p_primParam[index].color2.b);
	setRGB3((POLY_GT4 *)p_env->p_currBuffer->p_primitive[index].data, p_env->p_primParam[index].color3.r, p_env->p_primParam[index].color3.g, p_env->p_primParam[index].color3.b);
	break;
      default:
	printf("\nUnknown Type for update at index %d %d\n", index, p_env->p_currBuffer->p_primitive[index].type);
	break;
    }
  }
}

void movPrim(struct s_environment *p_env)
{  
  if(p_env->gamePad.one.fourth.bit.circle == 0)
  {
    if(p_env->prevTime == 0 || ((VSync(-1) - p_env->prevTime) > 60))
    {
      p_env->primCur = (p_env->primCur + 1) % p_env->otSize;
      p_env->prevTime = VSync(-1);
    }
  }
  
  if(p_env->gamePad.one.fourth.bit.ex == 0)
  {
    if(p_env->prevTime == 0 || ((VSync(-1) - p_env->prevTime) > 60))
    {
      p_env->p_primParam[p_env->primCur].color0.r = rand() % 256;
      p_env->p_primParam[p_env->primCur].color0.g = rand() % 256;
      p_env->p_primParam[p_env->primCur].color0.b = rand() % 256;
      p_env->prevTime = VSync(-1);
    }
  }
  
  if(p_env->gamePad.one.third.bit.up == 0)
  {
    if(p_env->p_primParam[p_env->primCur].vertex0.y > 0)
    {
      p_env->p_primParam[p_env->primCur].vertex0.y -= 1;
    }
  }
  
  if(p_env->gamePad.one.third.bit.right == 0)
  {
    if((p_env->p_primParam[p_env->primCur].vertex0.x + p_env->p_primParam[p_env->primCur].primSize.w) < SCREEN_WIDTH)
    {
      p_env->p_primParam[p_env->primCur].vertex0.x += 1;
    }
  }
  
  if(p_env->gamePad.one.third.bit.down == 0)
  {
    if((p_env->p_primParam[p_env->primCur].vertex0.y + p_env->p_primParam[p_env->primCur].primSize.h) < SCREEN_HEIGHT)
    {
      p_env->p_primParam[p_env->primCur].vertex0.y += 1;
    }
  }
  
  if(p_env->gamePad.one.third.bit.left == 0)
  {
    if(p_env->p_primParam[p_env->primCur].vertex0.x > 0)
    {
      p_env->p_primParam[p_env->primCur].vertex0.x -= 1;
    }
  }
  
  updatePrim(p_env);
}

char *memoryCardRead(uint32_t len)
{
  long cmds;
  long result;
  char *phrase = NULL;
  
  PadStopCom();
  
  phrase = malloc(len);
    
  MemCardInit(1);

  MemCardStart();

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardAccept(0);

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  if(MemCardOpen(0, "test", O_RDONLY) != 0)
  {
    printf("\nOpen Issue\n");
  }

  if(MemCardReadData((unsigned long *)phrase, 0, len)  != 0)
  {
    printf("\nRead Issue\n");
  }

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardClose();

  MemCardStop();
  
  PadStartCom();
  
  return phrase;
}

void memoryCardWrite(char *p_phrase, uint32_t len)
{
  long cmds;
  long result;
  
  PadStopCom();
    
  MemCardInit(1);

  MemCardStart();

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardAccept(0);

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  if(MemCardOpen(0, "test", O_WRONLY) != 0)
  {
    printf("\nOpen Issue\n");
  }

  if(MemCardWriteData((unsigned long *)p_phrase, 0, len)  != 0)
  {
    printf("\nWrite Issue\n");
  }

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardClose();

  MemCardStop();
  
  PadStartCom();
}

 