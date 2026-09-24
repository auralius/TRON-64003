#include "camera.h"
#include "tim.h"
#include "ov7670.h"
#include "ov2640.h"
#include "ov7725.h"
#include "ov5640.h"

#define USE_LCD 1

#if USE_LCD
#include "lcd.h"
#endif

Camera_HandleTypeDef hcamera;
// Resolution table
//----------------------------------------
const uint16_t dvp_cam_resolution[][2] = {
	{0, 0},
	// C/SIF Resolutions
	{88, 72},	/* QQCIF     */
	{176, 144}, /* QCIF      */
	{352, 288}, /* CIF       */
	{88, 60},	/* QQSIF     */
	{176, 120}, /* QSIF      */
	{352, 240}, /* SIF       */
	// VGA Resolutions
	{40, 30},	/* QQQQVGA   */
	{80, 60},	/* QQQVGA    */
	{160, 120}, /* QQVGA     */
	{320, 240}, /* QVGA      */
	{640, 480}, /* VGA       */
	{60, 40},	/* HQQQVGA   */
	{120, 80},	/* HQQVGA    */
	{240, 160}, /* HQVGA     */
	{480, 320}, /* HVGA      */
	// FFT Resolutions
	{64, 32},	/* 64x32     */
	{64, 64},	/* 64x64     */
	{128, 64},	/* 128x64    */
	{128, 128}, /* 128x64    */
	// Other
	{128, 160},	  /* LCD       */
	{128, 160},	  /* QQVGA2    */
	{720, 480},	  /* WVGA      */
	{752, 480},	  /* WVGA2     */
	{800, 600},	  /* SVGA      */
	{1024, 768},  /* XGA       */
	{1280, 1024}, /* SXGA      */
	{1600, 1200}, /* UXGA      */
	{1280, 720},  /* 720P      */
	{1920, 1080}, /* 1080P     */
	{1280, 960},  /* 960P      */
	{2592, 1944}, /* 5MP       */
};

int32_t Camera_WriteReg(Camera_HandleTypeDef *hov, uint8_t regAddr, const uint8_t *pData)
{
	uint8_t tt[2];
	tt[0] = regAddr;
	tt[1] = pData[0];
	if (HAL_I2C_Master_Transmit(hov->hi2c, hov->addr, tt, 2, hov->timeout) == HAL_OK)
	{
		return Camera_OK;
	}
	else
	{
		return camera_ERROR;
	}
}

int32_t Camera_ReadReg(Camera_HandleTypeDef *hov, uint8_t regAddr, uint8_t *pData)
{
	HAL_I2C_Master_Transmit(hov->hi2c, hov->addr + 1, &regAddr, 1, hov->timeout);
	if (HAL_I2C_Master_Receive(hov->hi2c, hov->addr + 1, pData, 1, hov->timeout) == HAL_OK)
	{
		return Camera_OK;
	}
	else
	{
		return camera_ERROR;
	}
}

int32_t Camera_WriteRegb2(Camera_HandleTypeDef *hov, uint16_t reg_addr, uint8_t reg_data)
{
	if (HAL_I2C_Mem_Write(hov->hi2c, hov->addr + 1, reg_addr,
						  I2C_MEMADD_SIZE_16BIT, &reg_data, 1, hov->timeout) == HAL_OK)
	{
		return Camera_OK;
	}
	else
	{
		return camera_ERROR;
	}
}

int32_t Camera_ReadRegb2(Camera_HandleTypeDef *hov, uint16_t reg_addr, uint8_t *reg_data)
{
	if (HAL_I2C_Mem_Read(hov->hi2c, hov->addr + 1, reg_addr,
						 I2C_MEMADD_SIZE_16BIT, reg_data, 1, hov->timeout) == HAL_OK)
	{
		return Camera_OK;
	}
	else
	{
		return camera_ERROR;
	}
}

int32_t Camera_WriteRegList(Camera_HandleTypeDef *hov, const struct regval_t *reg_list)
{
	const struct regval_t *pReg = reg_list;
	while (pReg->reg_addr != 0xFF && pReg->value != 0xFF)
	{
		int write_result = Camera_WriteReg(hov, pReg->reg_addr, &(pReg->value));
		if (write_result != Camera_OK)
		{
			return write_result;
		}
		pReg++;
	}
	return Camera_OK;
}

int32_t Camera_read_id(Camera_HandleTypeDef *hov)
{
	uint8_t temp[2] = {0U, 0U};

	if (hov == NULL || hov->hi2c == NULL)
	{
		return camera_ERROR;
	}

	if (hov->addr != OV5640_ADDRESS)
	{
		/*
		 * Legacy path retained for source compatibility only.
		 * The application no longer uses this path.
		 */
		temp[0] = 0x01U;

		if (Camera_WriteReg(hov, 0xFF, temp) != Camera_OK ||
		    Camera_ReadReg(hov, 0x1C, &temp[0]) != Camera_OK ||
		    Camera_ReadReg(hov, 0x1D, &temp[1]) != Camera_OK)
		{
			hov->manuf_id = 0U;
			hov->device_id = 0U;
			return camera_ERROR;
		}

		hov->manuf_id = ((uint16_t)temp[0] << 8) | temp[1];

		if (Camera_ReadReg(hov, 0x0A, &temp[0]) != Camera_OK ||
		    Camera_ReadReg(hov, 0x0B, &temp[1]) != Camera_OK)
		{
			hov->device_id = 0U;
			return camera_ERROR;
		}
	}
	else
	{
#define OV5640_CHIP_IDH 0x300A
#define OV5640_CHIP_IDL 0x300B

		if (Camera_ReadRegb2(hov, OV5640_CHIP_IDH, &temp[0]) != Camera_OK ||
		    Camera_ReadRegb2(hov, OV5640_CHIP_IDL, &temp[1]) != Camera_OK)
		{
			hov->manuf_id = 0U;
			hov->device_id = 0U;
			return camera_ERROR;
		}

		hov->manuf_id = 0U;
	}

	hov->device_id = ((uint16_t)temp[0] << 8) | temp[1];
	return Camera_OK;
}

