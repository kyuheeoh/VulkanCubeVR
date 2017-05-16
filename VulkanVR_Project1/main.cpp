#include "main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;   // message
	bool done; // flag saying when app is complete
	int argc;
	char **argv;

	// Ensure wParam is initialized.
	msg.wParam = 0;

	// Use the CommandLine functions to get the command line arguments.
	// Unfortunately, Microsoft outputs
	// this information as wide characters for Unicode, and we simply want the
	// Ascii version to be compatible
	// with the non-Windows side.  So, we have to convert the information to
	// Ascii character strings.
	LPWSTR *commandLineArgs = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (NULL == commandLineArgs) {
		argc = 0;
	}

	if (argc > 0) {
		argv = (char **)malloc(sizeof(char *) * argc);
		if (argv == NULL) {
			argc = 0;
		}
		else {
			for (int iii = 0; iii < argc; iii++) {
				size_t wideCharLen = wcslen(commandLineArgs[iii]);
				size_t numConverted = 0;

				argv[iii] = (char *)malloc(sizeof(char) * (wideCharLen + 1));
				if (argv[iii] != NULL) {
					wcstombs_s(&numConverted, argv[iii], wideCharLen + 1,
						commandLineArgs[iii], wideCharLen + 1);
				}
			}
		}
	}
	else {
		argv = NULL;
	}

	VkApp vrcube{};
	p_vrcube = &vrcube;

	vrcube.InitiateVulkan(&vrcube, argc, argv);

	// Free up the items we had to allocate for the command line arguments.
	if (argc > 0 && argv != NULL) {
		for (int iii = 0; iii < argc; iii++) {
			if (argv[iii] != NULL) {
				free(argv[iii]);
			}
		}
		free(argv);
	}

	vrcube.connection = hInstance;
	vrcube.name = TEXT(APP_SHORT_NAME);
	vrcube.CreateMSwindows(&vrcube);
	vrcube.SetupSwapchain(&vrcube);
	vrcube.PrepareStream(&vrcube);

	done = false; // initialize loop condition variable

				  // main message loop
	while (!done) {
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		if (msg.message == WM_QUIT) // check for a quit message
		{
			done = true; // if found, quit app
		}
		else {
			/* Translate and dispatch to event queue*/
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		RedrawWindow(vrcube.window_l, NULL, NULL, RDW_INTERNALPAINT);
		RedrawWindow(vrcube.window_r, NULL, NULL, RDW_INTERNALPAINT);
	}

	vrcube.CleanUp(&vrcube);

	return (int)msg.wParam;
}

VkApp::VkApp()
{

}
VkApp::~VkApp()
{
}
void VkApp::InitiateVulkan(VkApp * app, int argc, char ** argv)
{

	{
		vec3 eye_l = { -0.05f, 3.0f, 5.0f };
		vec3 eye_r = { 0.05f,3.0f,5.0 };
		vec3 origin = { 0, 0, 0 };
		vec3 up = { 0.0f, 1.0f, 0.0 };

		memset(app, 0, sizeof(*app));

		app->presentMode = VK_PRESENT_MODE_FIFO_KHR;
		app->frameCount = INT32_MAX;

		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "--use_staging") == 0) {
				app->use_staging_buffer = true;
				continue;
			}
			if ((strcmp(argv[i], "--present_mode") == 0) &&
				(i < argc - 1)) {
				app->presentMode = (VkPresentModeKHR)atoi(argv[i + 1]);
				i++;
				continue;
			}
			if (strcmp(argv[i], "--break") == 0) {
				app->use_break = true;
				continue;
			}
			if (strcmp(argv[i], "--validate") == 0) {
				app->validate = true;
				continue;
			}
			if (strcmp(argv[i], "--validate-checks-disabled") == 0) {
				app->validate = true;
				app->validate_checks_disabled = true;
				continue;
			}
			if (strcmp(argv[i], "--xlib") == 0) {
				MessageBox(NULL, L"--xlib is deprecated and no longer does anything", NULL, MB_OK);
				exit(1);
			}
			if (strcmp(argv[i], "--c") == 0 && app->frameCount == INT32_MAX &&
				i < argc - 1 && sscanf(argv[i + 1], "%d", &app->frameCount) == 1 &&
				app->frameCount >= 0) {
				i++;
				continue;
			}
			if (strcmp(argv[i], "--suppress_popups") == 0) {
				app->suppress_popups = true;
				continue;
			}
			if (strcmp(argv[i], "--display_timing") == 0) {
				app->VK_GOOGLE_display_timing_enabled = true;
				continue;
			}
			if (strcmp(argv[i], "--incremental_present") == 0) {
				app->VK_KHR_incremental_present_enabled = true;
				continue;
			}

			fprintf(stderr, "Usage:\n  %s [--use_staging] [--validate] [--validate-checks-disabled] [--break] "
				"[--c <framecount>] [--suppress_popups] [--incremental_present] [--display_timing] [--present_mode <present mode enum>]\n"
				"VK_PRESENT_MODE_IMMEDIATE_KHR = %d\n"
				"VK_PRESENT_MODE_MAILBOX_KHR = %d\n"
				"VK_PRESENT_MODE_FIFO_KHR = %d\n"
				"VK_PRESENT_MODE_FIFO_RELAXED_KHR = %d\n",
				APP_SHORT_NAME, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR,
				VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR);
			fflush(stderr);
			exit(1);

		}

		CreateVulkanInstance(app);

		app->width = 500;
		app->height = 500;

		app->spin_angle = 1.0f;
		app->spin_increment = 0.2f;
		app->pause = false;

		mat4x4_perspective(app->projection_matrix, (float)degreesToRadians(45.0f),
			1.0f, 0.1f, 100.0f);
		mat4x4_look_at(app->view_matrix_l, eye_l, origin, up);
		mat4x4_look_at(app->view_matrix_r, eye_r, origin, up);

		mat4x4_identity(app->model_matrix);

		app->projection_matrix[1][1] *= -1;  //Flip projection matrix from GL to Vulkan orientation.
	}

}

void VkApp::CreateMSwindows(VkApp * app)
{
	WNDCLASSEX win_class_l, win_class_r;
	ZeroMemory(&win_class_l, sizeof(WNDCLASSEX));
	ZeroMemory(&win_class_r, sizeof(WNDCLASSEX));

	// Initialize the window class structure:
	win_class_l.cbSize = sizeof(WNDCLASSEX);
	win_class_l.style = CS_HREDRAW | CS_VREDRAW;
	win_class_l.lpfnWndProc = WndProc_L;
	win_class_l.cbClsExtra = 0;
	win_class_l.cbWndExtra = 0;
	win_class_l.hInstance = app->connection; // hInstance
	win_class_l.hIcon = NULL;
	win_class_l.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class_l.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class_l.lpszMenuName = NULL;
	win_class_l.lpszClassName = L"Left\0";
	win_class_l.hIconSm = LoadIcon(NULL, IDI_WINLOGO);


	win_class_r.cbSize = sizeof(WNDCLASSEX);
	win_class_r.style = CS_HREDRAW | CS_VREDRAW;
	win_class_r.lpfnWndProc = WndProc_R;
	win_class_r.cbClsExtra = 0;
	win_class_r.cbWndExtra = 0;
	win_class_r.hInstance = app->connection; // hInstance
	win_class_r.hIcon = NULL;
	win_class_r.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class_r.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class_r.lpszMenuName = NULL;
	win_class_r.lpszClassName = L"Right\0";
	win_class_r.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	// Register window class:

	auto winl = RegisterClassEx(&win_class_l);
	auto winr = RegisterClassEx(&win_class_r);

	// Create window with the registered class:
	RECT wr = { 0, 0, app->width, app->height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	app->window_l = CreateWindowEx(0,
		L"Left\0",           // class name
		app->name,           // app name
		WS_OVERLAPPEDWINDOW | // window style
		WS_VISIBLE | WS_SYSMENU,
		100, 100,           // x/y coords
		wr.right - wr.left, // width
		wr.bottom - wr.top, // height
		NULL,               // handle to parent
		NULL,               // handle to menu
		app->connection,   // hInstance
		NULL);              // no extra parameters
	if (!app->window_l)
	{
		// It didn't work, so try to give a useful error:
		printf("Cannot create a window in which to draw!\n");
		fflush(stdout);
		exit(1);
	}

	app->window_r = CreateWindowEx(0,
		L"Right\0",           // class name
		app->name,           // app name
		WS_OVERLAPPEDWINDOW | // window style
		WS_VISIBLE | WS_SYSMENU,
		800, 100,           // x/y coords
		wr.right - wr.left, // width
		wr.bottom - wr.top, // height
		NULL,               // handle to parent
		NULL,               // handle to menu
		app->connection,   // hInstance
		NULL);              // no extra parameters
	if (!app->window_r)
	{
		// It didn't work, so try to give a useful error:
		printf("Cannot create a window in which to draw!\n");
		fflush(stdout);
		exit(1);
	}
	// Window client area size must be at least 1 pixel high, to prevent crash.
	app->minsize.x = GetSystemMetrics(SM_CXMINTRACK);
	app->minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;

}

void VkApp::SetupSwapchain(VkApp *app)
{
	VkResult err;

	// Create a WSI surface for the window:

	VkWin32SurfaceCreateInfoKHR createInfo_L;
	createInfo_L.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo_L.pNext = NULL;
	createInfo_L.flags = 0;
	createInfo_L.hinstance = app->connection;
	createInfo_L.hwnd = app->window_l;

	err = vkCreateWin32SurfaceKHR(app->inst, &createInfo_L, NULL, &app->surface_L);

	assert(!err);
	VkWin32SurfaceCreateInfoKHR createInfo_R;
	createInfo_R.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo_R.pNext = NULL;
	createInfo_R.flags = 0;
	createInfo_R.hinstance = app->connection;
	createInfo_R.hwnd = app->window_r;

	err = vkCreateWin32SurfaceKHR(app->inst, &createInfo_R, NULL, &app->surface_R);

	assert(!err);

	// Iterate over each queue to learn whether it supports presenting:
	VkBool32 *supportsPresent =
		(VkBool32 *)malloc(app->queue_family_count * sizeof(VkBool32));
	for (uint32_t i = 0; i < app->queue_family_count; i++) {
		app->fpGetPhysicalDeviceSurfaceSupportKHR(app->gpu, i, app->surface_L,
			&supportsPresent[i]);
	}
	///---surface R shoud be same as L.
	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	uint32_t leftGraphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t rightGraphicsQueueFamilyIndex = UINT32_MAX;

	for (uint32_t i = 0; i < app->queue_family_count; i++) {
		if ((app->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && (supportsPresent[i] == VK_TRUE))
		{
			leftGraphicsQueueFamilyIndex = i;
			rightGraphicsQueueFamilyIndex = i;
			break;
		}
	}

	// Generate error if could not find both a graphics and a present queue
	if (leftGraphicsQueueFamilyIndex == UINT32_MAX ||
		rightGraphicsQueueFamilyIndex == UINT32_MAX) {
		ERR_EXIT(L"Could not find both graphics and present queues\n",
			L"Swapchain Initialization Failure");
	}

	app->left_queue_family_index = leftGraphicsQueueFamilyIndex;
	app->right_queue_family_index = rightGraphicsQueueFamilyIndex;

	free(supportsPresent);

	CreateVulkanDevice(app);

	GET_DEVICE_PROC_ADDR(app->device, CreateSwapchainKHR);
	GET_DEVICE_PROC_ADDR(app->device, DestroySwapchainKHR);
	GET_DEVICE_PROC_ADDR(app->device, GetSwapchainImagesKHR);
	GET_DEVICE_PROC_ADDR(app->device, AcquireNextImageKHR);
	GET_DEVICE_PROC_ADDR(app->device, QueuePresentKHR);

	if (app->VK_GOOGLE_display_timing_enabled)
	{
		GET_DEVICE_PROC_ADDR(app->device, GetRefreshCycleDurationGOOGLE);
		GET_DEVICE_PROC_ADDR(app->device, GetPastPresentationTimingGOOGLE);
	}

	vkGetDeviceQueue(app->device, app->left_queue_family_index, 0,
		&app->left_graphics_queue);

	vkGetDeviceQueue(app->device, app->right_queue_family_index, 1,
		&app->right_graphics_queue);

	// Get the list of VkFormat's that are supported:
	uint32_t formatCount_L = 0;
	uint32_t formatCount_R = 0;
	err = app->fpGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface_L,
		&formatCount_L, NULL);
	assert(!err);
	VkSurfaceFormatKHR *surfFormats_L =
		(VkSurfaceFormatKHR *)malloc(formatCount_L * sizeof(VkSurfaceFormatKHR));
	err = app->fpGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface_L,
		&formatCount_L, surfFormats_L);
	assert(!err);


	err = app->fpGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface_R,
		&formatCount_R, NULL);
	assert(!err);
	VkSurfaceFormatKHR *surfFormats_R =
		(VkSurfaceFormatKHR *)malloc(formatCount_R * sizeof(VkSurfaceFormatKHR));
	err = app->fpGetPhysicalDeviceSurfaceFormatsKHR(app->gpu, app->surface_R,
		&formatCount_R, surfFormats_R);
	assert(!err);

	assert((formatCount_L == formatCount_R));


	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if (formatCount_L == 1 && surfFormats_L[0].format == VK_FORMAT_UNDEFINED)
	{
		app->format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else {
		assert(formatCount_L >= 1);
		app->format = surfFormats_L[0].format;
	}
	app->color_space = surfFormats_L[0].colorSpace;

	app->quit = false;
	app->curFrame = 0;

	// Create semaphores to synchronize acquiring presentable buffers before
	// rendering and waiting for drawing to be complete before presenting
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 0;


	// Create fences that we can use to throttle if we get too far
	// ahead of the image presents
	VkFenceCreateInfo fence_ci{};
	fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_ci.pNext = NULL;
	fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < FRAME_LAG; i++) {
		err = vkCreateFence(app->device, &fence_ci, NULL, &app->fences[i]);
		assert(!err);

		err = vkCreateSemaphore(app->device, &semaphoreCreateInfo, NULL,
			&app->image_acquired_semaphores_L[i]);
		assert(!err);
		err = vkCreateSemaphore(app->device, &semaphoreCreateInfo, NULL,
			&app->image_acquired_semaphores_R[i]);
		assert(!err);

		err = vkCreateSemaphore(app->device, &semaphoreCreateInfo, NULL,
			&app->draw_complete_semaphores_L[i]);
		assert(!err);

		err = vkCreateSemaphore(app->device, &semaphoreCreateInfo, NULL,
			&app->draw_complete_semaphores_R[i]);
		assert(!err);

	}

	app->frame_index_L = 0;
	app->frame_index_R = 0;
	// Get Memory information and properties
	vkGetPhysicalDeviceMemoryProperties(app->gpu, &app->memory_properties);
}

