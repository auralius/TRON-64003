#include "main.h"

#include <tk/tkernel.h>

#include <tm/tmonitor.h>

#include <string.h>

#include <stdint.h>

#include <stdio.h>

#include "usb_device.h"

#include "usbd_cdc.h"

#include "usbd_cdc_if.h"

/* Implemented in main.c. */

extern void CUTE_SetKernelSync(ID infer_sem, ID lcd_flg);

extern void CUTE_ControlLoop(void);

extern void CUTE_LCDLoop(void);

extern void CUTE_InferLoop(void);

/* Dataset snapshot API implemented in main.c. */

extern uint32_t CUTE_SerialSampleGeneration(void);

extern int CUTE_SerialSampleAcquire(uint32_t *generation,

                                    uint32_t *total_us,

                                    uint8_t *detection_count);

extern void CUTE_SerialSampleRelease(void);

extern const uint8_t *CUTE_SerialSampleFrameData(void);

extern uint16_t CUTE_SerialSampleFrameWidth(void);

extern uint16_t CUTE_SerialSampleFrameHeight(void);

extern uint16_t CUTE_SerialSampleCropX(void);

extern uint16_t CUTE_SerialSampleCropY(void);

extern uint16_t CUTE_SerialSampleCropWidth(void);

extern uint16_t CUTE_SerialSampleCropHeight(void);

extern int CUTE_SerialSampleDetection(uint8_t index,

                                      float *confidence,

                                      float *x1, float *y1,

                                      float *x2, float *y2);

/*

 * Lower numerical values are higher μT-Kernel priorities.

 */

#define HEARTBEAT_TASK_PRIORITY   7

#define CONTROL_TASK_PRIORITY     8

#define LCD_TASK_PRIORITY         9

#define INFER_TASK_PRIORITY      10

#define SERIAL_TASK_PRIORITY     11

#define HEARTBEAT_STACK_SIZE   (2U  * 1024U)

#define CONTROL_STACK_SIZE    (16U * 1024U)

#define LCD_STACK_SIZE        (16U * 1024U)

#define INFER_STACK_SIZE      (32U * 1024U)

#define SERIAL_STACK_SIZE      (4U * 1024U)

#define CUTE_SERIAL_SLOT_BYTES   (32U * 1024U)

#define CUTE_SERIAL_ARCH_ID      0xB3E82A71U

#define CUTE_SERIAL_FLAG_MLP32   0x0001U

/* STM32H743 2 MiB internal Flash: Bank 2, Sector 7 = 0x081E0000..0x081FFFFF. */

#define CUTE_FLASH_SLOT_ADDRESS   0x081E0000U

#define CUTE_FLASH_SLOT_BYTES     (128U * 1024U)

static uint8_t cute_serial_model[CUTE_SERIAL_SLOT_BYTES]

    __attribute__((aligned(32)));

static uint8_t heartbeat_stack[HEARTBEAT_STACK_SIZE]

    __attribute__((aligned(8)));

static uint8_t control_stack[CONTROL_STACK_SIZE]

    __attribute__((aligned(8)));

static uint8_t lcd_stack[LCD_STACK_SIZE]

    __attribute__((aligned(8)));

static uint8_t infer_stack[INFER_STACK_SIZE]

    __attribute__((aligned(8)));

static uint8_t serial_stack[SERIAL_STACK_SIZE]

    __attribute__((aligned(8)));





/* ============================================================

 * HEARTBEAT

 * ============================================================ */

static void heartbeat_task(INT stacd, void *exinf)

{

    (void)stacd;

    (void)exinf;

    while (1)

    {

        HAL_GPIO_TogglePin(PE3_GPIO_Port, PE3_Pin);

        /*

         * IMPORTANT:

         * Do not print anything over USB CDC here.

         * CDC is reserved for the binary CUTE serial protocol.

         */

        tk_dly_tsk(1000);

    }

}





/* ============================================================

 * CONTROL

 * ============================================================ */

static void control_task(INT stacd, void *exinf)

{

    (void)stacd;

    (void)exinf;

    CUTE_ControlLoop();

    tk_ext_tsk();

}





/* ============================================================

 * LCD

 * ============================================================ */

static void lcd_task(INT stacd, void *exinf)

{

    (void)stacd;

    (void)exinf;

    CUTE_LCDLoop();

    tk_ext_tsk();

}





/* ============================================================

 * INFERENCE

 * ============================================================ */

static void infer_task(INT stacd, void *exinf)

{

    (void)stacd;

    (void)exinf;

    CUTE_InferLoop();

    tk_ext_tsk();

}





/* ============================================================

 * USB CDC SERIAL

 *

 * Shared USB CDC service:

 *

 *   Model deployment:

 *     MCAP / MBEG / MDAT / MEND / MIN2

 *

 *   Dataset capture:

 *     RDYSAMPLE / STOPSAMPLE

 *     SAMPLE / FRAME / CROP / DET / BOX / END

 *

 * MEND commits the validated package to STM32 internal flash.

 *

 * ============================================================ */

static uint16_t cute_serial_le16(const uint8_t *p)

{

    return (uint16_t)(

        ((uint16_t)p[0]) |

        ((uint16_t)p[1] << 8)

    );

}

static uint32_t cute_serial_le32(const uint8_t *p)

{

    return

        ((uint32_t)p[0]) |

        ((uint32_t)p[1] << 8) |

        ((uint32_t)p[2] << 16) |

        ((uint32_t)p[3] << 24);

}

