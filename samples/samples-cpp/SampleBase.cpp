#include <stdio.h>
#include <vector>
#include <memory>
#include "SampleBase.hpp"
#include "SDL_syswm.h"

agpu_vertex_attrib_description SampleVertex::Description[] = {
    {0, 0, AGPU_TEXTURE_FORMAT_R32G32B32_FLOAT, 1, offsetof(SampleVertex, position), 0},
    {0, 1, AGPU_TEXTURE_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(SampleVertex, color), 0},
    {0, 2, AGPU_TEXTURE_FORMAT_R32G32B32_FLOAT, 1, offsetof(SampleVertex, normal), 0},
    {0, 3, AGPU_TEXTURE_FORMAT_R32G32_FLOAT, 1, offsetof(SampleVertex, texcoord), 0},
};

const int SampleVertex::DescriptionSize = 4;

void printMessage(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 1024, format, args);
#ifdef _WIN32
    OutputDebugStringA(buffer);
#else
    fputs(buffer, stdout);
#endif
    va_end(args);
}

void printError(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 1024, format, args);
#ifdef _WIN32
    OutputDebugStringA(buffer);
#else
    fputs(buffer, stderr);
#endif
    va_end(args);
}

std::string readWholeFile(const std::string &fileName)
{
    FILE *file = fopen(fileName.c_str(), "rb");
    if(!file)
    {
        printError("Failed to open file %s\n", fileName.c_str());
        return std::string();
    }

    // Allocate the data.
    std::vector<char> data;
    fseek(file, 0, SEEK_END);
    data.resize(ftell(file));
    fseek(file, 0, SEEK_SET);

    // Read the file
    if(fread(&data[0], data.size(), 1, file) != 1)
    {
        printError("Failed to read file %s\n", fileName.c_str());
        fclose(file);
        return std::string();
    }

    fclose(file);
    return std::string(data.begin(), data.end());
}

agpu_shader_ref AbstractSampleBase::compileShaderFromFile(const char *fileName, agpu_shader_type type)
{
    // Read the source file
    std::string fullName = fileName;
    switch (preferredShaderLanguage)
    {
    case AGPU_SHADER_LANGUAGE_GLSL:
        fullName += ".glsl";
        break;
    case AGPU_SHADER_LANGUAGE_HLSL:
        fullName += ".hlsl";
        break;
    case AGPU_SHADER_LANGUAGE_BINARY:
        fullName += ".cso";
        break;
    case AGPU_SHADER_LANGUAGE_SPIR_V:
        fullName += ".spv";
        break;
    case AGPU_SHADER_LANGUAGE_METAL:
        fullName += ".metal";
        break;
    default:
        break;
    }

    auto source = readWholeFile(fullName);
    if(source.empty())
        return nullptr;

    // Create the shader and compile it.
    agpu_shader_ref shader = device->createShader(type);
    shader->setShaderSource(preferredShaderLanguage, source.c_str(), (agpu_string_length)source.size());
    try
    {
        shader->compileShader(nullptr);
    }
    catch(agpu_exception &e)
    {
        auto logLength = shader->getCompilationLogLength();
        std::unique_ptr<char[]> logBuffer(new char[logLength+1]);
        shader->getCompilationLog(logLength+1, logBuffer.get());
        printError("Compilation error of '%s':%s\n", fullName.c_str(), logBuffer.get());
        return nullptr;
    }

    return shader;
}

agpu_pipeline_state_ref AbstractSampleBase::buildPipeline(const agpu_pipeline_builder_ref &builder)
{
    agpu_pipeline_state_ref pipeline = builder->build();

    // Check the link result.
    if(!pipeline)
    {
        auto logLength = builder->getBuildingLogLength();
        std::unique_ptr<char[]> logBuffer(new char[logLength + 1]);
        builder->getBuildingLog(logLength+1, logBuffer.get());
        printError("Pipeline building error: %s\n", logBuffer.get());
        return nullptr;
    }

    return pipeline;
}

