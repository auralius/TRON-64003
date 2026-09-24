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

#include "camera.h"

#include "lcd.h"

#include "cute_bridge.h"

#include "cute_postprocess.h"

#include <string.h>

#include <math.h>
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
void CUTE_AppLoop(void);

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

 * TFT96 is 160x80 while the camera is 160x120 (4:3).

 * 107x80 preserves the camera aspect ratio and is centered on the LCD.

 */

#define PREVIEW_W 107

#define PREVIEW_H 80

static uint16_t preview[PREVIEW_H][PREVIEW_W];

/*

 * Immutable copy of one complete camera frame.

 *

 * DCMI stays in CONTINUOUS mode all the time.  On a K1 request, the next

 * completed 160x120 frame is copied here.  This avoids the HAL DCMI

 * stop/snapshot/restart sequence, which proved unreliable when returning

 * from the frozen state.

 */

static uint16_t frozen_pic[FrameWidth * FrameHeight];

/*

 * CUTE-YOLO preprocessing target.

 *

 * The detector sees a centered 120x120 square crop from the frozen

 * 160x120 camera frame, resized with bilinear interpolation to 128x128

 * grayscale.  This uint8_t buffer is deliberately kept before INT8

 * quantization; once the .cute model is loaded, its input scale and

 * zero-point can be applied directly to these pixels.

 */

#define CUTE_INPUT_W        128U

#define CUTE_INPUT_H        128U

#define CUTE_CROP_W         120U

#define CUTE_CROP_H         120U

/* 80x80 is the largest square that fits on the 160x80 TFT. */

#define CUTE_LCD_W           80U

#define CUTE_LCD_H           80U

static uint8_t cute_gray[CUTE_INPUT_H][CUTE_INPUT_W];

static uint8_t cute_lcd_rgb565[CUTE_LCD_W * CUTE_LCD_H * 2U];

/* Full-screen 160x80 RGB565 zoom buffer for detection display. */

static uint16_t cute_zoom_rgb565[PREVIEW_H][FrameWidth];

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

static CameraState camera_state = CAM_LIVE;

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

 * Scale the complete 160x120 RGB565 camera frame to 107x80.

 * The source is treated as a flat raster because DCMI DMA writes it

 * sequentially, regardless of the legacy pic[][] declaration order.

 */

static void LCD_ShowCameraFit(const uint16_t *src)

{

  for (uint32_t y = 0; y < PREVIEW_H; y++)

  {

    const uint32_t sy = (y * FrameHeight) / PREVIEW_H;

    for (uint32_t x = 0; x < PREVIEW_W; x++)

    {

      const uint32_t sx = (x * FrameWidth) / PREVIEW_W;

      preview[y][x] = src[sy * FrameWidth + sx];

    }

  }

  const uint32_t x0 = (ST7735Ctx.Width - PREVIEW_W) / 2U;

  ST7735_FillRGBRect(&st7735_pObj,

                     x0,

                     0,

                     (uint8_t *)preview,

                     PREVIEW_W,

                     PREVIEW_H);

}

/*

 * The camera bytes are already in the byte order expected by the ST7735.

 * When the same two bytes are read as a little-endian uint16_t by the M7,

 * the numerical RGB565 word is byte-swapped.  Swap it back before extracting

 * R/G/B components.  This is the same issue handled by swap565() in the

 * working ESP32 CUTE-YOLO firmware.

 */

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

static void LCD_ShowCuteInput(void)