static uint32_t cute_serial_crc32(

    const uint8_t *data,

    uint32_t length,

    uint8_t zero_header_crc_field

)

{

    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t i = 0U; i < length; ++i)

    {

        uint8_t b = data[i];

        /*

         * The CUTE header CRC is calculated with bytes 124..127

         * treated as zero. Payload CRCs do not use this rule.

         */

        if (zero_header_crc_field &&

            (i >= 124U) &&

            (i < 128U))

        {

            b = 0U;

        }

        crc ^= (uint32_t)b;

        for (uint32_t bit = 0U; bit < 8U; ++bit)

        {

            if (crc & 1U)

            {

                crc = (crc >> 1) ^ 0xEDB88320U;

            }

            else

            {

                crc >>= 1;

            }

        }

    }

    return crc ^ 0xFFFFFFFFU;

}

static int cute_serial_validate_and_build_min2(

    const uint8_t *m,

    uint32_t model_bytes,

    uint32_t capacity_bytes,

    uint8_t slot_letter,

    uint8_t min2_reply[40]

)

{

    if ((m == NULL) ||

        (model_bytes < 128U) ||

        (model_bytes > capacity_bytes))

    {

        return 0;

    }

    if (memcmp(m, "CUTE", 4U) != 0)

    {

        return 0;

    }

    const uint16_t version      = cute_serial_le16(&m[4]);

    const uint16_t header_bytes = cute_serial_le16(&m[6]);

    const uint32_t arch_id      = cute_serial_le32(&m[8]);

    const uint32_t total_bytes  = cute_serial_le32(&m[12]);

    const uint32_t payload_crc  = cute_serial_le32(&m[16]);

    const uint16_t layer_count  = cute_serial_le16(&m[20]);

    const uint16_t flags        = cute_serial_le16(&m[22]);

    const uint8_t hybrid_blocks = m[65];

    const uint8_t encoded_m     = m[66];

    const uint32_t header_crc   = cute_serial_le32(&m[124]);

    if (version != 2U)

    {

        return 0;

    }

    if (arch_id != CUTE_SERIAL_ARCH_ID)

    {

        return 0;

    }

    if (total_bytes != model_bytes)

    {

        return 0;

    }

    if ((header_bytes < 128U) ||

        (header_bytes > total_bytes))

    {

        return 0;

    }

    if ((flags & (uint16_t)(~CUTE_SERIAL_FLAG_MLP32)) != 0U)

    {

        return 0;

    }

    if ((hybrid_blocks < 1U) ||

        (hybrid_blocks > 12U))

    {

        return 0;

    }

    uint8_t reported_m = 0U;

    uint16_t expected_layers = 0U;

    if ((flags & CUTE_SERIAL_FLAG_MLP32) != 0U)

    {

        reported_m = (encoded_m != 0U) ? encoded_m : 1U;

        if (reported_m > 12U)

        {

            return 0;

        }

        expected_layers = (uint16_t)(

            8U + (4U * (uint16_t)hybrid_blocks) + (uint16_t)reported_m

        );

    }

    else

    {

        if (encoded_m != 0U)

        {

            return 0;

        }

        expected_layers = (uint16_t)(

            7U + (4U * (uint16_t)hybrid_blocks)

        );

    }

    if (layer_count != expected_layers)

    {

        return 0;

    }

    const uint32_t expected_header_bytes =

        (128U + ((uint32_t)layer_count * 40U) + 15U) & ~15U;

    if ((uint32_t)header_bytes != expected_header_bytes)

    {

        return 0;

    }

    const uint32_t actual_header_crc =

        cute_serial_crc32(m, header_bytes, 1U);

    if (actual_header_crc != header_crc)

    {

        return 0;

    }

    const uint32_t actual_payload_crc =

        cute_serial_crc32(

            &m[header_bytes],

            total_bytes - header_bytes,

            0U

        );

    if (actual_payload_crc != payload_crc)

    {

        return 0;

    }

    memset(min2_reply, 0, 40U);

    min2_reply[0] = 'M';

    min2_reply[1] = 'I';

    min2_reply[2] = 'N';

    min2_reply[3] = '2';

    min2_reply[4] = slot_letter;

    min2_reply[5] = (uint8_t)version;

    min2_reply[6] = hybrid_blocks;

    min2_reply[7] = reported_m;

    min2_reply[8]  = (uint8_t)layer_count;

    min2_reply[9]  = (uint8_t)(layer_count >> 8);

    min2_reply[10] = (uint8_t)flags;

    min2_reply[11] = (uint8_t)(flags >> 8);

    min2_reply[12] = (uint8_t)total_bytes;

    min2_reply[13] = (uint8_t)(total_bytes >> 8);

    min2_reply[14] = (uint8_t)(total_bytes >> 16);

    min2_reply[15] = (uint8_t)(total_bytes >> 24);

    /* Label field in CUTE v2 header is exactly bytes 24..47. */

    memcpy(&min2_reply[16], &m[24], 24U);

    return 1;

}



/* ============================================================

 * STM32H743 internal-flash commit

 *

 * 0x081E0000 is Bank 2 / Sector 7 on the 2 MiB STM32H743.

 * A sector erase covers exactly the 128 KiB CUTE slot.

 *

 * H7 FLASHWORD programming granularity is 256 bits = 32 bytes.

 * The final partial FLASHWORD is padded with erased 0xFF bytes.

 * ============================================================ */

