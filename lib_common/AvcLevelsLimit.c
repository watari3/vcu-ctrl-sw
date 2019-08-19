/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#include "AvcLevelsLimit.h"
#include "Utils.h"
#include "LevelLimit.h"
#include "BufConst.h"
#include <assert.h>

/****************************************************************************/
bool AL_AVC_CheckLevel(int level)
{
  return ((level >= 10) && (level <= 13))
         || ((level >= 20) && (level <= 22))
         || ((level >= 30) && (level <= 32))
         || ((level >= 40) && (level <= 42))
         || ((level >= 50) && (level <= 52))
         || ((level >= 60) && (level <= 62));
}

/****************************************************************************/
static uint32_t AL_AVC_GetMaxMBperSec(int level)
{
  switch(level)
  {
  case 9:
  case 10: return 1485;
  case 11: return 3000;
  case 12: return 6000;
  case 13:
  case 20: return 11880;
  case 21: return 19800;
  case 22: return 20250;
  case 30: return 40500;
  case 31: return 108000;
  case 32: return 216000;
  case 40:
  case 41: return 245760;
  case 42: return 522240;
  case 50: return 589824;
  case 51: return 983040;
  case 52: return 2073600;
  case 60: return 4177920;
  case 61: return 8355840;
  case 62:
  default: return 16711680;
  }
}

/****************************************************************************/
static uint32_t AL_AVC_GetSliceRate(int level)
{
  switch(level)
  {
  case 9:
  case 10:
  case 11:
  case 12:
  case 13:
  case 20:
  case 21:
  case 22: return 0;
  case 30: return 22;
  case 31:
  case 32:
  case 40: return 60;
  case 41:
  case 42:
  case 50:
  case 51:
  case 52:
  case 60:
  case 61:
  case 62:
  default: return 24;
  }
}

/****************************************************************************/
enum Profile
{
  _NONE = 0,
  CAVLC_444 = 44,
  BASELINE = 66,
  CONSTRAINED_BASELINE = 67,
  MAIN = 77,
  EXTENDED = 88,
  HIGH = 100,
  HIGH_10 = 110,
  HIGH_422 = 122,
  HIGH_444_PREDICTIVE = 244,
  MULTIVIEW_HIGH = 118,
  STEREO_HIGH = 128
};

/****************************************************************************/
uint32_t AL_AVC_GetMaxNumberOfSlices(int profile, int level, int numUnitInTicks, int timeScale, int numMbsInPic)
{
  uint32_t maxMBPS = AL_AVC_GetMaxMBperSec(level);
  uint32_t sliceRate = AL_AVC_GetSliceRate(level);

  if(sliceRate == 0)
    return numMbsInPic;

  if(profile == BASELINE || profile == CONSTRAINED_BASELINE || profile == EXTENDED)
    return numMbsInPic;

  return Min(maxMBPS * 2 * numUnitInTicks / timeScale / sliceRate, numMbsInPic);
}

/****************************************************************************/
uint32_t AL_AVC_GetMaxCPBSize(int level)
{
  switch(level)
  {
  case 10: return 175u;
  case 9:  return 350u;
  case 11: return 500u;
  case 12: return 1000u;
  case 13: return 2000u;
  case 20: return 2000u;
  case 21: return 4000u;
  case 22: return 4000u;
  case 30: return 10000u;
  case 31: return 14000u;
  case 32: return 20000u;
  case 40: return 25000u;
  case 41: return 62500u;
  case 42: return 62500u;
  case 50: return 135000u;
  case 51: return 240000u;
  case 52: return 240000u;
  case 60: return 240000u;
  case 61: return 480000u;
  case 62:
  default: return 800000u;
  }
}

/******************************************************************************/
static int getMaxDpbMBs(int iLevel)
{
  switch(iLevel)
  {
  case 9: /* bConstrSet3 */
  case 10: return 396;
  case 11: return 900;
  case 12:
  case 13:
  case 20: return 2376;
  case 21: return 4752;
  case 22:
  case 30: return 8100;
  case 31: return 18000;
  case 32: return 20480;
  case 40:
  case 41: return 32768;
  case 42: return 34816;
  case 50: return 110400;
  case 51:
  case 52: return 184320;
  case 60:
  case 61:
  case 62:
  default: return 696320;
  }
}

