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
#include "glv_platform.h"
#include "glv_common.h"
#include "glvtrace_vk_helpers.h"
#include "glvtrace_vk_vk.h"
#include "glvtrace_vk_vkdbg.h"
#include "glvtrace_vk_vk_wsi_lunarg.h"
#include "glv_interconnect.h"
#include "glv_filelike.h"
#ifdef WIN32
#include "mhook/mhook-lib/mhook.h"
#endif
#include "glv_trace_packet_utils.h"
#include <stdio.h>

// declared as extern in glvtrace_vk_helpers.h
GLV_CRITICAL_SECTION g_memInfoLock;
VKMemInfo g_memInfo = {0, NULL, NULL, 0};

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocMemory(
    VkDevice device,
    const VkMemoryAllocInfo* pAllocInfo,
    VkDeviceMemory* pMem)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkAllocMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkAllocMemory, get_struct_chain_size((void*)pAllocInfo) + sizeof(VkDeviceMemory));
    result = real_vkAllocMemory(device, pAllocInfo, pMem);
    pPacket = interpret_body_as_vkAllocMemory(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(VkMemoryAllocInfo), pAllocInfo);
    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(VkDeviceMemory), pMem);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    // begin custom code
    add_new_handle_to_mem_info(*pMem, pAllocInfo->allocationSize, NULL);
    // end custom code
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkFlags flags,
    void** ppData)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkMapMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkMapMemory, sizeof(void*));
    result = real_vkMapMemory(device, mem, offset, size, flags, ppData);
    pPacket = interpret_body_as_vkMapMemory(pHeader);
    //TODO handle offset and size in packet and add_data_to_mem_info
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->size = size;
    pPacket->flags = flags;
    if (ppData != NULL)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppData), sizeof(void*), *ppData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));
        add_data_to_mem_info(mem, size, offset, *ppData);
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkUnmapMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkUnmapMemory* pPacket;
    VKAllocInfo *entry;
    SEND_ENTRYPOINT_PARAMS("vkUnmapMemory(mem %p)\n", mem);
    // insert into packet the data that was written by CPU between the vkMapMemory call and here
    // Note must do this prior to the real vkUnMap() or else may get a FAULT
    glv_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(mem);
    CREATE_TRACE_PACKET(vkUnmapMemory, (entry) ? entry->totalSize : 0);
    pPacket = interpret_body_as_vkUnmapMemory(pHeader);
    if (entry)
    {
        assert(entry->handle == mem);
        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), entry->rangeSize, entry->pData + entry->rangeOffset);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
        entry->pData = NULL;
    } else
    {
         glv_LogError("Failed to copy app memory into trace packet (idx = %u) on vkUnmapMemory\n", pHeader->global_packet_index);
    }
    glv_leave_critical_section(&g_memInfoLock);
