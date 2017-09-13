/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
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
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */
#ifndef CORE_VALIDATION_TYPES_H_
#define CORE_VALIDATION_TYPES_H_

#include "hash_vk_types.h"
#include "vk_safe_struct.h"
#include "vulkan/vulkan.h"
#include "vk_validation_error_messages.h"
#include "vk_layer_logging.h"
#include "vk_object_types.h"
#include "vk_extension_helper.h"
#include <atomic>
#include <functional>
#include <map>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <list>

// Fwd declarations -- including descriptor_set.h creates an ugly include loop
namespace cvdescriptorset {
class DescriptorSetLayoutDef;
class DescriptorSetLayout;
class DescriptorSet;
}  // namespace cvdescriptorset

struct GLOBAL_CB_NODE;

enum CALL_STATE {
    UNCALLED,       // Function has not been called
    QUERY_COUNT,    // Function called once to query a count
    QUERY_DETAILS,  // Function called w/ a count to query details
};

class BASE_NODE {
   public:
    // Track when object is being used by an in-flight command buffer
    std::atomic_int in_use;
    // Track command buffers that this object is bound to
    //  binding initialized when cmd referencing object is bound to command buffer
    //  binding removed when command buffer is reset or destroyed
    // When an object is destroyed, any bound cbs are set to INVALID
    std::unordered_set<GLOBAL_CB_NODE *> cb_bindings;

    BASE_NODE() { in_use.store(0); };
};

// Track command pools and their command buffers
struct COMMAND_POOL_NODE : public BASE_NODE {
    VkCommandPoolCreateFlags createFlags;
    uint32_t queueFamilyIndex;
    // Cmd buffers allocated from this pool
    std::unordered_set<VkCommandBuffer> commandBuffers;
};

// Utilities for barriers and the commmand pool
template <typename Barrier>
static bool IsTransferOp(const Barrier *barrier) {
    return barrier->srcQueueFamilyIndex != barrier->dstQueueFamilyIndex;
}

template <typename Barrier, bool assume_transfer = false>
static bool IsReleaseOp(const COMMAND_POOL_NODE *pool, const Barrier *barrier) {
    return (assume_transfer || IsTransferOp(barrier)) && (pool->queueFamilyIndex == barrier->srcQueueFamilyIndex);
}

template <typename Barrier, bool assume_transfer = false>
static bool IsAcquireOp(const COMMAND_POOL_NODE *pool, const Barrier *barrier) {
    return (assume_transfer || IsTransferOp(barrier)) && (pool->queueFamilyIndex == barrier->dstQueueFamilyIndex);
}

// Generic wrapper for vulkan objects
struct VK_OBJECT {
    uint64_t handle;
    VulkanObjectType type;
};

inline bool operator==(VK_OBJECT a, VK_OBJECT b) NOEXCEPT { return a.handle == b.handle && a.type == b.type; }

namespace std {
template <>
struct hash<VK_OBJECT> {
    size_t operator()(VK_OBJECT obj) const NOEXCEPT { return hash<uint64_t>()(obj.handle) ^ hash<uint32_t>()(obj.type); }
};
}  // namespace std

class PHYS_DEV_PROPERTIES_NODE {
   public:
    VkPhysicalDeviceProperties properties;
    std::vector<VkQueueFamilyProperties> queue_family_properties;
};

// Flags describing requirements imposed by the pipeline on a descriptor. These
// can't be checked at pipeline creation time as they depend on the Image or
// ImageView bound.
enum descriptor_req {
    DESCRIPTOR_REQ_VIEW_TYPE_1D = 1 << VK_IMAGE_VIEW_TYPE_1D,
    DESCRIPTOR_REQ_VIEW_TYPE_1D_ARRAY = 1 << VK_IMAGE_VIEW_TYPE_1D_ARRAY,
    DESCRIPTOR_REQ_VIEW_TYPE_2D = 1 << VK_IMAGE_VIEW_TYPE_2D,
    DESCRIPTOR_REQ_VIEW_TYPE_2D_ARRAY = 1 << VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    DESCRIPTOR_REQ_VIEW_TYPE_3D = 1 << VK_IMAGE_VIEW_TYPE_3D,
    DESCRIPTOR_REQ_VIEW_TYPE_CUBE = 1 << VK_IMAGE_VIEW_TYPE_CUBE,
    DESCRIPTOR_REQ_VIEW_TYPE_CUBE_ARRAY = 1 << VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,

    DESCRIPTOR_REQ_ALL_VIEW_TYPE_BITS = (1 << (VK_IMAGE_VIEW_TYPE_END_RANGE + 1)) - 1,

    DESCRIPTOR_REQ_SINGLE_SAMPLE = 2 << VK_IMAGE_VIEW_TYPE_END_RANGE,
    DESCRIPTOR_REQ_MULTI_SAMPLE = DESCRIPTOR_REQ_SINGLE_SAMPLE << 1,
};

struct DESCRIPTOR_POOL_STATE : BASE_NODE {
    VkDescriptorPool pool;
    uint32_t maxSets;        // Max descriptor sets allowed in this pool
    uint32_t availableSets;  // Available descriptor sets in this pool

    safe_VkDescriptorPoolCreateInfo createInfo;
    std::unordered_set<cvdescriptorset::DescriptorSet *> sets;  // Collection of all sets in this pool
    std::vector<uint32_t> maxDescriptorTypeCount;               // Max # of descriptors of each type in this pool
    std::vector<uint32_t> availableDescriptorTypeCount;         // Available # of descriptors of each type in this pool

    DESCRIPTOR_POOL_STATE(const VkDescriptorPool pool, const VkDescriptorPoolCreateInfo *pCreateInfo)
        : pool(pool),
          maxSets(pCreateInfo->maxSets),
          availableSets(pCreateInfo->maxSets),
          createInfo(pCreateInfo),
          maxDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE, 0),
          availableDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE, 0) {
        // Collect maximums per descriptor type.
        for (uint32_t i = 0; i < createInfo.poolSizeCount; ++i) {
            uint32_t typeIndex = static_cast<uint32_t>(createInfo.pPoolSizes[i].type);
            // Same descriptor types can appear several times
            maxDescriptorTypeCount[typeIndex] += createInfo.pPoolSizes[i].descriptorCount;
            availableDescriptorTypeCount[typeIndex] = maxDescriptorTypeCount[typeIndex];
        }
    }
};

// Generic memory binding struct to track objects bound to objects
struct MEM_BINDING {
    VkDeviceMemory mem;
    VkDeviceSize offset;
    VkDeviceSize size;
};

inline bool operator==(MEM_BINDING a, MEM_BINDING b) NOEXCEPT { return a.mem == b.mem && a.offset == b.offset && a.size == b.size; }

namespace std {
template <>
struct hash<MEM_BINDING> {
    size_t operator()(MEM_BINDING mb) const NOEXCEPT {
        auto intermediate = hash<uint64_t>()(reinterpret_cast<uint64_t &>(mb.mem)) ^ hash<uint64_t>()(mb.offset);
        return intermediate ^ hash<uint64_t>()(mb.size);
    }
};
}  // namespace std

