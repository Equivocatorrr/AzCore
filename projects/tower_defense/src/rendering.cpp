/*
    File: rendering.cpp
    Author: Philip Haynes
*/

#include "rendering.hpp"
#include "globals.hpp"

#include "AzCore/log_stream.hpp"
#include "AzCore/io.hpp"
#include "AzCore/font.hpp"

namespace Rendering {

io::logStream cout("rendering.log");

String error = "No error.";

void PushConstants::vert_t::Push(VkCommandBuffer commandBuffer, const Manager *rendering) const {
    vkCmdPushConstants(commandBuffer, rendering->data.pipeline2D->data.layout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vert_t), this);
}

void PushConstants::frag_t::Push(VkCommandBuffer commandBuffer, const Manager *rendering) const {
    vkCmdPushConstants(commandBuffer, rendering->data.pipeline2D->data.layout,
            VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(PushConstants, frag), sizeof(frag_t), this);
}

void PushConstants::font_t::Push(VkCommandBuffer commandBuffer, const Manager *rendering) const {
    vkCmdPushConstants(commandBuffer, rendering->data.pipelineFont->data.layout,
            VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(PushConstants, frag), sizeof(frag_t) + sizeof(font_t), (char*)this - sizeof(frag_t));
}

void PushConstants::Push2D(VkCommandBuffer commandBuffer, const Manager *rendering) const {
    vert.Push(commandBuffer, rendering);
    frag.Push(commandBuffer, rendering);
}

void PushConstants::PushFont(VkCommandBuffer commandBuffer, const Manager *rendering) const {
    vert.Push(commandBuffer, rendering);
    font.Push(commandBuffer, rendering);
}

