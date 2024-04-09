/*************************************************************************************************
 *          Macronix xSPI-NOR memory header file
 * @file    MXIC_SNOR_128M.h
 * @brief   This file contains all information of Macronix 128M bit xSPI memory.
 * @Author  Peter Chang / Macronix; peterchang01@mxic.com.tw
 *************************************************************************************************
 * Read Commands :
 *  Rate |    IO     | 3/4 Byte address | 4 Byte address | Enhance Read
 * ------+-----------+------------------+----------------+------------------
 *  STR  | 1-1-1+0DC | 0x03             |  0x13          |
 *       | 1-1-1     | 0x0B             |  0x0C          |
 *       | 1-1-2     | 0x3B             |  0x3C          |
 *       | 1-1-4     | 0x6B             |  0x6C          |
 *       | 1-1-8     | 0x03             |  0x7C          |
 *       | 1-2-2     | 0xBB             |  0xBC          |
 *       | 1-4-4     | 0xEA, 0xEB       |  0xEC          | 0xEA, 0xEB, 0xEC
 *       | 1-4-4+4DC | 0xE7             |        -       |
 *       | 4-4-4     | 0x0B, 0xEA, 0xEB |  0xEC          | 0xEA, 0xEB, 0xEC
 *       | 4-4-4+4DC | 0x0B, 0xE7       |        -       |
 *       | 8-8-8     |         -        |  0x7C, 0xEC    |
 * ------+-----------+------------------+----------------+------------------
 *  DTR  | 1-1-1+0DC |         -        |        -       |
 *       | 1-1-1     | 0x0D             |  0x0E          |
 *       | 1-1-2     |         -        |        -       |
 *       | 1-1-4     |         -        |        -       |
 *       | 1-1-8     |         -        |        -       |
 *       | 1-2-2     | 0xBD             |  0xBE          |
 *       | 1-4-4     | 0xED             |  0xEE          | 0xED, 0xEE
 *       | 1-4-4+4DC |         -        |        -       |
 *       | 4-4-4     | 0xED             |  0xEE          | 0xED, 0xEE
 *       | 4-4-4+4DC |         -        |        -       |
 *       | 8-8-8     |         -        |  0xEE          |
 *
 * Page Program Commands :
 *  Rate |    IO     | 3/4 Byte address | 4 Byte address
 * ------+-----------+------------------+----------------
 *  STR  | 1-1-1+0DC |         -        |        -
 *       | 1-1-1     | 0x02             | 0x12
 *       | 1-1-2     |         -        |        -
 *       | 1-1-4     | 0x32             |        -
 *       | 1-1-8     | 0x02             |        -
 *       | 1-2-2     |         -        |        -
 *       | 1-4-4     | 0x38             | 0x3E
 *       | 1-4-4+4DC |         -        |        -
 *       | 4-4-4     | 0x02             | 0x12
 *       | 4-4-4+4DC |         -        |        -
 *       | 8-8-8     |         -        | 0x12
 * ------+-----------+------------------+----------------
 *  DTR  | 1-1-1+0DC |         -        |        -
 *       | 1-1-1     |         -        |        -
 *       | 1-1-2     |         -        |        -
 *       | 1-1-4     |         -        |        -
 *       | 1-1-8     |         -        |        -
 *       | 1-2-2     |         -        |        -
 *       | 1-4-4     |         -        |        -
 *       | 1-4-4+4DC |         -        |        -
 *       | 4-4-4     |         -        |        -
 *       | 4-4-4+4DC |         -        |        -
 *       | 8-8-8     |         -        | 0x12
 *
 * Erase Commands :
 *  Rate |    IO     | 3/4 Byte address | 4 Byte address
 * ------+-----------+------------------+------------------
 *  STR  | 1-1-1+0DC | 1-0-0  Chip Erase 0x60, 0xC7
 *       | 1-1-1     | 1-1-0        Block Erase
 *       | 1-1-2     | 0x20, 0x52, 0xD8 | 0x21, 0x5C, 0xDC
 *       | 1-1-4     |                  |
 *       | 1-1-8     |                  |
 *       | 1-2-2     |                  |
 *       | 1-4-4     |                  |
 *       | 1-4-4+4DC |                  |
 *       +-----------+------------------+------------------
 *       | 4-4-4     | 4-0-0  Chip Erase 0x60, 0xC7
 *       | 4-4-4+4DC | 4-4-0        Block Erase
 *       |           | 0x20, 0x52, 0xD8 | 0x21, 0x5C, 0xDC
 *       +-----------+------------------+------------------
 *       | 8-8-8     | 8-0-0  Chip Erase 0x60, 0xC7
 *       |           | 8-8-0        Block Erase
 *       |           |                  | 0x21, 0xDC
 * ------+-----------+------------------+------------------
 *  DTR  | 1-1-1+0DC |                  -
 *       | 1-1-1     |                  -
 *       | 1-1-2     |                  -
 *       | 1-1-4     |                  -
 *       | 1-1-8     |                  -
 *       | 1-2-2     |                  -
 *       | 1-4-4     |                  -
 *       | 1-4-4+4DC |                  -
 *       | 4-4-4     |                  -
 *       | 4-4-4+4DC |                  -
 *       +-----------+------------------+------------------
 *       | 8-8-8     | 8-0-0  Chip Erase 0x60, 0xC7
 *       |           | 8-8-0        Block Erase
 *       |           |         -        | 0x21, 0xDC
 *************************************************************************************************
 * QE bit action :
 *   QE(Quad Enable, Non-volatile) bit of Status Register
 *   QE = 0; WP# & RESET# pin active
 *           Accept 1-1-1, 1-1-2, 1-2-2 commands
 *   QE = 1; WP# become SIO2 pin, RESET# become SIO3 pin
 *           Accept 1-1-1, 1-1-2, 1-2-2, 1-1-4, 1-4-4 commands
 *   Enter QPI mode by issue EQIO(0x35) command from 1-1-1 mode
 *           Accept 4-4-4 commands
 *   Exit QPI mode by issue RSTQIO(0xF5) command from 4-4-4 mode
 *           Accept commands, dependent QE bit status
 *
 * DC Example : Dummy Clock of MX25L51245G supported Max. SCLK frequency
 *  Index|             Instruction format
 *   DC  | 1-1-1, 1-1-2     | 1-2-2            | 1-4-4, 1-4-4(DTR)
 *  [1:0]| 1-1-4, 1-1-1(DTR)| 1-2-2(DTR)       | 4-4-4, 4-4-4(DTR)
 * ------+------------------+------------------+--------------------
 *   00  | 8 / 133M, DTR 66M| 4 /  84M, DTR 52M| 6 /  84M, DTR  52M
 *   01  | 6 / 133M, DTR 66M| 6 / 104M, DTR 66M| 4 /  70M, DTR  42M
 *   01  | 6 / 1-1-4 104M   |                  |
 *   10  | 8 / 133M, DTR 66M| 8 / 133M, DTR 66M| 8 / 104M, DTR  66M
 *   11  |10 / 166M, DTR 83M|10 / 166M, DTR 83M|10 / 133M, DTR 100M
 *************************************************************************************************
 * History :
 * 2023/12/20 Release V2.0.0
 *            Move enum MXIC_SNOR_WrapAroundTypeDef form Flash header file to MXIC_xSPINOR.h
 *            Rearrange and add comments
 * 2023/05/31 Add define MXIC_SNOR_CR2_DC, MXIC_SNOR_CR2_xOPI
 * 2023/03/31 Add define OCTAL_DATA_SEQUENCE_EVEN_ODD
 * 2023/01/30 Release V1.2.0
 * 2022/12/21 Add define MXIC_SNOR_PACKET_BUFFER_ADDRESS & MXIC_SNOR_PACKET_BUFFER_RESET_ADDRESS
 * 2022/12/12 Release V1.1.0
 * 2022/06/20 Add PKTI & PKTO commands
 *            Exclusive or MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD     with
 *                         MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD
 * 2022/06/15 Release V1.0.0
 * 2021/12/09 Initial version
 *************************************************************************************************/
/* Define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef __MXIC_SNOR_128M_H
#define __MXIC_SNOR_128M_H

#ifdef __cplusplus
 extern "C" {
#endif
/**************************************************************************************************
 * This file supported Flash Part Number
 * 1.8V : MX25U12832F, MX25U12833F, MX25U12835F, MX25U12835FZNI_08G, MX25U12843G, MX25U12845G
 *        MX25U12872F, MX25U12873F, MX25U12873G, MX25U12890F, MX25U12891F, MX25U12892F,
 * 3.3V : MX25L12801F, MX25L12805D, MX25L12833E, MX25L12833F, MX25L12835E, MX25L12835F, MX25L12836E
 *        MX25L12839F, MX25L12845E, MX25L12845G, MX25L12850F, MX25L12855E, MX25L12855F, MX25L12859F
 *        MX25L12865E, MX25L12865F, MX25L12871F, MX25L12872F, MX25L12873F, MX25L12873G, MX25L12875F
 * Octal :
 * 1.8V : MX25UW12345G, MX25UW12845GS, MX25UW12845GR, MX25UW12845GQ
 * RPMC :
 * 1.8V : MX77U12851F
 * 3.3V : MX77L12850F
 *************************************************************************************************/
#define MX25L12833F

/**************************************************************************************************
 * Macronix xSPI-NOR Flash ID
 *************************************************************************************************/
#define MXIC_SNOR_MEMORY_MANUFACTURER_ID    0xC2        // Macronix Manufacture ID = 0xC2

#if defined(MX25L12801F) || defined(MX25L12805D) || defined(MX25L12833E) || defined(MX25L12833F) || \
    defined(MX25L12835E) || defined(MX25L12835F) || defined(MX25L12836E) || defined(MX25L12839F) || \
    defined(MX25L12845E) || defined(MX25L12845G) || defined(MX25L12850F) || defined(MX25L12865E) || \
    defined(MX25L12865F) || defined(MX25L12871F) || defined(MX25L12872F) || defined(MX25L12873F) || \
    defined(MX25L12873G) || defined(MX25L12875F)
#define MXIC_SNOR_MEMORY_TYPE_ID            0x20
#elif defined(MX25U12832F) || defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12835FZNI_08G) || \
      defined(MX25U12843G) || defined(MX25U12845G) || defined(MX25U12872F) || defined(MX25U12873F) || \
      defined(MX25U12873G) || defined(MX25U12890F) || defined(MX25U12892F)
#define MXIC_SNOR_MEMORY_TYPE_ID            0x25
#elif defined(MX25L12855E) || defined(MX25L12855F) || defined(MX25L12859F)
#define MXIC_SNOR_MEMORY_TYPE_ID            0x26
#elif defined(MX77U12851F) || defined(MX77L12850F)
#define MXIC_SNOR_MEMORY_TYPE_ID            0x75
#elif defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define MXIC_SNOR_MEMORY_TYPE_ID            0x81
#elif defined(MX25UW12345G)
#define MXIC_SNOR_MEMORY_TYPE_ID            0x84
#elif defined(MX25U12891F)
#define MXIC_SNOR_MEMORY_TYPE_ID            0x90
#endif

#if defined(MX25L12801F) || defined(MX25L12805D) || defined(MX25L12833E) || defined(MX25L12833F) || \
    defined(MX25L12835E) || defined(MX25L12835F) || defined(MX25L12836E) || defined(MX25L12839F) || \
    defined(MX25L12845E) || defined(MX25L12845G) || defined(MX25L12850F) || defined(MX25L12855E) || \
    defined(MX25L12855F) || defined(MX25L12859F) || defined(MX25L12865E) || defined(MX25L12865F) || \
    defined(MX25L12871F) || defined(MX25L12872F) || defined(MX25L12873F) || defined(MX25L12873G) || \
    defined(MX25L12875F) || \
    defined(MX77L12850F)
#define MXIC_SNOR_MEMORY_CAPACITY_ID        0x18
#elif defined(MX25U12832F)   || defined(MX25U12833F)   || defined(MX25U12835F)   || defined(MX25U12835FZNI_08G) || \
      defined(MX25U12843G)   || defined(MX25U12845G)   || defined(MX25U12872F)   || defined(MX25U12873F)        || \
      defined(MX25U12873G)   || defined(MX25U12890F)   || defined(MX25U12891F)   || defined(MX25U12892F)        || \
      defined(MX25UW12345G)  || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)      || \
      defined(MX77U12851F)
#define MXIC_SNOR_MEMORY_CAPACITY_ID        0x38
#endif

#define MXIC_SNOR_DEVICE_ID ((uint32_t)((MXIC_SNOR_MEMORY_CAPACITY_ID << 16) | (MXIC_SNOR_MEMORY_TYPE_ID << 8) | MXIC_SNOR_MEMORY_MANUFACTURER_ID))

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Size & Page Size
 *************************************************************************************************/
#define MXIC_SNOR_FLASH_SIZE                ((128U / 8U) * 1024U * 1024U)   // 128M bit = 16M Byte
#define MXIC_SNOR_PAGE_SIZE                 256U                            // 256 Byte per Page

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Timing in ms
 *************************************************************************************************/
// Chip Erase Cycle Time;
#if defined(MX25L12833F) || defined(MX25L12871F) || defined(MX25L12872F)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       60000U
#elif defined(MX25U12845G)  || \
      defined(MX25UW12345G) || defined(MX25UW12845GS)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       75000U
#elif defined(MX25L12835F) || defined(MX25L12839F) || defined(MX25L12855F) || defined(MX25L12859F) || \
      defined(MX25L12865F) || defined(MX25L12873F) || defined(MX25L12875F)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       80000U
#elif defined(MX25U12832F) || defined(MX25U12872F) || defined(MX25U12892F) || defined(MX25UW12845GR) || \
      defined(MX25L12845G) || defined(MX25L12873G) || \
      defined(MX77U12851F)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       100000U
#elif defined(MX25L12850F) || \
      defined(MX77L12850F)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       120000U
