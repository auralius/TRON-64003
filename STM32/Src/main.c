/* USER CODE BEGIN Header */

/**

  ******************************************************************************

  * @file           : main.c

  * @brief          : Main program body

  ******************************************************************************

  * @attention

  *

  * <h2><center>© Copyright (c) 2020 STMicroelectronics.

  * All rights reserved.</center></h2>

  *

  * This software component is licensed by ST under BSD 3-Clause license,

  * the "License"; You may not use this file except in compliance with the

  * License. You may obtain a copy of the License at:

  *                        opensource.org/licenses/BSD-3-Clause

  *

  ******************************************************************************

  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"

#include "dcmi.h"

#include "dma.h"

#include "i2c.h"

#include "spi.h"

#include "tim.h"

#include "gpio.h"

/* Private includes ----------------------------------------------------------*/

/* USER CODE BEGIN Includes */

#include "usb_otg.h"

#include "usb_device.h"

#include "camera.h"

#include "lcd.h"

#include "cute_bridge.h"

#include "cute_postprocess.h"

#include <string.h>

#include <math.h>

#include <stdio.h>

#include <tk/tkernel.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void knl_start_mtkernel(void);

void SystemClock_Config(void);

void CUTE_SetKernelSync(ID infer_sem, ID lcd_flg);

void CUTE_ControlLoop(void);

void CUTE_LCDLoop(void);

void CUTE_InferLoop(void);

/* Dataset-dump API consumed only by the SERIAL task in usermain.c. */

uint32_t CUTE_SerialSampleGeneration(void);

int CUTE_SerialSampleAcquire(uint32_t *generation,

                             uint32_t *total_us,

                             uint8_t *detection_count);

void CUTE_SerialSampleRelease(void);

const uint8_t *CUTE_SerialSampleFrameData(void);

uint16_t CUTE_SerialSampleFrameWidth(void);

uint16_t CUTE_SerialSampleFrameHeight(void);

uint16_t CUTE_SerialSampleCropX(void);

uint16_t CUTE_SerialSampleCropY(void);

uint16_t CUTE_SerialSampleCropWidth(void);

uint16_t CUTE_SerialSampleCropHeight(void);

int CUTE_SerialSampleDetection(uint8_t index,

                               float *confidence,

                               float *x1, float *y1,

                               float *x2, float *y2);

/* USER CODE BEGIN PFP */

#ifdef TFT96

// QQVGA

#define FrameWidth 160

#define FrameHeight 120

#elif TFT18

// QQVGA2

#define FrameWidth 128

#define FrameHeight 160

#endif

// picture buffer

uint16_t pic[FrameWidth][FrameHeight];

volatile uint32_t DCMI_FrameIsReady = 0;

uint32_t Camera_FPS = 0;

/*
 * Portrait ST7789 display geometry.
 *
 * The camera and detector continue to use the proven 160x120 landscape
 * coordinate system. Only the LCD presentation is rotated:
 *
 *     160x120 source -> 90-degree CCW -> 120x160 portrait preview
 *
 * The 120x160 preview is centered on the 135x240 panel, leaving useful
 * top/bottom space for status text without altering detector geometry.
 */
#define PREVIEW_W      120U
#define PREVIEW_H      160U
#define PREVIEW_X      ((LCD_WIDTH  - PREVIEW_W) / 2U)
#define PREVIEW_Y      ((LCD_HEIGHT - PREVIEW_H) / 2U)

/*
 * Immutable copy of one complete camera frame.
 *
 * DCMI stays in CONTINUOUS mode all the time. On a K1 request, the next
 * completed 160x120 frame is copied here. This avoids stopping/restarting
 * DCMI and keeps inference/dataset coordinates in the original landscape
 * camera frame.
 */
static uint16_t frozen_pic[FrameWidth * FrameHeight];

/*
 * CUTE-YOLO preprocessing target.
 *
 * The detector sees a centered 120x120 square crop from the frozen
 * 160x120 camera frame, resized with bilinear interpolation to 128x128
 * grayscale. This path is intentionally unchanged by the LCD migration.
 */
#define CUTE_INPUT_W        128U
#define CUTE_INPUT_H        128U
#define CUTE_CROP_W         120U
#define CUTE_CROP_H         120U

/*
 * The new 135-pixel-wide panel can display the complete 128x128 detector
 * input without scaling. This function is diagnostic only.
 */
#define CUTE_LCD_W          128U
#define CUTE_LCD_H          128U

static uint8_t cute_gray[CUTE_INPUT_H][CUTE_INPUT_W];
static uint16_t cute_lcd_rgb565[CUTE_LCD_W * CUTE_LCD_H];

/*
 * Shared LCD rendering buffer.
 *
 * LIVE/FROZEN view:
 *     160x120 landscape source -> 120x160 portrait
 *
 * Detection view:
 *     source crop -> 120x160 portrait zoom
 *
 * This buffer is used only by the LCD task.
 */
static uint16_t cute_display_rgb565[PREVIEW_W * PREVIEW_H];

#define CUTE_ZOOM_MARGIN_FRAC  0.08f
#define CUTE_ZOOM_MIN_MARGIN   3.0f

typedef struct

{

  float x;

  float y;

  float w;

  float h;

} CuteDisplayCrop;

typedef enum

{

  CAM_LIVE = 0,

  CAM_FREEZE_REQUESTED,

  CAM_FROZEN

} CameraState;

static volatile CameraState camera_state = CAM_LIVE;

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

static void MPU_Config(void)

