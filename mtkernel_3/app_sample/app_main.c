/*
 * Cute-YOLO micro:bit V2
 * μT-Kernel 3.0
 *
 * RTOS STAGE 1.1 - TIMING + DEADLINE MONITOR
 * ------------
 * Preserve the byte-exact golden Cute-YOLO/Noodle implementation while
 * moving orchestration into meaningful μT-Kernel tasks.
 *
 * Application tasks:
 *   1) RX/COMM task   priority 8  - owns "sera" and command protocol
 *   2) LCD task       priority 9  - owns runtime ST7735 drawing
 *   3) INFERENCE task priority 10 - owns Noodle inference + decode/NMS
 *
 * Synchronization:
 *   - inference request event flag
 *   - LCD request event flag
 *   - completion event flag
 *
 * IMPORTANT:
 *   The inference function converts the shared 128x128 image in-place from
 *   uint8 [0,255] to int8 quantized storage. Therefore, for CUTE commands,
 *   the LCD image is drawn BEFORE inference starts. This preserves the
 *   existing golden firmware behavior without allocating another 16 KiB
 *   image buffer.
 *
 * Model upload (MBEG/MDAT/MEND) intentionally stays in RX/COMM in Stage 1.
 * RX waits for inference/LCD completion before reading the next command, so
 * model flash writes cannot race with active inference. Stage 2 can move
 * upload into a dedicated Model Update task guarded by a model mutex.
 */

#include <tk/tkernel.h>
#include <tm/tmonitor.h>

#include <stdint.h>

#include "cute_lcd.h"
#include "cute_model_flash.h"
#include "cute_detect_flash.h"

extern int cute_full_flash_run(
    uint8_t *gray_u8,
    uint32_t n,
    const int8_t **output_data,
    uint32_t *output_size
);

#define IMG_SIZE      (128u * 128u)
#define LCD_IMAGE_X   16u
#define BOX_COLOR     0x07E0u

/* μT-Kernel task priorities: smaller number = higher priority. */
#define PRI_RX        8
#define PRI_LCD       9
#define PRI_INFER     10

/*
 * Provisional end-to-end deadline.
 * This includes UART frame reception, LCD image draw, inference,
 * decode/NMS, LCD boxes, and the DTOK/FLOK response.
 *
 * After the first measured runs we can set this from actual evidence.
 */
#define CUTE_DEADLINE_MS 5000u

/* Task stacks. Inference keeps the old task's 3 KiB working allowance. */
#define STACK_RX      2048
#define STACK_INFER   3072
#define STACK_LCD     1536

/* Inference request flag bits. */
#define EVT_INFER_RUN       (1u << 0)

/* LCD request flag bits. */
#define EVT_LCD_IMAGE       (1u << 0)
#define EVT_LCD_BOXES       (1u << 1)

/* Completion flag bits. Only RX waits on this flag. */
#define EVT_INFER_DONE      (1u << 0)
#define EVT_LCD_DONE        (1u << 1)
#define EVT_LCD_READY       (1u << 2)

/*
 * One image buffer, deliberately retained from the golden firmware.
 * Ownership sequence for CUTE:
 *
 *   RX fills -> LCD reads -> INFERENCE mutates/reads -> RX may refill
 *
 * Event flags enforce that order.
 */
static UB image[IMG_SIZE];

/* Kernel object IDs, created in usermain(). */
static ID g_flg_infer_req = 0;
static ID g_flg_lcd_req = 0;
static ID g_flg_done = 0;

/* Shared inference result. Access is ordered by event flags. */
static volatile INT g_regression_mode = 0;
static volatile INT g_infer_rc = -999;
static const int8_t *g_head = 0;
static uint32_t g_head_size = 0;
static CuteDetectionSet g_detections;

static INT g_has_model_at_boot = 0;

/*
 * Stage 1.1 real-time instrumentation.
 *
 * tk_get_otm() provides μT-Kernel operating time.  We only need the
 * low 32 bits because individual transactions are much shorter than
 * the 32-bit millisecond wrap interval; unsigned subtraction also
 * handles a single wrap correctly.
 */