// Superclass for bindable object state (currently images and buffers)
class BINDABLE : public BASE_NODE {
   public:
    bool sparse;  // Is this object being bound with sparse memory or not?
    // Non-sparse binding data
    MEM_BINDING binding;
    // Memory requirements for this BINDABLE
    VkMemoryRequirements requirements;
    // bool to track if memory requirements were checked
    bool memory_requirements_checked;
    // Sparse binding data, initially just tracking MEM_BINDING per mem object
    //  There's more data for sparse bindings so need better long-term solution
    // TODO : Need to update solution to track all sparse binding data
    std::unordered_set<MEM_BINDING> sparse_bindings;

    std::unordered_set<VkDeviceMemory> bound_memory_set_;

    BINDABLE()
        : sparse(false), binding{}, requirements{}, memory_requirements_checked(false), sparse_bindings{}, bound_memory_set_{} {};

    // Update the cached set of memory bindings.
    // Code that changes binding.mem or sparse_bindings must call UpdateBoundMemorySet()
    void UpdateBoundMemorySet() {
        bound_memory_set_.clear();
        if (!sparse) {
            bound_memory_set_.insert(binding.mem);
        } else {
            for (auto sb : sparse_bindings) {
                bound_memory_set_.insert(sb.mem);
            }
        }
    }

    // Return unordered set of memory objects that are bound
    // Instead of creating a set from scratch each query, return the cached one
    const std::unordered_set<VkDeviceMemory> &GetBoundMemory() const { return bound_memory_set_; }
};

class BUFFER_STATE : public BINDABLE {
   public:
    VkBuffer buffer;
    VkBufferCreateInfo createInfo;
    BUFFER_STATE(VkBuffer buff, const VkBufferCreateInfo *pCreateInfo) : buffer(buff), createInfo(*pCreateInfo) {
        if ((createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) && (createInfo.queueFamilyIndexCount > 0)) {
            uint32_t *pQueueFamilyIndices = new uint32_t[createInfo.queueFamilyIndexCount];
            for (uint32_t i = 0; i < createInfo.queueFamilyIndexCount; i++) {
                pQueueFamilyIndices[i] = pCreateInfo->pQueueFamilyIndices[i];
            }
            createInfo.pQueueFamilyIndices = pQueueFamilyIndices;
        }

        if (createInfo.flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT) {
            sparse = true;
        }
    };

    BUFFER_STATE(BUFFER_STATE const &rh_obj) = delete;

    ~BUFFER_STATE() {
        if ((createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) && (createInfo.queueFamilyIndexCount > 0)) {
            delete[] createInfo.pQueueFamilyIndices;
            createInfo.pQueueFamilyIndices = nullptr;
        }
    };
};

class BUFFER_VIEW_STATE : public BASE_NODE {
   public:
    VkBufferView buffer_view;
    VkBufferViewCreateInfo create_info;
    BUFFER_VIEW_STATE(VkBufferView bv, const VkBufferViewCreateInfo *ci) : buffer_view(bv), create_info(*ci){};
    BUFFER_VIEW_STATE(const BUFFER_VIEW_STATE &rh_obj) = delete;
};

struct SAMPLER_STATE : public BASE_NODE {
    VkSampler sampler;
    VkSamplerCreateInfo createInfo;

    SAMPLER_STATE(const VkSampler *ps, const VkSamplerCreateInfo *pci) : sampler(*ps), createInfo(*pci){};
};

class IMAGE_STATE : public BINDABLE {
   public:
    VkImage image;
    VkImageCreateInfo createInfo;
    bool valid;                   // If this is a swapchain image backing memory track valid here as it doesn't have DEVICE_MEM_INFO
    bool acquired;                // If this is a swapchain image, has it been acquired by the app.
    bool shared_presentable;      // True for a front-buffered swapchain image
    bool layout_locked;           // A front-buffered image that has been presented can never have layout transitioned
    bool get_sparse_reqs_called;  // Track if GetImageSparseMemoryRequirements() has been called for this image
    bool sparse_metadata_required;  // Track if sparse metadata aspect is required for this image
    bool sparse_metadata_bound;     // Track if sparse metadata aspect is bound to this image
    std::vector<VkSparseImageMemoryRequirements> sparse_requirements;
    IMAGE_STATE(VkImage img, const VkImageCreateInfo *pCreateInfo)
        : image(img),
          createInfo(*pCreateInfo),
          valid(false),
          acquired(false),
          shared_presentable(false),
          layout_locked(false),
          get_sparse_reqs_called(false),
          sparse_metadata_required(false),
          sparse_metadata_bound(false),
          sparse_requirements{} {
        if ((createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) && (createInfo.queueFamilyIndexCount > 0)) {
            uint32_t *pQueueFamilyIndices = new uint32_t[createInfo.queueFamilyIndexCount];
            for (uint32_t i = 0; i < createInfo.queueFamilyIndexCount; i++) {
                pQueueFamilyIndices[i] = pCreateInfo->pQueueFamilyIndices[i];
            }
            createInfo.pQueueFamilyIndices = pQueueFamilyIndices;
        }

        if (createInfo.flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) {
            sparse = true;
        }
    };

    IMAGE_STATE(IMAGE_STATE const &rh_obj) = delete;

    ~IMAGE_STATE() {
        if ((createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) && (createInfo.queueFamilyIndexCount > 0)) {
            delete[] createInfo.pQueueFamilyIndices;
            createInfo.pQueueFamilyIndices = nullptr;
        }
    };
};

class IMAGE_VIEW_STATE : public BASE_NODE {
   public:
    VkImageView image_view;
    VkImageViewCreateInfo create_info;
    IMAGE_VIEW_STATE(VkImageView iv, const VkImageViewCreateInfo *ci) : image_view(iv), create_info(*ci){};
    IMAGE_VIEW_STATE(const IMAGE_VIEW_STATE &rh_obj) = delete;
};

struct MemRange {
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct MEMORY_RANGE {
    uint64_t handle;
    bool image;   // True for image, false for buffer
    bool linear;  // True for buffers and linear images
    bool valid;   // True if this range is know to be valid
    VkDeviceMemory memory;
    VkDeviceSize start;
    VkDeviceSize size;
    VkDeviceSize end;  // Store this pre-computed for simplicity
    // Set of ptrs to every range aliased with this one
    std::unordered_set<MEMORY_RANGE *> aliases;
};

// Data struct for tracking memory object
struct DEVICE_MEM_INFO : public BASE_NODE {
    void *object;       // Dispatchable object used to create this memory (device of swapchain)
    bool global_valid;  // If allocation is mapped or external, set to "true" to be picked up by subsequently bound ranges
    VkDeviceMemory mem;
    VkMemoryAllocateInfo alloc_info;
    bool is_dedicated;
    VkBuffer dedicated_buffer;
    VkImage dedicated_image;
    std::unordered_set<VK_OBJECT> obj_bindings;               // objects bound to this memory
    std::unordered_map<uint64_t, MEMORY_RANGE> bound_ranges;  // Map of object to its binding range
    // Convenience vectors image/buff handles to speed up iterating over images or buffers independently
    std::unordered_set<uint64_t> bound_images;
    std::unordered_set<uint64_t> bound_buffers;

