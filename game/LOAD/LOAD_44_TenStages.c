#include <common.h>
#ifdef CTR_NATIVE
#include <platform/native_netplay.h>
#endif

void (*mainMenuInit[6])() = {MM_JumpTo_Title_FirstTime, MM_JumpTo_Characters, MM_JumpTo_TrackSelect,
                             MM_JumpTo_BattleSetup,     CS_Garage_Init,       MM_JumpTo_Scrapbook};

#ifdef CTR_NATIVE
enum
{
        LOAD_NATIVE_NDBOX_INTRO_SONG_SYNC_TIME = 0x11c0,
};

static void LOAD_NativeAudio_SetStateAfterBankReload(u32 state)
{
        int isSameLatchedState = sdata->unkAudioState == (s16)state;
        int isStoppedSong0State = (state == 2) || (state == 5) || (state == 7);

        if ((sdata->cseqBoolPlay == 0) && isSameLatchedState && isStoppedSong0State)
        {
                // NOTE(aalhendi): Native can arrive here after LOAD_TenStages
                // stopped song-0 CSEQ music for a bank reload while the retail
                // audio-state latch still matches. Re-enter the same CSEQ state
                // so post-load menu/hub music is started again.
                Voiceline_EmptyFunc();
                Audio_SetState(state);
                sdata->unkAudioState = (s16)state;
                return;
        }

        Audio_SetState_Safe(state);
}
#endif

#if defined(__LP64__)
#define MAX_NATIVE_MODELS 256
#define MAX_NATIVE_HEADERS 512
#define MAX_NATIVE_ANIM_ARRAYS 512
#define MAX_NATIVE_ANIM_OBJS 1024
#define MAX_ANIMS_PER_HEADER 32

static struct Model s_nativeModelPool[MAX_NATIVE_MODELS];
static struct ModelHeader s_nativeHeaderPool[MAX_NATIVE_HEADERS];
static struct ModelAnim *s_nativeAnimPool[MAX_NATIVE_ANIM_ARRAYS][MAX_ANIMS_PER_HEADER];
static struct ModelAnim s_nativeAnimObjPool[MAX_NATIVE_ANIM_OBJS];
static int s_nativeModelCount = 0;
static int s_nativeHeaderCount = 0;
static int s_nativeAnimCount = 0;
static int s_nativeAnimObjCount = 0;

struct NativeModelCacheEntry {
        const void *raw;
        struct Model *model;
};
static struct NativeModelCacheEntry s_nativeModelCache[MAX_NATIVE_MODELS];
static int s_nativeModelCacheCount = 0;

static void LOAD_NativeModelPool_Reset(void)
{
        s_nativeModelCount = 0;
        s_nativeHeaderCount = 0;
        s_nativeAnimCount = 0;
        s_nativeAnimObjCount = 0;
        s_nativeModelCacheCount = 0;
}

static struct Model *LOAD_NativeModel_Convert(void *rawModelPtr)
{
        if (rawModelPtr == NULL) return NULL;

        for (int i = 0; i < s_nativeModelCacheCount; i++)
        {
                if (s_nativeModelCache[i].raw == rawModelPtr)
                        return s_nativeModelCache[i].model;
        }

        if (s_nativeModelCount >= MAX_NATIVE_MODELS) return NULL;

        const uint32_t *raw32 = (const uint32_t *)rawModelPtr;
        uintptr_t base_hi = (uintptr_t)rawModelPtr & 0xFFFFFFFF00000000ULL;

        struct Model *nm = &s_nativeModelPool[s_nativeModelCount++];
        memset(nm, 0, sizeof(*nm));

        if (s_nativeModelCacheCount < MAX_NATIVE_MODELS)
        {
                s_nativeModelCache[s_nativeModelCacheCount].raw = rawModelPtr;
                s_nativeModelCache[s_nativeModelCacheCount].model = nm;
                s_nativeModelCacheCount++;
        }

        // Name (16 bytes)
        memcpy(nm->name, &raw32[0], 16);

        // ID (s16) & numHeaders (s16)
        uint32_t word4 = raw32[4];
        nm->id = (s16)(word4 & 0xffff);
        nm->numHeaders = (s16)(word4 >> 16);

        // headers (raw32[5] is 32-bit pointer)
        uint32_t headersRel = raw32[5];
        if (headersRel != 0 && nm->numHeaders > 0 && s_nativeHeaderCount + nm->numHeaders <= MAX_NATIVE_HEADERS)
        {
                int headerStartIndex = s_nativeHeaderCount;
                s_nativeHeaderCount += nm->numHeaders;
                nm->headers = &s_nativeHeaderPool[headerStartIndex];

                const uint32_t *rawHdrArray = (const uint32_t *)(base_hi | (uintptr_t)headersRel);

                for (int h = 0; h < nm->numHeaders; h++)
                {
                        const uint32_t *rawHdr = &rawHdrArray[h * 16]; // 16 words = 64 bytes per header
                        struct ModelHeader *nh = &nm->headers[h];
                        memset(nh, 0, sizeof(*nh));

                        memcpy(nh->name, &rawHdr[0], 16);
                        nh->unk1 = (int)rawHdr[4];
                        nh->maxDistanceLOD = (s16)(rawHdr[5] & 0xffff);
                        nh->flags = (u16)(rawHdr[5] >> 16);
                        nh->scale[0] = (s16)(rawHdr[6] & 0xffff);
                        nh->scale[1] = (s16)(rawHdr[6] >> 16);
                        nh->scale[2] = (s16)(rawHdr[7] & 0xffff);
                        nh->scale[3] = (s16)(rawHdr[7] >> 16);
                        nh->ptrCommandList = rawHdr[8] ? (u32 *)(base_hi | rawHdr[8]) : NULL;
                        nh->ptrFrameData = rawHdr[9] ? (struct ModelFrame *)(base_hi | rawHdr[9]) : NULL;
                        nh->ptrTexLayout = rawHdr[10] ? (struct TextureLayout **)(base_hi | rawHdr[10]) : NULL;
                        nh->ptrColors = rawHdr[11] ? (u32 *)(base_hi | rawHdr[11]) : NULL;
#if defined(__LP64__)
                        nh->unk3 = rawHdr[12] ? (u32 *)(base_hi | rawHdr[12]) : NULL;
#else
                        nh->unk3 = rawHdr[12];
#endif
                        nh->numAnimations = rawHdr[13];

                        uint32_t animsRel = rawHdr[14];
                        if (animsRel != 0 && nh->numAnimations > 0 && s_nativeAnimCount < MAX_NATIVE_ANIM_ARRAYS)
                        {
                                struct ModelAnim **animArr = s_nativeAnimPool[s_nativeAnimCount++];
                                const uint32_t *rawAnims = (const uint32_t *)(base_hi | animsRel);
                                uint32_t maxA = (nh->numAnimations < MAX_ANIMS_PER_HEADER) ? nh->numAnimations : MAX_ANIMS_PER_HEADER;
                                for (uint32_t a = 0; a < maxA; a++)
                                {
                                        if (rawAnims[a] != 0 && s_nativeAnimObjCount < MAX_NATIVE_ANIM_OBJS)
                                        {
                                                const uint32_t *rawA = (const uint32_t *)(base_hi | rawAnims[a]);
                                                struct ModelAnim *na = &s_nativeAnimObjPool[s_nativeAnimObjCount++];
                                                memset(na, 0, sizeof(*na));
                                                memcpy(na->name, &rawA[0], 16);
                                                na->numFrames = (u16)(rawA[4] & 0xffff);
                                                na->frameSize = (s16)(rawA[4] >> 16);
                                                uint32_t deltaRel = rawA[5];
                                                na->ptrDeltaArray = deltaRel ? (u32 *)(base_hi | deltaRel) : NULL;
#if defined(__LP64__)
                                                na->firstFrame = (void *)&rawA[6];
#endif
                                                animArr[a] = na;
                                        }
                                        else
                                        {
                                                animArr[a] = NULL;
                                        }
                                }
                                nh->ptrAnimations = animArr;
                        }

                        nh->animtex = rawHdr[15] ? (struct AnimTex *)(base_hi | rawHdr[15]) : NULL;
                }
        }