{

  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */

  HAL_MPU_Disable();

  /* Configure the MPU attributes for the QSPI 256MB without instruction access */

  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;

  MPU_InitStruct.Number           = MPU_REGION_NUMBER0;

  MPU_InitStruct.BaseAddress      = QSPI_BASE;

  MPU_InitStruct.Size             = MPU_REGION_SIZE_256MB;

  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;

  MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;

  MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;

  MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;

  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;

  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;

  MPU_InitStruct.SubRegionDisable = 0x00;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes for the QSPI 8MB (QSPI Flash Size) to Cacheable WT */

  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;

  MPU_InitStruct.Number           = MPU_REGION_NUMBER1;

  MPU_InitStruct.BaseAddress      = QSPI_BASE;

  MPU_InitStruct.Size             = MPU_REGION_SIZE_8MB;

  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO;

  MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;

  MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;

  MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;

  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;

  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;

  MPU_InitStruct.SubRegionDisable = 0x00;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Setup AXI SRAM in Cacheable WB */

  MPU_InitStruct.Enable           = MPU_REGION_ENABLE;

  MPU_InitStruct.BaseAddress      = D1_AXISRAM_BASE;

  MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;

  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

  MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;

  MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;

  MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;

  MPU_InitStruct.Number           = MPU_REGION_NUMBER2;

  MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;

  MPU_InitStruct.SubRegionDisable = 0x00;

  MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enables the MPU */

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

static void CPU_CACHE_Enable(void)

{

  /* Enable I-Cache */

  SCB_EnableICache();

  /* Enable D-Cache */

  SCB_EnableDCache();

}

void LED_Blink(uint32_t Hdelay, uint32_t Ldelay)

{

  HAL_GPIO_WritePin(PE3_GPIO_Port, PE3_Pin, GPIO_PIN_SET);

  HAL_Delay(Hdelay - 1);

  HAL_GPIO_WritePin(PE3_GPIO_Port, PE3_Pin, GPIO_PIN_RESET);

  HAL_Delay(Ldelay - 1);

}

/*

 * Rotate one complete QQVGA RGB565 frame by 180 degrees.

 *

 * This lets the camera module itself be mounted upside-down so the flat cable

 * can leave the connector naturally without an extreme bend.  Rotation is

 * applied at the frame handoff, before either LCD rendering or CUTE-YOLO

 * preprocessing, so display orientation and detector coordinates remain

 * consistent.

 *

 * Pixel byte order is intentionally left untouched.

 */

static void Camera_CopyRotate180(uint16_t *dst, const uint16_t *src)

{

  const uint32_t npix = (uint32_t)FrameWidth * (uint32_t)FrameHeight;

  for (uint32_t i = 0; i < npix; ++i)

  {

    dst[i] = src[npix - 1U - i];

  }

}

/*

 * Scale the complete 160x120 RGB565 camera frame to 107x80.

 * The source is treated as a flat raster because DCMI DMA writes it

 * sequentially, regardless of the legacy pic[][] declaration order.

 */

/*
 * Rotate a mechanically-corrected 160x120 landscape frame 90 degrees CCW
 * into the 120x160 portrait LCD buffer.
 *
 * CONTROL already applies Camera_CopyRotate180() before publishing live or
 * frozen frames, so this function performs display rotation only.
 */
static void LCD_RotateFrameCCW(uint16_t *dst, const uint16_t *src)
{
  for (uint32_t y = 0U; y < (uint32_t)FrameHeight; ++y)
  {
    for (uint32_t x = 0U; x < (uint32_t)FrameWidth; ++x)
    {
      const uint32_t dx = y;
      const uint32_t dy = (uint32_t)FrameWidth - 1U - x;

      dst[dy * PREVIEW_W + dx] =
          src[y * (uint32_t)FrameWidth + x];
    }
  }
}

static void LCD_ShowCameraFit(const uint16_t *src)
{
  LCD_RotateFrameCCW(cute_display_rgb565, src);

  LCD_DrawRGB565((uint16_t)PREVIEW_X,
                 (uint16_t)PREVIEW_Y,
                 cute_display_rgb565,
                 (uint16_t)PREVIEW_W,
                 (uint16_t)PREVIEW_H);
}

static inline uint16_t CameraRGB565ToHost(uint16_t raw)

{

  return (uint16_t)((raw << 8) | (raw >> 8));

}

static inline float RGB565ToGray01(uint16_t raw)

{

  const uint16_t p = CameraRGB565ToHost(raw);

  const float r = (float)((p >> 11) & 0x1FU) / 31.0f;

  const float g = (float)((p >> 5)  & 0x3FU) / 63.0f;

  const float b = (float)( p        & 0x1FU) / 31.0f;

  return 0.299f * r + 0.587f * g + 0.114f * b;

}

/*

 * Frozen 160x120 RGB565 frame

 *       -> centered 120x120 crop (x = 20..139)

 *       -> bilinear resize to 128x128

 *       -> grayscale uint8_t [0,255]

 *

 * The half-pixel sampling convention intentionally matches the ESP32

 * frame_to_tensor() path.  Quantization is intentionally deferred until

 * we load the .cute model and know its input scale/zero-point.

 */

static void CUTE_PreprocessFrozenFrame(void)

{

  const uint16_t *src = frozen_pic;

  const int crop_x = ((int)FrameWidth  - (int)CUTE_CROP_W) / 2;

  const int crop_y = ((int)FrameHeight - (int)CUTE_CROP_H) / 2;

  for (uint32_t y = 0; y < CUTE_INPUT_H; ++y)

  {

    float sy = ((float)y + 0.5f) *

               ((float)CUTE_CROP_H / (float)CUTE_INPUT_H) - 0.5f;

    if (sy < 0.0f)

      sy = 0.0f;

    if (sy > (float)(CUTE_CROP_H - 1U))

      sy = (float)(CUTE_CROP_H - 1U);

    const int y0l = (int)sy;

    const int y1l = (y0l + 1 < (int)CUTE_CROP_H) ? y0l + 1 : y0l;

    const float wy = sy - (float)y0l;

    const int y0 = crop_y + y0l;

    const int y1 = crop_y + y1l;

    for (uint32_t x = 0; x < CUTE_INPUT_W; ++x)

    {

      float sx = ((float)x + 0.5f) *

                 ((float)CUTE_CROP_W / (float)CUTE_INPUT_W) - 0.5f;

      if (sx < 0.0f)

        sx = 0.0f;

      if (sx > (float)(CUTE_CROP_W - 1U))

        sx = (float)(CUTE_CROP_W - 1U);

      const int x0l = (int)sx;

      const int x1l = (x0l + 1 < (int)CUTE_CROP_W) ? x0l + 1 : x0l;

      const float wx = sx - (float)x0l;

      const int x0 = crop_x + x0l;

      const int x1 = crop_x + x1l;

      const float g00 = RGB565ToGray01(src[(uint32_t)y0 * FrameWidth + (uint32_t)x0]);

      const float g10 = RGB565ToGray01(src[(uint32_t)y0 * FrameWidth + (uint32_t)x1]);

      const float g01 = RGB565ToGray01(src[(uint32_t)y1 * FrameWidth + (uint32_t)x0]);

      const float g11 = RGB565ToGray01(src[(uint32_t)y1 * FrameWidth + (uint32_t)x1]);

      const float top    = g00 + wx * (g10 - g00);

      const float bottom = g01 + wx * (g11 - g01);

      const float gray01 = top + wy * (bottom - top);

      int gray8 = (int)(gray01 * 255.0f + 0.5f);

      if (gray8 < 0)

        gray8 = 0;

      if (gray8 > 255)

        gray8 = 255;

      cute_gray[y][x] = (uint8_t)gray8;

    }

  }

}

/*

 * Show the exact 128x128 detector image on the tiny TFT.

 *

 * The detector buffer remains 128x128; only this diagnostic display is

 * reduced to 80x80 so the square fits the 160x80 panel without distortion.

 */

/*
 * Show the exact 128x128 grayscale detector input without resampling.
 *
 * Synthetic RGB565 values are byte-swapped in memory because the low-level
 * LCD path transmits the byte stream exactly as stored, while ordinary
 * uint16_t constants are little-endian on Cortex-M7.
 */
static void LCD_ShowCuteInput(void)
{
  for (uint32_t y = 0U; y < CUTE_LCD_H; ++y)
  {
    for (uint32_t x = 0U; x < CUTE_LCD_W; ++x)
    {
      const uint8_t gray = cute_gray[y][x];

      const uint16_t rgb565 =
          (uint16_t)(((uint16_t)(gray >> 3) << 11) |
                     ((uint16_t)(gray >> 2) << 5)  |
                     ((uint16_t)(gray >> 3)));

      cute_lcd_rgb565[y * CUTE_LCD_W + x] =
          (uint16_t)((rgb565 << 8) | (rgb565 >> 8));
    }
  }

  LCD_FillScreen(BLACK);

  const uint16_t x0 =
      (uint16_t)((LCD_WIDTH - CUTE_LCD_W) / 2U);
  const uint16_t y0 =
      (uint16_t)((LCD_HEIGHT - CUTE_LCD_H) / 2U);

  LCD_DrawRGB565(x0,
                 y0,
                 cute_lcd_rgb565,
                 (uint16_t)CUTE_LCD_W,
                 (uint16_t)CUTE_LCD_H);
}

/* Map a normalized detector x coordinate back to the frozen 160x120 frame. */

static inline float CUTE_DetectorToFrameX(float x)

{

  const float crop_x = ((float)FrameWidth - (float)CUTE_CROP_W) * 0.5f;

  return crop_x + x * (float)CUTE_CROP_W;

}

/* Map a normalized detector y coordinate back to the frozen 160x120 frame. */

static inline float CUTE_DetectorToFrameY(float y)

{

  const float crop_y = ((float)FrameHeight - (float)CUTE_CROP_H) * 0.5f;

  return crop_y + y * (float)CUTE_CROP_H;

}

static inline float CUTE_ClampF(float x, float lo, float hi)

{

  if (x < lo) return lo;

  if (x > hi) return hi;

  return x;

}

/*

 * Build one crop that covers every post-NMS detection.

 *

 * 1. Union all surviving boxes.

 * 2. Add a small breathing margin.

 * 3. Expand the source crop to 4:3. After the 90-degree CCW LCD rotation

 *    that becomes the 3:4 aspect of the 120x160 portrait preview.

 * 4. Shift it inside the source frame where possible.

 *

 * If a large union cannot physically fit the requested crop inside the

 * 160x120 camera image, the rectangle may extend beyond the source; the

 * renderer fills those samples black. This preserves geometry and keeps

 * every detection visible without stretching the image.

 */

static int CUTE_MakeCoveringCrop(const CuteDetectionSet *set,

                                 CuteDisplayCrop *crop)

{

  if (!set || !crop || set->count == 0U)

    return 0;

  float x1 = CUTE_DetectorToFrameX(set->items[0].x1);

  float y1 = CUTE_DetectorToFrameY(set->items[0].y1);

  float x2 = CUTE_DetectorToFrameX(set->items[0].x2);

  float y2 = CUTE_DetectorToFrameY(set->items[0].y2);

  for (uint8_t i = 1; i < set->count; ++i)

  {

    const float bx1 = CUTE_DetectorToFrameX(set->items[i].x1);

    const float by1 = CUTE_DetectorToFrameY(set->items[i].y1);

    const float bx2 = CUTE_DetectorToFrameX(set->items[i].x2);

    const float by2 = CUTE_DetectorToFrameY(set->items[i].y2);

    if (bx1 < x1) x1 = bx1;

    if (by1 < y1) y1 = by1;

    if (bx2 > x2) x2 = bx2;

    if (by2 > y2) y2 = by2;

  }

  float union_w = x2 - x1;

  float union_h = y2 - y1;

  if (union_w < 1.0f) union_w = 1.0f;

  if (union_h < 1.0f) union_h = 1.0f;

  float pad_x = union_w * CUTE_ZOOM_MARGIN_FRAC;

  float pad_y = union_h * CUTE_ZOOM_MARGIN_FRAC;

  if (pad_x < CUTE_ZOOM_MIN_MARGIN) pad_x = CUTE_ZOOM_MIN_MARGIN;

  if (pad_y < CUTE_ZOOM_MIN_MARGIN) pad_y = CUTE_ZOOM_MIN_MARGIN;

  x1 -= pad_x;

  x2 += pad_x;

  y1 -= pad_y;

  y2 += pad_y;

  float w = x2 - x1;

  float h = y2 - y1;

  const float cx = 0.5f * (x1 + x2);

  const float cy = 0.5f * (y1 + y2);

  const float lcd_aspect = (float)FrameWidth / (float)PREVIEW_H;

  if ((w / h) < lcd_aspect)

    w = h * lcd_aspect;

  else

    h = w / lcd_aspect;

  float left = cx - 0.5f * w;

  float top  = cy - 0.5f * h;

  /* Preserve crop size/aspect; shift it into the source where possible. */

  if (w <= (float)FrameWidth)

    left = CUTE_ClampF(left, 0.0f, (float)FrameWidth - w);

  if (h <= (float)FrameHeight)

    top = CUTE_ClampF(top, 0.0f, (float)FrameHeight - h);

  crop->x = left;

  crop->y = top;

  crop->w = w;

  crop->h = h;

  return 1;

}

/* Draw a 2-pixel box in LCD coordinates. */

/*
 * Draw a 2-pixel box in portrait-preview coordinates.
 *
 * Input coordinates are relative to the 120x160 preview. The helper adds
 * PREVIEW_X/PREVIEW_Y so the black status margins remain untouched.
 */
static void CUTE_DrawLCDRect(int x1, int y1, int x2, int y2, uint16_t color)
{
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= (int)PREVIEW_W) x2 = (int)PREVIEW_W - 1;
  if (y2 >= (int)PREVIEW_H) y2 = (int)PREVIEW_H - 1;

  if (x2 <= x1 || y2 <= y1)
    return;

  const uint16_t sx = (uint16_t)(PREVIEW_X + (uint32_t)x1);
  const uint16_t sy = (uint16_t)(PREVIEW_Y + (uint32_t)y1);
  const uint16_t w = (uint16_t)(x2 - x1 + 1);
  const uint16_t h = (uint16_t)(y2 - y1 + 1);

  LCD_FillRect(sx, sy, w, 2U, color);
  LCD_FillRect(sx, (uint16_t)(sy + h - 2U), w, 2U, color);
  LCD_FillRect(sx, sy, 2U, h, color);
  LCD_FillRect((uint16_t)(sx + w - 2U), sy, 2U, h, color);
}

