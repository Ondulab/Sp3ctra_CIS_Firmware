/**
 ******************************************************************************
 * @file           : imu_gestures.c
 * @brief          : Device-side gesture detectors (HIT per face / resting FACE)
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 ******************************************************************************
 *
 * Runs in the 1 kHz HID task, on the full-rate IMU samples (the host only
 * sees the 200 Hz stream: shocks are much cleaner here). Gestures become
 * device facts, sent in slp_hid - the VST turns them into ordinary MIDI.
 *
 *  - HIT: a shock, with the STRUCK FACE and a 1..127 velocity. Percussion
 *    trigger, not a "shake detector": a Schmitt threshold on the high-passed
 *    acceleration (its own fast reference, see imuGestures_update), a short
 *    peak window, then a decaying rejection envelope that swallows the
 *    rebound while letting a genuine second strike through ~33 ms later -
 *    fingernail tapping at 8 Hz has three times the room it needs.
 *    Which face was hit comes from the direction of the first 3 ms of the
 *    impulse, before any rebound has a vote.
 *  - FACE: which face the device rests on, from the gravity tracker with
 *    angular hysteresis (enter 35 deg, leave 50 deg) + 300 ms stability +
 *    "not rotating" gate.
 *
 * ONLY THE LONG FACES COUNT, for both detectors. The board is a 194 x 25 mm
 * strip: its two large faces (CIS side / back) and its two long edges are
 * surfaces you can lay it on and strike; its two 25 mm ends are not. A shock
 * along the length reports SLP_FACE_MOVING - the HIT still fires, it just
 * belongs to no face.
 *
 * AXIS MAPPING (hardware, v3/v4 board, U10 = ICM-42688-P):
 *   The part sits on the top copper at (121.71, 85.05) with the footprint
 *   rotated 90 deg; the board outline runs 80.8..274.8 mm in X (the 194 mm
 *   LENGTH) and 79.2..104.2 mm in Y (the 25 mm WIDTH). That rotation maps the
 *   sensor's X onto the board's Y and the sensor's Y onto the board's X, so:
 *     acc[0] = ACROSS the bar   -> the two long EDGES  (LEFT / RIGHT)
 *     acc[1] = ALONG  the bar   -> the two short ENDS  (ignored)
 *     acc[2] = THROUGH the board -> the two large FACES (PAPER / BACK)
 *   acc[2] is confirmed by measurement (reading position = -1 g on Z, IMU
 *   calibration log) and acc[1] by gui_imu.c, which slides its cursor along
 *   the long OLED with acc[1]. The LEFT/RIGHT naming (not the axis) is the
 *   only guess left, and cis_handedness already flips it.
 *
 * SIGNS: at rest the accelerometer reads +1 g along the axis pointing UP, so
 * ges_faceFromAxis() maps a direction to the face whose outward normal is
 * OPPOSITE to it. That single table serves both detectors: a strike pushes
 * the device inwards, i.e. against the struck face's normal, so feeding it
 * the impulse direction returns the face that was hit.
 */

/* Includes ------------------------------------------------------------------*/
#include <math.h>

#include "globals.h"
#include "sp3ctra_link.h"
#include "icm42688.h"
#include "imu_gestures.h"

/* Private define ------------------------------------------------------------*/
/* All timings count 1 kHz update() calls, i.e. milliseconds. */

/* Which IMU axis is which direction of the bar - see the header comment. */
#define GES_AXIS_WIDTH         (0)      /* long edges: LEFT / RIGHT */
#define GES_AXIS_LENGTH        (1)      /* short ends: no face */
#define GES_AXIS_NORMAL        (2)      /* large faces: PAPER / BACK */

#define GES_LP_TAU_MS          (200.0f) /* gravity tracker, for the resting FACE */
#define GES_DC_TAU_MS          (20.0f)  /* shock high-pass reference, see below */
#define GES_WARMUP_MS          (400U)   /* let the trackers settle before firing */

/* Shock detector. The threshold is what "a small hit" costs: 0.22 g of
 * high-passed acceleration is a fingernail on the case, well clear of the
 * ~0.02 g noise floor at 258 Hz of bandwidth. */