void VkApp::CreateVulkanDevice(VkApp *app)
{
	VkResult  err;
	float queue_priorities[1] = { 0.0 };
	VkDeviceQueueCreateInfo queues[2];
	queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queues[0].pNext = NULL;
	queues[0].queueFamilyIndex = app->left_queue_family_index;
	queues[0].queueCount = 2;
	queues[0].pQueuePriorities = queue_priorities;
	queues[0].flags = 0;

	queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queues[1].pNext = NULL;
	queues[1].queueFamilyIndex = app->right_queue_family_index;
	queues[1].queueCount = 2;
	queues[1].pQueuePriorities = queue_priorities;
	queues[1].flags = 0;

	VkDeviceCreateInfo deviceInfo = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,  //sType
		NULL,															//pNext
		0,																	//flags
		2,																	//queueCreateInfoCount 
		queues,														//pQueueCreateInfos
		0,																	//enabledLayerCount
		NULL,															//ppEnabledLayerNames
		app->enabled_extension_count,					//enabledExtensionCount
		(const char *const *)app->extension_names,	//ppEnabledExtensionNames
		NULL																//pEnabledFeatures
																			// If specific features are required, pass them in here
	};

	err = vkCreateDevice(app->gpu, &deviceInfo, NULL, &app->device);
	assert(!err);
}

void VkApp::CreateVulkanInstance(VkApp *app)
{
	VkResult err;
	uint32_t instance_extension_count = 0;
	uint32_t instance_layer_count = 0;
	uint32_t validation_layer_count = 0;
	char **instance_validation_layers = NULL;
	app->enabled_extension_count = 0;
	app->enabled_layer_count = 0;

	char *instance_validation_layers_alt1[] = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	char *instance_validation_layers_alt2[] = {
		"VK_LAYER_GOOGLE_threading",      "VK_LAYER_LUNARG_parameter_validation",
		"VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_core_validation",
		"VK_LAYER_LUNARG_swapchain",      "VK_LAYER_GOOGLE_unique_objects"
	};

	/* Look for validation layers */
	VkBool32 validation_found = 0;
	if (app->validate) {

		err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
		assert(!err);

		instance_validation_layers = instance_validation_layers_alt1;
		if (instance_layer_count > 0) {
			VkLayerProperties *instance_layers = (VkLayerProperties *)
				malloc(sizeof(VkLayerProperties) * instance_layer_count);
			err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
				instance_layers);
			assert(!err);


			validation_found = CheckVulkanLayers(
				ARRAY_SIZE(instance_validation_layers_alt1),
				instance_validation_layers, instance_layer_count,
				instance_layers);
			if (validation_found) {
				app->enabled_layer_count = ARRAY_SIZE(instance_validation_layers_alt1);
				app->enabled_layers[0] = "VK_LAYER_LUNARG_standard_validation";
				validation_layer_count = 1;
			}
			else {
				// use alternative set of validation layers
				instance_validation_layers = instance_validation_layers_alt2;
				app->enabled_layer_count = ARRAY_SIZE(instance_validation_layers_alt2);
				validation_found = CheckVulkanLayers(
					ARRAY_SIZE(instance_validation_layers_alt2),
					instance_validation_layers, instance_layer_count,
					instance_layers);
				validation_layer_count =
					ARRAY_SIZE(instance_validation_layers_alt2);
				for (uint32_t i = 0; i < validation_layer_count; i++) {
					app->enabled_layers[i] = instance_validation_layers[i];
				}
			}
			free(instance_layers);
		}

		if (!validation_found) {
			ERR_EXIT(L"vkEnumerateInstanceLayerProperties failed to find "
				L"required validation layer.\n\n"
				L"Please look at the Getting Started guide for additional "
				L"information.\n",
				L"vkCreateInstance Failure");
		}
	}

	/* Look for instance extensions */
	VkBool32 surfaceExtFound = 0;
	VkBool32 platformSurfaceExtFound = 0;
	memset(app->extension_names, 0, sizeof(app->extension_names));

	err = vkEnumerateInstanceExtensionProperties(
		NULL, &instance_extension_count, NULL);
	assert(!err);

	if (instance_extension_count > 0) {
		VkExtensionProperties *instance_extensions = (VkExtensionProperties *)
			malloc(sizeof(VkExtensionProperties) * instance_extension_count);
		err = vkEnumerateInstanceExtensionProperties(
			NULL, &instance_extension_count, instance_extensions);
		assert(!err);
		for (uint32_t i = 0; i < instance_extension_count; i++) {
			if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				surfaceExtFound = 1;
				app->extension_names[app->enabled_extension_count++] =
					VK_KHR_SURFACE_EXTENSION_NAME;
			}

			if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				platformSurfaceExtFound = 1;
				app->extension_names[app->enabled_extension_count++] =
					VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
			}

			if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				if (app->validate) {
					app->extension_names[app->enabled_extension_count++] =
						VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
				}
			}
			assert(app->enabled_extension_count < 64);
		}

		free(instance_extensions);
	}

	if (!surfaceExtFound) {
		ERR_EXIT(L"vkEnumerateInstanceExtensionProperties failed to find "
			L"the " VK_KHR_SURFACE_EXTENSION_NAME
			L" extension.\n\nDo you have a compatible "
			L"Vulkan installable client driver (ICD) installed?\nPlease "
			L"look at the Getting Started guide for additional "
			L"information.\n",
			L"vkCreateInstance Failure");
	}
	if (!platformSurfaceExtFound) {

		ERR_EXIT(L"vkEnumerateInstanceExtensionProperties failed to find "
			L"the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
			L" extension.\n\nDo you have a compatible "
			L"Vulkan installable client driver (ICD) installed?\nPlease "
			L"look at the Getting Started guide for additional "
			L"information.\n",
			L"vkCreateInstance Failure");

	}
	const VkApplicationInfo appInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		NULL,
		APP_SHORT_NAME,
		VK_MAKE_VERSION(0,0,1),
		APP_SHORT_NAME,
		VK_MAKE_VERSION(0,0,1),
		VK_MAKE_VERSION(1,0,46)
	};
	VkInstanceCreateInfo inst_info = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		NULL,
		0,
		&appInfo,
		app->enabled_layer_count,
		(const char *const *)instance_validation_layers,
		app->enabled_extension_count,
		(const char *const *)app->extension_names,
	};

	/*
	* This is info for a temp callback to use during CreateInstance.
	* After the instance is created, we use the instance-based
	* function to register the final callback.
	*/
	VkDebugReportCallbackCreateInfoEXT dbgCreateInfoTemp;
	VkValidationFlagsEXT val_flags;
	if (app->validate) {
		dbgCreateInfoTemp.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfoTemp.pNext = NULL;
		dbgCreateInfoTemp.pfnCallback = app->use_break ? BreakCallback : dbgFunc;
		dbgCreateInfoTemp.pUserData = app;
		dbgCreateInfoTemp.flags =
			VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		if (app->validate_checks_disabled) {
			val_flags.sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT;
			val_flags.pNext = NULL;
			val_flags.disabledValidationCheckCount = 1;
			VkValidationCheckEXT disabled_check = VK_VALIDATION_CHECK_ALL_EXT;
			val_flags.pDisabledValidationChecks = &disabled_check;
			dbgCreateInfoTemp.pNext = (void*)&val_flags;
		}
		inst_info.pNext = &dbgCreateInfoTemp;
	}

	uint32_t gpu_count;

	err = vkCreateInstance(&inst_info, NULL, &app->inst);
	if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
		ERR_EXIT(L"Cannot find a compatible Vulkan installable client driver "
			L"(ICD).\n\nPlease look at the Getting Started guide for "
			L"additional information.\n",
			L"vkCreateInstance Failure");
	}
	else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
		ERR_EXIT(L"Cannot find a specified extension library"
			L".\nMake sure your layers path is set appropriately.\n",
			L"vkCreateInstance Failure");
	}
	else if (err) {
		ERR_EXIT(L"vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
			L"installable client driver (ICD) installed?\nPlease look at "
			L"the Getting Started guide for additional information.\n",
			L"vkCreateInstance Failure");
	}

	/* Make initial call to query gpu_count, then second call for gpu info*/
	err = vkEnumeratePhysicalDevices(app->inst, &gpu_count, NULL);
	assert(!err && gpu_count > 0);

	if (gpu_count > 0) {
		VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * gpu_count);
		err = vkEnumeratePhysicalDevices(app->inst, &gpu_count, physical_devices);
		assert(!err);
		/* For cube app we just grab the first physical device */
		app->gpu = physical_devices[0];
		free(physical_devices);
	}
	else {
		ERR_EXIT(L"vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
			L"Do you have a compatible Vulkan installable client driver (ICD) "
			L"installed?\nPlease look at the Getting Started guide for "
			L"additional information.\n",
			L"vkEnumeratePhysicalDevices Failure");
	}

	/* Look for device extensions */
	uint32_t device_extension_count = 0;
	VkBool32 swapchainExtFound = 0;
	app->enabled_extension_count = 0;
	memset(app->extension_names, 0, sizeof(app->extension_names));

	err = vkEnumerateDeviceExtensionProperties(app->gpu, NULL,
		&device_extension_count, NULL);
	assert(!err);

	if (device_extension_count > 0) {
		VkExtensionProperties *device_extensions = (VkExtensionProperties *)
			malloc(sizeof(VkExtensionProperties) * device_extension_count);
		err = vkEnumerateDeviceExtensionProperties(
			app->gpu, NULL, &device_extension_count, device_extensions);
		assert(!err);

		for (uint32_t i = 0; i < device_extension_count; i++) {
			if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				device_extensions[i].extensionName)) {
				swapchainExtFound = 1;
				app->extension_names[app->enabled_extension_count++] =
					VK_KHR_SWAPCHAIN_EXTENSION_NAME;
			}
			assert(app->enabled_extension_count < 64);
		}

		if (app->VK_KHR_incremental_present_enabled) {
			// Even though the user "enabled" the extension via the command
			// line, we must make sure that it's enumerated for use with the
			// device.  Therefore, disable it here, and re-enable it again if
			// enumerated.
			app->VK_KHR_incremental_present_enabled = false;
			for (uint32_t i = 0; i < device_extension_count; i++) {
				if (!strcmp(VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,
					device_extensions[i].extensionName)) {
					app->extension_names[app->enabled_extension_count++] =
						VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME;
					app->VK_KHR_incremental_present_enabled = true;
					DbgMsg("VK_KHR_incremental_present extension enabled\n");
				}
				assert(app->enabled_extension_count < 64);
			}
			if (!app->VK_KHR_incremental_present_enabled) {
				DbgMsg("VK_KHR_incremental_present extension NOT AVAILABLE\n");
			}
		}

		if (app->VK_GOOGLE_display_timing_enabled) {
			// Even though the user "enabled" the extension via the command
			// line, we must make sure that it's enumerated for use with the
			// device.  Therefore, disable it here, and re-enable it again if
			// enumerated.
			app->VK_GOOGLE_display_timing_enabled = false;
			for (uint32_t i = 0; i < device_extension_count; i++) {
				if (!strcmp(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME,
					device_extensions[i].extensionName)) {
					app->extension_names[app->enabled_extension_count++] =
						VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME;
					app->VK_GOOGLE_display_timing_enabled = true;
					DbgMsg("VK_GOOGLE_display_timing extension enabled\n");
				}
				assert(app->enabled_extension_count < 64);
			}
			if (!app->VK_GOOGLE_display_timing_enabled) {
				DbgMsg("VK_GOOGLE_display_timing extension NOT AVAILABLE\n");
			}
		}

		free(device_extensions);
	}

	if (!swapchainExtFound) {
		ERR_EXIT(L"vkEnumerateDeviceExtensionProperties failed to find "
			L"the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
			L" extension.\n\nDo you have a compatible "
			L"Vulkan installable client driver (ICD) installed?\nPlease "
			L"look at the Getting Started guide for additional "
			L"information.\n",
			L"vkCreateInstance Failure");
	}

	if (app->validate) {
		app->CreateDebugReportCallback =
			(PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
				app->inst, "vkCreateDebugReportCallbackEXT");
		app->DestroyDebugReportCallback =
			(PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
				app->inst, "vkDestroyDebugReportCallbackEXT");
		if (!app->CreateDebugReportCallback) {
			ERR_EXIT(
				L"GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n",
				L"vkGetProcAddr Failure");
		}
		if (!app->DestroyDebugReportCallback) {
			ERR_EXIT(
				L"GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n",
				L"vkGetProcAddr Failure");
		}
		app->DebugReportMessage =
			(PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
				app->inst, "vkDebugReportMessageEXT");
		if (!app->DebugReportMessage) {
			ERR_EXIT(L"GetProcAddr: Unable to find vkDebugReportMessageEXT\n",
				L"vkGetProcAddr Failure");
		}

		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
		PFN_vkDebugReportCallbackEXT callback;
		callback = app->use_break ? BreakCallback : dbgFunc;
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfo.pNext = NULL;
		dbgCreateInfo.pfnCallback = callback;
		dbgCreateInfo.pUserData = app;
		dbgCreateInfo.flags =
			VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		err = app->CreateDebugReportCallback(app->inst, &dbgCreateInfo, NULL,
			&app->msg_callback);
		switch (err) {
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			ERR_EXIT(L"CreateDebugReportCallback: out of host memory\n",
				L"CreateDebugReportCallback Failure");
			break;
		default:
			ERR_EXIT(L"CreateDebugReportCallback: unknown failure\n",
				L"CreateDebugReportCallback Failure");
			break;
		}
	}
	vkGetPhysicalDeviceProperties(app->gpu, &app->gpu_props);

	/* Call with NULL data to get count */
	vkGetPhysicalDeviceQueueFamilyProperties(app->gpu,
		&app->queue_family_count, NULL);
	assert(app->queue_family_count >= 1);

	app->queue_props = (VkQueueFamilyProperties *)malloc(
		app->queue_family_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(
		app->gpu, &app->queue_family_count, app->queue_props);

	// Query fine-grained feature support for this device.
	//  If app has specific feature requirements it should check supported
	//  features based on this query
	VkPhysicalDeviceFeatures physDevFeatures;
	vkGetPhysicalDeviceFeatures(app->gpu, &physDevFeatures);

	GET_INSTANCE_PROC_ADDR(app->inst, GetPhysicalDeviceSurfaceSupportKHR);
	GET_INSTANCE_PROC_ADDR(app->inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
	GET_INSTANCE_PROC_ADDR(app->inst, GetPhysicalDeviceSurfaceFormatsKHR);
	GET_INSTANCE_PROC_ADDR(app->inst, GetPhysicalDeviceSurfacePresentModesKHR);
	GET_INSTANCE_PROC_ADDR(app->inst, GetSwapchainImagesKHR);
}


VkBool32 VkApp::CheckVulkanLayers(uint32_t check_count, char **check_names, uint32_t layer_count, VkLayerProperties *layers)
{
	for (uint32_t i = 0; i < check_count; i++) {
		VkBool32 found = 0;
		for (uint32_t j = 0; j < layer_count; j++) {
			if (!strcmp(check_names[i], layers[j].layerName)) {
				found = 1;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
			return 0;
		}
	}
	return 1;
}


void VkApp::VulkanRun_L(VkApp *app)
{
	if (!app->prepared)
		return;

	Draw_L(app);

	app->curFrame++;
	if (app->frameCount != INT_MAX && app->curFrame == app->frameCount) {
		PostQuitMessage(validation_error);
	}
}

void VkApp::VulkanRun_R(VkApp *app)
{
	if (!app->prepared)
		return;

	Draw_R(app);

	app->curFrame++;
	if (app->frameCount != INT_MAX && app->curFrame == app->frameCount)
	{
		PostQuitMessage(validation_error);
	}
}

void VkApp::PrepareStream(VkApp *app)
{
	VkResult err;

	const VkCommandPoolCreateInfo cmd_pool_info_L
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,		//sType
		NULL,																				//pNext
		0,																						//flags
		app->left_queue_family_index,										//queueFamilyIndex
	};

	err = vkCreateCommandPool(app->device, &cmd_pool_info_L, NULL, &app->cmd_pool_L);
	assert(!err);

	const VkCommandPoolCreateInfo cmd_pool_info_R
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,		//sType
		NULL,																				//pNext
		0,																						//flags
		app->right_queue_family_index,										//queueFamilyIndex
	};

	err = vkCreateCommandPool(app->device, &cmd_pool_info_R, NULL, &app->cmd_pool_R);
	assert(!err);


	const VkCommandBufferAllocateInfo cmd_info_L
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,		//sType
		NULL,																					//pNext
		app->cmd_pool_L,																	//commandPool
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,									//level
		1,																							//commandBufferCount
	};

	err = vkAllocateCommandBuffers(app->device, &cmd_info_L, &app->cmd_L);
	assert(!err);

	const VkCommandBufferAllocateInfo cmd_info_R
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,		//sType
		NULL,																					//pNext
		app->cmd_pool_R,																	//commandPool
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,									//level
		1,																							//commandBufferCount
	};

	err = vkAllocateCommandBuffers(app->device, &cmd_info_R, &app->cmd_R);
	assert(!err);


	VkCommandBufferBeginInfo cmd_buf_info
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,			//sType
		NULL,																					//pNext 
		0,																							//flags
		NULL,																					//pInheritanceInfo
	};
	err = vkBeginCommandBuffer(app->cmd_L, &cmd_buf_info);
	assert(!err);
	err = vkBeginCommandBuffer(app->cmd_R, &cmd_buf_info);
	assert(!err);

	PrepareBuffers(app);
	PrepareDepth(app);
	PrepareTextures(app);
	PrepareCubeBuffers(app);

	PrepareDescriptorLayout(app);
	PrepareRenderPass(app);
	PreparePipeline(app);

	for (uint32_t i = 0; i < app->swapchainImageCount; i++)
	{
		err =
			vkAllocateCommandBuffers(app->device, &cmd_info_L, &app->swapchain_image_resources_L[i].cmd);
		assert(!err);

		err =
			vkAllocateCommandBuffers(app->device, &cmd_info_R, &app->swapchain_image_resources_R[i].cmd);
		assert(!err);
	}

	PrepareDescriptorPool(app);
	PrepareDescriptorSet(app);

	PrepareFramebuffers(app);

	for (uint32_t i = 0; i < app->swapchainImageCount; i++)
	{
		app->current_buffer = i;
		DrawBuildCmd(app, i);
	}

	/*
	* Prepare functions above may generate pipeline commands
	* that need to be flushed before beginning the render loop.
	*/
	FlushInitiationCmd(app);
	if (app->staging_texture.image)
	{
		DestroyTextureImage(app, &app->staging_texture);
	}

	app->current_buffer = 0;
	app->prepared = true;
}