agpu_pipeline_state_ref AbstractSampleBase::buildComputePipeline(const agpu_compute_pipeline_builder_ref &builder)
{
    agpu_pipeline_state_ref pipeline = builder->build();

	// Check the link result.
	if (!pipeline)
	{
		auto logLength = builder->getBuildingLogLength();
		std::unique_ptr<char[]> logBuffer(new char[logLength + 1]);
        builder->getBuildingLog(logLength+1, logBuffer.get());
		printError("Pipeline building error: %s\n", logBuffer.get());
		return nullptr;
	}

	return pipeline;
}

agpu_buffer_ref AbstractSampleBase::createImmutableVertexBuffer(size_t capacity, size_t vertexSize, void *initialData)
{
    agpu_buffer_description desc;
    desc.size = agpu_uint(capacity * vertexSize);
    desc.usage = AGPU_STATIC;
    desc.binding = AGPU_ARRAY_BUFFER;
    desc.mapping_flags = 0;
    desc.stride = agpu_uint(vertexSize);
    return device->createBuffer(&desc, initialData);
}

agpu_buffer_ref AbstractSampleBase::createImmutableIndexBuffer(size_t capacity, size_t indexSize, void *initialData)
{
    agpu_buffer_description desc;
    desc.size = agpu_uint(capacity * indexSize);
    desc.usage = AGPU_STATIC;
    desc.binding = AGPU_ELEMENT_ARRAY_BUFFER;
    desc.mapping_flags = 0;
    desc.stride = agpu_uint(indexSize);
    return device->createBuffer(&desc, initialData);
}

agpu_buffer_ref AbstractSampleBase::createImmutableDrawBuffer(size_t capacity, void *initialData)
{
    size_t commandSize = sizeof(agpu_draw_elements_command);
    agpu_buffer_description desc;
    desc.size = agpu_uint(capacity * commandSize);
    desc.usage = AGPU_STATIC;
    desc.binding = AGPU_DRAW_INDIRECT_BUFFER;
    desc.mapping_flags = 0;
    desc.stride = agpu_uint(commandSize);
    return device->createBuffer(&desc, initialData);
}

agpu_buffer_ref AbstractSampleBase::createUploadableUniformBuffer(size_t capacity, void *initialData)
{
    agpu_buffer_description desc;
    desc.size = agpu_uint(capacity);
    desc.usage = AGPU_DYNAMIC;
    desc.binding = AGPU_UNIFORM_BUFFER;
    desc.mapping_flags = AGPU_MAP_DYNAMIC_STORAGE_BIT | AGPU_MAP_WRITE_BIT;
    desc.stride = 0;
    return device->createBuffer(&desc, initialData);
}

agpu_buffer_ref AbstractSampleBase::createMappableStorage(size_t capacity, void *initialData)
{
	agpu_buffer_description desc;
	desc.size = agpu_uint(capacity);
	desc.usage = AGPU_DYNAMIC;
	desc.binding = AGPU_STORAGE_BUFFER;
	desc.mapping_flags = AGPU_MAP_WRITE_BIT | AGPU_MAP_READ_BIT;
	desc.stride = 0;
	if (hasPersistentCoherentMapping)
		desc.mapping_flags |= AGPU_MAP_PERSISTENT_BIT | AGPU_MAP_COHERENT_BIT;

	return device->createBuffer(&desc, initialData);
}

agpu_texture_ref AbstractSampleBase::loadTexture(const char *fileName)
{
    auto surface = SDL_LoadBMP(fileName);
    if (!surface)
        return nullptr;

    auto convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(surface);
    if (!convertedSurface)
        return nullptr;

    agpu_texture_description desc;
    memset(&desc, 0, sizeof(desc));
    desc.type = AGPU_TEXTURE_2D;
    desc.format = AGPU_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    desc.width = convertedSurface->w;
    desc.height = convertedSurface->h;
    desc.depthOrArraySize = 1;
    desc.miplevels = 1;
    desc.sample_count = 1;
    desc.sample_quality = 0;
    desc.flags = AGPU_TEXTURE_FLAG_UPLOADED;
    agpu_texture_ref texture = device->createTexture(&desc);
    if (!texture)
        return nullptr;

    texture->uploadTextureData(0, 0, convertedSurface->pitch, convertedSurface->pitch*convertedSurface->h, convertedSurface->pixels);
    SDL_FreeSurface(convertedSurface);

    return texture;
}

