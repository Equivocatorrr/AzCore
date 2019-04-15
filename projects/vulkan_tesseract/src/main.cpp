/*
    File: main.cpp
    Author: Philip Haynes
    Description: High-level definition of the structure of our program.
*/

#include "AzCore/io.hpp"
#include "AzCore/vk.hpp"

io::logStream cout("main.log");

const u32 maxVertices = 8192;
// const u32 totalPoints = 1024;
// const u32 threads = 4;

struct Vertex {
    vec4 color;
    vec2 pos;
    f32 depth;
};

void DrawCircle(VkCommandBuffer *cmdBuf, Vertex *vertices, u32 *vertex, vec2 center, f32 radius, vec4 color, f32 aspectRatio, f32 depth) {
    u32 circumference = min(max(sqrt(radius*tau*1200.0), 4.0), 80.0);
    center.x *= aspectRatio;
    vertices[(*vertex)++] = {color, center};
    for (u32 i = 0; i <= circumference; i++) {
        f32 angle = (f32)i * tau / (f32)circumference;
        vertices[(*vertex)++] = {color, center + vec2(sin(angle)*radius*aspectRatio, cos(angle)*radius), depth};
    }
    vkCmdDraw(*cmdBuf, circumference+2, 1, *vertex - circumference - 2, 0);
}