void VkApp::PrepareBuffers(VkApp *app)
{
	VkResult err;
	VkSwapchainKHR oldSwapchain_L = app->swapchain_L;
	VkSwapchainKHR oldSwapchain_R = app->swapchain_R;

	// Check the surface capabilities and formats
	VkSurfaceCapabilitiesKHR surfCapabilities_L, surfCapabilities_R;
	err = app->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(
		app->gpu, app->surface_L, &surfCapabilities_L);
	assert(!err);
	err = app->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(
		app->gpu, app->surface_R, &surfCapabilities_R);
	assert(!err);

	uint32_t presentModeCount_L, presentModeCount_R;
	err = app->fpGetPhysicalDeviceSurfacePresentModesKHR(
		app->gpu, app->surface_L, &presentModeCount_L, NULL);
	assert(!err);
	VkPresentModeKHR *presentModes_L =
		(VkPresentModeKHR *)malloc(presentModeCount_L * sizeof(VkPresentModeKHR));
	assert(presentModes_L);
	err = app->fpGetPhysicalDeviceSurfacePresentModesKHR(
		app->gpu, app->surface_L, &presentModeCount_L, presentModes_L);
	assert(!err);

	err = app->fpGetPhysicalDeviceSurfacePresentModesKHR(
		app->gpu, app->surface_R, &presentModeCount_R, NULL);
	assert(!err);
	VkPresentModeKHR *presentModes_R =
		(VkPresentModeKHR *)malloc(presentModeCount_R * sizeof(VkPresentModeKHR));
	assert(presentModes_R);
	err = app->fpGetPhysicalDeviceSurfacePresentModesKHR(
		app->gpu, app->surface_R, &presentModeCount_R, presentModes_R);
	assert(!err);

	VkExtent2D swapchainExtent_L, swapchainExtent_R;

	swapchainExtent_R = { app->width, app->height };
	swapchainExtent_L = { app->width, app->height };
	

	// width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
	if ((surfCapabilities_L.currentExtent.width == 0xFFFFFFFF) ||
		(surfCapabilities_R.currentExtent.width == 0xFFFFFFFF))
	{
		// If the surface size is undefined, the size is set to the size
		// of the images requested, which must fit within the minimum and
		// maximum values.
		
		if (swapchainExtent_L.width < surfCapabilities_L.minImageExtent.width) 
		{
			swapchainExtent_L.width = surfCapabilities_L.minImageExtent.width;
		}
		else if (swapchainExtent_L.width > surfCapabilities_L.maxImageExtent.width) 
		{
			swapchainExtent_L.width = surfCapabilities_L.maxImageExtent.width;
		}

		if (swapchainExtent_L.height < surfCapabilities_L.minImageExtent.height)
		{
			swapchainExtent_L.height = surfCapabilities_L.minImageExtent.height;
		}
		else if (swapchainExtent_L.height > surfCapabilities_L.maxImageExtent.height) 
		{
			swapchainExtent_L.height = surfCapabilities_L.maxImageExtent.height;
		}
	
		if (swapchainExtent_R.width < surfCapabilities_R.minImageExtent.width) {
			swapchainExtent_R.width = surfCapabilities_R.minImageExtent.width;
		}
		else if (swapchainExtent_R.width > surfCapabilities_R.maxImageExtent.width) {
			swapchainExtent_R.width = surfCapabilities_R.maxImageExtent.width;
		}

		if (swapchainExtent_R.height < surfCapabilities_R.minImageExtent.height) {
			swapchainExtent_R.height = surfCapabilities_R.minImageExtent.height;
		}
		else if (swapchainExtent_R.height > surfCapabilities_R.maxImageExtent.height) {
			swapchainExtent_R.height = surfCapabilities_R.maxImageExtent.height;
		}
	
	}
	else
	{
		// If the surface size is defined, the swap chain size must match
		

		if (app->height != surfCapabilities_R.currentExtent.height)
		{
			swapchainExtent_R.height = surfCapabilities_R.currentExtent.height;
			app->height = surfCapabilities_R.currentExtent.height;
			swapchainExtent_L.height = surfCapabilities_R.currentExtent.height;
		}

		if (app->width != surfCapabilities_R.currentExtent.width)
		{
			swapchainExtent_R.width = surfCapabilities_R.currentExtent.width;
			app->width = surfCapabilities_R.currentExtent.width;
			swapchainExtent_L.width = surfCapabilities_R.currentExtent.width;
		}
		
		if (app->height != surfCapabilities_L.currentExtent.height)
		{
			swapchainExtent_L.height = surfCapabilities_L.currentExtent.height;
			app->height = surfCapabilities_L.currentExtent.height;
			swapchainExtent_R.height = surfCapabilities_L.currentExtent.height;
		}

		if (app->width != surfCapabilities_L.currentExtent.width)
		{
			app->width = surfCapabilities_L.currentExtent.width;
			swapchainExtent_L.width = app->width;
			swapchainExtent_R.width = app->width;
		}
	}

	// The FIFO present mode is guaranteed by the spec to be supported
	// and to have no tearing.  It's a great default present mode to use.
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

	//  There are times when you may wish to use another present mode.  The
	//  following code shows how to select them, and the comments provide some
	//  reasons you may wish to use them.
	//
	// It should be noted that Vulkan 1.0 doesn't provide a method for
	// synchronizing rendering with the presentation engine's display.  There
	// is a method provided for throttling rendering with the display, but
	// there are some presentation engines for which this method will not work.
	// If an application doesn't throttle its rendering, and if it renders much
	// faster than the refresh rate of the display, this can waste power on
	// mobile devices.  That is because power is being spent rendering images
	// that may never be seen.

	// VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care about
	// tearing, or have some way of synchronizing their rendering with the
	// display.
	// VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications thaft
	// generally render a new presentable image every refresh cycle, but are
	// occasionally early.  In this case, the application wants the new image
	// to be displayed instead of the previously-queued-for-presentation image
	// that has not yet been displayed.
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
	// render a new presentable image every refresh cycle, but are occasionally
	// late.  In this case (perhaps because of stuttering/latency concerns),
	// the application wants the late image to be immediately displayed, even
	// though that may mean some tearing.

	if (app->presentMode != swapchainPresentMode) {

		for (size_t i = 0; i < presentModeCount_L; ++i) {
			if (presentModes_L[i] == app->presentMode) {
				swapchainPresentMode = app->presentMode;
				break;
			}
		}
	}

	if (swapchainPresentMode != app->presentMode) {
		ERR_EXIT(L"Present mode specified is not supported\n", L"Present mode unsupported");
	}

	// Determine the number of VkImages to use in the swap chain.
	// Application desires to acquire 3 images at a time for triple
	// buffering
	uint32_t desiredNumOfSwapchainImages = 3;
	if (desiredNumOfSwapchainImages < surfCapabilities_L.minImageCount) {
		desiredNumOfSwapchainImages = surfCapabilities_L.minImageCount;
	}
	if (desiredNumOfSwapchainImages < surfCapabilities_R.minImageCount) {
		desiredNumOfSwapchainImages = surfCapabilities_R.minImageCount;
	}

	// If maxImageCount is 0, we can ask for as many images as we want;
	// otherwise we're limited to maxImageCount

	if ((surfCapabilities_L.maxImageCount > 0) &&
		(desiredNumOfSwapchainImages > (surfCapabilities_L.maxImageCount / 2))) {
		// Application must settle for fewer images than desired:
		desiredNumOfSwapchainImages = (surfCapabilities_L.maxImageCount / 2);
	}


	if ((surfCapabilities_R.maxImageCount > 0) &&
		(desiredNumOfSwapchainImages > (surfCapabilities_R.maxImageCount / 2))) {
		// Application must settle for fewer images than desired:
		desiredNumOfSwapchainImages = (surfCapabilities_R.maxImageCount / 2);
	}

	VkSurfaceTransformFlagBitsKHR preTransform_L, preTransform_R;
	if (surfCapabilities_L.supportedTransforms &
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform_L = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else {
		preTransform_L = surfCapabilities_L.currentTransform;
	}

	if (surfCapabilities_R.supportedTransforms &
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform_R = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else {
		preTransform_R = surfCapabilities_R.currentTransform;
	}

	// Find a supported composite alpha mode - one of these is guaranteed to be set
	VkCompositeAlphaFlagBitsKHR compositeAlpha_L = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR compositeAlpha_R = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (uint32_t i = 0; i < sizeof(compositeAlphaFlags); i++) {
		if (surfCapabilities_L.supportedCompositeAlpha & compositeAlphaFlags[i]) {
			compositeAlpha_L = compositeAlphaFlags[i];
			break;
		}
		if (surfCapabilities_R.supportedCompositeAlpha & compositeAlphaFlags[i]) {
			compositeAlpha_R = compositeAlphaFlags[i];
			break;
		}
	}

	VkSwapchainCreateInfoKHR swapchain_ci_L, swapchain_ci_R;

	swapchain_ci_L.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci_L.pNext = NULL;
	swapchain_ci_L.surface = app->surface_L;
	swapchain_ci_L.minImageCount = desiredNumOfSwapchainImages;
	swapchain_ci_L.imageFormat = app->format;
	swapchain_ci_L.imageColorSpace = app->color_space;
	swapchain_ci_L.imageExtent =
	{
		swapchainExtent_L.width, swapchainExtent_L.height,
	};
	swapchain_ci_L.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci_L.preTransform = preTransform_L;
	swapchain_ci_L.compositeAlpha = compositeAlpha_L;
	swapchain_ci_L.imageArrayLayers = 1;
	swapchain_ci_L.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci_L.queueFamilyIndexCount = 0;
	swapchain_ci_L.pQueueFamilyIndices = NULL;
	swapchain_ci_L.presentMode = swapchainPresentMode;
	swapchain_ci_L.oldSwapchain = oldSwapchain_L;
	swapchain_ci_L.clipped = true;

	swapchain_ci_R.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci_R.pNext = NULL;
	swapchain_ci_R.surface = app->surface_R;
	swapchain_ci_R.minImageCount = desiredNumOfSwapchainImages;
	swapchain_ci_R.imageFormat = app->format;
	swapchain_ci_R.imageColorSpace = app->color_space;
	swapchain_ci_R.imageExtent =
	{
		swapchainExtent_R.width, swapchainExtent_R.height,
	};
	swapchain_ci_R.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci_R.preTransform = preTransform_R;
	swapchain_ci_R.compositeAlpha = compositeAlpha_R;
	swapchain_ci_R.imageArrayLayers = 1;
	swapchain_ci_R.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci_R.queueFamilyIndexCount = 0;
	swapchain_ci_R.pQueueFamilyIndices = NULL;
	swapchain_ci_R.presentMode = swapchainPresentMode;
	swapchain_ci_R.oldSwapchain = oldSwapchain_R;
	swapchain_ci_R.clipped = true;
	uint32_t i;

	err = app->fpCreateSwapchainKHR(app->device, &swapchain_ci_L, NULL,
		&app->swapchain_L);
	assert(!err);

	err = app->fpCreateSwapchainKHR(app->device, &swapchain_ci_R, NULL,
		&app->swapchain_R);
	assert(!err);

	// If we just re-created an existing swapchain, we should destroy the old
	// swapchain at this point.
	// Note: destroying the swapchain also cleans up all its associated
	// presentable images once the platform is done with them.
	if (oldSwapchain_L != VK_NULL_HANDLE) {
		// AMD driver times out waiting on fences used in AcquireNextImage on
		// a swapchain that is subsequently destroyed before the wait.
		vkWaitForFences(app->device, FRAME_LAG, app->fences, VK_TRUE, UINT64_MAX);
		app->fpDestroySwapchainKHR(app->device, oldSwapchain_L, NULL);
	}

	if (oldSwapchain_R != VK_NULL_HANDLE) {
		// AMD driver times out waiting on fences used in AcquireNextImage on
		// a swapchain that is subsequently destroyed before the wait.
		vkWaitForFences(app->device, FRAME_LAG, app->fences, VK_TRUE, UINT64_MAX);
		app->fpDestroySwapchainKHR(app->device, oldSwapchain_R, NULL);
	}

	err = app->fpGetSwapchainImagesKHR(app->device, app->swapchain_L,
		&app->swapchainImageCount, NULL);
	assert(!err);

	err = app->fpGetSwapchainImagesKHR(app->device, app->swapchain_R,
		&app->swapchainImageCount, NULL);
	assert(!err);

	VkImage *swapchainImages_L =
		(VkImage *)malloc(app->swapchainImageCount * sizeof(VkImage));
	assert(swapchainImages_L);
	err = app->fpGetSwapchainImagesKHR(app->device, app->swapchain_L,
		&app->swapchainImageCount,
		swapchainImages_L);
	assert(!err);

	VkImage *swapchainImages_R =
		(VkImage *)malloc(app->swapchainImageCount * sizeof(VkImage));
	assert(swapchainImages_R);
	err = app->fpGetSwapchainImagesKHR(app->device, app->swapchain_R,
		&app->swapchainImageCount,
		swapchainImages_R);
	assert(!err);

	app->swapchain_image_resources_L = (SwapchainImageResources *)malloc(sizeof(SwapchainImageResources) *
		app->swapchainImageCount);
	assert(app->swapchain_image_resources_L);

	app->swapchain_image_resources_R = (SwapchainImageResources *)malloc(sizeof(SwapchainImageResources) *
		app->swapchainImageCount);
	assert(app->swapchain_image_resources_R);

	VkFenceCreateInfo fence_ci = {
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		NULL,
		VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (i = 0; i < app->swapchainImageCount; i++)
	{
		VkImageViewCreateInfo color_image_view_L{};
		VkImageViewCreateInfo color_image_view_R{};
		color_image_view_L.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view_L.pNext = NULL;
		color_image_view_L.format = app->format;
		color_image_view_L.components =
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};
		color_image_view_L.subresourceRange =
		{
			VK_IMAGE_ASPECT_COLOR_BIT, //aspectMask
			0,												//baseMipLevel
			1,												//levelCount
			0,												//baseArrayLayer
			1												//layerCount  
		};
		color_image_view_L.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view_L.flags = 0;
		/////////////////////////////////////////////////////////////////////////////////////////////
		color_image_view_R.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view_R.pNext = NULL;
		color_image_view_R.format = app->format;
		color_image_view_R.components =
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};
		color_image_view_R.subresourceRange =
		{
			VK_IMAGE_ASPECT_COLOR_BIT, //aspectMask
			0,												//baseMipLevel
			1,												//levelCount
			0,												//baseArrayLayer
			1												//layerCount  
		};
		color_image_view_R.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view_R.flags = 0;
		///////////////////////////////////////////////////////////////////////////////////////////

		app->swapchain_image_resources_L[i].image = swapchainImages_L[i];
		app->swapchain_image_resources_R[i].image = swapchainImages_R[i];

		color_image_view_L.image = app->swapchain_image_resources_L[i].image;
		color_image_view_R.image = app->swapchain_image_resources_R[i].image;

		err = vkCreateImageView(app->device, &color_image_view_L, NULL,
			&app->swapchain_image_resources_L[i].view);
		assert(!err);
		err = vkCreateImageView(app->device, &color_image_view_R, NULL,
			&app->swapchain_image_resources_R[i].view);
		assert(!err);

		err = vkCreateFence(app->device, &fence_ci, NULL, &app->swapchain_image_resources_L[i].fence);
		assert(!err);

		err = vkCreateFence(app->device, &fence_ci, NULL, &app->swapchain_image_resources_R[i].fence);
		assert(!err);
	}

	if (app->VK_GOOGLE_display_timing_enabled) {
		VkRefreshCycleDurationGOOGLE rc_dur_L, rc_dur_R;
		err = app->fpGetRefreshCycleDurationGOOGLE(app->device,
			app->swapchain_L,
			&rc_dur_L);
		assert(!err);
		err = app->fpGetRefreshCycleDurationGOOGLE(app->device,
			app->swapchain_R,
			&rc_dur_R);
		assert(!err);
		app->refresh_duration_L = rc_dur_L.refreshDuration;
		app->refresh_duration_R = rc_dur_R.refreshDuration;

		app->syncd_with_actual_presents = false;
		// Initially target 1X the refresh duration:
		app->target_IPD_L = app->refresh_duration_L;
		app->target_IPD_R = app->refresh_duration_R;

		app->refresh_duration_multiplier = 1;
		app->prev_desired_present_time = 0;
		app->next_present_id = 1;
	}

	if (NULL != presentModes_L) {
		free(presentModes_L);
	}
	if (NULL != presentModes_R) {
		free(presentModes_R);
	}
}

void VkApp::PrepareDepth(VkApp* app)
{
	const VkFormat depth_format = VK_FORMAT_D16_UNORM;
	VkImageCreateInfo depthImageInfo;
	depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageInfo.pNext = NULL;
	depthImageInfo.flags = 0;
	depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
	depthImageInfo.format = depth_format;
	depthImageInfo.extent = { app->width,app->height, 1 };
	depthImageInfo.mipLevels = 1;
	depthImageInfo.arrayLayers = 1;
	depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;


	VkImageViewCreateInfo depthViewInfo;
	depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthViewInfo.pNext = NULL;
	depthViewInfo.image = VK_NULL_HANDLE;
	depthViewInfo.format = depth_format;
	depthViewInfo.subresourceRange =
	{ VK_IMAGE_ASPECT_DEPTH_BIT,	//.aspectMask  
		0,												//.baseMipLevel
		1,												//.levelCount
		0,												//.baseArrayLayer
		1												//.layerCount
	};
	depthViewInfo.flags = 0;
	depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	depthViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	depthViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	depthViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

	VkMemoryRequirements mem_reqs_L, mem_reqs_R;
	VkResult err;
	bool pass;

	app->depth_L.format = depth_format;
	app->depth_R.format = depth_format;

	/* create image */
	err = vkCreateImage(app->device, &depthImageInfo, NULL, &app->depth_L.image);
	assert(!err);

	vkGetImageMemoryRequirements(app->device, app->depth_L.image, &mem_reqs_L);
	assert(!err);

	app->depth_L.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	app->depth_L.mem_alloc.pNext = NULL;
	app->depth_L.mem_alloc.allocationSize = mem_reqs_L.size;
	app->depth_L.mem_alloc.memoryTypeIndex = 0;

	pass = CheckMemoryType(app, mem_reqs_L.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&app->depth_L.mem_alloc.memoryTypeIndex);

	assert(pass);

	err = vkCreateImage(app->device, &depthImageInfo, NULL, &app->depth_R.image);
	assert(!err);

	vkGetImageMemoryRequirements(app->device, app->depth_R.image, &mem_reqs_R);
	assert(!err);

	app->depth_R.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	app->depth_R.mem_alloc.pNext = NULL;
	app->depth_R.mem_alloc.allocationSize = mem_reqs_R.size;
	app->depth_R.mem_alloc.memoryTypeIndex = 0;

	pass = CheckMemoryType(app, mem_reqs_R.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&app->depth_R.mem_alloc.memoryTypeIndex);

	assert(pass);


	/* allocate memory */
	err = vkAllocateMemory(app->device, &app->depth_L.mem_alloc, NULL,
		&app->depth_L.mem);
	assert(!err);
	err = vkAllocateMemory(app->device, &app->depth_R.mem_alloc, NULL,
		&app->depth_R.mem);
	assert(!err);

	/* bind memory */
	err =
		vkBindImageMemory(app->device, app->depth_L.image, app->depth_L.mem, 0);
	assert(!err);
	err =
		vkBindImageMemory(app->device, app->depth_R.image, app->depth_R.mem, 0);
	assert(!err);

	/* create image view */
	depthViewInfo.image = app->depth_L.image;
	err = vkCreateImageView(app->device, &depthViewInfo, NULL, &app->depth_L.view);
	assert(!err);
	depthViewInfo.image = app->depth_R.image;
	err = vkCreateImageView(app->device, &depthViewInfo, NULL, &app->depth_R.view);
	assert(!err);
	depthViewInfo.image = VK_NULL_HANDLE;
}

bool VkApp::CheckMemoryType(VkApp* app, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
{
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((app->memory_properties.memoryTypes[i].propertyFlags &
				requirements_mask) == requirements_mask)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}

void VkApp::PrepareTextures(VkApp *app) {
	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
	VkFormatProperties props;
	uint32_t i;

	vkGetPhysicalDeviceFormatProperties(app->gpu, tex_format, &props);

	for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
		VkResult err;

		if ((props.linearTilingFeatures &
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
			!app->use_staging_buffer) {
			/* Device can texture using linear textures */
			PrepareTextureImage(
				app, tex_files[i], &app->textures[i], VK_IMAGE_TILING_LINEAR,
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			// Nothing in the pipeline needs to be complete to start, and don't allow fragment
			// shader to run until layout transition completes
			SetImageLayout(app, app->textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED, app->textures[i].imageLayout,
				VK_ACCESS_HOST_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			app->staging_texture.image = 0;
		}
		else if (props.optimalTilingFeatures &
			VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
			/* Must use staging buffer to copy linear texture to optimized */

			memset(&app->staging_texture, 0, sizeof(app->staging_texture));
			PrepareTextureImage(
				app, tex_files[i], &app->staging_texture, VK_IMAGE_TILING_LINEAR,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			PrepareTextureImage(
				app, tex_files[i], &app->textures[i], VK_IMAGE_TILING_OPTIMAL,
				(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			SetImageLayout(app, app->staging_texture.image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_HOST_WRITE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT);

			SetImageLayout(app, app->textures[i].image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_ACCESS_HOST_WRITE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT);

			VkImageCopy copy_region;
			copy_region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copy_region.srcOffset = { 0, 0, 0 };
			copy_region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copy_region.dstOffset = { 0, 0, 0 };
			copy_region.extent = { app->staging_texture.tex_width,
				app->staging_texture.tex_height, 1 };

			vkCmdCopyImage(
				app->cmd_L, app->staging_texture.image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->textures[i].image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
			vkCmdCopyImage(
				app->cmd_R, app->staging_texture.image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, app->textures[i].image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

			SetImageLayout(app, app->textures[i].image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				app->textures[i].imageLayout,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		}
		else {
			/* Can't support VK_FORMAT_R8G8B8A8_UNORM !? */
			assert(!"No support for R8G8B8A8_UNORM as texture image format");
		}

		const VkSamplerCreateInfo sampler = {
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,	//.sType
			NULL,																//.pNext 	
			0,																		//.flags
			VK_FILTER_NEAREST,											//.magFilter 
			VK_FILTER_NEAREST,											//.minFilter 
			VK_SAMPLER_MIPMAP_MODE_NEAREST,				//.mipmapMode 
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,	//.addressModeU 
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,	//.addressModeV 
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,	//.addressModeW
			0.0f,																	//.mipLodBias 
			VK_FALSE,															//.anisotropyEnable 
			1,																		//.maxAnisotropy 
			0,
			VK_COMPARE_OP_NEVER,									//.compareOp 
			0.0f,																	//.minLod			
			0.0f,																	//.maxLod 
			VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,		//.borderColor
			VK_FALSE,															//.unnormalizedCoordinates		
		};

		VkImageViewCreateInfo view;
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.pNext = NULL;
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = tex_format;
		view.components =
		{
			VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A,
		};
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.flags = 0;

		/* create sampler */
		err = vkCreateSampler(app->device, &sampler, NULL,
			&app->textures[i].sampler);
		assert(!err);

		/* create image view */
		view.image = app->textures[i].image;
		err = vkCreateImageView(app->device, &view, NULL,
			&app->textures[i].view);
		assert(!err);
	}
}

void VkApp::PrepareTextureImage(VkApp *app, const char *filename, struct texture_object *tex_obj,
	VkImageTiling tiling, VkImageUsageFlags usage, VkFlags required_props)
{
	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
	uint32_t tex_width;
	uint32_t tex_height;
	VkResult  err;
	bool pass;

	if (!LoadTexture(filename, NULL, NULL, &tex_width, &tex_height)) {
		ERR_EXIT(L"Failed to load textures", L"Load Texture Failure");
	}

	tex_obj->tex_width = tex_width;
	tex_obj->tex_height = tex_height;

	const VkImageCreateInfo imageCreateInfo
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	//.sType
		NULL,															//.pNext 
		0,																	//.flags 
		VK_IMAGE_TYPE_2D,										//.imageType
		tex_format,													//.format 
		{ tex_width, tex_height, 1 },							//.extent 
		1,																	//.mipLevels
		1,																	//.arrayLayers 
		VK_SAMPLE_COUNT_1_BIT,								//.samples
		tiling,															//.tiling 
		usage,															//.usage 							
		VK_SHARING_MODE_EXCLUSIVE,						//.VkSharingMode   
		0,																	//.queueFamilyIndexCount
		nullptr,															//.pQueueFamilyIndices
		VK_IMAGE_LAYOUT_PREINITIALIZED,				//.initialLayout 
	};

	VkMemoryRequirements mem_reqs;

	err =
		vkCreateImage(app->device, &imageCreateInfo, NULL, &tex_obj->image);
	assert(!err);

	vkGetImageMemoryRequirements(app->device, tex_obj->image, &mem_reqs);

	tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	tex_obj->mem_alloc.pNext = NULL;
	tex_obj->mem_alloc.allocationSize = mem_reqs.size;
	tex_obj->mem_alloc.memoryTypeIndex = 0;

	pass = CheckMemoryType(app, mem_reqs.memoryTypeBits, required_props,
		&tex_obj->mem_alloc.memoryTypeIndex);
	assert(pass);

	/* allocate memory */
	err = vkAllocateMemory(app->device, &tex_obj->mem_alloc, NULL,
		&(tex_obj->mem));
	assert(!err);

	/* bind memory */
	err = vkBindImageMemory(app->device, tex_obj->image, tex_obj->mem, 0);
	assert(!err);

	if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		const VkImageSubresource subres
		{
			VK_IMAGE_ASPECT_COLOR_BIT, //.aspectMask 
			0,												//.mipLevel
			0,												//.arrayLayer 
		};
		VkSubresourceLayout layout;
		void *data;

		vkGetImageSubresourceLayout(app->device, tex_obj->image, &subres,
			&layout);

		err = vkMapMemory(app->device, tex_obj->mem, 0,
			tex_obj->mem_alloc.allocationSize, 0, &data);
		assert(!err);

		if (!LoadTexture(filename, (uint8_t*)data, &layout, &tex_width, &tex_height))
		{
			fprintf(stderr, "Error loading texture: %s\n", filename);
		}

		vkUnmapMemory(app->device, tex_obj->mem);
	}

	tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

bool VkApp::LoadTexture(const char *filename, uint8_t *rgba_data,
	VkSubresourceLayout *layout, uint32_t *width, uint32_t *height)
{
	FILE *fPtr = fopen(filename, "rb");
	char header[256], *cPtr, *tmp;

	if (!fPtr)
		return false;

	cPtr = fgets(header, 256, fPtr); // P6
	if (cPtr == NULL || strncmp(header, "P6\n", 3)) {
		fclose(fPtr);
		return false;
	}

	do {
		cPtr = fgets(header, 256, fPtr);
		if (cPtr == NULL) {
			fclose(fPtr);
			return false;
		}
	} while (!strncmp(header, "#", 1));

	sscanf(header, "%u %u", width, height);
	if (rgba_data == NULL) {
		fclose(fPtr);
		return true;
	}
	tmp = fgets(header, 256, fPtr); // Format
	(void)tmp;
	if (cPtr == NULL || strncmp(header, "255\n", 3)) {
		fclose(fPtr);
		return false;
	}

	for (uint32_t y = 0; y < *height; y++) {
		uint8_t *rowPtr = rgba_data;
		for (uint32_t x = 0; x < *width; x++) {
			size_t s = fread(rowPtr, 3, 1, fPtr);
			(void)s;
			rowPtr[3] = 255; /* Alpha of 1 */
			rowPtr += 4;
		}
		rgba_data += layout->rowPitch;
	}
	fclose(fPtr);
	return true;
}

void VkApp::SetImageLayout(VkApp *app, VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout old_image_layout,
	VkImageLayout new_image_layout,
	VkAccessFlagBits srcAccessMask,
	VkPipelineStageFlags src_stages,
	VkPipelineStageFlags dest_stages)
{
	assert((app->cmd_L) && (app->cmd_R));

	VkImageMemoryBarrier image_memory_barrier;
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = srcAccessMask;
	image_memory_barrier.dstAccessMask = 0;
	image_memory_barrier.oldLayout = old_image_layout;
	image_memory_barrier.newLayout = new_image_layout;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange = { aspectMask, 0, 1, 0, 1 };

	switch (new_image_layout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		/* Make sure anything that was copying from this image has completed */
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		image_memory_barrier.dstAccessMask =
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		break;

	default:
		image_memory_barrier.dstAccessMask = 0;
		break;
	}


	VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

	vkCmdPipelineBarrier(app->cmd_L, src_stages, dest_stages, 0, 0, NULL, 0,
		NULL, 1, pmemory_barrier);
	vkCmdPipelineBarrier(app->cmd_R, src_stages, dest_stages, 0, 0, NULL, 0,
		NULL, 1, pmemory_barrier);
}

void VkApp::PrepareCubeBuffers(VkApp* app)
{
	VkBufferCreateInfo buf_info_L, buf_info_R;
	VkMemoryRequirements mem_reqs_L, mem_reqs_R;
	VkMemoryAllocateInfo mem_alloc_L, mem_alloc_R;
	uint8_t *pData_L;
	uint8_t *pData_R;
	mat4x4 MVP_L, MVP_R, VP_L, VP_R;
	VkResult  err;
	bool  pass;
	struct vktexcube_vs_uniform data_L, data_R;

	mat4x4_mul(VP_L, app->projection_matrix, app->view_matrix_l);
	mat4x4_mul(VP_R, app->projection_matrix, app->view_matrix_r);
	mat4x4_mul(MVP_L, VP_L, app->model_matrix);
	mat4x4_mul(MVP_R, VP_R, app->model_matrix);
	memcpy(data_L.mvp, MVP_L, sizeof(MVP_L));
	memcpy(data_R.mvp, MVP_R, sizeof(MVP_R));
	//    dumpMatrix("MVP", MVP);

	for (unsigned int i = 0; i < 12 * 3; i++) {
		data_L.position[i][0] = g_vertex_buffer_data[i * 3];
		data_L.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
		data_L.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
		data_L.position[i][3] = 1.0f;
		data_L.attr[i][0] = g_uv_buffer_data[2 * i];
		data_L.attr[i][1] = g_uv_buffer_data[2 * i + 1];
		data_L.attr[i][2] = 0;
		data_L.attr[i][3] = 0;

		data_R.position[i][0] = g_vertex_buffer_data[i * 3];
		data_R.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
		data_R.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
		data_R.position[i][3] = 1.0f;
		data_R.attr[i][0] = g_uv_buffer_data[2 * i];
		data_R.attr[i][1] = g_uv_buffer_data[2 * i + 1];
		data_R.attr[i][2] = 0;
		data_R.attr[i][3] = 0;
	}

	memset(&buf_info_L, 0, sizeof(buf_info_L));
	buf_info_L.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info_L.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info_L.size = sizeof(data_L);

	memset(&buf_info_R, 0, sizeof(buf_info_R));
	buf_info_R.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info_R.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info_R.size = sizeof(data_R);

	for (unsigned int i = 0; i < app->swapchainImageCount; i++) {
		err =
			vkCreateBuffer(app->device, &buf_info_L, NULL,
				&app->swapchain_image_resources_L[i].uniform_buffer);
		assert(!err);

		err =
			vkCreateBuffer(app->device, &buf_info_R, NULL,
				&app->swapchain_image_resources_R[i].uniform_buffer);
		assert(!err);

		vkGetBufferMemoryRequirements(app->device,
			app->swapchain_image_resources_L[i].uniform_buffer,
			&mem_reqs_L);

		vkGetBufferMemoryRequirements(app->device,
			app->swapchain_image_resources_R[i].uniform_buffer,
			&mem_reqs_R);

		mem_alloc_L.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc_L.pNext = NULL;
		mem_alloc_L.allocationSize = mem_reqs_L.size;
		mem_alloc_L.memoryTypeIndex = 0;
		mem_alloc_R.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc_R.pNext = NULL;
		mem_alloc_R.allocationSize = mem_reqs_R.size;
		mem_alloc_R.memoryTypeIndex = 0;

		pass = CheckMemoryType(
			app, mem_reqs_L.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&mem_alloc_L.memoryTypeIndex);
		assert(pass);

		pass = CheckMemoryType(
			app, mem_reqs_R.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&mem_alloc_R.memoryTypeIndex);
		assert(pass);

		err = vkAllocateMemory(app->device, &mem_alloc_L, NULL,
			&app->swapchain_image_resources_L[i].uniform_memory);
		assert(!err);
		err = vkAllocateMemory(app->device, &mem_alloc_R, NULL,
			&app->swapchain_image_resources_R[i].uniform_memory);
		assert(!err);

		err = vkMapMemory(app->device, app->swapchain_image_resources_L[i].uniform_memory, 0,
			VK_WHOLE_SIZE, 0, (void **)&pData_L);
		assert(!err);

		memcpy(pData_L, &data_L, sizeof(data_L));

		vkUnmapMemory(app->device, app->swapchain_image_resources_L[i].uniform_memory);

		err = vkBindBufferMemory(app->device, app->swapchain_image_resources_L[i].uniform_buffer,
			app->swapchain_image_resources_L[i].uniform_memory, 0);
		assert(!err);

		err = vkMapMemory(app->device, app->swapchain_image_resources_R[i].uniform_memory, 0,
			VK_WHOLE_SIZE, 0, (void **)&pData_R);
		assert(!err);

		memcpy(pData_R, &data_R, sizeof(data_R));

		vkUnmapMemory(app->device, app->swapchain_image_resources_R[i].uniform_memory);

		err = vkBindBufferMemory(app->device, app->swapchain_image_resources_R[i].uniform_buffer,
			app->swapchain_image_resources_R[i].uniform_memory, 0);
		assert(!err);
	}
}

void VkApp::PrepareDescriptorLayout(VkApp *app)
{
	const VkDescriptorSetLayoutBinding layout_bindings[2]
	{
		{
			0,																	//.binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	//.descriptorType
			1,																	//.descriptorCount 
			VK_SHADER_STAGE_VERTEX_BIT,					//.stageFlags
			NULL																//.pImmutableSamplers
		},
		{
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			DEMO_TEXTURE_COUNT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			NULL
		}
	};

	const VkDescriptorSetLayoutCreateInfo descriptor_layout
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,	//.sType 
		NULL,																							//.pNext
		0,																									//.flags
		2,																									//.bindingCount
		layout_bindings,																			//.pBindings	
	};
	VkResult err;

	err = vkCreateDescriptorSetLayout(app->device, &descriptor_layout, NULL,
		&app->desc_layout);
	assert(!err);

	const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,			//.sType
		NULL,																						//.pNext 	
		0,																								//.flags
		1,																								//.setLayoutCount
		&app->desc_layout,																	//.pSetLayouts 

	};

	err = vkCreatePipelineLayout(app->device, &pPipelineLayoutCreateInfo, NULL,
		&app->pipeline_layout);
	assert(!err);
}

void VkApp::PrepareRenderPass(VkApp* app)
{
	// The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
	// because at the start of the renderpass, we don't care about their contents.
	// At the start of the subpass, the color attachment's layout will be transitioned
	// to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
	// will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
	// the renderpass, the color attachment's layout will be transitioned to
	// LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
	// the renderpass, no barriers are necessary.
	const VkAttachmentDescription attachments[2]
	{
		{
			0,																	//.flags
			app->format,												//.format
			VK_SAMPLE_COUNT_1_BIT,							//.samples
			VK_ATTACHMENT_LOAD_OP_CLEAR,				//.loadOp
			VK_ATTACHMENT_STORE_OP_STORE,			//.storeOp 
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,		//.stencilLoadOp 
			VK_ATTACHMENT_STORE_OP_DONT_CARE,	//.stencilStoreOp 
			VK_IMAGE_LAYOUT_UNDEFINED,					//.initialLayout 
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR	//.finalLayout 
		},
		{
			0,
			app->depth_L.format,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		}
	};
	const VkAttachmentReference color_reference
	{
		0,																					//.attachment 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL		//.layout
	};

	const VkAttachmentReference depth_reference
	{
		1,																								//.attachment 
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, //.layout
	};

	const VkSubpassDescription subpass
	{
		0,																//.flags 
		VK_PIPELINE_BIND_POINT_GRAPHICS,		//.pipelineBindPoint 
		0,															//.inputAttachmentCount
		NULL,														//.pInputAttachments 
		1,																//.colorAttachmentCount 
		&color_reference,									//.pColorAttachments
		NULL,														//.pResolveAttachments 
		&depth_reference,									//.pDepthStencilAttachment 
		0,																//.preserveAttachmentCount 
		NULL															//.pPreserveAttachments 
	};

	const VkRenderPassCreateInfo rp_info
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			//.sType 
		NULL,																				//.pNext
		0,																						//.flags 
		2,																						//.attachmentCount 
		attachments,																	//.pAttachments 
		1,																						//.subpassCount 
		&subpass,																			//.pSubpasses
		0,																						//.dependencyCount
		NULL																					//.pDependencies 
	};
	VkResult  err;

	err = vkCreateRenderPass(app->device, &rp_info, NULL, &app->render_pass);
	assert(!err);
}

void VkApp::PreparePipeline(VkApp* app)
{
	VkGraphicsPipelineCreateInfo pipeline;
	VkPipelineCacheCreateInfo pipelineCache;
	VkPipelineVertexInputStateCreateInfo vi;
	VkPipelineInputAssemblyStateCreateInfo ia;
	VkPipelineRasterizationStateCreateInfo rs;
	VkPipelineColorBlendStateCreateInfo cb;
	VkPipelineDepthStencilStateCreateInfo ds;
	VkPipelineViewportStateCreateInfo vp;
	VkPipelineMultisampleStateCreateInfo ms;
	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo dynamicState;
	VkResult err;

	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	memset(&dynamicState, 0, sizeof dynamicState);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables;

	memset(&pipeline, 0, sizeof(pipeline));
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.layout = app->pipeline_layout;

	memset(&vi, 0, sizeof(vi));
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	memset(&ia, 0, sizeof(ia));
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	memset(&rs, 0, sizeof(rs));
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_BACK_BIT;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.lineWidth = 1.0f;

	memset(&cb, 0, sizeof(cb));
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState att_state[1];
	memset(att_state, 0, sizeof(att_state));
	att_state[0].colorWriteMask = 0xf;
	att_state[0].blendEnable = VK_FALSE;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;

	memset(&vp, 0, sizeof(vp));
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_SCISSOR;

	memset(&ds, 0, sizeof(ds));
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.depthTestEnable = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;

	memset(&ms, 0, sizeof(ms));
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Two stages: vs and fs
	pipeline.stageCount = 2;
	VkPipelineShaderStageCreateInfo shaderStages[2];
	memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = PrepareVertexShader(app);
	shaderStages[0].pName = "main";

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = PrepareFragmentShader(app);
	shaderStages[1].pName = "main";

	memset(&pipelineCache, 0, sizeof(pipelineCache));
	pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	err = vkCreatePipelineCache(app->device, &pipelineCache, NULL,
		&app->pipelineCache);
	assert(!err);

	pipeline.pVertexInputState = &vi;
	pipeline.pInputAssemblyState = &ia;
	pipeline.pRasterizationState = &rs;
	pipeline.pColorBlendState = &cb;
	pipeline.pMultisampleState = &ms;
	pipeline.pViewportState = &vp;
	pipeline.pDepthStencilState = &ds;
	pipeline.pStages = shaderStages;
	pipeline.renderPass = app->render_pass;
	pipeline.pDynamicState = &dynamicState;

	pipeline.renderPass = app->render_pass;

	err = vkCreateGraphicsPipelines(app->device, app->pipelineCache, 1,
		&pipeline, NULL, &app->pipeline);
	assert(!err);

	vkDestroyShaderModule(app->device, app->frag_shader_module, NULL);
	vkDestroyShaderModule(app->device, app->vert_shader_module, NULL);
}

VkShaderModule VkApp::PrepareVertexShader(VkApp *app)
{
	void *vertShaderCode;
	size_t size;

	vertShaderCode = ReadSPV("cube-vert.spv", &size);
	if (!vertShaderCode) {
		ERR_EXIT(L"Failed to load cube-vert.spv", L"Load Shader Failure");
	}

	app->vert_shader_module =
		PrepareShaderModule(app, vertShaderCode, size);

	free(vertShaderCode);


	return app->vert_shader_module;
}

VkShaderModule VkApp::PrepareFragmentShader(VkApp *app)
{

	void *fragShaderCode;
	size_t size;

	fragShaderCode = ReadSPV("cube-frag.spv", &size);
	if (!fragShaderCode)
	{
		ERR_EXIT(L"Failed to load cube-frag.spv", L"Load Shader Failure");
	}

	app->frag_shader_module =
		PrepareShaderModule(app, fragShaderCode, size);

	free(fragShaderCode);


	return app->frag_shader_module;
}


char* VkApp::ReadSPV(const char *filename, size_t *psize)
{
	long int size;
	size_t retval;
	void *shader_code;

	FILE *fp = fopen(filename, "rb");
	if (!fp)
		return NULL;

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);

	shader_code = malloc(size);
	retval = fread(shader_code, size, 1, fp);
	assert(retval == 1);

	*psize = size;

	fclose(fp);
	return (char*)shader_code;
}

VkShaderModule VkApp::PrepareShaderModule(VkApp* app, const void *code, size_t size)
{
	VkShaderModule module;
	VkShaderModuleCreateInfo moduleCreateInfo;
	VkResult  err;

	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = NULL;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (uint32_t*)code;
	moduleCreateInfo.flags = 0;
	err = vkCreateShaderModule(app->device, &moduleCreateInfo, NULL, &module);
	assert(!err);

	return module;
}

void VkApp::PrepareDescriptorPool(VkApp *app)
{
	const VkDescriptorPoolSize type_counts[2]
	{
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			//.type 
			app->swapchainImageCount								//.descriptorCount 
		},
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			app->swapchainImageCount * DEMO_TEXTURE_COUNT
		}
	};

	const VkDescriptorPoolCreateInfo desc_pool_L
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,	//.sType 
		NULL,																				//.pNext
		0,																						//.flags
		app->swapchainImageCount,											//.maxSets 
		2,																						//.poolSizeCount 
		type_counts,																	//.pPoolSizes 
	};

	const VkDescriptorPoolCreateInfo desc_pool_R
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,	//.sType 
		NULL,																				//.pNext
		0,																						//.flags
		app->swapchainImageCount,											//.maxSets 
		2,																						//.poolSizeCount 
		type_counts,																	//.pPoolSizes 
	};

	VkResult err;

	err = vkCreateDescriptorPool(app->device, &desc_pool_L, NULL, &app->desc_pool_L);
	assert(!err);
	err = vkCreateDescriptorPool(app->device, &desc_pool_R, NULL, &app->desc_pool_R);
	assert(!err);
}

void VkApp::PrepareDescriptorSet(VkApp *app)
{
	VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
	VkWriteDescriptorSet writes_L[2], writes_R[2];
	VkResult err;

	VkDescriptorSetAllocateInfo alloc_info_L
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		//.sType
		NULL,																						//.pNext 
		app->desc_pool_L,																	//.descriptorPool 
		1,																								//.descriptorSetCount 
		&app->desc_layout																	//.pSetLayouts
	};

	VkDescriptorSetAllocateInfo alloc_info_R
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		//.sType
		NULL,																						//.pNext 
		app->desc_pool_R,																	//.descriptorPool 
		1,																								//.descriptorSetCount 
		&app->desc_layout																	//.pSetLayouts
	};


	VkDescriptorBufferInfo buffer_info;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(struct vktexcube_vs_uniform);

	memset(&tex_descs, 0, sizeof(tex_descs));
	for (unsigned int i = 0; i < DEMO_TEXTURE_COUNT; i++) {
		tex_descs[i].sampler = app->textures[i].sampler;
		tex_descs[i].imageView = app->textures[i].view;
		tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	memset(&writes_L, 0, sizeof(writes_L));

	writes_L[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes_L[0].descriptorCount = 1;
	writes_L[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes_L[0].pBufferInfo = &buffer_info;

	writes_L[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes_L[1].dstBinding = 1;
	writes_L[1].descriptorCount = DEMO_TEXTURE_COUNT;
	writes_L[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes_L[1].pImageInfo = tex_descs;

	memset(&writes_R, 0, sizeof(writes_R));

	writes_R[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes_R[0].descriptorCount = 1;
	writes_R[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes_R[0].pBufferInfo = &buffer_info;

	writes_R[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes_R[1].dstBinding = 1;
	writes_R[1].descriptorCount = DEMO_TEXTURE_COUNT;
	writes_R[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes_R[1].pImageInfo = tex_descs;




	for (unsigned int i = 0; i < app->swapchainImageCount; i++) {
		err = vkAllocateDescriptorSets(app->device, &alloc_info_L, &app->swapchain_image_resources_L[i].descriptor_set);
		assert(!err);
		err = vkAllocateDescriptorSets(app->device, &alloc_info_R, &app->swapchain_image_resources_R[i].descriptor_set);
		assert(!err);
		buffer_info.buffer = app->swapchain_image_resources_L[i].uniform_buffer;
		writes_L[0].dstSet = app->swapchain_image_resources_L[i].descriptor_set;
		writes_L[1].dstSet = app->swapchain_image_resources_L[i].descriptor_set;

		buffer_info.buffer = app->swapchain_image_resources_R[i].uniform_buffer;
		writes_R[0].dstSet = app->swapchain_image_resources_R[i].descriptor_set;
		writes_R[1].dstSet = app->swapchain_image_resources_R[i].descriptor_set;

		vkUpdateDescriptorSets(app->device, 2, writes_L, 0, NULL);
		vkUpdateDescriptorSets(app->device, 2, writes_R, 0, NULL);
	}
}

void VkApp::PrepareFramebuffers(VkApp *app)
{
	VkImageView attachments_L[2], attachments_R[2];
	attachments_L[1] = app->depth_L.view;
	attachments_R[1] = app->depth_R.view;

	const VkFramebufferCreateInfo fb_info_L
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,		//.sType 
		NULL,																				//	.pNext 
		0,																						//.flags
		app->render_pass,															//.renderPass 
		2,																						//.attachmentCount
		attachments_L,																//.pAttachments 
		app->width,																		//.width 
		app->height,																	//.height 
		1,																						//.layers 
	};
	const VkFramebufferCreateInfo fb_info_R
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,		//.sType 
		NULL,																				//	.pNext 
		0,																						//.flags
		app->render_pass,															//.renderPass 
		2,																						//.attachmentCount
		attachments_R,																//.pAttachments 
		app->width,																		//.width 
		app->height,																	//.height 
		1,																						//.layers 
	};

	VkResult err;
	uint32_t i;

	for (i = 0; i < app->swapchainImageCount; i++)
	{
		attachments_L[0] = app->swapchain_image_resources_L[i].view;
		attachments_R[0] = app->swapchain_image_resources_R[i].view;

		err = vkCreateFramebuffer(app->device, &fb_info_L, NULL,
			&app->swapchain_image_resources_L[i].framebuffer);
		assert(!err);
		err = vkCreateFramebuffer(app->device, &fb_info_R, NULL,
			&app->swapchain_image_resources_R[i].framebuffer);
		assert(!err);
	}
}


void VkApp::DrawBuildCmd(VkApp *app, int index)
{
	const VkCommandBufferBeginInfo cmd_buf_info
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	//.sType
		NULL,																				//.pNext
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,//.flags
		NULL,																				//.pInheritanceInfo 
	};
	const VkClearValue clear_values[2]
	{
		{ { 0.2f, 0.2f, 0.2f, 0.2f }, },
		{ { 1.0f, 0 } },
	};

	const VkRenderPassBeginInfo rp_begin_L
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,			//.sType 
		NULL,																				//.pNext 
		app->render_pass,															//.renderPass 
		app->swapchain_image_resources_L[app->current_buffer].framebuffer,	//.framebuffer 
		{ 0, 0, app->width,app->height },
		2,																						//.clearValueCount 
		clear_values																	//.pClearValues 
	};

	const VkRenderPassBeginInfo rp_begin_R
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,			//.sType 
		NULL,																				//.pNext 
		app->render_pass,															//.renderPass 
		app->swapchain_image_resources_R[app->current_buffer].framebuffer,	//.framebuffer 
		{ 0, 0, app->width,app->height },
		2,																						//.clearValueCount 
		clear_values																	//.pClearValues 
	};


	VkResult err;

	err = vkBeginCommandBuffer(app->swapchain_image_resources_L[index].cmd, &cmd_buf_info);
	assert(!err);

	vkCmdBeginRenderPass(app->swapchain_image_resources_L[index].cmd, &rp_begin_L, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(app->swapchain_image_resources_L[index].cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline);
	vkCmdBindDescriptorSets(app->swapchain_image_resources_L[index].cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		app->pipeline_layout, 0, 1,
		&app->swapchain_image_resources_L[app->current_buffer].descriptor_set,
		0, NULL);
	VkViewport viewport;
	memset(&viewport, 0, sizeof(viewport));
	viewport.height = (float)app->height;
	viewport.width = (float)app->width;
	viewport.minDepth = (float)0.0f;
	viewport.maxDepth = (float)1.0f;
	vkCmdSetViewport(app->swapchain_image_resources_L[index].cmd, 0, 1, &viewport);

	VkRect2D scissor;
	memset(&scissor, 0, sizeof(scissor));
	scissor.extent.width = app->width;
	scissor.extent.height = app->height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(app->swapchain_image_resources_L[index].cmd, 0, 1, &scissor);
	vkCmdDraw(app->swapchain_image_resources_L[index].cmd, 12 * 3, 1, 0, 0);
	// Note that ending the renderpass changes the image's layout from
	// COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
	vkCmdEndRenderPass(app->swapchain_image_resources_L[index].cmd);

	err = vkEndCommandBuffer(app->swapchain_image_resources_L[index].cmd);
	assert(!err);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	err = vkBeginCommandBuffer(app->swapchain_image_resources_R[index].cmd, &cmd_buf_info);
	assert(!err);

	vkCmdBeginRenderPass(app->swapchain_image_resources_R[index].cmd, &rp_begin_R, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(app->swapchain_image_resources_R[index].cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline);
	vkCmdBindDescriptorSets(app->swapchain_image_resources_R[index].cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		app->pipeline_layout, 0, 1,
		&app->swapchain_image_resources_R[app->current_buffer].descriptor_set,
		0, NULL);

	memset(&viewport, 0, sizeof(viewport));
	viewport.height = (float)app->height;
	viewport.width = (float)app->width;
	viewport.minDepth = (float)0.0f;
	viewport.maxDepth = (float)1.0f;
	vkCmdSetViewport(app->swapchain_image_resources_R[index].cmd, 0, 1, &viewport);

	memset(&scissor, 0, sizeof(scissor));
	scissor.extent.width = app->width;
	scissor.extent.height = app->height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(app->swapchain_image_resources_R[index].cmd, 0, 1, &scissor);
	vkCmdDraw(app->swapchain_image_resources_R[index].cmd, 12 * 3, 1, 0, 0);
	// Note that ending the renderpass changes the image's layout from
	// COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
	vkCmdEndRenderPass(app->swapchain_image_resources_R[index].cmd);

	err = vkEndCommandBuffer(app->swapchain_image_resources_R[index].cmd);
	assert(!err);

}

void VkApp::FlushInitiationCmd(VkApp *app) {
	VkResult err;

	// This function could get called twice if the texture uses a staging buffer
	// In that case the second call should be ignored
	if (app->cmd_L == VK_NULL_HANDLE)
		return;

	err = vkEndCommandBuffer(app->cmd_L);
	assert(!err);

	if (app->cmd_R == VK_NULL_HANDLE)
		return;

	err = vkEndCommandBuffer(app->cmd_R);
	assert(!err);


	VkFence fence;
	VkFenceCreateInfo fence_ci;
	fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_ci.pNext = NULL;
	fence_ci.flags = 0;
	err = vkCreateFence(app->device, &fence_ci, NULL, &fence);
	assert(!err);

	const VkCommandBuffer cmd_bufs_L[] = { app->cmd_L };
	const VkCommandBuffer cmd_bufs_R[] = { app->cmd_R };
	
	VkSubmitInfo submit_info_L;
	submit_info_L.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info_L.pNext = NULL;
	submit_info_L.waitSemaphoreCount = 0;
	submit_info_L.pWaitSemaphores = NULL;
	submit_info_L.pWaitDstStageMask = NULL;
	submit_info_L.commandBufferCount = 1;
	submit_info_L.pCommandBuffers = cmd_bufs_L;
	submit_info_L.signalSemaphoreCount = 0;
	submit_info_L.pSignalSemaphores = NULL;
	VkSubmitInfo submit_info_R;
	submit_info_R.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info_R.pNext = NULL;
	submit_info_R.waitSemaphoreCount = 0;
	submit_info_R.pWaitSemaphores = NULL;
	submit_info_R.pWaitDstStageMask = NULL;
	submit_info_R.commandBufferCount = 1;
	submit_info_R.pCommandBuffers = cmd_bufs_R;
	submit_info_R.signalSemaphoreCount = 0;



	err = vkQueueSubmit(app->left_graphics_queue, 1, &submit_info_L, fence);
	assert(!err);
	err = vkQueueSubmit(app->right_graphics_queue, 1, &submit_info_R, fence);
	assert(!err);
	err = vkWaitForFences(app->device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(!err);

	vkFreeCommandBuffers(app->device, app->cmd_pool_L, 1, cmd_bufs_L);
	vkFreeCommandBuffers(app->device, app->cmd_pool_R, 1, cmd_bufs_R);
	vkDestroyFence(app->device, fence, NULL);
	app->cmd_L = VK_NULL_HANDLE;
	app->cmd_R = VK_NULL_HANDLE;
}

void VkApp::DestroyTextureImage(VkApp *app, struct texture_object *tex_objs)
{
	/* clean up staging resources */
	vkFreeMemory(app->device, tex_objs->mem, NULL);
	vkDestroyImage(app->device, tex_objs->image, NULL);
}

void VkApp::Draw_L(VkApp* app)
{
	VkResult err;
	uint32_t leftbuf(0);

	// Ensure no more than FRAME_LAG presentations are outstanding
	vkWaitForFences(app->device, 1, &app->fences[app->frame_index_L], VK_TRUE, UINT64_MAX);
	vkResetFences(app->device, 1, &app->fences[app->frame_index_L]);

	// Get the index of the next available swapchain image:
	err = app->fpAcquireNextImageKHR(app->device, app->swapchain_L, UINT64_MAX,
		app->image_acquired_semaphores_L[app->frame_index_L],
		app->fences[app->frame_index_L],
		&leftbuf);
	assert(!err);

	app->current_buffer = leftbuf;

	UpdateDataBuffer(app);

	if (err == VK_ERROR_OUT_OF_DATE_KHR) {
		// app->swapchain is out of date (e.g. the window was resized) and
		// must be recreated:
		app->frame_index_L += 1;
		app->frame_index_L %= FRAME_LAG;

		Resize_L(app);
		Draw_L(app);
		return;
	}
	else if (err == VK_SUBOPTIMAL_KHR) {
		// app->swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
	}
	else {
		assert(!err);
	}
	//if (app->VK_GOOGLE_display_timing_enabled) {
	// Look at what happened to previous presents, and make appropriate
	// adjustments in timing:
	//UpdateTargetIPD(app);

	// Note: a real application would position its geometry to that it's in
	// the correct location for when the next image is presented.  It might
	// also wait, so that there's less latency between any input and when
	// the next image is rendered/presented.  This app program is so
	// simple that it doesn't do either of those.
	//}

	// Wait for the image acquired semaphore to be signaled to ensure
	// that the image won't be rendered to until the presentation
	// engine has fully released ownership to the application, and it is
	// okay to render to the image.


	VkPipelineStageFlags pipe_stage_flags;
	VkSubmitInfo submit_info_L;
	submit_info_L.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info_L.pNext = NULL;
	submit_info_L.pWaitDstStageMask = &pipe_stage_flags;
	pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info_L.waitSemaphoreCount = 1;
	submit_info_L.pWaitSemaphores = &app->image_acquired_semaphores_L[app->frame_index_L];
	submit_info_L.commandBufferCount = 1;
	submit_info_L.pCommandBuffers = &app->swapchain_image_resources_L[app->current_buffer].cmd;
	submit_info_L.signalSemaphoreCount = 1;
	submit_info_L.pSignalSemaphores = &app->draw_complete_semaphores_L[app->frame_index_L];
	vkResetFences(app->device, 1, &app->swapchain_image_resources_L[app->current_buffer].fence);
	err = vkQueueSubmit(app->left_graphics_queue, 1, &submit_info_L,
		app->swapchain_image_resources_L[app->current_buffer].fence);
	assert(!err);


	VkResult result_L;

	// If we are using separate queues we have to wait for image ownership,
	// otherwise wait for draw complete
	VkPresentInfoKHR present_L;

	present_L.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_L.pNext = NULL;
	present_L.waitSemaphoreCount = 1;
	present_L.pWaitSemaphores = &app->draw_complete_semaphores_L[app->frame_index_L];
	present_L.swapchainCount = 1;
	present_L.pSwapchains = &app->swapchain_L,
	present_L.pImageIndices = &app->current_buffer;
	present_L.pResults = &result_L;

	if (app->VK_KHR_incremental_present_enabled)
	{
		// If using VK_KHR_incremental_present, we provide a hint of the region
		// that contains changed content relative to the previously-presented
		// image.  The implementation can use this hint in order to save
		// work/power (by only copying the region in the hint).  The
		// implementation is free to ignore the hint though, and so we must
		// ensure that the entire image has the correctly-drawn content.
		uint32_t eighthOfWidth = app->width / 8;
		uint32_t eighthOfHeight = app->height / 8;
		VkRectLayerKHR rect;
		rect.offset.x = eighthOfWidth;
		rect.offset.y = eighthOfHeight;
		rect.extent.width = eighthOfWidth * 6;
		rect.extent.height = eighthOfHeight * 6;
		rect.layer = 0;

		VkPresentRegionKHR region;
		region.rectangleCount = 1;
		region.pRectangles = &rect;

		VkPresentRegionsKHR regions_L;
		regions_L.sType = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
		regions_L.pNext = present_L.pNext;
		regions_L.swapchainCount = present_L.swapchainCount;
		regions_L.pRegions = &region;


		present_L.pNext = &regions_L;

	}

	/*if (app->VK_GOOGLE_display_timing_enabled) {
	VkPresentTimeGOOGLE ptime;
	if (app->prev_desired_present_time == 0) {
	// This must be the first present for this swapchain.
	//
	// We don't know where we are relative to the presentation engine's
	// display's refresh cycle.  We also don't know how long rendering
	// takes.  Let's make a grossly-simplified assumption that the
	// desiredPresentTime should be half way between now and
	// now+target_IPD.  We will adjust over time.
	uint64_t curtime = getTimeInNanoseconds();
	if (curtime == 0) {
	// Since we didn't find out the current time, don't give a
	// desiredPresentTime:
	ptime.desiredPresentTime = 0;
	}
	else {
	ptime.desiredPresentTime = curtime + (app->target_IPD >> 1);
	}
	}
	else {
	ptime.desiredPresentTime = (app->prev_desired_present_time +
	app->target_IPD);
	}
	ptime.presentID = app->next_present_id++;
	app->prev_desired_present_time = ptime.desiredPresentTime;

	VkPresentTimesInfoGOOGLE present_time = {
	.sType = VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE,
	.pNext = present.pNext,
	.swapchainCount = present.swapchainCount,
	.pTimes = &ptime,
	};
	if (app->VK_GOOGLE_display_timing_enabled) {
	present.pNext = &present_time;
	}
	}
	*/
	err = app->fpQueuePresentKHR(app->left_graphics_queue, &present_L);

	app->frame_index_L += 1;
	app->frame_index_L %= FRAME_LAG;

	if (err == VK_ERROR_OUT_OF_DATE_KHR) {
		// app->swapchain is out of date (e.g. the window was resized) and
		// must be recreated:
		Resize_L(app);
	}
	else if (err == VK_SUBOPTIMAL_KHR) {
		// app->swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
	}
	else {
		assert(!err);
	}
}

void VkApp::Draw_R(VkApp* app)
{
	VkResult err;

	uint32_t rightbuf(0);
	// Ensure no more than FRAME_LAG presentations are outstanding
	vkWaitForFences(app->device, 1, &app->fences[app->frame_index_R], VK_TRUE, UINT64_MAX);
	vkResetFences(app->device, 1, &app->fences[app->frame_index_R]);

	// Get the index of the next available swapchain image:

	err = app->fpAcquireNextImageKHR(app->device, app->swapchain_R, UINT64_MAX,
		app->image_acquired_semaphores_R[app->frame_index_R],
		app->fences[app->frame_index_R],
		&rightbuf);
	assert(!err);

	app->current_buffer = rightbuf;

	UpdateDataBuffer(app);

	if (err == VK_ERROR_OUT_OF_DATE_KHR) {
		// app->swapchain is out of date (e.g. the window was resized) and
		// must be recreated:
		app->frame_index_R += 1;
		app->frame_index_R %= FRAME_LAG;

		Resize_R(app);
		Draw_R(app);
		return;
	}
	else if (err == VK_SUBOPTIMAL_KHR) {
		// app->swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
	}
	else {
		assert(!err);
	}
	//if (app->VK_GOOGLE_display_timing_enabled) {
	// Look at what happened to previous presents, and make appropriate
	// adjustments in timing:
	//UpdateTargetIPD(app);

	// Note: a real application would position its geometry to that it's in
	// the correct location for when the next image is presented.  It might
	// also wait, so that there's less latency between any input and when
	// the next image is rendered/presented.  This app program is so
	// simple that it doesn't do either of those.
	//}

	// Wait for the image acquired semaphore to be signaled to ensure
	// that the image won't be rendered to until the presentation
	// engine has fully released ownership to the application, and it is
	// okay to render to the image.


	VkPipelineStageFlags pipe_stage_flags;
	VkSubmitInfo submit_info_R;
	submit_info_R.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info_R.pNext = NULL;
	submit_info_R.pWaitDstStageMask = &pipe_stage_flags;
	pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info_R.waitSemaphoreCount = 1;
	submit_info_R.pWaitSemaphores = &app->image_acquired_semaphores_R[app->frame_index_R];
	submit_info_R.commandBufferCount = 1;
	submit_info_R.pCommandBuffers = &app->swapchain_image_resources_R[app->current_buffer].cmd;
	submit_info_R.signalSemaphoreCount = 1;
	submit_info_R.pSignalSemaphores = &app->draw_complete_semaphores_R[app->frame_index_R];
	vkResetFences(app->device, 1, &app->swapchain_image_resources_R[app->current_buffer].fence);
	err = vkQueueSubmit(app->right_graphics_queue, 1, &submit_info_R,
		app->swapchain_image_resources_R[app->current_buffer].fence);
	assert(!err);

	VkResult result_R;

	// If we are using separate queues we have to wait for image ownership,
	// otherwise wait for draw complete
	VkPresentInfoKHR present_R;

	present_R.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_R.pNext = NULL;
	present_R.waitSemaphoreCount = 1;
	present_R.pWaitSemaphores = &app->draw_complete_semaphores_R[app->frame_index_R];
	present_R.swapchainCount = 1;
	present_R.pSwapchains = &app->swapchain_R,
		present_R.pImageIndices = &app->current_buffer;
	present_R.pResults = &result_R;

	if (app->VK_KHR_incremental_present_enabled)
	{
		// If using VK_KHR_incremental_present, we provide a hint of the region
		// that contains changed content relative to the previously-presented
		// image.  The implementation can use this hint in order to save
		// work/power (by only copying the region in the hint).  The
		// implementation is free to ignore the hint though, and so we must
		// ensure that the entire image has the correctly-drawn content.
		uint32_t eighthOfWidth = app->width / 8;
		uint32_t eighthOfHeight = app->height / 8;
		VkRectLayerKHR rect;
		rect.offset.x = eighthOfWidth;
		rect.offset.y = eighthOfHeight;
		rect.extent.width = eighthOfWidth * 6;
		rect.extent.height = eighthOfHeight * 6;
		rect.layer = 0;

		VkPresentRegionKHR region;
		region.rectangleCount = 1;
		region.pRectangles = &rect;

		VkPresentRegionsKHR regions_R;

		regions_R.sType = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
		regions_R.pNext = present_R.pNext;
		regions_R.swapchainCount = present_R.swapchainCount;
		regions_R.pRegions = &region;

		present_R.pNext = &regions_R;
	}

	/*if (app->VK_GOOGLE_display_timing_enabled) {
	VkPresentTimeGOOGLE ptime;
	if (app->prev_desired_present_time == 0) {
	// This must be the first present for this swapchain.
	//
	// We don't know where we are relative to the presentation engine's
	// display's refresh cycle.  We also don't know how long rendering
	// takes.  Let's make a grossly-simplified assumption that the
	// desiredPresentTime should be half way between now and
	// now+target_IPD.  We will adjust over time.
	uint64_t curtime = getTimeInNanoseconds();
	if (curtime == 0) {
	// Since we didn't find out the current time, don't give a
	// desiredPresentTime:
	ptime.desiredPresentTime = 0;
	}
	else {
	ptime.desiredPresentTime = curtime + (app->target_IPD >> 1);
	}
	}
	else {
	ptime.desiredPresentTime = (app->prev_desired_present_time +
	app->target_IPD);
	}
	ptime.presentID = app->next_present_id++;
	app->prev_desired_present_time = ptime.desiredPresentTime;

	VkPresentTimesInfoGOOGLE present_time = {
	.sType = VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE,
	.pNext = present.pNext,
	.swapchainCount = present.swapchainCount,
	.pTimes = &ptime,
	};
	if (app->VK_GOOGLE_display_timing_enabled) {
	present.pNext = &present_time;
	}
	}
	*/

	err = app->fpQueuePresentKHR(app->right_graphics_queue, &present_R);

	app->frame_index_R += 1;
	app->frame_index_R %= FRAME_LAG;

	if (err == VK_ERROR_OUT_OF_DATE_KHR) {
		// app->swapchain is out of date (e.g. the window was resized) and
		// must be recreated:
		Resize_R(app);
	}
	else if (err == VK_SUBOPTIMAL_KHR) {
		// app->swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
	}
	else {
		assert(!err);
	}
}

void VkApp::UpdateDataBuffer(VkApp * app)
{
	mat4x4 MVP_L, MVP_R, Model, VP_L, VP_R;
	int matrixSize_L = sizeof(MVP_L);
	int matrixSize_R = sizeof(MVP_R);
	uint8_t *pData_L, *pData_R;
	VkResult err;

	mat4x4_mul(VP_L, app->projection_matrix, app->view_matrix_l);
	mat4x4_mul(VP_R, app->projection_matrix, app->view_matrix_r);

	// Rotate around the Y axis
	mat4x4_dup(Model, app->model_matrix);
	mat4x4_rotate(app->model_matrix, Model, 0.0f, 1.0f, 0.0f,
		(float)degreesToRadians(app->spin_angle));
	mat4x4_mul(MVP_L, VP_L, app->model_matrix);
	mat4x4_mul(MVP_R, VP_R, app->model_matrix);

	vkWaitForFences(app->device, 1, &app->swapchain_image_resources_L[app->current_buffer].fence,
		VK_TRUE, UINT64_MAX);
	err = vkMapMemory(app->device,
		app->swapchain_image_resources_L[app->current_buffer].uniform_memory, 0,
		VK_WHOLE_SIZE, 0, (void **)&pData_L);
	assert(!err);

	memcpy(pData_L, (const void *)&MVP_L[0][0], matrixSize_L);
	vkUnmapMemory(app->device, app->swapchain_image_resources_L[app->current_buffer].uniform_memory);

	vkWaitForFences(app->device, 1, &app->swapchain_image_resources_R[app->current_buffer].fence,
		VK_TRUE, UINT64_MAX);

	err = vkMapMemory(app->device,
		app->swapchain_image_resources_R[app->current_buffer].uniform_memory, 0,
		VK_WHOLE_SIZE, 0, (void **)&pData_R);
	assert(!err);

	memcpy(pData_R, (const void *)&MVP_R[0][0], matrixSize_R);
	vkUnmapMemory(app->device, app->swapchain_image_resources_R[app->current_buffer].uniform_memory);
}

void VkApp::Resize_L(VkApp * app)
{
	uint32_t i;

	// Don't react to resize until after first initialization.
	if (!app->prepared) {
		return;
	}
	// In order to properly resize the window, we must re-create the swapchain
	// AND redo the command buffers, etc.
	//
	// First, perform part of the app_cleanup() function:
	app->prepared = false;
	vkDeviceWaitIdle(app->device);

	for (i = 0; i < app->swapchainImageCount; i++) {
		vkDestroyFramebuffer(app->device, app->swapchain_image_resources_L[i].framebuffer, NULL);
		vkDestroyFramebuffer(app->device, app->swapchain_image_resources_R[i].framebuffer, NULL);
	}
	vkDestroyDescriptorPool(app->device, app->desc_pool_L, NULL);
	vkDestroyDescriptorPool(app->device, app->desc_pool_R, NULL);
	vkDestroyPipeline(app->device, app->pipeline, NULL);
	vkDestroyPipelineCache(app->device, app->pipelineCache, NULL);
	vkDestroyRenderPass(app->device, app->render_pass, NULL);
	vkDestroyPipelineLayout(app->device, app->pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(app->device, app->desc_layout, NULL);

	for (i = 0; i < DEMO_TEXTURE_COUNT; i++)
	{
		vkDestroyImageView(app->device, app->textures[i].view, NULL);
		vkDestroyImage(app->device, app->textures[i].image, NULL);
		vkFreeMemory(app->device, app->textures[i].mem, NULL);
		vkDestroySampler(app->device, app->textures[i].sampler, NULL);
	}

	vkDestroyImageView(app->device, app->depth_L.view, NULL);
	vkDestroyImageView(app->device, app->depth_R.view, NULL);
	vkDestroyImage(app->device, app->depth_L.image, NULL);
	vkDestroyImage(app->device, app->depth_R.image, NULL);
	vkFreeMemory(app->device, app->depth_L.mem, NULL);
	vkFreeMemory(app->device, app->depth_R.mem, NULL);

	for (i = 0; i < app->swapchainImageCount; i++) {
		vkDestroyImageView(app->device, app->swapchain_image_resources_L[i].view, NULL);
		vkFreeCommandBuffers(app->device, app->cmd_pool_L, 1,
			&app->swapchain_image_resources_L[i].cmd);
		vkDestroyBuffer(app->device, app->swapchain_image_resources_L[i].uniform_buffer, NULL);
		vkFreeMemory(app->device, app->swapchain_image_resources_L[i].uniform_memory, NULL);
		vkDestroyFence(app->device, app->swapchain_image_resources_L[i].fence, NULL);

		vkDestroyImageView(app->device, app->swapchain_image_resources_R[i].view, NULL);
		vkFreeCommandBuffers(app->device, app->cmd_pool_R, 1,
			&app->swapchain_image_resources_R[i].cmd);
		vkDestroyBuffer(app->device, app->swapchain_image_resources_R[i].uniform_buffer, NULL);
		vkFreeMemory(app->device, app->swapchain_image_resources_R[i].uniform_memory, NULL);
		vkDestroyFence(app->device, app->swapchain_image_resources_R[i].fence, NULL);
	}
	vkDestroyCommandPool(app->device, app->cmd_pool_L, NULL);
	vkDestroyCommandPool(app->device, app->cmd_pool_R, NULL);

	free(app->swapchain_image_resources_L);
	free(app->swapchain_image_resources_R);
	// Second, re-perform the app_prepare() function, which will re-create the
	// swapchain:
	RECT wr = { 0, 0, app->width, app->height };
	RECT _right  {};
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	GetWindowRect(app->window_r, &_right);
	MoveWindow(app->window_r, _right.left , _right.top, wr.right - wr.left, wr.bottom - wr.top, false);
	PrepareStream(app);
}

void VkApp::Resize_R(VkApp * app)
{
	uint32_t i;

	// Don't react to resize until after first initialization.
	if (!app->prepared) {
		return;
	}
	// In order to properly resize the window, we must re-create the swapchain
	// AND redo the command buffers, etc.
	//
	// First, perform part of the app_cleanup() function:
	app->prepared = false;
	vkDeviceWaitIdle(app->device);

	for (i = 0; i < app->swapchainImageCount; i++) {
		vkDestroyFramebuffer(app->device, app->swapchain_image_resources_L[i].framebuffer, NULL);
		vkDestroyFramebuffer(app->device, app->swapchain_image_resources_R[i].framebuffer, NULL);
	}
	vkDestroyDescriptorPool(app->device, app->desc_pool_L, NULL);
	vkDestroyDescriptorPool(app->device, app->desc_pool_R, NULL);
	vkDestroyPipeline(app->device, app->pipeline, NULL);
	vkDestroyPipelineCache(app->device, app->pipelineCache, NULL);
	vkDestroyRenderPass(app->device, app->render_pass, NULL);
	vkDestroyPipelineLayout(app->device, app->pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(app->device, app->desc_layout, NULL);

	for (i = 0; i < DEMO_TEXTURE_COUNT; i++)
	{
		vkDestroyImageView(app->device, app->textures[i].view, NULL);
		vkDestroyImage(app->device, app->textures[i].image, NULL);
		vkFreeMemory(app->device, app->textures[i].mem, NULL);
		vkDestroySampler(app->device, app->textures[i].sampler, NULL);
	}

	vkDestroyImageView(app->device, app->depth_L.view, NULL);
	vkDestroyImageView(app->device, app->depth_R.view, NULL);
	vkDestroyImage(app->device, app->depth_L.image, NULL);
	vkDestroyImage(app->device, app->depth_R.image, NULL);
	vkFreeMemory(app->device, app->depth_L.mem, NULL);
	vkFreeMemory(app->device, app->depth_R.mem, NULL);

	for (i = 0; i < app->swapchainImageCount; i++) {
		vkDestroyImageView(app->device, app->swapchain_image_resources_L[i].view, NULL);
		vkFreeCommandBuffers(app->device, app->cmd_pool_L, 1,
			&app->swapchain_image_resources_L[i].cmd);
		vkDestroyBuffer(app->device, app->swapchain_image_resources_L[i].uniform_buffer, NULL);
		vkFreeMemory(app->device, app->swapchain_image_resources_L[i].uniform_memory, NULL);
		vkDestroyFence(app->device, app->swapchain_image_resources_L[i].fence, NULL);

		vkDestroyImageView(app->device, app->swapchain_image_resources_R[i].view, NULL);
		vkFreeCommandBuffers(app->device, app->cmd_pool_R, 1,
			&app->swapchain_image_resources_R[i].cmd);
		vkDestroyBuffer(app->device, app->swapchain_image_resources_R[i].uniform_buffer, NULL);
		vkFreeMemory(app->device, app->swapchain_image_resources_R[i].uniform_memory, NULL);
		vkDestroyFence(app->device, app->swapchain_image_resources_R[i].fence, NULL);
	}
	vkDestroyCommandPool(app->device, app->cmd_pool_L, NULL);
	vkDestroyCommandPool(app->device, app->cmd_pool_R, NULL);

	free(app->swapchain_image_resources_L);
	free(app->swapchain_image_resources_R);
	// Second, re-perform the app_prepare() function, which will re-create the
	// swapchain:

	RECT wr = { 0, 0, app->width, app->height };
	RECT _left {};
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	GetWindowRect(app->window_l, &_left);
	MoveWindow(app->window_l, _left.left, _left.top, wr.right - wr.left, wr.bottom - wr.top, false);
	PrepareStream(app);
}



void VkApp::CleanUp(VkApp *app)
{
	uint32_t i;

	app->prepared = false;
	vkDeviceWaitIdle(app->device);

	// Wait for fences from present operations
	for (i = 0; i < FRAME_LAG; i++)
	{
		vkWaitForFences(app->device, 1, &app->fences[i], VK_TRUE, UINT64_MAX);
		vkDestroyFence(app->device, app->fences[i], NULL);
		vkDestroySemaphore(app->device, app->image_acquired_semaphores_L[i], NULL);
		vkDestroySemaphore(app->device, app->image_acquired_semaphores_R[i], NULL);
		vkDestroySemaphore(app->device, app->draw_complete_semaphores_L[i], NULL);
		vkDestroySemaphore(app->device, app->draw_complete_semaphores_R[i], NULL);
	}

	for (i = 0; i < app->swapchainImageCount; i++)
	{
		vkDestroyFramebuffer(app->device, app->swapchain_image_resources_L[i].framebuffer, NULL);
		vkDestroyFramebuffer(app->device, app->swapchain_image_resources_R[i].framebuffer, NULL);
	}

	vkDestroyDescriptorPool(app->device, app->desc_pool_L, NULL);
	vkDestroyDescriptorPool(app->device, app->desc_pool_R, NULL);

	vkDestroyPipeline(app->device, app->pipeline, NULL);
	vkDestroyPipelineCache(app->device, app->pipelineCache, NULL);
	vkDestroyRenderPass(app->device, app->render_pass, NULL);
	vkDestroyPipelineLayout(app->device, app->pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(app->device, app->desc_layout, NULL);

	for (i = 0; i < DEMO_TEXTURE_COUNT; i++)
	{
		vkDestroyImageView(app->device, app->textures[i].view, NULL);
		vkDestroyImage(app->device, app->textures[i].image, NULL);
		vkFreeMemory(app->device, app->textures[i].mem, NULL);
		vkDestroySampler(app->device, app->textures[i].sampler, NULL);
	}
	app->fpDestroySwapchainKHR(app->device, app->swapchain_L, NULL);
	app->fpDestroySwapchainKHR(app->device, app->swapchain_R, NULL);

	vkDestroyImageView(app->device, app->depth_L.view, NULL);
	vkDestroyImage(app->device, app->depth_L.image, NULL);
	vkFreeMemory(app->device, app->depth_L.mem, NULL);

	vkDestroyImageView(app->device, app->depth_R.view, NULL);
	vkDestroyImage(app->device, app->depth_R.image, NULL);
	vkFreeMemory(app->device, app->depth_R.mem, NULL);

	for (i = 0; i < app->swapchainImageCount; i++)
	{
		vkDestroyImageView(app->device, app->swapchain_image_resources_L[i].view, NULL);
		vkFreeCommandBuffers(app->device, app->cmd_pool_L, 1,
			&app->swapchain_image_resources_L[i].cmd);
		vkDestroyBuffer(app->device, app->swapchain_image_resources_L[i].uniform_buffer, NULL);
		vkFreeMemory(app->device, app->swapchain_image_resources_L[i].uniform_memory, NULL);
		vkDestroyFence(app->device, app->swapchain_image_resources_L[i].fence, NULL);

		vkDestroyImageView(app->device, app->swapchain_image_resources_R[i].view, NULL);
		vkFreeCommandBuffers(app->device, app->cmd_pool_R, 1,
			&app->swapchain_image_resources_R[i].cmd);
		vkDestroyBuffer(app->device, app->swapchain_image_resources_R[i].uniform_buffer, NULL);
		vkFreeMemory(app->device, app->swapchain_image_resources_R[i].uniform_memory, NULL);
		vkDestroyFence(app->device, app->swapchain_image_resources_R[i].fence, NULL);

	}

	free(app->swapchain_image_resources_L);
	free(app->swapchain_image_resources_R);
	free(app->queue_props);
	vkDestroyCommandPool(app->device, app->cmd_pool_L, NULL);
	vkDestroyCommandPool(app->device, app->cmd_pool_R, NULL);

	vkDeviceWaitIdle(app->device);
	vkDestroyDevice(app->device, NULL);
	if (app->validate) {
		app->DestroyDebugReportCallback(app->inst, app->msg_callback, NULL);
	}
	vkDestroySurfaceKHR(app->inst, app->surface_L, NULL);
	vkDestroySurfaceKHR(app->inst, app->surface_R, NULL);

	vkDestroyInstance(app->inst, NULL);
}

/*
void VkApp::UpdateTargetIPD(VkApp *demo)
{
// Look at what happened to previous presents, and make appropriate
// adjustments in timing:
VkResult err;
VkPastPresentationTimingGOOGLE* past = NULL;
uint32_t count_L(0), count_R(0);

err = demo->fpGetPastPresentationTimingGOOGLE(demo->device,
demo->swapchain_L,
&count_L,
NULL);
assert(!err);

err = demo->fpGetPastPresentationTimingGOOGLE(demo->device,
demo->swapchain_L,
&count_R,
NULL);
assert(!err);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
if (count_L) {
past = (VkPastPresentationTimingGOOGLE*)malloc(sizeof(VkPastPresentationTimingGOOGLE) * count_L);
assert(past);
err = demo->fpGetPastPresentationTimingGOOGLE(demo->device,
demo->swapchain_L,
&count_L,
past);
assert(!err);

bool early = false;
bool late = false;
bool calibrate_next = false;
for (uint32_t i = 0; i < count_L; i++) {
if (!demo->syncd_with_actual_presents) {
// This is the first time that we've received an
// actualPresentTime for this swapchain.  In order to not
// perceive these early frames as "late", we need to sync-up
// our future desiredPresentTime's with the
// actualPresentTime(s) that we're receiving now.
calibrate_next = true;

// So that we don't suspect any pending presents as late,
// record them all as suspected-late presents:
demo->last_late_id = demo->next_present_id - 1;
demo->last_early_id = 0;
demo->syncd_with_actual_presents = true;
break;
}
else if (CanPresentEarlier(past[i].earliestPresentTime,
past[i].actualPresentTime,
past[i].presentMargin,
demo->refresh_duration)) {
// This image could have been presented earlier.  We don't want
// to decrease the target_IPD until we've seen early presents
// for at least two seconds.
if (demo->last_early_id == past[i].presentID) {
// We've now seen two seconds worth of early presents.
// Flag it as such, and reset the counter:
early = true;
demo->last_early_id = 0;
}
else if (demo->last_early_id == 0) {
// This is the first early present we've seen.
// Calculate the presentID for two seconds from now.
uint64_t lastEarlyTime =
past[i].actualPresentTime + (2 * BILLION);
uint32_t howManyPresents =
(uint32_t)((lastEarlyTime - past[i].actualPresentTime) / demo->target_IPD);
demo->last_early_id = past[i].presentID + howManyPresents;
}
else {
// We are in the midst of a set of early images,
// and so we won't do anything.
}
late = false;
demo->last_late_id = 0;
}
else if (ActualTimeLate(past[i].desiredPresentTime,
past[i].actualPresentTime,
demo->refresh_duration)) {
// This image was presented after its desired time.  Since
// there's a delay between calling vkQueuePresentKHR and when
// we get the timing data, several presents may have been late.
// Thus, we need to threat all of the outstanding presents as
// being likely late, so that we only increase the target_IPD
// once for all of those presents.
if ((demo->last_late_id == 0) ||
(demo->last_late_id < past[i].presentID)) {
late = true;
// Record the last suspected-late present:
demo->last_late_id = demo->next_present_id - 1;
}
else {
// We are in the midst of a set of likely-late images,
// and so we won't do anything.
}
early = false;
demo->last_early_id = 0;
}
else {
// Since this image was not presented early or late, reset
// any sets of early or late presentIDs:
early = false;
late = false;
calibrate_next = true;
demo->last_early_id = 0;
demo->last_late_id = 0;
}
}

if (early) {
// Since we've seen at least two-seconds worth of presnts that
// could have occured earlier than desired, let's decrease the
// target_IPD (i.e. increase the frame rate):
//
// TODO(ianelliott): Try to calculate a better target_IPD based
// on the most recently-seen present (this is overly-simplistic).
demo->refresh_duration_multiplier--;
if (demo->refresh_duration_multiplier == 0) {
// This should never happen, but in case it does, don't
// try to go faster.
demo->refresh_duration_multiplier = 1;
}
demo->target_IPD =
demo->refresh_duration * demo->refresh_duration_multiplier;
}
if (late) {
// Since we found a new instance of a late present, we want to
// increase the target_IPD (i.e. decrease the frame rate):
//
// TODO(ianelliott): Try to calculate a better target_IPD based
// on the most recently-seen present (this is overly-simplistic).
demo->refresh_duration_multiplier++;
demo->target_IPD =
demo->refresh_duration * demo->refresh_duration_multiplier;
}

if (calibrate_next) {
int64_t multiple = demo->next_present_id - past[count - 1].presentID;
demo->prev_desired_present_time =
(past[count - 1].actualPresentTime +
(multiple * demo->target_IPD));
}
}
}
*/