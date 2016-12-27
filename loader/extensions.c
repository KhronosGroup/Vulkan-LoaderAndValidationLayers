/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vk_loader_platform.h"
#include "loader.h"
#include "extensions.h"
#include "table_ops.h"
#include <vulkan/vk_icd.h>
#include "wsi.h"

// Definitions for the VK_KHR_get_physical_device_properties2 extension

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2KHR(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR *pFeatures) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceFeatures2KHR(unwrapped_phys_dev, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFeatures2KHR(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR *pFeatures) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceFeatures2KHR) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceFeatures2KHR");
    }
    icd_term->GetPhysicalDeviceFeatures2KHR(phys_dev_term->phys_dev, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice,
                                  VkPhysicalDeviceProperties2KHR *pProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceProperties2KHR(unwrapped_phys_dev, pProperties);
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceProperties2KHR(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2KHR *pProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceProperties2KHR) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceProperties2KHR");
    }
    icd_term->GetPhysicalDeviceProperties2KHR(phys_dev_term->phys_dev,
                                              pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2KHR(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties2KHR *pFormatProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceFormatProperties2KHR(unwrapped_phys_dev, format,
                                                pFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFormatProperties2KHR(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties2KHR *pFormatProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceFormatProperties2KHR) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceFormatProperties2KHR");
    }
    icd_term->GetPhysicalDeviceFormatProperties2KHR(phys_dev_term->phys_dev,
                                                    format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2KHR *pImageFormatInfo,
    VkImageFormatProperties2KHR *pImageFormatProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->GetPhysicalDeviceImageFormatProperties2KHR(
        unwrapped_phys_dev, pImageFormatInfo, pImageFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2KHR *pImageFormatInfo,
    VkImageFormatProperties2KHR *pImageFormatProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceImageFormatProperties2KHR) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceImageFormatProperties2KHR");
    }
    return icd_term->GetPhysicalDeviceImageFormatProperties2KHR(
        phys_dev_term->phys_dev, pImageFormatInfo, pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2KHR(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2KHR *pQueueFamilyProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceQueueFamilyProperties2KHR(
        unwrapped_phys_dev, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceQueueFamilyProperties2KHR(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2KHR *pQueueFamilyProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceQueueFamilyProperties2KHR) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
    }
    icd_term->GetPhysicalDeviceQueueFamilyProperties2KHR(
        phys_dev_term->phys_dev, pQueueFamilyPropertyCount,
        pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties2KHR(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2KHR *pMemoryProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceMemoryProperties2KHR(unwrapped_phys_dev,
                                                pMemoryProperties);
}
VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceMemoryProperties2KHR(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2KHR *pMemoryProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceMemoryProperties2KHR) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceMemoryProperties2KHR");
    }
    icd_term->GetPhysicalDeviceMemoryProperties2KHR(phys_dev_term->phys_dev,
                                                    pMemoryProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSparseImageFormatInfo2KHR *pFormatInfo,
    uint32_t *pPropertyCount, VkSparseImageFormatProperties2KHR *pProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceSparseImageFormatProperties2KHR(
        unwrapped_phys_dev, pFormatInfo, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceSparseImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSparseImageFormatInfo2KHR *pFormatInfo,
    uint32_t *pPropertyCount, VkSparseImageFormatProperties2KHR *pProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceSparseImageFormatProperties2KHR) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
    }
    icd_term->GetPhysicalDeviceSparseImageFormatProperties2KHR(
        phys_dev_term->phys_dev, pFormatInfo, pPropertyCount, pProperties);
}

// Definitions for the VK_KHR_maintenance1 extension

VKAPI_ATTR void VKAPI_CALL
vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool,
                     VkCommandPoolTrimFlagsKHR flags) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    disp->TrimCommandPoolKHR(device, commandPool, flags);
}

// Definitions for the VK_KHX_device_group_creation extension

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroupsKHX(
    VkInstance instance, uint32_t *pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupPropertiesKHX *pPhysicalDeviceGroupProperties) {
    VkResult res = VK_SUCCESS;
    struct loader_instance *inst = NULL;

    loader_platform_thread_lock_mutex(&loader_lock);

    inst = loader_get_instance(instance);
    if (NULL == inst) {
        res = VK_ERROR_INITIALIZATION_FAILED;
        goto out;
    }

    if (pPhysicalDeviceGroupProperties == NULL || 0 == inst->total_gpu_count) {
        VkResult setup_res = setupLoaderTrampPhysDevs(inst);
        if (VK_SUCCESS != setup_res) {
            res = setup_res;
            goto out;
        }
    }

    res = inst->disp->layer_inst_disp.EnumeratePhysicalDeviceGroupsKHX(
        instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
    if ((VK_SUCCESS != res && VK_INCOMPLETE != res) ||
        NULL == pPhysicalDeviceGroupProperties) {
        goto out;
    }

    for (uint32_t group = 0; group < *pPhysicalDeviceGroupCount; group++) {
        for (uint32_t dev = 0;
             dev < pPhysicalDeviceGroupProperties[group].physicalDeviceCount;
             dev++) {
            for (uint32_t tramp = 0; tramp < inst->total_gpu_count; tramp++) {
                if (inst->phys_devs_tramp[tramp]->phys_dev ==
                    pPhysicalDeviceGroupProperties[group]
                        .physicalDevices[dev]) {
                    pPhysicalDeviceGroupProperties[group].physicalDevices[dev] =
                        (VkPhysicalDevice)inst->phys_devs_tramp[tramp];
                }
            }
        }
    }

out:

    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_EnumeratePhysicalDeviceGroupsKHX(
    VkInstance instance, uint32_t *pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupPropertiesKHX *pPhysicalDeviceGroupProperties) {
    struct loader_instance *inst = loader_get_instance(instance);
    VkResult res = VK_SUCCESS;
    uint32_t total_group_count = 0;
    uint32_t max_group_count = *pPhysicalDeviceGroupCount;
    uint32_t i = 0;

    // We have to loop through all ICDs which may be capable of handling this
    // call and sum all the possible physical device groups together.
    struct loader_icd_term *icd_term = inst->icd_terms;
    while (NULL != icd_term) {
        if (NULL != icd_term->EnumeratePhysicalDeviceGroupsKHX) {
            uint32_t cur_group_count = 0;
            res = icd_term->EnumeratePhysicalDeviceGroupsKHX(
                icd_term->instance, &cur_group_count, NULL);
            if (res != VK_SUCCESS) {
                break;
            } else if (NULL != pPhysicalDeviceGroupProperties &&
                       max_group_count > total_group_count) {

                uint32_t remain_count = max_group_count - total_group_count;
                res = icd_term->EnumeratePhysicalDeviceGroupsKHX(
                    icd_term->instance, &remain_count,
                    &pPhysicalDeviceGroupProperties[total_group_count]);
                if (res != VK_SUCCESS) {
                    break;
                }
            }
            total_group_count += cur_group_count;
        } else {
            // For ICDs which don't directly support this, create a group
            // for each physical device
            for (uint32_t j = 0; j < inst->total_gpu_count; j++) {
                if (inst->phys_devs_term[j]->icd_index == i) {
                    if (NULL != pPhysicalDeviceGroupProperties &&
                        max_group_count > total_group_count) {
                        pPhysicalDeviceGroupProperties[total_group_count]
                            .physicalDeviceCount = 1;
                        pPhysicalDeviceGroupProperties[total_group_count]
                            .physicalDevices[0] =
                            inst->phys_devs_term[j]->phys_dev;
                    }
                    total_group_count++;
                }
            }
        }
        icd_term = icd_term->next;
        i++;
    }

    *pPhysicalDeviceGroupCount = total_group_count;

    // Replace the physical devices with the value from the loader terminator
    // so we can de-reference them if needed.
    if (NULL != pPhysicalDeviceGroupProperties) {
        for (uint32_t group = 0; group < max_group_count; group++) {
            VkPhysicalDeviceGroupPropertiesKHX *cur_props =
                &pPhysicalDeviceGroupProperties[group];
            for (i = 0; i < cur_props->physicalDeviceCount; i++) {
                for (uint32_t term = 0; term < inst->total_gpu_count; term++) {
                    if (inst->phys_devs_term[term]->phys_dev ==
                        cur_props->physicalDevices[i]) {
                        cur_props->physicalDevices[i] =
                            (VkPhysicalDevice)inst->phys_devs_term[term];
                    }
                }
            }
        }

        if (VK_SUCCESS == res && max_group_count < total_group_count) {
            res = VK_INCOMPLETE;
        }
    }

out:

    return res;
}

// Definitions for the VK_KHX_device_group extension

VKAPI_ATTR void VKAPI_CALL vkGetDeviceGroupPeerMemoryFeaturesKHX(
    VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex,
    uint32_t remoteDeviceIndex,
    VkPeerMemoryFeatureFlagsKHX *pPeerMemoryFeatures) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    disp->GetDeviceGroupPeerMemoryFeaturesKHX(
        device, heapIndex, localDeviceIndex, remoteDeviceIndex,
        pPeerMemoryFeatures);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindBufferMemory2KHX(VkDevice device, uint32_t bindInfoCount,
                       const VkBindBufferMemoryInfoKHX *pBindInfos) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->BindBufferMemory2KHX(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindImageMemory2KHX(VkDevice device, uint32_t bindInfoCount,
                      const VkBindImageMemoryInfoKHX *pBindInfos) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->BindImageMemory2KHX(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMaskKHX(VkCommandBuffer commandBuffer,
                                                 uint32_t deviceMask) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdSetDeviceMaskKHX(commandBuffer, deviceMask);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupPresentCapabilitiesKHX(
    VkDevice device,
    VkDeviceGroupPresentCapabilitiesKHX *pDeviceGroupPresentCapabilities) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetDeviceGroupPresentCapabilitiesKHX(
        device, pDeviceGroupPresentCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupSurfacePresentModesKHX(
    VkDevice device, VkSurfaceKHR surface,
    VkDeviceGroupPresentModeFlagsKHX *pModes) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetDeviceGroupSurfacePresentModesKHX(device, surface, pModes);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_GetDeviceGroupSurfacePresentModesKHX(
    VkDevice device, VkSurfaceKHR surface,
    VkDeviceGroupPresentModeFlagsKHX *pModes) {
    uint32_t icd_index = 0;
    struct loader_device *dev;
    struct loader_icd_term *icd_term =
        loader_get_icd_and_device(device, &dev, &icd_index);
    if (NULL != icd_term) {
        if (NULL == icd_term->GetDeviceGroupSurfacePresentModesKHX) {
            loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                       0,
                       "ICD associated with VkPhysicalDevice does not support "
                       "vkGetDeviceGroupSurfacePresentModesKHX");
        }
        VkIcdSurface *icd_surface = (VkIcdSurface *)(surface);
        if (NULL != icd_surface->real_icd_surfaces) {
            if (NULL != (void *)icd_surface->real_icd_surfaces[icd_index]) {
                return icd_term->GetDeviceGroupSurfacePresentModesKHX(
                    device, icd_surface->real_icd_surfaces[icd_index], pModes);
            }
        }
        return icd_term->GetDeviceGroupSurfacePresentModesKHX(device, surface,
                                                              pModes);
    }
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImage2KHX(
    VkDevice device, const VkAcquireNextImageInfoKHX *pAcquireInfo,
    uint32_t *pImageIndex) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->AcquireNextImage2KHX(device, pAcquireInfo, pImageIndex);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchBaseKHX(
    VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY,
    uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY,
    uint32_t groupCountZ) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdDispatchBaseKHX(commandBuffer, baseGroupX, baseGroupY, baseGroupZ,
                             groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDevicePresentRectanglesKHX(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount,
    VkRect2D *pRects) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->GetPhysicalDevicePresentRectanglesKHX(
        unwrapped_phys_dev, surface, pRectCount, pRects);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_GetPhysicalDevicePresentRectanglesKHX(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount,
    VkRect2D *pRects) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL != icd_term) {
        if (NULL == icd_term->GetPhysicalDevicePresentRectanglesKHX) {
            loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                       0,
                       "ICD associated with VkPhysicalDevice does not support "
                       "vkGetPhysicalDevicePresentRectanglesKHX");
        }
        VkIcdSurface *icd_surface = (VkIcdSurface *)(surface);
        uint8_t icd_index = phys_dev_term->icd_index;
        if (NULL != icd_surface->real_icd_surfaces) {
            if (NULL != (void *)icd_surface->real_icd_surfaces[icd_index]) {
                icd_term->GetPhysicalDevicePresentRectanglesKHX(
                    phys_dev_term->phys_dev,
                    icd_surface->real_icd_surfaces[icd_index], pRectCount,
                    pRects);
            }
        }
        return icd_term->GetPhysicalDevicePresentRectanglesKHX(
            phys_dev_term->phys_dev, surface, pRectCount, pRects);
    }
    return VK_ERROR_INCOMPATIBLE_DRIVER;
}

// Definitions for the VK_KHX_external_memory_capabilities extension

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferPropertiesKHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalBufferInfoKHX *pExternalBufferInfo,
    VkExternalBufferPropertiesKHX *pExternalBufferProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceExternalBufferPropertiesKHX(
        unwrapped_phys_dev, pExternalBufferInfo, pExternalBufferProperties);
}

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceExternalBufferPropertiesKHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalBufferInfoKHX *pExternalBufferInfo,
    VkExternalBufferPropertiesKHX *pExternalBufferProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceExternalBufferPropertiesKHX) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceExternalBufferPropertiesKHX");
    }
    icd_term->GetPhysicalDeviceExternalBufferPropertiesKHX(
        phys_dev_term->phys_dev, pExternalBufferInfo,
        pExternalBufferProperties);
}

VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceProperties2KHX(VkPhysicalDevice physicalDevice,
                                  VkPhysicalDeviceProperties2KHX *pProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceProperties2KHX(unwrapped_phys_dev, pProperties);
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceProperties2KHX(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2KHX *pProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL != icd_term) {
        if (NULL == icd_term->GetPhysicalDeviceProperties2KHX) {
            loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                       0,
                       "ICD associated with VkPhysicalDevice does not support "
                       "vkGetPhysicalDeviceProperties2KHX");
        }
        icd_term->GetPhysicalDeviceProperties2KHX(phys_dev_term->phys_dev,
                                                  pProperties);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2KHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2KHX *pImageFormatInfo,
    VkImageFormatProperties2KHX *pImageFormatProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->GetPhysicalDeviceImageFormatProperties2KHX(
        unwrapped_phys_dev, pImageFormatInfo, pImageFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceImageFormatProperties2KHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2KHX *pImageFormatInfo,
    VkImageFormatProperties2KHX *pImageFormatProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL != icd_term) {
        if (NULL == icd_term->GetPhysicalDeviceImageFormatProperties2KHX) {
            loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                       0,
                       "ICD associated with VkPhysicalDevice does not support "
                       "vkGetPhysicalDeviceImageFormatProperties2KHX");
        }
        return icd_term->GetPhysicalDeviceImageFormatProperties2KHX(
            phys_dev_term->phys_dev, pImageFormatInfo, pImageFormatProperties);
    }
    return VK_ERROR_INCOMPATIBLE_DRIVER;
}

