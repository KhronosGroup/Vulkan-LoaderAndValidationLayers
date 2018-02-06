//
// File: vk_standard_validation.h
//
/*
 * Copyright (c) 2017 The Khronos Group Inc.
 * Copyright (c) 2017 Valve Corporation
 * Copyright (c) 2017 LunarG, Inc.
 * Copyright (c) 2017 Google, Inc.
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
 */

#pragma once

/*
 * vk_standard_validation.h
 *
 * This is a header file to simplify using the standard set of validation layers across any platform.
 * It defines the array "vkval_standard_validation_array" for C code and "vkval::standard_validation_array" for C++ code
 * that can be used when creating a VkInstance in order to enable the standard validation layer stack.
 *
 * In C code VkInstanceCreateInfo struct you can use the values, for example, as:
 *
 *   VkInstanceCreateInfo instance_create_info;
 *   instance_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
 *   instance_create_info.ppEnabledLayerNames = standard_validation_array;
 *
 * C++ code exists under the "vkval" namespace. The array can therefore be used as:
 * vkval::VALIDATION_LAYER_COUNT and vkval::standard_validation_array.
 * An example of using this code in C++ is:
 *
 *   VkInstanceCreateInfo instance_create_info = {};
 *   instance_create_info.enabledLayerCount = vkval::VALIDATION_LAYER_COUNT;
 *   instance_create_info.ppEnabledLayerNames = vkval::standard_validation_array;
 */

// Use #define so that layer stack can be changed in a single location
#define VK_STANDARD_VALIDATION_LAYERS                                                                      \
    "VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_parameter_validation", "VK_LAYER_LUNARG_object_tracker", \
        "VK_LAYER_LUNARG_core_validation", "VK_LAYER_GOOGLE_unique_objects"

#ifdef __cplusplus
namespace vkval {
static char const *const standard_validation_array[] = {VK_STANDARD_VALIDATION_LAYERS};
static const uint32_t VALIDATION_LAYER_COUNT = sizeof(standard_validation_array) / sizeof(standard_validation_array[0]);
}  // namespace vkval
#else
static char const *const vkval_standard_validation_array[] = {VK_STANDARD_VALIDATION_LAYERS};
static const uint32_t VALIDATION_LAYER_COUNT = sizeof(vkval_standard_validation_array) / sizeof(vkval_standard_validation_array[0]);
#endif
