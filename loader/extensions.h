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
 *
 */

#include "vk_loader_platform.h"
#include "loader.h"

bool extension_instance_gpa(struct loader_instance *ptr_instance,
                            const char *name, void **addr);

void extensions_create_instance(struct loader_instance *ptr_instance,
                                const VkInstanceCreateInfo *pCreateInfo);

// Instance extension terminators
VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFeatures2KHR(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR *pFeatures);
VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceProperties2KHR(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2KHR *pProperties);
VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFormatProperties2KHR(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties2KHR *pFormatProperties);
VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2KHR *pImageFormatInfo,
    VkImageFormatProperties2KHR *pImageFormatProperties);
VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceQueueFamilyProperties2KHR(
    VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2KHR *pQueueFamilyProperties);
VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceMemoryProperties2KHR(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2KHR *pMemoryProperties);
VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceSparseImageFormatProperties2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSparseImageFormatInfo2KHR *pFormatInfo,
    uint32_t *pPropertyCount, VkSparseImageFormatProperties2KHR *pProperties);

// VK_NV_external_memory_capabilities extension
VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties);