void Camera_Reset(Camera_HandleTypeDef *hov)
{
	uint8_t temp;
	temp = 0x01;
	Camera_WriteReg(hov, 0xFF, &temp);
	temp = 0x80;
	Camera_WriteReg(hov, 0x12, &temp);
	HAL_Delay(100);
}

void Camera_XCLK_Set(uint8_t xclktype)
{
	if (xclktype == XCLK_TIM)
	{
		TIM_OC_InitTypeDef sConfigOC = {0};
		GPIO_InitTypeDef GPIO_InitStruct = {0};

		// DeInit TIM1 PWM OutPut
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
		HAL_TIM_PWM_DeInit(&htim1);

		// Init TIM1 Channel 1 12Mhz PWM Output
		htim1.Instance = TIM1;
		htim1.Init.Prescaler = 1 - 1;
		htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
		htim1.Init.Period = 10 - 1;
		htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		htim1.Init.RepetitionCounter = 0;
		htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
		if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
		{
			Error_Handler();
		}

		sConfigOC.OCMode = TIM_OCMODE_PWM1;
		sConfigOC.Pulse = 5;
		sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
		sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
		sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
		sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
		if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
		{
			Error_Handler();
		}

		__HAL_RCC_GPIOA_CLK_ENABLE();
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

#if USE_LCD
		// Init 0.96''LCD Light Timer
		LCD_SoftPWMCtrlInit();
#endif
	}
	else
	{
#if USE_LCD
		// DeInit 0.96''LCD Light Timer
		LCD_SoftPWMCtrlDeInit();
#endif

		// DeInit TIM1 PWM OutPut
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
		HAL_TIM_PWM_DeInit(&htim1);

#if USE_LCD
		// Init TIM1 Channel 2N 10Khz PWM Output
		MX_TIM1_Init();
		LCD_SoftPWMEnable(0);
		HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
#endif

		// Init MCO1 PA8 12Mhz Output
		HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI48, RCC_MCODIV_4);
	}
}

/*
 * Fixed camera target for the STM32H743 CUTE-YOLO hardware.
 *
 * The original WeAct BSP probed OV7670 -> OV2640 -> OV7725 -> OV5640 once.
 * That made warm-reset startup fragile because the OV5640 could still be
 * settling when the single final ID read occurred.
 *
 * This project has a known OV5640, so:
 *   - select only the OV5640 SCCB address
 *   - give XCLK/sensor time to settle
 *   - retry the chip-ID read several times
 *   - only then run the existing OV5640 reset/configuration sequence
 */
#define OV5640_EXPECTED_ID          0x5640U
#define OV5640_BOOT_SETTLE_MS       250U
#define OV5640_ID_RETRY_COUNT       8U
#define OV5640_ID_RETRY_DELAY_MS    75U

int32_t Camera_Init_OV5640(I2C_HandleTypeDef *hi2c, framesize_t framesize)
{
	if (hi2c == NULL)
	{
		return camera_ERROR;
	}

	hcamera.hi2c = hi2c;
	hcamera.addr = OV5640_ADDRESS;
	hcamera.timeout = 100U;
	hcamera.manuf_id = 0U;
	hcamera.device_id = 0U;
	hcamera.framesize = framesize;
	hcamera.pixformat = PIXFORMAT_RGB565;

	/*
	 * MCO1/PA8 XCLK is configured by SystemClock_Config().
	 * On a reset the sensor can remain powered while the MCU/XCLK restarts,
	 * so do not probe immediately.
	 */
	HAL_Delay(OV5640_BOOT_SETTLE_MS);

	for (uint32_t attempt = 0U; attempt < OV5640_ID_RETRY_COUNT; ++attempt)
	{
		hcamera.device_id = 0U;

		if ((Camera_read_id(&hcamera) == Camera_OK) &&
		    (hcamera.device_id == OV5640_EXPECTED_ID))
		{
			/*
			 * ov5640_init() performs its own software-reset/default-register
			 * sequence, including the existing 300 ms post-reset delay.
			 *
			 * The upstream function currently returns 1 unconditionally,
			 * so the known chip ID is our success criterion here.
			 */
			(void)ov5640_init(framesize);
			return Camera_OK;
		}

		/*
		 * Clear a possibly wedged HAL I2C state between attempts.  The handle
		 * retains the CubeMX timing/configuration fields across DeInit/Init.
		 */
		if (hi2c->State != HAL_I2C_STATE_READY)
		{
			(void)HAL_I2C_DeInit(hi2c);
			HAL_Delay(5U);
			(void)HAL_I2C_Init(hi2c);
		}

		HAL_Delay(OV5640_ID_RETRY_DELAY_MS);
	}

	hcamera.addr = OV5640_ADDRESS;
	hcamera.manuf_id = 0U;
	hcamera.device_id = 0U;
	return camera_ERROR;
}

/*
 * Compatibility wrapper: even code that still calls Camera_Init_Device()
 * is now locked to OV5640.  There is no multi-camera probe anymore.
 */
void Camera_Init_Device(I2C_HandleTypeDef *hi2c, framesize_t framesize)
{
	(void)Camera_Init_OV5640(hi2c, framesize);
}