    MemRange mem_range;
    void *shadow_copy_base;    // Base of layer's allocation for guard band, data, and alignment space
    void *shadow_copy;         // Pointer to start of guard-band data before mapped region
    uint64_t shadow_pad_size;  // Size of the guard-band data before and after actual data. It MUST be a
                               // multiple of limits.minMemoryMapAlignment
    void *p_driver_data;       // Pointer to application's actual memory

    DEVICE_MEM_INFO(void *disp_object, const VkDeviceMemory in_mem, const VkMemoryAllocateInfo *p_alloc_info)
        : object(disp_object),
          global_valid(false),
          mem(in_mem),
          alloc_info(*p_alloc_info),
          is_dedicated(false),
          dedicated_buffer(VK_NULL_HANDLE),
          dedicated_image(VK_NULL_HANDLE),
          mem_range{},
          shadow_copy_base(0),
          shadow_copy(0),
          shadow_pad_size(0),
          p_driver_data(0){};
};

class SWAPCHAIN_NODE {
   public:
    safe_VkSwapchainCreateInfoKHR createInfo;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    bool replaced = false;
    bool shared_presentable = false;
    CALL_STATE vkGetSwapchainImagesKHRState = UNCALLED;
    uint32_t get_swapchain_image_count = 0;
    SWAPCHAIN_NODE(const VkSwapchainCreateInfoKHR *pCreateInfo, VkSwapchainKHR swapchain)
        : createInfo(pCreateInfo), swapchain(swapchain) {}
};

class IMAGE_CMD_BUF_LAYOUT_NODE {
   public:
    IMAGE_CMD_BUF_LAYOUT_NODE() = default;
    IMAGE_CMD_BUF_LAYOUT_NODE(VkImageLayout initialLayoutInput, VkImageLayout layoutInput)
        : initialLayout(initialLayoutInput), layout(layoutInput) {}

    VkImageLayout initialLayout;
    VkImageLayout layout;
};

// Store the DAG.
struct DAGNode {
    uint32_t pass;
    std::vector<uint32_t> prev;
    std::vector<uint32_t> next;
};

struct RENDER_PASS_STATE : public BASE_NODE {
    VkRenderPass renderPass;
    safe_VkRenderPassCreateInfo createInfo;
    std::vector<bool> hasSelfDependency;
    std::vector<DAGNode> subpassToNode;
    std::vector<int32_t> subpass_to_dependency_index;  // srcSubpass to dependency index of self dep, or -1 if none
    std::unordered_map<uint32_t, bool> attachment_first_read;