        return nm;
}

static struct Level s_nativeLevel[2];
static struct mesh_info s_nativeMeshInfo[2];
static struct VisMem s_nativeVisMem[2];
static struct Model *s_nativeLevModels[2][128];
static struct InstDef s_nativeInstDefs[2][256];
static struct InstDef *s_nativeInstDefPtrArray[2][256];
static struct SpawnType1 s_nativeSpawnType1[2];
static struct SpawnType2 s_nativeSpawnType2[2][32];
static struct SpawnType2 s_nativeSpawnType2_PosRot[2][32];
static struct NavHeader s_nativeNavHeader[2][4];
static struct NavHeader *s_nativeNavTable[2][4];

struct Level *LOAD_NativeLevel_Build(void *rawFile, int slot)
{
        if (rawFile == NULL) return NULL;
        int s;
        if (slot < 0)
        {
                s = (sdata->gGT && sdata->gGT->level1 == &s_nativeLevel[0]) ? 1 : 0;
        }
        else
        {
                s = (slot != 0) ? 1 : 0;
        }

        const uint32_t *raw32 = (const uint32_t *)rawFile;
        uintptr_t base_hi = (uintptr_t)rawFile & 0xFFFFFFFF00000000ULL;

        #define LEV_PTR64(x32) ((x32) ? (void *)(base_hi | (uintptr_t)(uint32_t)(x32)) : NULL)

        memset(&s_nativeLevel[s], 0, sizeof(s_nativeLevel[s]));

        // 0x00: ptr_mesh_info
        uint32_t pMesh32 = raw32[0];
        if (pMesh32 != 0)
        {
                const uint32_t *meshRaw = (const uint32_t *)LEV_PTR64(pMesh32);
                memset(&s_nativeMeshInfo[s], 0, sizeof(s_nativeMeshInfo[s]));
                s_nativeMeshInfo[s].numQuadBlock = (int)meshRaw[0];
                s_nativeMeshInfo[s].numVertex = (int)meshRaw[1];
                s_nativeMeshInfo[s].unk1 = (int)meshRaw[2];
                s_nativeMeshInfo[s].ptrQuadBlockArray = (struct QuadBlock *)LEV_PTR64(meshRaw[3]);
                s_nativeMeshInfo[s].ptrVertexArray = (struct LevVertex *)LEV_PTR64(meshRaw[4]);
                s_nativeMeshInfo[s].unk2 = (int)meshRaw[5];
                s_nativeMeshInfo[s].bspRoot = (struct BSP *)LEV_PTR64(meshRaw[6]);
                s_nativeMeshInfo[s].numBspNodes = (int)meshRaw[7];

                s_nativeLevel[s].ptr_mesh_info = &s_nativeMeshInfo[s];
        }

        // 0x04: ptr_skybox
        s_nativeLevel[s].ptr_skybox = (struct Skybox *)LEV_PTR64(raw32[1]);

        // 0x08: ptr_anim_tex
        s_nativeLevel[s].ptr_anim_tex = (struct AnimTex *)LEV_PTR64(raw32[2]);

        // 0x0C: numInstances
        uint32_t numInst = raw32[3];
        s_nativeLevel[s].numInstances = numInst;

        // 0x10: ptrInstDefs
        uint32_t pInstDefs32 = raw32[4];
        if (pInstDefs32 != 0)
        {
                const uint32_t *instRaw = (const uint32_t *)LEV_PTR64(pInstDefs32);
                uint32_t maxInst = (numInst < 256) ? numInst : 256;
                for (uint32_t i = 0; i < maxInst; i++)
                {
                        const uint32_t *oneRaw = &instRaw[i * 16];
                        memcpy(s_nativeInstDefs[s][i].name, &oneRaw[0], 16);
                        s_nativeInstDefs[s][i].model = LOAD_NativeModel_Convert(LEV_PTR64(oneRaw[4]));
                        s_nativeInstDefs[s][i].scale[0] = (s16)(oneRaw[5] & 0xffff);
                        s_nativeInstDefs[s][i].scale[1] = (s16)(oneRaw[5] >> 16);
                        s_nativeInstDefs[s][i].scale[2] = (s16)(oneRaw[6] & 0xffff);
                        s_nativeInstDefs[s][i].scale[3] = (s16)(oneRaw[6] >> 16);
                        s_nativeInstDefs[s][i].colorRGBA = oneRaw[7];
                        s_nativeInstDefs[s][i].flags = oneRaw[8];
                        s_nativeInstDefs[s][i].unk24 = (int)oneRaw[9];
                        s_nativeInstDefs[s][i].unk28 = (int)oneRaw[10];
                        s_nativeInstDefs[s][i].ptrInstance = NULL;
                        s_nativeInstDefs[s][i].pos[0] = (s16)(oneRaw[12] & 0xffff);
                        s_nativeInstDefs[s][i].pos[1] = (s16)(oneRaw[12] >> 16);
                        s_nativeInstDefs[s][i].pos[2] = (s16)(oneRaw[13] & 0xffff);
                        s_nativeInstDefs[s][i].rot[0] = (s16)(oneRaw[13] >> 16);
                        s_nativeInstDefs[s][i].rot[1] = (s16)(oneRaw[14] & 0xffff);
                        s_nativeInstDefs[s][i].rot[2] = (s16)(oneRaw[14] >> 16);
                        s_nativeInstDefs[s][i].modelID = (int)oneRaw[15];
                }
                s_nativeLevel[s].ptrInstDefs = s_nativeInstDefs[s];
        }