/******************************************************************************/
uint32_t AL_AVC_GetMaxDPBSize(int iLevel, int iWidth, int iHeight)
{
  assert(iWidth);
  assert(iHeight);

  int iMaxDpbMbs = getMaxDpbMBs(iLevel);
  int const iNumMbs = ((iWidth / 16) * (iHeight / 16));
  return Clip3(iMaxDpbMbs / iNumMbs, 2, MAX_REF);
}

/*************************************************************************/
// Max MB per frame
static const AL_TLevelLimit AVC_MAX_FRAME_MB[] =
{
  { 99u, 10u },
  { 396u, 11u },
  { 792u, 21u },
  { 1620u, 22u },
  { 3600u, 31u },
  { 5120u, 32u },
  { 8192u, 40u },
  { 8704u, 42u },
  { 22080u, 50u },
  { 36864u, 51u },
  { 139264u, 60u },
};

// Max MB Rate
static const AL_TLevelLimit AVC_MAX_MB_RATE[] =
{
  { 1485u, 10u },
  { 3000u, 11u },
  { 6000u, 12u },
  { 11880u, 13u },
  { 19800u, 21u },
  { 20250u, 22u },
  { 40500u, 30u },
  { 108000u, 31u },
  { 216000u, 32u },
  { 245760u, 40u },
  { 522240u, 42u },
  { 589824u, 50u },
  { 983040u, 51u },
  { 2073600u, 52u },
  { 4177920u, 60u },
  { 8355840u, 61u },
  { 16711680u, 62u }
};

// Max Video processing rate
static const AL_TLevelLimit AVC_MAX_VIDEO_BITRATE[] =
{
  { 64u, 10u },
  { 128u, 9u }, // 1b
  { 192u, 11u },
  { 384u, 12u },
  { 768u, 13u },
  { 2000u, 20u },
  { 4000u, 21u },
  { 10000u, 30u },
  { 14000u, 31u },
  { 20000u, 32u },
  { 50000u, 41u },
  { 135000u, 50u },
  { 240000u, 51u },
  { 480000u, 61u },
  { 800000u, 62u }
};

// Max Video processing rate
const AL_TLevelLimit AVC_MAX_VIDEO_DPB_SIZE[] =
{
  { 396u, 10u },
  { 900u, 11u },
  { 2376u, 12u },
  { 4752u, 21u },
  { 8100u, 22u },
  { 18000u, 31u },
  { 20480u, 32u },
  { 32768u, 40u },
  { 34816u, 42u },
  { 110400u, 50u },
  { 184320u, 51u },
  { 696320u, 60u }
};

#define NUM_LIMIT(array) (sizeof(array) / sizeof(AL_TLevelLimit))

/*************************************************************************/
uint8_t AL_AVC_GetLevelFromFrameSize(int numMbPerFrame)
{
  return AL_GetRequiredLevel(numMbPerFrame, AVC_MAX_FRAME_MB, NUM_LIMIT(AVC_MAX_FRAME_MB));
}

/*************************************************************************/
uint8_t AL_AVC_GetLevelFromMBRate(int mbRate)
{
  return AL_GetRequiredLevel(mbRate, AVC_MAX_MB_RATE, NUM_LIMIT(AVC_MAX_MB_RATE));
}

/*************************************************************************/
uint8_t AL_AVC_GetLevelFromBitrate(int bitrate)
{
  return AL_GetRequiredLevel(bitrate, AVC_MAX_VIDEO_BITRATE, NUM_LIMIT(AVC_MAX_VIDEO_BITRATE));
}

/*************************************************************************/
uint8_t AL_AVC_GetLevelFromDPBSize(int dpbSize)
{
  return AL_GetRequiredLevel(dpbSize, AVC_MAX_VIDEO_DPB_SIZE, NUM_LIMIT(AVC_MAX_VIDEO_DPB_SIZE));
}

