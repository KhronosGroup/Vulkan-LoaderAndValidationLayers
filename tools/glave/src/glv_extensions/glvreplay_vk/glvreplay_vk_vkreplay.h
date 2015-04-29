/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include <xcb/xcb.h>

#endif
#include "glvreplay_window.h"
#include "glvreplay_factory.h"
#include "glv_trace_packet_identifiers.h"

extern "C" {
#include "glv_vk_vk_structs.h"
#include "glv_vk_vk_wsi_lunarg_structs.h"
}

#include "vulkan.h"
#include "vkDbg.h"
#include "vk_wsi_lunarg.h"
#include "glave_snapshot.h"
#include "glvreplay_vk_vkdisplay.h"
#include "glvreplay_vk_func_ptrs.h"
#include "glvreplay_vk_objmapper.h"

#define CHECK_RETURN_VALUE(entrypoint) returnValue = handle_replay_errors(#entrypoint, replayResult, pPacket->result, returnValue);

class vkReplay {
public:
    ~vkReplay();
    vkReplay(glvreplay_settings *pReplaySettings);

    int init(glv_replay::Display & disp);
    vkDisplay * get_display() {return m_display;}
    glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header *packet);
    glv_replay::GLV_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const VkResult resCall, const VkResult resTrace, const glv_replay::GLV_REPLAY_RESULT resIn);

    void push_validation_msg(VkValidationLevel validationLevel, VkObject srcObject, size_t location, int32_t msgCode, const char* pMsg);
    glv_replay::GLV_REPLAY_RESULT pop_validation_msgs();
    int dump_validation_data();
private:
    struct vkFuncs m_vkFuncs;
    vkReplayObjMapper m_objMapper;
    void (*m_pDSDump)(char*);
    void (*m_pCBDump)(char*);
    GLVSNAPSHOT_PRINT_OBJECTS m_pGlvSnapshotPrint;
    vkDisplay *m_display;
    struct shaderPair {
        VkShader *addr;
        VkShader val;
    };
    struct validationMsg {
        VkValidationLevel validationLevel;
        VkObject srcObject;
        size_t location;
        int32_t msgCode;
        char msg[256];
    };
    std::vector<struct validationMsg> m_validationMsgs;
    std::vector<int> m_screenshotFrames;
    VkResult manually_replay_vkCreateInstance(struct_vkCreateInstance* pPacket);
    VkResult manually_replay_vkCreateDevice(struct_vkCreateDevice* pPacket);
    VkResult manually_replay_vkEnumeratePhysicalDevices(struct_vkEnumeratePhysicalDevices* pPacket);
    VkResult manually_replay_vkGetPhysicalDeviceInfo(struct_vkGetPhysicalDeviceInfo* pPacket);
    VkResult manually_replay_vkGetGlobalExtensionInfo(struct_vkGetGlobalExtensionInfo* pPacket);
    VkResult manually_replay_vkGetPhysicalDeviceExtensionInfo(struct_vkGetPhysicalDeviceExtensionInfo* pPacket);
    VkResult manually_replay_vkQueueSubmit(struct_vkQueueSubmit* pPacket);
    VkResult manually_replay_vkQueueAddMemReferences(struct_vkQueueAddMemReferences* pPacket);
    VkResult manually_replay_vkQueueRemoveMemReferences(struct_vkQueueRemoveMemReferences* pPacket);
    VkResult manually_replay_vkGetObjectInfo(struct_vkGetObjectInfo* pPacket);
    VkResult manually_replay_vkGetFormatInfo(struct_vkGetFormatInfo* pPacket);
    VkResult manually_replay_vkGetImageSubresourceInfo(struct_vkGetImageSubresourceInfo* pPacket);
    void manually_replay_vkUpdateDescriptors(struct_vkUpdateDescriptors* pPacket);
    VkResult manually_replay_vkCreateDescriptorSetLayout(struct_vkCreateDescriptorSetLayout* pPacket);
    void manually_replay_vkCmdBindDescriptorSets(struct_vkCmdBindDescriptorSets* pPacket);
    void manually_replay_vkCmdBindVertexBuffers(struct_vkCmdBindVertexBuffers* pPacket);
    VkResult manually_replay_vkCreateGraphicsPipeline(struct_vkCreateGraphicsPipeline* pPacket);
    VkResult manually_replay_vkCreatePipelineLayout(struct_vkCreatePipelineLayout* pPacket);
    void manually_replay_vkCmdWaitEvents(struct_vkCmdWaitEvents* pPacket);
    void manually_replay_vkCmdPipelineBarrier(struct_vkCmdPipelineBarrier* pPacket);
    VkResult manually_replay_vkCreateFramebuffer(struct_vkCreateFramebuffer* pPacket);
    VkResult manually_replay_vkCreateRenderPass(struct_vkCreateRenderPass* pPacket);
    void manually_replay_vkCmdBeginRenderPass(struct_vkCmdBeginRenderPass* pPacket);
    VkResult manually_replay_vkBeginCommandBuffer(struct_vkBeginCommandBuffer* pPacket);
    VkResult manually_replay_vkStorePipeline(struct_vkStorePipeline* pPacket);
    VkResult manually_replay_vkGetMultiDeviceCompatibility(struct_vkGetMultiDeviceCompatibility* pPacket);
    VkResult manually_replay_vkDestroyObject(struct_vkDestroyObject* pPacket);
    VkResult manually_replay_vkWaitForFences(struct_vkWaitForFences* pPacket);
    VkResult manually_replay_vkAllocMemory(struct_vkAllocMemory* pPacket);
    VkResult manually_replay_vkFreeMemory(struct_vkFreeMemory* pPacket);
    VkResult manually_replay_vkMapMemory(struct_vkMapMemory* pPacket);
    VkResult manually_replay_vkUnmapMemory(struct_vkUnmapMemory* pPacket);
    VkResult manually_replay_vkFlushMappedMemory(struct_vkFlushMappedMemory* pPacket);
    VkResult manually_replay_vkPinSystemMemory(struct_vkPinSystemMemory* pPacket);
    VkResult manually_replay_vkCreateSwapChainWSI(struct_vkCreateSwapChainWSI* pPacket);
    VkResult manually_replay_vkGetSwapChainInfoWSI(struct_vkGetSwapChainInfoWSI* pPacket);
    VkResult manually_replay_vkQueuePresentWSI(struct_vkQueuePresentWSI* pPacket);

    void process_screenshot_list(const char *list)
    {
        std::string spec(list), word;
        size_t start = 0, comma = 0;

        while (start < spec.size()) {
            comma = spec.find(',', start);

            if (comma == std::string::npos)
                word = std::string(spec, start);
            else
                word = std::string(spec, start, comma - start);

            m_screenshotFrames.push_back(atoi(word.c_str()));
            if (comma == std::string::npos)
                break;

            start = comma + 1;

        }
    }
};
