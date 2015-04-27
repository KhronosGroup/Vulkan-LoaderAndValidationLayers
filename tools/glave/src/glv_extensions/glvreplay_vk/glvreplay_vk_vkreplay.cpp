/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 * Copyright (C) 2015 Valve Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "vulkan.h"
#include "glvreplay_vk_vkreplay.h"
#include "glvreplay_vk.h"
#include "glvreplay_vk_settings.h"
//#include "glvreplay_vk_write_ppm.h"

#include <algorithm>
#include <queue>

extern "C" {
#include "glv_vk_vk_structs.h"
#include "vk_enum_string_helper.h"
}

glvreplay_settings *g_pReplaySettings;

vkReplay::vkReplay(glvreplay_settings *pReplaySettings)
{
    g_pReplaySettings = pReplaySettings;
    m_display = new vkDisplay();
    m_pDSDump = NULL;
    m_pCBDump = NULL;
    m_pGlvSnapshotPrint = NULL;
    m_objMapper.m_adjustForGPU = false;
    if (g_pReplaySettings && g_pReplaySettings->screenshotList) {
        process_screenshot_list(g_pReplaySettings->screenshotList);
    }
}

vkReplay::~vkReplay()
{
    delete m_display;
    glv_platform_close_library(m_vkFuncs.m_libHandle);
}