//    glv_LogError("manual address of vkUnmapMemory: %p\n", real_vkUnmapMemory);
    result = real_vkUnmapMemory(device, mem);
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkFreeMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkFreeMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkFreeMemory, 0);
    result = real_vkFreeMemory(device, mem);
    pPacket = interpret_body_as_vkFreeMemory(pHeader);
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    // begin custom code
    rm_handle_from_mem_info(mem);
    // end custom code
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDescriptorPool(
    VkDevice device,
    VkDescriptorPoolUsage poolUsage,
    uint32_t maxSets,
    const VkDescriptorPoolCreateInfo* pCreateInfo,
    VkDescriptorPool* pDescriptorPool)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkCreateDescriptorPool* pPacket = NULL;
    // begin custom code (needs to use get_struct_chain_size)
    CREATE_TRACE_PACKET(vkCreateDescriptorPool,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDescriptorPool));
    // end custom code
    result = real_vkCreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    pPacket = interpret_body_as_vkCreateDescriptorPool(pHeader);
    pPacket->device = device;
    pPacket->poolUsage = poolUsage;
    pPacket->maxSets = maxSets;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDescriptorPoolCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), pCreateInfo->count * sizeof(VkDescriptorTypeCount), pCreateInfo->pTypeCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorPool), sizeof(VkDescriptorPool), pDescriptorPool);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorPool));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicViewportState(
    VkDevice device,
    const VkDynamicVpStateCreateInfo* pCreateInfo,
    VkDynamicVpState* pState)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkCreateDynamicViewportState* pPacket = NULL;
    // begin custom code (needs to call get_struct_chain_size)
    uint32_t vpsCount = (pCreateInfo != NULL && pCreateInfo->pViewports != NULL) ? pCreateInfo->viewportAndScissorCount : 0;
    CREATE_TRACE_PACKET(vkCreateDynamicViewportState,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicVpState));
    // end custom code
    result = real_vkCreateDynamicViewportState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_vkCreateDynamicViewportState(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicVpStateCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(VkViewport), pCreateInfo->pViewports);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(VkRect), pCreateInfo->pScissors);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicVpState), pState);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    VkFramebuffer* pFramebuffer)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkCreateFramebuffer* pPacket = NULL;
    // begin custom code
    int dsSize = (pCreateInfo != NULL && pCreateInfo->pDepthStencilAttachment != NULL) ? sizeof(VkDepthStencilBindInfo) : 0;
    uint32_t colorCount = (pCreateInfo != NULL && pCreateInfo->pColorAttachments != NULL) ? pCreateInfo->colorAttachmentCount : 0;
    CREATE_TRACE_PACKET(vkCreateFramebuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkFramebuffer));
    // end custom code
    result = real_vkCreateFramebuffer(device, pCreateInfo, pFramebuffer);
    pPacket = interpret_body_as_vkCreateFramebuffer(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkFramebufferCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments), colorCount * sizeof(VkColorAttachmentBindInfo), pCreateInfo->pColorAttachments);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment), dsSize, pCreateInfo->pDepthStencilAttachment);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFramebuffer), sizeof(VkFramebuffer), pFramebuffer);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pFramebuffer));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    VkInstance* pInstance)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkCreateInstance* pPacket = NULL;
    uint64_t startTime;
    glv_platform_thread_once(&gInitOnce, InitTracer);
    SEND_ENTRYPOINT_ID(vkCreateInstance);
    if (real_vkCreateInstance == vkCreateInstance)
    {
        glv_platform_get_next_lib_sym((void **) &real_vkCreateInstance,"vkCreateInstance");
    }
    startTime = glv_get_time();
    result = real_vkCreateInstance(pCreateInfo, pInstance);
    CREATE_TRACE_PACKET(vkCreateInstance, sizeof(VkInstance) + get_struct_chain_size((void*)pCreateInfo));
    pHeader->entrypoint_begin_time = startTime;
    if (isHooked == FALSE) {
        AttachHooks();
        AttachHooks_vkdbg();
        AttachHooks_vk_wsi_lunarg();
    }
    pPacket = interpret_body_as_vkCreateInstance(pHeader);

    add_VkInstanceCreateInfo_to_packet(pHeader, (VkInstanceCreateInfo**)&(pPacket->pCreateInfo), (VkInstanceCreateInfo*) pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInstance), sizeof(VkInstance), pInstance);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pInstance));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    VkRenderPass* pRenderPass)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkCreateRenderPass* pPacket = NULL;
    // begin custom code (get_struct_chain_size)
    uint32_t colorCount = (pCreateInfo != NULL && (pCreateInfo->pColorFormats != NULL || pCreateInfo->pColorLayouts != NULL || pCreateInfo->pColorLoadOps != NULL || pCreateInfo->pColorStoreOps != NULL || pCreateInfo->pColorLoadClearValues != NULL)) ? pCreateInfo->colorAttachmentCount : 0;
    CREATE_TRACE_PACKET(vkCreateRenderPass, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkRenderPass));
    // end custom code
    result = real_vkCreateRenderPass(device, pCreateInfo, pRenderPass);
    pPacket = interpret_body_as_vkCreateRenderPass(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkRenderPassCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorFormats), colorCount * sizeof(VkFormat), pCreateInfo->pColorFormats);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLayouts), colorCount * sizeof(VkImageLayout), pCreateInfo->pColorLayouts);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps), colorCount * sizeof(VkAttachmentLoadOp), pCreateInfo->pColorLoadOps);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps), colorCount * sizeof(VkAttachmentStoreOp), pCreateInfo->pColorStoreOps);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues), colorCount * sizeof(VkClearColor), pCreateInfo->pColorLoadClearValues);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPass), sizeof(VkRenderPass), pRenderPass);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorFormats));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLayouts));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPass));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalExtensionInfo(
    VkExtensionInfoType infoType,
    uint32_t extensionIndex,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    struct_vkGetGlobalExtensionInfo* pPacket = NULL;
    uint64_t startTime;
    glv_platform_thread_once(&gInitOnce, InitTracer);
    if (real_vkGetGlobalExtensionInfo == vkGetGlobalExtensionInfo)
    {
        glv_platform_get_next_lib_sym((void **) &real_vkGetGlobalExtensionInfo,"vkGetGlobalExtensionInfo");
    }
    startTime = glv_get_time();
    result = real_vkGetGlobalExtensionInfo(infoType, extensionIndex, pDataSize, pData);
    CREATE_TRACE_PACKET(vkGetGlobalExtensionInfo, (((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0)));
    pHeader->entrypoint_begin_time = startTime;
    if (isHooked == FALSE) {
        AttachHooks();
        AttachHooks_vkdbg();
        AttachHooks_vk_wsi_lunarg();
    }
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_vkGetGlobalExtensionInfo(pHeader);
    pPacket->infoType = infoType;
    pPacket->extensionIndex = extensionIndex;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkEnumerateLayers(
    VkPhysicalDevice gpu,
    size_t maxStringSize,
    size_t* pLayerCount,
    char* const* pOutLayers,
    void* pReserved)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkEnumerateLayers* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(vkEnumerateLayers);
    startTime = glv_get_time();
    result = real_vkEnumerateLayers(gpu, maxStringSize, pLayerCount, pOutLayers, pReserved);
    size_t totStringSize = 0;
    uint32_t i = 0;
    for (i = 0; i < *pLayerCount; i++) {
        totStringSize += (pOutLayers[i] != NULL) ? strlen(pOutLayers[i]) + 1: 0;
    }
    CREATE_TRACE_PACKET(vkEnumerateLayers, totStringSize + sizeof(size_t));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_vkEnumerateLayers(pHeader);
    pPacket->physicalDevice = gpu;
    pPacket->maxStringSize = maxStringSize;
    pPacket->pReserved = pReserved;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pLayerCount), sizeof(size_t), pLayerCount);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pLayerCount));
    for (i = 0; i < *pLayerCount; i++) {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOutLayers[i]), ((pOutLayers[i] != NULL) ? strlen(pOutLayers[i]) + 1 : 0), pOutLayers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOutLayers[i]));
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkEnumeratePhysicalDevices* pPacket = NULL;
    uint64_t startTime;
    //TODO make sure can handle being called twice with pPD == 0
    SEND_ENTRYPOINT_ID(vkEnumeratePhysicalDevices);
    startTime = glv_get_time();
    result = real_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    CREATE_TRACE_PACKET(vkEnumeratePhysicalDevices, sizeof(uint32_t) + ((pPhysicalDevices && pPhysicalDeviceCount) ? *pPhysicalDeviceCount * sizeof(VkPhysicalDevice) : 0));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_vkEnumeratePhysicalDevices(pHeader);
    pPacket->instance = instance;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPhysicalDeviceCount), sizeof(uint32_t), pPhysicalDeviceCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPhysicalDevices), *pPhysicalDeviceCount*sizeof(VkPhysicalDevice), pPhysicalDevices);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPhysicalDeviceCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPhysicalDevices));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    const VkDescriptorSetLayout* pSetLayouts,
    VkDescriptorSet* pDescriptorSets,
    uint32_t* pCount)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkAllocDescriptorSets* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(vkAllocDescriptorSets);
    startTime = glv_get_time();
    result = real_vkAllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    size_t customSize = (*pCount <= 0) ? (sizeof(VkDescriptorSet)) : (*pCount * sizeof(VkDescriptorSet));
    CREATE_TRACE_PACKET(vkAllocDescriptorSets, sizeof(VkDescriptorSetLayout) + customSize + sizeof(uint32_t));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_vkAllocDescriptorSets(pHeader);
    pPacket->device = device;
    pPacket->descriptorPool = descriptorPool;
    pPacket->setUsage = setUsage;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayouts), count*sizeof(VkDescriptorSetLayout), pSetLayouts);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), customSize, pDescriptorSets);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayouts));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    FINISH_TRACE_PACKET();
    return result;
}

