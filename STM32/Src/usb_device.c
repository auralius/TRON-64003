#include "usb_device.h"
#include "main.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

USBD_HandleTypeDef hUsbDeviceFS;

void USB_DEVICE_Init(void)
{
    /*
     * USB FS is clocked from HSI48 in SystemClock_Config().
     * The WeAct board does not use VBUS sensing in the known-good UVC example.
     */
    HAL_PWREx_EnableUSBVoltageDetector();

    if (USBD_Init(&hUsbDeviceFS, &VCP_Desc, 0U) != USBD_OK)
    {
        Error_Handler();
    }

    if (USBD_RegisterClass(&hUsbDeviceFS, USBD_CDC_CLASS) != USBD_OK)
    {
        Error_Handler();
    }

    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_CDC_fops) != USBD_OK)
    {
        Error_Handler();
    }

    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
    {
        Error_Handler();
    }
}