// Definitions for the VK_KHX_external_memory_fd

VKAPI_ATTR VkResult VKAPI_CALL
vkGetMemoryFdKHX(VkDevice device, VkDeviceMemory memory,
                 VkExternalMemoryHandleTypeFlagBitsKHX handleType, int *pFd) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetMemoryFdKHX(device, memory, handleType, pFd);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdPropertiesKHX(
    VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHX handleType, int fd,
    VkMemoryFdPropertiesKHX *pMemoryFdProperties) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetMemoryFdPropertiesKHX(device, handleType, fd,
                                          pMemoryFdProperties);
}

#ifdef VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_KHX_external_memory_win32

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleKHX(
    VkDevice device, VkDeviceMemory memory,
    VkExternalMemoryHandleTypeFlagBitsKHX handleType, HANDLE *pHandle) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetMemoryWin32HandleKHX(device, memory, handleType, pHandle);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandlePropertiesKHX(
    VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHX handleType,
    HANDLE handle,
    VkMemoryWin32HandlePropertiesKHX *pMemoryWin32HandleProperties) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetMemoryWin32HandlePropertiesKHX(
        device, handleType, handle, pMemoryWin32HandleProperties);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_KHX_external_semaphore_capabilities extension

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalSemaphorePropertiesKHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfoKHX *pExternalSemaphoreInfo,
    VkExternalSemaphorePropertiesKHX *pExternalSemaphoreProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceExternalSemaphorePropertiesKHX(
        unwrapped_phys_dev, pExternalSemaphoreInfo,
        pExternalSemaphoreProperties);
}

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceExternalSemaphorePropertiesKHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfoKHX *pExternalSemaphoreInfo,
    VkExternalSemaphorePropertiesKHX *pExternalSemaphoreProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceExternalSemaphorePropertiesKHX) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceExternalSemaphorePropertiesKHX");
    }
    icd_term->GetPhysicalDeviceExternalSemaphorePropertiesKHX(
        phys_dev_term->phys_dev, pExternalSemaphoreInfo,
        pExternalSemaphoreProperties);
}