#define GES_HIT_THR_G          (0.15f)  /* onset floor of the envelope */
#define GES_WINDOW_MS          (8U)     /* peak measurement after onset */
#define GES_DIR_MS             (2U)     /* impulse direction integrated here */
#define GES_REFRACTORY_MS      (25U)    /* hard dead time after an event */
#define GES_MASK_RATIO         (0.55f)  /* envelope right after a hit, x peak */
#define GES_MASK_DECAY2        (0.951229f) /* ... decaying back to the floor:
                                         * exp(-2 / 40 ms), squared domain */
#define GES_VEL_TOP_G          (6.0f)   /* a firm knock: velocity 127 */
#define GES_VEL_WIDTH_MS       (4.0f)   /* nominal contact width, see ges_emitHit */
#define GES_VEL_HEADROOM_G     (1.0f)   /* gravity already eats this of the scale */
#define GES_VEL_GAMMA          (0.6f)   /* velocity curve, < 1 = opens the low end */
#define GES_FS_FALLBACK_G      (4.0f)   /* if the sensor has not answered yet */

#define GES_FACE_ENTER         (0.82f)  /* cos 35 deg: enter a face */
#define GES_FACE_EXIT          (0.64f)  /* cos 50 deg: keep the current face */
#define GES_FACE_STABLE_MS     (300U)   /* candidate must hold this long */
#define GES_FACE_MOVING_MS     (200U)   /* ... and MOVING this long to clear */
#define GES_FACE_GYRO_DPS      (25.0f)  /* rotating faster: no face decision */
#define GES_FACE_G_MIN         (0.75f)  /* |gravity| window: reject dynamics */
#define GES_FACE_G_MAX         (1.25f)

/* Private variables ----------------------------------------------------------*/
static ImuGestureState ges_state = {0};
static volatile bool ges_event_pending = false;

static float    ges_lp[3] = {0};          /* gravity tracker (low-passed acc) */
static float    ges_dc[3] = {0};          /* shock high-pass reference */
static bool     ges_seeded = false;
static uint32_t ges_warmup_ms = 0;

/* Impulse detector: everything squared, so the 1 kHz path has no sqrtf. */
enum { IMP_IDLE = 0, IMP_MEASURE, IMP_REFRACTORY };
static uint8_t  ges_imp_phase = IMP_IDLE;
static uint32_t ges_imp_ms = 0;
static float    ges_imp_peak2 = 0.0f;
static float    ges_env2 = GES_HIT_THR_G * GES_HIT_THR_G;
static float    ges_dir[3] = {0};        /* leading edge, for the sign */
static float    ges_eng[3] = {0};        /* per-axis energy, for the axis */
static float    ges_sum = 0.0f;          /* sum |h| over the window = impulse */

static uint8_t  ges_face_cand = SLP_FACE_MOVING;
static uint32_t ges_face_cand_ms = 0;

/* Private functions ----------------------------------------------------------*/

/** The face whose outward normal points OPPOSITE to `value` on `axis` - see
 *  the SIGNS note in the header comment. The bar's two ends have no face.
 *  Left/right are swapped by handedness so they always name the edges as the
 *  user sees them. */
static uint8_t ges_faceFromAxis(int axis, float value)
{
    uint8_t f;
    if (axis == GES_AXIS_NORMAL)
    {
        /* Sign settled ON THE DEVICE 2026-09-04, not from the datasheet: lying
         * CIS-down, the bar reported BACK, so at rest in that position the
         * sensor reads +Z. +Z therefore points out through the BACK face, and
         * the face we name is the one whose normal OPPOSES the direction -
         * hence +Z -> PAPER. This one line settles the struck face too: both
         * detectors share the rule (see SIGNS in the header comment). */
        f = (value < 0.0f) ? SLP_FACE_BACK : SLP_FACE_PAPER;
    }
    else if (axis == GES_AXIS_WIDTH)
    {
        f = (value < 0.0f) ? SLP_FACE_LEFT : SLP_FACE_RIGHT;
    }
    else
    {
        return SLP_FACE_MOVING;   /* along the bar: a short end, not a face */
    }

    if (shared_config.cis_handedness == 0U)
    {
        if (f == SLP_FACE_LEFT)
        {
            f = SLP_FACE_RIGHT;
        }
        else if (f == SLP_FACE_RIGHT)
        {
            f = SLP_FACE_LEFT;
        }
    }
    return f;
}