    RENDER_PASS_STATE(VkRenderPassCreateInfo const *pCreateInfo) : createInfo(pCreateInfo) {}
};

// vkCmd tracking -- complete as of header 1.0.68
// please keep in "none, then sorted" order
// Note: grepping vulkan.h for VKAPI_CALL.*vkCmd will return all functions except vkEndCommandBuffer

enum CMD_TYPE {
    CMD_NONE,
    CMD_BEGINQUERY,
    CMD_BEGINRENDERPASS,
    CMD_BINDDESCRIPTORSETS,
    CMD_BINDINDEXBUFFER,
    CMD_BINDPIPELINE,
    CMD_BINDVERTEXBUFFERS,
    CMD_BLITIMAGE,
    CMD_CLEARATTACHMENTS,
    CMD_CLEARCOLORIMAGE,
    CMD_CLEARDEPTHSTENCILIMAGE,
    CMD_COPYBUFFER,
    CMD_COPYBUFFERTOIMAGE,
    CMD_COPYIMAGE,
    CMD_COPYIMAGETOBUFFER,
    CMD_COPYQUERYPOOLRESULTS,
    CMD_DEBUGMARKERBEGINEXT,
    CMD_DEBUGMARKERENDEXT,
    CMD_DEBUGMARKERINSERTEXT,
    CMD_DISPATCH,
    CMD_DISPATCHBASEKHX,
    CMD_DISPATCHINDIRECT,
    CMD_DRAW,
    CMD_DRAWINDEXED,
    CMD_DRAWINDEXEDINDIRECT,
    CMD_DRAWINDEXEDINDIRECTCOUNTAMD,
    CMD_DRAWINDIRECT,
    CMD_DRAWINDIRECTCOUNTAMD,
    CMD_ENDCOMMANDBUFFER,  // Should be the last command in any RECORDED cmd buffer
    CMD_ENDQUERY,
    CMD_ENDRENDERPASS,
    CMD_EXECUTECOMMANDS,
    CMD_FILLBUFFER,
    CMD_INITATOMICCOUNTERS,
    CMD_LOADATOMICCOUNTERS,
    CMD_NEXTSUBPASS,
    CMD_PIPELINEBARRIER,
    CMD_PROCESSCOMMANDSNVX,
    CMD_PUSHCONSTANTS,
    CMD_PUSHDESCRIPTORSETKHR,
    CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR,
    CMD_RESERVESPACEFORCOMMANDSNVX,
    CMD_RESETEVENT,
    CMD_RESETQUERYPOOL,
    CMD_RESOLVEIMAGE,
    CMD_SAVEATOMICCOUNTERS,
    CMD_SETBLENDCONSTANTS,
    CMD_SETDEPTHBIAS,
    CMD_SETDEPTHBOUNDS,
    CMD_SETDEVICEMASKKHX,
    CMD_SETDISCARDRECTANGLEEXT,
    CMD_SETEVENT,
    CMD_SETLINEWIDTH,
    CMD_SETSAMPLELOCATIONSEXT,
    CMD_SETSCISSOR,
    CMD_SETSTENCILCOMPAREMASK,
    CMD_SETSTENCILREFERENCE,
    CMD_SETSTENCILWRITEMASK,
    CMD_SETVIEWPORT,
    CMD_SETVIEWPORTWSCALINGNV,
    CMD_UPDATEBUFFER,
    CMD_WAITEVENTS,
    CMD_WRITETIMESTAMP,
    CMD_COUNT
};

// Struct for mapping a cmd to its permanent default pipe/access flags
// Draw/Dispatch/ClearAttachments commands will also add in some flags dynamically based on current state
struct CmdFlags {
    VkPipelineStageFlags stage_flags;
    VkAccessFlags access_flags;
};

static const uint32_t READ_INDEX = 0;
static const uint32_t WRITE_INDEX = 1;

// Per-cmd read/write flags Read flags in slot 0, Write in 1
constexpr CmdFlags CommandToFlags[CMD_COUNT + 1][2] = {
    //    CMD_NONE,
    {{0, 0}, {0, 0}},
    //    CMD_BEGINQUERY,
    {{0, 0}, {0, 0}},
    //    CMD_BEGINRENDERPASS,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}, {0, 0}},
    //    CMD_BINDDESCRIPTORSETS,
    {{0, 0}, {0, 0}},
    //    CMD_BINDINDEXBUFFER,
    {{0, 0}, {0, 0}},
    //    CMD_BINDPIPELINE,
    {{0, 0}, {0, 0}},
    //    CMD_BINDVERTEXBUFFERS,
    {{0, 0}, {0, 0}},
    //    CMD_BLITIMAGE,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_CLEARATTACHMENTS,
    {{0, 0}, {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}},
    //    CMD_CLEARCOLORIMAGE,
    {{0, 0},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_CLEARDEPTHSTENCILIMAGE,
    {{0, 0},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_COPYBUFFER,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_COPYBUFFERTOIMAGE,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_COPYIMAGE,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_COPYIMAGETOBUFFER,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_COPYQUERYPOOLRESULTS,
    {{0, 0},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_DEBUGMARKERBEGINEXT,
    {{0, 0}, {0, 0}},
    //    CMD_DEBUGMARKERENDEXT,
    {{0, 0}, {0, 0}},
    //    CMD_DEBUGMARKERINSERTEXT,
    {{0, 0}, {0, 0}},
    //    CMD_DISPATCH,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0}},
    //    CMD_DISPATCHBASEKHX,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0}},
    //    CMD_DISPATCHINDIRECT,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0}},
    //    CMD_DRAW,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}},
    //    CMD_DRAWINDEXED,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}},
    //    CMD_DRAWINDEXEDINDIRECT,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}},
    //    CMD_DRAWINDEXEDINDIRECTCOUNTAMD,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}},
    //    CMD_DRAWINDIRECT,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
          VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}},
    //    CMD_DRAWINDIRECTCOUNTAMD, TODO : Make sure these are right
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
          VK_ACCESS_SHADER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}},
    //    CMD_ENDCOMMANDBUFFER,  // Should be last command in any RECORDED cmd buffer
    {{0, 0}, {0, 0}},
    //    CMD_ENDQUERY,
    {{0, 0}, {0, 0}},
    //    CMD_ENDRENDERPASS,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}, {0, 0}},
    //    CMD_EXECUTECOMMANDS,
    {{0, 0}, {0, 0}},
    //    CMD_FILLBUFFER,
    {{0, 0},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_INITATOMICCOUNTERS,
    {{0, 0}, {0, 0}},
    //    CMD_LOADATOMICCOUNTERS,
    {{0, 0}, {0, 0}},
    //    CMD_NEXTSUBPASS,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0}, {0, 0}},
    //    CMD_PIPELINEBARRIER,
    {{0, 0}, {0, 0}},
    //    CMD_PROCESSCOMMANDSNVX, TODO : Update these flags
    {{0, 0}, {0, 0}},
    //    CMD_PUSHCONSTANTS,
    {{0, 0}, {0, 0}},
    //    CMD_PUSHDESCRIPTORSETKHR,
    {{0, 0}, {0, 0}},
    //    CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR,
    {{0, 0}, {0, 0}},
    //    CMD_RESERVESPACEFORCOMMANDSNVX,
    {{0, 0}, {0, 0}},
    //    CMD_RESETEVENT,
    {{0, 0}, {0, 0}},
    //    CMD_RESETQUERYPOOL,
    {{0, 0}, {0, 0}},
    //    CMD_RESOLVEIMAGE,
    {{VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_SAVEATOMICCOUNTERS,
    {{0, 0}, {0, 0}},
    //    CMD_SETBLENDCONSTANTS,
    {{0, 0}, {0, 0}},
    //    CMD_SETDEPTHBIAS,
    {{0, 0}, {0, 0}},
    //    CMD_SETDEPTHBOUNDS,
    {{0, 0}, {0, 0}},
    //    CMD_SETDEVICEMASKKHX,
    {{0, 0}, {0, 0}},
    //    CMD_SETDISCARDRECTANGLEEXT,
    {{0, 0}, {0, 0}},
    //    CMD_SETEVENT,
    {{0, 0}, {0, 0}},
    //    CMD_SETLINEWIDTH,
    {{0, 0}, {0, 0}},
    //    CMD_SETSAMPLELOCATIONSEXT,
    {{0, 0}, {0, 0}},
    //    CMD_SETSCISSOR,
    {{0, 0}, {0, 0}},
    //    CMD_SETSTENCILCOMPAREMASK,
    {{0, 0}, {0, 0}},
    //    CMD_SETSTENCILREFERENCE,
    {{0, 0}, {0, 0}},
    //    CMD_SETSTENCILWRITEMASK,
    {{0, 0}, {0, 0}},
    //    CMD_SETVIEWPORT,
    {{0, 0}, {0, 0}},
    //    CMD_SETVIEWPORTWSCALINGNV,
    {{0, 0}, {0, 0}},
    //    CMD_UPDATEBUFFER,
    {{0, 0},
     {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT}},
    //    CMD_WAITEVENTS,
    {{0, 0}, {0, 0}},
    //    CMD_WRITETIMESTAMP,
    {{0, 0}, {0, 0}},
    // MAGIC NUMBER ENTRY TO MAKE SURE ARRAY IS CORRECT SIZE
    {{0, 0}, {0, 0xDEADBEEF}},
};

static_assert((CommandToFlags[CMD_WRITETIMESTAMP + 1][WRITE_INDEX].access_flags) == (0xDEADBEEF),
              "CommandToFlags is not correct size. Must have an entry for each CMD_TYPE.");

// fwd decl class for ptr below
class Command;
// Store details of memory access by a cmd
struct MemoryAccess {
    MemoryAccess()
        : precise(false),
          write(false),
          cmd(nullptr),
          location{0, 0, 0},
          src_stage_flags(0),
          src_access_flags(0),
          dst_stage_flags(0),
          dst_access_flags(0),
          mem_barrier(false),
          pipe_barrier(false),
          synch_commands{} {};
    MemoryAccess(VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, bool precise)
        : precise(precise),
          write(false),
          cmd(nullptr),
          location{mem, offset, size},
          src_stage_flags(0),
          src_access_flags(0),
          dst_stage_flags(0),
          dst_access_flags(0),
          mem_barrier(false),
          pipe_barrier(false),
          synch_commands{} {};

    // Once an appropriate barrier has been identified for an access, it will be Visible
    bool Visible() const {
        auto visible = (mem_barrier || (pipe_barrier && write));
        return visible;
    };
    bool precise;          // True if exact bounds of mem access are known, false if bounds are conservative
    bool write;            // False for read access
    Command *cmd;          // Ptr to cmd that makes this access
    MEM_BINDING location;  // mem/offset/size being accessed
    // The source flags cover all access bits for the original memory access
    //  These flags are checked against synch objects to check if scope covers this access
    VkPipelineStageFlags src_stage_flags;  // Stage flags within scope of this access
    VkAccessFlags src_access_flags;        // Access flags within scope of this access
    // The following members are only set when a valid synch object is added following this access
    //  When such an event occurs, the relevant dest scope flags are recorded here
    VkPipelineStageFlags dst_stage_flags;  // Stage flags within scope of this access
    VkAccessFlags dst_access_flags;        // Access flags within scope of this access
    // These bool shortcuts indicate if memory and/or pipeline barriers have occurred since this access
    bool mem_barrier;   // True after relevant mem barrier added to CB following this access
    bool pipe_barrier;  // True after relevant mem barrier added to CB following this access
    // A vector of all synch commands that affect this memory access
    std::vector<Command *> synch_commands;  // Relevent synch commands
};

class Command {
   public:
    Command(CMD_TYPE type, size_t seq, GLOBAL_CB_NODE *gcb) : type(type), seq(seq), cb_state(gcb), replay(false), synch(false){};
    Command(CMD_TYPE type, size_t seq, GLOBAL_CB_NODE *gcb, bool synch)
        : type(type), seq(seq), cb_state(gcb), replay(false), synch(synch){};
    virtual ~Command() {}
    void AddMemoryAccess(MemoryAccess access) { mem_accesses.push_back(access); };
    void SetSeq(size_t seq_num) { seq = seq_num; };
    CMD_TYPE type;
    size_t seq;  // seq # of cmd in this cmd buffer
    GLOBAL_CB_NODE *cb_state;
    bool replay;                             // Track if cmd has been replayed during QueueSubmit
    bool synch;
    std::vector<MemoryAccess> mem_accesses;  // vector of all mem accesses by this cmd
};

class SynchCommand : public Command {
   public:
    SynchCommand(CMD_TYPE type, size_t seq, GLOBAL_CB_NODE *gcb, VkPipelineStageFlags src_stage_flags,
                 VkPipelineStageFlags dst_stage_flags)
        : Command(type, seq, gcb, true), src_stage_flags(src_stage_flags), dst_stage_flags(dst_stage_flags){};
    VkPipelineStageFlags src_stage_flags;
    VkPipelineStageFlags dst_stage_flags;
    std::vector<VkMemoryBarrier> global_barriers;
    std::vector<VkBufferMemoryBarrier> buffer_barriers;
    std::vector<VkImageMemoryBarrier> image_barriers;
};

enum CB_STATE {
    CB_NEW,                 // Newly created CB w/o any cmds
    CB_RECORDING,           // BeginCB has been called on this CB
    CB_RECORDED,            // EndCB has been called on this CB
    CB_INVALID_COMPLETE,    // had a complete recording, but was since invalidated
    CB_INVALID_INCOMPLETE,  // fouled before recording was completed
};

// CB Status -- used to track status of various bindings on cmd buffer objects
typedef VkFlags CBStatusFlags;
enum CBStatusFlagBits {
    // clang-format off
    CBSTATUS_NONE                   = 0x00000000,   // No status is set
    CBSTATUS_LINE_WIDTH_SET         = 0x00000001,   // Line width has been set
    CBSTATUS_DEPTH_BIAS_SET         = 0x00000002,   // Depth bias has been set
    CBSTATUS_BLEND_CONSTANTS_SET    = 0x00000004,   // Blend constants state has been set
    CBSTATUS_DEPTH_BOUNDS_SET       = 0x00000008,   // Depth bounds state object has been set
    CBSTATUS_STENCIL_READ_MASK_SET  = 0x00000010,   // Stencil read mask has been set
    CBSTATUS_STENCIL_WRITE_MASK_SET = 0x00000020,   // Stencil write mask has been set
    CBSTATUS_STENCIL_REFERENCE_SET  = 0x00000040,   // Stencil reference has been set
    CBSTATUS_VIEWPORT_SET           = 0x00000080,
    CBSTATUS_SCISSOR_SET            = 0x00000100,
    CBSTATUS_INDEX_BUFFER_BOUND     = 0x00000200,   // Index buffer has been set
    CBSTATUS_ALL_STATE_SET          = 0x000001FF,   // All state set (intentionally exclude index buffer)
    // clang-format on
};

struct TEMPLATE_STATE {
    VkDescriptorUpdateTemplateKHR desc_update_template;
    safe_VkDescriptorUpdateTemplateCreateInfo create_info;

    TEMPLATE_STATE(VkDescriptorUpdateTemplateKHR update_template, safe_VkDescriptorUpdateTemplateCreateInfo *pCreateInfo)
        : desc_update_template(update_template), create_info(*pCreateInfo) {}
};

struct QueryObject {
    VkQueryPool pool;
    uint32_t index;
};

inline bool operator==(const QueryObject &query1, const QueryObject &query2) {
    return (query1.pool == query2.pool && query1.index == query2.index);
}

namespace std {
template <>
struct hash<QueryObject> {
    size_t operator()(QueryObject query) const throw() {
        return hash<uint64_t>()((uint64_t)(query.pool)) ^ hash<uint32_t>()(query.index);
    }
};
}  // namespace std
struct DRAW_DATA {
    std::vector<VkBuffer> buffers;
};

struct ImageSubresourcePair {
    VkImage image;
    bool hasSubresource;
    VkImageSubresource subresource;
};

inline bool operator==(const ImageSubresourcePair &img1, const ImageSubresourcePair &img2) {
    if (img1.image != img2.image || img1.hasSubresource != img2.hasSubresource) return false;
    return !img1.hasSubresource ||
           (img1.subresource.aspectMask == img2.subresource.aspectMask && img1.subresource.mipLevel == img2.subresource.mipLevel &&
            img1.subresource.arrayLayer == img2.subresource.arrayLayer);
}

namespace std {
template <>
struct hash<ImageSubresourcePair> {
    size_t operator()(ImageSubresourcePair img) const throw() {
        size_t hashVal = hash<uint64_t>()(reinterpret_cast<uint64_t &>(img.image));
        hashVal ^= hash<bool>()(img.hasSubresource);
        if (img.hasSubresource) {
            hashVal ^= hash<uint32_t>()(reinterpret_cast<uint32_t &>(img.subresource.aspectMask));
            hashVal ^= hash<uint32_t>()(img.subresource.mipLevel);
            hashVal ^= hash<uint32_t>()(img.subresource.arrayLayer);
        }
        return hashVal;
    }
};
}  // namespace std

// Canonical dictionary for PushConstantRanges
using PushConstantRangesDict = hash_util::Dictionary<PushConstantRanges>;
using PushConstantRangesId = PushConstantRangesDict::Id;

// Canonical dictionary for the pipeline layout's layout of descriptorsetlayouts
using DescriptorSetLayoutDef = cvdescriptorset::DescriptorSetLayoutDef;
using DescriptorSetLayoutId = std::shared_ptr<const DescriptorSetLayoutDef>;
using PipelineLayoutSetLayoutsDef = std::vector<DescriptorSetLayoutId>;
using PipelineLayoutSetLayoutsDict =
    hash_util::Dictionary<PipelineLayoutSetLayoutsDef, hash_util::IsOrderedContainer<PipelineLayoutSetLayoutsDef>>;
using PipelineLayoutSetLayoutsId = PipelineLayoutSetLayoutsDict::Id;

// Defines/stores a compatibility defintion for set N
// The "layout layout" must store at least set+1 entries, but only the first set+1 are considered for hash and equality testing
// Note: the "cannonical" data are referenced by Id, not including handle or device specific state
// Note: hash and equality only consider layout_id entries [0, set] for determining uniqueness
struct PipelineLayoutCompatDef {
    uint32_t set;
    PushConstantRangesId push_constant_ranges;
    PipelineLayoutSetLayoutsId set_layouts_id;
    PipelineLayoutCompatDef(const uint32_t set_index, const PushConstantRangesId pcr_id, const PipelineLayoutSetLayoutsId sl_id)
        : set(set_index), push_constant_ranges(pcr_id), set_layouts_id(sl_id) {}
    size_t hash() const;
    bool operator==(const PipelineLayoutCompatDef &other) const;
};

// Canonical dictionary for PipelineLayoutCompat records
using PipelineLayoutCompatDict = hash_util::Dictionary<PipelineLayoutCompatDef, hash_util::HasHashMember<PipelineLayoutCompatDef>>;
using PipelineLayoutCompatId = PipelineLayoutCompatDict::Id;

// Store layouts and pushconstants for PipelineLayout
struct PIPELINE_LAYOUT_NODE {
    VkPipelineLayout layout;
    std::vector<std::shared_ptr<cvdescriptorset::DescriptorSetLayout const>> set_layouts;
    PushConstantRangesId push_constant_ranges;
    std::vector<PipelineLayoutCompatId> compat_for_set;

    PIPELINE_LAYOUT_NODE() : layout(VK_NULL_HANDLE), set_layouts{}, push_constant_ranges{}, compat_for_set{} {}

    void reset() {
        layout = VK_NULL_HANDLE;
        set_layouts.clear();
        push_constant_ranges.reset();
        compat_for_set.clear();
    }
};

class PIPELINE_STATE : public BASE_NODE {
   public:
    VkPipeline pipeline;
    safe_VkGraphicsPipelineCreateInfo graphicsPipelineCI;
    // Hold shared ptr to RP in case RP itself is destroyed
    std::shared_ptr<RENDER_PASS_STATE> rp_state;
    safe_VkComputePipelineCreateInfo computePipelineCI;
    // Flag of which shader stages are active for this pipeline
    uint32_t active_shaders;
    uint32_t duplicate_shaders;
    // Capture which slots (set#->bindings) are actually used by the shaders of this pipeline
    std::unordered_map<uint32_t, std::map<uint32_t, descriptor_req>> active_slots;
    // Vtx input info (if any)
    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<VkPipelineColorBlendAttachmentState> attachments;
    bool blendConstantsEnabled;  // Blend constants enabled for any attachments
    PIPELINE_LAYOUT_NODE pipeline_layout;
    VkPrimitiveTopology topology_at_rasterizer;

    // Default constructor
    PIPELINE_STATE()
        : pipeline{},
          graphicsPipelineCI{},
          rp_state(nullptr),
          computePipelineCI{},
          active_shaders(0),
          duplicate_shaders(0),
          active_slots(),
          vertexBindingDescriptions(),
          attachments(),
          blendConstantsEnabled(false),
          pipeline_layout(),
          topology_at_rasterizer{} {}

    void initGraphicsPipeline(const VkGraphicsPipelineCreateInfo *pCreateInfo, std::shared_ptr<RENDER_PASS_STATE> &&rpstate) {
        bool uses_color_attachment = false;
        bool uses_depthstencil_attachment = false;
        if (pCreateInfo->subpass < rpstate->createInfo.subpassCount) {
            const auto &subpass = rpstate->createInfo.pSubpasses[pCreateInfo->subpass];

            for (uint32_t i = 0; i < subpass.colorAttachmentCount; ++i) {
                if (subpass.pColorAttachments[i].attachment != VK_ATTACHMENT_UNUSED) {
                    uses_color_attachment = true;
                    break;
                }
            }

            if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                uses_depthstencil_attachment = true;
            }
        }
        graphicsPipelineCI.initialize(pCreateInfo, uses_color_attachment, uses_depthstencil_attachment);
        // Make sure compute pipeline is null
        VkComputePipelineCreateInfo emptyComputeCI = {};
        computePipelineCI.initialize(&emptyComputeCI);
        for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
            const VkPipelineShaderStageCreateInfo *pPSSCI = &pCreateInfo->pStages[i];
            this->duplicate_shaders |= this->active_shaders & pPSSCI->stage;
            this->active_shaders |= pPSSCI->stage;
        }
        if (graphicsPipelineCI.pVertexInputState) {
            const auto pVICI = graphicsPipelineCI.pVertexInputState;
            if (pVICI->vertexBindingDescriptionCount) {
                this->vertexBindingDescriptions = std::vector<VkVertexInputBindingDescription>(
                    pVICI->pVertexBindingDescriptions, pVICI->pVertexBindingDescriptions + pVICI->vertexBindingDescriptionCount);
            }
        }
        if (graphicsPipelineCI.pColorBlendState) {
            const auto pCBCI = graphicsPipelineCI.pColorBlendState;
            if (pCBCI->attachmentCount) {
                this->attachments = std::vector<VkPipelineColorBlendAttachmentState>(pCBCI->pAttachments,
                                                                                     pCBCI->pAttachments + pCBCI->attachmentCount);
            }
        }
        if (graphicsPipelineCI.pInputAssemblyState) {
            topology_at_rasterizer = graphicsPipelineCI.pInputAssemblyState->topology;
        }
        rp_state = rpstate;
    }

    void initComputePipeline(const VkComputePipelineCreateInfo *pCreateInfo) {
        computePipelineCI.initialize(pCreateInfo);
        // Make sure gfx pipeline is null
        VkGraphicsPipelineCreateInfo emptyGraphicsCI = {};
        graphicsPipelineCI.initialize(&emptyGraphicsCI, false, false);
        switch (computePipelineCI.stage.stage) {
            case VK_SHADER_STAGE_COMPUTE_BIT:
                this->active_shaders |= VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                // TODO : Flag error
                break;
        }
    }
};

// Track last states that are bound per pipeline bind point (Gfx & Compute)
struct LAST_BOUND_STATE {
    PIPELINE_STATE *pipeline_state;
    VkPipelineLayout pipeline_layout;
    // Track each set that has been bound
    // Ordered bound set tracking where index is set# that given set is bound to
    std::vector<cvdescriptorset::DescriptorSet *> boundDescriptorSets;
    std::unique_ptr<cvdescriptorset::DescriptorSet> push_descriptor_set;
    // one dynamic offset per dynamic descriptor bound to this CB
    std::vector<std::vector<uint32_t>> dynamicOffsets;
    std::vector<PipelineLayoutCompatId> compat_id_for_set;