// Manually written because it needs to use get_struct_chain_size and allocate some extra pointers (why?)
// Also since it needs to app the array of pointers and sub-buffers (see comments in function)
GLVTRACER_EXPORT void VKAPI __HOOKED_vkUpdateDescriptors(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t updateCount,
    const void** ppUpdateArray)
{
    glv_trace_packet_header* pHeader;
    struct_vkUpdateDescriptors* pPacket = NULL;
    // begin custom code
    size_t arrayByteCount = get_struct_chain_size(*ppUpdateArray);
    CREATE_TRACE_PACKET(vkUpdateDescriptors, arrayByteCount + updateCount * sizeof(intptr_t));
    // end custom code
    real_vkUpdateDescriptors(device, descriptorSet, updateCount, ppUpdateArray);
    pPacket = interpret_body_as_vkUpdateDescriptors(pHeader);
    pPacket->device = device;
    pPacket->descriptorSet = descriptorSet;
    pPacket->updateCount = updateCount;
    // begin custom code
    // add buffer which is an array of pointers
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppUpdateArray), updateCount * sizeof(intptr_t), ppUpdateArray);
    // add all the sub buffers with descriptor updates
    add_update_descriptors_to_trace_packet(pHeader, updateCount, (void ***) &pPacket->ppUpdateArray, ppUpdateArray);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppUpdateArray));
    // end custom code
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void VKAPI __HOOKED_vkCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    VkWaitEvent                                 waitEvent,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    uint32_t                                    memBarrierCount,
    const void**                                ppMemBarriers)