// Definitions for the VK_KHX_external_semaphore_fd

VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreFdKHX(
    VkDevice device, const VkImportSemaphoreFdInfoKHX *pImportSemaphoreFdInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->ImportSemaphoreFdKHX(device, pImportSemaphoreFdInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreFdKHX(
    VkDevice device, VkSemaphore semaphore,
    VkExternalSemaphoreHandleTypeFlagBitsKHX handleType, int *pFd) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetSemaphoreFdKHX(device, semaphore, handleType, pFd);
}

#ifdef VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_KHX_external_semaphore_win32

VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreWin32HandleKHX(
    VkDevice device, const VkImportSemaphoreWin32HandleInfoKHX
                         *pImportSemaphoreWin32HandleInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->ImportSemaphoreWin32HandleKHX(device,
                                               pImportSemaphoreWin32HandleInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreWin32HandleKHX(
    VkDevice device, VkSemaphore semaphore,
    VkExternalSemaphoreHandleTypeFlagBitsKHX handleType, HANDLE *pHandle) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetSemaphoreWin32HandleKHX(device, semaphore, handleType,
                                            pHandle);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_KHX_push_descriptor extension

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetKHX(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet *pDescriptorWrites) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdPushDescriptorSetKHX(commandBuffer, pipelineBindPoint, layout, set,
                                  descriptorWriteCount, pDescriptorWrites);
}

// Definitions for the VK_EXT_acquire_xlib_display extension

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireXlibDisplayEXT(
    VkPhysicalDevice physicalDevice, Display *dpy, VkDisplayKHR display) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->AcquireXlibDisplayEXT(unwrapped_phys_dev, dpy, display);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_AcquireXlibDisplayEXT(
    VkPhysicalDevice physicalDevice, Display *dpy, VkDisplayKHR display) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->AcquireXlibDisplayEXT) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkAcquireXlibDisplayEXT");
    }
    return icd_term->AcquireXlibDisplayEXT(phys_dev_term->phys_dev, dpy,
                                           display);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkGetRandROutputDisplayEXT(VkPhysicalDevice physicalDevice, Display *dpy,
                           RROutput rrOutput, VkDisplayKHR *pDisplay) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->GetRandROutputDisplayEXT(unwrapped_phys_dev, dpy, rrOutput,
                                          pDisplay);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_GetRandROutputDisplayEXT(
    VkPhysicalDevice physicalDevice, Display *dpy, RROutput rrOutput,
    VkDisplayKHR *pDisplay) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetRandROutputDisplayEXT) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetRandROutputDisplayEXT");
    }
    return icd_term->GetRandROutputDisplayEXT(phys_dev_term->phys_dev, dpy,
                                              rrOutput, pDisplay);
}
#endif /* VK_USE_PLATFORM_XLIB_XRANDR_EXT */

