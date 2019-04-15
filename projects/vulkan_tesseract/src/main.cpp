/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/vk.hpp"

io::logStream cout("main.log");

const u32 maxVertices = 8192;

struct Vertex {
    vec4 color;
    vec2 pos;
    f32 depth;
};

void DrawCircle(VkCommandBuffer cmdBuf, Vertex *vertices, u32 *vertex, vec2 center, f32 radius, vec4 color, f32 aspectRatio, f32 depth) {
    u32 circumference = min(max(sqrt(radius*tau*1200.0), 4.0), 80.0);
    center.x *= aspectRatio;
    vertices[(*vertex)++] = {color, center, depth-radius};
    for (u32 i = 0; i <= circumference; i++) {
        f32 angle = (f32)i * tau / (f32)circumference;
        vertices[(*vertex)++] = {color, center + vec2(sin(angle)*radius*aspectRatio, cos(angle)*radius), depth};
    }
    vkCmdDraw(cmdBuf, circumference+2, 1, *vertex - circumference - 2, 0);
    vkCmdDraw(cmdBuf, 1, 1, UINT32_MAX, 0);
}

void DrawQuad(VkCommandBuffer cmdBuf, Vertex *vertices, u32 *vertex, vec2 points[4], vec4 colors[4], f32 depths[4]) {
    for (u32 i = 0; i < 4; i++) {
        vertices[(*vertex)++] = {colors[i], points[i], depths[i]};
    }
    vkCmdDraw(cmdBuf, 4, 1, *vertex - 4, 0);
    vkCmdDraw(cmdBuf, 1, 1, UINT32_MAX, 0);
}

void DrawLine(VkCommandBuffer cmdBuf, Vertex *vertices, u32 *vertex, vec2 p1, vec2 p2, vec4 color, f32 depth) {
    vertices[(*vertex)++] = {color, p1, depth};
    vertices[(*vertex)++] = {color, p2, depth};
    vkCmdDraw(cmdBuf, 2, 1, *vertex - 2, 0);
}