/** The axis a face lies on, or -1 for SLP_FACE_MOVING. */
static int ges_axisOfFace(uint8_t f)
{
    if (f == SLP_FACE_PAPER || f == SLP_FACE_BACK)  return GES_AXIS_NORMAL;
    if (f == SLP_FACE_LEFT  || f == SLP_FACE_RIGHT) return GES_AXIS_WIDTH;
    return -1;
}

/** The face across from `f` (handedness-agnostic: it swaps names in pairs). */
static uint8_t ges_oppositeFace(uint8_t f)
{
    switch (f)
    {
        case SLP_FACE_PAPER: return SLP_FACE_BACK;
        case SLP_FACE_BACK:  return SLP_FACE_PAPER;
        case SLP_FACE_LEFT:  return SLP_FACE_RIGHT;
        case SLP_FACE_RIGHT: return SLP_FACE_LEFT;
        default:             return SLP_FACE_MOVING;
    }
}

/** Dominant axis of a vector (0..2). */
static int ges_dominantAxis(const float v[3])
{
    int axis = 0;
    float m = fabsf(v[0]);
    if (fabsf(v[1]) > m)
    {
        axis = 1;
        m = fabsf(v[1]);
    }
    if (fabsf(v[2]) > m)
    {
        axis = 2;
    }
    return axis;
}

/** End of the peak window: publish one HIT (face + velocity). */
static void ges_emitHit(void)
{
    /* VELOCITY comes from the IMPULSE (sum of |h| over the window), not from
     * the peak sample. At 1 kHz a knock is only two or three samples wide, so
     * the peak depends on WHERE the sampling instant falls on it - up to 40 %
     * of jitter on the same blow. Worse, once an axis clips the peak stops
     * growing altogether while the contact keeps widening: summing still sees
     * the difference between a tap and a blow when the peak no longer can. */
    const float metric = ges_sum;

    /* Where velocity 127 sits. Two ceilings, whichever comes first:
     *   - the MUSICAL one, a firm knock at GES_VEL_TOP_G, so a +/-16 g scale
     *     does not squeeze every ordinary tap into the bottom of the course;
     *   - the HARDWARE one. Every axis clips at the full scale and gravity
     *     already occupies 1 g of the axis the bar rests on, so a +/-2 g scale
     *     saturates at ~1 g of high-passed signal: mapping to 0.85 x 2 g there
     *     made every blow above a light tap land on the SAME value - the
     *     "hitting harder changes nothing" the bench showed. */
    float fs = icm42688_accelFsG();
    if (fs <= 0.0f)
    {
        fs = GES_FS_FALLBACK_G;   /* sensor not configured yet */
    }
    float top = fs - GES_VEL_HEADROOM_G;
    if (top > GES_VEL_TOP_G)
    {
        top = GES_VEL_TOP_G;
    }
    if (top < 4.0f * GES_HIT_THR_G)
    {
        top = 4.0f * GES_HIT_THR_G;   /* always leave a usable span */
    }

    /* ... expressed in the impulse's own units (g x ms). */
    const float topMetric = top * GES_VEL_WIDTH_MS;

    float v = (topMetric > GES_HIT_THR_G) ? (metric - GES_HIT_THR_G) / (topMetric - GES_HIT_THR_G) : 1.0f;
    if (v < 0.0f)
    {
        v = 0.0f;
    }
    else if (v > 1.0f)
    {
        v = 1.0f;
    }
    v = powf(v, GES_VEL_GAMMA);

    /* WHICH FACE. The axis comes from the per-axis ENERGY over the whole
     * window: it cannot cancel, so the case ringing that follows the impact
     * reinforces the answer instead of muddling it (the leading-edge sum
     * alone confused the source, the ringing reversing sign within 2 ms).
     * The SIGN is the hard half at 1 kHz, so physics decides it whenever it
     * can: a bar resting on a face CANNOT be struck on that face, so a blow
     * on the resting axis is necessarily on the face across from it. Only a
     * free-floating device, or a blow off the resting axis, falls back to the
     * leading edge. */
    const int axis = ges_dominantAxis(ges_eng);
    const uint8_t resting = ges_state.face;

    uint8_t face;
    if (axis == ges_axisOfFace(resting))
    {
        face = ges_oppositeFace(resting);
    }
    else
    {
        face = ges_faceFromAxis(axis, ges_dir[axis]);
    }

    ges_state.hit_velocity = (uint8_t)(1.0f + 126.0f * v);
    ges_state.hit_face     = face;
    ges_state.hit_seq++;
    ges_event_pending = true;
}