{
    glv_trace_packet_header* pHeader;
    struct_vkCmdWaitEvents* pPacket = NULL;
    size_t customSize;
    customSize = (eventCount * sizeof(VkEvent)) + memBarrierCount * sizeof(void*) + calculate_memory_barrier_size(memBarrierCount, ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdWaitEvents, customSize);
    real_vkCmdWaitEvents(cmdBuffer, waitEvent, eventCount, pEvents, memBarrierCount, ppMemBarriers);
    pPacket = interpret_body_as_vkCmdWaitEvents(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->waitEvent = waitEvent;
    pPacket->eventCount = eventCount;
    pPacket->memBarrierCount = memBarrierCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pEvents), eventCount * sizeof(VkEvent), pEvents);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pEvents));
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers), memBarrierCount * sizeof(void*), ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < memBarrierCount; i++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) ppMemBarriers[i];
        switch (pNext->sType) {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz = sizeof(VkMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz = sizeof(VkBufferMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz = sizeof(VkImageMemoryBarrier);
                break;
            default:
                assert(0);
                siz = 0;
                break;
        }
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers[i]), siz, ppMemBarriers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers[i]));
    }
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void VKAPI __HOOKED_vkCmdPipelineBarrier(
    VkCmdBuffer                                 cmdBuffer,
    VkWaitEvent                                 waitEvent,
    uint32_t                                    pipeEventCount,
    const VkPipeEvent*                          pPipeEvents,
    uint32_t                                    memBarrierCount,
    const void**                                ppMemBarriers)
{
    glv_trace_packet_header* pHeader;
    struct_vkCmdPipelineBarrier* pPacket = NULL;
    size_t customSize;
    customSize = (pipeEventCount * sizeof(VkPipeEvent)) + memBarrierCount * sizeof(void*) + calculate_memory_barrier_size(memBarrierCount, ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdPipelineBarrier, customSize);
    real_vkCmdPipelineBarrier(cmdBuffer, waitEvent, pipeEventCount, pPipeEvents, memBarrierCount, ppMemBarriers);
    pPacket = interpret_body_as_vkCmdPipelineBarrier(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->waitEvent = waitEvent;
    pPacket->pipeEventCount = pipeEventCount;
    pPacket->memBarrierCount = memBarrierCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipeEvents), sizeof(VkPipeEvent), pPipeEvents);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipeEvents));
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers), memBarrierCount * sizeof(void*), ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < memBarrierCount; i++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) ppMemBarriers[i];
        switch (pNext->sType) {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz = sizeof(VkMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz = sizeof(VkBufferMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz = sizeof(VkImageMemoryBarrier);
                break;
            default:
                assert(0);
                siz = 0;
                break;
        }
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers[i]), siz, ppMemBarriers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers[i]));
    }
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers));
    FINISH_TRACE_PACKET();
}
