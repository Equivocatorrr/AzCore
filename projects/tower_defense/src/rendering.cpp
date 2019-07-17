/*
    File: rendering.cpp
    Author: Philip Haynes
*/

#include "rendering.hpp"
#include "assets.hpp"

#include "AzCore/log_stream.hpp"
#include "AzCore/io.hpp"
#include "AzCore/font.hpp"

namespace Rendering {

io::logStream cout("rendering.log");

String error = "No error.";

void PushConstants::vert_t::Push(VkCommandBuffer commandBuffer, Manager *rendering) {
    vkCmdPushConstants(commandBuffer, rendering->data.pipeline2D->data.layout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vert_t), this);
}

void PushConstants::frag_t::Push(VkCommandBuffer commandBuffer, Manager *rendering) {
    vkCmdPushConstants(commandBuffer, rendering->data.pipeline2D->data.layout,
            VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(PushConstants, frag), sizeof(frag_t), this);
}

void PushConstants::font_t::Push(VkCommandBuffer commandBuffer, Manager *rendering) {
    vkCmdPushConstants(commandBuffer, rendering->data.pipelineFont->data.layout,
            VK_SHADER_STAGE_FRAGMENT_BIT, offsetof(PushConstants, frag), sizeof(frag_t) + sizeof(font_t), (char*)this - sizeof(frag_t));
}

void PushConstants::Push2D(VkCommandBuffer commandBuffer, Manager *rendering) {
    vert.Push(commandBuffer, rendering);
    frag.Push(commandBuffer, rendering);
}

void PushConstants::PushFont(VkCommandBuffer commandBuffer, Manager *rendering) {
    vert.Push(commandBuffer, rendering);
    font.Push(commandBuffer, rendering);
}