typedef struct {
    uint32_t frame_id;
    uint32_t uart_rx_ms;
    uint32_t lcd_image_ms;
    uint32_t inference_ms;
    uint32_t decode_nms_ms;
    uint32_t lcd_boxes_ms;
    uint32_t response_ms;
    uint32_t total_ms;
    uint32_t deadline_ms;
    uint32_t deadline_met;
} CuteRuntimeTiming;

static CuteRuntimeTiming g_last_timing;
static uint32_t g_frame_counter = 0;

/* Written by INFERENCE task, consumed after EVT_INFER_DONE by RX task. */
static volatile uint32_t g_last_inference_ms = 0;
static volatile uint32_t g_last_decode_nms_ms = 0;

static uint32_t cute_now_ms(void)
{
    SYSTIM t;

    if (tk_get_otm(&t) < E_OK) {
        return 0;
    }

    return (uint32_t)t.lo;
}

static uint32_t cute_elapsed_ms(
    uint32_t start_ms,
    uint32_t end_ms)
{
    return end_ms - start_ms;
}

/* ------------------------------------------------------------
 * Small synchronization helpers.
 * ------------------------------------------------------------ */
static ER wait_event(ID flgid, UINT bit)
{
    UINT got = 0;

    return tk_wai_flg(
        flgid,
        bit,
        (TWF_ANDW | TWF_BITCLR),
        &got,
        TMO_FEVR
    );
}

static ER wait_event_or(ID flgid, UINT bits, UINT *got)
{
    return tk_wai_flg(
        flgid,
        bits,
        (TWF_ORW | TWF_BITCLR),
        got,
        TMO_FEVR
    );
}

/* ------------------------------------------------------------
 * Serial helpers. RX/COMM task is the ONLY owner of "sera".
 * ------------------------------------------------------------ */
static ER serial_read_exact(ID dd, UB *buf, SZ size)
{
    SZ total = 0;

    while (total < size) {
        SZ got = 0;

        ER er = tk_srea_dev(
            dd,
            0,
            buf + total,
            size - total,
            &got
        );

        if (er < E_OK) return er;
        if (got <= 0) continue;

        total += got;
    }

    return E_OK;
}

static ER serial_write_exact(
    ID dd,
    const UB *buf,
    SZ size)
{
    SZ total = 0;

    while (total < size) {
        SZ sent = 0;

        ER er = tk_swri_dev(
            dd,
            0,
            buf + total,
            size - total,
            &sent
        );

        if (er < E_OK) return er;
        if (sent <= 0) continue;

        total += sent;
    }

    return E_OK;
}

static uint32_t load_u32_le(const UB b[4])
{
    return
        ((uint32_t)b[0]) |
        ((uint32_t)b[1] << 8) |
        ((uint32_t)b[2] << 16) |
        ((uint32_t)b[3] << 24);
}

static uint16_t load_u16_le(const UB b[2])
{
    return
        (uint16_t)(
            ((uint16_t)b[0]) |
            ((uint16_t)b[1] << 8)
        );
}

static void store_u32_le(UB b[4], uint32_t x)
{
    b[0] = (UB)(x & 0xFFu);
    b[1] = (UB)((x >> 8) & 0xFFu);
    b[2] = (UB)((x >> 16) & 0xFFu);
    b[3] = (UB)((x >> 24) & 0xFFu);
}

/* ------------------------------------------------------------
 * Detector/LCD formatting helpers.
 * ------------------------------------------------------------ */
static uint8_t norm_to_px(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    INT p = (INT)(v * 127.0f + 0.5f);

    if (p < 0) p = 0;
    if (p > 127) p = 127;

    return (uint8_t)p;
}

static uint8_t conf_to_u8(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    INT p = (INT)(v * 255.0f + 0.5f);

    if (p < 0) p = 0;
    if (p > 255) p = 255;

    return (uint8_t)p;
}

/*
 * LCD task is the only task that calls this function.
 */
