/*
 * Engine Types
 */

#ifndef ENGTYP_H
#define ENGTYP_H

#include <stdint.h>
#include <stdlib.h>
#include <libgte.h>
#include <libgpu.h>
#include <libspu.h>

enum en_primType {TYPE_F4, TYPE_FT4, TYPE_G4, TYPE_GT4, TYPE_SPRITE, TYPE_TILE};

struct s_gamePad
{
  struct
  {
    struct
    {
      uint8_t status:8;
    } byte;
    
  } first;
  
  union
  {
    struct
    {
      uint8_t recvSize:4;
      uint8_t type:4;
    } nibble;
    uint8_t byte;
    
  } second;
  
  union
  {
    struct
    {
      uint8_t select:1;
      uint8_t na2:1;
      uint8_t na1:1;
      uint8_t start:1;
      uint8_t up:1;
      uint8_t right:1;
      uint8_t down:1;
      uint8_t left:1;
    } bit;
    uint8_t byte;
	  
  } third;
  
  union
  {
    struct
    {
      uint8_t l2:1;
      uint8_t r2:1;
      uint8_t l1:1;
      uint8_t r1:1;
      uint8_t triangle:1;
      uint8_t circle:1;
      uint8_t ex:1;
      uint8_t square:1;
    } bit;
    uint8_t byte;
	  
  } fourth;
};

struct s_xmlData
{
  char string[256];
};

struct s_primitive
{
  void *data;
  enum en_primType type;
};

struct s_buffer
{
  struct s_primitive *p_primitive;
  unsigned long *p_ot;
  DISPENV disp;
  DRAWENV draw;
};

struct s_vertex
{
  int32_t x;
  int32_t y;
};

struct s_dimensions
{
  uint32_t w;
  uint32_t h;
};

struct s_color
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct s_texture
{
  uint32_t size;
  
  char file[256];
  
  struct s_vertex vertex0;
  struct s_dimensions dimensions;
  
  uint8_t *p_data;
};

struct s_primParam
{
  enum en_primType type; 
  
  struct s_vertex vertex0;
  struct s_vertex vertex1;
  struct s_vertex vertex2;
  struct s_vertex vertex3;
  
  struct s_color color0;
  struct s_color color1;
  struct s_color color2;
  struct s_color color3;
  
  struct s_dimensions size;
  
  struct s_texture *p_texture;
};

struct s_environment
{
  int primCur;
  int primSize;
  int otSize;
  int bufSize;
  int prevTime;
  
  struct s_primParam *p_primParam;
  
  struct s_buffer buffer[DOUBLE_BUF];
  
  struct s_buffer *p_currBuffer;
  
  struct 
  {
    char *p_title;
    char *p_message;
    int  *p_data;
  } envMessage;
  
  struct
  {
    struct s_gamePad one;
    struct s_gamePad two;
  } gamePad;
  
  SpuCommonAttr soundAttr;
};

#endif //ENGTYP_H