/*

 * Show the RGB565 frozen image zoomed to the union of all detections, while

 * preserving aspect ratio, then draw every post-NMS detection box.

 */

static void LCD_ShowDetectionZoom(const CuteDetectionSet *set)
{
  CuteDisplayCrop crop;

  if (!CUTE_MakeCoveringCrop(set, &crop))
  {
    LCD_ShowCameraFit(frozen_pic);
    return;
  }

  /*
   * Render the selected landscape source crop directly into the portrait
   * 120x160 LCD buffer using the same 90-degree CCW orientation as LIVE.
   *
   * Inverse mapping for CCW display rotation:
   *
   *   source_y <- display_x
   *   source_x <- 1 - display_y
   */
  for (uint32_t y = 0U; y < PREVIEW_H; ++y)
  {
    const float nx =
        1.0f - (((float)y + 0.5f) / (float)PREVIEW_H);
    const float fx = crop.x + nx * crop.w;
    const int sx = (int)fx;

    for (uint32_t x = 0U; x < PREVIEW_W; ++x)
    {
      const float ny =
          ((float)x + 0.5f) / (float)PREVIEW_W;
      const float fy = crop.y + ny * crop.h;
      const int sy = (int)fy;

      if (sx >= 0 && sx < (int)FrameWidth &&
          sy >= 0 && sy < (int)FrameHeight)
      {
        cute_display_rgb565[y * PREVIEW_W + x] =
            frozen_pic[(uint32_t)sy * FrameWidth + (uint32_t)sx];
      }
      else
      {
        cute_display_rgb565[y * PREVIEW_W + x] = 0U;
      }
    }
  }

  LCD_DrawRGB565((uint16_t)PREVIEW_X,
                 (uint16_t)PREVIEW_Y,
                 cute_display_rgb565,
                 (uint16_t)PREVIEW_W,
                 (uint16_t)PREVIEW_H);

  static const uint16_t colors[] = {
      GREEN, CYAN, YELLOW, MAGENTA, WHITE
  };

  for (uint8_t i = 0U; i < set->count; ++i)
  {
    const float sx1 = CUTE_DetectorToFrameX(set->items[i].x1);
    const float sy1 = CUTE_DetectorToFrameY(set->items[i].y1);
    const float sx2 = CUTE_DetectorToFrameX(set->items[i].x2);
    const float sy2 = CUTE_DetectorToFrameY(set->items[i].y2);

    int x1 = (int)(((sy1 - crop.y) / crop.h) *
                   (float)PREVIEW_W + 0.5f);
    int x2 = (int)(((sy2 - crop.y) / crop.h) *
                   (float)PREVIEW_W + 0.5f);

    int y1 = (int)((1.0f - ((sx2 - crop.x) / crop.w)) *
                   (float)PREVIEW_H + 0.5f);
    int y2 = (int)((1.0f - ((sx1 - crop.x) / crop.w)) *
                   (float)PREVIEW_H + 0.5f);

    CUTE_DrawLCDRect(x1, y1, x2, y2, colors[i % 5U]);
  }
}