#elif defined(MX25UW12845GQ)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       125000U
#elif defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12835FZNI_08G) || defined(MX25U12843G) || \
      defined(MX25U12873F) || defined(MX25U12873G) || defined(MX25U12890F)        || defined(MX25U12891F)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       150000U
#elif defined(MX25L12801F)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       160000U
#elif defined(MX25L12805D) || defined(MX25L12833E) || defined(MX25L12835E) || defined(MX25L12836E) || \
      defined(MX25L12845E) || defined(MX25L12855E) || defined(MX25L12865E)
#define MXIC_SNOR_MAX_TIME_ERASE_CHIP       200000U
#endif

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Secured OTP support
 *************************************************************************************************/
#if defined(MX25L12805D)
#define MXIC_SNOR_SECURE_OTP_SIZE           (512U / 8U)         // Secured OTP Memory 512 bit = 64 Byte
#elif defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12835FZNI_08G) || defined(MX25U12843G) || \
      defined(MX25U12873F) || defined(MX25U12873G) || defined(MX25U12890F)        || defined(MX25U12891F) || \
      defined(MX25L12833E) || defined(MX25L12835E) || defined(MX25L12835F)        || defined(MX25L12836E) || \
      defined(MX25L12839F) || defined(MX25L12845E) || defined(MX25L12845G)        || defined(MX25L12850F) || \
      defined(MX25L12855E) || defined(MX25L12855F) || defined(MX25L12859F)        || defined(MX25L12865E) || \
      defined(MX25L12865F) || defined(MX25L12873F) || defined(MX25L12873G)        || defined(MX25L12875F) || \
      defined(MX77L12850F)
#define MXIC_SNOR_SECURE_OTP_SIZE           (4U * 1024U / 8U)   // Secured OTP Memory 4K bit = 512 Byte
#elif defined(MX25U12832F)  || defined(MX25U12845G)   || defined(MX25U12872F)   || defined(MX25U12892F)   || \
      defined(MX25L12833F)  || defined(MX25L12871F)   || defined(MX25L12872F)   || \
      defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ) || \
      defined(MX77U12851F)
#define MXIC_SNOR_SECURE_OTP_SIZE           (8U * 1024U / 8U)   // Secured OTP Memory 8K bit = 1024 Byte
//#define MXIC_SNOR_ADDITIONAL_MEMORY_SIZE    (4U * 1024U / 8U)   // Additional Memory 4K bit = 512 Byte
#endif

//#################################################################################################
//# Macronix xSPI-NOR Flash Commands & Register Group define                                      #
//#################################################################################################
/**************************************************************************************************
 * SUPPORT_DTR                                  // DTR (Double Transfer Rate) access
 * SUPPORT_FAST_READ_111_122_DTR_COMMAND        // Sub-Group, DTR fast read command 1-1-1 & 1-2-2
 *              IO  | 3/4 Byte Address | 4 Byte Address
 *  Read    : 1-1-1 | 0x0D             | 0x0E
 *            1-2-2 | 0xBD             | 0xBE
 *            1-4-4 | 0xED             | 0xEE
 *            4-4-4 | 0xED             | 0xEE
 *            8-8-8 |  -               | 0xEE
 *  Program : 8-8-8 |  -               | 0x12
 *  Erase   : 8-0-0 | 0x60, 0xC7 (Chip Erase)
 *            8-8-0 |  -               | 0x21, 0xDC
 *  Other   : Device dependents
 *
 * RPMC & Octal Flash not support QPI
 *************************************************************************************************/
#if defined(MX25U12843G)  || defined(MX25U12845G)   || defined(MX25U12873G)   || \
    defined(MX25L12845E)  || defined(MX25L12845G)   || defined(MX25L12855E)   || defined(MX25L12865E) || \
    defined(MX25L12873G)  || \
    defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define SUPPORT_DTR                               // DTR (Double Transfer Rate) access

#if defined(MX25L12845E) || defined(MX25L12855E) || defined(MX25L12865E)
#define SUPPORT_FAST_READ_111_122_DTR_COMMAND     // DTR fast read command 1-1-1 & 1-2-2, subset of DTR
#endif  // Sub-set of SUPPORT_DTR
#endif

/**************************************************************************************************
 * SUPPORT_QPI                                  // 4xx access command
 *           Rate |  IO       | 3/4 Byte Address | 4 Byte Address
 *  Read    : STR | 4-4-4     | 0x0B, 0xEA, 0xEB | 0xEC
 *            STR | 4-4-4+4DC | 0x0B, 0xE7       |  -
 *            DTR | 4-4-4     | 0xED             | 0xEE
 *  Program : STR | 4-4-4     | 0x02             | 0x12
 *  Erase   : STR | 4-0-0     | 0x60, 0xC7 (Chip Erase)
 *            STR | 4-4-0     | 0x20, 0x52, 0xD8 | 0x21, 0x5C, 0xDC
 *  Other   : STR | 4-x-x     | Device dependents; QPIID 0xAF, EQIO 0x35, RSTQIO 0xF5
 *
 * RPMC & Octal Flash not support QPI
 *************************************************************************************************/
#if defined(MX25U12832F) || defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12835FZNI_08G) || \
    defined(MX25U12843G) || defined(MX25U12845G) || defined(MX25U12872F) || defined(MX25U12873F)        || \
    defined(MX25U12873G) || defined(MX25U12890F) || defined(MX25U12891F) || defined(MX25U12892F)        || \
    defined(MX25L12833E) || defined(MX25L12833F) || defined(MX25L12835F) || defined(MX25L12839F)        || \
    defined(MX25L12845G) || defined(MX25L12855F) || defined(MX25L12859F) || defined(MX25L12865F)        || \
    defined(MX25L12871F) || defined(MX25L12872F) || defined(MX25L12873F) || defined(MX25L12873G)        || \
    defined(MX25L12875F) || \
    defined(MX77U12851F) || \
    defined(MX77U12851F)
#define SUPPORT_QPI                             // 4xx access command
#endif

/**************************************************************************************************
 *   Octal-Flash running on STR/DTR mode when enter 8-8-8 status that can't mixed use 1-1-1 commands.
 * Data sequence dependents on device define.
 *
 * SUPPORT_OCTAL                                // 8xx access command
 * OCTAL_DATA_SEQUENCE_EVEN_ODD                 // Sub-Group, Data sequence even -> odd address, MXxxx345 parts
 *           Rate |  IO   | 3/4 Byte Address | 4 Byte Address
 *  Read    : STR | 8-8-8 |  -               | 0x7C, 0xEC
 *            DTR | 8-8-8 |  -               | 0xEE
 *  Program : STR | 8-8-8 |  -               | 0x12
 *            DTR | 8-8-8 |  -               | 0x12
 *  Erase   : STR | 8-0-0 | 0x60, 0xC7 (Chip Erase)
 *            STR | 8-8-0 |  -               | 0x21, 0xDC
 *            DTR | 8-0-0 | 0x60, 0xC7 (Chip Erase)
 *            DTR | 8-8-0 |  -               | 0x21, 0xDC
 *  Other   : xTR | 8-x-x | Device dependents
 *
 * Octal Flash constant :
 *  OCTAL_READ_REGISTER_DUMMY_CLOCK         4U
 *  OCTAL_STATUS_REGISTER_ADDRESS           0U
 *  OCTAL_CONFIGURATION_REGISTER_ADDRESS    1U
 *
 * RPMC & Octal Flash not support QPI
 *************************************************************************************************/
#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define SUPPORT_OCTAL                           // 8xx access command

//#define OCTAL_DATA_SEQUENCE_EVEN_ODD          // Data sequence even -> odd address
#endif

/**************************************************************************************************
 *   3 Byte address support up to 128M bit Flash. If device over 128M bit then ...
 * Device provide three ways to access over 128M bit area that use 3 Byte address commands :
 * 1. EN4B 0xB7 & EX4B 0xE9 let 3 Byte address commands enter/exit 4 Byte mode.
 * 2. RDEAR 0xC8 & WREAR 0xC5 to read/write Extended Address Register that point out current bank.
 *    128M bit per bank.
 * 3. Device force 4 Byte address on 3/4 Byte address commands directly.
 *
 * SUPPORT_3BYTE_ADDRESS_COMMAND                // 3/4 Byte address access command
 * SUPPORT_34BYTE_ADDRESS_SWITCH                // Sub-Group, Switch 3/4 Byte addressing mode & EAR operation
 * FORCE_USE_4BYTE_ADDRESS                      // Sub-Group, Force use 4 Byte address on 3/4 Byte commands
 *
 *             Rate |    IO     | 3/4 Byte address
 *            ------+-----------+------------------
 *  Read    :  STR  | 1-1-1+0DC | 0x03
 *                  | 1-1-1     | 0x0B
 *                  | 1-1-2     | 0x3B
 *                  | 1-1-4     | 0x6B
 *                  | 1-1-8     | 0x03
 *                  | 1-2-2     | 0xBB
 *                  | 1-4-4     | 0xEA, 0xEB
 *                  | 1-4-4+4DC | 0xE7
 *                  | 4-4-4     | 0x0B, 0xEA, 0xEB
 *                  | 4-4-4+4DC | 0x0B, 0xE7
 *            ------+-----------+------------------
 *             DTR  | 1-1-1     | 0x0D
 *                  | 1-2-2     | 0xBD
 *                  | 1-4-4     | 0xED
 *                  | 4-4-4     | 0xED
 *
 *  Program :  STR  | 1-1-1     | 0x02
 *                  | 1-1-4     | 0x32
 *                  | 1-1-8     | 0x02
 *                  | 1-4-4     | 0x38
 *                  | 4-4-4     | 0x02
 *            ------+-----------+------------------
 *             DTR  | x-x-x     |  -
 *
 *  Erase   :  STR  | 1-0-0     | 0x60, 0xC7 (Chip Erase)
 *                  | 1-1-0     | 0x20, 0x52, 0xD8
 *                  | 4-0-0     | 0x60, 0xC7 (Chip Erase)
 *                  | 4-4-0     | 0x20, 0x52, 0xD8
 *                  | 8-0-0     | 0x60, 0xC7 (Chip Erase)
 *            ------+-----------+-------------------------
 *             DTR  | 8-0-0     | 0x60, 0xC7 (Chip Erase)
 *  Other   : 0xB7, 0xE9, 0xC5, 0xC8            // Switch 3/4 Byte addressing mode & EAR operation
 *
 *   4 Byte address commands can access up to 32G bit Flash.
 *
 * SUPPORT_4BYTE_ADDRESS_COMMAND                // 4 Byte address access command
 *             Rate |    IO     | 4 Byte address
 *            ------+-----------+----------------
 *  Read    :  STR  | 1-1-1+0DC | 0x13
 *                  | 1-1-1     | 0x0C
 *                  | 1-1-2     | 0x3C
 *                  | 1-1-4     | 0x6C
 *                  | 1-1-8     | 0x7C
 *                  | 1-2-2     | 0xBC
 *                  | 1-4-4     | 0xEC
 *                  | 4-4-4     | 0xEC
 *                  | 8-8-8     | 0x7C, 0xEC
 *            ------+-----------+----------------
 *             DTR  | 1-1-1     | 0x0E
 *                  | 1-2-2     | 0xBE
 *                  | 1-4-4     | 0xEE
 *                  | 4-4-4     | 0xEE
 *                  | 8-8-8     | 0xEE
 *
 *  Program :  STR  | 1-1-1     | 0x12
 *                  | 1-4-4     | 0x3E
 *                  | 4-4-4     | 0x12
 *                  | 8-8-8     | 0x12
 *            ------+-----------+----------------
 *             DTR  | 8-8-8     | 0x12
 *
 *  Erase   :  STR  | 1-0-0     | 0x60, 0xC7 (Chip Erase)
 *                  | 1-1-0     | 0x21, 0x5C, 0xDC
 *                  | 4-0-0     | 0x60, 0xC7 (Chip Erase)
 *                  | 4-4-0     | 0x21, 0x5C, 0xDC
 *                  | 8-0-0     | 0x60, 0xC7 (Chip Erase)
 *                  | 8-8-0     | 0x21, 0xDC
 *            ------+-----------+------------------+------------------
 *             DTR  | 8-0-0     | 0x60, 0xC7 (Chip Erase)
 *                  | 8-8-0     | 0x21, 0xDC
 *  Other   : Device dependents
 *
 *   Parallel mode is a independent mode that need issue enter/exit parallel mode command.
 * Then host can switch 1-1-8/1-1-1 command set to access Flash.
 * 3 Byte address command not support 3/4 Byte switch function.
 *
 * SUPPORT_PARALLEL_MODE                        // 1-1-8 access command after enter Parallel(1-1-8) mode
 *           Rate |   IO  | 3 Byte address | 4 Byte address
 *  Read    : STR | 1-1-8 | 0x03           | 0x7C
 *  Program : STR | 1-1-8 | 0x02           |  -
 *  Erase   : xTR | 1-1-8 |  -             |  -
 *  Other   : STR | 1-0-0 | ENPLM 0x55, EXPLM 0x45
 *            STR | 1-0-8 | RDID 0x9F,  RDSR 0x05
 *            STR | 1-1-8 | 0x90, 0xAB
 *
 * Octal Flash not support 3/4 Byte switch & EAR operation command
 *************************************************************************************************/
#define SUPPORT_3BYTE_ADDRESS_COMMAND           // 3 Byte address access command

//#define SUPPORT_34BYTE_ADDRESS_SWITCH           // 3 Byte address switch to 4 Byte address access command
//#define FORCE_USE_4BYTE_ADDRESS                 // Force use 4 Byte address on 3/4 Byte command

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define SUPPORT_4BYTE_ADDRESS_COMMAND           // 4 Byte address access command
#endif

#if defined(MX25L12836E) || defined(MX25L12845E) || defined(MX25L12865E)
#define SUPPORT_PARALLEL_MODE                   // 1-1-8 access command
#endif