        // 0x14: numModels
        uint32_t numModels = raw32[5];
        s_nativeLevel[s].numModels = numModels;

        // 0x18: ptrModelsPtrArray
        uint32_t pModelArr32 = raw32[6];
        if (pModelArr32 != 0)
        {
                const uint32_t *mPtrs32 = (const uint32_t *)LEV_PTR64(pModelArr32);
                uint32_t count = (numModels < 127) ? numModels : 127;
                for (uint32_t i = 0; i < count; i++)
                {
                        s_nativeLevModels[s][i] = LOAD_NativeModel_Convert(LEV_PTR64(mPtrs32[i]));
                }
                s_nativeLevModels[s][count] = NULL;
                s_nativeLevel[s].ptrModelsPtrArray = s_nativeLevModels[s];
        }

        // 0x1C: unk3
        s_nativeLevel[s].unk3 = LEV_PTR64(raw32[7]);

        // 0x20: unk4
        s_nativeLevel[s].unk4 = LEV_PTR64(raw32[8]);

        // 0x24: ptrInstDefPtrArray
        uint32_t pInstDefPtrArr32 = raw32[9];
        if (pInstDefPtrArr32 != 0 && pInstDefs32 != 0)
        {
                const uint32_t *idPtrs32 = (const uint32_t *)LEV_PTR64(pInstDefPtrArr32);
                uintptr_t rawBase = (uintptr_t)LEV_PTR64(pInstDefs32);
                uint32_t pIdx = 0;
                while (idPtrs32[pIdx] != 0 && pIdx < 255)
                {
                        uintptr_t targetAddr = (uintptr_t)LEV_PTR64(idPtrs32[pIdx]);
                        if (targetAddr >= rawBase && targetAddr < rawBase + numInst * 64)
                        {
                                uint32_t instIdx = (uint32_t)((targetAddr - rawBase) / 64);
                                s_nativeInstDefPtrArray[s][pIdx] = &s_nativeInstDefs[s][instIdx];
                        }
                        else
                        {
                                s_nativeInstDefPtrArray[s][pIdx] = NULL;
                        }
                        pIdx++;
                }
                s_nativeInstDefPtrArray[s][pIdx] = NULL;
                s_nativeLevel[s].ptrInstDefPtrArray = s_nativeInstDefPtrArray[s];
        }

        // 0x28: unk5
        s_nativeLevel[s].unk5 = LEV_PTR64(raw32[10]);

        // 0x2C: null1
        s_nativeLevel[s].null1 = LEV_PTR64(raw32[11]);

        // 0x30: null2
        s_nativeLevel[s].null2 = LEV_PTR64(raw32[12]);

        // 0x34: numWaterVertices
        s_nativeLevel[s].numWaterVertices = (int)raw32[13];

        // 0x38: ptr_water
        s_nativeLevel[s].ptr_water = (struct WaterVert *)LEV_PTR64(raw32[14]);

        // 0x3C: levTexLookup
        s_nativeLevel[s].levTexLookup = (struct LevTexLookup *)LEV_PTR64(raw32[15]);

        // 0x40: ptr_named_tex_array
        s_nativeLevel[s].ptr_named_tex_array = (struct Icon *)LEV_PTR64(raw32[16]);

        // 0x44: ptr_tex_waterEnvMap
        s_nativeLevel[s].ptr_tex_waterEnvMap = LEV_PTR64(raw32[17]);

        // 0x48: glowGradient[3] (24 bytes)
        memcpy(s_nativeLevel[s].glowGradient, &raw32[18], sizeof(s_nativeLevel[s].glowGradient));

        // 0x6C: DriverSpawn[8] (96 bytes = 24 words)
        memcpy(s_nativeLevel[s].DriverSpawn, &raw32[27], sizeof(s_nativeLevel[s].DriverSpawn));

        // 0xCC: unk_Lev_CC
        s_nativeLevel[s].unk_Lev_CC = LEV_PTR64(raw32[51]);

        // 0xD0: unk_Lev_D0
        s_nativeLevel[s].unk_Lev_D0 = LEV_PTR64(raw32[52]);

        // 0xD4: ptrLowTexArray
        s_nativeLevel[s].ptrLowTexArray = LEV_PTR64(raw32[53]);

        // 0xD8: clearColorRGBA
        s_nativeLevel[s].clearColorRGBA = raw32[54];

        // 0xDC: configFlags
        s_nativeLevel[s].configFlags = raw32[55];

        // 0xE0: build_start
        s_nativeLevel[s].build_start = (char *)LEV_PTR64(raw32[56]);

        // 0xE4: build_end
        s_nativeLevel[s].build_end = (char *)LEV_PTR64(raw32[57]);

        // 0xE8: build_type
        s_nativeLevel[s].build_type = (char *)LEV_PTR64(raw32[58]);

        // 0xEC: unk_EC (24 bytes = 6 words)
        memcpy(s_nativeLevel[s].unk_EC, &raw32[59], sizeof(s_nativeLevel[s].unk_EC));

        // 0x104: rainBuffer (48 bytes = 12 words)
        memcpy(&s_nativeLevel[s].rainBuffer, &raw32[65], sizeof(s_nativeLevel[s].rainBuffer));

        // 0x134: ptrSpawnType1
        uint32_t pSpawnType1_32 = raw32[77];
        if (pSpawnType1_32 != 0)
        {
                const uint32_t *st1Raw = (const uint32_t *)LEV_PTR64(pSpawnType1_32);
                int count = (int)st1Raw[0];
                memset(&s_nativeSpawnType1[s], 0, sizeof(s_nativeSpawnType1[s]));
                s_nativeSpawnType1[s].count = count;
                uint32_t maxCount = (count < 16) ? count : 16;
                for (uint32_t i = 0; i < maxCount; i++)
                {
                        s_nativeSpawnType1[s].pointers[i] = st1Raw[1 + i] ? (void *)(base_hi | st1Raw[1 + i]) : NULL;
                }
                s_nativeLevel[s].ptrSpawnType1 = &s_nativeSpawnType1[s];
        }

        // 0x138: numSpawnType2
        int numST2 = (int)raw32[78];
        s_nativeLevel[s].numSpawnType2 = numST2;

        // 0x13C: ptrSpawnType2
        uint32_t pST2_32 = raw32[79];
        if (pST2_32 != 0 && numST2 > 0)
        {
                const uint32_t *st2Raw = (const uint32_t *)LEV_PTR64(pST2_32);
                memset(s_nativeSpawnType2[s], 0, sizeof(s_nativeSpawnType2[s]));
                int count = (numST2 < 32) ? numST2 : 32;
                for (int i = 0; i < count; i++)
                {
                        s_nativeSpawnType2[s][i].numCoords = (int)st2Raw[i * 2 + 0];
                        s_nativeSpawnType2[s][i].posCoords = (s16 *)LEV_PTR64(st2Raw[i * 2 + 1]);
                }
                s_nativeLevel[s].ptrSpawnType2 = s_nativeSpawnType2[s];
        }
        else
        {
                s_nativeLevel[s].ptrSpawnType2 = NULL;
        }