static int cute_flash_commit(

    const uint8_t *src,

    uint32_t model_bytes

)

{

    if ((src == NULL) ||

        (model_bytes == 0U) ||

        (model_bytes > CUTE_SERIAL_SLOT_BYTES) ||

        (model_bytes > CUTE_FLASH_SLOT_BYTES))

    {

        return 0;

    }

    FLASH_EraseInitTypeDef erase = {0};

    uint32_t sector_error = 0xFFFFFFFFU;

    if (HAL_FLASH_Unlock() != HAL_OK)

    {

        return 0;

    }

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;

    erase.Banks        = FLASH_BANK_2;

    erase.Sector       = FLASH_SECTOR_7;

    erase.NbSectors    = 1U;

    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK)

    {

        (void)HAL_FLASH_Lock();

        return 0;

    }

    for (uint32_t offset = 0U;

         offset < model_bytes;

         offset += 32U)

    {

        uint32_t flashword[8]

            __attribute__((aligned(32)));

        for (uint32_t i = 0U; i < 8U; ++i)

        {

            flashword[i] = 0xFFFFFFFFU;

        }

        uint32_t remaining = model_bytes - offset;

        uint32_t copy_bytes =

            (remaining < 32U) ? remaining : 32U;

        memcpy(

            (uint8_t *)flashword,

            &src[offset],

            copy_bytes

        );

        if (HAL_FLASH_Program(

                FLASH_TYPEPROGRAM_FLASHWORD,

                CUTE_FLASH_SLOT_ADDRESS + offset,

                (uint32_t)(uintptr_t)flashword

            ) != HAL_OK)

        {

            (void)HAL_FLASH_Lock();

            return 0;

        }

    }

    if (HAL_FLASH_Lock() != HAL_OK)

    {

        return 0;

    }

    /*

     * cute_model_probe() may have read this flash slot earlier during boot,

     * so discard cached flash data before the read-back verification.

     *

     * Clean+invalidate is used rather than invalidate-only so dirty SRAM

     * cache lines are not lost.

     */

    __DSB();

    SCB_CleanInvalidateDCache();

    __DSB();

    __ISB();

    const uint8_t *flash_model =

        (const uint8_t *)(uintptr_t)CUTE_FLASH_SLOT_ADDRESS;

    if (memcmp(flash_model, src, model_bytes) != 0)

    {

        return 0;

    }

    return 1;

}



/* ============================================================

 * Optional serial dataset transport

 *

 * Host heartbeat:

 *     RDYSAMPLE\n

 *

 * After each new completed inference while the heartbeat is fresh:

 *     SAMPLE <id> <label> <total_us> <count>\n

 *     FRAME <w> <h> RGB565BE <nbytes>\n

 *     <binary frame bytes>

 *     \n

 *     CROP <x> <y> <w> <h>\n

 *     DET <count>\n

 *     BOX <conf> <x1> <y1> <x2> <y2>\n

 *     ...

 *     END\n

 *

 * This mirrors the ESP32 firmware and is compatible with the existing

 * collect_cute_dataset.py receiver.

 * ============================================================ */

#define CUTE_SAMPLE_READY_WINDOW_MS 2500U

static uint32_t cute_sample_last_ready_ms = 0U;

static uint32_t cute_sample_last_sent_generation = 0U;

static uint32_t cute_sample_id = 0U;

static uint8_t cute_sample_host_seen = 0U;

static int cute_cdc_wait_idle(uint32_t timeout_ms)