/**************************************************************************************************
 *   Some Read command support Performance Enhance function that send command once only.
 * Host need send out 0xFF data for 8 ~ 10 clock to Release Read Enhanced mode.
 *
 * SUPPORT_PERFORMANCE_ENHANCE_READ             // performance enhance indicator
 *            Rate |  IO   | 3/4 Byte address | 4 Byte address | Enhance Read
 *           ------+-------+------------------+----------------+------------------
 *  Read    : STR  | 1-4-4 | 0xEA, 0xEB       | 0xEC           | 0xEA, 0xEB, 0xEC
 *                 | 4-4-4 | 0x0B, 0xEA, 0xEB | 0xEC           | 0xEA, 0xEB, 0xEC
 *           ------+-------+------------------+----------------+------------------
 *            DTR  | 1-4-4 | 0xED             | 0xEE           | 0xED, 0xEE
 *                 | 4-4-4 | 0xED             | 0xEE           | 0xED, 0xEE
 *  Program :
 *  Erase   :
 *  Other   :
 *
 *   Force read out data wrap around from initial address.
 *
 * SUPPORT_BURST_READ_WRAP_AROUND               // Burst Read Wrap Around
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D SBL 0xC0
 *
 *   Device support Read While Write function.
 *
 * SUPPORT_WRITE_BUFFER_ACCESS                  // Read While Write (RWW) command
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D 0x25, 0x22, 0x24, 0x31
 *
 * Octal Flash not support performance enhance read
 *************************************************************************************************/
#if !defined(SUPPORT_OCTAL) && \
    !defined(MX25L12805D) && !defined(MX25L12836E)  // Not support
#define SUPPORT_PERFORMANCE_ENHANCE_READ        // performance enhance indicator
#endif

#if !defined(MX25L12801F) && !defined(MX25L12805D) && !defined(MX25L12836E) && !defined(MX25L12845E) && \
    !defined(MX25L12850F) && !defined(MX25L12865E) && !defined(MX77L12850F) && !defined(MX25L12855E)  // Not support
#define SUPPORT_BURST_READ_WRAP_AROUND          // Burst Read Wrap Around
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define SUPPORT_WRITE_BUFFER_ACCESS             // Read While Write (RWW) command
#endif

/**************************************************************************************************
 *   The Factory Mode Enable (FMEN) instruction is for enhance Program and Erase performance for
 * increase factory production throughput.
 *
 * SUPPORT_FACTORY_MODE                         // Factory Mode
 *  Read    :
 *  Program : Speed up
 *  Erase   : Speed up
 *  Other   : STR 1-0-0/4-0-0 FMEN 0x41
 *
 *   The Suspend instruction interrupts a Program or Erase operation to allow the device conduct
 * other operations.
 *
 * SUPPORT_PROGRAM_ERASE_SUSPEND                // Program/Erase Suspend
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : STR 1-0-0/4-0-0 0xB0, 0x30, 0x75, 0x7A
 *
 *   Continuous Program mode (CP mode). 0xAD command initial start address once then continuous
 * program 2 Byte data. SO pin can be change to RY/BY# during CP mode.
 *
 * SUPPORT_SO_OUTPUT_RYBY                      // SO to output RY/BY# during CP mode
 *  Read    :
 *  Program : STR 3 Byte 1-1-1 0xAD
 *  Erase   :
 *  Other   : STR 1-0-0 ESRY 0x70, DSRY 0x80
 *
 *   Idle Erase command set is designed for to runs the erase in background, and store the spare
 * area.
 *
 * SUPPORT_IDLE_ERASE                         // Idle Erase mode
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : STR 1-x-x IE 0x5E, RIE 0x5F, RDIEST 0x14
 *************************************************************************************************/
#if defined(MX25U12832F) || defined(MX25U12843G) || defined(MX25U12872F) || defined(MX25U12873G) || \
    defined(MX25U12892F) || \
    defined(MX25L12833F) || defined(MX25L12845G) || defined(MX25L12871F) || defined(MX25L12872F) || \
    defined(MX25L12873G) || \
    defined(MX77U12851F)
#define SUPPORT_FACTORY_MODE                    // Factory Mode
#endif

#if !defined(MX25L12805D) && !defined(MX25L12835E) && !defined(MX25L12836E) && !defined(MX25L12845E) && \
    !defined(MX25L12855E) && !defined(MX25L12865E)   // Not support
#define SUPPORT_PROGRAM_ERASE_SUSPEND           // Program/Erase Suspend
#endif

#if defined(MX25L12833E) || defined(MX25L12835E) || defined(MX25L12855E) || defined(MX25L12865E)
#define SUPPORT_SO_OUTPUT_RYBY                 // SO to output RY/BY# during CP mode
#endif

//#define SUPPORT_IDLE_ERASE                     // Idle Erase mode

/**************************************************************************************************
 *   Armor Flash support Security Field commands.
 * Armor 1 need enter/exit Security Field commands.
 * Armor 2, 3 support PKTI/PKTO commands.
 *
 * SUPPORT_SECURITY_FIELD_COMMAND                // Security Field (Armor Flash)
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : STR 1-1-1 ENSF 0xB2, EXSF 0xC2, PKTI 0x2E, PKTO 0x2A
 *
 *   RPMC Flash support OP1 & OP2 commands.
 *
 * SUPPORT_RPMC                                 // Replay Protected Monotonic Counter
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : STR 1-0-1 OP1 0x9B, OP2 0x96
 *************************************************************************************************/
//#define SUPPORT_SECURITY_FIELD_COMMAND                  // Security Field (Armor Flash)

#if defined(MX77U12851F) || \
    defined(MX77L12850F)
#define SUPPORT_RPMC                            // Replay Protected Monotonic Counter
#endif

/**************************************************************************************************
 * SUPPORT_SOFTWARE_RESET                       // Software Reset
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 1-0-0/4-0-0/8S-0-0/8D-0-0 0x00, 0x66, 0x99
 *************************************************************************************************/
#if !defined(MX25L12805D) && !defined(MX25L12836E) && !defined(MX25L12845E) && !defined(MX25L12855E) && \
    !defined(MX25L12865E)  // Not support
#define SUPPORT_SOFTWARE_RESET                  // Software Reset
#endif

/**************************************************************************************************
 *   Read Serial Flash Discoverable Parameter command
 *
 * SUPPORT_SFDP                                 // Read SFDP
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D 0x5A
 *************************************************************************************************/
#if !defined(MX25L12805D)  // Not support
#define SUPPORT_SFDP                            // SFDP
#endif

/**************************************************************************************************
 *   Device support mixed protection mode :
 * Advanced Sector Protection / Individual Sector Protection; SPB/DPB operations
 * Block protection; Permanent Lock Operations
 * Password protection
 *
 * SUPPORT_ADVANCED_SECTOR_PROTECTION           // Advance Sector Protection / Individual Sector Protection
 * SUPPORT_SPB_LOCK                             // Sub-Group, SPB lock operation
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : SPB/DPB 0x68, 0x7E, 0x98, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0x2D, 0x2C
 *            SPBLK 0xA6, 0xA7
 *
 * SUPPORT_BLOCK_PROTECTION                     // 64K Block write lock protection command
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 0xE2, 0x64, 0xFB, 0x3F, 0x64, 0xA6, 0x3C, 0x36, 0x39, 0x26, 0xD5, 0x28, 0xDD, 0x21
 *
 * SUPPORT_PASSWORD                             // Password protection
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 0x27, 0x28, 0x29
 *
 * SUPPORT_SECURITY_REGISTER                    // Security Register, Mode indicator bit
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : RDSCUR 0x2B
 *
 * ARMOR Flash Not support Advance Sector Protection
 *************************************************************************************************/
#if !defined(SUPPORT_SECURITY_FIELD_COMMAND) && \
    !defined(MX25L12801F) && !defined(MX25L12805D) && !defined(MX25L12850F) && \
    !defined(MX77L12850F)
#define SUPPORT_ADVANCED_SECTOR_PROTECTION      // Advance Sector Protection / Individual Sector Protection

#if defined(MX25L12835F) || defined(MX25L12839F) || defined(MX25L12855F) || defined(MX25L12859F) || \
    defined(MX25L12865F) || defined(MX25L12873F) || defined(MX25L12875F)
#define SUPPORT_SPB_LOCK                        // Sub-function of Advance Sector Protection, 0xA6, 0xA7
#endif
#endif

//#define SUPPORT_BLOCK_PROTECTION                // 64K Block write lock protection command

#if defined(MX25U12845G)  || \
    defined(MX25L12835F)  || defined(MX25L12839F)   || defined(MX25L12855F)   || defined(MX25L12859F) || \
    defined(MX25L12865F)  || defined(MX25L12873F)   || defined(MX25L12875F)   || \
    defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define SUPPORT_PASSWORD                        // Password
#endif

#define SUPPORT_SECURITY_REGISTER               // Security Register

/**************************************************************************************************
 * SUPPORT_FAST_BOOT_REGISTER                   // Fast Boot Register command
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 1-0-1/8S-8S-8S/8D-8D-8D 0x16, 0x17, 0x18
 *************************************************************************************************/
#if defined(MX25U12845G)  || \
    defined(MX25L12835F)  || defined(MX25L12839F)   || defined(MX25L12855F)   || defined(MX25L12859F) || \
    defined(MX25L12865F)  || defined(MX25L12873F)   || defined(MX25L12875F)   || \
    defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define SUPPORT_FAST_BOOT_REGISTER              // Fast Boot Register command
#endif

/**************************************************************************************************
 * SUPPORT_DEEP_POWER_DOWN                      // Deep Power Down
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 1-0-0/4-0-0/8S-0-0/8D-0-0 0xB9, 0xAB
 *************************************************************************************************/
#define SUPPORT_DEEP_POWER_DOWN                 // Deep Power Down

/**************************************************************************************************
 * SUPPORT_RES_REMS_ID                          // Read Electronic ID, Read Electronic Manufacturer ID & Device ID
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : STR 1-1-1/4-4-4/1-1-8 0xAB, 0x90, 1-2-2 0xEF, 1-4-4 0xDF
 *            DTR 1-4-4 0xCF
 *
 * Octal Flash not supported
 *************************************************************************************************/
#if !defined(SUPPORT_OCTAL)
#define SUPPORT_RES_REMS_ID
#endif

/**************************************************************************************************
 * SUPPORT_CONFIGURATION_REGISTER2              // Configuration Register 2 command
 *  Read    :
 *  Program :
 *  Erase   :
 *  Other   : 1-1-1/8S-8S-8S/8D-8D-8D 0x71, 0x72
 *************************************************************************************************/
#if defined(MX25U12892F)  || \
    defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define SUPPORT_CONFIGURATION_REGISTER2         // Configuration Register 2 command
#endif


//#################################################################################################
//# Macronix xSPI-NOR Flash Commands value define                                                 #
//#################################################################################################
/**************************************************************************************************
 * Macronix xSPI-NOR Flash Read Command Pool
 * Normal read command use 0 dummy clock always. It is device dependets frequency limited.
 *************************************************************************************************/
/* 3 Byte or 3/4 Byte address STR command ********************************************************/
#ifdef SUPPORT_3BYTE_ADDRESS_COMMAND
#define MXIC_SNOR_NORMAL_READ_111_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
#define MXIC_SNOR_FAST_READ_111_CMD                     0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4

#if defined(SUPPORT_OCTAL) || \
    defined(MX25L12805D)
// Empty define, Not support 112, 122, 114, 144 commands
#elif defined(MX25L12836E)
#define MXIC_SNOR_FAST_READ_112_CMD                     0x3B    // DREAD, 1I 2O Read 3/4 Byte Address; 1-1-2
#define MXIC_SNOR_FAST_READ_114_CMD                     0x6B    // QREAD, 1I 4O Read 3/4 Byte Address; 1-1-4

#elif defined(MX25L12839F)
#define MXIC_SNOR_FAST_READ_144_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
#define MXIC_SNOR_FAST_READ_114_CMD                     0x6B    // QREAD, 1I 4O Read 3/4 Byte Address; 1-1-4

#elif defined(MX25L12845E) || defined(MX25L12865E)
#define MXIC_SNOR_FAST_READ_122_CMD                     0xBB    // 2READ, 2 x I/O Read 3/4 Byte Address; 1-2-2
#define MXIC_SNOR_FAST_READ_144_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance

#else
#define MXIC_SNOR_FAST_READ_122_CMD                     0xBB    // 2READ, 2 x I/O Read 3/4 Byte Address; 1-2-2
#define MXIC_SNOR_FAST_READ_112_CMD                     0x3B    // DREAD, 1I 2O Read 3/4 Byte Address; 1-1-2
#define MXIC_SNOR_FAST_READ_144_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
//#define MXIC_SNOR_FAST_READ_144_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
#define MXIC_SNOR_FAST_READ_114_CMD                     0x6B    // QREAD, 1I 4O Read 3/4 Byte Address; 1-1-4
#endif  // defined(SUPPORT_OCTAL) || defined(MX25L12805D)

#if defined(MX25U12832F) || defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12835FZNI_08G) || \
    defined(MX25U12843G) || defined(MX25U12872F) || defined(MX25U12873F) || defined(MX25U12873G)        || \
    defined(MX25U12890F) || defined(MX25U12891F) || defined(MX25U12892F) || \
    defined(MX25L12833E) || defined(MX25L12835E)
#define MXIC_SNOR_FAST_READ_144_4DC_CMD                 0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
#endif

#ifdef SUPPORT_PARALLEL_MODE
#define MXIC_SNOR_NORMAL_READ_118_CMD                   0x03    // READ, Normal Read 3/4 Byte Address with 0 dummy clock; 1-1-1/1-1-8
#endif  // SUPPORT_PARALLEL_MODE

/*
 * 3 Byte or 3/4 Byte address STR 4-4-4 command
 * MXIC_SNOR_FREAD_444 mode will use 1 command only that come from 0xEA, 0xEB.
 * 0xEA/0xEB dummy clock dependents on DC[1:0] define & support performance enhance read.
 * Define used command as MXIC_SNOR_FAST_READ_444_CMD
 * 0x0B & 0xE7 STR 4-4-4 with 4 dummy clock command, define used command as MXIC_SNOR_FAST_READ_444_4DC_CMD
 */