// Definitions for the VK_EXT_debug_marker extension commands which
// need to have a terminator function

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectTagEXT(
    VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    // If this is a physical device, we have to replace it with the proper one
    // for the next call.
    if (pTagInfo->objectType ==
        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
        struct loader_physical_device_tramp *phys_dev_tramp =
            (struct loader_physical_device_tramp *)(uintptr_t)pTagInfo->object;
        pTagInfo->object = (uint64_t)(uintptr_t)phys_dev_tramp->phys_dev;
    }
    return disp->DebugMarkerSetObjectTagEXT(device, pTagInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_DebugMarkerSetObjectTagEXT(
    VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    uint32_t icd_index = 0;
    struct loader_device *dev;
    struct loader_icd_term *icd_term =
        loader_get_icd_and_device(device, &dev, &icd_index);
    if (NULL != icd_term && NULL != icd_term->DebugMarkerSetObjectTagEXT) {
        // If this is a physical device, we have to replace it with the proper
        // one for the next call.
        if (pTagInfo->objectType ==
            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
            struct loader_physical_device_term *phys_dev_term =
                (struct loader_physical_device_term *)(uintptr_t)
                    pTagInfo->object;
            pTagInfo->object = (uint64_t)(uintptr_t)phys_dev_term->phys_dev;

            // If this is a KHR_surface, and the ICD has created its own, we
            // have to replace it with the proper one for the next call.
        } else if (pTagInfo->objectType ==
                   VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT) {
            if (NULL != icd_term && NULL != icd_term->CreateSwapchainKHR) {
                VkIcdSurface *icd_surface =
                    (VkIcdSurface *)(uintptr_t)pTagInfo->object;
                if (NULL != icd_surface->real_icd_surfaces) {
                    pTagInfo->object =
                        (uint64_t)icd_surface->real_icd_surfaces[icd_index];
                }
            }
        }
        return icd_term->DebugMarkerSetObjectTagEXT(device, pTagInfo);
    } else {
        return VK_SUCCESS;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT(
    VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    // If this is a physical device, we have to replace it with the proper one
    // for the next call.
    if (pNameInfo->objectType ==
        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
        struct loader_physical_device_tramp *phys_dev_tramp =
            (struct loader_physical_device_tramp *)(uintptr_t)pNameInfo->object;
        pNameInfo->object = (uint64_t)(uintptr_t)phys_dev_tramp->phys_dev;
    }
    return disp->DebugMarkerSetObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_DebugMarkerSetObjectNameEXT(
    VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    uint32_t icd_index = 0;
    struct loader_device *dev;
    struct loader_icd_term *icd_term =
        loader_get_icd_and_device(device, &dev, &icd_index);
    if (NULL != icd_term && NULL != icd_term->DebugMarkerSetObjectNameEXT) {
        // If this is a physical device, we have to replace it with the proper
        // one for the next call.
        if (pNameInfo->objectType ==
            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
            struct loader_physical_device_term *phys_dev_term =
                (struct loader_physical_device_term *)(uintptr_t)
                    pNameInfo->object;
            pNameInfo->object = (uint64_t)(uintptr_t)phys_dev_term->phys_dev;

            // If this is a KHR_surface, and the ICD has created its own, we
            // have to replace it with the proper one for the next call.
        } else if (pNameInfo->objectType ==
                   VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT) {
            if (NULL != icd_term && NULL != icd_term->CreateSwapchainKHR) {
                VkIcdSurface *icd_surface =
                    (VkIcdSurface *)(uintptr_t)pNameInfo->object;
                if (NULL != icd_surface->real_icd_surfaces) {
                    pNameInfo->object =
                        (uint64_t)(
                            uintptr_t)icd_surface->real_icd_surfaces[icd_index];
                }
            }
        }
        return icd_term->DebugMarkerSetObjectNameEXT(device, pNameInfo);
    } else {
        return VK_SUCCESS;
    }
}

// Definitions for the VK_EXT_direct_mode_display extension

VKAPI_ATTR VkResult VKAPI_CALL
vkReleaseDisplayEXT(VkPhysicalDevice physicalDevice, VkDisplayKHR display) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->ReleaseDisplayEXT(unwrapped_phys_dev, display);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_ReleaseDisplayEXT(
    VkPhysicalDevice physicalDevice, VkDisplayKHR display) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->ReleaseDisplayEXT) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkReleaseDisplayEXT");
    }
    return icd_term->ReleaseDisplayEXT(phys_dev_term->phys_dev, display);
}

// Definitions for the VK_EXT_display_surface_counter extension

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilities2EXT(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VkSurfaceCapabilities2EXT *pSurfaceCapabilities) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->GetPhysicalDeviceSurfaceCapabilities2EXT(
        unwrapped_phys_dev, surface, pSurfaceCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceSurfaceCapabilities2EXT(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VkSurfaceCapabilities2EXT *pSurfaceCapabilities) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL != icd_term) {
        if (NULL == icd_term->GetPhysicalDeviceSurfaceCapabilities2EXT) {
            loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                       0,
                       "ICD associated with VkPhysicalDevice does not support "
                       "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
        }
        VkIcdSurface *icd_surface = (VkIcdSurface *)(surface);
        uint8_t icd_index = phys_dev_term->icd_index;
        if (NULL != icd_surface->real_icd_surfaces) {
            if (NULL != (void *)icd_surface->real_icd_surfaces[icd_index]) {
                return icd_term->GetPhysicalDeviceSurfaceCapabilities2EXT(
                    phys_dev_term->phys_dev,
                    icd_surface->real_icd_surfaces[icd_index],
                    pSurfaceCapabilities);
            }
        }
    }
    return icd_term->GetPhysicalDeviceSurfaceCapabilities2EXT(
        phys_dev_term->phys_dev, surface, pSurfaceCapabilities);
}

// Definitions for the VK_AMD_draw_indirect_count extension

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountAMD(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdDrawIndirectCountAMD(commandBuffer, buffer, offset, countBuffer,
                                  countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountAMD(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdDrawIndexedIndirectCountAMD(commandBuffer, buffer, offset,
                                         countBuffer, countBufferOffset,
                                         maxDrawCount, stride);
}

// Definitions for the VK_NV_external_memory_capabilities extension

VKAPI_ATTR VkResult VKAPI_CALL
vkGetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);

    return disp->GetPhysicalDeviceExternalImageFormatPropertiesNV(
        unwrapped_phys_dev, format, type, tiling, usage, flags,
        externalHandleType, pExternalImageFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;

    if (!icd_term->GetPhysicalDeviceExternalImageFormatPropertiesNV) {
        if (externalHandleType) {
            return VK_ERROR_FORMAT_NOT_SUPPORTED;
        }

        if (!icd_term->GetPhysicalDeviceImageFormatProperties) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        pExternalImageFormatProperties->externalMemoryFeatures = 0;
        pExternalImageFormatProperties->exportFromImportedHandleTypes = 0;
        pExternalImageFormatProperties->compatibleHandleTypes = 0;

        return icd_term->GetPhysicalDeviceImageFormatProperties(
            phys_dev_term->phys_dev, format, type, tiling, usage, flags,
            &pExternalImageFormatProperties->imageFormatProperties);
    }

    return icd_term->GetPhysicalDeviceExternalImageFormatPropertiesNV(
        phys_dev_term->phys_dev, format, type, tiling, usage, flags,
        externalHandleType, pExternalImageFormatProperties);
}

#ifdef VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_NV_external_memory_win32 extension

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleNV(
    VkDevice device, VkDeviceMemory memory,
    VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE *pHandle) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetMemoryWin32HandleNV(device, memory, handleType, pHandle);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_NVX_device_generated_commands

VKAPI_ATTR void VKAPI_CALL vkCmdProcessCommandsNVX(
    VkCommandBuffer commandBuffer,
    const VkCmdProcessCommandsInfoNVX *pProcessCommandsInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdProcessCommandsNVX(commandBuffer, pProcessCommandsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdReserveSpaceForCommandsNVX(
    VkCommandBuffer commandBuffer,
    const VkCmdReserveSpaceForCommandsInfoNVX *pReserveSpaceInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdReserveSpaceForCommandsNVX(commandBuffer, pReserveSpaceInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateIndirectCommandsLayoutNVX(
    VkDevice device, const VkIndirectCommandsLayoutCreateInfoNVX *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkIndirectCommandsLayoutNVX *pIndirectCommandsLayout) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->CreateIndirectCommandsLayoutNVX(
        device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyIndirectCommandsLayoutNVX(
    VkDevice device, VkIndirectCommandsLayoutNVX indirectCommandsLayout,
    const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    disp->DestroyIndirectCommandsLayoutNVX(device, indirectCommandsLayout,
                                           pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateObjectTableNVX(
    VkDevice device, const VkObjectTableCreateInfoNVX *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkObjectTableNVX *pObjectTable) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->CreateObjectTableNVX(device, pCreateInfo, pAllocator,
                                      pObjectTable);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyObjectTableNVX(VkDevice device, VkObjectTableNVX objectTable,
                        const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    disp->DestroyObjectTableNVX(device, objectTable, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkRegisterObjectsNVX(
    VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount,
    const VkObjectTableEntryNVX *const *ppObjectTableEntries,
    const uint32_t *pObjectIndices) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->RegisterObjectsNVX(device, objectTable, objectCount,
                                    ppObjectTableEntries, pObjectIndices);
}

VKAPI_ATTR VkResult VKAPI_CALL vkUnregisterObjectsNVX(
    VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount,
    const VkObjectEntryTypeNVX *pObjectEntryTypes,
    const uint32_t *pObjectIndices) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->UnregisterObjectsNVX(device, objectTable, objectCount,
                                      pObjectEntryTypes, pObjectIndices);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX(
    VkPhysicalDevice physicalDevice,
    VkDeviceGeneratedCommandsFeaturesNVX *pFeatures,
    VkDeviceGeneratedCommandsLimitsNVX *pLimits) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceGeneratedCommandsPropertiesNVX(unwrapped_phys_dev,
                                                          pFeatures, pLimits);
}

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceGeneratedCommandsPropertiesNVX(
    VkPhysicalDevice physicalDevice,
    VkDeviceGeneratedCommandsFeaturesNVX *pFeatures,
    VkDeviceGeneratedCommandsLimitsNVX *pLimits) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceGeneratedCommandsPropertiesNVX) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX");
    }
    icd_term->GetPhysicalDeviceGeneratedCommandsPropertiesNVX(
        phys_dev_term->phys_dev, pFeatures, pLimits);
}

// GPA helpers for extensions

bool extension_instance_gpa(struct loader_instance *ptr_instance,
                            const char *name, void **addr) {
    *addr = NULL;

    // Functions for the VK_KHR_get_physical_device_properties2 extension

    if (!strcmp("vkGetPhysicalDeviceFeatures2KHR", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khr_get_physical_device_properties2 == 1)
                    ? (void *)vkGetPhysicalDeviceFeatures2KHR
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceProperties2KHR", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khr_get_physical_device_properties2 == 1)
                    ? (void *)vkGetPhysicalDeviceProperties2KHR
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceFormatProperties2KHR", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khr_get_physical_device_properties2 == 1)
                    ? (void *)vkGetPhysicalDeviceFormatProperties2KHR
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceImageFormatProperties2KHR", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khr_get_physical_device_properties2 == 1)
                    ? (void *)vkGetPhysicalDeviceImageFormatProperties2KHR
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceQueueFamilyProperties2KHR", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khr_get_physical_device_properties2 == 1)
                    ? (void *)vkGetPhysicalDeviceQueueFamilyProperties2KHR
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceMemoryProperties2KHR", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khr_get_physical_device_properties2 == 1)
                    ? (void *)vkGetPhysicalDeviceMemoryProperties2KHR
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceSparseImageFormatProperties2KHR", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khr_get_physical_device_properties2 == 1)
                    ? (void *)vkGetPhysicalDeviceSparseImageFormatProperties2KHR
                    : NULL;
        return true;
    }

    // Functions for the VK_KHR_maintenance1 extension

    if (!strcmp("vkTrimCommandPoolKHR", name)) {
        *addr = (void *)vkTrimCommandPoolKHR;
        return true;
    }

    // Functions for the VK_KHX_device_group extension

    if (!strcmp("vkGetDeviceGroupPeerMemoryFeaturesKHX", name)) {
        *addr = (void *)vkGetDeviceGroupPeerMemoryFeaturesKHX;
        return true;
    }
    if (!strcmp("vkBindBufferMemory2KHX", name)) {
        *addr = (void *)vkBindBufferMemory2KHX;
        return true;
    }
    if (!strcmp("vkBindImageMemory2KHX", name)) {
        *addr = (void *)vkBindImageMemory2KHX;
        return true;
    }
    if (!strcmp("vkCmdSetDeviceMaskKHX", name)) {
        *addr = (void *)vkCmdSetDeviceMaskKHX;
        return true;
    }
    if (!strcmp("vkGetDeviceGroupPresentCapabilitiesKHX", name)) {
        *addr = (void *)vkGetDeviceGroupPresentCapabilitiesKHX;
        return true;
    }
    if (!strcmp("vkGetDeviceGroupSurfacePresentModesKHX", name)) {
        *addr = (void *)vkGetDeviceGroupSurfacePresentModesKHX;
        return true;
    }
    if (!strcmp("vkAcquireNextImage2KHX", name)) {
        *addr = (void *)vkAcquireNextImage2KHX;
        return true;
    }
    if (!strcmp("vkCmdDispatchBaseKHX", name)) {
        *addr = (void *)vkCmdDispatchBaseKHX;
        return true;
    }
    if (!strcmp("vkGetPhysicalDevicePresentRectanglesKHX", name)) {
        *addr = (void *)vkGetPhysicalDevicePresentRectanglesKHX;
        return true;
    }

    // Functions for the VK_KHX_device_group_creation extension

    if (!strcmp("vkEnumeratePhysicalDeviceGroupsKHX", name)) {
        *addr =
            (ptr_instance->enabled_known_extensions.khx_device_group_creation ==
             1)
                ? (void *)vkEnumeratePhysicalDeviceGroupsKHX
                : NULL;
        return true;
    }

    // Functions for the VK_KHX_external_memory_capabilities extension

    if (!strcmp("vkGetPhysicalDeviceExternalBufferPropertiesKHX", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khx_external_memory_capabilities == 1)
                    ? (void *)vkGetPhysicalDeviceExternalBufferPropertiesKHX
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceProperties2KHX", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khx_external_memory_capabilities == 1)
                    ? (void *)vkGetPhysicalDeviceProperties2KHX
                    : NULL;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceImageFormatProperties2KHX", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khx_external_memory_capabilities == 1)
                    ? (void *)vkGetPhysicalDeviceImageFormatProperties2KHX
                    : NULL;
        return true;
    }

    // Functions for the KHX_external_memory_fd extension

    if (!strcmp("vkGetMemoryFdKHX", name)) {
        *addr = (void *)vkGetMemoryFdKHX;
        return true;
    }
    if (!strcmp("vkGetMemoryFdPropertiesKHX", name)) {
        *addr = (void *)vkGetMemoryFdPropertiesKHX;
        return true;
    }

// Functions for the KHX_external_memory_win32 extension

#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (!strcmp("vkGetMemoryWin32HandleKHX", name)) {
        *addr = (void *)vkGetMemoryWin32HandleKHX;
        return true;
    }
    if (!strcmp("vkGetMemoryWin32HandlePropertiesKHX", name)) {
        *addr = (void *)vkGetMemoryWin32HandlePropertiesKHX;
        return true;
    }
#endif

    // Functions for the VK_KHX_external_semaphore_capabilities extension

    if (!strcmp("vkGetPhysicalDeviceExternalSemaphorePropertiesKHX", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .khx_external_semaphore_capabilities == 1)
                    ? (void *)vkGetPhysicalDeviceExternalSemaphorePropertiesKHX
                    : NULL;
        return true;
    }

    // Functions for the KHX_external_semaphore_fd extension

    if (!strcmp("vkImportSemaphoreFdKHX", name)) {
        *addr = (void *)vkImportSemaphoreFdKHX;
        return true;
    }
    if (!strcmp("vkGetSemaphoreFdKHX", name)) {
        *addr = (void *)vkGetSemaphoreFdKHX;
        return true;
    }

// Functions for the KHX_external_semaphore_win32 extension

#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (!strcmp("vkImportSemaphoreWin32HandleKHX", name)) {
        *addr = (void *)vkImportSemaphoreWin32HandleKHX;
        return true;
    }
    if (!strcmp("vkGetSemaphoreWin32HandleKHX", name)) {
        *addr = (void *)vkGetSemaphoreWin32HandleKHX;
        return true;
    }
#endif

    // Functions for the VK_KHX_push_descriptor extension

    if (!strcmp("vkCmdPushDescriptorSetKHX", name)) {
        *addr = (void *)vkCmdPushDescriptorSetKHX;
        return true;
    }

// Functions for the VK_EXT_acquire_xlib_display extension

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    if (!strcmp("vkAcquireXlibDisplayEXT", name)) {
        *addr =
            (ptr_instance->enabled_known_extensions.ext_acquire_xlib_display ==
             1)
                ? (void *)vkAcquireXlibDisplayEXT
                : NULL;
        return true;
    }

    if (!strcmp("vkGetRandROutputDisplayEXT", name)) {
        *addr =
            (ptr_instance->enabled_known_extensions.ext_acquire_xlib_display ==
             1)
                ? (void *)vkGetRandROutputDisplayEXT
                : NULL;
        return true;
    }

#endif // VK_USE_PLATFORM_XLIB_XRANDR_EXT

    // Definitions for the VK_EXT_debug_marker extension commands which
    // need to have a terminator function.  Since these are device
    // commands, we always need to return a valid value for them.

    if (!strcmp("vkDebugMarkerSetObjectTagEXT", name)) {
        *addr = (void *)vkDebugMarkerSetObjectTagEXT;
        return true;
    }
    if (!strcmp("vkDebugMarkerSetObjectNameEXT", name)) {
        *addr = (void *)vkDebugMarkerSetObjectNameEXT;
        return true;
    }

    // Functions for the VK_EXT_direct_mode_display extension

    if (!strcmp("vkReleaseDisplayEXT", name)) {
        *addr =
            (ptr_instance->enabled_known_extensions.ext_direct_mode_display ==
             1)
                ? (void *)vkReleaseDisplayEXT
                : NULL;
        return true;
    }

    // Functions for the VK_EXT_display_surface_counter extension

    if (!strcmp("vkGetPhysicalDeviceSurfaceCapabilities2EXT", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .ext_display_surface_counter == 1)
                    ? (void *)vkGetPhysicalDeviceSurfaceCapabilities2EXT
                    : NULL;
        return true;
    }

    // Functions for the VK_AMD_draw_indirect_count extension

    if (!strcmp("vkCmdDrawIndirectCountAMD", name)) {
        *addr = (void *)vkCmdDrawIndirectCountAMD;
        return true;
    }
    if (!strcmp("vkCmdDrawIndexedIndirectCountAMD", name)) {
        *addr = (void *)vkCmdDrawIndexedIndirectCountAMD;
        return true;
    }
    // Functions for the VK_NV_external_memory_capabilities extension

    if (!strcmp("vkGetPhysicalDeviceExternalImageFormatPropertiesNV", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .nv_external_memory_capabilities == 1)
                    ? (void *)vkGetPhysicalDeviceExternalImageFormatPropertiesNV
                    : NULL;
        return true;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR

    // Functions for the VK_NV_external_memory_win32 extension
    if (!strcmp("vkGetMemoryWin32HandleNV", name)) {
        *addr = (void *)vkGetMemoryWin32HandleNV;
        return true;
    }

#endif // VK_USE_PLATFORM_WIN32_KHR

    // Functions for the VK_NVX_device_generated_commands extension

    if (!strcmp("vkCmdProcessCommandsNVX", name)) {
        *addr = (void *)vkCmdProcessCommandsNVX;
        return true;
    }
    if (!strcmp("vkCmdReserveSpaceForCommandsNVX", name)) {
        *addr = (void *)vkCmdReserveSpaceForCommandsNVX;
        return true;
    }
    if (!strcmp("vkCreateIndirectCommandsLayoutNVX", name)) {
        *addr = (void *)vkCreateIndirectCommandsLayoutNVX;
        return true;
    }
    if (!strcmp("vkDestroyIndirectCommandsLayoutNVX", name)) {
        *addr = (void *)vkDestroyIndirectCommandsLayoutNVX;
        return true;
    }
    if (!strcmp("vkCreateObjectTableNVX", name)) {
        *addr = (void *)vkCreateObjectTableNVX;
        return true;
    }
    if (!strcmp("vkDestroyObjectTableNVX", name)) {
        *addr = (void *)vkDestroyObjectTableNVX;
        return true;
    }
    if (!strcmp("vkRegisterObjectsNVX", name)) {
        *addr = (void *)vkRegisterObjectsNVX;
        return true;
    }
    if (!strcmp("vkUnregisterObjectsNVX", name)) {
        *addr = (void *)vkUnregisterObjectsNVX;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX", name)) {
        *addr = (void *)vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX;
        return true;
    }

    return false;
}

void extensions_create_instance(struct loader_instance *ptr_instance,
                                const VkInstanceCreateInfo *pCreateInfo) {
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (0 ==
            strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                   VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions
                .khr_get_physical_device_properties2 = 1;
            continue;
        } else if (0 ==
                   strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                          VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions
                .nv_external_memory_capabilities = 1;
            continue;
        } else if (0 == strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                               VK_KHX_DEVICE_GROUP_CREATION_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions.khx_device_group_creation =
                1;
            continue;
        } else if (0 ==
                   strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                          VK_KHX_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions
                .khx_external_memory_capabilities = 1;
            continue;
        } else if (0 ==
                   strcmp(
                       pCreateInfo->ppEnabledExtensionNames[i],
                       VK_KHX_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions
                .khx_external_semaphore_capabilities = 1;
            continue;
#ifdef VK_USE_PLATFORM_XLIB_KHR
        } else if (0 == strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                               VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions.ext_acquire_xlib_display = 1;
            continue;
#endif
        } else if (0 == strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                               VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions.ext_direct_mode_display = 1;
            continue;
        } else if (0 == strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                               VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME)) {
            ptr_instance->enabled_known_extensions.ext_display_surface_counter =
                1;
            continue;
        }
    }
}