{

  for (uint32_t y = 0; y < CUTE_LCD_H; ++y)

  {

    const uint32_t sy = (y * CUTE_INPUT_H) / CUTE_LCD_H;

    for (uint32_t x = 0; x < CUTE_LCD_W; ++x)

    {

      const uint32_t sx = (x * CUTE_INPUT_W) / CUTE_LCD_W;

      const uint8_t gray = cute_gray[sy][sx];

      const uint16_t rgb565 =

          (uint16_t)(((uint16_t)(gray >> 3) << 11) |

                     ((uint16_t)(gray >> 2) << 5)  |

                     ((uint16_t)(gray >> 3)));

      const uint32_t i = (y * CUTE_LCD_W + x) * 2U;

      /* ST7735_FillRGBRect() sends the byte stream as supplied. */

      cute_lcd_rgb565[i]     = (uint8_t)(rgb565 >> 8);

      cute_lcd_rgb565[i + 1] = (uint8_t)(rgb565 & 0xFFU);

    }

  }

  ST7735_LCD_Driver.FillRect(&st7735_pObj,

                             0, 0,

                             ST7735Ctx.Width, PREVIEW_H,

                             BLACK);

  const uint32_t x0 = (ST7735Ctx.Width - CUTE_LCD_W) / 2U;

  ST7735_FillRGBRect(&st7735_pObj,

                     x0,

                     0,

                     cute_lcd_rgb565,

                     CUTE_LCD_W,

                     CUTE_LCD_H);

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

 * 3. Expand the crop to the physical LCD aspect ratio (160:80 = 2:1).

 * 4. Shift it inside the source frame where possible.

 *

 * If a very large/tall union cannot physically fit a 2:1 rectangle inside

 * the 160x120 camera image, the rectangle is allowed to extend beyond the

 * source; the renderer fills those samples black.  This preserves geometry

 * and keeps every detection visible without stretching the image.

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

static void CUTE_DrawLCDRect(int x1, int y1, int x2, int y2, uint16_t color)

{

  if (x1 < 0) x1 = 0;

  if (y1 < 0) y1 = 0;

  if (x2 >= (int)FrameWidth) x2 = (int)FrameWidth - 1;

  if (y2 >= (int)PREVIEW_H) y2 = (int)PREVIEW_H - 1;

  if (x2 <= x1 || y2 <= y1) return;

  const uint32_t w = (uint32_t)(x2 - x1 + 1);

  const uint32_t h = (uint32_t)(y2 - y1 + 1);

  ST7735_LCD_Driver.DrawHLine(&st7735_pObj, (uint32_t)x1, (uint32_t)y1, w, color);

  ST7735_LCD_Driver.DrawHLine(&st7735_pObj, (uint32_t)x1, (uint32_t)y2, w, color);

  ST7735_LCD_Driver.DrawVLine(&st7735_pObj, (uint32_t)x1, (uint32_t)y1, h, color);

  ST7735_LCD_Driver.DrawVLine(&st7735_pObj, (uint32_t)x2, (uint32_t)y1, h, color);

  if (x2 - x1 > 3 && y2 - y1 > 3)

  {

    ST7735_LCD_Driver.DrawHLine(&st7735_pObj, (uint32_t)(x1 + 1), (uint32_t)(y1 + 1), w - 2U, color);

    ST7735_LCD_Driver.DrawHLine(&st7735_pObj, (uint32_t)(x1 + 1), (uint32_t)(y2 - 1), w - 2U, color);

    ST7735_LCD_Driver.DrawVLine(&st7735_pObj, (uint32_t)(x1 + 1), (uint32_t)(y1 + 1), h - 2U, color);

    ST7735_LCD_Driver.DrawVLine(&st7735_pObj, (uint32_t)(x2 - 1), (uint32_t)(y1 + 1), h - 2U, color);

  }

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

  for (uint32_t y = 0; y < PREVIEW_H; ++y)

  {

    const float fy = crop.y +

        (((float)y + 0.5f) * crop.h / (float)PREVIEW_H);

    const int sy = (int)fy;

    for (uint32_t x = 0; x < FrameWidth; ++x)

    {

      const float fx = crop.x +

          (((float)x + 0.5f) * crop.w / (float)FrameWidth);

      const int sx = (int)fx;

      if (sx >= 0 && sx < (int)FrameWidth &&

          sy >= 0 && sy < (int)FrameHeight)

      {

        cute_zoom_rgb565[y][x] =

            frozen_pic[(uint32_t)sy * FrameWidth + (uint32_t)sx];

      }

      else

      {

        cute_zoom_rgb565[y][x] = 0U;

      }

    }

  }

  ST7735_FillRGBRect(&st7735_pObj,

                     0, 0,

                     (uint8_t *)cute_zoom_rgb565,

                     FrameWidth, PREVIEW_H);

  static const uint16_t colors[] = {

      GREEN, CYAN, YELLOW, MAGENTA, WHITE

  };

  for (uint8_t i = 0; i < set->count; ++i)

  {

    const float sx1 = CUTE_DetectorToFrameX(set->items[i].x1);

    const float sy1 = CUTE_DetectorToFrameY(set->items[i].y1);

    const float sx2 = CUTE_DetectorToFrameX(set->items[i].x2);

    const float sy2 = CUTE_DetectorToFrameY(set->items[i].y2);

    int x1 = (int)(((sx1 - crop.x) / crop.w) * (float)FrameWidth + 0.5f);

    int y1 = (int)(((sy1 - crop.y) / crop.h) * (float)PREVIEW_H + 0.5f);

    int x2 = (int)(((sx2 - crop.x) / crop.w) * (float)FrameWidth + 0.5f);

    int y2 = (int)(((sy2 - crop.y) / crop.h) * (float)PREVIEW_H + 0.5f);

    CUTE_DrawLCDRect(x1, y1, x2, y2, colors[i % 5U]);

  }

}


/*
 * Run the previously proven camera + CUTE-YOLO state machine as ONE
 * μT-Kernel-managed application context.
 *
 * usermain() calls this function and never returns while the demo is active.
 * We deliberately keep the old camera/inference/LCD structure together for
 * the first RTOS milestone.  Task splitting comes later.
 */
void CUTE_AppLoop(void)
{
	uint8_t text[20];
	static uint8_t k1_armed = 1;

	/*
	 * The same long K1 press that entered μT-Kernel may still be held.
	 * Wait cooperatively until it is released so it is not interpreted
	 * immediately as a freeze request.
	 */
	while (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET)
	{
		tk_dly_tsk(10);
	}

	/* Clear the 160x80 preview area before starting live video. */
	ST7735_LCD_Driver.FillRect(&st7735_pObj,
			0, 0,
			ST7735Ctx.Width, PREVIEW_H,
			BLACK);

	camera_state = CAM_LIVE;
	DCMI_FrameIsReady = 0;

	HAL_DCMI_Start_DMA(&hdcmi,
			DCMI_MODE_CONTINUOUS,
			(uint32_t) &pic,
			FrameWidth * FrameHeight * 2 / 4);

	while (1)
	{
		/*
		 * K1 is active-high in the original WeAct demo:
		 * RESET = released, SET = pressed.
		 *
		 * DCMI intentionally remains in CONTINUOUS mode in all states.
		 * We freeze by copying one completed frame into frozen_pic[].
		 */
		GPIO_PinState k1 = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);

		if ((k1 == GPIO_PIN_SET) && k1_armed)
		{
			tk_dly_tsk(20); /* debounce */

			if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET)
			{
				k1_armed = 0;

				if (camera_state == CAM_LIVE)
				{
					/*
					 * Do not stop DCMI here.
					 * Ask the frame handler below to copy the NEXT complete frame.
					 */
					camera_state = CAM_FREEZE_REQUESTED;
				}
				else if (camera_state == CAM_FROZEN)
				{
					/*
					 * Resume is only a display/state change. Camera DMA has been
					 * running continuously in the background.
					 */
					camera_state = CAM_LIVE;

					ST7735_LCD_Driver.FillRect(&st7735_pObj,
							0, 0,
							ST7735Ctx.Width, PREVIEW_H,
							BLACK);
				}
			}
		}

		/* Re-arm the button only after it has been released. */
		if (k1 == GPIO_PIN_RESET)
		{
			k1_armed = 1;
		}

		/* A complete DCMI frame has arrived. */
		if (DCMI_FrameIsReady)
		{
			DCMI_FrameIsReady = 0;

			if (camera_state == CAM_LIVE)
			{
				LCD_ShowCameraFit((const uint16_t*) pic);

				sprintf((char*) &text, "%luFPS", (unsigned long) Camera_FPS);
				LCD_ShowString(5, 5, 60, 16, 12, text);
			}
			else if (camera_state == CAM_FREEZE_REQUESTED)
			{
				/*
				 * Copy exactly one completed frame into an immutable buffer.
				 * DCMI continues capturing into pic[] in the background.
				 */
				memcpy(frozen_pic, (const void*) pic, sizeof(frozen_pic));
				camera_state = CAM_FROZEN;

				/*
				 * Build the exact square grayscale image that will later be fed
				 * to CUTE-YOLO and show it as an 80x80 diagnostic preview.
				 */
				CUTE_PreprocessFrozenFrame();
				LCD_ShowCuteInput();

				if (cute_runtime_ready())
				{
					const uint32_t infer_t0 = HAL_GetTick();
					const int infer_ok = cute_runtime_run_gray8(
							&cute_gray[0][0]);
					const uint32_t infer_ms = HAL_GetTick() - infer_t0;

					if (infer_ok)
					{
						CuteDetectionSet detections;

						if (cute_postprocess_decode(&detections))
						{
							if (detections.count > 0U)
							{
								/*
								 * Positive detection: zoom the frozen RGB565 frame to one
								 * aspect-preserving crop covering ALL post-NMS boxes, then
								 * draw the individual boxes on top.
								 */
								LCD_ShowDetectionZoom(&detections);
							}
							else
							{
								/* No positive detection: keep a full-frame color view. */
								LCD_ShowCameraFit(frozen_pic);
							}
						}
						else
						{
							LCD_ShowString(3, 5, 120, 16, 12,
									(uint8_t*) "DECODE ERR");
						}

						/*
						 * Keep timing/CRC available to the debugger without covering
						 * the tiny image.
						 */
						volatile uint32_t cute_last_infer_ms = infer_ms;
						volatile uint32_t cute_last_head_crc =
								cute_runtime_head_crc32();
						(void) cute_last_infer_ms;
						(void) cute_last_head_crc;
					}
					else
					{
						LCD_ShowString(3, 5, 75, 16, 12,
								(uint8_t*) "INFER ERR");
						LCD_ShowString(3, 20, 110, 16, 12,
								(uint8_t*) cute_runtime_error_c());
					}
				}
				else
				{
					LCD_ShowString(3, 5, 100, 16, 12,
							(uint8_t*) "NO RUNTIME");
				}
			}
			else
			{
				/*
				 * CAM_FROZEN:
				 * camera frames are still arriving, but we deliberately ignore
				 * them so the LCD and frozen_pic[] remain unchanged.
				 */
			}
		}

		/*
		 * Yield briefly to μT-Kernel.  With only this application context,
		 * this mainly proves cooperative RTOS timing without changing the
		 * camera/CUTE behavior.
		 */
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

  /* USER CODE BEGIN 2 */

  uint8_t text[20];

  LCD_Test();

  /* Probe the independently-flashed .cute package, then preallocate Noodle. */

  const int cute_model_ok = cute_model_probe();

  const int cute_runtime_ok = cute_model_ok ? cute_runtime_begin() : 0;

  if (cute_runtime_ok)

  {

    LCD_ShowString(0, 40, ST7735Ctx.Width, 16, 12,

                   (uint8_t *)"CUTE+NOODLE READY");

  }

  else

  {

    LCD_ShowString(0, 40, ST7735Ctx.Width, 16, 12,

                   (uint8_t *)(cute_model_ok ? cute_runtime_error_c()

                                             : cute_model_status_c()));

  }

  HAL_Delay(1200);

  sprintf((char *)&text, "Camera Not Found");

  LCD_ShowString(0, 58, ST7735Ctx.Width, 16, 16, text);

  //	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);

  //	HAL_Delay(10);

  #ifdef TFT96

	Camera_Init_Device(&hi2c1, FRAMESIZE_QQVGA);

	#elif TFT18

	Camera_Init_Device(&hi2c1, FRAMESIZE_QQVGA2);

	#endif

	//clean Ypos 58

	ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 58, ST7735Ctx.Width, 16, BLACK);

  while (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)

  {

    sprintf((char *)&text, "Camera id:0x%lx   ", (unsigned long)hcamera.device_id);

    LCD_ShowString(0, 58, ST7735Ctx.Width, 16, 12, text);

    LED_Blink(5, 500);

    sprintf((char *)&text, "LongPress K1 to Run");

    LCD_ShowString(0, 58, ST7735Ctx.Width, 16, 12, text);

    LED_Blink(5, 500);

  }

  /*

   * The long press above starts the demo. Wait for K1 to be released so

   * that the same press is not interpreted as a freeze request.

   */

  knl_start_mtkernel();

  /*
   * Normal application execution now lives in usermain() -> CUTE_AppLoop().
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

  RCC_OscInitStruct.PLL.PLLM = 5;

  RCC_OscInitStruct.PLL.PLLN = 96;

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

  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;

  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;

  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;

  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)

  {

    Error_Handler();

  }

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