/*

 * --------------------------------------------------------------------------

 * uT-Kernel milestone 3: final task split

 *

 *   HEARTBEAT  PRI=7   (created in usermain.c)

 *   CONTROL    PRI=8   camera state + K1 + frame handoff

 *   LCD        PRI=9   sole owner of ST7789 rendering

 *   INFER      PRI=10  preprocess + Noodle + decode/NMS

 *

 * DCMI remains in CONTINUOUS mode exactly as in the proven bare-metal build.

 * --------------------------------------------------------------------------

 */

typedef enum

{

	CUTE_RESULT_IDLE = 0,

	CUTE_RESULT_RUNNING,

	CUTE_RESULT_OK,

	CUTE_RESULT_NO_RUNTIME,

	CUTE_RESULT_INFER_ERROR,

	CUTE_RESULT_DECODE_ERROR,

	CUTE_RESULT_SIGNAL_ERROR

} CuteResultStatus;

/*

 * LCD event bits.

 *

 * Event flags are ideal here because LIVE updates may be coalesced while

 * state-changing events (FROZEN / RESULT / CLEAR) remain visible to the LCD

 * task.  There is only one waiter.

 */

#define CUTE_LCD_EVT_LIVE     0x00000001U

#define CUTE_LCD_EVT_FROZEN   0x00000002U