    void reset() {
        pipeline_state = nullptr;
        pipeline_layout = VK_NULL_HANDLE;
        boundDescriptorSets.clear();
        push_descriptor_set = nullptr;
        dynamicOffsets.clear();
    }
};

// Struct container to group sets of r/w buffers & images & r/w mem access maps
struct MemAccessGroup {
    std::unordered_set<VkImageView> read_images;
    std::unordered_set<VkBuffer> read_buffers;
    std::unordered_set<VkImageView> write_images;
    std::unordered_set<VkBuffer> write_buffers;
    std::vector<std::unordered_map<VkDeviceMemory, std::vector<MemoryAccess>>> access_maps;
    MemAccessGroup() { access_maps.resize(2); }
    void reset() {
        read_images.clear();
        read_buffers.clear();
        write_images.clear();
        write_buffers.clear();
        access_maps.clear();
        access_maps.resize(2);
    }
};

// Cmd Buffer Wrapper Struct - TODO : This desperately needs its own class
struct GLOBAL_CB_NODE : public BASE_NODE {
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo createInfo = {};
    VkCommandBufferBeginInfo beginInfo;
    VkCommandBufferInheritanceInfo inheritanceInfo;
    VkDevice device;  // device this CB belongs to
    bool hasDrawCmd;
    CB_STATE state;        // Track cmd buffer update state
    uint64_t submitCount;  // Number of times CB has been submitted
    typedef uint64_t ImageLayoutUpdateCount;
    ImageLayoutUpdateCount image_layout_change_count;  // The sequence number for changes to image layout (for cached validation)
    CBStatusFlags status;                              // Track status of various bindings on cmd buffer
    CBStatusFlags static_status;                       // All state bits provided by current graphics pipeline
                                                       // rather than dynamic state
    // Currently storing "lastBound" objects on per-CB basis
    //  long-term may want to create caches of "lastBound" states and could have
    //  each individual CMD_NODE referencing its own "lastBound" state
    // Store last bound state for Gfx & Compute pipeline bind points
    LAST_BOUND_STATE lastBound[VK_PIPELINE_BIND_POINT_RANGE_SIZE];

