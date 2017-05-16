#pragma once

#include <cstdio>
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vk_sdk_platform.h>

#include "linmath.h"
#include "gettime.h"

#define APP_SHORT_NAME "cubeVR"
#define APP_LONG_NAME "The Vulkan Cube VR Demo Program"
#define DEMO_TEXTURE_COUNT 1
#define FRAME_LAG 2
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define ERR_EXIT(err_msg, err_class)                                             \
    do {                                                                         \
        if (!app->suppress_popups) MessageBox(NULL, err_msg, err_class, MB_OK); \
        exit(1);                                                                 \
    } while (0)
void DbgMsg(char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	printf(fmt, va);
	fflush(stdout);
	va_end(va);
}

static void* pvrcube;

static bool in_callback;

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                                                              \
    {                                                                                                         \
        app->fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);             \
        if (app->fp##entrypoint == NULL) {                                                                   \
            ERR_EXIT(L"vkGetInstanceProcAddr failed to find vk" TEXT(#entrypoint), L"vkGetInstanceProcAddr Failure"); \
        }                                                                                                     \
    }

static PFN_vkGetDeviceProcAddr g_gdpa = NULL;

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                                                    \
    {                                                                                                            \
        if (!g_gdpa) g_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(app->inst, "vkGetDeviceProcAddr"); \
        app->fp##entrypoint = (PFN_vk##entrypoint)g_gdpa(dev, "vk" #entrypoint);                                \
        if (app->fp##entrypoint == NULL) {                                                                      \
            ERR_EXIT(L"vkGetDeviceProcAddr failed to find vk" TEXT(#entrypoint), L"vkGetDeviceProcAddr Failure");        \
        }                                                                                                        \
    }

struct texture_object {
	VkSampler sampler;

	VkImage image;
	VkImageLayout imageLayout;

	VkMemoryAllocateInfo mem_alloc;
	VkDeviceMemory mem;
	VkImageView view;
	uint32_t tex_width, tex_height;
};

static char *tex_files[] = { "lunarg.ppm" };

static int validation_error = 0;

struct vktexcube_vs_uniform {
	// Must start with MVP
	float mvp[4][4];
	float position[12 * 3][4];
	float attr[12 * 3][4];
};

typedef struct {
	VkImage image;
	VkCommandBuffer cmd;
	VkCommandBuffer graphics_to_present_cmd;
	VkImageView view;
	VkBuffer uniform_buffer;
	VkDeviceMemory uniform_memory;
	VkFramebuffer framebuffer;
	VkDescriptorSet descriptor_set;
	VkFence fence;
} SwapchainImageResources;

static const float g_vertex_buffer_data[] = {
	-1.0f,-1.0f,-1.0f,  // -X side
	-1.0f,-1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,

	-1.0f,-1.0f,-1.0f,  // -Z side
	1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,

	-1.0f,-1.0f,-1.0f,  // -Y side
	1.0f,-1.0f,-1.0f,
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,

	-1.0f, 1.0f,-1.0f,  // +Y side
	-1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f,-1.0f,

	1.0f, 1.0f,-1.0f,  // +X side
	1.0f, 1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,

	-1.0f, 1.0f, 1.0f,  // +Z side
	-1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
	0.0f, 1.0f,  // -X side
	1.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,

	1.0f, 1.0f,  // -Z side
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,

	1.0f, 0.0f,  // -Y side
	1.0f, 1.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	0.0f, 0.0f,

	1.0f, 0.0f,  // +Y side
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,

	1.0f, 0.0f,  // +X side
	0.0f, 0.0f,
	0.0f, 1.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,

	0.0f, 0.0f,  // +Z side
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
};

LRESULT CALLBACK WndProc_L(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_R(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class VkApp
{
public:
#define APP_NAME_STR_LEN 80

	VkApp();
	~VkApp();

	HINSTANCE connection;         // hInstance - Windows Instance
	LPCWSTR name;  // Name to put on the window/icon
	HWND window_l, window_r;                  // hWnd - window handle
	POINT minsize;                // minimum window size


	VkSurfaceKHR surface_L, surface_R;
	bool prepared;
	bool use_staging_buffer;

	bool VK_KHR_incremental_present_enabled;

	bool VK_GOOGLE_display_timing_enabled;
	bool syncd_with_actual_presents;
	uint64_t refresh_duration_L, refresh_duration_R;
	uint64_t refresh_duration_multiplier;
	uint64_t target_IPD_L, target_IPD_R;  // image present duration (inverse of frame rate)
	uint64_t prev_desired_present_time;
	uint32_t next_present_id;
	uint32_t last_early_id;  // 0 if no early images
	uint32_t last_late_id;   // 0 if no late images

	VkInstance inst;
	VkPhysicalDevice gpu;
	VkDevice device;
	VkQueue left_graphics_queue;
	VkQueue right_graphics_queue;
	uint32_t left_queue_family_index;
	uint32_t right_queue_family_index;
	VkSemaphore image_acquired_semaphores_L[FRAME_LAG];
	VkSemaphore image_acquired_semaphores_R[FRAME_LAG];
	VkSemaphore draw_complete_semaphores_L[FRAME_LAG];
	VkSemaphore draw_complete_semaphores_R[FRAME_LAG];

	VkPhysicalDeviceProperties gpu_props;
	VkQueueFamilyProperties *queue_props;
	VkPhysicalDeviceMemoryProperties memory_properties;

	uint32_t enabled_extension_count;
	uint32_t enabled_layer_count;
	char *extension_names[64];
	char *enabled_layers[64];

	uint32_t width, height;
	VkFormat format;
	VkColorSpaceKHR color_space;

	PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
	PFN_vkQueuePresentKHR fpQueuePresentKHR;
	PFN_vkGetRefreshCycleDurationGOOGLE fpGetRefreshCycleDurationGOOGLE;
	PFN_vkGetPastPresentationTimingGOOGLE fpGetPastPresentationTimingGOOGLE;
	uint32_t swapchainImageCount;
	VkSwapchainKHR swapchain_L;
	VkSwapchainKHR swapchain_R;
	SwapchainImageResources *swapchain_image_resources_L;
	SwapchainImageResources *swapchain_image_resources_R;
	VkPresentModeKHR presentMode;
	VkFence fences[FRAME_LAG];
	uint32_t frame_index_L;
	uint32_t frame_index_R;

	VkCommandPool cmd_pool_L;
	VkCommandPool cmd_pool_R;

	struct _depth
	{
		VkFormat format;
		VkImage image;
		VkMemoryAllocateInfo mem_alloc;
		VkDeviceMemory mem;
		VkImageView view;
	};

	struct _depth depth_L, depth_R;

	struct texture_object textures[DEMO_TEXTURE_COUNT];
	struct texture_object staging_texture;

	VkCommandBuffer cmd_L, cmd_R;

	// Buffer for initialization commands
	VkPipelineLayout pipeline_layout;
	VkDescriptorSetLayout desc_layout;
	VkPipelineCache pipelineCache;
	VkRenderPass render_pass;
	VkPipeline pipeline;

	mat4x4 projection_matrix;
	mat4x4 view_matrix_l;
	mat4x4 view_matrix_r;
	mat4x4 model_matrix;

	float spin_angle;
	float spin_increment;
	bool pause;

	VkShaderModule vert_shader_module;
	VkShaderModule frag_shader_module;

	VkDescriptorPool desc_pool_L, desc_pool_R;

	bool quit;
	uint32_t curFrame;
	uint32_t frameCount;
	bool validate = true;
	bool validate_checks_disabled;
	bool use_break;
	bool suppress_popups;
	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
	VkDebugReportCallbackEXT msg_callback;
	PFN_vkDebugReportMessageEXT DebugReportMessage;

	uint32_t current_buffer;
	uint32_t queue_family_count;

	void InitiateVulkan(VkApp *app, int argc, char **argv);
	void CreateVulkanInstance(VkApp * app);
	void CreateMSwindows(VkApp * app);

	VkBool32 CheckVulkanLayers(uint32_t check_count, char **check_names, uint32_t layer_count, VkLayerProperties *layers);

	void SetupSwapchain(VkApp *app);
	void CreateVulkanDevice(VkApp * app);
	void VulkanRun_L(VkApp * app);
	void VulkanRun_R(VkApp * app);
	void PrepareStream(VkApp * app);

	void PrepareBuffers(VkApp * demo);
	void PrepareDepth(VkApp * app);
	bool CheckMemoryType(VkApp * app, uint32_t typeBits, VkFlags requirements_mask, uint32_t * typeIndex);
	void PrepareTextures(VkApp * app);
	void PrepareTextureImage(VkApp * app, const char * filename, texture_object * tex_obj, VkImageTiling tiling, VkImageUsageFlags usage, VkFlags required_props);
	bool LoadTexture(const char * filename, uint8_t * rgba_data, VkSubresourceLayout * layout, uint32_t * width, uint32_t * height);
	void SetImageLayout(VkApp * app, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkAccessFlagBits srcAccessMask, VkPipelineStageFlags src_stages, VkPipelineStageFlags dest_stages);
	void PrepareCubeBuffers(VkApp * app);
	void PrepareDescriptorLayout(VkApp * app);
	void PrepareRenderPass(VkApp * app);
	void PreparePipeline(VkApp * app);
	VkShaderModule PrepareVertexShader(VkApp * app);
	VkShaderModule PrepareFragmentShader(VkApp * app);
	char* ReadSPV(const char * filename, size_t * psize);
	VkShaderModule PrepareShaderModule(VkApp * app, const void * code, size_t size);
	void PrepareDescriptorPool(VkApp * app);
	void PrepareDescriptorSet(VkApp * app);
	void PrepareFramebuffers(VkApp * app);
	void DrawBuildCmd(VkApp * app, int index);
	void FlushInitiationCmd(VkApp * app);
	void DestroyTextureImage(VkApp * app, texture_object * tex_objs);
	void Draw_L(VkApp *app);
	void Draw_R(VkApp *app);
	void UpdateDataBuffer(VkApp * app);
	void Resize_L(VkApp* app);
	void Resize_R(VkApp* app);
	void CleanUp(VkApp * app);

};

static VkApp* p_vrcube;

LRESULT CALLBACK WndProc_L(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg) {
	case WM_CLOSE:
		PostQuitMessage(validation_error);
		break;
	case WM_PAINT:
		// The validation callback calls MessageBox which can generate paint
		// events - don't make more Vulkan calls if we got here from the
		// callback
		if (!in_callback)
		{
			p_vrcube->VulkanRun_L(p_vrcube);
			p_vrcube->VulkanRun_R(p_vrcube);
			assert(p_vrcube->frame_index_L == p_vrcube->frame_index_R);
		}
		break;
	case WM_GETMINMAXINFO:     // set window's minimum size
		((MINMAXINFO*)lParam)->ptMinTrackSize = p_vrcube->minsize;
		return 0;
	case WM_SIZE:
		// Resize the application to the new window size, except when
		// it was minimized. Vulkan doesn't support images or swapchains
		// with width=0 and height=0.
		if (wParam != SIZE_MINIMIZED) {
			p_vrcube->width = lParam & 0xffff;
			p_vrcube->height = (lParam & 0xffff0000) >> 16;
			p_vrcube->Resize_L(p_vrcube);
		}
		break;
	default:
		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

LRESULT CALLBACK WndProc_R(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg) {
	case WM_CLOSE:
		PostQuitMessage(validation_error);
		break;
	case WM_PAINT:
		// The validation callback calls MessageBox which can generate paint
		// events - don't make more Vulkan calls if we got here from the
		// callback
		if (!in_callback)
		{
			p_vrcube->VulkanRun_R(p_vrcube);
			p_vrcube->VulkanRun_L(p_vrcube);
			assert(p_vrcube->frame_index_L == p_vrcube->frame_index_R);
		}
		break;
	case WM_GETMINMAXINFO:     // set window's minimum size
		((MINMAXINFO*)lParam)->ptMinTrackSize = p_vrcube->minsize;
		return 0;
	case WM_SIZE:
		// Resize the application to the new window size, except when
		// it was minimized. Vulkan doesn't support images or swapchains
		// with width=0 and height=0.
		if (wParam != SIZE_MINIMIZED) {
			p_vrcube->width = lParam & 0xffff;
			p_vrcube->height = (lParam & 0xffff0000) >> 16;
			p_vrcube->Resize_R(p_vrcube);
		}
		break;
	default:

		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

VKAPI_ATTR VkBool32 VKAPI_CALL BreakCallback
(
	VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
	size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
	void *pUserData
)
{
	DebugBreak();
	return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc
(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location,
	int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData
)
{
	// clang-format off
	char *message = (char *)malloc(strlen(pMsg) + 100);
	wchar_t* widemsg = (wchar_t*)malloc(strlen(pMsg) + 100);

	assert(message);

	if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		sprintf(message, "INFORMATION: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		sprintf(message, "PERFORMANCE WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		sprintf(message, "DEBUG: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}
	else {
		sprintf(message, "INFORMATION: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		validation_error = 1;
	}

	in_callback = true;
	VkApp *app = (VkApp*)pUserData;

	if (!app->suppress_popups)
	{
		mbstowcs(widemsg, message, (int)strlen(message) + 1);

		MessageBox(NULL, widemsg, L"Alert", MB_OK);
	};
	in_callback = false;

	free(message);
	free(widemsg);
	//clang-format on

	/*
	* false indicates that layer should not bail-out of an
	* API call that had validation failures. This may mean that the
	* app dies inside the driver due to invalid parameter(s).
	* That's what would happen without validation layers, so we'll
	* keep that behavior here.
	*/
	return false;
}