#define CUTE_LCD_EVT_RESULT   0x00000004U

#define CUTE_LCD_EVT_CLEAR    0x00000008U

#define CUTE_LCD_EVT_ALL      (CUTE_LCD_EVT_LIVE | CUTE_LCD_EVT_FROZEN | CUTE_LCD_EVT_RESULT | CUTE_LCD_EVT_CLEAR)

static ID cute_infer_sem = 0;

static ID cute_lcd_flg   = 0;

static volatile uint8_t cute_infer_busy = 0;

static volatile CuteResultStatus cute_result_status = CUTE_RESULT_IDLE;

static CuteDetectionSet cute_result_detections;

/*

 * Total detector latency:

 *   RGB565 frozen frame -> 128x128 gray preprocessing

 *   + Noodle inference

 *   + decode/NMS

 *

 * Stored in microseconds.  DWT CYCCNT is used for sub-millisecond

 * resolution, with HAL tick as a safe fallback.

 */

static volatile uint32_t cute_last_total_us = 0;

static volatile uint32_t cute_last_head_crc = 0;

/*

 * Dataset dump snapshot state.

 *

 * frozen_pic[] and cute_result_detections remain immutable while the SERIAL

 * task holds cute_serial_sample_busy.  CONTROL refuses to return to LIVE

 * during that short USB transfer.

 */

static volatile uint8_t  cute_serial_sample_busy = 0;

static volatile uint32_t cute_sample_generation = 0;

/*

 * Stable, 180-degree-rotated live-frame handoff from CONTROL to LCD.

 *

 * CONTROL only writes this buffer when cute_lcd_live_busy == 0.

 * LCD clears the busy flag after it has finished rendering, so DMA activity

 * in pic[] can never modify the frame currently being drawn.

 */

static uint16_t cute_lcd_live_pic[FrameWidth * FrameHeight];

static volatile uint8_t cute_lcd_live_busy = 0;



/* --------------------------------------------------------------------------

 * High-resolution detector timing

 * -------------------------------------------------------------------------- */

static void CUTE_TimingEnable(void)

{

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  __DSB();

  __ISB();

}

static uint32_t CUTE_CyclesToUs(uint32_t cycles, uint32_t fallback_ms)

{

  const uint32_t hz = HAL_RCC_GetSysClockFreq();

  if (hz != 0U && cycles != 0U)

  {

    const uint64_t us =

        (((uint64_t)cycles * 1000000ULL) + ((uint64_t)hz / 2ULL)) /

        (uint64_t)hz;

    if (us <= 0xFFFFFFFFULL)

      return (uint32_t)us;

  }

  return fallback_ms * 1000U;

}

/* --------------------------------------------------------------------------

 * Immutable sample access for the USB SERIAL task

 * -------------------------------------------------------------------------- */

uint32_t CUTE_SerialSampleGeneration(void)

{

  return cute_sample_generation;

}

int CUTE_SerialSampleAcquire(uint32_t *generation,

                             uint32_t *total_us,

                             uint8_t *detection_count)

{

  if (!generation || !total_us || !detection_count)

    return 0;

  /*

   * Set the lock first, then re-check state.  CONTROL has a higher priority

   * than SERIAL; if it already resumed LIVE, this re-check simply fails.

   */

  cute_serial_sample_busy = 1U;

  __DMB();

  if (camera_state != CAM_FROZEN ||

      cute_infer_busy ||

      cute_result_status != CUTE_RESULT_OK)

  {

    cute_serial_sample_busy = 0U;

    return 0;

  }

  *generation = cute_sample_generation;

  *total_us = cute_last_total_us;

  *detection_count = cute_result_detections.count;

  return 1;

}

void CUTE_SerialSampleRelease(void)

{

  __DMB();

  cute_serial_sample_busy = 0U;

}

const uint8_t *CUTE_SerialSampleFrameData(void)

{

  return (const uint8_t *)frozen_pic;

}

uint16_t CUTE_SerialSampleFrameWidth(void)

{

  return (uint16_t)FrameWidth;

}

uint16_t CUTE_SerialSampleFrameHeight(void)

{

  return (uint16_t)FrameHeight;

}

uint16_t CUTE_SerialSampleCropX(void)

{

  return (uint16_t)(((uint32_t)FrameWidth - CUTE_CROP_W) / 2U);

}

uint16_t CUTE_SerialSampleCropY(void)