static void draw_detections(
    const CuteDetectionSet *set)
{
    for (uint8_t i = 0;
         i < set->count;
         ++i) {

        uint16_t x1 =
            LCD_IMAGE_X +
            norm_to_px(set->items[i].x1);

        uint16_t y1 =
            norm_to_px(set->items[i].y1);

        uint16_t x2 =
            LCD_IMAGE_X +
            norm_to_px(set->items[i].x2);

        uint16_t y2 =
            norm_to_px(set->items[i].y2);

        if (x2 < x1) {
            uint16_t t = x1;
            x1 = x2;
            x2 = t;
        }

        if (y2 < y1) {
            uint16_t t = y1;
            y1 = y2;
            y2 = t;
        }

        const uint16_t w =
            x2 - x1 + 1u;

        const uint16_t h =
            y2 - y1 + 1u;

        cute_lcd_draw_rect(
            x1, y1, w, h, BOX_COLOR);

        if (w > 4u && h > 4u) {
            cute_lcd_draw_rect(
                x1 + 1u,
                y1 + 1u,
                w - 2u,
                h - 2u,
                BOX_COLOR);
        }
    }
}

static void send_detector_error(ID dd, INT rc)
{
    UB reply[8] = {
        'D','T','E','R',
        0,0,0,0
    };

    store_u32_le(
        &reply[4],
        (uint32_t)rc);

    (void)serial_write_exact(
        dd,
        reply,
        sizeof(reply));
}

static ER send_detections(
    ID dd,
    const CuteDetectionSet *set)
{
    UB hdr[8] = {
        'D','T','O','K',
        set->count,
        0,0,0
    };

    ER er =
        serial_write_exact(
            dd,
            hdr,
            sizeof(hdr));

    if (er < E_OK) return er;

    for (uint8_t i = 0;
         i < set->count;
         ++i) {

        UB rec[5] = {
            conf_to_u8(
                set->items[i].confidence),
            norm_to_px(set->items[i].x1),
            norm_to_px(set->items[i].y1),
            norm_to_px(set->items[i].x2),
            norm_to_px(set->items[i].y2)
        };

        er =
            serial_write_exact(
                dd,
                rec,
                sizeof(rec));

        if (er < E_OK) return er;
    }

    return E_OK;
}

/* ------------------------------------------------------------
 * Model upload protocol.
 * Stage 1 intentionally keeps this in RX/COMM.
 * ------------------------------------------------------------ */
static void handle_model_begin(ID dd)
{
    UB size_bytes[4];

    if (serial_read_exact(
            dd,
            size_bytes,
            4) < E_OK) {
        return;
    }

    const uint32_t total =
        load_u32_le(size_bytes);

    char slot = '\0';

    const INT rc =
        cute_model_upload_begin(
            total,
            &slot);

    UB reply[8] = {
        'M','R','E','D',
        0,0,0,0
    };

    if (rc != 0) {
        reply[0] = 'M';
        reply[1] = 'E';
        reply[2] = 'R';
        reply[3] = 'R';

        store_u32_le(
            &reply[4],
            (uint32_t)rc);
    } else {
        reply[4] = (UB)slot;
    }

    (void)serial_write_exact(
        dd,
        reply,
        sizeof(reply));
}

static void handle_model_data(ID dd)
{
    UB nbytes[2];

    if (serial_read_exact(
            dd,
            nbytes,
            2) < E_OK) {
        return;
    }

    const uint16_t n =
        load_u16_le(nbytes);

    if (n == 0 || n > 256u) {
        return;
    }

    UB chunk[256];

    if (serial_read_exact(
            dd,
            chunk,
            n) < E_OK) {
        return;
    }

    const INT rc =
        cute_model_upload_write(
            chunk,
            n);

    UB reply[8] = {
        'M','A','C','K',
        0,0,0,0
    };

    if (rc != 0) {
        reply[0] = 'M';
        reply[1] = 'E';
        reply[2] = 'R';
        reply[3] = 'R';

        store_u32_le(
            &reply[4],
            (uint32_t)rc);
    } else {
        store_u32_le(
            &reply[4],
            cute_model_upload_received());
    }

    (void)serial_write_exact(
        dd,
        reply,
        sizeof(reply));
}

static void handle_model_end(ID dd)
{
    char slot = '\0';

    const INT rc =
        cute_model_upload_end(
            &slot);

    UB reply[8] = {
        'M','A','C','T',
        0,0,0,0
    };

    if (rc != 0) {
        reply[0] = 'M';
        reply[1] = 'E';
        reply[2] = 'R';
        reply[3] = 'R';

        store_u32_le(
            &reply[4],
            (uint32_t)rc);
    } else {
        reply[4] = (UB)slot;
    }

    (void)serial_write_exact(
        dd,
        reply,
        sizeof(reply));
}

