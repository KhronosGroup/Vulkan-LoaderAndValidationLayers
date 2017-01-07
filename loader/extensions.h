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

// Instance extension terminators for the VK_KHX_external_memory_capabilities
// extension

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceExternalBufferPropertiesKHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalBufferInfoKHX *pExternalBufferInfo,
    VkExternalBufferPropertiesKHX *pExternalBufferProperties);

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceProperties2KHX(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2KHX *pProperties);

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceImageFormatProperties2KHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2KHX *pImageFormatInfo,
    VkImageFormatProperties2KHX *pImageFormatProperties);

// Instance extension terminators for the VK_KHX_external_semaphore_capabilities
// extension

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceExternalSemaphorePropertiesKHX(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfoKHX *pExternalSemaphoreInfo,
    VkExternalSemaphorePropertiesKHX *pExternalSemaphoreProperties);

// Instance extension terminators for the VK_EXT_display_surface_counter
// extension

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceSurfaceCapabilities2EXT(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VkSurfaceCapabilities2EXT *pSurfaceCapabilities);

// Device extension terminators for the VK_NV_external_memory_capabilities
// extension

VKAPI_ATTR VkResult VKAPI_CALL terminator_DebugMarkerSetObjectTagEXT(
    VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo);

VKAPI_ATTR VkResult VKAPI_CALL terminator_DebugMarkerSetObjectNameEXT(
    VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo);

// Instance extension terminators for the VK_NV_external_memory_capabilities
// extension

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties);

// Instance extension terminators for the VK_NVX_device_generated_commands
// extension
VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceGeneratedCommandsPropertiesNVX(
    VkPhysicalDevice physicalDevice,
    VkDeviceGeneratedCommandsFeaturesNVX *pFeatures,
    VkDeviceGeneratedCommandsLimitsNVX *pLimits);
