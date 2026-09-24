#include "usbd_cdc_if.h"
#include "usb_device.h"

#define APP_RX_DATA_SIZE  512U
#define APP_TX_DATA_SIZE  512U
#define CDC_RX_RING_SIZE 4096U

static uint8_t cdc_rx_ring[CDC_RX_RING_SIZE];
static volatile uint32_t cdc_rx_head = 0;
static volatile uint32_t cdc_rx_tail = 0;
static volatile uint8_t cdc_rx_overflow = 0U;

static uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
static uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

static USBD_CDC_LineCodingTypeDef LineCoding = {
    115200U, /* nominal terminal setting; USB CDC itself has no UART baud */
    0x00U,   /* 1 stop bit */
    0x00U,   /* no parity */
    0x08U    /* 8 data bits */
};

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t *pbuf, uint32_t *len);
static int8_t CDC_TransmitCplt_FS(uint8_t *pbuf, uint32_t *len, uint8_t epnum);

USBD_CDC_ItfTypeDef USBD_CDC_fops = {
    CDC_Init_FS,
    CDC_DeInit_FS,
    CDC_Control_FS,
    CDC_Receive_FS,
    CDC_TransmitCplt_FS
};

static int8_t CDC_Init_FS(void)
{
    cdc_rx_head = 0U;
    cdc_rx_tail = 0U;
    cdc_rx_overflow = 0U;

    (void)USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0U);
    (void)USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);

    return (int8_t)USBD_OK;
}

static int8_t CDC_DeInit_FS(void)
{
    cdc_rx_head = 0U;
    cdc_rx_tail = 0U;
    cdc_rx_overflow = 0U;

    return (int8_t)USBD_OK;
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    (void)length;

    switch (cmd)
    {
        case CDC_SET_LINE_CODING:
            LineCoding.bitrate =
                ((uint32_t)pbuf[0]) |
                ((uint32_t)pbuf[1] << 8) |
                ((uint32_t)pbuf[2] << 16) |
                ((uint32_t)pbuf[3] << 24);
            LineCoding.format     = pbuf[4];
            LineCoding.paritytype = pbuf[5];
            LineCoding.datatype   = pbuf[6];
            break;

        case CDC_GET_LINE_CODING:
            pbuf[0] = (uint8_t)(LineCoding.bitrate);
            pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
            pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
            pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
            pbuf[4] = LineCoding.format;
            pbuf[5] = LineCoding.paritytype;
            pbuf[6] = LineCoding.datatype;
            break;

        case CDC_SEND_ENCAPSULATED_COMMAND:
        case CDC_GET_ENCAPSULATED_RESPONSE:
        case CDC_SET_COMM_FEATURE:
        case CDC_GET_COMM_FEATURE:
        case CDC_CLEAR_COMM_FEATURE:
        case CDC_SET_CONTROL_LINE_STATE:
        case CDC_SEND_BREAK:
        default:
            break;
    }

    return (int8_t)USBD_OK;
}

static int8_t CDC_Receive_FS(uint8_t *Buf, uint32_t *Len)
{
    uint32_t head = cdc_rx_head;

    for (uint32_t i = 0; i < *Len; ++i)
    {
        uint32_t next = head + 1U;

        if (next >= CDC_RX_RING_SIZE)
            next = 0U;

        /* Ring full: flag overflow and drop the remainder of this USB packet. */
        if (next == cdc_rx_tail)
        {
            cdc_rx_overflow = 1U;
            break;
        }

        cdc_rx_ring[head] = Buf[i];
        head = next;
    }

    cdc_rx_head = head;

    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);

    return (USBD_OK);
}

static int8_t CDC_TransmitCplt_FS(uint8_t *pbuf, uint32_t *len, uint8_t epnum)
{
    (void)pbuf;
    (void)len;
    (void)epnum;
    return (int8_t)USBD_OK;
}

uint8_t CDC_Transmit_FS(uint8_t *buf, uint16_t len)
{
    USBD_CDC_HandleTypeDef *hcdc =
        (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];

    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
    {
        return (uint8_t)USBD_FAIL;
    }

    if (hcdc == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    if (hcdc->TxState != 0U)
    {
        return (uint8_t)USBD_BUSY;
    }

    (void)USBD_CDC_SetTxBuffer(&hUsbDeviceFS, buf, (uint32_t)len);
    return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}

uint32_t CDC_Available_FS(void)
{
    uint32_t head = cdc_rx_head;
    uint32_t tail = cdc_rx_tail;

    if (head >= tail)
        return head - tail;

    return CDC_RX_RING_SIZE - tail + head;
}


uint32_t CDC_Read_FS(uint8_t *dst, uint32_t max_len)
{
    uint32_t count = 0;

    while ((count < max_len) && (cdc_rx_tail != cdc_rx_head))
    {
        dst[count++] = cdc_rx_ring[cdc_rx_tail];

        uint32_t next = cdc_rx_tail + 1U;

        if (next >= CDC_RX_RING_SIZE)
            next = 0U;

        cdc_rx_tail = next;
    }

    return count;
}

uint8_t CDC_RxOverflow_FS(void)
{
    return cdc_rx_overflow;
}

void CDC_ClearRxOverflow_FS(void)
{
    cdc_rx_overflow = 0U;
}