        // 0x140: numSpawnType2_PosRot
        int numST2PR = (int)raw32[80];
        s_nativeLevel[s].numSpawnType2_PosRot = numST2PR;

        // 0x144: ptrSpawnType2_PosRot
        uint32_t pST2PR_32 = raw32[81];
        if (pST2PR_32 != 0 && numST2PR > 0)
        {
                const uint32_t *st2Raw = (const uint32_t *)LEV_PTR64(pST2PR_32);
                memset(s_nativeSpawnType2_PosRot[s], 0, sizeof(s_nativeSpawnType2_PosRot[s]));
                int count = (numST2PR < 32) ? numST2PR : 32;
                for (int i = 0; i < count; i++)
                {
                        s_nativeSpawnType2_PosRot[s][i].numCoords = (int)st2Raw[i * 2 + 0];
                        s_nativeSpawnType2_PosRot[s][i].posCoords = (s16 *)LEV_PTR64(st2Raw[i * 2 + 1]);
                }
                s_nativeLevel[s].ptrSpawnType2_PosRot = s_nativeSpawnType2_PosRot[s];
        }
        else
        {
                s_nativeLevel[s].ptrSpawnType2_PosRot = NULL;
        }

        // 0x148: cnt_restart_points
        s_nativeLevel[s].cnt_restart_points = (int)raw32[82];

        // 0x14C: ptr_restart_points
        s_nativeLevel[s].ptr_restart_points = (struct CheckpointNode *)LEV_PTR64(raw32[83]);

        // 0x150: unk_150 (16 bytes = 4 words)
        memcpy(s_nativeLevel[s].unk_150, &raw32[84], sizeof(s_nativeLevel[s].unk_150));

        // 0x160: clearColor[3] (12 bytes = 3 words)
        memcpy(s_nativeLevel[s].clearColor, &raw32[88], sizeof(s_nativeLevel[s].clearColor));

        // 0x16C: unk_16C
        s_nativeLevel[s].unk_16C = (int)raw32[91];

        // 0x170: unk_170
        s_nativeLevel[s].unk_170 = (void *)LEV_PTR64(raw32[92]);

        // 0x174: numSCVert
        s_nativeLevel[s].numSCVert = (int)raw32[93];

        // 0x178: ptrSCVert
        s_nativeLevel[s].ptrSCVert = LEV_PTR64(raw32[94]);

        // 0x17C: stars (6 bytes)
        memcpy(&s_nativeLevel[s].stars, (const char *)rawFile + 0x17c, sizeof(s_nativeLevel[s].stars));

        // 0x184: splitLines[2] (4 bytes)
        memcpy(s_nativeLevel[s].splitLines, (const char *)rawFile + 0x184, sizeof(s_nativeLevel[s].splitLines));

        // 0x188: LevNavTable
        uint32_t pNavTable32 = raw32[98];
        if (pNavTable32 != 0)
        {
                const uint32_t *rawNavTable = (const uint32_t *)LEV_PTR64(pNavTable32);
                memset(s_nativeNavHeader[s], 0, sizeof(s_nativeNavHeader[s]));
                memset(s_nativeNavTable[s], 0, sizeof(s_nativeNavTable[s]));
                for (int i = 0; i < 3; i++)
                {
                        if (rawNavTable[i] != 0)
                        {
                                const char *rawHdr = (const char *)LEV_PTR64(rawNavTable[i]);
                                s_nativeNavHeader[s][i].magicNumber = *(const s16 *)(rawHdr + 0);
                                s_nativeNavHeader[s][i].numPoints = *(const s16 *)(rawHdr + 2);
                                s_nativeNavHeader[s][i].posY_firstNode = *(const int *)(rawHdr + 4);
                                s_nativeNavHeader[s][i].last = NULL;
                                memcpy(s_nativeNavHeader[s][i].rampPhys1, rawHdr + 0x0c, sizeof(s_nativeNavHeader[s][i].rampPhys1));
                                memcpy(s_nativeNavHeader[s][i].rampPhys2, rawHdr + 0x2c, sizeof(s_nativeNavHeader[s][i].rampPhys2));
#if defined(__LP64__)
                                s_nativeNavHeader[s][i].frame = (struct NavFrame *)(rawHdr + 0x4c);
#endif
                                s_nativeNavTable[s][i] = &s_nativeNavHeader[s][i];
                        }
                }
                s_nativeLevel[s].LevNavTable = s_nativeNavTable[s];
        }
        else
        {
                s_nativeLevel[s].LevNavTable = NULL;
        }

        // 0x18C: unk_18C
        s_nativeLevel[s].unk_18C = (int)raw32[99];

        // 0x190: visMem
        uint32_t pVisMem32 = raw32[100];
        if (pVisMem32 != 0)
        {
                const uint32_t *vm32 = (const uint32_t *)LEV_PTR64(pVisMem32);
                memset(&s_nativeVisMem[s], 0, sizeof(s_nativeVisMem[s]));
                for (int p = 0; p < 4; p++)
                {
                        s_nativeVisMem[s].visLeafList[p] = (int *)LEV_PTR64(vm32[p]);
                        s_nativeVisMem[s].visFaceList[p] = (int *)LEV_PTR64(vm32[4 + p]);
                        s_nativeVisMem[s].visOVertList[p] = (int *)LEV_PTR64(vm32[8 + p]);
                        s_nativeVisMem[s].visSCVertList[p] = LEV_PTR64(vm32[12 + p]);
                        s_nativeVisMem[s].visLeafSrc[p] = LEV_PTR64(vm32[16 + p]);
                        s_nativeVisMem[s].visFaceSrc[p] = LEV_PTR64(vm32[20 + p]);
                        s_nativeVisMem[s].visOVertSrc[p] = LEV_PTR64(vm32[24 + p]);
                        s_nativeVisMem[s].visSCVertSrc[p] = LEV_PTR64(vm32[28 + p]);
                        s_nativeVisMem[s].bspList[p] = (struct VisMemBspListNode *)LEV_PTR64(vm32[32 + p]);
                }
                s_nativeLevel[s].visMem = &s_nativeVisMem[s];
        }

        #undef LEV_PTR64
        return &s_nativeLevel[s];
}
#endif