    uint32_t viewportMask;
    uint32_t scissorMask;
    VkRenderPassBeginInfo activeRenderPassBeginInfo;
    RENDER_PASS_STATE *activeRenderPass;
    VkSubpassContents activeSubpassContents;
    uint32_t activeSubpass;
    VkFramebuffer activeFramebuffer;
    std::unordered_set<VkFramebuffer> framebuffers;
    // Unified data structs to track objects bound to this command buffer as well as object
    //  dependencies that have been broken : either destroyed objects, or updated descriptor sets
    std::unordered_set<VK_OBJECT> object_bindings;
    std::vector<VK_OBJECT> broken_bindings;
    //
    std::vector<std::unique_ptr<Command>> commands;  // Commands in this command buffer
    std::vector<SynchCommand *> synch_commands;      // Synch Commands in this command buffer
    // Track all mem_accesses by this CB at the point of a draw
    MemAccessGroup mem_accesses;

    std::unordered_set<VkEvent> waitedEvents;
    std::vector<VkEvent> writeEventsBeforeWait;
    std::vector<VkEvent> events;
    std::unordered_map<QueryObject, std::unordered_set<VkEvent>> waitedEventsBeforeQueryReset;
    std::unordered_map<QueryObject, bool> queryToStateMap;  // 0 is unavailable, 1 is available
    std::unordered_set<QueryObject> activeQueries;
    std::unordered_set<QueryObject> startedQueries;
    std::unordered_map<ImageSubresourcePair, IMAGE_CMD_BUF_LAYOUT_NODE> imageLayoutMap;
    std::unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
    std::vector<DRAW_DATA> drawData;
    DRAW_DATA currentDrawData;
    bool vertex_buffer_used;  // Track for perf warning to make sure any bound vtx buffer used
    VkCommandBuffer primaryCommandBuffer;
    // If primary, the secondary command buffers we will call.
    // If secondary, the primary command buffers we will be called by.
    std::unordered_set<GLOBAL_CB_NODE *> linkedCommandBuffers;
    // Validation functions run at primary CB queue submit time
    std::vector<std::function<bool()>> queue_submit_functions;
    // Validation functions run when secondary CB is executed in primary
    std::vector<std::function<bool(GLOBAL_CB_NODE *, VkFramebuffer)>> cmd_execute_commands_functions;
    std::unordered_set<VkDeviceMemory> memObjs;
    std::vector<std::function<bool(VkQueue)>> eventUpdates;
    std::vector<std::function<bool(VkQueue)>> queryUpdates;
    std::unordered_set<cvdescriptorset::DescriptorSet *> validated_descriptor_sets;
};

struct SEMAPHORE_WAIT {
    VkSemaphore semaphore;
    VkQueue queue;
    uint64_t seq;
};

struct CB_SUBMISSION {
    CB_SUBMISSION(std::vector<VkCommandBuffer> const &cbs, std::vector<SEMAPHORE_WAIT> const &waitSemaphores,
                  std::vector<VkSemaphore> const &signalSemaphores, std::vector<VkSemaphore> const &externalSemaphores,
                  VkFence fence)
        : cbs(cbs),
          waitSemaphores(waitSemaphores),
          signalSemaphores(signalSemaphores),
          externalSemaphores(externalSemaphores),
          fence(fence) {}