static void handle_model_info(ID dd)
{
    UB reply[32];

    for (uint32_t i = 0;
         i < sizeof(reply);
         ++i) {
        reply[i] = 0;
    }

    reply[0] = 'M';
    reply[1] = 'I';
    reply[2] = 'N';
    reply[3] = 'F';

    reply[4] =
        (UB)cute_model_active_slot();

    if (cute_model_ready()) {
        const char *label =
            cute_model_label();

        for (uint32_t i = 0;
             i < 23u && label[i];
             ++i) {
            reply[8 + i] = (UB)label[i];
        }
    }

    (void)serial_write_exact(
        dd,
        reply,
        sizeof(reply));
}


/*
 * Conservative timing response: exactly 32 bytes, matching the size of the
 * already-proven MINF reply.
 *
 * Commands:
 *   STAT  (preferred)
 *   RTIM  (alias)
 *
 * Response layout:
 *   0..3    "STOK"
 *   4..7    frame_id
 *   8..11   UART receive time [ms]
 *   12..15  LCD image time [ms]
 *   16..19  Noodle inference time [ms]
 *   20..23  decode + NMS time [ms]
 *   24..27  LCD box-overlay time [ms]
 *   28..31  end-to-end total time [ms]
 *
 * The current provisional deadline remains CUTE_DEADLINE_MS in firmware.
 * The host compares total_ms against the same value.
 */
static void handle_runtime_timing(ID dd)
{
    UB reply[32];

    for (uint32_t i = 0;
         i < sizeof(reply);
         ++i) {
        reply[i] = 0;
    }

    reply[0] = 'S';
    reply[1] = 'T';
    reply[2] = 'O';
    reply[3] = 'K';

    store_u32_le(&reply[4],  g_last_timing.frame_id);
    store_u32_le(&reply[8],  g_last_timing.uart_rx_ms);
    store_u32_le(&reply[12], g_last_timing.lcd_image_ms);
    store_u32_le(&reply[16], g_last_timing.inference_ms);
    store_u32_le(&reply[20], g_last_timing.decode_nms_ms);
    store_u32_le(&reply[24], g_last_timing.lcd_boxes_ms);
    store_u32_le(&reply[28], g_last_timing.total_ms);

    (void)serial_write_exact(
        dd,
        reply,
        sizeof(reply));
}

/* ------------------------------------------------------------
 * INFERENCE TASK
 *
 * Exclusive owner of:
 *   - cute_full_flash_run()
 *   - Noodle tensors/arena used by that routine
 *   - cute_detect_flash_decode()
 *
 * No Noodle mutex is required because no other task executes inference.
 * ------------------------------------------------------------ */
static void inference_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    for (;;) {
        if (wait_event(
                g_flg_infer_req,
                EVT_INFER_RUN) < E_OK) {
            continue;
        }

        g_infer_rc = -999;
        g_head = 0;
        g_head_size = 0;
        g_detections.count = 0;
        g_last_inference_ms = 0;
        g_last_decode_nms_ms = 0;

        if (!cute_model_ready()) {
            g_infer_rc = -900;
            (void)tk_set_flg(
                g_flg_done,
                EVT_INFER_DONE);
            continue;
        }

        const int8_t *head = 0;
        uint32_t head_size = 0;

        const uint32_t t_infer_start =
            cute_now_ms();

        INT rc =
            cute_full_flash_run(
                image,
                IMG_SIZE,
                &head,
                &head_size);

        const uint32_t t_infer_end =
            cute_now_ms();

        g_last_inference_ms =
            cute_elapsed_ms(
                t_infer_start,
                t_infer_end);

        if (rc != 0 ||
            !head ||
            head_size != 1280u) {

            g_infer_rc =
                rc ? rc : -901;

            (void)tk_set_flg(
                g_flg_done,
                EVT_INFER_DONE);
            continue;
        }

        g_head = head;
        g_head_size = head_size;

        if (!g_regression_mode) {
            const uint32_t t_decode_start =
                cute_now_ms();

            rc =
                cute_detect_flash_decode(
                    head,
                    head_size,
                    &g_detections);

            const uint32_t t_decode_end =
                cute_now_ms();

            g_last_decode_nms_ms =
                cute_elapsed_ms(
                    t_decode_start,
                    t_decode_end);

            if (rc != 0) {
                g_infer_rc =
                    -950 + rc;

                (void)tk_set_flg(
                    g_flg_done,
                    EVT_INFER_DONE);
                continue;
            }
        }

        g_infer_rc = 0;

        (void)tk_set_flg(
            g_flg_done,
            EVT_INFER_DONE);
    }
}