/** One high-passed sample through the percussion trigger.
 *  @return true only while the PEAK WINDOW is open, i.e. while the shock
 *          reference must hold still. Never longer - see IMP_REFRACTORY. */
static bool ges_updateImpulse(const float h[3], float m2)
{
    /* Rejection envelope: decays back to the absolute threshold floor. */
    ges_env2 *= GES_MASK_DECAY2;
    if (ges_env2 < GES_HIT_THR_G * GES_HIT_THR_G)
    {
        ges_env2 = GES_HIT_THR_G * GES_HIT_THR_G;
    }

    switch (ges_imp_phase)
    {
        case IMP_MEASURE:
            if (m2 > ges_imp_peak2)
            {
                ges_imp_peak2 = m2;
            }
            ges_sum    += sqrtf(m2);
            ges_eng[0] += h[0] * h[0];
            ges_eng[1] += h[1] * h[1];
            ges_eng[2] += h[2] * h[2];
            /* Direction = the onset sample plus GES_DIR_MS more, integrated:
             * the rebound that follows must not get a vote, and summing beats
             * a single sample whose direction is still noisy. */
            if (ges_imp_ms < GES_DIR_MS)
            {
                ges_dir[0] += h[0];
                ges_dir[1] += h[1];
                ges_dir[2] += h[2];
            }
            if (++ges_imp_ms >= GES_WINDOW_MS)
            {
                ges_emitHit();
                /* Arm the rebound mask from the peak we just measured: a knock
                 * of the same strength gets through as soon as the refractory
                 * ends, its echo does not. */
                ges_env2 = (GES_MASK_RATIO * GES_MASK_RATIO) * ges_imp_peak2;
                ges_imp_phase = IMP_REFRACTORY;
                ges_imp_ms = 0;
            }
            return true;

        case IMP_REFRACTORY:
            if (++ges_imp_ms >= GES_REFRACTORY_MS)
            {
                ges_imp_phase = IMP_IDLE;
            }
            /* false ON PURPOSE - the reference must catch up here. Freezing it
             * through the refractory too was a LATCH: a fast reorientation
             * leaves the reference behind, the residue looks like a permanent
             * onset, and the detector cycles MEASURE -> REFRACTORY -> MEASURE
             * for ever, firing max-velocity hits and never letting the
             * reference converge, precisely because it is behind. The freeze
             * is now bounded by the 8 ms window, so 25 ms of every 33 ms
             * always go to tracking: it cannot starve. */
            return false;

        case IMP_IDLE:
        default:
            if (m2 > ges_env2)
            {
                ges_imp_phase = IMP_MEASURE;
                ges_imp_ms = 0;
                ges_imp_peak2 = m2;
                ges_sum = sqrtf(m2);
                for (int i = 0; i < 3; i++)
                {
                    ges_dir[i] = h[i];
                    ges_eng[i] = h[i] * h[i];
                }
                return true;
            }
            return false;
    }
}