#ifdef SUPPORT_QPI
#define MXIC_SNOR_FAST_READ_444_CMD                     0xEB    // 4READ, 4 I/O Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
//#define MXIC_SNOR_FAST_READ_444_CMD1                    0xEA    // 4READ, 4 I/O Read 3/4 Byte Address Top; 1-4-4/4-4-4 + Enhance
//#define MXIC_SNOR_FAST_READ_444_CMD2                    0x0B    // FAST READ, Fast Read 3/4 Byte Address; 1-1-1/4-4-4 + Enhance

#if defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12835FZNI_08G) || defined(MX25U12843G) || \
    defined(MX25U12873F) || defined(MX25U12873G) || defined(MX25U12890F)        || defined(MX25U12891F) || \
    defined(MX25L12833E)
#define MXIC_SNOR_FAST_READ_444_4DC_CMD                 0x0B    // FAST READ, Fast Read 3/4 Byte Address with 4 dummy clock; 1-1-1/4-4-4
#endif

#if defined(MX25U12843G) || defined(MX25U12873G)
#define MXIC_SNOR_FAST_READ_444_4DC_CMD1                0xE7    // W4READ, 4 I/O Read 3/4 Byte Address with 4 dummy clock; 1-4-4/4-4-4
#endif
#endif  // SUPPORT_QPI

/* 3 Byte or 3/4 Byte address DTR command ********************************************************/
#ifdef SUPPORT_DTR
#ifdef SUPPORT_FAST_READ_111_122_DTR_COMMAND
#define MXIC_SNOR_FAST_READ_111_DTR_CMD                 0x0D    // FASTDTRD, Fast DTR Read 3/4 Byte Address; 1-1-1
#define MXIC_SNOR_FAST_READ_122_DTR_CMD                 0xBD    // 2DTRD, Dual I/O DTR Read 3/4 Byte Address; 1-2-2
#endif  // SUPPORT_FAST_READ_111_122_DTR_COMMAND

#if !defined(SUPPORT_OCTAL)  // Not support
#define MXIC_SNOR_FAST_READ_144_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
#endif  // !defined(SUPPORT_OCTAL)

#ifdef SUPPORT_QPI
#define MXIC_SNOR_FAST_READ_444_DTR_CMD                 0xED    // 4DTRD, Quad I/O DTR Read 3/4 Byte Address; 1-4-4/4-4-4 + Enhance
#endif  // SUPPORT_QPI
#endif  // SUPPORT_DTR
#endif  // SUPPORT_3BYTE_ADDRESS_COMMAND

/* 4 Byte address STR command ********************************************************************/
#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
#define MXIC_SNOR_4BYTE_ADDRESS_NORMAL_READ_111_CMD     0x13    // READ4B, Normal Read 4 Byte address with 0 dummy clock; 1-1-1
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_CMD       0x0C    // FAST READ4B, Fast Read 4 Byte address; 1-1-1

#if !defined(SUPPORT_OCTAL)  // Not support
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_112_CMD       0x3C    // DREAD4B, Read by Dual Output 4 Byte address; 1-1-2
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_114_CMD       0x6C    // QREAD4B, Read by Quad Output 4 Byte address; 1-1-4
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_CMD       0xBC    // 2READ4B, Read by 2 x I/O 4 Byte address; 1-2-2
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
#endif  // !defined(SUPPORT_OCTAL)

#ifdef SUPPORT_QPI
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_CMD       0xEC    // 4READ4B, Read by 4 x I/O 4 Byte address; 1-4-4/4-4-4/8S-8S-8S + Enhance
#endif  // SUPPORT_QPI

#ifdef SUPPORT_OCTAL
//#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_118_CMD       0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
//#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD1      0x7C    // OREAD4B, Read by 8 x I/O 4 Byte address; 1-1-8/8S-8S-8S
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_CMD       0xEC    // 8READ,   Read by 8 x I/O 4 Byte address; 8S-8S-8S
#endif  // SUPPORT_OCTAL

/* 4 Byte address DTR command ********************************************************************/
#ifdef SUPPORT_DTR
#ifdef SUPPORT_FAST_READ_111_122_DTR_COMMAND
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_111_DTR_CMD   0x0E    // FRDTRD4B, Fast DTR Read 4 Byte address; 1-1-1
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_122_DTR_CMD   0xBE    // 2DTRD4B, DTR read by 2 x I/O 4 Byte address; 1-2-2
#endif  // SUPPORT_FAST_READ_111_122_DTR_COMMAND

#if !defined(SUPPORT_OCTAL)  // Not support
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_144_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D + Enhance
#endif  // !defined(SUPPORT_OCTAL)

#ifdef SUPPORT_QPI
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_444_DTR_CMD   0xEE    // 4DTRD4B, Quad I/O DTR Read 4 Byte address; 1-4-4/4-4-4/8D-8D-8D + Enhance
#endif  // SUPPORT_QPI

#ifdef SUPPORT_OCTAL
#define MXIC_SNOR_4BYTE_ADDRESS_FAST_READ_888_DTR_CMD   0xEE    // 8DTRD,   Octal I/O DTR Read 4 Byte address; 8D-8D-8D
#endif  // SUPPORT_OCTAL
#endif  // SUPPORT_DTR
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Page Program Command Pool
 *************************************************************************************************/
/* 3 Byte or 3/4 Byte Address Page Program Command ***********************************************/
#ifdef  SUPPORT_3BYTE_ADDRESS_COMMAND
#define MXIC_SNOR_PAGE_PROG_111_CMD                     0x02   // PP, Page Program 3/4 Byte Address; 1-1-1/4-4-4/1-1-8

#if defined(MX25L12833E) || defined(MX25L12835E) || defined(MX25L12836E) || defined(MX25L12845E) || \
    defined(MX25L12855E) || defined(MX25L12865E)
#define MXIC_SNOR_CONTINUOUSLY_PROG_111_CMD             0xAD   // CP, Continuously Program 2 Byte data + 3 Byte Address; 1-1-1
#endif

//#define MXIC_SNOR_PAGE_PROG_114_CMD                     0x32   // QPP, 1I4P Page Program 3 Byte Address; 1-1-4

#if !defined(SUPPORT_OCTAL) && \
    !defined(MX25L12801F) && !defined(MX25L12805D)  // Not support
#define MXIC_SNOR_PAGE_PROG_144_CMD                     0x38   // 4PP, Quad Page Program 3/4 Byte Address; 1-4-4
#endif  // !defined(SUPPORT_OCTAL)

#ifdef SUPPORT_PARALLEL_MODE
#define MXIC_SNOR_PAGE_PROG_118_CMD                     0x02   // PP, Page Program 3/4 Byte Address; 1-1-1/4-4-4/1-1-8
#endif  // SUPPORT_PARALLEL_MODE

#ifdef SUPPORT_QPI
#define MXIC_SNOR_PAGE_PROG_444_CMD                     0x02   // PP, Page Program 3/4 Byte Address; 1-1-1/4-4-4/1-1-8
#endif  // SUPPORT_QPI
#endif  // SUPPORT_3BYTE_ADDRESS_COMMAND

/* 4 Byte Address Page Program Command ***********************************************************/
#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
#define MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_111_CMD       0x12   // PP4B, Page Program 4 Byte address; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D

#if !defined(SUPPORT_OCTAL)  // Not support
#define MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_144_CMD       0x3E   // 4PP4B, Quad Input Page Program 4 Byte address; 1-4-4
#endif  // !defined(SUPPORT_OCTAL)

#ifdef SUPPORT_QPI
#define MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_444_CMD       0x12   // PP4B, Page Program 4 Byte address; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
#endif  // SUPPORT_QPI

#ifdef SUPPORT_OCTAL
#define MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_888_CMD       0x12   // PP4B, Page Program 4 Byte address; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
#define MXIC_SNOR_4BYTE_ADDRESS_PAGE_PROG_888_DTR_CMD   0x12   // PP4B, Page Program 4 Byte address; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
#endif  // SUPPORT_OCTAL
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Erase Command Pool
 *************************************************************************************************/
/* 0 Byte Address Chip Erase Command */
#define MXIC_SNOR_ERASE_CHIP_CMD                        0x60   // CE, Chip Erase 0 Byte Address; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_ERASE_CHIP_CMD1                       0xC7   // CE, Chip Erase 0 Byte Address; 1-0-0/4-0-0/8S-0-0/8D-0-0

/* 3 Byte or 3/4 Byte Address Erase Command ******************************************************/
#ifdef  SUPPORT_3BYTE_ADDRESS_COMMAND
#define MXIC_SNOR_ERASE_4K_CMD                          0x20   // SE, Sector Erase 4KB 3/4 Byte Address; 1-1-0/4-4-0

#if !defined(SUPPORT_OCTAL) && \
    !defined(MX25L12801F) && !defined(MX25L12805D)  // Not support
#define MXIC_SNOR_ERASE_32K_CMD                         0x52   // BE32K, Block Erase 32KB 3/4 Byte Address; 1-1-0/4-4-0
#endif  // !defined(SUPPORT_OCTAL)

#define MXIC_SNOR_ERASE_64K_CMD                         0xD8   // BE, Block Erase 64KB 3/4 Byte Address; 1-1-0/4-4-0
#endif  // SUPPORT_3BYTE_ADDRESS_COMMAND

/* 4 Byte Address Erase Command ******************************************************************/
#ifdef SUPPORT_4BYTE_ADDRESS_COMMAND
#define MXIC_SNOR_4BYTE_ADDRESS_ERASE_4K_CMD            0x21   // SE4B, Sector Erase 4KB 4 Byte address; 1-1-0/4-4-0/8S-8S-0/8D-8D-0

#if !defined(SUPPORT_OCTAL)  // Not support
#define MXIC_SNOR_4BYTE_ADDRESS_ERASE_32K_CMD           0x5C   // BE32K4B, Block Erase 32KB 4 Byte address; 1-1-0/4-4-0
#endif  // !defined(SUPPORT_OCTAL)

#define MXIC_SNOR_4BYTE_ADDRESS_ERASE_64K_CMD           0xDC   // BE4B, Block Erase 64KB 4 Byte address; 1-1-0/4-4-0/8S-8S-0/8D-8D-0
#endif  // SUPPORT_4BYTE_ADDRESS_COMMAND

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Write Buffer Access Command Pool; RWW (Read While Write)
 *************************************************************************************************/
#ifdef SUPPORT_WRITE_BUFFER_ACCESS
#define MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_READ_CMD      0x25   // RDBUF, Write Buffer Read    (4 Byte address); 1-1-1/8S-8S-8S/8D-8D-8D
#define MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_INITIAL_CMD   0x22   // WRBI, Write Buffer Initial  (4 Byte address); 1-1-1/8S-8S-8S/8D-8D-8D
#define MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONTINUE_CMD  0x24   // WRCT, Write Buffer Continue (4 Byte address); 1-1-1/8S-8S-8S/8D-8D-8D
#define MXIC_SNOR_4BYTE_ADDRESS_WRITE_BUF_CONFIRM_CMD   0x31   // WRCF, Write Buffer Confirm  (4 Byte address); 1-1-1/8S-0-0/8D-0-0
#endif  // SUPPORT_WRITE_BUFFER_ACCESS

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Register Access Command Pool
 *************************************************************************************************/
/* Write Enable bit Operations */
#define MXIC_SNOR_WRITE_ENABLE_CMD                      0x06   // WREN, Write Enable; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_WRITE_DISABLE_CMD                     0x04   // WRDI, Write Disable; 1-0-0/4-0-0/8S-0-0/8D-0-0

/* Status & Configuration Register Operations */
#define MXIC_SNOR_READ_STATUS_REG_CMD                   0x05   // RDSR, Read Status Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D

#if !defined(MX25L12805D) && !defined(MX25L12833E) && !defined(MX25L12835E) && !defined(MX25L12836E) && \
    !defined(MX25L12845E) && !defined(MX25L12855E) && !defined(MX25L12865E)  // Not support
#define MXIC_SNOR_READ_CONFIGURATION_REG_CMD            0x15   // RDCR, Read Configuration Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
#endif

#define MXIC_SNOR_WRITE_STATUS_CONFIGURATION_REG_CMD    0x01   // WRSR, Write Status/Configuration Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D

/* Identification Operations */
#define MXIC_SNOR_READ_ID_CMD                           0x9F   // RDID, Read IDentification; 1-0-1/8S-8S-8S/8D-8D-8D/1-0-8

#ifdef SUPPORT_RES_REMS_ID
#define MXIC_SNOR_READ_ELECTRONIC_ID_CMD                0xAB   // RES, Read Electronic ID; 1-1-1/4-4-4/1-1-8

#if !defined(MX25L12839F)  // Not support
#define MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD    0x90   // REMS, Read Electronic Manufacturer ID & Device ID; 1-1-1/1-1-8
#endif

#if defined(MX25L12833E) || defined(MX25L12835E) || defined(MX25L12836E) || defined(MX25L12845E) || \
    defined(MX25L12855E) || defined(MX25L12865E)
#define MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD2   0xEF   // REMS2, Read Electronic Manufacturer ID & Device ID; 1-2-2
#define MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4   0xDF   // REMS4, Read Electronic Manufacturer ID & Device ID; 1-4-4
#endif

#if defined(MX25L12845E) || defined(MX25L12855E) || defined(MX25L12865E)
#define MXIC_SNOR_READ_ELECTRONIC_MANFACTURER_ID_CMD4D  0xCF   // REMS4D, Read Electronic Manufacturer ID & Device ID; 1-4-4 DTR
#endif
#endif  // SUPPORT_RES_REMS_ID