int LOAD_TenStages(struct GameTracker *gGT, int loadingStage, struct BigHeader *bigfile)
{
        s16 sVar4;
        int iVar5;
        u32 uVar6;
        int iVar9;
        int iVar12;
        u32 uVar16;
        int levelID;
        int ovrRegion1;
        int ovrRegion3;

        // if game is loading
        if (sdata->load_inProgress != 0)
        {
                return loadingStage;
        }

        levelID = gGT->levelID;

        // Used in stage 0, 4, 5, 6
        int boolPlayMusicDuringLoading = (levelID == ADVENTURE_GARAGE) || (levelID == NAUGHTY_DOG_CRATE);

        switch (loadingStage)
        {
        case 0:
        {
                // NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003368c-0x80033698 for loading-start volume backup/XA pause.
                if (!boolPlayMusicDuringLoading)
                {
                        Cutscene_VolumeBackup();
                }
                CDSYS_XAPauseRequest();

                // if first boot (SCEA + Copyright + ND Box)
                if (sdata->boolFirstBoot != 0)
                {
                        int vramSize;

                        sdata->boolFirstBoot = 0;

                        // Load Intro TIM for Copyright Page from VRAM file
                        LOAD_VramFile(bigfile, 0x1fe, NULL, &vramSize, -1);
                        MainInit_VRAMDisplay();

#ifdef CTR_NATIVE
                        // NOTE(aalhendi): SCEA is already held by XA playback in MainMain. The copyright
                        // TIM has no XA, so keep it visible until the intro CSEQ reaches
                        // the point retail normally reaches while loading the ND crate.
                        // Present every wait tick so both host swapchain images are
                        // overwritten with copyright instead of briefly revealing SCEA.
                        while (((sdata->songPool[0].flags & 3) == 1) &&
                               (sdata->songPool[0].timeSpentPlaying < LOAD_NATIVE_NDBOX_INTRO_SONG_SYNC_TIME))
                        {
                                VSync(0);
                                Platform_PresentVRAMDisplay();
                        }
#endif

                        gGT->db[0].drawEnv.isbg = 0;
                        gGT->db[1].drawEnv.isbg = 0;
                }

                // if not first boot (dont do this for ND Box)
                else
                {
                        // change active allocation system to #1
                        // used for whole game (except adventure arena)
                        MEMPACK_SwapPacks(0);

                        sdata->levelID = MainInit_StringToLevID(gGT->levelName);

                        // erase all memory loaded after first boot
                        MEMPACK_PopToState(sdata->bookmarkID);
                }

                gGT->level1 = 0;
                gGT->level2 = 0;
                gGT->numPlyrCurrGame = gGT->numPlyrNextGame;
                strcpy(gGT->levelName, data.metaDataLEV[levelID].name_Debug);

                // pop back here for every load, after first load,
                // this permanently reserves LNG, bigfile header, etc
                sdata->bookmarkID = MEMPACK_PushState();

                // Reset HUD
                gGT->hudFlags &= ~(1 | 8);

                // disable all rendering except loading screen
                // no overlay transition (advhub),  use normal spawn
                gGT->renderFlags &= 0x1000;
                gGT->overlayTransition = 0;
                gGT->Debug_ToggleNormalSpawn = 1;
                gGT->visMem1 = 0;
                gGT->visMem2 = 0;

                // Required for Scrapbook "Press Start",
                // may also be required for other edge-cases
                DrawSync(0);

                // ========== Start of flags ===============


                // disable certain game mode flags
                gGT->gameMode1 &= ~(GAME_CUTSCENE | END_OF_RACE | ADVENTURE_ARENA | MAIN_MENU);
                gGT->gameMode2 &= ~(LEV_SWAP | CREDITS | NO_LEV_INSTANCE);

                if ((strncmp(gGT->levelName, sdata->s_ndi, 3) == 0) || (strncmp(gGT->levelName, sdata->s_ending, 6) == 0))
                {
                        gGT->gameMode1 |= GAME_CUTSCENE;
                }
                else if (strncmp(gGT->levelName, sdata->s_intro, 5) == 0)
                {
                        gGT->gameMode1 |= GAME_CUTSCENE;
                        gGT->gameMode2 |= LEV_SWAP;
                }
                else if ((strncmp(gGT->levelName, sdata->s_screen, 6) == 0) || (strncmp(gGT->levelName, sdata->s_garage, 6) == 0))
                {
                        gGT->gameMode1 |= MAIN_MENU;
                        gGT->numPlyrNextGame = gGT->numPlyrCurrGame;
                        gGT->numPlyrCurrGame = 4;

                        if (strncmp(gGT->levelName, sdata->s_garage, 6) == 0)
                        {
                                gGT->numPlyrCurrGame = 1;
                                sdata->mainMenuState = 4;
                        }
                }
		else if (strncmp(gGT->levelName, sdata->s_hub, 3) == 0)
		{
			gGT->numPlyrCurrGame = 1;
#ifdef CTR_NATIVE
			if ((gGT->gameMode2 & COOPERATIVE_ADVENTURE) != 0)
				gGT->numPlyrNextGame = 2;
			else
				gGT->numPlyrNextGame = 1;
#else
			gGT->numPlyrNextGame = 1;
#endif
			gGT->gameMode1 |= ADVENTURE_ARENA;
			gGT->gameMode2 |= LEV_SWAP;
		}
                else if (strncmp(gGT->levelName, sdata->s_credit, 6) == 0)
                {
                        gGT->numPlyrNextGame = 1;
                        gGT->numPlyrCurrGame = 1;
                        gGT->gameMode1 |= GAME_CUTSCENE;
                        gGT->gameMode2 |= (LEV_SWAP | CREDITS);
                }


                // ========== End of setting numPlyr ================
                // ========== Set LevelLOD variables ================


                // default
                sdata->levelLOD = gGT->numPlyrCurrGame;

                // main menu or adv garage
                if ((gGT->gameMode1 & MAIN_MENU) != 0)
                {
                        sdata->levelLOD = 1;
                }

                // if relic, or time trial
                if ((gGT->gameMode1 & (TIME_TRIAL | RELIC_RACE)) != 0)
                {
                        sdata->levelLOD = 8;
                }

#ifdef CTR_NATIVE
                /* Safety: reset stale g_NetplayRacing if no netplay session.
                 * Prevents the netplay model-loading path from breaking
                 * single-player Arcade/Cup after ending a netplay race. */
                if (g_NetplayRacing && Netplay_GetState() == NETPLAY_LEGACY_DISCONNECTED)
                {
                        g_NetplayRacing = 0;
                }

                /* Netplay: each instance is single-player locally even though
                 * numPlyrCurrGame is 2-8 to represent connected peers. We must
                 * load the 1P MPK (high-poly kart models) — NOT the 2P/3P/4P
                 * MPK, which has a different structure that crashes
                 * LOAD_GlobalModelPtrs_MPK when numPlyrCurrGame != 1.
                 * This is also why the kart was invisible: the 2P MPK has
                 * InstDrawPerPlayer slots for 2 viewports, but rendering
                 * only iterates 1 viewport in netplay, so the model was
                 * never drawn. */
                if (g_NetplayRacing)
                {
                        sdata->levelLOD = 1;
                }
#endif

                gGT->hudFlags |= 2;


                // ========== End of LevelLOD ================
                // ========== Alloc Prim + OT ================


                // OG game
                MainInit_PrimMem(gGT);
                MainInit_OTMem(gGT);

                if (((gGT->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) != 0) || ((gGT->gameMode2 & CREDITS) != 0))
                {
                        MainInit_JitPoolsNew(gGT);
                        return loadingStage + 1;
                }

                break;
        }
        case 1:
        {
                // if XA has not paused since CDSYS_XAPauseRequest in stage #0,
                // then quit the function and try again next frame
                if (sdata->XA_State == 4)
                        return loadingStage;

                // NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033b38-0x80033c00 for end-event overlay selection.
                if ((gGT->gameMode1 & CRYSTAL_CHALLENGE) != 0)
                        ovrRegion1 = 0;

                else if ((gGT->gameMode1 & TIME_TRIAL) != 0)
                        ovrRegion1 = 3;

                else if ((gGT->gameMode1 & ARCADE_MODE) != 0)
                        ovrRegion1 = 1;

                else if ((gGT->gameMode1 & RELIC_RACE) != 0)
                        ovrRegion1 = 2;

                else if ((gGT->gameMode1 & ADVENTURE_MODE) != 0)
                        ovrRegion1 = 1;

                else
                {
                        ovrRegion1 = 4;

                        if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
                                break;
                }

                LOAD_OvrEndRace(ovrRegion1);
                break;
        }
        case 2:
        {
                // NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033be8-0x80033c00 for LOD overlay selection.
                LOAD_OvrLOD(gGT->numPlyrCurrGame);
                break;
        }
        case 3:
        {
                // NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033c0c-0x80033cd4 for thread overlay routing.
                if ((levelID != ADVENTURE_GARAGE) && ((gGT->gameMode1 & MAIN_MENU) != 0))
                {
                        ovrRegion3 = 0;
                }
                else if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
                {
                        ovrRegion3 = 3;

                        if (gGT->podiumRewardID == NOFUNC)
                                ovrRegion3 = 2;
                }
                else if ((gGT->podiumRewardID != NOFUNC) || ((gGT->gameMode1 & GAME_CUTSCENE) != 0) || ((gGT->gameMode2 & CREDITS) != 0) ||
                         (levelID == ADVENTURE_GARAGE))
                {
                        ovrRegion3 = 3;
                }
                else
                {
                        ovrRegion3 = 1;

                        if (gGT->overlayIndex_Threads == 1)
                                break;
                }

                LOAD_OvrThreads(ovrRegion3);
                break;
        }
        case 4:
        {
                // NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033cf4-0x80033d04 for post-overlay music restart gate.
                if (!boolPlayMusicDuringLoading)
                {
                        Music_Restart();
                }

                // If in main menu (character selection, track selection, any part of it)
                if ((gGT->gameMode1 & MAIN_MENU) != 0)
                {
                        if ((u32)sdata->mainMenuState < len(mainMenuInit))
                                mainMenuInit[sdata->mainMenuState]();
                }

                // Needed, or else Post-Boss Outro
                // will break the character animations
                sdata->ptrMPK = 0;

                // Clear driver extras, and podium models
                uintptr_t *ptrArray = &data.driverModelExtras[0];
                for (int i = 0; i < 11; i++)
                {
                        ptrArray[i] = 0;
                }

                // NOTE(aalhendi): Retail gates stage advancement until the driver MPK callback sets ptrMPK.
                sdata->load_inProgress = 1;
                LOAD_DriverMPK(bigfile, sdata->levelLOD, LOAD_Callback_DriverModels);
                break;
        }
        case 5:
        {
                // clear and reset
                LibraryOfModels_Clear(gGT);

#if defined(__LP64__)
                LOAD_NativeModelPool_Reset();
#endif

                static struct Model *s_nativePlyrModels[128];
                if (sdata->ptrMPK != 0)
                {
                        const uint32_t *raw32 = (const uint32_t *)((uintptr_t)sdata->ptrMPK + 4);
                        uintptr_t base_hi = (uintptr_t)sdata->ptrMPK & 0xFFFFFFFF00000000ULL;
                        int count = 0;
                        while (*raw32 != 0 && count < 127)
                        {
                                uintptr_t model_addr = base_hi | (uintptr_t)(*raw32);
#if defined(__LP64__)
                                s_nativePlyrModels[count++] = LOAD_NativeModel_Convert((void *)model_addr);
#else
                                s_nativePlyrModels[count++] = (struct Model *)model_addr;
#endif
                                raw32++;
                        }
                        s_nativePlyrModels[count] = NULL;
                        sdata->PLYROBJECTLIST = (int **)s_nativePlyrModels;
                }
                else
                {
                        sdata->PLYROBJECTLIST = 0;
                }

                for (int i = 0; i < 3; i++)
                {
                        if (data.driverModelExtras[i] != 0)
                        {
                                data.driverModelExtras[i] += 4;
#if defined(__LP64__)
                                data.driverModelExtras[i] = (uintptr_t)LOAD_NativeModel_Convert((void *)data.driverModelExtras[i]);
#endif
                        }
                }

                LOAD_GlobalModelPtrs_MPK();
                DecalGlobal_Clear(gGT);

                gGT->mpkIcons = 0;
                if (sdata->ptrMPK != 0)
                {
                        uintptr_t base_hi = (uintptr_t)sdata->ptrMPK & 0xFFFFFFFF00000000ULL;
                        gGT->mpkIcons = *(u32 *)sdata->ptrMPK;

                        if (gGT->mpkIcons != 0)
                                DecalGlobal_Store(gGT, (struct LevTexLookup *)(base_hi | (uintptr_t)gGT->mpkIcons));
                }

                if (!boolPlayMusicDuringLoading)
                {
                        // NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033eb8-0x80033ed0 for music stop/CSEQ stop/bank reload.
                        Music_Stop();
                        CseqMusic_StopAll();
                        Music_LoadBanks();
                }

                break;
        }
        case 6:
        {
                if (!boolPlayMusicDuringLoading)
                {
                        // NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033ef8-0x80033f14 for async bank parse and volume restore.
                        iVar9 = Music_AsyncParseBanks();

                        if (iVar9 == 0)
                        {
                                // quit and restart stage 6 next frame
                                return loadingStage;
                        }

                        Cutscene_VolumeRestore();
                }

                // NOTE(aalhendi): driver model DRAM file headers were already advanced and converted in stage 5 before LOAD_GlobalModelPtrs_MPK.

                // == banks are done parsing ===

                // If this world is made of multiple LEVs
                if ((gGT->gameMode2 & LEV_SWAP) != 0)
                {
                        // Cutscene Packs
                        iVar9 = 0x6b000;
                        iVar12 = 0x40000;

                        // If you're in Adventure Arena
                        if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
                        {
                                // Adv Arena Packs
                                iVar9 = 0x68800;
                                iVar12 = 0x68800;
                        }

                        // Allocate room for LEV swapping
                        void *hubAlloc = MEMPACK_AllocMem(iVar9 + iVar12); // "HUB ALLOC"
                        sdata->ptrHubAlloc = hubAlloc;

                        // Change active allocation system to #2
                        // pack = [hubAlloc, hubAlloc+size1]
                        MEMPACK_SwapPacks(1);
                        MEMPACK_NewPack(hubAlloc, iVar9);

                        // Change active allocation system to #3
                        // pack = [hubAlloc+size1, hubAlloc+size1+size2]
                        MEMPACK_SwapPacks(2);
                        MEMPACK_NewPack((void *)((char *)hubAlloc + iVar9), iVar12);

                        // Intro cutscene with oxide spaceship and all racers
                        if ((gGT->gameMode1 & ADVENTURE_ARENA) == 0)
                        {
                                // Always start with pool 1
                                sVar4 = 1;
                        }

                        // If you're in Adventure Arena
                        else
                        {
                                // Get 1 or 2, depending on map
                                sVar4 = LOAD_GetAdvPackIndex();

                                // Then swap:
                                // Turn 1 into 2
                                // Turn 2 into 1
                                sVar4 = 3 - sVar4;
                        }

                        // keep track of subpack levels
                        gGT->activeMempackIndex = sVar4;
                        gGT->levID_in_each_mempack[sVar4] = gGT->levelID;
                        gGT->levID_in_each_mempack[3 - sVar4] = 0xffff;

                        // the rest of memory will load pointer maps,
                        // loaded at HighMem in main pack, end of RAM,
                        // so the pointer maps dont bloat subpacks
                        MEMPACK_SwapPacks(0);

                        sdata->PatchMem_Size = MEMPACK_GetFreeBytes();
                        sdata->PatchMem_Ptr = MEMPACK_AllocHighMem(sdata->PatchMem_Size); //, "Patch Table Memory");

                        // For Oxide-Intro and Credits, set active pack
                        MEMPACK_SwapPacks(gGT->activeMempackIndex);
                }

                // NOTE(aalhendi): Retail sets the load gate before queueing level files.
                sdata->load_inProgress = 1;

                // add VRAM to loading queue
                LOAD_AppendQueue(0, LT_VRAM, LOAD_GetBigfileIndex(gGT->levelID, sdata->levelLOD, LVI_VRAM), NULL, NULL);

                // add LEV to loading queue
                LOAD_AppendQueue(0, LT_GETADDR, LOAD_GetBigfileIndex(gGT->levelID, sdata->levelLOD, LVI_LEV), NULL, LOAD_Callback_LEV);

                // NOTE(aalhendi): ASM-verified NTSC-U 926 0x800340c0-0x80034180; retail queues PTR maps by level-ID ranges.
                if (((u32)(levelID - GEM_STONE_VALLEY) < 0xe) || ((u32)(levelID - CREDITS_CRASH) < 0x14))
                {
                        // add PTR file to loading queue
                        LOAD_AppendQueue(0, LT_SETADDR, LOAD_GetBigfileIndex(gGT->levelID, sdata->levelLOD, LVI_PTR), (void *)sdata->PatchMem_Ptr, LOAD_Callback_PatchMem);
                }
                break;
        }
        case 7:
        {
                // get level pointer
#if defined(__LP64__)
                struct Level *lev = LOAD_NativeLevel_Build(sdata->ptrLevelFile, 0);
#else
                struct Level *lev = sdata->ptrLevelFile;
#endif

                gGT->level1 = lev;
                gGT->visMem1 = lev ? lev->visMem : NULL;

                if (lev != 0)
                {
                        DecalGlobal_Store(gGT, lev->levTexLookup);
                }

                DebugFont_Init(gGT);

                // if level is not nullptr
                if (lev != 0)
                {
                        LibraryOfModels_Store(gGT, lev->numModels, lev->ptrModelsPtrArray);

                        gGT->ptrCircle = (u32)(uintptr_t)DecalGlobal_FindInLEV(lev, rdata.s_circle);
                        gGT->ptrClod = (u32)(uintptr_t)DecalGlobal_FindInLEV(lev, rdata.s_clod);
                        gGT->ptrDustpuff = (u32)(uintptr_t)DecalGlobal_FindInLEV(lev, rdata.s_dustpuff);
                        gGT->ptrSmoking = (u32)(uintptr_t)DecalGlobal_FindInLEV(lev, rdata.s_smokering); // "Smoke Ring"
                        gGT->ptrSparkle = (u32)(uintptr_t)DecalGlobal_FindInLEV(lev, rdata.s_sparkle);
                }

                // if linked list of icons exists
                if (gGT->mpkIcons != 0)
                {
#if defined(__LP64__)
                        uintptr_t base_hi = (uintptr_t)sdata->ptrMPK ? ((uintptr_t)sdata->ptrMPK & 0xFFFFFFFF00000000ULL) : 0x100000000ULL;
                        uintptr_t mpkIconsAddr = base_hi | (uintptr_t)gGT->mpkIcons;
                        uint32_t listRel = *(uint32_t *)(mpkIconsAddr + 4);
                        u32 *mpkIconList = listRel ? (u32 *)(base_hi | (uintptr_t)listRel) : NULL;
#else
                        u32 *mpkIconList = (u32 *)*(u32 *)(gGT->mpkIcons + 4);
#endif

                        if (mpkIconList != NULL)
                        {
                                gGT->trafficLightIcon[0] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightredoff);
                                gGT->trafficLightIcon[1] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightredon);
                                gGT->trafficLightIcon[2] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightgreenoff);
                                gGT->trafficLightIcon[3] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightgreenon);
                        }
                }

                gGT->gameMode1_prevFrame = 1;

                MEMPACK_SwapPacks(0);
                if (((gGT->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) == 0) && ((gGT->gameMode2 & CREDITS) == 0))
                {
                        MainInit_JitPoolsNew(gGT);
                        return loadingStage + 1;
                }

                if ((gGT->gameMode2 & LEV_SWAP) == 0)
                        break;

                // === Assume LEV_SWAP Active ===

                if ((gGT->gameMode1 & ADVENTURE_ARENA) == 0)
                        break;

                // === Assume AdventureArena Active ===

                // podium reward
                if (gGT->podiumRewardID == 0)
                        break;

                // === Assume PodiumReward Active ===

                // Set Pack of the hub you're NOT on
                MEMPACK_SwapPacks(3 - gGT->activeMempackIndex);

                // Load model+vrm files on the VRAM page
                // that does NOT overwrite the hub VRAM
                iVar9 = LOAD_GetAdvPackIndex() - 1;

                // NOTE(aalhendi): Retail gates stage advancement until
                // LOAD_Callback_Podiums runs after the final podium file.
                sdata->load_inProgress = 1;

                // VRAM for podium and all related models
                LOAD_AppendQueue(0, LT_VRAM, BI_PODIUMVRMS + iVar9, NULL, NULL);

                int fileIndex;
                u8 *ptrIndexArr = &gGT->podium_modelIndex_First;
                uintptr_t *ptrModelPtrArr = &data.podiumModel_firstPlace;
                void (*setPtrCb)(struct LoadQueueSlot *) = (void (*)(struct LoadQueueSlot *))-2;

                // podium first place
                if ((ptrIndexArr[0] != 0) && (ptrIndexArr[0] != STATIC_OXIDEDANCE))
                {
                        fileIndex = BI_DANCEMODELWIN + iVar9 + (ptrIndexArr[0] - STATIC_CRASHDANCE) * 2;
                        LOAD_AppendQueue(0, LT_GETADDR, fileIndex, &ptrModelPtrArr[0], setPtrCb);
                }

                // podium second place
                if (ptrIndexArr[1] != 0)
                {
                        fileIndex = BI_DANCEMODELLOSE + iVar9 + (ptrIndexArr[1] - STATIC_CRASHDANCE) * 2;
                        LOAD_AppendQueue(0, LT_GETADDR, fileIndex, &ptrModelPtrArr[1], setPtrCb);
                }

                // podium third place
                if (ptrIndexArr[2] != 0)
                {
                        fileIndex = BI_DANCEMODELLOSE + iVar9 + (ptrIndexArr[2] - STATIC_CRASHDANCE) * 2;
                        LOAD_AppendQueue(0, LT_GETADDR, fileIndex, &ptrModelPtrArr[2], setPtrCb);
                }

                // TAWNA
                fileIndex = BI_DANCETAWNAGIRL + iVar9 + (gGT->podium_modelIndex_tawna - STATIC_TAWNA1) * 2;

                // add TAWNA to loading queue
                LOAD_AppendQueue(0, LT_GETADDR, fileIndex, (void *)&data.podiumModel_tawna, setPtrCb);

                // if 0x7e+5 (dingo)
                if (gGT->podium_modelIndex_First == STATIC_DINGODANCE)
                {
                        // add "DingoFire" to loading queue
                        LOAD_AppendQueue(0, LT_GETADDR, BI_DINGOFIRE + iVar9, (void *)&data.podiumModel_dingoFire, setPtrCb);
                }

                // add Podium
                LOAD_AppendQueue(0, LT_GETADDR, BI_PODIUM + iVar9, NULL, LOAD_Callback_Podiums);

                // Disable LEV instances on Adv Hub, for podium scene
                gGT->gameMode2 = gGT->gameMode2 | 0x100;
                break;
        }
        case 8:
        {
                // If going to the podium
                if (((gGT->gameMode1 & ADVENTURE_ARENA) != 0) && (gGT->podiumRewardID != NOFUNC) // 0
                )
                {
                        struct Model **modelPtrArr = (struct Model **)&data.podiumModel_firstPlace;

                        for (int i = 0; i < 8; i++)
                        {
                                struct Model *m = modelPtrArr[i];

                                if (m == 0)
                                        continue;

                                if (i < 7)
                                {
                                        m = (struct Model *)((u8 *)m + 4);
#if defined(__LP64__)
                                        m = LOAD_NativeModel_Convert((void *)m);
#endif
                                        modelPtrArr[i] = m;
                                }

                                if (m == NULL || m->id == -1)
                                        continue;

                                gGT->modelPtr[m->id] = m;
                        }

                        MEMPACK_SwapPacks(gGT->activeMempackIndex);
                }

                // Level ID
                iVar9 = gGT->levelID;

                // Main Menu
                if (iVar9 == 0x27)
                {
                        uVar16 = 7;
                LAB_800346b0:
                        // NOTE(aalhendi): ASM-verified NTSC-U 926 0x80034694-0x800346b8 for the retail post-load audio state call.
#if defined(CTR_NATIVE)
                        LOAD_NativeAudio_SetStateAfterBankReload(uVar16);
#else
                        Audio_SetState_Safe(uVar16);
#endif
                        return loadingStage + 1;
                }

                // One of the maps on Adventure Arena
                if (iVar9 - 0x19U < 5)
                {
                        uVar16 = 6;

                        // podium reward
                        if (gGT->podiumRewardID == NOFUNC) // 0
                        {
                                uVar16 = 5;
                        }
                        goto LAB_800346b0;
                }

                // oxide intro
                if (iVar9 == 0x1e)
                {
                        uVar16 = 3;
                        goto LAB_800346b0;
                }

                // credits
                if (iVar9 == 0x2c)
                {
                        uVar16 = 2;
                        goto LAB_800346b0;
                }

                // Naughty Dog Box
                if (iVar9 == 0x29)
                {
                        uVar16 = 4;
                        goto LAB_800346b0;
                }

                // stop/pause cseq music
                uVar16 = 1;

                if (iVar9 - 0x2aU < 2)
                        goto LAB_800346b0;
                break;
        }
        case 9:
        {
                if (sdata->XA_State == 2)
                        return loadingStage;

                // MAIN_MENU is used for main menu, scrapbook, and adventure garage.
                if (((gGT->gameMode1 & MAIN_MENU) != 0) && (gGT->levelID != ADVENTURE_GARAGE))
                {
                        // disable rendering everything, draw loading screen and instances
                        gGT->renderFlags = (gGT->renderFlags & 0x1000) | 0x20;

                        if (RaceFlag_IsFullyOffScreen() == 1)
                                RaceFlag_BeginTransition(1);
                }

                else if ((gGT->gameMode2 & CREDITS) != 0)
                {
                        // disable rendering everything, draw loading screen and instances
                        gGT->renderFlags = (gGT->renderFlags & 0x1000) | 0x20;
                }

                // Normal level
                else
                {
                        // enable all flags except loading screen
                        gGT->renderFlags = gGT->renderFlags | 0xffffefff;
                }

                gGT->hudFlags = gGT->hudFlags | 8;
                gGT->framesInThisLEV = 0;
                gGT->msInThisLEV = 0;

                ElimBG_Deactivate(gGT);

                // signify end of load
                return -2;
        }
        default:
                return loadingStage;
        }

        loadingStage++;
        return loadingStage;
}