i32 main(i32 argumentCount, char** argumentValues) {

    bool enableLayers = false, enableCoreValidation = false;

    cout << "\nTest program received " << argumentCount << " arguments:\n";
    for (i32 i = 0; i < argumentCount; i++) {
        cout << i << ": " << argumentValues[i] << std::endl;
        if (equals(argumentValues[i], "--enable-layers")) {
            enableLayers = true;
        } else if (equals(argumentValues[i], "--core-validation")) {
            enableCoreValidation = true;
        }
    }

    io::Input input;
    io::Window window;
    window.input = &input;
    window.name = "AzCore Tesseract";
    window.width = 800;
    window.height = 800;
    if (!window.Open()) {
        cout << "Failed to open window: " << io::error << std::endl;
        return 1;
    }

    vk::Instance instance;
    instance.AppInfo("AzCore Tesseract", 1, 0, 0);
    Ptr<vk::Window> vkWindow = instance.AddWindowForSurface(&window);

    if (enableLayers) {
        cout << "Validation layers enabled." << std::endl;
        Array<const char*> layers = {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_GOOGLE_unique_objects"
        };
        if (enableCoreValidation) {
            layers.Append("VK_LAYER_LUNARG_core_validation");
        }
        instance.AddLayers(layers);
    }

    Ptr<vk::Device> device = instance.AddDevice();

    Ptr<vk::Queue> queueGraphics = device->AddQueue();
    queueGraphics->queueType = vk::QueueType::GRAPHICS;
    Ptr<vk::Queue> queueTransfer = device->AddQueue();
    queueTransfer->queueType = vk::QueueType::TRANSFER;
    Ptr<vk::Queue> queuePresent = device->AddQueue();
    queuePresent->queueType = vk::QueueType::PRESENT;

    Ptr<vk::Memory> vkStagingMemory = device->AddMemory();
    vkStagingMemory->deviceLocal = false;
    Ptr<vk::Memory> vkBufferMemory = device->AddMemory();

    Ptr<vk::Buffer> vkStagingBuffer = vkStagingMemory->AddBuffer();
    vkStagingBuffer->size = sizeof(Vertex) * maxVertices;
    vkStagingBuffer->usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    Ptr<vk::Buffer> vkVertexBuffer = vkBufferMemory->AddBuffer();
    vkVertexBuffer->size = sizeof(Vertex) * maxVertices;
    vkVertexBuffer->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    Ptr<vk::Swapchain> vkSwapchain = device->AddSwapchain();
    vkSwapchain->window = vkWindow;
    // vkSwapchain->vsync = false;

    Ptr<vk::RenderPass> vkRenderPass = device->AddRenderPass();
    Ptr<vk::Attachment> vkAttachment = vkRenderPass->AddAttachment(vkSwapchain);
    vkAttachment->clearColor = true;
    vkAttachment->clearColorValue = {0.0, 0.1, 0.2, 1.0};
    // vkAttachment->bufferDepthStencil = true;
    // vkAttachment->clearDepth = true;
    // vkAttachment->clearDepthStencilValue.depth = 1000.0;
    vkAttachment->resolveColor = true;
    vkAttachment->sampleCount = VK_SAMPLE_COUNT_8_BIT;

    Ptr<vk::Subpass> vkSubpass = vkRenderPass->AddSubpass();
    vkSubpass->UseAttachment(vkAttachment, vk::AttachmentType::ATTACHMENT_ALL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    Range<vk::Shader> vkShaders = device->AddShaders(2);
    vkShaders[0].filename = "data/shaders/2D.frag.spv";
    vkShaders[1].filename = "data/shaders/2D.vert.spv";

    Array<vk::ShaderRef> vkShaderRefs = {
        vk::ShaderRef(vkShaders.ToPtr(0), VK_SHADER_STAGE_FRAGMENT_BIT),
        vk::ShaderRef(vkShaders.ToPtr(1), VK_SHADER_STAGE_VERTEX_BIT)
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkVertexInputBindingDescription inputBindingDescription = {};
    inputBindingDescription.binding = 0;
    inputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    inputBindingDescription.stride = sizeof(Vertex);
    Array<VkVertexInputAttributeDescription> inputAttributeDescriptions(3);
    inputAttributeDescriptions[0].binding = 0;
    inputAttributeDescriptions[0].location = 0;
    inputAttributeDescriptions[0].offset = offsetof(Vertex, color);
    inputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    inputAttributeDescriptions[1].binding = 0;
    inputAttributeDescriptions[1].location = 1;
    inputAttributeDescriptions[1].offset = offsetof(Vertex, pos);
    inputAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    inputAttributeDescriptions[2].binding = 0;
    inputAttributeDescriptions[2].location = 2;
    inputAttributeDescriptions[2].offset = offsetof(Vertex, depth);
    inputAttributeDescriptions[2].format = VK_FORMAT_R32_SFLOAT;

    Ptr<vk::Pipeline> pipelineTriangleFan = device->AddPipeline();
    pipelineTriangleFan->renderPass = vkRenderPass;
    pipelineTriangleFan->subpass = 0;
    pipelineTriangleFan->shaders = vkShaderRefs;
    pipelineTriangleFan->dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pipelineTriangleFan->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipelineTriangleFan->inputAssembly.primitiveRestartEnable = VK_TRUE;
    pipelineTriangleFan->inputBindingDescriptions = {inputBindingDescription};
    pipelineTriangleFan->inputAttributeDescriptions = inputAttributeDescriptions;
    pipelineTriangleFan->colorBlendAttachments.Append(colorBlendAttachment);
    // pipelineTriangleFan->depthStencil.depthTestEnable = VK_TRUE;
    // pipelineTriangleFan->depthStencil.depthWriteEnable = VK_TRUE;
    pipelineTriangleFan->rasterizer.cullMode = VK_CULL_MODE_NONE;

    Ptr<vk::Pipeline> pipelineLines = device->AddPipeline();
    pipelineLines->renderPass = vkRenderPass;
    pipelineLines->subpass = 0;
    pipelineLines->shaders = vkShaderRefs;
    pipelineLines->dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pipelineLines->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pipelineLines->inputBindingDescriptions = {inputBindingDescription};
    pipelineLines->inputAttributeDescriptions = inputAttributeDescriptions;
    pipelineLines->colorBlendAttachments.Append(colorBlendAttachment);
    pipelineLines->rasterizer.lineWidth = 4.0;
    // pipelineLines->depthStencil.depthTestEnable = VK_TRUE;
    // pipelineLines->depthStencil.depthWriteEnable = VK_TRUE;

    Ptr<vk::Framebuffer> vkFramebuffer = device->AddFramebuffer();
    vkFramebuffer->renderPass = vkRenderPass;
    vkFramebuffer->swapchain = vkSwapchain;

    Ptr<vk::CommandPool> vkCommandPoolTransfer = device->AddCommandPool(queueTransfer);
    vkCommandPoolTransfer->transient = true;
    vkCommandPoolTransfer->resettable = true;
    Ptr<vk::CommandBuffer> vkCommandBufferTransfer = vkCommandPoolTransfer->AddCommandBuffer();
    vkCommandBufferTransfer->oneTimeSubmit = true;

    Ptr<vk::CommandPool> vkCommandPoolAllDrawing = device->AddCommandPool(queueGraphics);
    vkCommandPoolAllDrawing->resettable = true;
    vkCommandPoolAllDrawing->transient = true;

    Ptr<vk::CommandBuffer> vkCommandBufferAllDrawing = vkCommandPoolAllDrawing->AddCommandBuffer();
    vkCommandBufferAllDrawing->oneTimeSubmit = true;

    Ptr<vk::Semaphore> semaphoreTransferComplete = device->AddSemaphore();
    Ptr<vk::Semaphore> semaphoreRenderFinished = device->AddSemaphore();

    Ptr<vk::QueueSubmission> queueSubmissionTransfer = device->AddQueueSubmission();
    queueSubmissionTransfer->commandBuffers = {vkCommandBufferTransfer};
    queueSubmissionTransfer->signalSemaphores = {semaphoreTransferComplete};

    Ptr<vk::QueueSubmission> queueSubmissionDraw = device->AddQueueSubmission();
    queueSubmissionDraw->commandBuffers = {vkCommandBufferAllDrawing};
    queueSubmissionDraw->signalSemaphores = {semaphoreRenderFinished};
    // Since we're transferring the vertex buffer every frame
    queueSubmissionDraw->waitSemaphores = {{semaphoreTransferComplete, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT}, {}};
    queueSubmissionDraw->noAutoConfig = true;

    if (!instance.Init()) {
        cout << "Failed to init instance: " << vk::error << std::endl;
        return 1;
    }

    if (!window.Show()) {
        cout << "Failed to show window: " << io::error << std::endl;
        return 1;
    }

    Vertex *vertices = new Vertex[maxVertices];

    f32 rotateAngle = 0.0;
    f32 eyeWidth = -0.2;

    vec3 offset(0.0);
    offset.z = 3.0;
    bool faceMode = false;
    bool pause = false;
    bool enableStereoGraphic = false;
    bool resized = false;

    while (true) {
        input.Tick(1.0/60.0);
        if (!window.Update()) {
            break;
        }
        if (input.Pressed(KC_KEY_ESC)) {
            break;
        }
        if (input.Pressed(KC_KEY_F)) {
            faceMode = !faceMode;
        }
        if (input.Pressed(KC_KEY_PAUSE)) {
            pause = !pause;
        }

        f32 aspectRatio = (f32)window.height / (f32)window.width;

        if (input.Pressed(KC_KEY_Q)) {
            enableStereoGraphic = !enableStereoGraphic;
            if (enableStereoGraphic) {
                offset.x -= eyeWidth/2.0;
                if (aspectRatio > 0.9) {
                    window.Resize(window.width * 2.0, window.height);
                    continue;
                }
            } else {
                offset.x += eyeWidth/2.0;
                if (abs(aspectRatio - 0.5) < 0.05) {
                    window.Resize(window.height, window.height);
                    continue;
                }
            }
        }
        if (enableStereoGraphic) {
            if (input.Down(KC_KEY_A)) {
                eyeWidth -= 0.01;
                offset.x += 0.005;
            }
            if (input.Down(KC_KEY_D)) {
                eyeWidth += 0.01;
                offset.x -= 0.005;
            }
        }

        if (resized || window.resized) {
            if (!vkSwapchain->Resize()) {
                cout << "Failed to resize vkSwapchain: " << vk::error << std::endl;
                return 1;
            }
            resized = false;
        }

        VkResult acquisitionResult = vkSwapchain->AcquireNextImage();

        if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_NOT_READY) {
            cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
            resized = true;
            continue; // Don't render this frame.
        } else if (acquisitionResult == VK_TIMEOUT) {
            cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
            continue;
        } else if (acquisitionResult != VK_SUCCESS) {
            cout << vk::error << std::endl;
            return 1;
        }

        u32 vertex = 0;

        VkCommandBuffer cmdBuf = vkCommandBufferAllDrawing->Begin();
        if (cmdBuf == VK_NULL_HANDLE) {
            cout << "Failed to Begin recording vkCommandBufferDraw: " << vk::error << std::endl;
            return 1;
        }
        vkRenderPass->Begin(cmdBuf, vkFramebuffer);

        VkViewport viewport{};
        viewport.width = window.width;
        viewport.height = window.height;
        viewport.maxDepth = 1.0;
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = {window.width, window.height};
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        VkDeviceSize zeroOffset = 0;
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &vkVertexBuffer->data.buffer, &zeroOffset);

        vec5 points[16] = {
            {-1.0, -1.0, -1.0, -1.0,  1.0},
            { 1.0, -1.0, -1.0, -1.0,  1.0},
            {-1.0,  1.0, -1.0, -1.0,  1.0},
            { 1.0,  1.0, -1.0, -1.0,  1.0},
            {-1.0, -1.0,  1.0, -1.0,  1.0},
            { 1.0, -1.0,  1.0, -1.0,  1.0},
            {-1.0,  1.0,  1.0, -1.0,  1.0},
            { 1.0,  1.0,  1.0, -1.0,  1.0},
            {-1.0, -1.0, -1.0,  1.0,  1.0},
            { 1.0, -1.0, -1.0,  1.0,  1.0},
            {-1.0,  1.0, -1.0,  1.0,  1.0},
            { 1.0,  1.0, -1.0,  1.0,  1.0},
            {-1.0, -1.0,  1.0,  1.0,  1.0},
            { 1.0, -1.0,  1.0,  1.0,  1.0},
            {-1.0,  1.0,  1.0,  1.0,  1.0},
            { 1.0,  1.0,  1.0,  1.0,  1.0}
        };
        mat5 view(1.0);
        view = view.RotateBasic(rotateAngle, Plane::YW).RotateBasic(rotateAngle*halfpi, Plane::XZ);
        view.h.v1 = offset.x;
        view.h.v2 = offset.y;
        view.h.v3 = offset.z;

        if (input.Down(KC_KEY_UP)) {
            offset.y += 0.05;
        }
        if (input.Down(KC_KEY_DOWN)) {
            offset.y -= 0.05;
        }
        if (input.Down(KC_KEY_RIGHT)) {
            offset.x -= 0.05;
        }
        if (input.Down(KC_KEY_LEFT)) {
            offset.x += 0.05;
        }
        if (input.Down(KC_KEY_W)) {
            offset.z -= 0.05;
        }
        if (input.Down(KC_KEY_S)) {
            offset.z += 0.05;
        }

        if (!pause) {
            rotateAngle += 0.004;
        }

        vec2 proj[2][16];
        f32 d[2][16];

        for (u32 i = 0; i < 16; i++) {
            vec5 projected[2];
            projected[0] = view * points[i];
            d[0][i] = max(projected[0].z+projected[0].w+projected[0].v, 0.000001);
            proj[0][i].x = projected[0].x / d[0][i] * aspectRatio;
            proj[0][i].y = projected[0].y / d[0][i];
            if (enableStereoGraphic) {
                proj[0][i].x += 0.5;
                view.h.v1 += eyeWidth;
                projected[1] = view * points[i];
                view.h.v1 -= eyeWidth;
                d[1][i] = max(projected[1].z+projected[1].w+projected[1].v, 0.000001);
                proj[1][i].x = projected[1].x / d[1][i] * aspectRatio - 0.5;
                proj[1][i].y = projected[1].y / d[1][i];
            };
        }
        if (faceMode) {
            pipelineTriangleFan->Bind(cmdBuf);
            u8 planes[24][4] = {
                { 0,  1,  2,  3},
                { 4,  5,  6,  7},
                { 8,  9, 10, 11},
                {12, 13, 14, 15},

                { 0,  2,  4,  6},
                { 8, 10, 12, 14},
                { 1,  3,  5,  7},
                { 9, 11, 13, 15},

                { 0,  4,  8, 12},
                { 1,  5,  9, 13},
                { 2,  6, 10, 14},
                { 3,  7, 11, 15},

                { 0,  8,  1,  9},
                { 2, 10,  3, 11},
                { 4, 12,  5, 13},
                { 6, 14,  7, 15},

                { 0,  8,  2, 10},
                { 4, 12,  6, 14},
                { 1,  9,  3, 11},
                { 5, 13,  7, 15},

                { 0,  1,  4,  5},
                { 8,  9, 12, 13},
                { 2,  3,  6,  7},
                {10, 11, 14, 15},

            };
            for (u32 eye = 0; eye < (enableStereoGraphic ? 2 : 1); eye++) {
                for (u32 i = 0; i < 24; i++) {
                    const u32 index[4] = {0, 1, 3, 2};
                    vec2 facePoints[4];
                    vec4 colors[4];
                    f32 depths[4];
                    for (u32 ii = 0; ii < 4; ii++) {
                        facePoints[ii] = proj[eye][planes[i][index[ii]]];
                        colors[ii].a = 0.25;
                        colors[ii].rgb = hsvToRgb(vec3(
                            (f32)planes[i][index[ii]] / 16.0,
                            clamp(1.0/d[eye][planes[i][index[ii]]]*4.0, 0.0, 1.0),
                            1.0
                        ));
                        depths[ii] = d[eye][planes[i][index[ii]]];
                    }
                    DrawQuad(cmdBuf, vertices, &vertex, facePoints, colors, depths);
                }
            }
        }

        pipelineLines->Bind(cmdBuf);
        for (u32 i = 0; i < 16; i++) {
            for (u32 ii = i+1; ii < 16; ii++) {
                for (u32 eye = 0; eye < (enableStereoGraphic ? 2 : 1); eye++) {
                    f32 a = abs(points[i]-points[ii]);
                    const f32 epsilon = 0.0001;
                    if (a == median(a, 2.0-epsilon, 2.0+epsilon)) {
                        vec4 color1(0.5);
                        color1.rgb = hsvToRgb(vec3((f32)i / 16.0, clamp(1.0/d[eye][i]*4.0, 0.0, 1.0), 1.0));
                        vec4 color2(0.5);
                        color2.rgb = hsvToRgb(vec3((f32)ii / 16.0, clamp(1.0/d[eye][ii]*4.0, 0.0, 1.0), 1.0));
                        vertices[vertex++] = {color1, proj[eye][i], d[eye][i]};
                        vertices[vertex++] = {color2, proj[eye][ii], d[eye][ii]};
                        vkCmdDraw(cmdBuf, 2, 1, vertex-2, 0);
                    }
                }
            }
        }
        pipelineTriangleFan->Bind(cmdBuf);

        for (u32 i = 0; i < 16; i++) {
            for (u32 eye = 0; eye < (enableStereoGraphic ? 2 : 1); eye++) {
                if (d[eye][i] > 0.001) {
                    DrawCircle(cmdBuf, vertices, &vertex, proj[eye][i]/vec2(aspectRatio, 1.0), 0.05/d[eye][i], vec4(1.0), aspectRatio, d[eye][i]);
                }
            }
        }

        vkCmdEndRenderPass(cmdBuf);

        vkCommandBufferAllDrawing->End();

        vkStagingBuffer->CopyData(vertices, sizeof(Vertex) * vertex);
        cmdBuf = vkCommandBufferTransfer->Begin();
        vkVertexBuffer->Copy(cmdBuf, vkStagingBuffer, sizeof(Vertex) * vertex);
        vkCommandBufferTransfer->End();

        queueSubmissionDraw->waitSemaphores[1] = {vkSwapchain->SemaphoreImageAvailable(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        if (!queueSubmissionDraw->Config()) {
            cout << "Failed to re-Config queueSubmissionDraw: " << vk::error << std::endl;
            return 1;
        }

        if (!device->SubmitCommandBuffers(queueTransfer, {queueSubmissionTransfer})) {
            cout << "Failed to sumbit transfer command buffers to transfer queue: " << vk::error << std::endl;
            return 1;
        }
        if (!device->SubmitCommandBuffers(queueGraphics, {queueSubmissionDraw})) {
            cout << "Failed to sumbit draw command buffers to graphics queue: " << vk::error << std::endl;
            return 1;
        }
        if (!vkSwapchain->Present(queuePresent, {semaphoreRenderFinished->semaphore})) {
            cout << "Failed to present: " << vk::error << std::endl;
            return 1;
        }

        vkDeviceWaitIdle(device->data.device);

    }
    instance.Deinit();
    window.Close();

    delete[] vertices;

    return 0;
}