static void ges_updateFace(const float gyr[3])
{
    const float g2  = ges_lp[0] * ges_lp[0] + ges_lp[1] * ges_lp[1] + ges_lp[2] * ges_lp[2];
    const float gy2 = gyr[0] * gyr[0] + gyr[1] * gyr[1] + gyr[2] * gyr[2];

    uint8_t cand = SLP_FACE_MOVING;
    if (g2 > GES_FACE_G_MIN * GES_FACE_G_MIN && g2 < GES_FACE_G_MAX * GES_FACE_G_MAX &&
        gy2 < GES_FACE_GYRO_DPS * GES_FACE_GYRO_DPS)
    {
        /* Only the long faces are candidates: standing the bar on an end is
         * not a position, so the length axis never names one. The angular
         * threshold below then rejects everything near it anyway. */
        const int axis = (fabsf(ges_lp[GES_AXIS_WIDTH]) > fabsf(ges_lp[GES_AXIS_NORMAL]))
                       ? GES_AXIS_WIDTH : GES_AXIS_NORMAL;
        const float m = fabsf(ges_lp[axis]);

        if (fabsf(ges_lp[GES_AXIS_LENGTH]) <= m)
        {
            const uint8_t f = ges_faceFromAxis(axis, ges_lp[axis]);
            const float comp = m / sqrtf(g2);
            /* Hysteresis: staying on the current face is cheaper than entering. */
            if (comp >= ((f == ges_state.face) ? GES_FACE_EXIT : GES_FACE_ENTER))
            {
                cand = f;
            }
        }
    }

    if (cand == ges_state.face)
    {
        ges_face_cand = cand;
        ges_face_cand_ms = 0;
    }
    else if (cand == ges_face_cand)
    {
        const uint32_t need = (cand == SLP_FACE_MOVING) ? GES_FACE_MOVING_MS : GES_FACE_STABLE_MS;
        if (++ges_face_cand_ms >= need)
        {
            ges_state.face = cand;
            ges_face_cand_ms = 0;
            ges_event_pending = true;
        }
    }
    else
    {
        ges_face_cand = cand;
        ges_face_cand_ms = 0;
    }
}

/* Public functions -----------------------------------------------------------*/

void imuGestures_update(const float acc[3], const float gyr[3])
{
    /* Seeding the trackers with the first sample costs two lines and saves a
     * phantom 1 g "hit" at boot. */
    if (!ges_seeded)
    {
        for (int i = 0; i < 3; i++)
        {
            ges_lp[i] = acc[i];
            ges_dc[i] = acc[i];
        }
        ges_seeded = true;
    }

    /* TWO references, because they answer two different questions.
     *   ges_lp (tau 200 ms) IS the gravity vector: slow enough to ignore the
     *     way the device is being handled, which is what naming a face needs.
     *   ges_dc (tau 20 ms) is the shock high-pass reference. Subtracting the
     *     gravity tracker instead would leave, on a brisk 90 deg flip, some
     *     0.8 g of pure tracking lag in the "impulse" - phantom hits every
     *     time the bar is turned over. At 20 ms that lag falls to ~0.08 g,
     *     half the threshold, while a 2 ms impact still passes ~90 % intact. */
    const float h[3] = { acc[0] - ges_dc[0], acc[1] - ges_dc[1], acc[2] - ges_dc[2] };
    const float m2 = h[0] * h[0] + h[1] * h[1] + h[2] * h[2];

    bool busy = false;
    if (ges_warmup_ms < GES_WARMUP_MS)
    {
        ges_warmup_ms++;
    }
    else
    {
        busy = ges_updateImpulse(h, m2);
    }

    /* The shock reference FREEZES for the 8 ms peak window, and only there:
     * a 20 ms tau would otherwise eat a third of the peak while we measure it.
     * It must keep running everywhere else, refractory included, or it can
     * never catch up after a reorientation (see IMP_REFRACTORY). The gravity
     * tracker never freezes - a 10 ms transient moves it by a few percent,
     * which its own gates absorb. */
    if (!busy)
    {
        const float alphaDc = 1.0f / GES_DC_TAU_MS;
        ges_dc[0] += alphaDc * h[0];
        ges_dc[1] += alphaDc * h[1];
        ges_dc[2] += alphaDc * h[2];
    }

    const float alphaLp = 1.0f / GES_LP_TAU_MS;
    ges_lp[0] += alphaLp * (acc[0] - ges_lp[0]);
    ges_lp[1] += alphaLp * (acc[1] - ges_lp[1]);
    ges_lp[2] += alphaLp * (acc[2] - ges_lp[2]);

    ges_updateFace(gyr);
}

const ImuGestureState *imuGestures_state(void)
{
    return &ges_state;
}

bool imuGestures_takeEvent(void)
{
    const bool p = ges_event_pending;
    ges_event_pending = false;
    return p;
}