/* ------------------------------------------------------------
 * LCD TASK
 *
 * Exclusive owner of all cute_lcd_* calls.
 * It initializes the display and then sleeps on an event flag.
 * ------------------------------------------------------------ */
static void lcd_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    /*
     * LCD hardware initialization is deliberately performed in usermain()
     * using the same boot-time context/order as the golden firmware.
     * This task owns all runtime drawing operations.
     */
    (void)tk_set_flg(
        g_flg_done,
        EVT_LCD_READY);

    for (;;) {
        UINT events = 0;

        if (wait_event_or(
                g_flg_lcd_req,
                EVT_LCD_IMAGE | EVT_LCD_BOXES,
                &events) < E_OK) {
            continue;
        }

        /*
         * Requests are normally issued one at a time by RX.
         * If both are ever present, preserve the sensible order:
         * image first, rectangles second.
         */
        if (events & EVT_LCD_IMAGE) {
            cute_lcd_draw_gray128(image);
        }

        if (events & EVT_LCD_BOXES) {
            draw_detections(
                &g_detections);
        }

        (void)tk_set_flg(
            g_flg_done,
            EVT_LCD_DONE);
    }
}

/* ------------------------------------------------------------
 * RX / COMM TASK
 *
 * Highest-priority application task.
 * Exclusive owner of serial device "sera" and protocol responses.
 *
 * It blocks on event flags while inference or LCD work executes, allowing
 * μT-Kernel to dispatch those tasks deterministically.
 * ------------------------------------------------------------ */
static void handle_image_rtos(
    ID dd,
    INT regression_mode)
{
    CuteRuntimeTiming timing = {0};

    timing.frame_id =
        ++g_frame_counter;

    timing.deadline_ms =
        CUTE_DEADLINE_MS;

    const uint32_t t_total_start =
        cute_now_ms();

    const uint32_t t_rx_start =
        t_total_start;

    if (serial_read_exact(
            dd,
            image,
            IMG_SIZE) < E_OK) {
        return;
    }

    const uint32_t t_rx_end =
        cute_now_ms();

    timing.uart_rx_ms =
        cute_elapsed_ms(
            t_rx_start,
            t_rx_end);

    /*
     * Golden behavior: CUTE shows the original uint8 image before
     * cute_full_flash_run() quantizes it in-place.
     */
    if (!regression_mode) {
        const uint32_t t_lcd_start =
            cute_now_ms();

        if (tk_set_flg(
                g_flg_lcd_req,
                EVT_LCD_IMAGE) < E_OK ||
            wait_event(
                g_flg_done,
                EVT_LCD_DONE) < E_OK) {

            send_detector_error(dd, -980);
            return;
        }

        const uint32_t t_lcd_end =
            cute_now_ms();

        timing.lcd_image_ms =
            cute_elapsed_ms(
                t_lcd_start,
                t_lcd_end);
    }

    g_regression_mode =
        regression_mode ? 1 : 0;

    if (tk_set_flg(
            g_flg_infer_req,
            EVT_INFER_RUN) < E_OK ||
        wait_event(
            g_flg_done,
            EVT_INFER_DONE) < E_OK) {

        send_detector_error(dd, -981);
        return;
    }

    timing.inference_ms =
        g_last_inference_ms;

    timing.decode_nms_ms =
        g_last_decode_nms_ms;

    const INT rc = g_infer_rc;

    if (rc != 0) {
        send_detector_error(dd, rc);
        return;
    }

    if (regression_mode) {
        UB hdr[8] = {
            'F','L','O','K',
            0,0,0,0
        };

        store_u32_le(
            &hdr[4],
            g_head_size);

        const uint32_t t_response_start =
            cute_now_ms();

        if (serial_write_exact(
                dd,
                hdr,
                sizeof(hdr)) < E_OK) {
            return;
        }

        if (serial_write_exact(
                dd,
                (const UB *)g_head,
                (SZ)g_head_size) < E_OK) {
            return;
        }

        const uint32_t t_response_end =
            cute_now_ms();

        timing.response_ms =
            cute_elapsed_ms(
                t_response_start,
                t_response_end);

        timing.total_ms =
            cute_elapsed_ms(
                t_total_start,
                t_response_end);

        timing.deadline_met =
            (timing.total_ms <= timing.deadline_ms)
            ? 1u
            : 0u;

        g_last_timing = timing;

        return;
    }

    /*
     * Inference/decode has completed; ask the LCD owner task to overlay
     * the returned detections onto the already-displayed image.
     */
    const uint32_t t_boxes_start =
        cute_now_ms();

    if (tk_set_flg(
            g_flg_lcd_req,
            EVT_LCD_BOXES) < E_OK ||
        wait_event(
            g_flg_done,
            EVT_LCD_DONE) < E_OK) {

        send_detector_error(dd, -982);
        return;
    }

    const uint32_t t_boxes_end =
        cute_now_ms();

    timing.lcd_boxes_ms =
        cute_elapsed_ms(
            t_boxes_start,
            t_boxes_end);

    const uint32_t t_response_start =
        cute_now_ms();

    if (send_detections(
            dd,
            &g_detections) < E_OK) {
        return;
    }

    const uint32_t t_response_end =
        cute_now_ms();

    timing.response_ms =
        cute_elapsed_ms(
            t_response_start,
            t_response_end);

    timing.total_ms =
        cute_elapsed_ms(
            t_total_start,
            t_response_end);

    timing.deadline_met =
        (timing.total_ms <= timing.deadline_ms)
        ? 1u
        : 0u;

    g_last_timing = timing;
}