    std::vector<VkCommandBuffer> cbs;
    std::vector<SEMAPHORE_WAIT> waitSemaphores;
    std::vector<VkSemaphore> signalSemaphores;
    std::vector<VkSemaphore> externalSemaphores;
    VkFence fence;
};

struct IMAGE_LAYOUT_NODE {
    VkImageLayout layout;
    VkFormat format;
};

// CHECK_DISABLED struct is a container for bools that can block validation checks from being performed.
// The end goal is to have all checks guarded by a bool. The bools are all "false" by default meaning that all checks
// are enabled. At CreateInstance time, the user can use the VK_EXT_validation_flags extension to pass in enum values
// of VkValidationCheckEXT that will selectively disable checks.
struct CHECK_DISABLED {
    bool command_buffer_state;
    bool create_descriptor_set_layout;
    bool destroy_buffer_view;       // Skip validation at DestroyBufferView time
    bool destroy_image_view;        // Skip validation at DestroyImageView time
    bool destroy_pipeline;          // Skip validation at DestroyPipeline time
    bool destroy_descriptor_pool;   // Skip validation at DestroyDescriptorPool time
    bool destroy_framebuffer;       // Skip validation at DestroyFramebuffer time
    bool destroy_renderpass;        // Skip validation at DestroyRenderpass time
    bool destroy_image;             // Skip validation at DestroyImage time
    bool destroy_sampler;           // Skip validation at DestroySampler time
    bool destroy_command_pool;      // Skip validation at DestroyCommandPool time
    bool destroy_event;             // Skip validation at DestroyEvent time
    bool free_memory;               // Skip validation at FreeMemory time
    bool object_in_use;             // Skip all object in_use checking
    bool idle_descriptor_set;       // Skip check to verify that descriptor set is no in-use
    bool push_constant_range;       // Skip push constant range checks
    bool free_descriptor_sets;      // Skip validation prior to vkFreeDescriptorSets()
    bool allocate_descriptor_sets;  // Skip validation prior to vkAllocateDescriptorSets()
    bool update_descriptor_sets;    // Skip validation prior to vkUpdateDescriptorSets()
    bool wait_for_fences;
    bool get_fence_state;
    bool queue_wait_idle;
    bool device_wait_idle;
    bool destroy_fence;
    bool destroy_semaphore;
    bool destroy_query_pool;
    bool get_query_pool_results;
    bool destroy_buffer;
    bool shader_validation;  // Skip validation for shaders