{

    const uint32_t t0 = HAL_GetTick();

    for (;;)

    {

        USBD_CDC_HandleTypeDef *hcdc =

            (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;

        if ((hcdc != NULL) && (hcdc->TxState == 0U))

        {

            return 1;

        }

        if ((uint32_t)(HAL_GetTick() - t0) >= timeout_ms)

        {

            return 0;

        }

        tk_dly_tsk(1);

    }

}

static int cute_cdc_send_blocking(const uint8_t *data, uint16_t len)

{

    const uint32_t t0 = HAL_GetTick();

    if ((data == NULL) || (len == 0U))

    {

        return 0;

    }

    for (;;)

    {

        const uint8_t rc = CDC_Transmit_FS((uint8_t *)data, len);

        if (rc == USBD_OK)

        {

            break;

        }

        if (rc != USBD_BUSY)

        {

            return 0;

        }

        if ((uint32_t)(HAL_GetTick() - t0) >= 5000U)

        {

            return 0;

        }

        tk_dly_tsk(1);

    }

    /*

     * CDC_Transmit_FS() queues the supplied pointer asynchronously.

     * Wait for completion before a stack/local text buffer is reused.

     */

    return cute_cdc_wait_idle(5000U);

}

static int cute_cdc_send_text(const char *text)

{

    if (text == NULL)

    {

        return 0;

    }

    const size_t n = strlen(text);

    if ((n == 0U) || (n > 65535U))

    {

        return 0;

    }

    return cute_cdc_send_blocking(

        (const uint8_t *)text,

        (uint16_t)n

    );

}

static void cute_format_unit6(float value, char out[16])

{

    if (!(value >= 0.0f))

    {

        value = 0.0f;

    }

    if (value > 1.0f)

    {

        value = 1.0f;

    }

    uint32_t scaled =

        (uint32_t)(value * 1000000.0f + 0.5f);

    if (scaled > 1000000U)

    {

        scaled = 1000000U;

    }

    (void)snprintf(

        out,

        16U,

        "%lu.%06lu",

        (unsigned long)(scaled / 1000000U),

        (unsigned long)(scaled % 1000000U)

    );

}

static void cute_active_label(char out[25])

{

    const uint8_t *m =

        (const uint8_t *)(uintptr_t)CUTE_FLASH_SLOT_ADDRESS;

    memset(out, 0, 25U);

    if (memcmp(m, "CUTE", 4U) == 0)

    {

        memcpy(out, &m[24], 24U);

        out[24] = '\0';

    }

    if (out[0] == '\0')

    {

        memcpy(out, "object", 7U);

        return;

    }

    /*

     * SAMPLE is whitespace-delimited. Keep one token even if a future

     * model label contains spaces or non-printable bytes.

     */

    for (uint32_t i = 0U; i < 24U && out[i] != '\0'; ++i)

    {

        const unsigned char ch = (unsigned char)out[i];

        if ((ch <= 0x20U) || (ch >= 0x7FU))

        {

            out[i] = '_';

        }

    }

}

static int cute_sample_receiver_ready(void)

{

    if (!cute_sample_host_seen ||

        cute_sample_last_ready_ms == 0U)

    {

        return 0;

    }

    return

        (uint32_t)(HAL_GetTick() - cute_sample_last_ready_ms)

        <= CUTE_SAMPLE_READY_WINDOW_MS;

}

static void cute_sample_process_command(const char *line)

{

    if (line == NULL)

    {

        return;

    }

    if ((strcmp(line, "RDYSAMPLE") == 0) ||

        (strcmp(line, "RDYFRAME") == 0))

    {

        const int was_ready = cute_sample_receiver_ready();

        cute_sample_last_ready_ms = HAL_GetTick();

        /*

         * If the heartbeat had expired, start from the current generation.

         * This matches the ESP32 behavior: reconnecting does not dump a stale

         * inference that happened while no collector was listening.

         */

        if (!was_ready)

        {

            cute_sample_last_sent_generation =

                CUTE_SerialSampleGeneration();

        }

        if (!cute_sample_host_seen)

        {

            cute_sample_host_seen = 1U;

            (void)cute_cdc_send_text(

                "SERIALDATA HOST_READY\n"

            );

        }

    }

    else if ((strcmp(line, "STOPSAMPLE") == 0) ||

             (strcmp(line, "STOPFRAME") == 0))

    {

        cute_sample_last_ready_ms = 0U;

        cute_sample_host_seen = 0U;

        (void)cute_cdc_send_text(

            "SERIALDATA STOPPED\n"

        );

    }

}

static int cute_send_dataset_sample_if_ready(void)

{

    if (!cute_sample_receiver_ready())

    {

        return 0;

    }

    /*

     * Fast path: do not touch the snapshot lock once the current generation

     * has already been sent.

     */

    if (CUTE_SerialSampleGeneration() ==

        cute_sample_last_sent_generation)

    {

        return 0;

    }

    uint32_t generation = 0U;

    uint32_t total_us = 0U;

    uint8_t detection_count = 0U;

    if (!CUTE_SerialSampleAcquire(

            &generation,

            &total_us,

            &detection_count))

    {

        return 0;

    }

    if (generation == cute_sample_last_sent_generation)

    {

        CUTE_SerialSampleRelease();

        return 0;

    }

    const uint16_t frame_w = CUTE_SerialSampleFrameWidth();

    const uint16_t frame_h = CUTE_SerialSampleFrameHeight();

    const uint32_t frame_bytes =

        (uint32_t)frame_w * (uint32_t)frame_h * 2U;

    const uint8_t *frame =

        CUTE_SerialSampleFrameData();

    char label[25];

    char line[128];

    cute_active_label(label);

    int ok = 1;

    (void)snprintf(

        line,

        sizeof(line),

        "SAMPLE %lu %s %lu %u\n",

        (unsigned long)cute_sample_id,

        label,

        (unsigned long)total_us,

        (unsigned)detection_count

    );

    ok = ok && cute_cdc_send_text(line);

    (void)snprintf(

        line,

        sizeof(line),

        "FRAME %u %u RGB565BE %lu\n",

        (unsigned)frame_w,

        (unsigned)frame_h,

        (unsigned long)frame_bytes

    );

    ok = ok && cute_cdc_send_text(line);

    if (ok)

    {

        if (frame_bytes > 65535U)

        {

            ok = 0;

        }

        else

        {

            ok = cute_cdc_send_blocking(

                frame,

                (uint16_t)frame_bytes

            );

        }

    }

    ok = ok && cute_cdc_send_text("\n");

    (void)snprintf(

        line,

        sizeof(line),

        "CROP %u %u %u %u\n",

        (unsigned)CUTE_SerialSampleCropX(),

        (unsigned)CUTE_SerialSampleCropY(),

        (unsigned)CUTE_SerialSampleCropWidth(),

        (unsigned)CUTE_SerialSampleCropHeight()

    );

    ok = ok && cute_cdc_send_text(line);

    (void)snprintf(

        line,

        sizeof(line),

        "DET %u\n",

        (unsigned)detection_count

    );

    ok = ok && cute_cdc_send_text(line);

    for (uint8_t i = 0U;

         ok && i < detection_count;

         ++i)

    {

        float confidence;

        float x1, y1, x2, y2;

        if (!CUTE_SerialSampleDetection(

                i,

                &confidence,

                &x1, &y1,

                &x2, &y2))

        {

            ok = 0;

            break;

        }

        char s_conf[16];

        char s_x1[16];

        char s_y1[16];

        char s_x2[16];

        char s_y2[16];

        cute_format_unit6(confidence, s_conf);

        cute_format_unit6(x1, s_x1);

        cute_format_unit6(y1, s_y1);

        cute_format_unit6(x2, s_x2);

        cute_format_unit6(y2, s_y2);

        (void)snprintf(

            line,

            sizeof(line),

            "BOX %s %s %s %s %s\n",

            s_conf,

            s_x1, s_y1,

            s_x2, s_y2

        );

        ok = cute_cdc_send_text(line);

    }

    if (ok)

    {

        ok = cute_cdc_send_text("END\n");

    }

    CUTE_SerialSampleRelease();

    if (ok)

    {

        cute_sample_last_sent_generation = generation;

        cute_sample_id++;

        return 1;

    }

    return 0;

}

static void serial_task(INT stacd, void *exinf)

{

    (void)stacd;

    (void)exinf;

    static uint8_t mcap_reply[16] =

    {

        'M', 'C', 'A', 'P',

        2U,      /* max CUTE version */

        1U,      /* min H */

        12U,     /* max H */

        12U,     /* max M */

        /* 32 KiB = 0x00008000 */

        0x00U, 0x80U, 0x00U, 0x00U,

        /* architecture ID = 0xB3E82A71 */

        0x71U, 0x2AU, 0xE8U, 0xB3U

    };

    static uint8_t mred_reply[8] =

    {

        'M', 'R', 'E', 'D',

        'A', 0U, 0U, 0U

    };

    static uint8_t mack_reply[8] =

    {

        'M', 'A', 'C', 'K',

        0U, 0U, 0U, 0U

    };

    static uint8_t merr_reply[8] =

    {

        'M', 'E', 'R', 'R',

        0U, 0U, 0U, 0U

    };

    static uint8_t mact_reply[8] =

    {

        'M', 'A', 'C', 'T',

        'A', 0U, 0U, 0U

    };

    static uint8_t min2_reply[40];

    enum

    {

        SERIAL_WAIT_CMD = 0,

        SERIAL_WAIT_MBEG_SIZE,

        SERIAL_WAIT_MDAT_LEN,

        SERIAL_WAIT_MDAT_DATA,

        SERIAL_WAIT_TEXT_LINE

    };

    uint8_t state = SERIAL_WAIT_CMD;

    uint8_t cmd[4];

    uint32_t cmd_used = 0U;

    uint8_t size_bytes[4];

    uint32_t size_used = 0U;

    uint8_t len_bytes[2];

    uint32_t len_used = 0U;

    uint32_t expected_model_bytes = 0U;

    uint32_t received_model_bytes = 0U;

    uint16_t current_chunk_bytes = 0U;

    uint16_t current_chunk_received = 0U;

    uint8_t staged_model_valid = 0U;

    char text_command[24];

    uint32_t text_used = 0U;

    for (;;)

    {

        uint8_t b;

        /*

         * Overflow should never happen with the Studio stop-and-wait

         * protocol, but abort the current transaction if it does.

         */

        if (CDC_RxOverflow_FS())

        {

            CDC_ClearRxOverflow_FS();

            state = SERIAL_WAIT_CMD;

            cmd_used = 0U;

            size_used = 0U;

            len_used = 0U;

            expected_model_bytes = 0U;

            received_model_bytes = 0U;

            current_chunk_bytes = 0U;

            current_chunk_received = 0U;

            staged_model_valid = 0U;

            text_used = 0U;

        }

        if (CDC_Read_FS(&b, 1U) != 1U)

        {

            /*

             * Inference may finish while the host is otherwise silent.

             * A fresh RDYSAMPLE heartbeat keeps this send path armed for

             * 2.5 seconds, exactly like the ESP32 implementation.

             */

            (void)cute_send_dataset_sample_if_ready();

            tk_dly_tsk(1);

            continue;

        }





        /* ====================================================

         * Newline-terminated dataset command

         * ==================================================== */

        if (state == SERIAL_WAIT_TEXT_LINE)

        {

            if (b == '\r')

            {

                continue;

            }

            if (b == '\n')

            {

                text_command[text_used] = '\0';

                cute_sample_process_command(text_command);

                text_used = 0U;

                state = SERIAL_WAIT_CMD;

                (void)cute_send_dataset_sample_if_ready();

                continue;

            }

            if (text_used + 1U < sizeof(text_command))

            {

                text_command[text_used++] = (char)b;

            }

            else

            {

                text_used = 0U;

                state = SERIAL_WAIT_CMD;

            }

            continue;

        }

        /* ====================================================

         * Waiting for a 4-byte command

         * ==================================================== */

        if (state == SERIAL_WAIT_CMD)

        {

            cmd[cmd_used++] = b;

            if (cmd_used < 4U)

            {

                continue;

            }

            cmd_used = 0U;

            /*

             * Dataset heartbeat commands are newline-terminated while the

             * model deploy protocol uses fixed four-byte commands.

             * Their first four bytes are unique, so both protocols can

             * safely share the same CDC stream.

             */

            if ((memcmp(cmd, "RDYS", 4U) == 0) ||

                (memcmp(cmd, "RDYF", 4U) == 0) ||

                (memcmp(cmd, "STOP", 4U) == 0))

            {

                memcpy(text_command, cmd, 4U);

                text_used = 4U;

                state = SERIAL_WAIT_TEXT_LINE;

                continue;

            }





            /* ---------------- MCAP ---------------- */

            if (memcmp(cmd, "MCAP", 4U) == 0)

            {

                uint8_t rc;

                do

                {

                    rc = CDC_Transmit_FS(

                        mcap_reply,

                        sizeof(mcap_reply)

                    );

                    if (rc == USBD_BUSY)

                    {

                        tk_dly_tsk(1);

                    }

                } while (rc == USBD_BUSY);

            }





            /* ---------------- MBEG ---------------- */

            else if (memcmp(cmd, "MBEG", 4U) == 0)

            {

                size_used = 0U;

                state = SERIAL_WAIT_MBEG_SIZE;

            }





            /* ---------------- MDAT ---------------- */

            else if (memcmp(cmd, "MDAT", 4U) == 0)

            {

                len_used = 0U;

                state = SERIAL_WAIT_MDAT_LEN;

            }





            /* ---------------- MEND ---------------- */

            else if (memcmp(cmd, "MEND", 4U) == 0)

            {

                uint8_t rc;

                /*

                 * The host has finished sending MDAT packets.

                 * Validate the complete staged package in RAM.

                 *

                 * Flash is touched only after the entire staged package

                 * has passed structural and CRC validation.

                 */

                if ((expected_model_bytes == 0U) ||

                    (received_model_bytes != expected_model_bytes))

                {

                    /* MERR rc = -3 : incomplete transfer */

                    merr_reply[4] = 0xFDU;

                    merr_reply[5] = 0xFFU;

                    merr_reply[6] = 0xFFU;

                    merr_reply[7] = 0xFFU;

                    staged_model_valid = 0U;

                    do

                    {

                        rc = CDC_Transmit_FS(

                            merr_reply,

                            sizeof(merr_reply)

                        );

                        if (rc == USBD_BUSY)

                        {

                            tk_dly_tsk(1);

                        }

                    } while (rc == USBD_BUSY);

                }

                else if (!cute_serial_validate_and_build_min2(

                    cute_serial_model,

                    received_model_bytes,

                    CUTE_SERIAL_SLOT_BYTES,

                    'A',

                    min2_reply

                ))

                {

                    /* MERR rc = -4 : staged CUTE validation failed */

                    merr_reply[4] = 0xFCU;

                    merr_reply[5] = 0xFFU;

                    merr_reply[6] = 0xFFU;

                    merr_reply[7] = 0xFFU;

                    staged_model_valid = 0U;

                    do

                    {

                        rc = CDC_Transmit_FS(

                            merr_reply,

                            sizeof(merr_reply)

                        );

                        if (rc == USBD_BUSY)

                        {

                            tk_dly_tsk(1);

                        }

                    } while (rc == USBD_BUSY);

                }

                else if (!cute_flash_commit(

                    cute_serial_model,

                    received_model_bytes

                ))

                {

                    /* MERR rc = -5 : STM32 flash erase/program/readback failed */

                    merr_reply[4] = 0xFBU;

                    merr_reply[5] = 0xFFU;

                    merr_reply[6] = 0xFFU;

                    merr_reply[7] = 0xFFU;

                    staged_model_valid = 0U;

                    do

                    {

                        rc = CDC_Transmit_FS(

                            merr_reply,

                            sizeof(merr_reply)

                        );

                        if (rc == USBD_BUSY)

                        {

                            tk_dly_tsk(1);

                        }

                    } while (rc == USBD_BUSY);

                }

                else if (!cute_serial_validate_and_build_min2(

                    (const uint8_t *)(uintptr_t)CUTE_FLASH_SLOT_ADDRESS,

                    received_model_bytes,

                    CUTE_FLASH_SLOT_BYTES,

                    'A',

                    min2_reply

                ))

                {

                    /* MERR rc = -6 : programmed flash package failed validation */

                    merr_reply[4] = 0xFAU;

                    merr_reply[5] = 0xFFU;

                    merr_reply[6] = 0xFFU;

                    merr_reply[7] = 0xFFU;

                    staged_model_valid = 0U;

                    do

                    {

                        rc = CDC_Transmit_FS(

                            merr_reply,

                            sizeof(merr_reply)

                        );

                        if (rc == USBD_BUSY)

                        {

                            tk_dly_tsk(1);

                        }

                    } while (rc == USBD_BUSY);

                }

                else

                {

                    /*

                     * MACT is sent only after the package has been:

                     *   1. structurally/CRC validated in RAM,

                     *   2. programmed to STM32 flash,

                     *   3. byte-verified,

                     *   4. structurally/CRC validated again from flash.

                     */

                    staged_model_valid = 1U;

                    do

                    {

                        rc = CDC_Transmit_FS(

                            mact_reply,

                            sizeof(mact_reply)

                        );

                        if (rc == USBD_BUSY)

                        {

                            tk_dly_tsk(1);

                        }

                    } while (rc == USBD_BUSY);

                }

            }





            /* ---------------- MIN2 ---------------- */

            else if (memcmp(cmd, "MIN2", 4U) == 0)

            {

                uint8_t rc;

                if (staged_model_valid)

                {

                    do

                    {

                        rc = CDC_Transmit_FS(

                            min2_reply,

                            sizeof(min2_reply)

                        );

                        if (rc == USBD_BUSY)

                        {

                            tk_dly_tsk(1);

                        }

                    } while (rc == USBD_BUSY);

                }

                else

                {

                    /*

                     * Studio only sends MIN2 after a successful MACT.

                     * Keep a deterministic 40-byte response here so a

                     * protocol mistake fails as a topology mismatch rather

                     * than hanging on a short read.

                     */

                    uint8_t invalid_info[40] = {0};

                    invalid_info[0] = 'M';

                    invalid_info[1] = 'I';

                    invalid_info[2] = 'N';

                    invalid_info[3] = '2';

                    do

                    {

                        rc = CDC_Transmit_FS(

                            invalid_info,

                            sizeof(invalid_info)

                        );

                        if (rc == USBD_BUSY)

                        {

                            tk_dly_tsk(1);

                        }

                    } while (rc == USBD_BUSY);

                }

            }





            continue;

        }





        /* ====================================================

         * MBEG payload:

         *     uint32 model size, little-endian

         * ==================================================== */

        if (state == SERIAL_WAIT_MBEG_SIZE)

        {

            size_bytes[size_used++] = b;

            if (size_used < 4U)

            {

                continue;

            }

            expected_model_bytes =

                ((uint32_t)size_bytes[0]) |

                ((uint32_t)size_bytes[1] << 8) |

                ((uint32_t)size_bytes[2] << 16) |

                ((uint32_t)size_bytes[3] << 24);

            size_used = 0U;

            if ((expected_model_bytes == 0U) ||

                (expected_model_bytes > CUTE_SERIAL_SLOT_BYTES))

            {

                uint8_t rc;

                /* MERR rc = -1 */

                merr_reply[4] = 0xFFU;

                merr_reply[5] = 0xFFU;

                merr_reply[6] = 0xFFU;

                merr_reply[7] = 0xFFU;

                do

                {

                    rc = CDC_Transmit_FS(

                        merr_reply,

                        sizeof(merr_reply)

                    );

                    if (rc == USBD_BUSY)

                    {

                        tk_dly_tsk(1);

                    }

                } while (rc == USBD_BUSY);

                expected_model_bytes = 0U;

                received_model_bytes = 0U;

                staged_model_valid = 0U;

                state = SERIAL_WAIT_CMD;

                continue;

            }

            /*

             * Begin a fresh RAM-staged upload.

             * Flash is deliberately not erased until MEND validation.

             */

            received_model_bytes = 0U;

            staged_model_valid = 0U;

            {

                uint8_t rc;

                do

                {

                    rc = CDC_Transmit_FS(

                        mred_reply,

                        sizeof(mred_reply)

                    );

                    if (rc == USBD_BUSY)

                    {

                        tk_dly_tsk(1);

                    }

                } while (rc == USBD_BUSY);

            }

            state = SERIAL_WAIT_CMD;

            continue;

        }





        /* ====================================================

         * MDAT length:

         *     uint16 chunk length, little-endian

         * ==================================================== */

        if (state == SERIAL_WAIT_MDAT_LEN)

        {

            len_bytes[len_used++] = b;

            if (len_used < 2U)

            {

                continue;

            }

            current_chunk_bytes =

                (uint16_t)(

                    ((uint16_t)len_bytes[0]) |

                    ((uint16_t)len_bytes[1] << 8)

                );

            len_used = 0U;

            current_chunk_received = 0U;

            /*

             * Studio currently uses chunks <= 256 bytes.

             * Also reject anything that would exceed the announced

             * model length or the STM32 model staging buffer.

             */

            if ((current_chunk_bytes == 0U) ||

                (current_chunk_bytes > 256U) ||

                (expected_model_bytes == 0U) ||

                ((received_model_bytes + current_chunk_bytes) >

                    expected_model_bytes) ||

                ((received_model_bytes + current_chunk_bytes) >

                    CUTE_SERIAL_SLOT_BYTES))

            {

                uint8_t rc;

                /* MERR rc = -2 */

                merr_reply[4] = 0xFEU;

                merr_reply[5] = 0xFFU;

                merr_reply[6] = 0xFFU;

                merr_reply[7] = 0xFFU;

                do

                {

                    rc = CDC_Transmit_FS(

                        merr_reply,

                        sizeof(merr_reply)

                    );

                    if (rc == USBD_BUSY)

                    {

                        tk_dly_tsk(1);

                    }

                } while (rc == USBD_BUSY);

                state = SERIAL_WAIT_CMD;

                continue;

            }

            state = SERIAL_WAIT_MDAT_DATA;

            continue;

        }





        /* ====================================================

         * MDAT binary payload

         * ==================================================== */

        if (state == SERIAL_WAIT_MDAT_DATA)

        {

            cute_serial_model[

                received_model_bytes + current_chunk_received

            ] = b;

            current_chunk_received++;

            if (current_chunk_received < current_chunk_bytes)

            {

                continue;

            }

            received_model_bytes += current_chunk_bytes;

            /*

             * MACK + uint32 cumulative received byte count.

             */

            mack_reply[4] =

                (uint8_t)(received_model_bytes);

            mack_reply[5] =

                (uint8_t)(received_model_bytes >> 8);

            mack_reply[6] =

                (uint8_t)(received_model_bytes >> 16);

            mack_reply[7] =

                (uint8_t)(received_model_bytes >> 24);

            {

                uint8_t rc;

                do

                {

                    rc = CDC_Transmit_FS(

                        mack_reply,

                        sizeof(mack_reply)

                    );

                    if (rc == USBD_BUSY)

                    {

                        tk_dly_tsk(1);

                    }

                } while (rc == USBD_BUSY);

            }

            current_chunk_bytes = 0U;

            current_chunk_received = 0U;

            state = SERIAL_WAIT_CMD;

            continue;

        }

    }

}





/* ============================================================

 * Fatal indication

 * ============================================================ */

static void fatal_blink(uint32_t delay_ms)

{

    while (1)

    {

        HAL_GPIO_TogglePin(PE3_GPIO_Port, PE3_Pin);

        tk_dly_tsk(delay_ms);

    }

}





/* ============================================================

 * μT-Kernel user entry

 * ============================================================ */

INT usermain(void)

{

    tm_printf((UB *)

        "Hello from STM32H743 + microT-Kernel 3.0!\r\n");

    /*

     * CONTROL -> INFER:

     *     binary semaphore

     *

     * CONTROL/INFER -> LCD:

     *     event flag

     */

    T_CSEM csem =

    {

        .exinf   = NULL,

        .sematr  = TA_TFIFO,

        .isemcnt = 0,

        .maxsem  = 1,

    };

    ID infer_sem = tk_cre_sem(&csem);

    if (infer_sem <= 0)

    {

        fatal_blink(100);

    }





    T_CFLG cflg =

    {

        .exinf   = NULL,

        .flgatr  = TA_TFIFO | TA_WSGL,

        .iflgptn = 0U,

    };

    ID lcd_flg = tk_cre_flg(&cflg);

    if (lcd_flg <= 0)

    {

        fatal_blink(200);

    }





    CUTE_SetKernelSync(infer_sem, lcd_flg);





    /* --------------------------------------------------------

     * Task definitions

     * -------------------------------------------------------- */

    T_CTSK heartbeat_ctsk =

    {

        .exinf   = NULL,

        .tskatr  = TA_HLNG | TA_RNG3 | TA_USERBUF,

        .task    = heartbeat_task,

        .itskpri = HEARTBEAT_TASK_PRIORITY,

        .stksz   = HEARTBEAT_STACK_SIZE,

        .bufptr  = heartbeat_stack,

    };





    T_CTSK control_ctsk =

    {

        .exinf   = NULL,

        .tskatr  = TA_HLNG | TA_RNG3 | TA_USERBUF,

        .task    = control_task,

        .itskpri = CONTROL_TASK_PRIORITY,

        .stksz   = CONTROL_STACK_SIZE,

        .bufptr  = control_stack,

    };





    T_CTSK lcd_ctsk =

    {

        .exinf   = NULL,

        .tskatr  = TA_HLNG | TA_RNG3 | TA_USERBUF,

        .task    = lcd_task,

        .itskpri = LCD_TASK_PRIORITY,

        .stksz   = LCD_STACK_SIZE,

        .bufptr  = lcd_stack,

    };





    T_CTSK infer_ctsk =

    {

        .exinf   = NULL,

        .tskatr  = TA_HLNG | TA_RNG3 | TA_USERBUF,

        .task    = infer_task,

        .itskpri = INFER_TASK_PRIORITY,

        .stksz   = INFER_STACK_SIZE,

        .bufptr  = infer_stack,

    };





    T_CTSK serial_ctsk =

    {

        .exinf   = NULL,

        .tskatr  = TA_HLNG | TA_RNG3 | TA_USERBUF,

        .task    = serial_task,

        .itskpri = SERIAL_TASK_PRIORITY,

        .stksz   = SERIAL_STACK_SIZE,

        .bufptr  = serial_stack,

    };





    /* --------------------------------------------------------

     * Create tasks

     * -------------------------------------------------------- */

    ID heartbeat_tskid = tk_cre_tsk(&heartbeat_ctsk);

    if (heartbeat_tskid <= 0)

    {

        fatal_blink(250);

    }





    ID infer_tskid = tk_cre_tsk(&infer_ctsk);

    if (infer_tskid <= 0)

    {

        fatal_blink(300);

    }





    ID lcd_tskid = tk_cre_tsk(&lcd_ctsk);

    if (lcd_tskid <= 0)

    {

        fatal_blink(350);

    }





    ID control_tskid = tk_cre_tsk(&control_ctsk);

    if (control_tskid <= 0)

    {

        fatal_blink(400);

    }





    ID serial_tskid = tk_cre_tsk(&serial_ctsk);

    if (serial_tskid <= 0)

    {

        fatal_blink(450);

    }





    /*

     * Start waiters first:

     *

     *   INFER  -> blocks on infer_sem

     *   LCD    -> blocks on lcd_flg

     *   CONTROL-> starts DCMI and publishes events

     *   HEARTBEAT -> visible liveness

     *   SERIAL -> polls USB CDC RX ring

     */

    if (tk_sta_tsk(infer_tskid, 0) < E_OK)

    {

        fatal_blink(500);

    }





    if (tk_sta_tsk(lcd_tskid, 0) < E_OK)

    {

        fatal_blink(550);

    }





    if (tk_sta_tsk(control_tskid, 0) < E_OK)

    {

        fatal_blink(600);

    }





    if (tk_sta_tsk(heartbeat_tskid, 0) < E_OK)

    {

        fatal_blink(700);

    }





    if (tk_sta_tsk(serial_tskid, 0) < E_OK)

    {

        fatal_blink(750);

    }





    /*

     * usermain() runs in μT-Kernel's small high-priority initial task.

     * The real application is now handled by the five worker tasks.

     */

    tk_slp_tsk(TMO_FEVR);

    return 0;

}