int vkReplay::init(glv_replay::Display & disp)
{
    int err;
#if defined PLATFORM_LINUX
    void * handle = dlopen("libvulkan.so", RTLD_LAZY);
#else
    HMODULE handle = LoadLibrary("vulkan.dll" );
#endif

    if (handle == NULL) {
        glv_LogError("Failed to open vulkan library.\n");
        return -1;
    }
    m_vkFuncs.init_funcs(handle);
    disp.set_implementation(m_display);
    if ((err = m_display->init(disp.get_gpu())) != 0) {
        glv_LogError("Failed to init vulkan display.\n");
        return err;
    }
    if (disp.get_window_handle() == 0)
    {
        if ((err = m_display->create_window(disp.get_width(), disp.get_height())) != 0) {
            glv_LogError("Failed to create Window\n");
            return err;
        }
    }
    else
    {
        if ((err = m_display->set_window(disp.get_window_handle(), disp.get_width(), disp.get_height())) != 0)
        {
            glv_LogError("Failed to set Window\n");
            return err;
        }
    }
    return 0;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::handle_replay_errors(const char* entrypointName, const VkResult resCall, const VkResult resTrace, const glv_replay::GLV_REPLAY_RESULT resIn)
{
    glv_replay::GLV_REPLAY_RESULT res = resIn;
    if (resCall != resTrace) {
        glv_LogWarn("Mismatched return from API call (%s) traced result %s, replay result %s\n", entrypointName,
                string_VkResult((VkResult)resTrace), string_VkResult((VkResult)resCall));
        res = glv_replay::GLV_REPLAY_BAD_RETURN;
    }
#if 0
    if (resCall != VK_SUCCESS) {
        glv_LogWarn("API call (%s) returned failed result %s\n", entrypointName, string_VK_RESULT(resCall));
    }
#endif
    return res;
}

void vkReplay::push_validation_msg(VkValidationLevel validationLevel, VkObject srcObject, size_t location, int32_t msgCode, const char * pMsg)
{
    struct validationMsg msgObj;
    msgObj.validationLevel = validationLevel;
    msgObj.srcObject = srcObject;
    msgObj.location = location;
    msgObj.msgCode = msgCode;
    strncpy(msgObj.msg, pMsg, 256);
    msgObj.msg[255] = '\0';
    m_validationMsgs.push_back(msgObj);
}

glv_replay::GLV_REPLAY_RESULT vkReplay::pop_validation_msgs()
{
    if (m_validationMsgs.size() == 0)
        return glv_replay::GLV_REPLAY_SUCCESS;
    m_validationMsgs.clear();
    return glv_replay::GLV_REPLAY_VALIDATION_ERROR;
}

int vkReplay::dump_validation_data()
{
    if  (m_pDSDump && m_pCBDump)
    {
        m_pDSDump((char *) "pipeline_dump.dot");
        m_pCBDump((char *) "cb_dump.dot");
    }
    if (m_pGlvSnapshotPrint != NULL)
    {
        m_pGlvSnapshotPrint();
    }
   return 0;
}

VkResult vkReplay::manually_replay_vkCreateInstance(struct_vkCreateInstance* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    if (!m_display->m_initedVK)
    {
        VkInstance inst;
        if (g_vkReplaySettings.debugLevel > 0)
        {
            // get the list of layers that the debugger wants to enable
            uint32_t numLayers = 0;
            char ** layersStr = get_enableLayers_list(&numLayers);
            apply_layerSettings_overrides();
            if (layersStr != NULL && numLayers > 0)
            {
                // save for restoring later
                const char* const* ppSavedExtensionNames = pPacket->pCreateInfo->ppEnabledExtensionNames;
                size_t savedExtensionCount = pPacket->pCreateInfo->extensionCount;

                // pointers to const locations that will need updating / restoring
                const char* const** pppExtNames = (const char* const**)&pPacket->pCreateInfo->ppEnabledExtensionNames;
                uint32_t* pExtensionCount = (uint32_t*)&pPacket->pCreateInfo->extensionCount;
                uint32_t totalCount = pPacket->pCreateInfo->extensionCount + numLayers;

                *pExtensionCount = totalCount;

                *pppExtNames = GLV_NEW_ARRAY(const char*, totalCount);

                // copy app-supplied extension names
                memcpy((void*)*pppExtNames, ppSavedExtensionNames, savedExtensionCount * sizeof(char*));

                // copy debugger-supplied layer names
                memcpy((void*)&pPacket->pCreateInfo->ppEnabledExtensionNames[savedExtensionCount], layersStr, numLayers * sizeof(char*));

                // make the call
                replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, &inst);

                // clean up and restore
                void* pTmp = (void*)pPacket->pCreateInfo->ppEnabledExtensionNames;
                GLV_DELETE(pTmp);
                *pppExtNames = ppSavedExtensionNames;
                *pExtensionCount = savedExtensionCount;
            }
            else
            {
                replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, &inst);
            }
            release_enableLayer_list(layersStr);

            // get entrypoints from layers that we plan to call
#if !defined(_WIN32)
            m_pDSDump = (void (*)(char*)) m_vkFuncs.real_vkGetProcAddr(NULL, "drawStateDumpDotFile");
            m_pCBDump = (void (*)(char*)) m_vkFuncs.real_vkGetProcAddr(NULL, "drawStateDumpCommandBufferDotFile");
            m_pGlvSnapshotPrint = (GLVSNAPSHOT_PRINT_OBJECTS) m_vkFuncs.real_vkGetProcAddr(NULL, "glvSnapshotPrintObjects");
#endif
        }
        else
        {
            replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, &inst);
        }
        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_map(pPacket->pInstance, &inst);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateDevice(struct_vkCreateDevice* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    if (!m_display->m_initedVK)
    {
        VkDevice device;
        replayResult = m_vkFuncs.real_vkCreateDevice(m_objMapper.remap(pPacket->physicalDevice), pPacket->pCreateInfo, &device);
        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_map(pPacket->pDevice, &device);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkEnumeratePhysicalDevices(struct_vkEnumeratePhysicalDevices* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    if (!m_display->m_initedVK)
    {
        uint32_t deviceCount = *(pPacket->pPhysicalDeviceCount);
        VkPhysicalDevice *pDevices = pPacket->pPhysicalDevices;

        if (pPacket->pPhysicalDevices != NULL)
            pDevices = GLV_NEW_ARRAY(VkPhysicalDevice, deviceCount);
        replayResult = m_vkFuncs.real_vkEnumeratePhysicalDevices(m_objMapper.remap(pPacket->instance), &deviceCount, pDevices);

        //TODO handle different number of physical devices in trace versus replay
        if (deviceCount != *(pPacket->pPhysicalDeviceCount))
        {
            glv_LogWarn("Number of physical devices mismatched in replay %u versus trace %u\n", deviceCount, *(pPacket->pPhysicalDeviceCount));
        }
        else if (deviceCount == 0)
        {
             glv_LogError("vkEnumeratePhysicalDevices number of gpus is zero\n");
        }
        else if (pDevices != NULL)
        {
            glv_LogInfo("Enumerated %d physical devices in the system\n", deviceCount);
        }
        // TODO handle enumeration results in a different order from trace to replay
        for (uint32_t i = 0; i < deviceCount; i++)
        {
            if (pDevices != NULL)
            {
                m_objMapper.add_to_map(&(pPacket->pPhysicalDevices[i]), &(pDevices[i]));
            }
        }
        GLV_DELETE(pDevices);
    }
    if (pPacket->pPhysicalDevices != NULL)
    {
        if (vkDbgRegisterMsgCallback(m_objMapper.remap(pPacket->instance), g_fpDbgMsgCallback, NULL) != VK_SUCCESS)
        {
            glv_LogError("Failed to register vulkan callback for replayer error handling\\n");
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetPhysicalDeviceInfo(struct_vkGetPhysicalDeviceInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    if (!m_display->m_initedVK)
    {
        switch (pPacket->infoType) {
        case VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES:
        {
            VkPhysicalDeviceProperties deviceProps;
            size_t dataSize = sizeof(VkPhysicalDeviceProperties);
            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(m_objMapper.remap(pPacket->physicalDevice), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : &deviceProps);
            if (pPacket->pData != NULL)
            {
                glv_LogInfo("Replay Physical Device Properties\n");
                glv_LogInfo("Vendor ID %x, Device ID %x, name %s\n", deviceProps.vendorId, deviceProps.deviceId, deviceProps.deviceName);
                glv_LogInfo("API version %u, Driver version %u, gpu Type %u\n", deviceProps.apiVersion, deviceProps.driverVersion, deviceProps.deviceType);
                glv_LogInfo("Max Descriptor Sets: %u\n", deviceProps.maxDescriptorSets);
                glv_LogInfo("Max Bound Descriptor Sets: %u\n", deviceProps.maxBoundDescriptorSets);
                glv_LogInfo("Max Thread Group Size: %u\n", deviceProps.maxThreadGroupSize);
                glv_LogInfo("Max Color Attachments: %u\n", deviceProps.maxColorAttachments);
                glv_LogInfo("Max Inline Memory Update Size: %llu\n", deviceProps.maxInlineMemoryUpdateSize);
            }
            break;
        }
        case VK_PHYSICAL_DEVICE_INFO_TYPE_PERFORMANCE:
        {
            VkPhysicalDevicePerformance devicePerfs;
            size_t dataSize = sizeof(VkPhysicalDevicePerformance);
            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(m_objMapper.remap(pPacket->physicalDevice), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : &devicePerfs);
            if (pPacket->pData != NULL)
            {
                glv_LogInfo("Replay Physical Device Performance\n");
                glv_LogInfo("Max device clock %f, max shader ALUs/clock %f, max texel fetches/clock %f\n", devicePerfs.maxDeviceClock, devicePerfs.aluPerClock, devicePerfs.texPerClock);
                glv_LogInfo("Max primitives/clock %f, Max pixels/clock %f\n",devicePerfs.primsPerClock, devicePerfs.pixelsPerClock);
            }
            break;
        }
        case VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES:
        {
            VkPhysicalDeviceQueueProperties *pGpuQueue, *pQ;
            size_t dataSize = sizeof(VkPhysicalDeviceQueueProperties);
            size_t numQueues = 1;
            assert(pPacket->pDataSize);
            if ((*(pPacket->pDataSize) % dataSize) != 0)
                glv_LogWarn("vkGetPhysicalDeviceInfo() for QUEUE_PROPERTIES not an integral data size assuming 1\n");
            else
                numQueues = *(pPacket->pDataSize) / dataSize;
            dataSize = numQueues * dataSize;
            pQ = static_cast < VkPhysicalDeviceQueueProperties *> (glv_malloc(dataSize));
            pGpuQueue = pQ;
            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(m_objMapper.remap(pPacket->physicalDevice), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : pGpuQueue);
            if (pPacket->pData != NULL)
            {
                for (unsigned int i = 0; i < numQueues; i++)
                {
                    glv_LogInfo("Replay Physical Device Queue Property for index %d, flags %u\n", i, pGpuQueue->queueFlags);
                    glv_LogInfo("Max available count %u, max atomic counters %u, supports timestamps %u\n",pGpuQueue->queueCount, pGpuQueue->maxAtomicCounters, pGpuQueue->supportsTimestamps);
                    pGpuQueue++;
                }
            }
            glv_free(pQ);
            break;
        }
        default:
        {
            size_t size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(m_objMapper.remap(pPacket->physicalDevice), pPacket->infoType, &size, pData);
            if (replayResult == VK_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData != NULL)
                {
                    glv_LogWarn("vkGetGpuInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                }
                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                {
                    glv_LogWarn("vkGetGpuInfo returned differing data contents than the trace file contained.\n");
                }
                // TODO : We could pull this out into its own case of switch, and also may want to perform some
                //   validation between the trace values and replay values
                else if (pPacket->infoType == VK_PHYSICAL_DEVICE_INFO_TYPE_DISPLAY_PROPERTIES_WSI) {
                    VkDisplayPropertiesWSI* physicalInfo = (VkDisplayPropertiesWSI*)pData;
                    VkDisplayPropertiesWSI* mapInfo = (VkDisplayPropertiesWSI*)pPacket->pData;
                    uint32_t count = size / sizeof(*physicalInfo), i;
                    for (i = 0; i < count; i++) {
                        m_objMapper.add_to_map(&mapInfo[i].display, &physicalInfo[i].display);
                    }
                }
            }
            glv_free(pData);
            break;
        }
        };
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetGlobalExtensionInfo(struct_vkGetGlobalExtensionInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    if (!m_display->m_initedVK) {
        replayResult = m_vkFuncs.real_vkGetGlobalExtensionInfo(pPacket->infoType, pPacket->extensionIndex, pPacket->pDataSize, pPacket->pData);
// TODO: Confirm that replay'd properties match with traced properties to ensure compatibility.
//        if (replayResult == VK_SUCCESS) {
//            for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)
//            {
//                if (!strncmp(g_extensions[ext], pPacket->pExtName, strlen(g_extensions[ext]))) {
//                    bool extInList = false;
//                    for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {
//                        if (!strncmp(m_display->m_extensions[j], g_extensions[ext], strlen(g_extensions[ext])))
//                            extInList = true;
//                        break;
//                    }
//                    if (!extInList)
//                        m_display->m_extensions.push_back((char *) g_extensions[ext]);
//                    break;
//                }
//            }
//        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetPhysicalDeviceExtensionInfo(struct_vkGetPhysicalDeviceExtensionInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    if (!m_display->m_initedVK) {
        replayResult = m_vkFuncs.real_vkGetPhysicalDeviceExtensionInfo(m_objMapper.remap(pPacket->physicalDevice), pPacket->infoType, pPacket->extensionIndex, pPacket->pDataSize, pPacket->pData);
// TODO: Confirm that replay'd properties match with traced properties to ensure compatibility.
//        if (replayResult == VK_SUCCESS) {
//            for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)
//            {
//                if (!strncmp(g_extensions[ext], pPacket->pExtName, strlen(g_extensions[ext]))) {
//                    bool extInList = false;
//                    for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {
//                        if (!strncmp(m_display->m_extensions[j], g_extensions[ext], strlen(g_extensions[ext])))
//                            extInList = true;
//                        break;
//                    }
//                    if (!extInList)
//                        m_display->m_extensions.push_back((char *) g_extensions[ext]);
//                    break;
//                }
//            }
//        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetSwapChainInfoWSI(struct_vkGetSwapChainInfoWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    size_t dataSize = *pPacket->pDataSize;
    void* pData = glv_malloc(dataSize);
    replayResult = m_vkFuncs.real_vkGetSwapChainInfoWSI(m_objMapper.remap(pPacket->swapChain), pPacket->infoType, &dataSize, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (dataSize != *pPacket->pDataSize)
        {
            glv_LogWarn("SwapChainInfo dataSize differs between trace (%d bytes) and replay (%d bytes)", *pPacket->pDataSize, dataSize);
        }
        if (pPacket->infoType == VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_WSI)
        {
            VkSwapChainImageInfoWSI* pImageInfoReplay = (VkSwapChainImageInfoWSI*)pData;
            VkSwapChainImageInfoWSI* pImageInfoTrace = (VkSwapChainImageInfoWSI*)pPacket->pData;
            size_t imageCountReplay = dataSize / sizeof(VkSwapChainImageInfoWSI);
            size_t imageCountTrace = *pPacket->pDataSize / sizeof(VkSwapChainImageInfoWSI);
            for (size_t i = 0; i < imageCountReplay && i < imageCountTrace; i++)
            {
                imageObj imgObj;
                imgObj.replayImage = pImageInfoReplay[i].image;
                m_objMapper.add_to_map(&pImageInfoTrace[i].image, &imgObj);

                gpuMemObj memObj;
                memObj.replayGpuMem = pImageInfoReplay[i].memory;
                m_objMapper.add_to_map(&pImageInfoTrace[i].memory, &memObj);
            }
        }
    }
    glv_free(pData);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkQueueSubmit(struct_vkQueueSubmit* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkCmdBuffer *remappedBuffers = NULL;
    if (pPacket->pCmdBuffers != NULL)
    {
        remappedBuffers = GLV_NEW_ARRAY( VkCmdBuffer, pPacket->cmdBufferCount);
        for (uint32_t i = 0; i < pPacket->cmdBufferCount; i++)
        {
            *(remappedBuffers + i) = m_objMapper.remap(*(pPacket->pCmdBuffers + i));
        }
    }
    replayResult = m_vkFuncs.real_vkQueueSubmit(m_objMapper.remap(pPacket->queue),
                                                  pPacket->cmdBufferCount,
                                                  remappedBuffers,
                                                  m_objMapper.remap(pPacket->fence));
    GLV_DELETE(remappedBuffers);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkQueueAddMemReferences(struct_vkQueueAddMemReferences* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDeviceMemory *remappedMem = NULL;
    if (pPacket->pMems != NULL)
    {
        remappedMem = GLV_NEW_ARRAY( VkDeviceMemory, pPacket->count);
        for (uint32_t i = 0; i < pPacket->count; i++)
        {
            *(remappedMem + i) = m_objMapper.remap(*(pPacket->pMems + i));
        }
    }
    replayResult = m_vkFuncs.real_vkQueueAddMemReferences(m_objMapper.remap(pPacket->queue),
                                                          pPacket->count,
                                                          remappedMem);
    GLV_DELETE(remappedMem);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkQueueRemoveMemReferences(struct_vkQueueRemoveMemReferences* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDeviceMemory *remappedMem = NULL;
    if (pPacket->pMems != NULL)
    {
        remappedMem = GLV_NEW_ARRAY( VkDeviceMemory, pPacket->count);
        for (uint32_t i = 0; i < pPacket->count; i++)
        {
            *(remappedMem + i) = m_objMapper.remap(*(pPacket->pMems + i));
        }
    }
    replayResult = m_vkFuncs.real_vkQueueRemoveMemReferences(m_objMapper.remap(pPacket->queue),
                                                             pPacket->count,
                                                             remappedMem);
    GLV_DELETE(remappedMem);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetObjectInfo(struct_vkGetObjectInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
        memcpy(pData, pPacket->pData, *pPacket->pDataSize);
    }
    // TODO only search for object once rather than at remap() and init_objMemXXX()
    replayResult = m_vkFuncs.real_vkGetObjectInfo(m_objMapper.remap(pPacket->device), pPacket->objType, m_objMapper.remap(pPacket->object, pPacket->objType), pPacket->infoType, &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkGetObjectInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\\n", size, *pPacket->pDataSize);
        } 
        else if (pData != NULL)
        {
            switch (pPacket->infoType)
            {
                case VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT:
                {
                    uint32_t traceCount = *((uint32_t *) pPacket->pData);
                    uint32_t replayCount = *((uint32_t *) pData);
                    if (traceCount != replayCount)
                        glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_ALLOCATION_COUNT) mismatch: trace count %u, replay count %u\\n", traceCount, replayCount);
                    if (m_objMapper.m_adjustForGPU)
                        m_objMapper.init_objMemCount(pPacket->object, replayCount);
                    break;
                }
                case VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS:
                {
                    VkMemoryRequirements *traceReqs = (VkMemoryRequirements *) pPacket->pData;
                    VkMemoryRequirements *replayReqs = (VkMemoryRequirements *) pData;
                    size_t num = size / sizeof(VkMemoryRequirements);
                    for (unsigned int i = 0; i < num; i++)
                    {
                        if (traceReqs->size != replayReqs->size)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace size %u, replay size %u\\n", traceReqs->size, replayReqs->size);
                        if (traceReqs->alignment != replayReqs->alignment)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace alignment %u, replay aligmnent %u\\n", traceReqs->alignment, replayReqs->alignment);
                        if (traceReqs->granularity != replayReqs->granularity)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace granularity %u, replay granularity %u\\n", traceReqs->granularity, replayReqs->granularity);
                        if (traceReqs->memPropsAllowed != replayReqs->memPropsAllowed)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace memPropsAllowed %u, replay memPropsAllowed %u\\n", traceReqs->memPropsAllowed, replayReqs->memPropsAllowed);
                        if (traceReqs->memPropsRequired != replayReqs->memPropsRequired)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace memPropsRequired %u, replay memPropsRequired %u\\n", traceReqs->memPropsRequired, replayReqs->memPropsRequired);
                        traceReqs++;
                        replayReqs++;
                    }
                    if (m_objMapper.m_adjustForGPU)
                        m_objMapper.init_objMemReqs(pPacket->object, replayReqs - num, num);
                    break;
                }
                default:
                    if (memcmp(pData, pPacket->pData, size) != 0)
                        glv_LogWarn("vkGetObjectInfo() mismatch on *pData: between trace and replay *pDataSize %u\\n", size);
            }
        }
    }
    glv_free(pData);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetFormatInfo(struct_vkGetFormatInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_vkFuncs.real_vkGetFormatInfo(m_objMapper.remap(pPacket->device), pPacket->format, pPacket->infoType, &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkGetFormatInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("vkGetFormatInfo returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetImageSubresourceInfo(struct_vkGetImageSubresourceInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_vkFuncs.real_vkGetImageSubresourceInfo(m_objMapper.remap(pPacket->device), m_objMapper.remap(pPacket->image), pPacket->pSubresource, pPacket->infoType, &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkGetImageSubresourceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("vkGetImageSubresourceInfo returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    return replayResult;
}

void vkReplay::manually_replay_vkUpdateDescriptors(struct_vkUpdateDescriptors* pPacket)
{
    // We have to remap handles internal to the structures so save the handles prior to remap and then restore
    // Rather than doing a deep memcpy of the entire struct and fixing any intermediate pointers, do save and restores via STL queue

    std::queue<VkSampler> saveSamplers;
    std::queue<VkBufferView> saveBufferViews;
    std::queue<VkImageView> saveImageViews;
    std::queue<VkDescriptorSet> saveDescSets;
    uint32_t i = 0;
    for (i = 0; i < pPacket->updateCount; i++)
    {
        VkUpdateSamplers* pUpdateArray = (VkUpdateSamplers*)(pPacket->ppUpdateArray[i]);
        if (pUpdateArray != NULL)
        {
            switch(pUpdateArray->sType)
            {
                case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                {
                    for (uint32_t i = 0; i < ((VkUpdateSamplers*)pUpdateArray)->count; i++)
                    {
                        VkSampler* pLocalSampler = (VkSampler*) &((VkUpdateSamplers*)pUpdateArray)->pSamplers[i];
                        saveSamplers.push(*pLocalSampler);
                        *pLocalSampler = m_objMapper.remap(((VkUpdateSamplers*)pUpdateArray)->pSamplers[i]);
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                {
                    VkUpdateSamplerTextures *pUST = (VkUpdateSamplerTextures *) pUpdateArray;
                    for (uint32_t i = 0; i < pUST->count; i++) {
                        // remap SamplerImageViewInfo.Sampler
                        VkSampler *pLocalSampler = (VkSampler *) &pUST->pSamplerImageViews[i].sampler;
                        saveSamplers.push(*pLocalSampler);
                        *pLocalSampler = m_objMapper.remap(pUST->pSamplerImageViews[i].sampler);

                        // remap SamplerImageViewInfo.ImageViewAttachInfo.ImageView
                        VkImageView *pLocalView = (VkImageView *) &pUST->pSamplerImageViews[i].pImageView->view;
                        saveImageViews.push(*pLocalView);
                        *pLocalView = m_objMapper.remap(pUST->pSamplerImageViews[i].pImageView->view);
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
                {
                    VkUpdateImages *pUI = (VkUpdateImages*) pUpdateArray;
                    for (uint32_t i = 0; i < pUI->count; i++) {
                        VkImageView* pLocalView = (VkImageView*) &pUI->pImageViews[i].view;
                        saveImageViews.push(*pLocalView);
                        *pLocalView = m_objMapper.remap(pUI->pImageViews[i].view);
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
                {
                    VkUpdateBuffers *pUB = (VkUpdateBuffers *) pUpdateArray;
                    for (uint32_t i = 0; i < pUB->count; i++) {
                        VkBufferView* pLocalView = (VkBufferView*) &pUB->pBufferViews[i].view;
                        saveBufferViews.push(*pLocalView);
                        *pLocalView = m_objMapper.remap(pUB->pBufferViews[i].view);
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
                {
                    saveDescSets.push(((VkUpdateAsCopy*)pUpdateArray)->descriptorSet);
                    ((VkUpdateAsCopy*)pUpdateArray)->descriptorSet = m_objMapper.remap(((VkUpdateAsCopy*)pUpdateArray)->descriptorSet);
                    break;
                }
                default:
                {
                    assert(0);
                    break;
                }
            }
        }
    }
    m_vkFuncs.real_vkUpdateDescriptors(m_objMapper.remap(pPacket->device), m_objMapper.remap(pPacket->descriptorSet), pPacket->updateCount, pPacket->ppUpdateArray);
    for (i = 0; i < pPacket->updateCount; i++)
    {
        VkUpdateSamplers* pUpdateArray = (VkUpdateSamplers*)(pPacket->ppUpdateArray[i]);
        if (pUpdateArray != NULL)
        {
            switch(pUpdateArray->sType)
            {
                case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                    for (uint32_t i = 0; i < ((VkUpdateSamplers*)pUpdateArray)->count; i++) {
                        VkSampler* pLocalSampler = (VkSampler*) &((VkUpdateSamplers*)pUpdateArray)->pSamplers[i];
                        *pLocalSampler = saveSamplers.front();
                        saveSamplers.pop();
                    }
                    break;
                case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                {
                    VkUpdateSamplerTextures *pUST = (VkUpdateSamplerTextures *) pUpdateArray;
                    for (uint32_t i = 0; i < pUST->count; i++) {
                        VkSampler *plocalSampler = (VkSampler *) &pUST->pSamplerImageViews[i].sampler;
                        *plocalSampler = saveSamplers.front();
                        saveSamplers.pop();
                        VkImageView *pLocalView = (VkImageView *) &pUST->pSamplerImageViews[i].pImageView->view;
                        *pLocalView = saveImageViews.front();
                        saveImageViews.pop();
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
                {
                    VkUpdateImages *pUI = (VkUpdateImages*) pUpdateArray;
                    for (uint32_t i = 0; i < pUI->count; i++) {
                        VkImageView* pLocalView = (VkImageView*) &pUI->pImageViews[i].view;
                        *pLocalView = saveImageViews.front();
                        saveImageViews.pop();
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
                {
                    VkUpdateBuffers *pUB = (VkUpdateBuffers *) pUpdateArray;
                    for (uint32_t i = 0; i < pUB->count; i++) {
                        VkBufferView* pLocalView = (VkBufferView*) &pUB->pBufferViews[i].view;
                        *pLocalView = saveBufferViews.front();
                        saveBufferViews.pop();
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
                    ((VkUpdateAsCopy*)pUpdateArray)->descriptorSet = saveDescSets.front();
                    saveDescSets.pop();
                    //pLocalUpdateChain = (void*)((VK_UPDATE_SAMPLERS*)pLocalUpdateChain)->pNext;
                    break;
                default:
                    assert(0);
                    break;
            }
        }
    }
    return;
}

VkResult vkReplay::manually_replay_vkCreateDescriptorSetLayout(struct_vkCreateDescriptorSetLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkSampler *local_pImmutableSamplers;
    // TODO: Need to make a whole new CreateInfo struct so that we can remap pImmutableSamplers without affecting the packet.
    VkDescriptorSetLayoutCreateInfo *pInfo = (VkDescriptorSetLayoutCreateInfo*) pPacket->pCreateInfo;
    while (pInfo != NULL)
    {
        if (pInfo->pBinding != NULL)
        {
            for (unsigned int i = 0; i < pInfo->count; i++)
            {
                VkDescriptorSetLayoutBinding *pBinding = (VkDescriptorSetLayoutBinding *) &pInfo->pBinding[i];
                local_pImmutableSamplers = (VkSampler*)glv_malloc(sizeof(VkSampler) * pInfo->count);
                if (pBinding->pImmutableSamplers != NULL)
                {
                    for (unsigned int j = 0; j < pBinding->count; j++)
                    {
                        VkSampler* pSampler = (VkSampler*)&pBinding->pImmutableSamplers[j];
                        *pSampler = m_objMapper.remap(pBinding->pImmutableSamplers[j]);
                    }
                }
            }
        }
        pInfo = (VkDescriptorSetLayoutCreateInfo*)pPacket->pCreateInfo->pNext;
    }
    VkDescriptorSetLayout setLayout;
    replayResult = m_vkFuncs.real_vkCreateDescriptorSetLayout(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &setLayout);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pSetLayout, &setLayout);
    }
    glv_free(local_pImmutableSamplers);
    return replayResult;
}

void vkReplay::manually_replay_vkCmdBindDescriptorSets(struct_vkCmdBindDescriptorSets* pPacket)
{

    VkDescriptorSet *pSaveSets = (VkDescriptorSet *) glv_malloc(sizeof(VkDescriptorSet) * pPacket->setCount);
    if (pSaveSets == NULL)
    {
        glv_LogError("Replay of CmdBindDescriptorSets out of memory\n");
    }
    for (uint32_t idx = 0; idx < pPacket->setCount && pPacket->pDescriptorSets != NULL; idx++)
    {
        VkDescriptorSet *pSet = (VkDescriptorSet *) &(pPacket->pDescriptorSets[idx]);
        pSaveSets[idx] = pPacket->pDescriptorSets[idx];
        *pSet = m_objMapper.remap(pPacket->pDescriptorSets[idx]);
    }
    m_vkFuncs.real_vkCmdBindDescriptorSets(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->firstSet, pPacket->setCount, pPacket->pDescriptorSets, pPacket->dynamicOffsetCount, pPacket->pDynamicOffsets);
    for (uint32_t idx = 0; idx < pPacket->setCount && pPacket->pDescriptorSets != NULL; idx++)
    {
        VkDescriptorSet *pSet = (VkDescriptorSet *) &(pPacket->pDescriptorSets[idx]);
        *pSet = pSaveSets[idx];
    }
    glv_free(pSaveSets);
    return;
}

void vkReplay::manually_replay_vkCmdBindVertexBuffers(struct_vkCmdBindVertexBuffers* pPacket)
{

    VkBuffer *pSaveBuff = GLV_NEW_ARRAY(VkBuffer, pPacket->bindingCount);
    if (pSaveBuff == NULL)
    {
        glv_LogError("Replay of CmdBindVertexBuffers out of memory\n");
    }
    uint32_t i = 0;
    if (pPacket->pBuffers) {
        for (i = 0; i < pPacket->bindingCount; i++)
        {
            VkBuffer *pBuff = (VkBuffer*) &(pPacket->pBuffers[i]);
            pSaveBuff[i] = pPacket->pBuffers[i];
            *pBuff = m_objMapper.remap(pPacket->pBuffers[i]);
        }
    }
    m_vkFuncs.real_vkCmdBindVertexBuffers(m_objMapper.remap(pPacket->cmdBuffer), pPacket->startBinding, pPacket->bindingCount, pPacket->pBuffers, pPacket->pOffsets);
    for (uint32_t k = 0; k < i; k++)
    {
        VkBuffer *pBuff = (VkBuffer*) &(pPacket->pBuffers[k]);
        *pBuff = pSaveBuff[k];
    }
    GLV_DELETE(pSaveBuff);
    return;
}

VkResult vkReplay::manually_replay_vkCreateGraphicsPipeline(struct_vkCreateGraphicsPipeline* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkGraphicsPipelineCreateInfo createInfo;

    // array to store the original trace-time shaders, so that we can remap them inside the packet and then
    // restore them after replaying the API call.
    struct shaderPair saveShader[10];
    unsigned int idx = 0;
    memcpy(&createInfo, pPacket->pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo));
    createInfo.layout = m_objMapper.remap(createInfo.layout);
    // Cast to shader type, as those are of primary interest and all structs in LL have same header w/ sType & pNext
    VkPipelineShaderStageCreateInfo* pPacketNext = (VkPipelineShaderStageCreateInfo*)pPacket->pCreateInfo->pNext;
    VkPipelineShaderStageCreateInfo* pNext = (VkPipelineShaderStageCreateInfo*)createInfo.pNext;
    while (VK_NULL_HANDLE != pPacketNext)
    {
        if (VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO == pNext->sType)
        {
            saveShader[idx].val = pNext->shader.shader;
            saveShader[idx++].addr = &(pNext->shader.shader);
            pNext->shader.shader = m_objMapper.remap(pPacketNext->shader.shader);
        }
        else
        {
            // others acceptable types are:
            // VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
            // VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
            // VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
            // VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
            // VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO;
            // VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO;
            // however they do not currently contain any vulkan objects that need to be remapped.
            // SO, there is nothing to do here!
        }
        pPacketNext = (VkPipelineShaderStageCreateInfo*)pPacketNext->pNext;
        pNext = (VkPipelineShaderStageCreateInfo*)pNext->pNext;
    }

    VkPipeline pipeline;
    replayResult = m_vkFuncs.real_vkCreateGraphicsPipeline(m_objMapper.remap(pPacket->device), &createInfo, &pipeline);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pPipeline, &pipeline);
    }

    // restore packet to contain the original shaders before being remapped.
    for (unsigned int i = 0; i < idx; i++)
        *(saveShader[i].addr) = saveShader[i].val;

    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreatePipelineLayout(struct_vkCreatePipelineLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    // array to store the original trace-time layouts, so that we can remap them inside the packet and then
    // restore them after replaying the API call.
    VkDescriptorSetLayout* pSaveLayouts = (VkDescriptorSetLayout*) glv_malloc(sizeof(VkDescriptorSetLayout) * pPacket->pCreateInfo->descriptorSetCount);
    if (!pSaveLayouts) {
        glv_LogError("Replay of CreatePipelineLayout out of memory\n");
    }
    uint32_t i = 0;
    for (i = 0; (i < pPacket->pCreateInfo->descriptorSetCount) && (pPacket->pCreateInfo->pSetLayouts != NULL); i++) {
        VkDescriptorSetLayout* pSL = (VkDescriptorSetLayout*) &(pPacket->pCreateInfo->pSetLayouts[i]);
        pSaveLayouts[i] = pPacket->pCreateInfo->pSetLayouts[i];
        *pSL = m_objMapper.remap(pPacket->pCreateInfo->pSetLayouts[i]);
    }
    VkPipelineLayout localPipelineLayout;
    replayResult = m_vkFuncs.real_vkCreatePipelineLayout(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &localPipelineLayout);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pPipelineLayout, &localPipelineLayout);
    }
    // restore packet to contain the original Set Layouts before being remapped.
    for (uint32_t k = 0; k < i; k++) {
        VkDescriptorSetLayout* pSL = (VkDescriptorSetLayout*) &(pPacket->pCreateInfo->pSetLayouts[k]);
        *pSL = pSaveLayouts[k];
    }
    glv_free(pSaveLayouts);
    return replayResult;
}

void vkReplay::manually_replay_vkCmdWaitEvents(struct_vkCmdWaitEvents* pPacket)
{

    VkEvent* saveEvent = GLV_NEW_ARRAY(VkEvent, pPacket->eventCount);
    uint32_t idx = 0;
    uint32_t numRemapBuf = 0;
    uint32_t numRemapImg = 0;
    for (idx = 0; idx < pPacket->eventCount; idx++)
    {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pEvents[idx]);
        saveEvent[idx] = pPacket->pEvents[idx];
        *pEvent = m_objMapper.remap(pPacket->pEvents[idx]);
    }

    VkBuffer* saveBuf = GLV_NEW_ARRAY(VkBuffer, pPacket->memBarrierCount);
    VkImage* saveImg = GLV_NEW_ARRAY(VkImage, pPacket->memBarrierCount);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = m_objMapper.remap(pNextBuf->buffer);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            assert(numRemapImg < 100);
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = m_objMapper.remap(pNextImg->image);
        }
    }
    m_vkFuncs.real_vkCmdWaitEvents(m_objMapper.remap(pPacket->cmdBuffer), pPacket->waitEvent, pPacket->eventCount, pPacket->pEvents, pPacket->memBarrierCount, pPacket->ppMemBarriers);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    for (idx = 0; idx < pPacket->eventCount; idx++) {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pEvents[idx]);
        *pEvent = saveEvent[idx];
    }
    GLV_DELETE(saveEvent);
    GLV_DELETE(saveBuf);
    GLV_DELETE(saveImg);
    return;
}

void vkReplay::manually_replay_vkCmdPipelineBarrier(struct_vkCmdPipelineBarrier* pPacket)
{

    uint32_t idx = 0;
    uint32_t numRemapBuf = 0;
    uint32_t numRemapImg = 0;
    VkBuffer* saveBuf = GLV_NEW_ARRAY(VkBuffer, pPacket->memBarrierCount);
    VkImage* saveImg = GLV_NEW_ARRAY(VkImage, pPacket->memBarrierCount);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = m_objMapper.remap(pNextBuf->buffer);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = m_objMapper.remap(pNextImg->image);
        }
    }
    m_vkFuncs.real_vkCmdPipelineBarrier(m_objMapper.remap(pPacket->cmdBuffer), pPacket->waitEvent, pPacket->pipeEventCount, pPacket->pPipeEvents, pPacket->memBarrierCount, pPacket->ppMemBarriers);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    GLV_DELETE(saveBuf);
    GLV_DELETE(saveImg);
    return;
}

VkResult vkReplay::manually_replay_vkCreateFramebuffer(struct_vkCreateFramebuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkFramebufferCreateInfo *pInfo = (VkFramebufferCreateInfo *) pPacket->pCreateInfo;
    VkColorAttachmentBindInfo *pColorAttachments, *pSavedColor = (VkColorAttachmentBindInfo*)pInfo->pColorAttachments;
    bool allocatedColorAttachments = false;
    if (pSavedColor != NULL)
    {
        allocatedColorAttachments = true;
        pColorAttachments = GLV_NEW_ARRAY(VkColorAttachmentBindInfo, pInfo->colorAttachmentCount);
        memcpy(pColorAttachments, pSavedColor, sizeof(VkColorAttachmentBindInfo) * pInfo->colorAttachmentCount);
        for (uint32_t i = 0; i < pInfo->colorAttachmentCount; i++)
        {
            pColorAttachments[i].view = m_objMapper.remap(pInfo->pColorAttachments[i].view);
        }
        pInfo->pColorAttachments = pColorAttachments;
    }
    // remap depth stencil target
    const VkDepthStencilBindInfo *pSavedDS = pInfo->pDepthStencilAttachment;
    VkDepthStencilBindInfo depthTarget;
    if (pSavedDS != NULL)
    {
        memcpy(&depthTarget, pSavedDS, sizeof(VkDepthStencilBindInfo));
        depthTarget.view = m_objMapper.remap(pSavedDS->view);
        pInfo->pDepthStencilAttachment = &depthTarget;
    }
    VkFramebuffer local_framebuffer;
    replayResult = m_vkFuncs.real_vkCreateFramebuffer(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_framebuffer);
    pInfo->pColorAttachments = pSavedColor;
    pInfo->pDepthStencilAttachment = pSavedDS;
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pFramebuffer, &local_framebuffer);
    }
    if (allocatedColorAttachments)
    {
        GLV_DELETE((void*)pColorAttachments);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateRenderPass(struct_vkCreateRenderPass* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkRenderPass local_renderpass;
    replayResult = m_vkFuncs.real_vkCreateRenderPass(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_renderpass);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pRenderPass, &local_renderpass);
    }
    return replayResult;
}

void vkReplay::manually_replay_vkCmdBeginRenderPass(struct_vkCmdBeginRenderPass* pPacket)
{

    VkRenderPassBegin local_renderPassBegin;
    local_renderPassBegin.framebuffer = m_objMapper.remap(pPacket->pRenderPassBegin->framebuffer);
    local_renderPassBegin.renderPass = m_objMapper.remap(pPacket->pRenderPassBegin->renderPass);
    m_vkFuncs.real_vkCmdBeginRenderPass(m_objMapper.remap(pPacket->cmdBuffer), &local_renderPassBegin);
    return;
}

VkResult vkReplay::manually_replay_vkBeginCommandBuffer(struct_vkBeginCommandBuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkCmdBufferBeginInfo* pInfo = (VkCmdBufferBeginInfo*)pPacket->pBeginInfo;
    // assume only zero or one graphics_begin_info in the chain
    VkRenderPassBegin savedRP, *pRP;
    VkCmdBufferGraphicsBeginInfo *pGInfo = NULL;
    while (pInfo != NULL)
    {
        if (pInfo->sType == VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO)
        {
            pGInfo = (VkCmdBufferGraphicsBeginInfo *) pInfo;
            savedRP = pGInfo->renderPassContinue;
            pRP = &(pGInfo->renderPassContinue);
            pRP->renderPass = m_objMapper.remap(savedRP.renderPass);
            pRP->framebuffer = m_objMapper.remap(savedRP.framebuffer);
            break;
        }
        pInfo = (VkCmdBufferBeginInfo*) pInfo->pNext;
    }
    replayResult = m_vkFuncs.real_vkBeginCommandBuffer(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pBeginInfo);
    if (pGInfo != NULL)
        pGInfo->renderPassContinue = savedRP;
    return replayResult;
}

VkResult vkReplay::manually_replay_vkStorePipeline(struct_vkStorePipeline* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_vkFuncs.real_vkStorePipeline(m_objMapper.remap(pPacket->device), m_objMapper.remap(pPacket->pipeline), &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkStorePipeline returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("vkStorePipeline returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetMultiDeviceCompatibility(struct_vkGetMultiDeviceCompatibility* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkPhysicalDeviceCompatibilityInfo compatInfo;
    VkPhysicalDevice handle0, handle1;
    handle0 = m_objMapper.remap(pPacket->physicalDevice0);
    handle1 = m_objMapper.remap(pPacket->physicalDevice1);
    replayResult = m_vkFuncs.real_vkGetMultiDeviceCompatibility(handle0, handle1, &compatInfo);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkDestroyObject(struct_vkDestroyObject* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkObject object = m_objMapper.remap(pPacket->object, pPacket->objType);
    if (object != VK_NULL_HANDLE)
        replayResult = m_vkFuncs.real_vkDestroyObject(m_objMapper.remap(pPacket->device), pPacket->objType, object);
    if (replayResult == VK_SUCCESS)
        m_objMapper.rm_from_map(pPacket->object, pPacket->objType);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkWaitForFences(struct_vkWaitForFences* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    uint32_t i;

    VkFence *pFence = GLV_NEW_ARRAY(VkFence, pPacket->fenceCount);
    for (i = 0; i < pPacket->fenceCount; i++)
    {
        *(pFence + i) = m_objMapper.remap(*(pPacket->pFences + i));
    }
    replayResult = m_vkFuncs.real_vkWaitForFences(m_objMapper.remap(pPacket->device), pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);
    GLV_DELETE(pFence);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkAllocMemory(struct_vkAllocMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    gpuMemObj local_mem;

    if (!m_objMapper.m_adjustForGPU)
        replayResult = m_vkFuncs.real_vkAllocMemory(m_objMapper.remap(pPacket->device), pPacket->pAllocInfo, &local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS || m_objMapper.m_adjustForGPU)
    {
        local_mem.pGpuMem = new (gpuMemory);
        if (local_mem.pGpuMem)
            local_mem.pGpuMem->setAllocInfo(pPacket->pAllocInfo, m_objMapper.m_adjustForGPU);
        m_objMapper.add_to_map(pPacket->pMem, &local_mem);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkFreeMemory(struct_vkFreeMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    gpuMemObj local_mem;
    local_mem = m_objMapper.m_devicememorys.find(pPacket->mem)->second;
    // TODO how/when to free pendingAlloc that did not use and existing gpuMemObj
    replayResult = m_vkFuncs.real_vkFreeMemory(m_objMapper.remap(pPacket->device), local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS)
    {
        delete local_mem.pGpuMem;
        m_objMapper.rm_from_map(pPacket->mem);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkMapMemory(struct_vkMapMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    gpuMemObj local_mem = m_objMapper.m_devicememorys.find(pPacket->mem)->second;
    void* pData;
    if (!local_mem.pGpuMem->isPendingAlloc())
    {
        replayResult = m_vkFuncs.real_vkMapMemory(m_objMapper.remap(pPacket->device), local_mem.replayGpuMem, pPacket->offset, pPacket->size, pPacket->flags, &pData);
        if (replayResult == VK_SUCCESS)
        {
            if (local_mem.pGpuMem)
            {
                local_mem.pGpuMem->setMemoryMapRange(pData, 0, 0, false);
            }
        }
    }
    else
    {
        if (local_mem.pGpuMem)
        {
            local_mem.pGpuMem->setMemoryMapRange(NULL, 0, 0, true);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkUnmapMemory(struct_vkUnmapMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    gpuMemObj local_mem = m_objMapper.m_devicememorys.find(pPacket->mem)->second;
    if (!local_mem.pGpuMem->isPendingAlloc())
    {
        if (local_mem.pGpuMem)
        {
            local_mem.pGpuMem->copyMappingData(pPacket->pData);  // copies data from packet into memory buffer
        }
        replayResult = m_vkFuncs.real_vkUnmapMemory(m_objMapper.remap(pPacket->device), local_mem.replayGpuMem);
    }
    else
    {
        if (local_mem.pGpuMem)
        {
            unsigned char *pBuf = (unsigned char *) glv_malloc(local_mem.pGpuMem->getMemoryMapSize());
            if (!pBuf)
            {
                glv_LogError("vkUnmapMemory() malloc failed");
            }
            local_mem.pGpuMem->setMemoryDataAddr(pBuf);
            local_mem.pGpuMem->copyMappingData(pPacket->pData);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkPinSystemMemory(struct_vkPinSystemMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    gpuMemObj local_mem;
    /* TODO do we need to skip (make pending) this call for m_adjustForGPU */
    replayResult = m_vkFuncs.real_vkPinSystemMemory(m_objMapper.remap(pPacket->device), pPacket->pSysMem, pPacket->memSize, &local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS)
        m_objMapper.add_to_map(pPacket->pMem, &local_mem);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateSwapChainWSI(struct_vkCreateSwapChainWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkSwapChainWSI local_pSwapChain;
    VkDisplayWSI *local_pDisplays = GLV_NEW_ARRAY(VkDisplayWSI, pPacket->pCreateInfo->displayCount);
    uint32_t idx = 0;

    for (idx = 0; idx < pPacket->pCreateInfo->displayCount; idx++)
    {
        VkDisplayWSI *pDisplays = (VkDisplayWSI *) &(pPacket->pCreateInfo->pDisplays[idx]);
        local_pDisplays[idx] = pPacket->pCreateInfo->pDisplays[idx];
        *pDisplays = m_objMapper.remap(pPacket->pCreateInfo->pDisplays[idx]);
    }

    void** ppNativeWindowHandle = (void**)&(pPacket->pCreateInfo->pNativeWindowHandle);
    *ppNativeWindowHandle = (void*)m_display->get_window_handle();
    void** ppNativeWindowSystemHandle = (void**)&(pPacket->pCreateInfo->pNativeWindowSystemHandle);
    *ppNativeWindowSystemHandle = (void*)m_display->get_connection_handle();

    replayResult = m_vkFuncs.real_vkCreateSwapChainWSI(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pSwapChain);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pSwapChain, &local_pSwapChain);
    }
    for (idx = 0; idx < pPacket->pCreateInfo->displayCount; idx++)
    {
        VkDisplayWSI *pDisplays = (VkDisplayWSI *) &(pPacket->pCreateInfo->pDisplays[idx]);
        *pDisplays = local_pDisplays[idx];
    }
    GLV_DELETE(local_pDisplays);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkQueuePresentWSI(struct_vkQueuePresentWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkImage save_image = pPacket->pPresentInfo->image;
    ((VkPresentInfoWSI *) pPacket->pPresentInfo)->image = m_objMapper.remap(pPacket->pPresentInfo->image);
    replayResult = m_vkFuncs.real_vkQueuePresentWSI(m_objMapper.remap(pPacket->queue), pPacket->pPresentInfo);
    ((VkPresentInfoWSI *) pPacket->pPresentInfo)->image = save_image;
    return replayResult;
}