#ifdef SUPPORT_QPI
#define MXIC_SNOR_MULTIPLE_IO_READ_ID_CMD               0xAF   // QPIID, QPI ID Read; QPI 4-0-4
/* QPI mode Operations; Enter 4xx/Exit to 1xx Command */
#define MXIC_SNOR_ENABLE_QSPI_CMD                       0x35   // EQIO, Enable QPI; 1-0-0
#define MXIC_SNOR_RESET_QSPI_CMD                        0xF5   // RSTQIO, Reset QPI; QPI 4-0-0
#endif  // SUPPORT_QPI

#ifdef SUPPORT_CONFIGURATION_REGISTER2
#define MXIC_SNOR_READ_CONFIGURATION_REG2_CMD           0x71   // RDCR2, Read Configuration Register 2; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
#define MXIC_SNOR_WRITE_CONFIGURATION_REG2_CMD          0x72   // WRCR2, Write Configuration Register 2; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
#endif  // SUPPORT_CONFIGURATION_REGISTER2

/* 3/4 Byte addressing mode Operations; Enter/Exit 3 Byte address command accept 4 Byte address mode */
#ifdef SUPPORT_34BYTE_ADDRESS_SWITCH
#define MXIC_SNOR_ENTER_4BYTE_ADDRESS_MODE_CMD          0xB7   // EN4B, Enter 4-Byte mode (3/4 Byte address commands); 1-0-0/4-0-0
#define MXIC_SNOR_EXIT_4BYTE_ADDRESS_MODE_CMD           0xE9   // EX4B, Exit 4-Byte mode (3/4 Byte address commands); 1-0-0/4-0-0
/* Extended Address Register Operations; Switch over 128M bit Flash bank for 3 Byte address command */
#define MXIC_SNOR_READ_EXTENDED_ADDRESS_REG_CMD         0xC8   // RDEAR, Read Extended Address Register; 1-0-1/4-0-4
#define MXIC_SNOR_WRITE_EXTENDED_ADDRESS_REG_CMD        0xC5   // WREAR, Write Extended Address Register; 1-0-1/4-0-4
#endif  // SUPPORT_34BYTE_ADDRESS_SWITCH

/* Fast Boot Register Operations */
#ifdef SUPPORT_FAST_BOOT_REGISTER
#define MXIC_SNOR_READ_FAST_BOOT_REGISTER_CMD           0x16   // RDFBR, Read Fast Boot Register; 1-0-1/8S-8S-8S/8D-8D-8D
#define MXIC_SNOR_WRITE_FAST_BOOT_REGISTER_CMD          0x17   // WRFBR, Write Fast Boot Register; 1-0-1/8S-8S-8S/8D-8D-8D
#define MXIC_SNOR_ERASE_FAST_BOOT_REGISTER_CMD          0x18   // ESFBR, Erase Fast Boot Register; 1-0-0/8S-0-0/8D-0-0
#endif  // SUPPORT_FAST_BOOT_REGISTER

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Protection Command Pool
 *************************************************************************************************/
/* Secured OTP/Additional Memory Array Operations */
#ifdef MXIC_SNOR_SECURE_OTP_SIZE
#define MXIC_SNOR_ENTER_SECURED_OTP_CMD                 0xB1   // ENSO, Enter Secured OTP; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_EXIT_SECURED_OTP_CMD                  0xC1   // EXSO, Exit Secured OTP; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_WRITE_SECURITY_REGISTER_CMD           0x2F   // WRSCUR, Write Security Register; 1-0-0/4-0-0/8S-0-0/8D-0-0

#elif defined(MXIC_SNOR_ADDITIONAL_MEMORY_SIZE)
#define MXIC_SNOR_ENTER_4K_BIT_MODE_CMD                 0xA5   // EN4K, Enter 4K bit Mode; 1-0-0
#define MXIC_SNOR_EXIT_4K_BIT_MODE_CMD                  0xB5   // EX4K, Exit 4K bit Mode; 1-0-0
#endif  // MXIC_SNOR_SECURE_OTP_SIZE

/* Security Register Operations */
#ifdef SUPPORT_SECURITY_REGISTER
#define MXIC_SNOR_READ_SECURITY_REGISTER_CMD            0x2B   // RDSCUR, Read Security Register; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D

#if defined(MX25L12833E) || defined(MX25L12835E) || defined(MX25L12836E) || defined(MX25L12845E) || \
    defined(MX25L12855E) || defined(MX25L12865E)
#define MXIC_SNOR_CLEAR_SECURITY_REGISTER_FAIL_FLAG_CMD 0x30   // CLSR, Clear Security Register bit 6 & bit 5; 1-0-0
#endif
#endif  // SUPPORT_SECURITY_REGISTER

/* SPB/DPB/Permanent Lock Operations; Advanced Sector Protection */
#ifdef SUPPORT_ADVANCED_SECTOR_PROTECTION
/* SPB/DPB Operations; Advanced Sector Protection */
#define MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD           0x68   // WPSEL, Write Protect Selection; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_GANG_BLOCK_LOCK_CMD                   0x7E   // GBLK, Gang Block Lock; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_GANG_BLOCK_UNLOCK_CMD                 0x98   // GBULK, Gang Block Unlock; 1-0-0/4-0-0/8S-0-0/8D-0-0

#if defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12835FZNI_08G) || defined(MX25U12843G) || \
    defined(MX25U12873F) || defined(MX25U12873G) || defined(MX25U12890F)        || defined(MX25U12891F) || \
    defined(MX25L12833E) || defined(MX25L12835E) || defined(MX25L12836E)        || defined(MX25L12845E) || \
    defined(MX25L12855E) || defined(MX25L12865E)
#define MXIC_SNOR_SINGLE_BLOCK_LOCK_CMD                 0x36   // SBLK, Single Block Lock; 1-1-0 + 3/4 Byte Address
#define MXIC_SNOR_SINGLE_BLOCK_UNLOCK_CMD               0x39   // SBULK, Single Block Un-Lock; 1-1-0 + 3/4 Byte Address
#define MXIC_SNOR_READ_BLOCK_LOCK_STATUS_CMD            0x3C   // RDBLOCK, Read Block Lock Status; 1-1-1 + 3/4 Byte Address

#else
#define MXIC_SNOR_READ_DPB_REGISTER_CMD                 0xE0   // RDDPB, Read DPB Register; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
#define MXIC_SNOR_WRITE_DPB_REGISTER_CMD                0xE1   // WRDPB, Write DPB Register; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
#define MXIC_SNOR_READ_SPB_STATUS_CMD                   0xE2   // RDSPB, Read SPB Status; 1-1-1/8S-8S-8S/8D-8D-8D + 4 Byte Address
#define MXIC_SNOR_WRITE_SPB_CMD                         0xE3   // WRSPB, SPB bit program; 1-1-0/8S-8S-0/8D-8D-0 + 4 Byte Address
#define MXIC_SNOR_ERASE_SPB_CMD                         0xE4   // ESSPB, All SPB bit Erase; 1-0-0/8S-0-0/8D-0-0

/* Lock Register Operations */
#define MXIC_SNOR_READ_LOCK_REGISTER_CMD                0x2D   // RDLR, Read Lock Register; 1-0-1/8S-8S-8S/8D-8D-8D
#define MXIC_SNOR_WRITE_LOCK_REGISTER_CMD               0x2C   // WRLR, Write Lock Register; 1-0-1/8S-8S-8S/8D-8D-8D
#endif

#ifdef SUPPORT_SPB_LOCK                                        // Command 0xA6, 0xA7 & SPB Lock Register
#define MXIC_SNOR_SPB_LOCK_SET_CMD                      0xA6   // SPBLK, SPB Lock set; 1-0-0
#define MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD            0xA7   // RDSPBLK, SPB Lock Register Read; 1-0-1
#endif  // SUPPORT_SPB_LOCK
#endif  // SUPPORT_ADVANCED_SECTOR_PROTECTION

/* Permanent Lock Operations */
#ifdef SUPPORT_BLOCK_PROTECTION
#define MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD1     0xD5   // RRLCR, Read Read-Lock Area Configuration Register; 1-0-1
#define MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD1    0xDD   // WRLCR, Write Read-Lock Area Configuration Register; 1-0-1
#define MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD      0x26   // RRLCR, Read Read-Lock Area Configuration Register; 1-0-1
#define MXIC_SNOR_WRITE_READ_LOCK_CONFIGURATION_CMD     0x28   // WRLCR, Write Read-Lock Area Configuration Register; 1-0-1

#define MXIC_SNOR_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD      0xA6   // PLLK, Permanent Lock bit Lock Down; 1-0-0
#define MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD 0xA7   // RDPLLK, Read Permanent Lock bit Lock Down; 1-1-1
#define MXIC_SNOR_PERMANENT_LOCK_CMD                    0x64   // PLOCK, Permanent Lock; 1-1-0 + 3/4 Byte Address

//#if 0
//#define MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_CMD        0x3F   // RDPLOCK, Read Block Permanent Lock Status; 1-1-1 + 3/4 Byte Address
//#elif 0
//#define MXIC_SNOR_READ_PERMANENT_LOCK_STATUS_8DC_CMD    0x3F   // RDPLOCK, Read Block Permanent Lock Status; 1-1-1 + 3 Byte Address + 8 DC
//#endif

#define MXIC_SNOR_WRITE_READ_LOCK_BIT_CMD               0x21   // WRLB, Write Read-Lock Bit; 1-0-1
//#define MXIC_SNOR_BLOCK_WRITE_LOCK_PROTECTION_CMD       0xE2   // BLOCKP, Block Write Lock Protection; 1-1-0
//#define MXIC_SNOR_READ_BLOCK_WRITE_LOCK_STATUS_CMD      0xFB   // RDBLOCK, Read Block Write Lock status; 1-1-1
//#define MXIC_SNOR_CHIP_UNPROCECT_CMD                    0xF3   // UNLOCK, Chip unprotect; 1-0-0
#endif  // SUPPORT_BLOCK_PROTECTION

/* Password Register Operations */
#ifdef SUPPORT_PASSWORD
#define MXIC_SNOR_READ_PASSWORD_REGISTER_CMD            0x27   // RDPASS, Read Password Register; 1-0-1
#define MXIC_SNOR_WRITE_PASSWORD_REGISTER_CMD           0x28   // WRPASS, Write Password Register; 1-0-1
#define MXIC_SNOR_PASSWORD_UNLOCK_CMD                   0x29   // PASSULK, Password Unlock; 1-0-1
#endif  // SUPPORT_PASSWORD

/* Replay Protected Monotonic Counter */
#ifdef SUPPORT_RPMC
#define MXIC_SNOR_RPMC_OP1_CMD                          0x9B   // OP1, RPMC Operation 1; SPI 1-0-1
#define MXIC_SNOR_RPMC_OP2_CMD                          0x96   // OP2, RPMC Operation 2; SPI 1-0-1
#endif  // SUPPORT_RPMC

/* Armor Flash Enter/Exit Security Field Operations */
#ifdef SUPPORT_SECURITY_FIELD_COMMAND
#if 0  // Armor 2
#define MXIC_SNOR_PACKET_IN_CMD                         0x2E   // PKTI, Packet In; SPI 1-1-1
#define MXIC_SNOR_PACKET_OUT_CMD                        0x2A   // PKTO, Packet Out; SPI 1-1-1
#else
#define MXIC_SNOR_ENTER_SECURITY_FIELD_CMD              0xB2   // ENSF, Enter Security Field; SPI 1-0-0
#define MXIC_SNOR_EXIT_SECURITY_FIELD_CMD               0xC2   // EXSF, Exit Security Field; SPI 1-0-0
#endif

#define MXIC_SNOR_PACKET_BUFFER_RESET_ADDRESS           0x007FFFE0  // Write this address to reset packet buffer pointer
#define MXIC_SNOR_PACKET_BUFFER_ADDRESS                 0x007FFE00  // Packet buffer address for read/write packet command
#endif  // SUPPORT_SECURITY_FIELD_COMMAND

/**************************************************************************************************
 * Macronix xSPI-NOR Flash Miscellaneous Command Pool
 *************************************************************************************************/
/* None & Reset Operations */
#ifdef SUPPORT_SOFTWARE_RESET
//#if 0
//#define MXIC_SNOR_SOFTWARE_RESET_CMD                    0xFF   // SWRES, Software Reset; 1-0-0
//#else
#define MXIC_SNOR_NO_OPERATION_CMD                      0x00   // NOP, No Operation; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_RESET_ENABLE_CMD                      0x66   // RSTEN, Reset Enable; 1-0-0/4-0-0/8S-0-0/8D-0-0
#define MXIC_SNOR_RESET_MEMORY_CMD                      0x99   // RST, Reset Memory; 1-0-0/4-0-0/8S-0-0/8D-0-0
//#endif
#endif  // SUPPORT_SOFTWARE_RESET

/* Read Serial Flash Discoverable Parameter command */
#ifdef SUPPORT_SFDP
#define MXIC_SNOR_READ_SFDP_CMD                         0x5A    // RDSFDP, Read Serial Flash Discoverable Parameter; 1-1-1/4-4-4/8S-8S-8S/8D-8D-8D
#endif  // SUPPORT_SFDP

/* Enter/Release from Program/Erase Suspend Operations */
#ifdef SUPPORT_PROGRAM_ERASE_SUSPEND
#if defined(MX25U12890F) || \
    defined(MX25L12833E)
#define MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD             0x75   // PGM/ERS Suspend, Suspends Program/Erase; 1-0-0/4-0-0
#define MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD              0x7A   // PGM/ERS Resume, Resumes Program/Erase; 1-0-0/4-0-0

#else
#define MXIC_SNOR_PROGRAM_ERASE_SUSPEND_CMD             0xB0   // PGM/ERS Suspend, Suspends Program/Erase; 1-0-0/4-0-0
#define MXIC_SNOR_PROGRAM_ERASE_RESUME_CMD              0x30   // PGM/ERS Resume, Resumes Program/Erase; 1-0-0/4-0-0
#endif
#endif  // SUPPORT_PROGRAM_ERASE_SUSPEND