{

  return (uint16_t)(((uint32_t)FrameHeight - CUTE_CROP_H) / 2U);

}

uint16_t CUTE_SerialSampleCropWidth(void)

{

  return (uint16_t)CUTE_CROP_W;

}

uint16_t CUTE_SerialSampleCropHeight(void)

{

  return (uint16_t)CUTE_CROP_H;

}

int CUTE_SerialSampleDetection(uint8_t index,

                               float *confidence,

                               float *x1, float *y1,

                               float *x2, float *y2)

{

  if (!cute_serial_sample_busy ||

      index >= cute_result_detections.count ||

      !confidence || !x1 || !y1 || !x2 || !y2)

  {

    return 0;

  }

  *confidence = cute_result_detections.items[index].confidence;

  *x1 = cute_result_detections.items[index].x1;

  *y1 = cute_result_detections.items[index].y1;

  *x2 = cute_result_detections.items[index].x2;

  *y2 = cute_result_detections.items[index].y2;

  return 1;

}

/*

 * usermain() creates the kernel objects and passes their IDs here before

 * application tasks are started.

 */

void CUTE_SetKernelSync(ID infer_sem, ID lcd_flg)

{

	cute_infer_sem = infer_sem;

	cute_lcd_flg   = lcd_flg;

}

/*

 * Dedicated inference task.

 *

 * Only this task calls the Noodle/CUTE runtime.  frozen_pic[] is immutable

 * from the moment CONTROL freezes it until inference completes.

 */

void CUTE_InferLoop(void)

{

	while (1)

	{

		if (tk_wai_sem(cute_infer_sem, 1, TMO_FEVR) < E_OK)

		{

			continue;

		}

		cute_result_status = CUTE_RESULT_RUNNING;

		cute_last_total_us = 0U;

		/*

		 * Total detector timing begins BEFORE preprocessing and ends AFTER

		 * decode/NMS. Camera capture and LCD rendering are intentionally

		 * outside this interval.

		 */

		CUTE_TimingEnable();

		const uint32_t total_t0_cycles = DWT->CYCCNT;

		const uint32_t total_t0_ms = HAL_GetTick();

		CUTE_PreprocessFrozenFrame();

		if (!cute_runtime_ready())

		{

			cute_result_status = CUTE_RESULT_NO_RUNTIME;

		}

		else

		{

			const int infer_ok = cute_runtime_run_gray8(&cute_gray[0][0]);

			if (!infer_ok)

			{

				cute_result_status = CUTE_RESULT_INFER_ERROR;

			}

			else

			{

				if (cute_postprocess_decode(&cute_result_detections))

				{

					cute_result_status = CUTE_RESULT_OK;

				}

				else

				{

					cute_result_status = CUTE_RESULT_DECODE_ERROR;

				}

			}

		}

		const uint32_t total_cycles = DWT->CYCCNT - total_t0_cycles;

		const uint32_t total_ms = HAL_GetTick() - total_t0_ms;

		cute_last_total_us = CUTE_CyclesToUs(total_cycles, total_ms);

		/*

		 * Debug integrity work is deliberately outside the timed detector

		 * path so it does not inflate the reported latency.

		 */

		if (cute_result_status == CUTE_RESULT_OK ||

		    cute_result_status == CUTE_RESULT_DECODE_ERROR)

		{

			cute_last_head_crc = cute_runtime_head_crc32();

		}

		if (cute_result_status == CUTE_RESULT_OK)

		{

			/* One generation == one completed, dumpable inference result. */

			cute_sample_generation++;

		}

		cute_infer_busy = 0;

		/*

		 * Wake LCD directly.  Because LCD is priority 9 and INFER is 10,

		 * result rendering can preempt the inference task immediately after

		 * the inference result is complete.

		 */

		(void)tk_set_flg(cute_lcd_flg, CUTE_LCD_EVT_RESULT);

	}

}

/*

 * Sole LCD owner.

 *

 * No other RTOS task calls ST7789/LCD rendering functions after the kernel

 * starts.  This removes SPI/LCD contention completely.

 */

void CUTE_LCDLoop(void)
{
  uint8_t text[24];

  while (1)
  {
    UINT events = 0U;

    ER ercd = tk_wai_flg(cute_lcd_flg,
                         CUTE_LCD_EVT_ALL,
                         TWF_ORW | TWF_CLR,
                         &events,
                         TMO_FEVR);

    if (ercd < E_OK)
    {
      continue;
    }

    if ((events & CUTE_LCD_EVT_CLEAR) != 0U)
    {
      LCD_FillScreen(BLACK);
    }

    if ((events & CUTE_LCD_EVT_LIVE) != 0U)
    {
      if (camera_state == CAM_LIVE)
      {
        LCD_ShowCameraFit(cute_lcd_live_pic);
      }

      cute_lcd_live_busy = 0;
    }

    if ((events & CUTE_LCD_EVT_FROZEN) != 0U)
    {
      if (camera_state == CAM_FROZEN)
      {
        LCD_ShowCameraFit(frozen_pic);
      }
    }

    if ((events & CUTE_LCD_EVT_RESULT) != 0U)
    {
      if (camera_state == CAM_FROZEN)
      {
        switch (cute_result_status)
        {
        case CUTE_RESULT_OK:
          if (cute_result_detections.count > 0U)
          {
            LCD_ShowDetectionZoom(&cute_result_detections);
          }
          else
          {
            LCD_ShowCameraFit(frozen_pic);
          }

          {
            const uint32_t total_ms =
                (cute_last_total_us + 500U) / 1000U;

            (void)sprintf((char *)text,
                          "T=%lums",
                          (unsigned long)total_ms);

            /* Free 40-pixel top margin above the 120x160 result image. */
            LCD_FillRect(0U, 0U, LCD_WIDTH, PREVIEW_Y, BLACK);
            LCD_ShowString(8U, 12U, 120U, 16U, 12U, text);
          }
          break;

        case CUTE_RESULT_NO_RUNTIME:
          LCD_FillScreen(BLACK);
          LCD_ShowString(8U, 12U, 120U, 16U, 12U,
                         (uint8_t *)"NO RUNTIME");
          break;

        case CUTE_RESULT_INFER_ERROR:
          LCD_FillScreen(BLACK);
          LCD_ShowString(8U, 12U, 120U, 16U, 12U,
                         (uint8_t *)"INFER ERR");
          LCD_ShowString(8U, 30U, 120U, 16U, 12U,
                         (uint8_t *)cute_runtime_error_c());
          break;

        case CUTE_RESULT_DECODE_ERROR:
          LCD_FillScreen(BLACK);
          LCD_ShowString(8U, 12U, 120U, 16U, 12U,
                         (uint8_t *)"DECODE ERR");
          break;

        case CUTE_RESULT_SIGNAL_ERROR:
          LCD_FillScreen(BLACK);
          LCD_ShowString(8U, 12U, 120U, 16U, 12U,
                         (uint8_t *)"SEM ERROR");
          break;

        default:
          break;
        }
      }
    }
  }
}