    void SetAll(bool value) { std::fill(&command_buffer_state, &shader_validation + 1, value); }
};

struct MT_FB_ATTACHMENT_INFO {
    IMAGE_VIEW_STATE *view_state;
    VkImage image;
};

class FRAMEBUFFER_STATE : public BASE_NODE {
   public:
    VkFramebuffer framebuffer;
    safe_VkFramebufferCreateInfo createInfo;
    std::shared_ptr<RENDER_PASS_STATE> rp_state;
    std::vector<MT_FB_ATTACHMENT_INFO> attachments;
    FRAMEBUFFER_STATE(VkFramebuffer fb, const VkFramebufferCreateInfo *pCreateInfo, std::shared_ptr<RENDER_PASS_STATE> &&rpstate)
        : framebuffer(fb), createInfo(pCreateInfo), rp_state(rpstate){};
};

struct shader_module;
struct DeviceExtensions;

// Fwd declarations of layer_data and helpers to look-up/validate state from layer_data maps
namespace core_validation {
struct layer_data;
cvdescriptorset::DescriptorSet *GetSetNode(const layer_data *, VkDescriptorSet);
std::shared_ptr<cvdescriptorset::DescriptorSetLayout const> const GetDescriptorSetLayout(layer_data const *, VkDescriptorSetLayout);
DESCRIPTOR_POOL_STATE *GetDescriptorPoolState(const layer_data *, const VkDescriptorPool);
BUFFER_STATE *GetBufferState(const layer_data *, VkBuffer);
IMAGE_STATE *GetImageState(const layer_data *, VkImage);
DEVICE_MEM_INFO *GetMemObjInfo(const layer_data *, VkDeviceMemory);
BUFFER_VIEW_STATE *GetBufferViewState(const layer_data *, VkBufferView);
SAMPLER_STATE *GetSamplerState(const layer_data *, VkSampler);
IMAGE_VIEW_STATE *GetImageViewState(const layer_data *, VkImageView);
SWAPCHAIN_NODE *GetSwapchainNode(const layer_data *, VkSwapchainKHR);
GLOBAL_CB_NODE *GetCBNode(layer_data const *my_data, const VkCommandBuffer cb);
RENDER_PASS_STATE *GetRenderPassState(layer_data const *dev_data, VkRenderPass renderpass);
std::shared_ptr<RENDER_PASS_STATE> GetRenderPassStateSharedPtr(layer_data const *dev_data, VkRenderPass renderpass);
FRAMEBUFFER_STATE *GetFramebufferState(const layer_data *my_data, VkFramebuffer framebuffer);
COMMAND_POOL_NODE *GetCommandPoolNode(layer_data *dev_data, VkCommandPool pool);
shader_module const *GetShaderModuleState(layer_data const *dev_data, VkShaderModule module);
const PHYS_DEV_PROPERTIES_NODE *GetPhysDevProperties(const layer_data *device_data);
const VkPhysicalDeviceFeatures *GetEnabledFeatures(const layer_data *device_data);
const DeviceExtensions *GetEnabledExtensions(const layer_data *device_data);

void invalidateCommandBuffers(const layer_data *, std::unordered_set<GLOBAL_CB_NODE *> const &, VK_OBJECT);
bool ValidateMemoryIsBoundToBuffer(const layer_data *, const BUFFER_STATE *, const char *, UNIQUE_VALIDATION_ERROR_CODE);
bool ValidateMemoryIsBoundToImage(const layer_data *, const IMAGE_STATE *, const char *, UNIQUE_VALIDATION_ERROR_CODE);
void AddCommandBufferBindingSampler(GLOBAL_CB_NODE *, SAMPLER_STATE *);
void AddCommandBufferBindingImage(const layer_data *, GLOBAL_CB_NODE *, IMAGE_STATE *);
void AddCommandBufferBindingImageView(const layer_data *, GLOBAL_CB_NODE *, IMAGE_VIEW_STATE *);
void AddCommandBufferBindingBuffer(const layer_data *, GLOBAL_CB_NODE *, BUFFER_STATE *);
void AddCommandBufferBindingBufferView(const layer_data *, GLOBAL_CB_NODE *, BUFFER_VIEW_STATE *);
bool ValidateObjectNotInUse(const layer_data *dev_data, BASE_NODE *obj_node, VK_OBJECT obj_struct, const char *caller_name,
                            UNIQUE_VALIDATION_ERROR_CODE error_code);
void invalidateCommandBuffers(const layer_data *dev_data, std::unordered_set<GLOBAL_CB_NODE *> const &cb_nodes, VK_OBJECT obj);
void RemoveImageMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info);
void RemoveBufferMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info);
bool ClearMemoryObjectBindings(layer_data *dev_data, uint64_t handle, VulkanObjectType type);
bool ValidateCmdQueueFlags(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const char *caller_name, VkQueueFlags flags,
                           UNIQUE_VALIDATION_ERROR_CODE error_code);
bool ValidateCmd(layer_data *my_data, const GLOBAL_CB_NODE *pCB, const CMD_TYPE cmd, const char *caller_name);
bool insideRenderPass(const layer_data *my_data, const GLOBAL_CB_NODE *pCB, const char *apiName,
                      UNIQUE_VALIDATION_ERROR_CODE msgCode);
void SetImageMemoryValid(layer_data *dev_data, IMAGE_STATE *image_state, bool valid);
bool outsideRenderPass(const layer_data *my_data, GLOBAL_CB_NODE *pCB, const char *apiName, UNIQUE_VALIDATION_ERROR_CODE msgCode);
void SetLayout(GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair, const IMAGE_CMD_BUF_LAYOUT_NODE &node);
void SetLayout(GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair, const VkImageLayout &layout);
bool ValidateImageMemoryIsValid(layer_data *dev_data, IMAGE_STATE *image_state, const char *functionName);
bool ValidateImageSampleCount(layer_data *dev_data, IMAGE_STATE *image_state, VkSampleCountFlagBits sample_count,
                              const char *location, UNIQUE_VALIDATION_ERROR_CODE msgCode);
bool rangesIntersect(layer_data const *dev_data, MEMORY_RANGE const *range1, VkDeviceSize offset, VkDeviceSize end);
bool ValidateBufferMemoryIsValid(layer_data *dev_data, BUFFER_STATE *buffer_state, const char *functionName);
void SetBufferMemoryValid(layer_data *dev_data, BUFFER_STATE *buffer_state, bool valid);
bool ValidateCmdSubpassState(const layer_data *dev_data, const GLOBAL_CB_NODE *pCB, const CMD_TYPE cmd_type);
bool ValidateCmd(layer_data *dev_data, const GLOBAL_CB_NODE *cb_state, const CMD_TYPE cmd, const char *caller_name);

// Prototypes for layer_data accessor functions.  These should be in their own header file at some point
VkFormatProperties GetFormatProperties(core_validation::layer_data *device_data, VkFormat format);
VkResult GetImageFormatProperties(core_validation::layer_data *device_data, const VkImageCreateInfo *image_ci,
                                  VkImageFormatProperties *image_format_properties);
const debug_report_data *GetReportData(const layer_data *);
const VkPhysicalDeviceProperties *GetPhysicalDeviceProperties(layer_data *);
const CHECK_DISABLED *GetDisables(layer_data *);
std::unordered_map<VkImage, std::unique_ptr<IMAGE_STATE>> *GetImageMap(core_validation::layer_data *);
std::unordered_map<VkImage, std::vector<ImageSubresourcePair>> *GetImageSubresourceMap(layer_data *);
std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> *GetImageLayoutMap(layer_data *);
std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> const *GetImageLayoutMap(layer_data const *);
std::unordered_map<VkBuffer, std::unique_ptr<BUFFER_STATE>> *GetBufferMap(layer_data *device_data);
std::unordered_map<VkBufferView, std::unique_ptr<BUFFER_VIEW_STATE>> *GetBufferViewMap(layer_data *device_data);
std::unordered_map<VkImageView, std::unique_ptr<IMAGE_VIEW_STATE>> *GetImageViewMap(layer_data *device_data);
const DeviceExtensions *GetDeviceExtensions(const layer_data *);
}  // namespace core_validation

#endif  // CORE_VALIDATION_TYPES_H_