/* Deep Power Down Operations */
#ifdef SUPPORT_DEEP_POWER_DOWN
#define MXIC_SNOR_DEEP_POWER_DOWN_CMD                   0xB9   // DP, Deep power down; 1-0-0/4-0-0/8S-0-0/8D-0-0

#if !defined(MX25U12832F) && !defined(MX25U12872F) && !defined(MX25U12892F)  // Not support
#define MXIC_SNOR_RELEASE_FROM_DEEP_POWER_DOWN_CMD      0xAB   // RDP, Release from Deep Power down; 1-0-0/4-0-0/8S-0-0/8D-0-0/1-1-8
#endif
#endif  // SUPPORT_DEEP_POWER_DOWN

/* Factory Mode Enable Operations */
#ifdef SUPPORT_FACTORY_MODE
#define MXIC_SNOR_FACTORY_MODE_ENABLE_CMD               0x41   // FMEN, Factory Mode Enable; 1-0-0/4-0-0

#if defined(MX25U12832F) || defined(MX25U12872F) || defined(MX25U12892F) || \
    defined(MX77U12851F)
#define MXIC_SNOR_READ_FACTORY_MODE_STATUS_REGISTER_CMD 0x44   // RDFMSR, Read Factory Mode Status Register; 1-0-1/4-0-4
#endif
#endif  // SUPPORT_FACTORY_MODE

/* Burst Read Wrap Around Operations */
#ifdef SUPPORT_BURST_READ_WRAP_AROUND
#if defined(MX25L12833E) || defined(MX25L12835E)
#define MXIC_SNOR_SET_BURST_LENGTH_CMD                  0x77   // SBL, Set burst length; 1-0-1/4-0-4

#else
#define MXIC_SNOR_SET_BURST_LENGTH_CMD                  0xC0   // SBL, Set burst length; 1-0-1/4-0-4/8S-8S-8S/8D-8D-8D
#endif
#endif  // SUPPORT_BURST_READ_WRAP_AROUND

// Special Commands *******************************************************************************
/* SO to output RY/BY# during CP mode */
#ifdef SUPPORT_SO_OUTPUT_RYBY
#define MXIC_SNOR_ENABLE_SO_OUTPUT_RYBY_CMD             0x70   // ESRY, Enable SO to output RY/BY# during CP mode; 1-0-0
#define MXIC_SNOR_DISABLE_SO_OUTPUT_RYBY_CMD            0x80   // DSRY, Disable SO to output RY/BY# during CP mode; 1-0-0
#define MXIC_SNOR_HOLD_ENABLE_CMD                       0xAA   // HDE, Hold Enable, Enable HOLD# pin function; 1-0-0
#endif  // SUPPORT_RYBY_OUTPUT

#ifdef SUPPORT_PARALLEL_MODE
#define MXIC_SNOR_ENTER_PARALLEL_MODE_CMD               0x55   // ENPLM, Enter Parallel Mode; 1-0-0
#define MXIC_SNOR_EXIT_PARALLEL_MODE_CMD                0x45   // EXPLM, Exit Parallel Mode; 1-0-0
#endif  // SUPPORT_PARALLEL_MODE

#ifdef SUPPORT_IDLE_ERASE
#define MXIC_SNOR_IDLE_ERASE_CMD                        0x5E   // IE, Idele Erase; 1-0-0
#define MXIC_SNOR_RELEASE_FROM_IDLE_ERASE_CMD           0x5F   // RIE, Release from Idele Erase; 1-0-0
#define MXIC_SNOR_READ_IDLE_ERASE_STATUS_CMD            0x14   // RDIEST, Read Idele Erase Status; 1-0-1

typedef struct              // Status Register
{
  uint8_t DM : 2;           // Device Mode; 0 = Stand by, 1 = Idle Erase, 2 = Release Idle Erase
  uint8_t SSS : 2;          // Spare Sector Status; 0 = Empty, 1 = Full, 2 = Other
  uint8_t Reserved : 4;
} MXIC_SNOR_IdleEraseStatus_t;
#endif  // SUPPORT_IDLE_ERASE

//#################################################################################################
//# Macronix xSPI-NOR Flash Register bit & location define                                        #
//#################################################################################################
/**************************************************************************************************
 * Status Register
 *  QE bit in Status Register[6]                // Effect BSP action
 *  SRWD bit in Status Register[7]              // Not effect BSP action
 *************************************************************************************************/
#define MXIC_SNOR_SR_WIP        0x01U     /*!< Write in progress */
#define MXIC_SNOR_SR_WEL        0x02U     /*!< Write enable latch */
#define MXIC_SNOR_SR_BP         0x3CU     /*!< Block protected against program and erase operations */

#if !defined(SUPPORT_OCTAL) && \
    !defined(MX25L12805D)  // Not support
#define MXIC_SNOR_SR_QE         0x40U     /*!< QE bit; Quad IO mode enabled if =1 */
#endif

#if !defined(SUPPORT_OCTAL) && !defined(SUPPORT_RPMC) && \
    !defined(MX25U12872F) && !defined(MX25U12873F) && !defined(MX25U12873G) && !defined(MX25U12891F) && \
    !defined(MX25L12871F) && !defined(MX25L12873G)  // Not support
#define MXIC_SNOR_SR_SRWD       0x80U     /*!< Status register write enable/disable */
#endif

typedef struct              // Status Register
{
  uint8_t WIP : 1;          // Write in progress; 1 = Writing Operation
  uint8_t WEL : 1;          // Write enable latch; 1 = Write Enable
  uint8_t BP : 4;           // Block protected against program and erase operations

#ifdef MXIC_SNOR_SR_QE
  uint8_t QE : 1;           // QE bit; 1 = Quad Enable
#else
  uint8_t Reserved : 1;
#endif

#ifdef MXIC_SNOR_SR_SRWD
  uint8_t SRWD : 1;         // Status register write enable/disable; 1 = Status Register Write Disabled
#else
  uint8_t Reserved1 : 1;
#endif
} MXIC_SNOR_StatusRegister_t;

/**************************************************************************************************
 * Configuration Register
 * Accessed by RDCR 0x51, WRSR 0x01 commands
 * ODS    : Output Driver Strength; This index is device dependents.
 *          Modify ODS_INDEX to change Flash Output Driver Strength
 * DC     : Dummy clock cycle; This index is device dependents & SCLK frequency related
 *          Modify DC_INDEX to change dummy clock cycle.
 *************************************************************************************************/
#ifdef MXIC_SNOR_READ_CONFIGURATION_REG_CMD
#if defined(SUPPORT_SECURITY_FIELD_COMMAND) || \
    defined(MX25L12850F) || defined(MX77L12850F)
// Empty, Configuration Register not support ODS, Output Driver Strength

#elif defined(MX25L12845G) || defined(MX25L12873G)
#define MXIC_SNOR_CR_ODS        0x03U     /*!< Output driver strength */

#else
#define MXIC_SNOR_CR_ODS        0x07U     /*!< Output driver strength */
#endif

#define MXIC_SNOR_CR_TB         0x08U     /*!< Top/Bottom bit used to configure the block protect area */

#if defined(SUPPORT_OCTAL) || \
    defined(MX25U12843G) || defined(MX25U12845G) || defined(MX25U12873G) || \
    defined(MX25L12845G) || defined(MX25L12873G)
#define MXIC_SNOR_CR_PBE        0x10U     /*!< Preamble Bit Enable */
#endif

#if defined(SUPPORT_OCTAL) || \
    defined(MX25U12833F) || defined(MX25U12835F) || defined(MX25U12873F) || defined(MX25U12891F) || \
    defined(MX25L12850F) || \
    defined(MX77L12850F)
// Empty, Configuration Register not support Dummy Clock

#elif defined(MX25U12835FZNI_08G) || defined(MX25U12890F)  // DC 1 bit only
#define MXIC_SNOR_CR_DC         0x80U     /*!< Dummy Clock Cycles setting, 1 bit @ bit 7 */

#else
#define MXIC_SNOR_CR_DC         0xC0U     /*!< Dummy Clock Cycles setting, 2 bit */
#endif

typedef struct              // Configuration Register
{
#if MXIC_SNOR_CR_ODS == 0x03U
  uint8_t ODS : 2;          // Output driver strength, 2 bit ODS
  uint8_t Reserved : 1;
#elif MXIC_SNOR_CR_ODS == 0x07U
  uint8_t ODS : 3;          // Output driver strength
#else
  uint8_t Reserved : 3;
#endif

  uint8_t TB : 1;           // Top/Bottom bit used to configure the block protect area; 1 = Bottom Area Protect (Default = 0)

#ifdef MXIC_SNOR_CR_PBE
  uint8_t PBE : 1;          // Preamble Bit Enable; 1 = Enable
#else
  uint8_t Reserved1 : 1;
#endif

  uint8_t Reserved2 : 1;

#if MXIC_SNOR_CR_DC == 0x80U
  uint8_t Reserved3 : 1;
  uint8_t DC : 1;           // Dummy Clock Cycles setting, 1 bit @ bit 7
#elif MXIC_SNOR_CR_DC == 0xC0U
  uint8_t DC : 2;           // Dummy Clock Cycles setting, 2 bit
#else
  uint8_t Reserved3 : 2;
#endif
} MXIC_SNOR_ConfigurationRegister_t;

#if 0                       // RDCR command support read out CR1 & CR2
#define MXIC_SNOR_CR2_LHSwitch  ((uint8_t)0x02)   /*!< L/H Switch */

typedef struct              // Configuration Register 2
{
  uint8_t Reserved : 1;
  uint8_t LHSwitch : 1;     // 0 = Ultra low power mode, 1 = High performance mode
  uint8_t Reserved2 : 6;
} MXIC_SNOR_ConfigurationRegister2_t;
#endif

#define ODS_INDEX 7         // Modify ODS_INDEX to change Flash Output Driver Strength, Strength mapping is device dependents

#if MXIC_SNOR_CR_DC == 0x80U    // Configuration Register DC 1 bit
#define DC_INDEX  1             // Modify DC_INDEX to change Dummy Clock Cycle, Clock mapping is device dependents

#if DC_INDEX == 0
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B
#define MXIC_SNOR_DUMMY_CLOCK_122        4    // 0xBB
#define MXIC_SNOR_DUMMY_CLOCK_144        6    // 0xEB
#define MXIC_SNOR_DUMMY_CLOCK_444        6    // 0xEB
#elif DC_INDEX == 1
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B
#define MXIC_SNOR_DUMMY_CLOCK_122        4    // 0xBB
#define MXIC_SNOR_DUMMY_CLOCK_144        8    // 0xEB
#define MXIC_SNOR_DUMMY_CLOCK_444        8    // 0xEB
#endif

#elif MXIC_SNOR_CR_DC == 0xC0U  // Configuration Register DC 2 bit
#define DC_INDEX  3             // Modify DC_INDEX to change Dummy Clock Cycle, Clock mapping is device dependents

#if DC_INDEX == 0
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        4    // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        6    // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        6    // 0xEB, 0xEC

#ifdef SUPPORT_DTR
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    8    // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    4    // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    6    // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    6    // 0xED, 0xEE
#endif  // SUPPORT_DTR

#elif DC_INDEX == 1
#if defined(MX25U12843G) || defined(MX25U12873G) || \
    defined(MX25L12845G) || defined(MX25L12873G)
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        8    // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        4    // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        4    // 0xEB, 0xEC
#else  //
#define MXIC_SNOR_DUMMY_CLOCK_111        6    // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        6    // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        6    // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        6    // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        4    // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        4    // 0xEB, 0xEC
#endif

#ifdef SUPPORT_DTR
#if defined(MX25U12843G) || defined(MX25U12873G) || \
    defined(MX25L12845G) || defined(MX25L12873G)
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    6    // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    6    // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    6    // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    6    // 0xED, 0xEE
#else
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    6    // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    6    // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    4    // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    4    // 0xED, 0xEE
#endif
#endif  // SUPPORT_DTR

#elif DC_INDEX == 2
#if defined(MX25U12843G) || defined(MX25U12873G) || \
    defined(MX25L12845G) || defined(MX25L12873G)
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        4    // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        8    // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        8    // 0xEB, 0xEC
#else
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        8    // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        8    // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        8    // 0xEB, 0xEC
#endif

#ifdef SUPPORT_DTR
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    8    // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    8    // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    8    // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    8    // 0xED, 0xEE
#endif  // SUPPORT_DTR

#elif DC_INDEX == 3
#if defined(MX25U12843G) || defined(MX25U12873G) || \
    defined(MX25L12845G) || defined(MX25L12873G)
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        8    // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        10   // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        10   // 0xEB, 0xEC
#else
#define MXIC_SNOR_DUMMY_CLOCK_111        10   // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        10   // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        10   // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        10   // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        10   // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        10   // 0xEB, 0xEC
#endif

#ifdef SUPPORT_DTR
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    10   // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    10   // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    10   // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    10   // 0xED, 0xEE
#endif  // SUPPORT_DTR
#endif  // DC_INDEX == ?

#elif !defined(SUPPORT_OCTAL)   // Not a Octal Flash & Configuration Register not support DC
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B
#define MXIC_SNOR_DUMMY_CLOCK_122        4    // 0xBB
#define MXIC_SNOR_DUMMY_CLOCK_144        6    // 0xEB
#define MXIC_SNOR_DUMMY_CLOCK_444        6    // 0xEB

#ifdef SUPPORT_DTR
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    8    // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    4    // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    6    // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    6    // 0xED, 0xEE
#endif  // SUPPORT_DTR
#endif  // MXIC_SNOR_CR_DC == 0x80U

#else   // MXIC_SNOR_READ_CONFIGURATION_REG_CMD, Device not support Configuration Register
#define MXIC_SNOR_DUMMY_CLOCK_111        8    // 0x0B, 0x0C
#define MXIC_SNOR_DUMMY_CLOCK_112        8    // 0x3B, 0x3C
#define MXIC_SNOR_DUMMY_CLOCK_114        8    // 0x6B, 0x6C
#define MXIC_SNOR_DUMMY_CLOCK_122        4    // 0xBB, 0xBC
#define MXIC_SNOR_DUMMY_CLOCK_144        6    // 0xEB, 0xEC
#define MXIC_SNOR_DUMMY_CLOCK_444        6    // 0xEB, 0xEC

