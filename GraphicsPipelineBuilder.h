#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>

class GraphicsPipelineBuilder final {

    // Descending size order (approximate for 64-bit):
// Vulkan create infos (largest, many pointers/fields)
    VkPipelineColorBlendStateCreateInfo colorBlend{};
    VkPipelineRasterizationStateCreateInfo raster{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    VkPipelineMultisampleStateCreateInfo multisample{};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

    // Optionals of large structs (wraps above structs; typically similar size/alignment)
    std::optional<VkPipelineDiscardRectangleStateCreateInfoEXT> discardRect;
    std::optional<VkPipelineRenderingCreateInfo> dynamicRenderingInfo; // VK_KHR_dynamic_rendering
    std::optional<VkPipelineDepthStencilStateCreateInfo> depthStencil{};
    std::optional<VkPipelineTessellationStateCreateInfo> tessellation{};

    // STL containers (dynamic, but object header moderately sized; place after large PODs)
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    std::vector<VkDynamicState> dynamicStates;

    // Small PODs
    VkViewport viewport{}; // ~24 bytes
    VkRect2D scissor{};    // ~16 bytes

    // Handles/pointers (8 bytes)
    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    static constexpr const char* kDefaultEntryPoint = "main";

public:

    GraphicsPipelineBuilder() = default;

    GraphicsPipelineBuilder(const GraphicsPipelineBuilder&)
    {

    }

    GraphicsPipelineBuilder& operator=(const GraphicsPipelineBuilder& other)
    {
        if (this == &other)
        {
            return *this;
        }

        // Copy POD Vulkan create infos
        colorBlend = other.colorBlend;
        raster = other.raster;
        viewportState = other.viewportState;
        vertexInput = other.vertexInput;
        multisample = other.multisample;
        inputAssembly = other.inputAssembly;

        // Optionals
        discardRect = other.discardRect;
        dynamicRenderingInfo = other.dynamicRenderingInfo;
        depthStencil = other.depthStencil;
        tessellation = other.tessellation;

        // Containers
        shaderStages = other.shaderStages;
        colorBlendAttachments = other.colorBlendAttachments;
        dynamicStates = other.dynamicStates;

        // Small PODs
        viewport = other.viewport;
        scissor = other.scissor;

        // Handles/pointers
        device = other.device;
        renderPass = other.renderPass;
        pipelineLayout = other.pipelineLayout;
        pipeline = other.pipeline;

        return *this;
    }

    GraphicsPipelineBuilder& setDevice(VkDevice d) { device = d; return *this; }
    GraphicsPipelineBuilder& setRenderPass(VkRenderPass rp) { renderPass = rp; return *this; }
    GraphicsPipelineBuilder& setPipelineLayout(VkPipelineLayout layout) { pipelineLayout = layout; return *this; }

    GraphicsPipelineBuilder& addShaderStage(const VkPipelineShaderStageCreateInfo& stage) {
        shaderStages.push_back(stage); return *this;
    }
    // Helper: add vertex stage
    GraphicsPipelineBuilder& addVertexShader(VkShaderModule module, const char* entry = kDefaultEntryPoint) {
        VkPipelineShaderStageCreateInfo s{};
        s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        s.stage = VK_SHADER_STAGE_VERTEX_BIT;
        s.module = module;
        s.pName = entry;
        return addShaderStage(s);
    }
    // Helper: add fragment stage
    GraphicsPipelineBuilder& addFragmentShader(VkShaderModule module, const char* entry = kDefaultEntryPoint) {
        VkPipelineShaderStageCreateInfo s{};
        s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        s.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        s.module = module;
        s.pName = entry;
        return addShaderStage(s);
    }
    // Replace stages in one call
    GraphicsPipelineBuilder& setShaderStages(VkShaderModule vert, VkShaderModule frag, const char* entry = kDefaultEntryPoint) {
        shaderStages.clear();
        addVertexShader(vert, entry);
        addFragmentShader(frag, entry);
        return *this;
    }
    // Clear stages when reusing the builder
    GraphicsPipelineBuilder& clearShaderStages() { shaderStages.clear(); return *this; }

    GraphicsPipelineBuilder& setVertexInput(const VkPipelineVertexInputStateCreateInfo& vi) {
        vertexInput = vi; return *this;
    }

    GraphicsPipelineBuilder& setInputAssembly(VkPrimitiveTopology topology, VkBool32 primitiveRestart = VK_FALSE) {
        inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = primitiveRestart;
        return *this;
    }

    GraphicsPipelineBuilder& setViewport(float x, float y, float w, float h, float minDepth = 0.f, float maxDepth = 1.f) {
        viewport = { x, y, w, h, minDepth, maxDepth };
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = scissor.extent.width > 0 ? 1u : 0u;
        viewportState.pScissors = (viewportState.scissorCount > 0) ? &scissor : nullptr;
        return *this;
    }
    GraphicsPipelineBuilder& setScissor(const VkOffset2D& offset, const VkExtent2D& extent) {
        scissor = { offset, extent };
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = (viewport.width > 0 && viewport.height > 0) ? 1u : 0u;
        viewportState.pViewports = (viewportState.viewportCount > 0) ? &viewport : nullptr;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        return *this;
    }

    GraphicsPipelineBuilder& setRasterFill(VkPolygonMode mode, VkCullModeFlags cull, VkFrontFace frontFace, VkBool32 depthClamp = VK_FALSE) {
        raster = {};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.depthClampEnable = depthClamp;
        raster.rasterizerDiscardEnable = VK_FALSE;
        raster.polygonMode = mode;
        raster.cullMode = cull;
        raster.frontFace = frontFace;
        raster.depthBiasEnable = VK_FALSE;
        raster.lineWidth = 1.0f;
        return *this;
    }

    GraphicsPipelineBuilder& setMultisample(VkSampleCountFlagBits samples) {
        multisample = {};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = samples;
        multisample.sampleShadingEnable = VK_FALSE;
        return *this;
    }

    GraphicsPipelineBuilder& enableDepthTest(VkCompareOp compare = VK_COMPARE_OP_LESS_OR_EQUAL, VkBool32 write = VK_TRUE) {
        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = write;
        ds.depthCompareOp = compare;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.stencilTestEnable = VK_FALSE;
        depthStencil = ds;
        return *this;
    }
    GraphicsPipelineBuilder& disableDepth() { depthStencil.reset(); return *this; }

    GraphicsPipelineBuilder& addColorBlendAttachment(VkBool32 blendEnable = VK_FALSE,
        VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT, VkBlendFactor srcColor = VK_BLEND_FACTOR_SRC_ALPHA, VkBlendFactor dstColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VkBlendFactor srcAlpha = VK_BLEND_FACTOR_ONE, VkBlendFactor dstAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA) {
        VkPipelineColorBlendAttachmentState att{};
        att.blendEnable = blendEnable;
        att.colorWriteMask = writeMask;
        if (blendEnable) {
            att.srcColorBlendFactor = srcColor;
            att.dstColorBlendFactor = dstColor;
            att.colorBlendOp = VK_BLEND_OP_ADD;
            att.srcAlphaBlendFactor = srcAlpha;
            att.dstAlphaBlendFactor = dstAlpha;
            att.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        colorBlendAttachments.push_back(att);
        return *this;
    }
    GraphicsPipelineBuilder& setColorBlendLogic(VkBool32 logicEnable = VK_FALSE, VkLogicOp op = VK_LOGIC_OP_COPY) {
        colorBlend = {};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.logicOpEnable = logicEnable;
        colorBlend.logicOp = op;
        colorBlend.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
        colorBlend.pAttachments = colorBlendAttachments.empty() ? nullptr : colorBlendAttachments.data();
        return *this;
    }

    GraphicsPipelineBuilder& setDynamicStates(const std::vector<VkDynamicState>& states) {
        dynamicStates = states;
        return *this;
    }

    GraphicsPipelineBuilder& setDynamicRendering(const VkPipelineRenderingCreateInfo& info) {
        dynamicRenderingInfo = info;
        return *this;
    }

    VkPipeline build() {
        if (device == VK_NULL_HANDLE || pipelineLayout == VK_NULL_HANDLE) {
            throw std::runtime_error("GraphicsPipelineBuilder: device and pipelineLayout must be set.");
        }
        if (shaderStages.empty()) {
            throw std::runtime_error("GraphicsPipelineBuilder: at least one shader stage required.");
        }
        if (!dynamicRenderingInfo.has_value() && renderPass == VK_NULL_HANDLE) {
            throw std::runtime_error("GraphicsPipelineBuilder: renderPass must be set unless using dynamic rendering.");
        }

        if (inputAssembly.sType == 0) setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        if (raster.sType == 0) setRasterFill(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        if (multisample.sType == 0) setMultisample(VK_SAMPLE_COUNT_1_BIT);
        if (colorBlend.sType == 0) {
            if (colorBlendAttachments.empty()) addColorBlendAttachment(VK_FALSE);
            setColorBlendLogic(VK_FALSE);
        }
        if (viewportState.sType == 0) {
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;
            dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        }

        VkPipelineDynamicStateCreateInfo dyn{};
        dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyn.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dyn.pDynamicStates = dynamicStates.empty() ? nullptr : dynamicStates.data();

        VkPipelineViewportStateCreateInfo vp = viewportState;
        vp.pNext = nullptr;

        VkPipelineTessellationStateCreateInfo* pTess = nullptr;
        if (tessellation.has_value()) {
            VkPipelineTessellationStateCreateInfo tess{};
            tess = *tessellation;
            pTess = &tess;
        }

        VkPipelineDepthStencilStateCreateInfo* pDS = nullptr;
        if (depthStencil.has_value()) {
            VkPipelineDepthStencilStateCreateInfo ds{};
            ds = *depthStencil;
            pDS = &ds;
        }

        VkGraphicsPipelineCreateInfo gpci{};
        gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gpci.stageCount = static_cast<uint32_t>(shaderStages.size());
        gpci.pStages = shaderStages.data();
        gpci.pVertexInputState = &vertexInput;
        gpci.pInputAssemblyState = &inputAssembly;
        gpci.pTessellationState = pTess;
        gpci.pViewportState = &vp;
        gpci.pRasterizationState = &raster;
        gpci.pMultisampleState = &multisample;
        gpci.pDepthStencilState = pDS;
        gpci.pColorBlendState = &colorBlend;
        gpci.pDynamicState = dynamicStates.empty() ? nullptr : &dyn;
        gpci.layout = pipelineLayout;
        gpci.renderPass = renderPass;
        gpci.subpass = 0;

        if (dynamicRenderingInfo.has_value()) {
            VkPipelineRenderingCreateInfo renderingInfo{};
            renderingInfo = *dynamicRenderingInfo;
            renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
            gpci.pNext = &renderingInfo;
            gpci.renderPass = VK_NULL_HANDLE;
        }

        const VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpci, nullptr, &pipeline);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("vkCreateGraphicsPipelines failed.");
        }
        return pipeline;
    }

    VkPipelineVertexInputStateCreateInfo MakeDefaultVertexInput(
        const std::vector<VkVertexInputBindingDescription>& bindings,
        const std::vector<VkVertexInputAttributeDescription>& attrs) const
    {
        static VkPipelineVertexInputStateCreateInfo vi;
        vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vi.pVertexBindingDescriptions = bindings.data();
        vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
        vi.pVertexAttributeDescriptions = attrs.data();
        return vi;
    }
};