bool Manager::Init() {
    data.device = data.instance.AddDevice();
    data.queueGraphics = data.device->AddQueue();
    data.queueGraphics->queueType = vk::QueueType::GRAPHICS;
    data.queuePresent = data.device->AddQueue();
    data.queuePresent->queueType = vk::QueueType::PRESENT;
    data.swapchain = data.device->AddSwapchain();
    data.swapchain->vsync = false;
    data.swapchain->window = data.instance.AddWindowForSurface(&globals->window);
    data.framebuffer = data.device->AddFramebuffer();
    data.framebuffer->swapchain = data.swapchain;
    data.renderPass = data.device->AddRenderPass();
    auto attachment = data.renderPass->AddAttachment(data.swapchain);
    attachment->sampleCount = VK_SAMPLE_COUNT_8_BIT;
    attachment->resolveColor = true;
    auto subpass = data.renderPass->AddSubpass();
    subpass->UseAttachment(attachment, vk::AttachmentType::ATTACHMENT_ALL,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    data.framebuffer->renderPass = data.renderPass;
    attachment->clearColor = true;
    // attachment->clearColorValue = {1.0, 1.0, 1.0, 1.0};
    attachment->clearColorValue = {0.0, 0.05, 0.1, 1.0}; // AzCore blue
    if (data.concurrency < 1) {
        data.concurrency = 1;
    }
    data.commandPools.Resize(data.concurrency);
    data.commandBuffersSecondary.Resize(data.concurrency);
    for (i32 i = 0; i < data.concurrency; i++) {
        data.commandPools[i] = data.device->AddCommandPool(data.queueGraphics);
        data.commandPools[i]->resettable = true;
        data.commandBuffersSecondary[i] = data.commandPools[i]->AddCommandBuffer();
        data.commandBuffersSecondary[i]->oneTimeSubmit = true;
        data.commandBuffersSecondary[i]->secondary = true;
        data.commandBuffersSecondary[i]->renderPass = data.renderPass;
        data.commandBuffersSecondary[i]->renderPassContinue = true;
        data.commandBuffersSecondary[i]->framebuffer = data.framebuffer;
    }

    data.semaphoreImageAvailable = data.device->AddSemaphore();
    data.semaphoreRenderComplete = data.device->AddSemaphore();

    for (i32 i = 0; i < 2; i++) {
        data.commandBufferPrimary[i] = data.commandPools[0]->AddCommandBuffer();
        data.queueSubmission[i] = data.device->AddQueueSubmission();
        data.queueSubmission[i]->commandBuffers = {data.commandBufferPrimary[i]};
        data.queueSubmission[i]->signalSemaphores = {data.semaphoreRenderComplete};
        data.queueSubmission[i]->waitSemaphores = {vk::SemaphoreWait(data.swapchain, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)};
        data.queueSubmission[i]->noAutoConfig = true;
    }

    data.queueSubmissionTransfer = data.device->AddQueueSubmission();
    data.queueSubmissionTransfer->commandBuffers = {data.commandBufferPrimary[0]};

    data.textureSampler = data.device->AddSampler();
    data.textureSampler->anisotropy = 4;
    data.textureSampler->mipLodBias = -1.0; // Crisp!!!
    data.textureSampler->maxLod = 1000000000000.0; // Just, like, BIG

    data.stagingMemory = data.device->AddMemory();
    data.stagingMemory->deviceLocal = false;
    data.bufferMemory = data.device->AddMemory();
    data.textureMemory = data.device->AddMemory();

    data.fontStagingMemory = data.device->AddMemory();
    data.fontStagingMemory->deviceLocal = false;
    data.fontBufferMemory = data.device->AddMemory();
    data.fontImageMemory = data.device->AddMemory();

    // Unit square
    Array<Vertex> vertices = {
        {vec2(0.0, 0.0), vec2(0.0, 0.0)},
        {vec2(0.0, 1.0), vec2(0.0, 1.0)},
        {vec2(1.0, 1.0), vec2(1.0, 1.0)},
        {vec2(1.0, 0.0), vec2(1.0, 0.0)}
    };
    Array<u32> indices = {0, 1, 2, 2, 3, 0};

    vk::Buffer baseBuffer = vk::Buffer();
    baseBuffer.size = 1;
    baseBuffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    Range<vk::Buffer> bufferStagingBuffers = data.stagingMemory->AddBuffers(2, baseBuffer);
    bufferStagingBuffers[0].size = vertices.size * sizeof(Vertex);
    bufferStagingBuffers[1].size = indices.size * sizeof(u32);

    data.vertexBuffer = data.bufferMemory->AddBuffer();
    data.indexBuffer = data.bufferMemory->AddBuffer();
    data.vertexBuffer->size = bufferStagingBuffers[0].size;
    data.indexBuffer->size = bufferStagingBuffers[1].size;
    data.vertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    data.indexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    auto texStagingBuffers = data.stagingMemory->AddBuffers(globals->assets.textures.size, baseBuffer);

    data.fontStagingVertexBuffer = data.fontStagingMemory->AddBuffer(baseBuffer);
    data.fontStagingImageBuffers = data.fontStagingMemory->AddBuffers(globals->assets.fonts.size, baseBuffer);

    data.fontVertexBuffer = data.fontBufferMemory->AddBuffer(baseBuffer);
    data.fontVertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    vk::Image baseImage = vk::Image();
    baseImage.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    baseImage.format = VK_FORMAT_R8G8B8A8_UNORM;
    auto texImages = data.textureMemory->AddImages(globals->assets.textures.size, baseImage);

    baseImage.format = VK_FORMAT_R8_UNORM;
    baseImage.width = 1;
    baseImage.height = 1;
    data.fontImages = data.fontImageMemory->AddImages(globals->assets.fonts.size, baseImage);

    for (i32 i = 0; i < texImages.size; i++) {
        const i32 channels = globals->assets.textures[i].channels;
        if (channels != 4) {
            error = "Invalid channel count (" + ToString(channels) + ") in textures[" + ToString(i) + "]";
            return false;
        }
        texImages[i].width = globals->assets.textures[i].width;
        texImages[i].height = globals->assets.textures[i].height;
        texImages[i].mipLevels = floor(log2((f32)max(texImages[i].width, texImages[i].height))) + 1;

        texStagingBuffers[i].size = channels * texImages[i].width * texImages[i].height;
    }

    data.descriptors = data.device->AddDescriptors();
    Ptr<vk::DescriptorLayout> descriptorLayoutTexture = data.descriptors->AddLayout();
    descriptorLayoutTexture->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorLayoutTexture->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorLayoutTexture->bindings.Resize(1);
    descriptorLayoutTexture->bindings[0].binding = 0;
    descriptorLayoutTexture->bindings[0].count = globals->assets.textures.size;
    Ptr<vk::DescriptorLayout> descriptorLayoutFont = data.descriptors->AddLayout();
    descriptorLayoutFont->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorLayoutFont->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorLayoutFont->bindings.Resize(1);
    descriptorLayoutFont->bindings[0].binding = 0;
    descriptorLayoutFont->bindings[0].count = globals->assets.fonts.size;

    data.descriptorSet2D = data.descriptors->AddSet(descriptorLayoutTexture);
    if (!data.descriptorSet2D->AddDescriptor(texImages, data.textureSampler, 0)) {
        error = "Failed to add Texture Descriptor: " + vk::error;
        return false;
    }
    data.descriptorSetFont = data.descriptors->AddSet(descriptorLayoutFont);
    if (!data.descriptorSetFont->AddDescriptor(data.fontImages, data.textureSampler, 0)) {
        error = "Failed to add Font Descriptor: " + vk::error;
        return false;
    }

    Range<vk::Shader> shaders = data.device->AddShaders(3);
    shaders[0].filename = "data/shaders/2D.vert.spv";
    shaders[1].filename = "data/shaders/2D.frag.spv";
    shaders[2].filename = "data/shaders/Font.frag.spv";

    vk::ShaderRef shaderRefs[3] = {
        vk::ShaderRef(shaders.ToPtr(0), VK_SHADER_STAGE_VERTEX_BIT),
        vk::ShaderRef(shaders.ToPtr(1), VK_SHADER_STAGE_FRAGMENT_BIT),
        vk::ShaderRef(shaders.ToPtr(2), VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    data.pipeline2D = data.device->AddPipeline();
    data.pipeline2D->renderPass = data.renderPass;
    data.pipeline2D->subpass = 0;
    data.pipeline2D->shaders.Append(shaderRefs[0]);
    data.pipeline2D->shaders.Append(shaderRefs[1]);

    data.pipeline2D->descriptorLayouts.Append(descriptorLayoutTexture);

    data.pipeline2D->dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    data.pipelineFont = data.device->AddPipeline();
    data.pipelineFont->renderPass = data.renderPass;
    data.pipelineFont->subpass = 0;
    data.pipelineFont->shaders.Append(shaderRefs[0]);
    data.pipelineFont->shaders.Append(shaderRefs[2]);

    data.pipelineFont->descriptorLayouts.Append(descriptorLayoutFont);

    data.pipelineFont->dynamicStates = data.pipeline2D->dynamicStates;

    VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
    vertexInputAttributeDescription.binding = 0;
    vertexInputAttributeDescription.location = 0;
    vertexInputAttributeDescription.offset = offsetof(Vertex, pos);
    vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    data.pipeline2D->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
    data.pipelineFont->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
    vertexInputAttributeDescription.location = 1;
    vertexInputAttributeDescription.offset = offsetof(Vertex, tex);
    vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    data.pipeline2D->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
    data.pipelineFont->inputAttributeDescriptions.Append(vertexInputAttributeDescription);
    VkVertexInputBindingDescription vertexInputBindingDescription = {};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBindingDescription.stride = sizeof(Vertex);
    data.pipeline2D->inputBindingDescriptions.Append(vertexInputBindingDescription);
    data.pipelineFont->inputBindingDescriptions.Append(vertexInputBindingDescription);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    data.pipeline2D->colorBlendAttachments.Append(colorBlendAttachment);
    data.pipelineFont->colorBlendAttachments.Append(colorBlendAttachment);

    data.pipeline2D->pushConstantRanges = {
        {/* stage flags */ VK_SHADER_STAGE_VERTEX_BIT, /* offset */ 0, /* size */ 32},
        {/* stage flags */ VK_SHADER_STAGE_FRAGMENT_BIT, /* offset */ 32, /* size */ 20}
    };
    data.pipelineFont->pushConstantRanges = {
        {/* stage flags */ VK_SHADER_STAGE_VERTEX_BIT, /* offset */ 0, /* size */ 32},
        {/* stage flags */ VK_SHADER_STAGE_FRAGMENT_BIT, /* offset */ 32, /* size */ 28}
    };

    if (!data.instance.Init()) {
        error = "Failed to init vk::instance: " + vk::error;
        return false;
    }

    // Everybody do the transfer!
    bufferStagingBuffers[0].CopyData(vertices.data);
    bufferStagingBuffers[1].CopyData(indices.data);
    for (i32 i = 0; i < texStagingBuffers.size; i++) {
        texStagingBuffers[i].CopyData(globals->assets.textures[i].pixels.data);
    }

    VkCommandBuffer cmdBufCopy = data.commandBufferPrimary[0]->Begin();
    data.vertexBuffer->Copy(cmdBufCopy, bufferStagingBuffers.ToPtr(0));
    data.indexBuffer->Copy(cmdBufCopy, bufferStagingBuffers.ToPtr(1));

    for (i32 i = 0; i < texStagingBuffers.size; i++) {
        texImages[i].TransitionLayout(cmdBufCopy, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        texImages[i].Copy(cmdBufCopy, texStagingBuffers.ToPtr(i));
        texImages[i].GenerateMipMaps(cmdBufCopy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    if (!data.commandBufferPrimary[0]->End()) {
        error = "Failed to copy from staging buffers: " + vk::error;
        return false;
    }
    if (!data.device->SubmitCommandBuffers(data.queueGraphics, {data.queueSubmissionTransfer})) {
        error = "Failed to submit transfer command buffers: " + vk::error;
        return false;
    }
    vk::QueueWaitIdle(data.queueGraphics);

    if (!UpdateFonts()) {
        error = "Failed to update fonts: " + error;
        return false;
    }

    return true;
}

bool Manager::Deinit() {
    if (!data.instance.Deinit()) {
        error = vk::error;
        return false;
    }
    return true;
}

bool Manager::UpdateFonts() {
    // Will be done on-the-fly
    if (data.fontStagingMemory->data.initted) {
        data.fontStagingMemory->Deinit();
    }
    if (data.fontBufferMemory->data.initted) {
        data.fontBufferMemory->Deinit();
    }
    if (data.fontImageMemory->data.initted) {
        data.fontImageMemory->Deinit();
    }

    // Vertex buffer
    Array<Vertex> fontVertices;
    fontIndexOffsets = {0};
    for (i32 i = 0; i < globals->assets.fonts.size; i++) {
        for (font::Glyph& glyph : globals->assets.fonts[i].fontBuilder.glyphs) {
            if (glyph.info.size.x == 0.0 || glyph.info.size.y == 0.0) {
                continue;
            }
            const f32 boundSquare = globals->assets.fonts[i].fontBuilder.boundSquare;
            f32 posTop = -glyph.info.offset.y * boundSquare;
            f32 posLeft = -glyph.info.offset.x * boundSquare;
            f32 posBot = -glyph.info.size.y * boundSquare + posTop;
            f32 posRight = glyph.info.size.x * boundSquare + posLeft;
            f32 texLeft = glyph.info.pos.x;
            f32 texBot = glyph.info.pos.y;
            f32 texRight = (glyph.info.pos.x + glyph.info.size.x);
            f32 texTop = (glyph.info.pos.y + glyph.info.size.y);
            Vertex quad[4];
            quad[0].pos = vec2(posLeft, posTop);
            quad[0].tex = vec2(texLeft, texTop);
            quad[1].pos = vec2(posLeft, posBot);
            quad[1].tex = vec2(texLeft, texBot);
            quad[2].pos = vec2(posRight, posBot);
            quad[2].tex = vec2(texRight, texBot);
            quad[3].pos = vec2(posRight, posTop);
            quad[3].tex = vec2(texRight, texTop);
            fontVertices.Append(quad[3]);
            fontVertices.Append(quad[2]);
            fontVertices.Append(quad[1]);
            fontVertices.Append(quad[0]);
        }
        fontIndexOffsets.Append(
            fontIndexOffsets.Back() + globals->assets.fonts[i].fontBuilder.glyphs.size * 4
        );
    }

    data.fontStagingVertexBuffer->size = fontVertices.size * sizeof(Vertex);
    data.fontVertexBuffer->size = data.fontStagingVertexBuffer->size;

    for (i32 i = 0; i < data.fontImages.size; i++) {
        data.fontImages[i].width = globals->assets.fonts[i].fontBuilder.dimensions.x;
        data.fontImages[i].height = globals->assets.fonts[i].fontBuilder.dimensions.y;
        data.fontImages[i].mipLevels = floor(log2((f32)max(data.fontImages[i].width, data.fontImages[i].height))) + 1;

        data.fontStagingImageBuffers[i].size = data.fontImages[i].width * data.fontImages[i].height;
    }

    // Initialize everything
    if (!data.fontStagingMemory->Init(&(*data.device))) {
        return false;
    }
    if (!data.fontBufferMemory->Init(&(*data.device))) {
        return false;
    }
    if (!data.fontImageMemory->Init(&(*data.device))) {
        return false;
    }

    // Update the descriptors
    if (!data.descriptors->Update()) {
        return false;
    }

    data.fontStagingVertexBuffer->CopyData(fontVertices.data);
    for (i32 i = 0; i < data.fontStagingImageBuffers.size; i++) {
        data.fontStagingImageBuffers[i].CopyData(globals->assets.fonts[i].fontBuilder.pixels.data);
    }

    VkCommandBuffer cmdBufCopy = data.commandBufferPrimary[0]->Begin();

    data.fontVertexBuffer->Copy(cmdBufCopy, data.fontStagingVertexBuffer);

    for (i32 i = 0; i < data.fontStagingImageBuffers.size; i++) {
        data.fontImages[i].TransitionLayout(cmdBufCopy, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        data.fontImages[i].Copy(cmdBufCopy, data.fontStagingImageBuffers.ToPtr(i));
        data.fontImages[i].GenerateMipMaps(cmdBufCopy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (!data.commandBufferPrimary[0]->End()) {
        error = "Failed to copy from staging buffers: " + vk::error;
        return false;
    }
    if (!data.device->SubmitCommandBuffers(data.queueGraphics, {data.queueSubmissionTransfer})) {
        error = "Failed to submit transfer command buffers: " + vk::error;
        return false;
    }
    vk::QueueWaitIdle(data.queueGraphics);

    return true;
}

bool Manager::Draw() {
    if (globals->window.resized || data.resized) {
        if (!data.swapchain->Resize()) {
            error = "Failed to resize swapchain: " + vk::error;
            return false;
        }
        data.resized = false;
    }

    bool updateFontMemory = false;
    for (i32 i = 0; i < globals->assets.fonts.size; i++) {
        Assets::Font& font = globals->assets.fonts[i];
        if (font.fontBuilder.indicesToAdd.size != 0) {
            font.fontBuilder.Build();
            updateFontMemory = true;
        }
    }
    if (updateFontMemory) {
        if (!UpdateFonts()) {
            return false;
        }
    }

    VkResult acquisitionResult = data.swapchain->AcquireNextImage();

    if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_NOT_READY) {
        cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
        data.resized = true;
        return true; // Don't render this frame.
    } else if (acquisitionResult == VK_TIMEOUT) {
        cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
        return true;
    } else if (acquisitionResult != VK_SUCCESS) {
        error = "Failed to acquire swapchain image: " + vk::error;
        return false;
    }

    screenSize = vec2((f32)globals->window.width, (f32)globals->window.height);
    aspectRatio = screenSize.y / screenSize.x;

    Array<VkCommandBuffer> commandBuffersSecondary;
    commandBuffersSecondary.Reserve(data.commandBuffersSecondary.size);

    for (auto& commandBuffer : data.commandBuffersSecondary) {
        VkCommandBuffer cmdBuf = commandBuffer->Begin();
        vk::CmdSetViewportAndScissor(cmdBuf, globals->window.width, globals->window.height);
        vk::CmdBindIndexBuffer(cmdBuf, data.indexBuffer, VK_INDEX_TYPE_UINT32);
        commandBuffersSecondary.Append(cmdBuf);
    }

    data.scissorStack = {{vec2i(0), vec2i((i32)globals->window.width, (i32)globals->window.height)}};

    for (auto& renderCallback : data.renderCallbacks) {
        renderCallback.callback(renderCallback.userdata, this, commandBuffersSecondary);
    }

    for (auto& commandBuffer : data.commandBuffersSecondary) {
        commandBuffer->End();
    }

    data.buffer = !data.buffer;

    VkCommandBuffer cmdBuf = data.commandBufferPrimary[data.buffer]->Begin();
    if (cmdBuf == VK_NULL_HANDLE) {
        error = "Failed to Begin recording primary command buffer: " + vk::error;
        return false;
    }

    data.renderPass->Begin(cmdBuf, data.framebuffer, false);

    vk::CmdExecuteCommands(cmdBuf, data.commandBuffersSecondary);

    vkCmdEndRenderPass(cmdBuf);

    data.commandBufferPrimary[data.buffer]->End();

    if (!data.queueSubmission[data.buffer]->Config()) {
        error = "Failed to configure queue submisson: " + vk::error;
        return false;
    }

    // Submit to queue
    if (!data.device->SubmitCommandBuffers(data.queueGraphics, {data.queueSubmission[data.buffer]})) {
        error = "Failed to SubmitCommandBuffers: " + vk::error;
        return false;
    }

    if (!data.swapchain->Present(data.queuePresent, {data.semaphoreRenderComplete->semaphore})) {
        error = "Failed to present: " + vk::error;
        return false;
    }

    vk::DeviceWaitIdle(data.device);

    return true;
}

void Manager::BindPipeline2D(VkCommandBuffer commandBuffer) {
    data.pipeline2D->Bind(commandBuffer);
    vk::CmdBindVertexBuffer(commandBuffer, 0, data.vertexBuffer);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipeline2D->data.layout,
            0, 1, &data.descriptorSet2D->data.set, 0, nullptr);
}

void Manager::BindPipelineFont(VkCommandBuffer commandBuffer) {
    data.pipelineFont->Bind(commandBuffer);
    vk::CmdBindVertexBuffer(commandBuffer, 0, data.fontVertexBuffer);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipelineFont->data.layout,
            0, 1, &data.descriptorSetFont->data.set, 0, nullptr);
}

void Manager::PushScissor(VkCommandBuffer commandBuffer, vec2i min, vec2i max) {
    const ScissorState &prev = data.scissorStack.Back();
    ScissorState state;
    state.min.x = ::max(min.x, prev.min.x);
    state.min.y = ::max(min.y, prev.min.y);
    state.max.x = ::min(max.x, prev.max.x);
    state.max.y = ::min(max.y, prev.max.y);
    data.scissorStack.Append(state);
    vk::CmdSetScissor(commandBuffer, (u32)(state.max.x-state.min.x), (u32)(state.max.y-state.min.y), state.min.x, state.min.y);
}

void Manager::PopScissor(VkCommandBuffer commandBuffer) {
    data.scissorStack.Erase(data.scissorStack.size-1);
    const ScissorState &state = data.scissorStack.Back();
    vk::CmdSetScissor(commandBuffer, (u32)(state.max.x-state.min.x), (u32)(state.max.y-state.min.y), state.min.x, state.min.y);
}

constexpr f32 lineHeight = 1.3;

f32 Manager::CharacterWidth(char32 character, const Assets::Font *fontDesired, const Assets::Font *fontFallback) const {
    const Assets::Font *actualFont = fontDesired;
    i32 glyphIndex = fontDesired->font.GetGlyphIndex(character);
    if (glyphIndex == 0) {
        i32 glyphIndexFallback = fontFallback->font.GetGlyphIndex(character);
        if (glyphIndexFallback != 0) {
            glyphIndex = glyphIndexFallback;
            actualFont = fontFallback;
        }
    }
    const i32 glyphId = actualFont->fontBuilder.indexToId[glyphIndex];
    return actualFont->fontBuilder.glyphs[glyphId].info.advance.x;
}

f32 Manager::LineWidth(const char32 *string, i32 fontIndex) const {
    const Assets::Font *fontDesired = &globals->assets.fonts[fontIndex];
    const Assets::Font *fontFallback = &globals->assets.fonts[0];
    f32 size = 0.0;
    for (i32 i = 0; string[i] != '\n' && string[i] != 0; i++) {
        size += CharacterWidth(string[i], fontDesired, fontFallback);
    }
    return size;
}

vec2 Manager::StringSize(WString string, i32 fontIndex) const {
    const Assets::Font *fontDesired = &globals->assets.fonts[fontIndex];
    const Assets::Font *fontFallback = &globals->assets.fonts[0];
    vec2 size = vec2(0.0, (1.0 + lineHeight) * 0.5);
    f32 lineSize = 0.0;
    for (i32 i = 0; i < string.size; i++) {
        const char32 character = string[i];
        if (character == '\n') {
            lineSize = 0.0;
            size.y += lineHeight;
            continue;
        }
        lineSize += CharacterWidth(character, fontDesired, fontFallback);
        if (lineSize > size.x) {
            size.x = lineSize;
        }
    }
    return size;
}

f32 Manager::StringWidth(WString string, i32 fontIndex) const {
    return StringSize(string, fontIndex).x;
}

f32 StringHeight(WString string) {
    f32 size = (1.0 + lineHeight) * 0.5;
    for (i32 i = 0; i < string.size; i++) {
        const char32 character = string[i];
        if (character == '\n') {
            size += lineHeight;
        }
    }
    return size;
}

WString Manager::StringAddNewlines(WString string, i32 fontIndex, f32 maxWidth) const {
    if (maxWidth < 0.0) {
        cout << "Why are we negative???" << std::endl;
    }
    if (maxWidth <= 0.0) {
        return string;
    }
    const Assets::Font *fontDesired = &globals->assets.fonts[fontIndex];
    const Assets::Font *fontFallback = &globals->assets.fonts[0];
    f32 lineSize = 0.0;
    i32 lastSpace = -1;
    i32 charsThisLine = 0;
    for (i32 i = 0; i < string.size; i++) {
        if (string[i] == '\n') {
            lineSize = 0.0;
            lastSpace = -1;
            charsThisLine = 0;
            continue;
        }
        lineSize += CharacterWidth(string[i], fontDesired, fontFallback);
        charsThisLine++;
        if (string[i] == ' ') {
            lastSpace = i;
        }
        if (lineSize >= maxWidth && charsThisLine > 1) {
            if (lastSpace == -1) {
                string.Insert(i, char32('\n'));
            } else {
                string[lastSpace] = '\n';
                i = lastSpace;
            }
            lineSize = 0.0;
            lastSpace = -1;
            charsThisLine = 0;
        }
    }
    return string;
}

void Manager::DrawCharSS(VkCommandBuffer commandBuffer, char32 character,
                         i32 fontIndex, vec4 color, vec2 position, vec2 scale) {
    Assets::Font *fontDesired = &globals->assets.fonts[fontIndex];
    Assets::Font *fontFallback = &globals->assets.fonts[0];
    fontDesired->fontBuilder.AddRange(character, character);
    fontFallback->fontBuilder.AddRange(character, character);
    Assets::Font *font = fontDesired;
    Rendering::PushConstants pc = Rendering::PushConstants();
    pc.frag.color = color;
    i32 actualFontIndex = fontIndex;
    i32 glyphIndex = fontDesired->font.GetGlyphIndex(character);
    if (glyphIndex == 0) {
        const i32 glyphFallback = fontFallback->font.GetGlyphIndex(character);
        if (glyphFallback != 0) {
            glyphIndex = glyphFallback;
            font = fontFallback;
            actualFontIndex = 0;
        }
    }
    vec2 fullScale = vec2(aspectRatio * scale.x, scale.y);
    i32 glyphId = font->fontBuilder.indexToId[glyphIndex];
    font::Glyph& glyph = font->fontBuilder.glyphs[glyphId];
    pc.frag.texIndex = actualFontIndex;
    if (glyph.components.size != 0) {
        for (const font::Component& component : glyph.components) {
            i32 componentId = font->fontBuilder.indexToId[component.glyphIndex];
            pc.vert.transform = mat2::Scaler(fullScale);
            pc.font.edge = 0.5 / (font::sdfDistance * screenSize.y * pc.vert.transform.h.y2);
            pc.vert.position = position + component.offset * fullScale;
            pc.PushFont(commandBuffer, this);
            vkCmdDrawIndexed(commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + componentId * 4, 0);
        }
    } else {
        pc.font.edge = 0.5 / (font::sdfDistance * screenSize.y * scale.y);
        pc.vert.transform = mat2::Scaler(fullScale);
        pc.vert.position = position;
        pc.PushFont(commandBuffer, this);
        vkCmdDrawIndexed(commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + glyphId * 4, 0);
    }
}

void Manager::DrawTextSS(VkCommandBuffer commandBuffer, WString string,
                         i32 fontIndex, vec4 color, vec2 position, vec2 scale,
                         FontAlign alignH, FontAlign alignV, f32 maxWidth, f32 edge, f32 bounds) {
    Assets::Font *fontDesired = &globals->assets.fonts[fontIndex];
    Assets::Font *fontFallback = &globals->assets.fonts[0];
    fontDesired->fontBuilder.AddString(string);
    fontFallback->fontBuilder.AddString(string);
    scale.x *= aspectRatio;
    Rendering::PushConstants pc = Rendering::PushConstants();
    pc.frag.color = color;
    position.y += scale.y * (lineHeight + 1.0) * 0.5;
    f32 width = 0.0;
    if (alignH != LEFT) {
        width = StringWidth(string, fontIndex) * scale.x;
        if (alignH == MIDDLE) {
            position.x -= width * 0.5;
        } else if (alignH == RIGHT) {
            position.x -= width;
        } else {
            // JUSTIFY
        }
    }
    if (alignV != TOP) {
        f32 height = StringHeight(string) * scale.y;
        if (alignV == MIDDLE) {
            position.y -= height * 0.5;
        } else {
            position.y -= height;
        }
    }
    vec2 cursor = position;
    f32 spaceScale = 1.0;
    f32 spaceWidth = CharacterWidth((char32)' ', fontDesired, fontFallback) * scale.x;
    for (i32 i = 0; i < string.size; i++) {
        char32 character = string[i];
        if (character == '\n' || i == 0) {
            if (i == 0) {
                i--;
            }
            cursor.x = position.x;
            if (alignH != LEFT) {
                f32 lineWidth = LineWidth(&string[i+1], fontIndex) * scale.x;
                if (alignH == RIGHT) {
                    cursor.x += width - lineWidth;
                } else if (alignH == MIDDLE) {
                    cursor.x += (width - lineWidth) * 0.5;
                } else if (alignH == JUSTIFY) {
                    i32 numSpaces = 0;
                    for (i32 ii = i+1; string[ii] != 0 && string[ii] != '\n'; ii++) {
                        if (string[ii] == ' ') {
                            numSpaces++;
                        }
                    }
                    spaceScale = 1.0 + max((maxWidth - lineWidth) / numSpaces / spaceWidth, 0.0);
                    if (spaceScale > 4.0) {
                        spaceScale = 1.5;
                    }
                }
            }
            if (i == -1) {
                i++;
            } else {
                cursor.y += scale.y * lineHeight;
                continue;
            }
        }
        pc.frag.texIndex = fontIndex;
        Assets::Font *font = fontDesired;
        i32 actualFontIndex = fontIndex;
        i32 glyphIndex = fontDesired->font.GetGlyphIndex(character);
        if (glyphIndex == 0) {
            const i32 glyphFallback = fontFallback->font.GetGlyphIndex(character);
            if (glyphFallback != 0) {
                glyphIndex = glyphFallback;
                font = fontFallback;
                pc.frag.texIndex = 0;
                actualFontIndex = 0;
            }
        }
        i32 glyphId = font->fontBuilder.indexToId[glyphIndex];
        font::Glyph& glyph = font->fontBuilder.glyphs[glyphId];

        pc.frag.texIndex = actualFontIndex;
        pc.font.edge = edge / (font::sdfDistance * screenSize.y * scale.y);
        pc.font.bounds = bounds;
        pc.vert.transform = mat2::Scaler(scale);
        if (glyph.components.size != 0) {
            for (const font::Component& component : glyph.components) {
                i32 componentId = font->fontBuilder.indexToId[component.glyphIndex];
                // const font::Glyph& componentGlyph = font->fontBuilder.glyphs[componentId];
                pc.vert.transform = component.transform * mat2::Scaler(scale);
                pc.font.edge = edge / (font::sdfDistance * screenSize.y * abs(pc.vert.transform.h.y2));
                pc.vert.position = cursor + component.offset * scale * vec2(1.0, -1.0);
                pc.PushFont(commandBuffer, this);
                vkCmdDrawIndexed(commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + componentId * 4, 0);
            }
        } else {
            if (character != ' ') {
                pc.vert.position = cursor;
                pc.PushFont(commandBuffer, this);
                vkCmdDrawIndexed(commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + glyphId * 4, 0);
            }
        }
        if (character == ' ') {
            cursor += glyph.info.advance * spaceScale * scale;
        } else {
            cursor += glyph.info.advance * scale;
        }
    }
}

void Manager::DrawQuadSS(VkCommandBuffer commandBuffer, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin, Radians32 rotation) const {
    Rendering::PushConstants pc = Rendering::PushConstants();
    pc.frag.color = color;
    pc.frag.texIndex = texIndex;
    pc.vert.position = position;
    pc.vert.transform = mat2::Scaler(scalePre);
    if (rotation != 0.0) {
        pc.vert.transform = pc.vert.transform * mat2::Rotation(rotation.value());
    }
    pc.vert.transform = pc.vert.transform * mat2::Scaler(scalePost);
    pc.vert.origin = origin;
    pc.Push2D(commandBuffer, this);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}

void Manager::DrawChar(VkCommandBuffer commandBuffer, char32 character, i32 fontIndex, vec4 color, vec2 position, vec2 scale) {
    const vec2 screenSizeFactor = vec2(2.0) / screenSize;
    DrawCharSS(commandBuffer, character, fontIndex, color, position * screenSizeFactor + vec2(-1.0), scale * screenSizeFactor);
}

void Manager::DrawText(VkCommandBuffer commandBuffer, WString text, i32 fontIndex, vec4 color, vec2 position, vec2 scale, FontAlign alignH, FontAlign alignV, f32 maxWidth, f32 edge, f32 bounds) {
    const vec2 screenSizeFactor = vec2(2.0) / screenSize;
    edge += 0.3 + min(0.2, max(0.0, (scale.y - 12.0) / 12.0));
    bounds -= min(0.05, max(0.0, (16.0 - scale.y) * 0.01));
    DrawTextSS(commandBuffer, text, fontIndex, color, position * screenSizeFactor + vec2(-1.0), scale * screenSizeFactor.y, alignH, alignV, maxWidth * screenSizeFactor.x, edge, bounds);
}

void Manager::DrawQuad(VkCommandBuffer commandBuffer, i32 texIndex, vec4 color, vec2 position, vec2 scalePre, vec2 scalePost, vec2 origin, Radians32 rotation) const {
    const vec2 screenSizeFactor = vec2(2.0) / screenSize;
    DrawQuadSS(commandBuffer, texIndex, color, position * screenSizeFactor + vec2(-1.0), scalePre, scalePost * screenSizeFactor, origin, rotation);
}

} // namespace Rendering