#ifdef SUPPORT_DTR
#if defined(MX25L12845E) || defined(MX25L12855E) || defined(MX25L12865E)
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    6    // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    6    // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    8    // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    6    // 0xED, 0xEE
#else
#define MXIC_SNOR_DUMMY_CLOCK_111_DTR    6    // 0x0D, 0x0E
#define MXIC_SNOR_DUMMY_CLOCK_122_DTR    4    // 0xBD, 0xBE
#define MXIC_SNOR_DUMMY_CLOCK_144_DTR    6    // 0xED, 0xEE
#define MXIC_SNOR_DUMMY_CLOCK_444_DTR    6    // 0xED, 0xEE
#endif
#endif  // SUPPORT_DTR
#endif  // MXIC_SNOR_READ_CONFIGURATION_REG_CMD

/**************************************************************************************************
 * Configuration Register 2
 * Accessed by RDCR2 0x71, WRCR2 0x72 commands
 *************************************************************************************************/
#ifdef SUPPORT_CONFIGURATION_REGISTER2
typedef enum                        // Device Dependents
{
  CR2_00000000h = 0x00000000,
  CR2_00000200h = 0x00000200,
  CR2_00000300h = 0x00000300,
  CR2_00000400h = 0x00000400,
  CR2_00000500h = 0x00000500,
  CR2_00000800h = 0x00000800,
  CR2_00000C00h = 0x00000C00,
  CR2_00000D00h = 0x00000D00,
  CR2_00000E00h = 0x00000E00,
  CR2_00000F00h = 0x00000F00,
  CR2_40000000h = 0x40000000,
  CR2_40000100h = 0x40000100,
  CR2_40000200h = 0x40000200,
  CR2_40000300h = 0x40000300,
  CR2_40000400h = 0x40000400,
  CR2_40000500h = 0x40000500,
  CR2_40000600h = 0x40000600,
  CR2_40000700h = 0x40000700,
  CR2_60000000h = 0x60000000,
  CR2_80000000h = 0x80000000,
  CR2_80000100h = 0x80000100,
  CR2_Cx000000h = 0xC0000000,
} MXIC_SNOR_CR2AddressTypeDef;

/* Configuration Register 2 */
#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define MXIC_SNOR_CR2_xOPI              0x03U   /* SOPI/DOPI mode select*/
#define MXIC_SNOR_CR2_xOPI_111_STR      0x00U   /* 1-1-1/STR */
#define MXIC_SNOR_CR2_xOPI_888_STR      0x01U   /* 8-8-8/STR */
#define MXIC_SNOR_CR2_xOPI_888_DTR      0x02U   /* 8-8-8/DTR */

typedef struct
{
  uint8_t SOPI : 1;         // STR OPI Enable
  uint8_t DOPI : 1;         // DTR OPI Enable
  uint8_t Reserved : 1;     // Reserved
  uint8_t CRCEN : 1;        // Enable Parity Checking; 1 = Parity Check Enable
  uint8_t Reserved1 : 4;    // Reserved
} MXIC_SNOR_CR2_00000000h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t DQSPRC : 1;       // DTR DQS pre-cycle
          // 0 = 0 Cycle
          // 1 = 1 Cycle
  uint8_t DOS : 1;          // DQS on STR mode; 1 = Enable
  uint8_t Reserved : 6;     // Reserved
} MXIC_SNOR_CR2_00000200h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define MXIC_SNOR_CR2_DC                0x07U   /*!< Dummy Clock Cycles setting */

typedef struct
{
  uint8_t DC : 3;           // Dummy cycle; Refer to "Dummy Cycle and Frequency Table (MHz)"
  uint8_t Reserved : 5;     // Reserved
} MXIC_SNOR_CR2_00000300h_t;

#define DC_INDEX  0         // Modify DC_INDEX to change Dummy Clock Cycle, Clock mapping is device dependents

#if DC_INDEX == 0
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       20
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   20

#elif DC_INDEX == 1
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       18
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   18

#elif DC_INDEX == 2
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       16
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   16

#elif DC_INDEX == 3
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       14
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   14

#elif DC_INDEX == 4
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       12
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   12

#elif DC_INDEX == 5
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       10
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   10

#elif DC_INDEX == 6
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       8
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   8

#elif DC_INDEX == 7
#define MXIC_SNOR_DUMMY_CLOCK_111       8
#define MXIC_SNOR_DUMMY_CLOCK_888       6
#define MXIC_SNOR_DUMMY_CLOCK_888_DTR   6
#endif  // DC_INDEX == ?
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t ECS : 2;          // ECS# pin goes low define
          // 00 = 2 bit error or double programmed
          // 01 = 1 or 2 bit error or double programmed
          // 10 = 2 bit error only
          // 11 = 1 or 2 bit error
  uint8_t Reserved : 6;     // Reserved
} MXIC_SNOR_CR2_00000400h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t PPTSEL : 1;       // Preamable pattern selection; refer to "9-3-2. Preamable Pattern Select Bit Table"
  uint8_t Reserved : 3;     // Reserved
  uint8_t CRCBEN : 1;       // CRC# output enable; 1 = CRC# output Enable
  uint8_t CRC_CYC : 2;      // CRC chunk size configuration
          // 00 = 16Byte
          // 01 = 32Byte
          // 10 = 64Byte
          // 11 = 128Byte
  uint8_t CRCBIN : 1;       // CRC# input enable; 1 = CRC# input Enable
} MXIC_SNOR_CR2_00000500h_t;
#endif

/*
 * CR2 ECC address mapping
 * 00000800h = 04000800h = 08000800h = 0C000800
 * 00000C00h = 04000C00h = 08000C00h = 0C000C00
 * 00000D00h = 04000D00h = 08000D00h = 0C000D00
 * 00000E00h = 04000E00h = 08000E00h = 0C000E00
 * 00000F00h = 04000F00h = 08000F00h = 0C000F00
 */
#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t ECCCNT : 4;       // ECC failure chunk counter
  uint8_t ECCFS : 3;        // ECC fail status
          // 000 = None
          // xx1 = 1 bit corrected
          // x1x = 2 bits deteced
          // 1xx = Double programmed page detected
  uint8_t ECCFAVLD : 1;     // ECC fail address valid indicator
          // 0 = ECC failure address invalid (no fail address recorded)
          // 1 = ECC failure address valid (there's fail address recorded)
} MXIC_SNOR_CR2_00000800h_t;

typedef struct
{
  uint8_t Reserved : 4;     // Reserved
  uint8_t ECCFA : 4;        // ECC failure chunk address; ECC 1st failure chunk address(A7:A4)
} MXIC_SNOR_CR2_00000C00h_t;

typedef struct
{
  uint8_t ECCFA : 8;        // ECC failure chunk address; ECC 1st failure chunk address(A15:A8)
} MXIC_SNOR_CR2_00000D00h_t;

typedef struct
{
  uint8_t ECCFA : 8;        // ECC failure chunk address; ECC 1st failure chunk address(A23:A16)
} MXIC_SNOR_CR2_00000E00h_t;

typedef struct
{
  uint8_t ECCFA : 2;        // ECC failure chunk address; ECC 1st failure chunk address(A25:A24)
  uint8_t Reserved : 6;     // Reserved
} MXIC_SNOR_CR2_00000F00h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t DEFSOPI : 1;      // Enable SOPI after Power on or reset (OTP)
  uint8_t DEFDOPI : 1;      // Enable DOPI after Power on or reset (OTP)
  uint8_t Reserved : 1;     // Reserved
  uint8_t CRCEN : 1;        // Enable Parity checking (OTP); 1 = Parity check Disable,
  uint8_t Reserved1 : 4;    // Reserved
} MXIC_SNOR_CR2_40000000h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t FTRLKDN : 1;      // Lock down all the NV bit of CR2 (OTP)
          // 0 = All NV bit of CR2 are unlock
          // 1 = All NV bit of CR2 are lockdown
  uint8_t Reserved : 7;     // Reserved
} MXIC_SNOR_CR2_40000100h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t DQSPRC : 1;       // DQS pre-toggle at DOPI mode (OTP)
          // 0 = 0 cycle
          // 1 = 1 cycle
  uint8_t DOS : 1;          // DQS on STR mode (OTP); 1 = Enable
  uint8_t Reserved : 6;     // Reserved
} MXIC_SNOR_CR2_40000200h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t DC : 3;           // Dummy cycle (OTP); Refer to "Dummy Cycle and Frequency Table (MHz)"
  uint8_t Reserved : 5;     // Reserved
} MXIC_SNOR_CR2_40000300h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t ECS : 2;          // ECS# pin goes low define (OTP)
          // 00 = 2 bit error or double programmed
          // 01 = 1 or 2 bit error or double programmed
          // 10 = 2 bit error only
          // 11 = 1 or 2 bit error
  uint8_t Reserved : 6;     // Reserved
} MXIC_SNOR_CR2_40000400h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t DLPPTSEL : 1;     // Learning pattern selection (1 for logic CKB) (OTP); refer to "9-3-2. Preamable Pattern Select Bit Table"
  uint8_t Reserved : 3;     // Reserved
  uint8_t CRCBEN : 1;       // CRC# output enable (OTP); 1 = CRC# output Enable
  uint8_t CRC_CYC : 2;      // CRC chunk size configuration (OTP)
          // 00 = 16Byte
          // 01 = 32Byte
          // 10 = 64Byte
          // 11 = 128Byte
  uint8_t CRCBIN : 1;       // CRC# input enable (OTP); 1 = CRC# input Enable
} MXIC_SNOR_CR2_40000500h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t ODS : 3;          // Output strength selection (OTP); Refer to "Table 6. Output Driver Strength Table"
  uint8_t Reserved : 1;     // Reserved
  uint8_t PBE : 1;          // Data learning pattern enable (OTP); 1 = Enable
  uint8_t Reserved1 : 3;    // Reserved
} MXIC_SNOR_CR2_40000600h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t WRAPSIZE : 2;     // Wrap around size selection (OTP)
          // 00 = Reserved
          // 01 = 16 Byte
          // 10 = 32 Byte
          // 11 = 64 Byte
  uint8_t Reserved : 2;     // Reserved
  uint8_t ENWRAP : 1;       // Enable wrap around # (OTP); 1 = Disable
  uint8_t Reserved1 : 3;    // Reserved
} MXIC_SNOR_CR2_40000700h_t;
#endif

#if defined(MX25U12892F)
typedef struct
{
  uint8_t BTS : 1;          // Boot Swap; 1 = Boot Swap
  uint8_t BTSPS : 1;        // Boot Swap Pre Set; 1 = Boot Swap Pre Set
  uint8_t Reserved : 6;     // Reserved
} MXIC_SNOR_CR2_60000000h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t Reserved : 3;     // Reserved
  uint8_t WBSS : 1;         // Write buffer sequence status; 1 = Initiated
  uint8_t CRCERR : 1;       // CMD# or Parity checked fail; 1 = CMD# or Parity check fail
  uint8_t Reserved1 : 3;    // Reserved
} MXIC_SNOR_CR2_80000000h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t BKST0 : 1;        // Bank0 status; 1 = Bank during program/erase, 0 = Bank is ready
  uint8_t BKST1 : 1;        // Bank1 status; 1 = Bank during program/erase, 0 = Bank is ready
  uint8_t BKST2 : 1;        // Bank2 status; 1 = Bank during program/erase, 0 = Bank is ready
  uint8_t BKST3 : 1;        // Bank3 status; 1 = Bank during program/erase, 0 = Bank is ready
  uint8_t Reserved : 4;     // Reserved
} MXIC_SNOR_CR2_80000100h_t;
#endif

#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
typedef struct
{
  uint8_t RWWDS : 1;        // RWW Device Status; 0 = ready, 1 = busy
  uint8_t RWWBS : 1;        // RWW Bank Status; 0 = ready, 1 = busy
  uint8_t Reserved : 6;     // Reserved
} MXIC_SNOR_CR2_Cx000000h_t;
#endif
#endif  // SUPPORT_CONFIGURATION_REGISTER2

/**************************************************************************************************
 * Fast Boot Register
 *************************************************************************************************/
#ifdef SUPPORT_FAST_BOOT_REGISTER
#define MXIC_SNOR_FBR_FBE               ((uint32_t)0x00000001) /*!< FastBoot Enable */
#define MXIC_SNOR_FBR_FBSD              ((uint32_t)0x00000006) /*!< FastBoot Start Delay Cycle */
#define MXIC_SNOR_FBR_FBSA              ((uint32_t)0xFFFFFFF0) /*!< FastBoot Start Address */

typedef struct              // Fast Boot Register
{
  uint32_t FBE : 1;         // FBE Fast Boot Enable; 1 = Enable
  uint32_t FBSD : 2;        // FBSD FastBoot Start Delay Cycle
           // 00 :  7 delay cycles; Octal 11
           // 01 :  9 delay cycles; Octal 15
           // 10 : 11 delay cycles; Octal 17
           // 11 : 13 delay cycles; Octal 21
  uint32_t Reserved : 1;    // Reserved
  uint32_t FBSA : 28;       // FBSA Fast Boot Start Address
           // 16 Byted boundary address for the start of boot code access
} MXIC_SNOR_FastBootRegister_t;
#endif  // SUPPORT_FAST_BOOT_REGISTER

/**************************************************************************************************
 * Security Register
 *************************************************************************************************/
#ifdef SUPPORT_SECURITY_REGISTER
#ifdef MXIC_SNOR_SECURE_OTP_SIZE
#define MXIC_SNOR_SCUR_OTP_INDICATOR    ((uint8_t)0x01)    /*!< Secured OTP indicator bit */
#define MXIC_SNOR_SCUR_LDSO             ((uint8_t)0x02)    /*!< Indicate if Lock-down Secured OTP */
#endif