int SampleBase::main(int argc, const char **argv)
{
    char nameBuffer[256];
    SDL_Init(SDL_INIT_VIDEO);

    screenWidth = 640;
    screenHeight = 480;

    int flags = 0;

    // Get the platform.
    agpu_platform *platform;
    agpuGetPlatforms(1, &platform, nullptr);
    if (!platform)
    {
        printError("Failed to get AGPU platform\n");
        return -1;
    }

    printMessage("Choosen platform: %s\n", agpuGetPlatformName(platform));
    snprintf(nameBuffer, sizeof(nameBuffer), "AGPU Sample - %s Platform", agpuGetPlatformName(platform));
    SDL_Window * window = SDL_CreateWindow(nameBuffer, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, flags);
    if(!window)
    {
        printError("Failed to open window\n");
        return -1;
    }

    // Get the window info.
    SDL_SysWMinfo windowInfo;
    SDL_VERSION(&windowInfo.version);
    SDL_GetWindowWMInfo(window, &windowInfo);

    // Open the device
    agpu_swap_chain_create_info swapChainCreateInfo;
    agpu_device_open_info openInfo;
    memset(&openInfo, 0, sizeof(openInfo));
    memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
    switch(windowInfo.subsystem)
    {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
    case SDL_SYSWM_WINDOWS:
        swapChainCreateInfo.window = (agpu_pointer)windowInfo.info.win.window;
        break;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
    case SDL_SYSWM_X11:
        openInfo.display = (agpu_pointer)windowInfo.info.x11.display;
        swapChainCreateInfo.window = (agpu_pointer)(uintptr_t)windowInfo.info.x11.window;
        break;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
    case SDL_SYSWM_COCOA:
        swapChainCreateInfo.window = (agpu_pointer)windowInfo.info.cocoa.window;
        break;
#endif
    default:
        printError("Unsupported window system\n");
        return -1;
    }

    swapChainCreateInfo.colorbuffer_format = AGPU_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    swapChainCreateInfo.depth_stencil_format = AGPU_TEXTURE_FORMAT_D32_FLOAT_S8X24_UINT;
    swapChainCreateInfo.width = screenWidth;
    swapChainCreateInfo.height = screenHeight;
    swapChainCreateInfo.buffer_count = 3;
#ifdef _DEBUG
    // Use the debug layer when debugging. This is useful for low level backends.
    openInfo.debug_layer= true;
#endif

    device = platform->openDevice(&openInfo);
    if(!device)
    {
        printError("Failed to open the device\n");
        return false;
    }

	hasPersistentCoherentMapping = device->isFeatureSupported(AGPU_FEATURE_PERSISTENT_COHERENT_MEMORY_MAPPING);

    // Get the default command queue
    commandQueue = device->getDefaultCommandQueue();

    // Create the swap chain.
    swapChain = device->createSwapChain(commandQueue.get(), &swapChainCreateInfo);
    if(!swapChain)
    {
        printError("Failed to create the swap chain\n");
        return false;
    }

    // Get the preferred shader language.
    preferredShaderLanguage = device->getPreferredIntermediateShaderLanguage();
    if(preferredShaderLanguage == AGPU_SHADER_LANGUAGE_NONE)
    {
        preferredShaderLanguage = device->getPreferredHighLevelShaderLanguage();
        if(preferredShaderLanguage == AGPU_SHADER_LANGUAGE_NONE)
            preferredShaderLanguage = device->getPreferredShaderLanguage();
    }

    if(!initializeSample())
        return -1;

    quit = false;
    while(!quit)
    {
        processEvents();
        render();

        SDL_Delay(3);
    }

    shutdownSample();
    swapChain.reset();
    commandQueue.reset();
    device.reset();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void SampleBase::processEvents()
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_MOUSEBUTTONDOWN:
            printMessage("Mouse down\n");
            break;
        case SDL_FINGERDOWN:
            printMessage("Finger down\n");
            break;
        case SDL_KEYDOWN:
            onKeyDown(event);
            break;
        case SDL_KEYUP:
            onKeyUp(event);
            break;
        case SDL_QUIT:
            quit = true;
            break;
        default:
            //ignore
            break;
        }
    }
}