/*

 * Camera/control task.

 *

 * This task does not touch the ST7789 at all.  Its job is limited to

 * camera state, K1, stable frame copies, and signalling the LCD/INFER tasks.

 */

void CUTE_ControlLoop(void)

{

	static uint8_t k1_armed = 1;

	/*

	 * The long K1 press that entered uT-Kernel may still be held.

	 */

	while (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET)

	{

		tk_dly_tsk(10);

	}

	camera_state = CAM_LIVE;

	DCMI_FrameIsReady = 0;

	cute_infer_busy = 0;

	cute_result_status = CUTE_RESULT_IDLE;

	cute_lcd_live_busy = 0;

	(void)tk_set_flg(cute_lcd_flg, CUTE_LCD_EVT_CLEAR);

	const HAL_StatusTypeDef dcmi_rc =
			HAL_DCMI_Start_DMA(&hdcmi,
					DCMI_MODE_CONTINUOUS,
					(uint32_t)&pic,
					FrameWidth * FrameHeight * 2U / 4U);

	if (dcmi_rc != HAL_OK)
	{
		/*
		 * Camera was identified/configured but DCMI itself did not start.
		 * Use a different blink rate from the OV5640-ID failure above.
		 */
		while (1)
		{
			HAL_GPIO_TogglePin(PE3_GPIO_Port, PE3_Pin);
			HAL_Delay(350U);
		}
	}

	while (1)

	{

		GPIO_PinState k1 = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);

		if ((k1 == GPIO_PIN_SET) && k1_armed)

		{

			tk_dly_tsk(20);

			if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET)

			{

				k1_armed = 0;

				if (camera_state == CAM_LIVE)

				{

					/*

					 * Freeze the NEXT completed DCMI frame.  DMA itself never

					 * stops.

					 */

					camera_state = CAM_FREEZE_REQUESTED;

				}

				else if ((camera_state == CAM_FROZEN) &&

						 !cute_infer_busy &&

						 !cute_serial_sample_busy)

				{

					camera_state = CAM_LIVE;

					cute_result_status = CUTE_RESULT_IDLE;

					/*

					 * Discard any old live snapshot and let CONTROL publish a

					 * fresh one from the next complete DCMI frame.

					 */

					cute_lcd_live_busy = 0;

					(void)tk_set_flg(cute_lcd_flg,

							CUTE_LCD_EVT_CLEAR);

				}

			}

		}

		if (k1 == GPIO_PIN_RESET)

		{

			k1_armed = 1;

		}

		if (DCMI_FrameIsReady)

		{

			DCMI_FrameIsReady = 0;

			if (camera_state == CAM_LIVE)

			{

				/*

				 * LCD may be slower than the camera.  Drop display frames rather

				 * than ever overwrite a frame while LCD is reading it.

				 */

				if (!cute_lcd_live_busy)

				{

					Camera_CopyRotate180(cute_lcd_live_pic,

							(const uint16_t*)pic);

					cute_lcd_live_busy = 1;

					if (tk_set_flg(cute_lcd_flg,

							CUTE_LCD_EVT_LIVE) < E_OK)

					{

						cute_lcd_live_busy = 0;

					}

				}

			}

			else if (camera_state == CAM_FREEZE_REQUESTED)

			{

				/*

				 * Capture exactly one immutable frame, rotated 180 degrees.  pic[] continues to be

				 * refreshed by DCMI in the background.

				 */

				Camera_CopyRotate180(frozen_pic,

						(const uint16_t*)pic);

				camera_state = CAM_FROZEN;

				(void)tk_set_flg(cute_lcd_flg,

						CUTE_LCD_EVT_FROZEN);

				cute_infer_busy = 1;

				cute_result_status = CUTE_RESULT_RUNNING;

				if (tk_sig_sem(cute_infer_sem, 1) < E_OK)

				{

					cute_infer_busy = 0;

					cute_result_status = CUTE_RESULT_SIGNAL_ERROR;

					(void)tk_set_flg(cute_lcd_flg,

							CUTE_LCD_EVT_RESULT);

				}

			}

			else

			{

				/*

				 * CAM_FROZEN:

				 * new DCMI frames are deliberately ignored.

				 */

			}

		}

		tk_dly_tsk(1);

	}

}

/* USER CODE END 0 */

/**

  * @brief  The application entry point.

  * @retval int

  */

int main(void)