static void rx_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;

    /*
     * Do all T-Monitor output before opening "sera".
     * T-Monitor and sera share the UART on this port.
     */
    tm_putstring(
        (UB*)"Cute-YOLO μT-Kernel RTOS Stage 1.1 timing\n");

    tm_putstring(
        (UB*)"Tasks: RX/COMM + INFERENCE + LCD\n");

    if (g_has_model_at_boot) {
        tm_putstring(
            (UB*)"Valid .cute model found in flash\n");
    } else {
        tm_putstring(
            (UB*)"No valid .cute model; upload one first\n");
    }

    tm_putstring(
        (UB*)"Waiting for LCD task...\n");

    if (wait_event(
            g_flg_done,
            EVT_LCD_READY) < E_OK) {

        tm_putstring(
            (UB*)"ERROR: LCD ready event failed\n");

        tk_exd_tsk();
    }

    tm_putstring(
        (UB*)"LCD ready. Opening sera...\n");

    ID dd =
        tk_opn_dev(
            (UB*)"sera",
            TD_READ | TD_WRITE);

    if (dd < E_OK) {
        tm_putstring(
            (UB*)"ERROR: cannot open sera\n");

        tk_exd_tsk();
    }

    /*
     * No tm_putstring() calls beyond this point:
     * RX/COMM now exclusively owns sera.
     *
     * Four-byte command framing:
     *
     * CUTE + image
     * LOGT + image
     * MBEG + uint32 model size
     * MDAT + uint16 chunk size + bytes
     * MEND
     * MINF
     * STAT  -> last real-time timing record (preferred)
     * RTIM  -> alias for STAT
     */
    UW rolling = 0;

    for (;;) {
        UB c;

        if (serial_read_exact(
                dd,
                &c,
                1) < E_OK) {
            continue;
        }

        rolling =
            (rolling << 8) |
            (UW)c;

        if (rolling == 0x43555445u) { /* CUTE */
            handle_image_rtos(dd, 0);
            rolling = 0;
        }
        else if (rolling == 0x4C4F4754u) { /* LOGT */
            handle_image_rtos(dd, 1);
            rolling = 0;
        }
        else if (rolling == 0x4D424547u) { /* MBEG */
            handle_model_begin(dd);
            rolling = 0;
        }
        else if (rolling == 0x4D444154u) { /* MDAT */
            handle_model_data(dd);
            rolling = 0;
        }
        else if (rolling == 0x4D454E44u) { /* MEND */
            handle_model_end(dd);
            rolling = 0;
        }
        else if (rolling == 0x4D494E46u) { /* MINF */
            handle_model_info(dd);
            rolling = 0;
        }
        else if (rolling == 0x53544154u) { /* STAT */
            handle_runtime_timing(dd);
            rolling = 0;
        }
        else if (rolling == 0x5254494Du) { /* RTIM alias */
            handle_runtime_timing(dd);
            rolling = 0;
        }
    }
}