void SampleBase::onKeyDown(const SDL_Event &event)
{
    switch(event.key.keysym.sym)
    {
    case SDLK_ESCAPE:
        quit = true;
        break;
    default:
        // ignore
        break;
    }
}

void SampleBase::onKeyUp(const SDL_Event &event)
{
}

bool SampleBase::initializeSample()
{
    return true;
}

void SampleBase::shutdownSample()
{
}

void SampleBase::render()
{
}

void SampleBase::swapBuffers()
{
    swapChain->swapBuffers();
}

agpu_renderpass_ref SampleBase::createMainPass(const glm::vec4 &clearColor)
{
    // Color attachment
    agpu_renderpass_color_attachment_description colorAttachment;
    memset(&colorAttachment, 0, sizeof(colorAttachment));
    colorAttachment.format = AGPU_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.begin_action = AGPU_ATTACHMENT_CLEAR;
    colorAttachment.end_action = AGPU_ATTACHMENT_KEEP;
    colorAttachment.clear_value.r = clearColor.r;
    colorAttachment.clear_value.g = clearColor.g;
    colorAttachment.clear_value.b = clearColor.b;
    colorAttachment.clear_value.a = clearColor.a;

    // Depth stencil
    agpu_renderpass_depth_stencil_description depthStencil;
    memset(&depthStencil, 0, sizeof(depthStencil));
    depthStencil.format = AGPU_TEXTURE_FORMAT_D32_FLOAT_S8X24_UINT;
    depthStencil.begin_action = AGPU_ATTACHMENT_CLEAR;
    depthStencil.end_action = AGPU_ATTACHMENT_KEEP;
    depthStencil.clear_value.depth = 1.0;

    agpu_renderpass_description description;
    memset(&description, 0, sizeof(description));
    description.color_attachment_count = 1;
    description.color_attachments = &colorAttachment;
    description.depth_stencil_attachment = &depthStencil;

    agpu_renderpass_ref result = device->createRenderPass(&description);
    return result;
}


int ComputeSampleBase::main(int argc, const char **argv)
{
    // Get the platform.
    agpu_platform *platform;
    agpuGetPlatforms(1, &platform, nullptr);
    if (!platform)
    {
        printError("Failed to get AGPU platform\n");
        return -1;
    }

    printMessage("Choosen platform: %s\n", agpuGetPlatformName(platform));

    // Open the device
    agpu_device_open_info openInfo;
    memset(&openInfo, 0, sizeof(openInfo));
#ifdef _DEBUG
    // Use the debug layer when debugging. This is useful for low level backends.
    openInfo.debug_layer= true;
#endif

    device = platform->openDevice(&openInfo);
    if(!device)
    {
        printError("Failed to open the device\n");
        return false;
    }

	hasPersistentCoherentMapping = device->isFeatureSupported(AGPU_FEATURE_PERSISTENT_COHERENT_MEMORY_MAPPING);

    // Get the default command queue
    commandQueue = device->getDefaultCommandQueue();

    // Get the preferred shader language.
    preferredShaderLanguage = device->getPreferredIntermediateShaderLanguage();
    if(preferredShaderLanguage == AGPU_SHADER_LANGUAGE_NONE)
    {
        preferredShaderLanguage = device->getPreferredHighLevelShaderLanguage();
        if(preferredShaderLanguage == AGPU_SHADER_LANGUAGE_NONE)
            preferredShaderLanguage = device->getPreferredShaderLanguage();
    }

    return run(argc, argv);
}

int ComputeSampleBase::run(int argc, const char **argv)
{
    return 0;
}