{

  /* USER CODE BEGIN 1 */

#ifdef W25Qxx

  SCB->VTOR = QSPI_BASE;

#endif

  MPU_Config();

  CPU_CACHE_Enable();

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  MX_GPIO_Init();

  MX_DMA_Init();

  MX_DCMI_Init();

  MX_I2C1_Init();

  MX_SPI4_Init();

  MX_TIM1_Init();

  MX_USB_OTG_FS_PCD_Init();

  USB_DEVICE_Init();

  /* USER CODE BEGIN 2 */

  /*

   * Initialize the proven 1.14-inch ST7789 in 135x240 portrait mode.

   * The final CUTE build disables the temporary RGBW geometry test pattern.

   * The LCD task becomes the sole display owner after μT-Kernel starts.

   */

  LCD_Test();

  /* Probe the flash-backed .cute package, then preallocate Noodle. */

  const int cute_model_ok = cute_model_probe();

  const int cute_runtime_ok = cute_model_ok ? cute_runtime_begin() : 0;

  /*

   * Do not draw boot-status strings here.  Runtime/model failures are

   * reported by the CUTE inference/LCD state machine when relevant.

   */

  /*
   * This hardware is now explicitly locked to OV5640.
   * Robust startup waits for XCLK/sensor settling and retries the chip ID
   * instead of probing four different OmniVision families once each.
   */
  #ifdef TFT96
  const int32_t camera_init_rc =
      Camera_Init_OV5640(&hi2c1, FRAMESIZE_QQVGA);
  #elif TFT18
  const int32_t camera_init_rc =
      Camera_Init_OV5640(&hi2c1, FRAMESIZE_QQVGA2);
  #endif

  if (camera_init_rc != Camera_OK)
  {
    /*
     * Distinct boot failure: the LCD itself is already initialized, so leave
     * an explicit message rather than entering μT-Kernel with no frame source.
     */
    LCD_FillScreen(BLACK);
    LCD_ShowString(8U, 12U, 120U, 16U, 12U,
                   (uint8_t *)"OV5640 FAIL");

    while (1)
    {
      HAL_GPIO_TogglePin(PE3_GPIO_Port, PE3_Pin);
      HAL_Delay(120U);
    }
  }

  /* Start the application from a clean 135x240 portrait LCD. */

  LCD_FillScreen(BLACK);

  /*

   * Start the RTOS camera/CUTE application immediately after peripheral

   * initialization.  K1 is no longer required to enter uT-Kernel.

   *

   * Once running, K1 keeps its normal application role:

   * LIVE -> freeze next completed frame + infer

   * FROZEN -> return to LIVE after inference completes

   */

  knl_start_mtkernel();

  /*

   * Normal application execution now lives in the usermain() worker tasks.

   * knl_start_mtkernel() is not expected to return during normal operation.

   */

  while (1)

  {

  }

}

/**

 * @brief System Clock Configuration

 * @retval None

 */

void SystemClock_Config(void)

{

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable

  */

  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage

  */

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters

  * in the RCC_OscInitTypeDef structure.

  */

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;

  RCC_OscInitStruct.HSEState = RCC_HSE_ON;

  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  /*

   * STM32H743 rev.V maximum-performance clock:

   *

   *   HSE      = 25 MHz

   *   PLLM     = 5      -> PLL input = 5 MHz

   *   PLLN     = 192    -> VCO       = 960 MHz

   *   PLLP     = 2      -> SYSCLK    = 480 MHz

   *

   * VOS0 is already selected above.

   */

  RCC_OscInitStruct.PLL.PLLM = 5;

  RCC_OscInitStruct.PLL.PLLN = 192;

  RCC_OscInitStruct.PLL.PLLP = 2;

  RCC_OscInitStruct.PLL.PLLQ = 2;

  RCC_OscInitStruct.PLL.PLLR = 2;

  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;

  RCC_OscInitStruct.PLL.PLLFRACN = 0;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)

  {

    Error_Handler();

  }

  /** Initializes the CPU, AHB and APB buses clocks

  */

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2

                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;

  /*

   * Keep buses within the VOS0 maxima:

   *

   *   CPU/SYSCLK = 480 MHz

   *   HCLK       = 240 MHz

   *   APB1..4    = 120 MHz

   */

  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;

  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;

  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;

  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;

  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  /*

   * At VOS0 with a 240 MHz AXI/Flash interface clock, use 4 wait states.

   */

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)

  {

    Error_Handler();

  }

  /*

   * USB OTG FS requires an accurate 48 MHz kernel clock.

   * HSI48 is already enabled above and is also used by MCO1 for the

   * camera XCLK.  Route HSI48 directly to USB without changing the

   * working main PLL or the camera clock.

   */

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;

  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)

  {

    Error_Handler();

  }

  /*

   * Keep the existing camera XCLK unchanged:

   * HSI48 / 4 = 12 MHz.

   */

  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI48, RCC_MCODIV_4);

}

/* USER CODE BEGIN 4 */

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)

{

	static uint32_t count = 0,tick = 0;

	if(HAL_GetTick() - tick >= 1000)

	{

		tick = HAL_GetTick();

		Camera_FPS = count;

		count = 0;

	}

	count ++;

  DCMI_FrameIsReady = 1;

}

/* USER CODE END 4 */

/**

  * @brief  This function is executed in case of error occurrence.

  * @retval None

  */

void Error_Handler(void)

{

  /* USER CODE BEGIN Error_Handler_Debug */

  /* User can add his own implementation to report the HAL error return state */

  while (1)

  {

    LED_Blink(5, 250);

  }

  /* USER CODE END Error_Handler_Debug */

}

#ifdef  USE_FULL_ASSERT

/**

  * @brief  Reports the name of the source file and the source line number

  *         where the assert_param error has occurred.

  * @param  file: pointer to the source file name

  * @param  line: assert_param error line source number

  * @retval None

  */

void assert_failed(uint8_t *file, uint32_t line)

{

  /* USER CODE BEGIN 6 */

  /* User can add his own implementation to report the file name and line number,

     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* USER CODE END 6 */

}

#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/