void DrawLine(VkCommandBuffer *cmdBuf, Vertex *vertices, u32 *vertex, vec2 p1, vec2 p2, vec4 color, f32 depth) {
    vertices[(*vertex)++] = {color, p1, depth};
    vertices[(*vertex)++] = {color, p2, depth};
    vkCmdDraw(*cmdBuf, 2, 1, *vertex - 2, 0);
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
    vkAttachment->bufferDepthStencil = true;
    vkAttachment->clearDepth = true;
    vkAttachment->clearDepthStencilValue.depth = 1000.0;
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
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
    pipelineTriangleFan->inputBindingDescriptions = {inputBindingDescription};
    pipelineTriangleFan->inputAttributeDescriptions = inputAttributeDescriptions;
    pipelineTriangleFan->colorBlendAttachments.Append(colorBlendAttachment);
    pipelineTriangleFan->depthStencil.depthTestEnable = VK_TRUE;
    pipelineTriangleFan->depthStencil.depthWriteEnable = VK_TRUE;

    Ptr<vk::Pipeline> pipelineLines = device->AddPipeline();
    pipelineLines->renderPass = vkRenderPass;
    pipelineLines->subpass = 0;
    pipelineLines->shaders = vkShaderRefs;
    pipelineLines->dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pipelineLines->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pipelineLines->inputBindingDescriptions = {inputBindingDescription};
    pipelineLines->inputAttributeDescriptions = inputAttributeDescriptions;
    pipelineLines->colorBlendAttachments.Append(colorBlendAttachment);
    pipelineLines->rasterizer.lineWidth = 3.0;
    pipelineLines->depthStencil.depthTestEnable = VK_TRUE;
    pipelineLines->depthStencil.depthWriteEnable = VK_TRUE;

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

    // Ptr<vk::CommandPool> vkCommandPoolDraw[threads];
    // Ptr<vk::CommandBuffer> vkCommandBufferDraw[threads];
    // for (u32 i = 0; i < threads; i++) {
    //     vkCommandPoolDraw[i] = device->AddCommandPool(queueGraphics);
    //     vkCommandPoolDraw[i]->transient = true;
    //     vkCommandPoolDraw[i]->resettable = true;
    //     vkCommandBufferDraw[i] = vkCommandPoolDraw[i]->AddCommandBuffer();
    //     vkCommandBufferDraw[i]->secondary = true;
    //     vkCommandBufferDraw[i]->oneTimeSubmit = true;
    //     vkCommandBufferDraw[i]->renderPassContinue = true;
    //     vkCommandBufferDraw[i]->renderPass = vkRenderPass;
    //     vkCommandBufferDraw[i]->framebuffer = vkFramebuffer;
    // }

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

    RandomNumberGenerator rng;

    // vec2 *linePoints = new vec2[totalPoints];
    // vec2 *linePointVelocities = new vec2[totalPoints];
    // vec4 *linePointColors = new vec4[totalPoints];
    // for (u32 i = 0; i < totalPoints; i++) {
    //     linePoints[i] = {random(-1.0, 1.0, rng), random(-1.0, 1.0, rng)};
    //     f32 angle = random(0.0, tau, rng);
    //     linePointVelocities[i] = vec2(sin(angle), cos(angle)) * random(0.001, 0.01, rng);
    //     linePointColors[i].rgb = hsvToRgb(vec3(random(0.0, 1.0, rng), sqrt(random(0.0, 1.0, rng)), sqrt(random(0.0, 1.0, rng))));
    //     linePointColors[i].a = 1.0;
    // }

    // bool first = true;
    f32 rotateAngle = 0.0;

    vec3 offset(0.0);
    offset.z = 3.0;

    while (window.Update()) {
        if (input.Pressed(KC_KEY_ESC)) {
            break;
        }

        if (window.resized) {
            // first = true;
            if (!vkSwapchain->Resize()) {
                cout << "Failed to resize vkSwapchain: " << vk::error << std::endl;
                return 1;
            }
        }

        VkResult acquisitionResult = vkSwapchain->AcquireNextImage();

        if (acquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || acquisitionResult == VK_NOT_READY) {
            cout << "Skipping a frame because acquisition returned: " << vk::ErrorString(acquisitionResult) << std::endl;
            // first = true;
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

        pipelineTriangleFan->Bind(cmdBuf);

        // vertices[vertex++] = {{0.0, 0.1, 0.2, 0.5}, {-1.0, -1.0}};
        // vertices[vertex++] = {{0.0, 0.1, 0.2, 0.5}, {-1.0,  1.0}};
        // vertices[vertex++] = {{0.0, 0.1, 0.2, 0.5}, { 1.0,  1.0}};
        // vertices[vertex++] = {{0.0, 0.1, 0.2, 0.5}, { 1.0, -1.0}};
        // vkCmdDraw(cmdBuf, 4, 1, vertex-4, 0);

        f32 aspectRatio = (f32)window.height / (f32)window.width;

        vec5 points[16] = {
            {-1.0, -1.0, -1.0, -1.0,  1.0},
            {-1.0,  1.0, -1.0, -1.0,  1.0},
            { 1.0, -1.0, -1.0, -1.0,  1.0},
            { 1.0,  1.0, -1.0, -1.0,  1.0},
            {-1.0, -1.0,  1.0, -1.0,  1.0},
            {-1.0,  1.0,  1.0, -1.0,  1.0},
            { 1.0, -1.0,  1.0, -1.0,  1.0},
            { 1.0,  1.0,  1.0, -1.0,  1.0},
            {-1.0, -1.0, -1.0,  1.0,  1.0},
            {-1.0,  1.0, -1.0,  1.0,  1.0},
            { 1.0, -1.0, -1.0,  1.0,  1.0},
            { 1.0,  1.0, -1.0,  1.0,  1.0},
            {-1.0, -1.0,  1.0,  1.0,  1.0},
            {-1.0,  1.0,  1.0,  1.0,  1.0},
            { 1.0, -1.0,  1.0,  1.0,  1.0},
            { 1.0,  1.0,  1.0,  1.0,  1.0}
        };
        mat5 view;
        view = view.RotateBasic(rotateAngle, Plane::YW).RotateBasic(rotateAngle*2.0, Plane::XZ);
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

        rotateAngle += 0.004;

        vec2 proj[16];
        f32 d[16];

        for (u32 i = 0; i < 16; i++) {
            points[i] *= 0.5;
            vec5 projected = view * points[i];
            f32 depth = max(projected.z+projected.w+projected.v, 0.001);
            proj[i].x = projected.x / depth * aspectRatio;
            proj[i].y = projected.y / depth;
            d[i] = depth;
        }
        pipelineLines->Bind(cmdBuf);
        for (u32 i = 0; i < 16; i++) {
            for (u32 ii = i+1; ii < 16; ii++) {
                f32 a = abs(points[i]-points[ii]);
                const f32 epsilon = 0.0001;
                if (a == median(a, 1.0-epsilon, 1.0+epsilon)) {
                    vec4 color1(1.0);
                    color1.rgb = hsvToRgb(vec3((f32)i / 16.0, clamp(1.0/d[i]*2.0, 0.0, 1.0), 1.0));
                    vec4 color2(1.0);
                    color2.rgb = hsvToRgb(vec3((f32)ii / 16.0, clamp(1.0/d[ii]*2.0, 0.0, 1.0), 1.0));
                    vertices[vertex++] = {color1, proj[i], d[i]};
                    vertices[vertex++] = {color2, proj[ii], d[ii]};
                    vkCmdDraw(cmdBuf, 2, 1, vertex-2, 0);
                }
            }
        }

        pipelineTriangleFan->Bind(cmdBuf);

        for (u32 i = 0; i < 16; i++) {
            if (d[i] > 0.01) {
                DrawCircle(&cmdBuf, vertices, &vertex, proj[i]/vec2(aspectRatio, 1.0), 0.03/d[i], vec4(1.0), aspectRatio, d[i]);
            }
        }

        // VkCommandBuffer secondaryBuffers[threads];
        // Thread thread[threads];
        // for (u32 i = 0; i < threads; i++) {
        //     secondaryBuffers[i] = vkCommandBufferDraw[i]->Begin();
        //     if (secondaryBuffers[i] == VK_NULL_HANDLE) {
        //         cout << "Failed to begin secondary command buffer " << i << ": " << vk::error << std::endl;
        //         return 1;
        //     }
        //     thread[i] = Thread(threadProc, secondaryBuffers[i], i, vertices, pipelineLines, pipelineTriangleFan, linePointColors, linePoints, linePointVelocities, window.width, window.height, vkVertexBuffer->data.buffer);
        // }
        // for (u32 i = 0 ; i < threads; i++) {
        //     if (thread[i].joinable()) {
        //         thread[i].join();
        //     }
        //     vkCommandBufferDraw[i]->End();
        // }
        // vkCmdExecuteCommands(cmdBuf, threads, secondaryBuffers);

        // pipelineTriangleFan->Bind(cmdBuf);

        // vertices[vertex++] = {{1.0, 1.0, 1.0, 1.0}, {0.0, 0.0}};
        // for (i32 a = 0; a <= 50; a++) {
        //     f32 angle = (f32)a * tau / 50.0;
        //     vec3 color = hsvToRgb(vec3(angle/tau, 1.0, 1.0));
        //     vertices[vertex++] = {{color.r, color.g, color.b, 1.0}, {sin(angle)*0.5, cos(angle)*0.5}};
        // }
        // vkCmdDraw(cmdBuf, 52, 1, vertex-52, 0);

        // pipelineTriangleFan->Bind(cmdBuf);
        // for (u32 i = 0; i < totalPoints; i++) {
        //     DrawCircle(&cmdBuf, vertices, &vertex, linePoints[i], 0.015, linePointColors[i]);
        // }

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
    // delete[] linePoints;
    // delete[] linePointVelocities;
    // delete[] linePointColors;

    return 0;
}
