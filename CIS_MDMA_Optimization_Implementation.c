/**
 ******************************************************************************
 * @file           : CIS_MDMA_Optimization_Implementation.c
 * @brief          : Implémentation optimisée des copies MDMA CIS
 ******************************************************************************
 * @attention
 *
 * Proposition d'optimisation pour réduire l'utilisation mémoire DTCM
 * en utilisant des copies MDMA sélectives avec block address offset
 *
 ******************************************************************************
 */

#include "main.h"
#include "config.h"
#include "mdma.h"
#include "cis.h"

/* Nouvelles définitions optimisées */
#define CIS_USEFUL_DATA_SIZE    (CIS_400DPI_PIXELS_PER_LANE + CIS_BLACK_LINE)
#define CIS_OPTIMIZED_BUFF_SIZE (CIS_USEFUL_DATA_SIZE * CIS_ADC_OUT_LANES)

/* Variables pour les linked lists MDMA */
static MDMA_LinkNodeTypeDef mdmaNodes[3];  // Un noeud par canal ADC
static MDMA_LinkNodeConfTypeDef nodeConfigs[3];

/* Buffers optimisés (à terme, remplacer les buffers actuels) */
#ifdef USE_OPTIMIZED_BUFFERS
int32_t cisDataCpy_optimized[CIS_OPTIMIZED_BUFF_SIZE * 3] = {0};
#endif