/* ------------------------------------------------------------
 * Kernel object creation information.
 * ------------------------------------------------------------ */
static const T_CFLG cflg_infer_req = {
    .flgatr = TA_TFIFO | TA_WSGL,
    .iflgptn = 0
};

static const T_CFLG cflg_lcd_req = {
    .flgatr = TA_TFIFO | TA_WSGL,
    .iflgptn = 0
};

static const T_CFLG cflg_done = {
    .flgatr = TA_TFIFO | TA_WSGL,
    .iflgptn = 0
};

static const T_CTSK ctsk_rx = {
    .tskatr = TA_HLNG | TA_RNG3,
    .task = &rx_task,
    .itskpri = PRI_RX,
    .stksz = STACK_RX
};

static const T_CTSK ctsk_infer = {
    .tskatr = TA_HLNG | TA_RNG3,
    .task = &inference_task,
    .itskpri = PRI_INFER,
    .stksz = STACK_INFER
};

static const T_CTSK ctsk_lcd = {
    .tskatr = TA_HLNG | TA_RNG3,
    .task = &lcd_task,
    .itskpri = PRI_LCD,
    .stksz = STACK_LCD
};

EXPORT INT usermain(void)
{
    tm_putstring(
        (UB*)"Cute-YOLO micro:bit flash-backed RTOS firmware\n");

    /*
     * Model discovery is performed once before application tasks start.
     * Stage 1 keeps model updates serialized by the RX task.
     */
    g_has_model_at_boot =
        cute_model_flash_init();

    g_last_timing.deadline_ms =
        CUTE_DEADLINE_MS;

    /*
     * Preserve the exact hardware bring-up location used by the golden
     * single-task firmware.  cute_lcd.c directly bit-bangs nRF52833 GPIO
     * rather than using a μT-Kernel device driver, so boot-time setup stays
     * here while all frame/box rendering remains in the LCD task.
     */
    cute_lcd_init();
    cute_lcd_clear(0x0000u);

    tm_putstring(
        (UB*)"LCD hardware initialized in initial task\n");

    g_flg_infer_req =
        tk_cre_flg(
            &cflg_infer_req);

    g_flg_lcd_req =
        tk_cre_flg(
            &cflg_lcd_req);

    g_flg_done =
        tk_cre_flg(
            &cflg_done);

    if (g_flg_infer_req < E_OK ||
        g_flg_lcd_req < E_OK ||
        g_flg_done < E_OK) {

        tm_putstring(
            (UB*)"ERROR: cannot create event flags\n");

        return 0;
    }

    ID infer_id =
        tk_cre_tsk(
            &ctsk_infer);

    ID lcd_id =
        tk_cre_tsk(
            &ctsk_lcd);

    ID rx_id =
        tk_cre_tsk(
            &ctsk_rx);

    if (infer_id < E_OK ||
        lcd_id < E_OK ||
        rx_id < E_OK) {

        tm_putstring(
            (UB*)"ERROR: cannot create application tasks\n");

        return 0;
    }

    /*
     * Start workers before the high-priority communication task.
     * They immediately block/wait as appropriate.
     */
    if (tk_sta_tsk(infer_id, 0) < E_OK ||
        tk_sta_tsk(lcd_id, 0) < E_OK ||
        tk_sta_tsk(rx_id, 0) < E_OK) {

        tm_putstring(
            (UB*)"ERROR: cannot start application tasks\n");

        return 0;
    }

    /*
     * μT-Kernel initial task must remain alive.
     */
    tk_slp_tsk(TMO_FEVR);

    return 0;
}