#if !defined(MX25L12805D) && !defined(MX25L12835E) && !defined(MX25L12836E) && !defined(MX25L12845E) && \
    !defined(MX25L12855E) && !defined(MX25L12865E)
#define MXIC_SNOR_SCUR_PSB              ((uint8_t)0x04)    /*!< Program Suspend bit */
#define MXIC_SNOR_SCUR_ESB              ((uint8_t)0x08)    /*!< Erase Suspend bit */
#endif

#ifdef MXIC_SNOR_ENTER_SECURITY_FIELD_CMD
#define MXIC_SNOR_SCUR_ENSF             ((uint8_t)0x10)    /*!< Security Field bit */
#elif defined(MXIC_SNOR_CONTINUOUSLY_PROG_111_CMD)
#define MXIC_SNOR_SCUR_CPM              ((uint8_t)0x10)    /*!< Continuously Program mode */
#endif

#if !defined(MX25L12805D)
#define MXIC_SNOR_SCUR_P_FAIL           ((uint8_t)0x20)    /*!< Indicate Program failed */
#define MXIC_SNOR_SCUR_E_FAIL           ((uint8_t)0x40)    /*!< Indicate Erase failed */
#endif

#ifdef MXIC_SNOR_WRITE_PROTECT_SELECTION_CMD
#define MXIC_SNOR_SCUR_WPSEL            ((uint8_t)0x80)    /*!< Write Protection Selection */
#endif

typedef struct              // Security Register
{
#ifdef MXIC_SNOR_SECURE_OTP_SIZE
  uint8_t SecuredOTP : 1;   // Secured OTP indicator bit; 1 = Factory Lock
  uint8_t LDSO : 1;         // Lock-down Secured OTP; 1 = Lock Down
#else
  uint8_t Reserved : 2;     // Reserved
#endif

#ifdef MXIC_SNOR_SCUR_PSB
  uint8_t PSB : 1;          // Program Suspend bit; 1 = Progarm Suspended
#else
  uint8_t Reserved2 : 1;    // Reserved
#endif

#ifdef MXIC_SNOR_SCUR_ESB
  uint8_t ESB : 1;          // Erase Suspend bit; 1 = Erase Suspended
#else
  uint8_t Reserved3 : 1;    // Reserved
#endif

#ifdef MXIC_SNOR_SCUR_ENSF
  uint8_t ENSF : 1;         // Security Field bit; 1 = Enter Security Field
#elif defined(MXIC_SNOR_SCUR_CPM)
  uint8_t CPM : 1;          // Continuously Program mode; 1 = CP mode (Default = 0)
#else
  uint8_t Reserved4 : 1;    // Reserved
#endif

#ifdef MXIC_SNOR_SCUR_P_FAIL
  uint8_t P_FAIL : 1;       // Program Failed; 1 = Indicate Program Failed (Default = 0)
#else
  uint8_t Reserved5 : 1;    // Reserved
#endif

#ifdef MXIC_SNOR_SCUR_E_FAIL
  uint8_t E_FAIL : 1;       // Erase Failed; 1 = Indicate Erase Failed (Default = 0)
#else
  uint8_t Reserved6 : 1;    // Reserved
#endif

#ifdef MXIC_SNOR_SCUR_WPSEL
  uint8_t WPSEL : 1;        // Write Protection Selection (Default = 0)
          // 0 = Normal WP Mode
          // 1 = Advanced Sector Protection Mode
#else
  uint8_t Reserved7 : 1;    // Reserved
#endif
} MXIC_SNOR_SecurityRegister_t;
#endif  // SUPPORT_SECURITY_REGISTER

/**************************************************************************************************
 * Advance Sector Protection / Individual Sector Protection Function
 * Lock Register, DPB Register, SPB Register & SPB Lock Register
 * Octal Flash Lock Register is a 8 bit structure, others is 16 bit structure
 *************************************************************************************************/
#ifdef SUPPORT_ADVANCED_SECTOR_PROTECTION
// DPB/SPB Register ***********************************************************************************
typedef enum
{
  MXIC_SNOR_BLOCK_UNLOCKED = 0,
  MXIC_SNOR_BLOCK_LOCKED   = 0xFF,
} MXIC_SNOR_BlockLockTypeDef;

typedef enum
{
  MXIC_SNOR_DPB_UNPROTECT = 0,
  MXIC_SNOR_DPB_PROTECT   = 0xFF,
} MXIC_SNOR_DPBRegisterTypeDef;

typedef enum
{
  MXIC_SNOR_SPB_UNPROTECT = 0,
  MXIC_SNOR_SPB_PROTECT   = 0xFF,
} MXIC_SNOR_SPBRegisterTypeDef;

// Lock or Read-Lock Register *********************************************************************
#ifdef MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD
#define MXIC_SNOR_RLCR_RL0  0b00000001U
#define MXIC_SNOR_RLCR_RL1  0b00000010U
#define MXIC_SNOR_RLCR_RL2  0b00000100U
#define MXIC_SNOR_RLCR_RL3  0b00001000U
#define MXIC_SNOR_RLCR_RL4  0b00010000U
#define MXIC_SNOR_RLCR_RL5  0b00100000U
#define MXIC_SNOR_RLCR_RL6  0b01000000U
#define MXIC_SNOR_RLCR_RL7  0b10000000U

typedef struct              // 8 bit Register
{
  uint8_t RL0 : 1;          // 0 = Unlock, 1 = Read locked
  uint8_t RL1 : 1;          // 0 = Unlock, 1 = Read locked
  uint8_t RL2 : 1;          // 0 = Unlock, 1 = Read locked
  uint8_t RL3 : 1;          // 0 = Unlock, 1 = Read locked
  uint8_t RL4 : 1;          // 0 = Unlock, 1 = Read locked
  uint8_t RL5 : 1;          // 0 = Unlock, 1 = Read locked
  uint8_t RL6 : 1;          // 0 = Unlock, 1 = Read locked
  uint8_t RL7 : 1;          // 0 = Unlock, 1 = Read locked
} MXIC_SNOR_ReadLockRegister_t;

#elif defined(MXIC_SNOR_READ_LOCK_REGISTER_CMD)
// Lock Register **********************************************************************************
#if defined(MX25UW12345G) || defined(MX25UW12845GS) || defined(MX25UW12845GR) || defined(MX25UW12845GQ)
#define MXIC_SNOR_LR_PWDMLB             ((uint8_t)0x04) /*!< Password Protection Mode Lock bit (OTP) */
#define MXIC_SNOR_LR_SPBLKDN            ((uint8_t)0x40) /*!< SPB Lock Down (OTP) */

typedef struct              // 8 bit Register
{
  uint8_t Reserved : 2;    // Reserved for Future Use
  uint8_t PWDMLB : 1;      // Password Protection Mode Lock bit (OTP, Default = 1)
           // 0 = Password Protection Mode Enable
           // 1 = Solid Protection Mode
  uint8_t Reserved1 : 3;   // Reserved for Future Use
  uint8_t SPBLKDN : 1;     // SPB Lock Down bit
           // 0 = SPB bit Protected
           // 1 = SPB bit Unprotected
  uint8_t Reserved2 : 1;   // Reserved for Future Use
} MXIC_SNOR_LockRegister_t;

#else                      // 16 bit Register
#if defined(MX25L12835F) || defined(MX25L12839F) || defined(MX25L12855F) || defined(MX25L12859F) || \
    defined(MX25L12865F) || defined(MX25L12873F) || defined(MX25L12875F)
#define MXIC_SNOR_LR_SPBMLB             ((uint16_t)0x0002) /*!< Solid Protection Mode Lock bit (OTP) */
#endif

#if defined(MX25U12845G) || \
    defined(MX25L12835F) || defined(MX25L12839F) || defined(MX25L12855F) || defined(MX25L12859F) || \
    defined(MX25L12865F) || defined(MX25L12873F) || defined(MX25L12875F)
#define MXIC_SNOR_LR_PWDMLB             ((uint16_t)0x0004) /*!< Password Protection Mode Lock bit (OTP) */
#endif

#if defined(MX25L12855F) || defined(MX25L12859F)
#define MXIC_SNOR_LR_SPBOTP             ((uint16_t)0x0008) /*!< SPB OTP bit (OTP) */
#endif

#if defined(MX25L12855F) || defined(MX25L12859F)
#define MXIC_SNOR_LR_RDPME              ((uint16_t)0x0020) /*!< Read Protect Mode Enable bit (OTP, Default = 1) */
#endif

#if defined(MX25U12832F) || defined(MX25U12845G) || defined(MX25U12872F) || defined(MX25U12892F) || \
    defined(MX25L12833F) || defined(MX25L12845G) || defined(MX25L12871F) || defined(MX25L12872F) || \
    defined(MX25L12873G) || \
    defined(MX77U12851F)
#define MXIC_SNOR_LR_SPBLKDN            ((uint16_t)0x0040) /*!< SPB Lock Down (OTP) */
#endif

typedef struct              // 16 bit Register
{
  uint16_t Reserved0 : 1;   // Reserved for Future Use

#ifdef MXIC_SNOR_LR_SPBMLB
  uint16_t SPBMLB : 1;      // Solid Protection Mode Lock bit (OTP, Default = 1)
           // 0 = Solid Protection Mode Enable
           // 1 = Solid Protection Mode not Enable
#else
  uint16_t Reserved1 : 1;   // Reserved for Future Use
#endif

#ifdef MXIC_SNOR_LR_PWDMLB
  uint16_t PWDMLB : 1;      // Password Protection Mode Lock bit (OTP, Default = 1)
           // 0 = Password Protection Mode Enable
           // 1 = Password Protection Mode not Enable
#else
  uint16_t Reserved2 : 1;   // Reserved for Future Use
#endif

#ifdef MXIC_SNOR_LR_SPBOTP
  uint16_t SPBOTP : 1;      // SPB OTP bit (OTP, Default = 1)
           // 0 = SPB Erase command disabled, SPB is OTP bit.
           // 1 = SPB Erase command enabled.
#else
  uint16_t Reserved3 : 1;   // Reserved for Future Use
#endif

  uint16_t Reserved4 : 1;   // Reserved for Future Use

#ifdef MXIC_SNOR_LR_RDPME
  uint16_t RDPME : 1;       // Read Protect Mode Enable bit (OTP, Default = 1)
           // 0 = Read Protect Mode enabled.
           // 1 = Read Protect Mode not enabled.
#else
  uint16_t Reserved5 : 1;   // Reserved for Future Use
#endif

#ifdef MXIC_SNOR_LR_SPBLKDN
  uint16_t SPBLKDN : 1;     // SPB Lock Down (OTP); 1 = SPB changeable, 0 = Freeze SPB
#else
  uint16_t Reserved6 : 1;   // Reserved for Future Use
#endif

  uint16_t Reserved7 : 9;   // Reserved for Future Use
} MXIC_SNOR_LockRegister_t;
#endif
#endif  // MXIC_SNOR_READ_READ_LOCK_CONFIGURATION_CMD

// Command 0xA6, 0xA7; Permanent or SPB Lock Register *********************************************
#ifdef MXIC_SNOR_READ_PERMANENT_LOCK_BIT_LOCK_DOWN_CMD
#define MXIC_SNOR_PLDR_PLLOCK            (uint8_t)0x01  /*!< Permanent lock bit */

typedef struct              // Permanent Lock Down Register
{
  uint8_t PLLOCK : 1;       // SPB Lock bit
          // 0 = Permanent lock bit lock down
          // 1 = Permanent lock bit not lock down
  uint8_t Reserved : 7;     // Reserved
} MXIC_SNOR_PermanentLockDownRegister_t;

#elif defined(MXIC_SNOR_READ_SPB_LOCK_REGISTER_CMD)
// SPB Lock Register ******************************************************************************
#define MXIC_SNOR_SPBLR_SPBLK           (uint8_t)0x01)  /*!< SPB Lock Bit */

typedef struct              // SPB Lock Register
{
  uint8_t SPBLK : 1;        // SPB Lock bit
          // 0 = SPBs Protected (Password Protect`ion Mode Default)
          // 1 = SPBs Unprotected (Solid Protection Mode Default)
  uint8_t Reserved : 7;     // Reserved
} MXIC_SNOR_SPBLockRegister_t;
#endif
#endif  // SUPPORT_ADVANCED SECTOR PROTECTION

/**************************************************************************************************
 * Replay Protected Monotonic Counter
 * Extended Status Register
 *************************************************************************************************/
#ifdef SUPPORT_RPMC
#define MXIC_SNOR_RPMC_KEY_SIZE   32    // 32 Byte

typedef enum
{
  MXIC_SNOR_RPMC_WRITE_ROOT_KEY_REGISTER = 0,
  MXIC_SNOR_RPMC_UPDATE_HMAC_KEY_REGISTER,
  MXIC_SNOR_RPMC_INCREMENT_MONOTONIC_COUNTER,
  MXIC_SNOR_RPMC_REQUEST_MONOTONIC_COUNTER,
} MXIC_SNOR_RPMCCmdTypeDef;

#define MXIC_SNOR_RPMC_ESR_BUSY         0x01  /*!< Extended Status Register busy bit */
#define MXIC_SNOR_RPMC_ESR_SUCCESSFUL   0x80  /*!< RPMC OP1 command succefssful completion status */

typedef struct            // RPMC OP2 Read data type
{
  uint8_t ESR;            // 1 Byte Extended Status Register
  uint8_t Tag[12];        // 12 Byte Tag
  uint8_t CRD[4];         // 4 Byte Counter Read Data; Big endian
  uint8_t Signature[32];  // 32 Byte Signature
} __attribute__(( packed )) RPMC_OP2ReadData_t;
#endif  // SUPPORT_RPMC

#ifdef __cplusplus
}
#endif

#endif /* __MXIC_SNOR_128M_H */
/************************ (C) COPYRIGHT Macronix **********************************END OF FILE****/