bool Manager::Init() {
    if (window == nullptr) {
        error = "Manager needs a window.";
        return false;
    }
    data.device = data.instance.AddDevice();
    data.queueGraphics = data.device->AddQueue();
    data.queueGraphics->queueType = vk::QueueType::GRAPHICS;
    data.queuePresent = data.device->AddQueue();
    data.queuePresent->queueType = vk::QueueType::PRESENT;
    data.swapchain = data.device->AddSwapchain();
    data.swapchain->vsync = false;
    data.swapchain->window = data.instance.AddWindowForSurface(window);
    data.framebuffer = data.device->AddFramebuffer();
    data.framebuffer->swapchain = data.swapchain;
    data.renderPass = data.device->AddRenderPass();
    auto attachment = data.renderPass->AddAttachment(data.swapchain);
    auto subpass = data.renderPass->AddSubpass();
    subpass->UseAttachment(attachment, vk::AttachmentType::ATTACHMENT_ALL,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    data.framebuffer->renderPass = data.renderPass;
    attachment->clearColor = true;
    attachment->clearColorValue = {0.0, 0.05, 0.1, 1.0};
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
    data.textureSampler->mipLodBias = -0.25; // Crisp!!!
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

    auto texStagingBuffers = data.stagingMemory->AddBuffers(textures->size, baseBuffer);

    data.fontStagingVertexBuffer = data.fontStagingMemory->AddBuffer(baseBuffer);
    data.fontStagingImageBuffers = data.fontStagingMemory->AddBuffers(fonts->size, baseBuffer);

    data.fontVertexBuffer = data.fontBufferMemory->AddBuffer(baseBuffer);
    data.fontVertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    vk::Image baseImage = vk::Image();
    baseImage.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    baseImage.format = VK_FORMAT_R8G8B8A8_UNORM;
    auto texImages = data.textureMemory->AddImages(textures->size, baseImage);

    baseImage.format = VK_FORMAT_R8_UNORM;
    baseImage.width = 1;
    baseImage.height = 1;
    data.fontImages = data.fontImageMemory->AddImages(fonts->size, baseImage);

    for (i32 i = 0; i < texImages.size; i++) {
        const i32 channels = (*textures)[i].channels;
        if (channels != 4) {
            error = "Invalid channel count (" + ToString(channels) + ") in textures[" + ToString(i) + "]";
            return false;
        }
        texImages[i].width = (*textures)[i].width;
        texImages[i].height = (*textures)[i].height;
        texImages[i].mipLevels = floor(log2((f32)max(texImages[i].width, texImages[i].height))) + 1;

        texStagingBuffers[i].size = channels * texImages[i].width * texImages[i].height;
    }

    data.descriptors = data.device->AddDescriptors();
    Ptr<vk::DescriptorLayout> descriptorLayoutTexture = data.descriptors->AddLayout();
    descriptorLayoutTexture->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorLayoutTexture->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorLayoutTexture->bindings.Resize(1);
    descriptorLayoutTexture->bindings[0].binding = 0;
    descriptorLayoutTexture->bindings[0].count = textures->size;
    Ptr<vk::DescriptorLayout> descriptorLayoutFont = data.descriptors->AddLayout();
    descriptorLayoutFont->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorLayoutFont->stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorLayoutFont->bindings.Resize(1);
    descriptorLayoutFont->bindings[0].binding = 0;
    descriptorLayoutFont->bindings[0].count = fonts->size;

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
        texStagingBuffers[i].CopyData((*textures)[i].pixels.data);
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
    for (i32 i = 0; i < fonts->size; i++) {
        for (font::Glyph& glyph : (*fonts)[i].fontBuilder.glyphs) {
            if (glyph.info.size.x == 0.0 || glyph.info.size.y == 0.0) {
                continue;
            }
            const f32 boundSquare = (*fonts)[i].fontBuilder.boundSquare;
            f32 posLeft = -glyph.info.offset.x * boundSquare;
            f32 posBot = (-glyph.info.offset.y - glyph.info.size.y) * boundSquare;
            f32 posRight = (glyph.info.size.x - glyph.info.offset.x) * boundSquare;
            f32 posTop = -glyph.info.offset.y * boundSquare;
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
            fontIndexOffsets.Back() + (*fonts)[i].fontBuilder.glyphs.size * 4
        );
    }

    data.fontStagingVertexBuffer->size = fontVertices.size * sizeof(Vertex);
    data.fontVertexBuffer->size = data.fontStagingVertexBuffer->size;

    for (i32 i = 0; i < data.fontImages.size; i++) {
        data.fontImages[i].width = (*fonts)[i].fontBuilder.dimensions.x;
        data.fontImages[i].height = (*fonts)[i].fontBuilder.dimensions.y;
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
        data.fontStagingImageBuffers[i].CopyData((*fonts)[i].fontBuilder.pixels.data);
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
    if (window->resized || data.resized) {
        if (!data.swapchain->Resize()) {
            error = "Failed to resize swapchain: " + vk::error;
            return false;
        }
        data.resized = false;
    }

    bool updateFontMemory = false;
    for (i32 i = 0; i < fonts->size; i++) {
        Assets::Font& font = (*fonts)[i];
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

    screenSize = vec2((f32)window->width, (f32)window->height);
    aspectRatio = screenSize.y / screenSize.x;

    Array<VkCommandBuffer> commandBuffersSecondary;
    commandBuffersSecondary.Reserve(data.commandBuffersSecondary.size);

    for (auto& commandBuffer : data.commandBuffersSecondary) {
        VkCommandBuffer cmdBuf = commandBuffer->Begin();
        vk::CmdSetViewportAndScissor(cmdBuf, window->width, window->height);
        vk::CmdBindIndexBuffer(cmdBuf, data.indexBuffer, VK_INDEX_TYPE_UINT32);
        commandBuffersSecondary.Append(cmdBuf);
    }

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

void Manager::DrawCharSS(VkCommandBuffer commandBuffer, char32 character,
                         i32 fontIndex, vec2 position, vec2 scale) {
    const Assets::Font *font = &(*fonts)[fontIndex];
    const i32 glyphIndexOffset = font->fontBuilder.indexToId[font->font.GetGlyphIndex(character)] * 4;
    Rendering::PushConstants pc = Rendering::PushConstants();
    pc.frag.texIndex = fontIndex;
    pc.font.edge = 0.5 / (font::sdfDistance * screenSize.y * scale.y);
    pc.vert.transform = pc.vert.transform.Scale(vec2(aspectRatio * scale.x, scale.y));
    pc.vert.position = position;
    pc.PushFont(commandBuffer, this);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, fontIndexOffsets[fontIndex] + glyphIndexOffset, 0);
}

void Manager::DrawTextSS(VkCommandBuffer commandBuffer, WString text,
                         i32 fontIndex, vec2 position, vec2 scale,
                         FontAlign alignH, FontAlign alignV, f32 lineWidth) {
    Assets::Font *fontDesired = &(*fonts)[fontIndex];
    Assets::Font *fontFallback = &(*fonts)[0];
    fontDesired->fontBuilder.AddString(text);
    fontFallback->fontBuilder.AddString(text);
    scale.x *= aspectRatio;
    Rendering::PushConstants pc = Rendering::PushConstants();
    pc.font.edge = 0.5 / (font::sdfDistance * screenSize.y * scale.y);
    pc.vert.transform = pc.vert.transform.Scale(scale);
    vec2 cursor = position;
    for (i32 i = 0; i < text.size; i++) {
        char32 character = text[i];
        if (character == '\n') {
            cursor = position;
            cursor.y += scale.y;
            continue;
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
        if (glyph.components.size != 0) {
            text[i] = '?';
            i--;
            continue;
        }
        if (character != ' ') {
            pc.vert.position = cursor;
            pc.PushFont(commandBuffer, this);
            vkCmdDrawIndexed(commandBuffer, 6, 1, 0, fontIndexOffsets[actualFontIndex] + glyphId * 4, 0);
        }
        cursor += glyph.info.advance * scale;
    }
}

} // namespace Rendering