/**
 * @brief  Configuration des transferts MDMA optimisés avec copies sélectives
 * @param  None  
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef cis_configureMDMA_Optimized(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    /* Configuration pour ADC1 -> Canal Rouge */
    nodeConfigs[0].Init = hmdma_mdma_channel1_dma1_stream0_tc_0.Init;
    nodeConfigs[0].Init.SourceBlockAddressOffset = CIS_MAX_LANE_SIZE * sizeof(uint16_t);
    nodeConfigs[0].Init.DestBlockAddressOffset = CIS_USEFUL_DATA_SIZE * sizeof(uint16_t);
    
    nodeConfigs[0].SrcAddress = (uint32_t)&cisData_ADC1[CIS_SP_WIDTH];
    nodeConfigs[0].DstAddress = (uint32_t)&cisDataCpy[0];
    nodeConfigs[0].BlockDataLength = CIS_USEFUL_DATA_SIZE * sizeof(uint16_t);
    nodeConfigs[0].BlockCount = 3; // R, G, B pour ADC1
    
    /* Configuration pour ADC2 -> Canal Vert */  
    nodeConfigs[1].Init = hmdma_mdma_channel2_dma1_stream1_tc_0.Init;
    nodeConfigs[1].Init.SourceBlockAddressOffset = CIS_MAX_LANE_SIZE * sizeof(uint16_t);
    nodeConfigs[1].Init.DestBlockAddressOffset = CIS_USEFUL_DATA_SIZE * sizeof(uint16_t);
    
    nodeConfigs[1].SrcAddress = (uint32_t)&cisData_ADC2[CIS_SP_WIDTH];
    nodeConfigs[1].DstAddress = (uint32_t)&cisDataCpy[CIS_OPTIMIZED_BUFF_SIZE];
    nodeConfigs[1].BlockDataLength = CIS_USEFUL_DATA_SIZE * sizeof(uint16_t);
    nodeConfigs[1].BlockCount = 3; // R, G, B pour ADC2
    
    /* Configuration pour ADC3 -> Canal Bleu */
    nodeConfigs[2].Init = hmdma_mdma_channel3_dma2_stream0_tc_0.Init;
    nodeConfigs[2].Init.SourceBlockAddressOffset = CIS_MAX_LANE_SIZE * sizeof(uint16_t);
    nodeConfigs[2].Init.DestBlockAddressOffset = CIS_USEFUL_DATA_SIZE * sizeof(uint16_t);
    
    nodeConfigs[2].SrcAddress = (uint32_t)&cisData_ADC3[CIS_SP_WIDTH];
    nodeConfigs[2].DstAddress = (uint32_t)&cisDataCpy[CIS_OPTIMIZED_BUFF_SIZE * 2];
    nodeConfigs[2].BlockDataLength = CIS_USEFUL_DATA_SIZE * sizeof(uint16_t);
    nodeConfigs[2].BlockCount = 3; // R, G, B pour ADC3
    
    /* Création des noeuds de linked list */
    for (int i = 0; i < 3; i++)
    {
        status = HAL_MDMA_LinkedList_CreateNode(&mdmaNodes[i], &nodeConfigs[i]);
        if (status != HAL_OK)
        {
            return status;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief  Démarrage des transferts MDMA optimisés
 * @param  None
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef cis_startMDMA_Optimized(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    /* Démarrage des transferts avec linked lists */
    status = HAL_MDMA_Start_IT(&hmdma_mdma_channel1_dma1_stream0_tc_0, 
                               nodeConfigs[0].SrcAddress, 
                               nodeConfigs[0].DstAddress,
                               nodeConfigs[0].BlockDataLength, 
                               nodeConfigs[0].BlockCount);
    if (status != HAL_OK) return status;
    
    status = HAL_MDMA_Start_IT(&hmdma_mdma_channel2_dma1_stream1_tc_0,
                               nodeConfigs[1].SrcAddress,
                               nodeConfigs[1].DstAddress, 
                               nodeConfigs[1].BlockDataLength,
                               nodeConfigs[1].BlockCount);
    if (status != HAL_OK) return status;
    
    status = HAL_MDMA_Start_IT(&hmdma_mdma_channel3_dma2_stream0_tc_0,
                               nodeConfigs[2].SrcAddress,
                               nodeConfigs[2].DstAddress,
                               nodeConfigs[2].BlockDataLength, 
                               nodeConfigs[2].BlockCount);
    
    return status;
}

/**
 * @brief  Fonction d'aide pour calculer les nouveaux offsets optimisés
 * @param  dpi: Résolution (200 ou 400 DPI)
 * @retval None
 */
void cis_calculateOptimizedOffsets(uint16_t dpi)
{
    uint32_t pixels_per_lane = (dpi == 400) ? CIS_400DPI_PIXELS_PER_LANE : CIS_200DPI_PIXELS_PER_LANE;
    uint32_t useful_data_size = pixels_per_lane + CIS_BLACK_LINE;
    uint32_t optimized_buff_size = useful_data_size * CIS_ADC_OUT_LANES;
    
    /* Mise à jour des offsets dans cisConfig */
    cisConfig.red_lane_offset = 0;  // Pas d'offset SP_WIDTH dans les buffers optimisés
    cisConfig.green_lane_offset = optimized_buff_size;
    cisConfig.blue_lane_offset = optimized_buff_size * 2;
    
    printf("Optimized offsets for %d DPI:\n", dpi);
    printf("  useful_data_size: %lu\n", useful_data_size);
    printf("  optimized_buff_size: %lu\n", optimized_buff_size);
    printf("  red_offset: %ld\n", cisConfig.red_lane_offset);
    printf("  green_offset: %ld\n", cisConfig.green_lane_offset);
    printf("  blue_offset: %ld\n", cisConfig.blue_lane_offset);
}

/**
 * @brief  Version modifiée de cis_imageProcess pour buffers optimisés
 * @param  cisDataCpy: Pointeur vers les données optimisées
 * @param  imageBuffers: Pointeur vers les buffers de sortie
 * @retval None
 */
void cis_imageProcess_Optimized(int32_t *cisDataCpy, struct packet_Scanline *imageBuffers)
{
    int32_t lane, i, packet, iteration;
    uint32_t startTick;
    uint32_t numPackets = cisConfig.udp_nb_packet_per_line;
    const int32_t pixelPerPacket = cisConfig.pixels_nb / numPackets;
    const int32_t lanePackets = numPackets / CIS_ADC_OUT_LANES;
    
    /* Les offsets sont maintenant basés sur les buffers optimisés */
    const int32_t red_offset = CIS_BLACK_LINE;    // Skip black line to get pixels
    const int32_t green_offset = CIS_OPTIMIZED_BUFF_SIZE + CIS_BLACK_LINE;
    const int32_t blue_offset = (CIS_OPTIMIZED_BUFF_SIZE * 2) + CIS_BLACK_LINE;

    for (iteration = shared_config.cis_oversampling; iteration-- > 0; )
    {
        int32_t curIter = shared_config.cis_oversampling - iteration;

        /* Attendre que tous les canaux soient prêts */
        startTick = HAL_GetTick();
        for (i = CIS_ADC_OUT_LANES; i-- > 0; )
        {
            while (cisBufferState[i] != CIS_BUFFER_COMPLETE)
            {
                if ((HAL_GetTick() - startTick) > CIS_CAPTURE_TIMEOUT)
                {
                    printf("Timeout: Full buffer state not reached for lane %d\n", (int)i + 1);
                    cis_resetStart();
                    return;
                }
            }
            cisBufferState[i] = CIS_BUFFER_OFFSET_NONE;
        }

        /* Appliquer la calibration */
        cis_applyLinearCalibration(cisDataCpy, 255);

        /* Traitement identique mais avec les nouveaux offsets */
        if (shared_config.cis_handedness)
        {
            for (packet = numPackets - 1; packet >= 0; packet--)
            {
                lane = packet / lanePackets;
                int32_t localPacketIndex = packet - (lane * lanePackets);
                int32_t startIdx = pixelPerPacket * (localPacketIndex + 1) - 1;
                int32_t endIdx = pixelPerPacket * localPacketIndex;

                /* Utilisation des nouveaux offsets optimisés */
                int32_t *redBase = cisDataCpy + red_offset + lane * CIS_USEFUL_DATA_SIZE;
                int32_t *greenBase = cisDataCpy + green_offset + lane * CIS_USEFUL_DATA_SIZE;  
                int32_t *blueBase = cisDataCpy + blue_offset + lane * CIS_USEFUL_DATA_SIZE;

                /* Traitement des pixels identique */
                for (i = startIdx; i >= endIdx; i--)
                {
                    int32_t offsetIndex = i - endIdx;
                    uint8_t sample_R = (uint8_t)redBase[i];
                    uint8_t sample_G = (uint8_t)greenBase[i];
                    uint8_t sample_B = (uint8_t)blueBase[i];

                    if (curIter == 1)
                    {
                        imageBuffers[packet].imageData_R[offsetIndex] = sample_R;
                        imageBuffers[packet].imageData_G[offsetIndex] = sample_G;
                        imageBuffers[packet].imageData_B[offsetIndex] = sample_B;
                    }
                    else
                    {
                        imageBuffers[packet].imageData_R[offsetIndex] += (sample_R - imageBuffers[packet].imageData_R[offsetIndex]) / curIter;
                        imageBuffers[packet].imageData_G[offsetIndex] += (sample_G - imageBuffers[packet].imageData_G[offsetIndex]) / curIter;
                        imageBuffers[packet].imageData_B[offsetIndex] += (sample_B - imageBuffers[packet].imageData_B[offsetIndex]) / curIter;
                    }
                }

                if (curIter == shared_config.cis_oversampling)
                {
                    imageBuffers[packet].fragment_id = packet;
                    imageBuffers[packet].line_id = shared_var.cis_process_cnt;
                }
            }
        }
        else 
        {
            /* Version miroir - code similaire mais inversé */
            /* ... (implémentation similaire pour cis_handedness == 0) */
        }

        /* Démarrer les transferts MDMA optimisés */
        cis_startMDMA_Optimized();
    }
}

/**
 * @brief  Version de démarrage de capture avec MDMA optimisé
 * @param  None
 * @retval None
 */
void cis_startCapture_Optimized(void)
{
    /* Configuration MDMA optimisée */
    if (cis_configureMDMA_Optimized() != HAL_OK)
    {
        Error_Handler();
    }
    
    /* Démarrage de la capture normale */
    cis_startCapture();
    
    /* Remplacer les MDMA par les versions optimisées */
    cis_startMDMA_Optimized();
}

/* Exemple de migration progressive */
#ifdef ENABLE_CIS_OPTIMIZATION
    #define cis_startCapture() cis_startCapture_Optimized()
    #define cis_imageProcess(data, buffers) cis_imageProcess_Optimized(data, buffers)
#endif
