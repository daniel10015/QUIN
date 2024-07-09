workspace "QUIN"
	architecture "x64"
configurations
{
	"Debug",
	"Release",
	"Dist"
}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


project "GLFW"
	location "QUIN/vendor/GLFW"
	kind "StaticLib"
	language "C"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"QUIN/vendor/GLFW/include/GLFW/glfw3.h",
		"QUIN/vendor/GLFW/include/GLFW/glfw3native.h",
		"QUIN/vendor/GLFW/src/glfw_config.h",
		"QUIN/vendor/GLFW/src/context.c",
		"QUIN/vendor/GLFW/src/init.c",
		"QUIN/vendor/GLFW/src/input.c",
		"QUIN/vendor/GLFW/src/monitor.c",
		"QUIN/vendor/GLFW/src/vulkan.c",
		"QUIN/vendor/GLFW/src/platform.c",
		"QUIN/vendor/GLFW/src/null_init.c",
		"QUIN/vendor/GLFW/src/null_monitor.c",
		"QUIN/vendor/GLFW/src/null_window.c",
		"QUIN/vendor/GLFW/src/null_joystick.c",
		"QUIN/vendor/GLFW/src/window.c"
	}

	libdirs { "/VulkanSDK/1.3.280.0/Lib/" }

	links
	{
		"vulkan-1" -- will need to link vulkan later
	}

	filter "system:windows"
		buildoptions("-std=c11", "ldgi32")
		systemversion "latest"
		staticruntime "On"

		files
		{
			"QUIN/vendor/GLFW/src/win32_init.c",
			"QUIN/vendor/GLFW/src/win32_module.c",
			"QUIN/vendor/GLFW/src/win32_joystick.c",
			"QUIN/vendor/GLFW/src/win32_monitor.c",
			"QUIN/vendor/GLFW/src/win32_time.c",
			"QUIN/vendor/GLFW/src/win32_thread.c",
			"QUIN/vendor/GLFW/src/win32_window.c",
			"QUIN/vendor/GLFW/src/win32_platform.h",
			"QUIN/vendor/GLFW/src/wgl_context.c",
			"QUIN/vendor/GLFW/src/egl_context.c",
			"QUIN/vendor/GLFW/src/osmesa_context.c"
		}

		defines 
		{ 
			"_GLFW_WIN32",
			"_CRT_SECURE_NO_WARNINGS",
			"_GLFW_VULKAN_STATIC"
		}

	filter "configurations:Debug"
		staticruntime "On"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		staticruntime "On"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		staticruntime "On"
		runtime "Release"
		optimize "on"



project "QUIN"
	location "QUIN" -- set path
	kind "SharedLib" -- specify it's a dll 
	language "C++" 

	targetdir ( "bin/" .. outputdir .. "/%{prj.name}" )
	objdir ( "bin-int/" .. outputdir .. "/%{prj.name}" )

	pchheader "qnpch.h"
	pchsource "src/Quin/qnpch.cpp" -- will be ignored on other build systems

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/src/**.frag",
		"%{prj.name}/src/**.vert"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/src/Quin",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/vendor/GLFW/include",
		"%{prj.name}/vendor/stb",
		"%{prj.name}/vendor/json/include",
		"/VulkanSDK/1.3.280.0/Include"
	}

	libdirs { "/VulkanSDK/1.3.280.0/Lib/" }

	links
	{
		"GLFW",
		"vulkan-1" -- will need to link vulkan later
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		
		defines
		{
			"QN_PLATFORM_WINDOWS",
			"QN_BUILD_DLL"
		}
	
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox/")
		}

	filter "configurations:Debug"
		defines "QN_ENABLE_ASSERTS" -- assert only in debug mode
		defines "QN_DEBUG"
		buildoptions "/MDd"
		symbols "On"

	filter "configurations:Release"
		defines "QN_RELEASE"
		buildoptions "/MD"
		optimize "On"

	filter "configurations:Dist"
		defines "QN_DIST"
		buildoptions "/MD"
		optimize "On"

project "Sandbox"
	location "Sandbox" 
	kind "ConsoleApp" 
	language "C++" 
	targetdir ( "bin/" .. outputdir .. "/%{prj.name}" )
	objdir ( "bin-int/" .. outputdir .. "/%{prj.name}" )

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/src/**.frag",
		"%{prj.name}/src/**.vert"
	}
	
	includedirs
	{
		"QUIN/vendor/spdlog/include",
		"QUIN/vendor/json/include",
		"QUIN/src",
		"/VulkanSDK/1.3.280.0/Include"
	}

	links
	{
		"QUIN"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		
		defines
		{
			"QN_PLATFORM_WINDOWS"
		}

		-- shader programs
		buildmessage 'Compiling Shader Programs'
		prebuildcommands 
		{
			"call CompileShaders.bat"
		}

	filter "configurations:Debug"
		defines "QN_DEBUG"
		buildoptions "/MDd"
		symbols "On"

	filter "configurations:Release"
		defines "QN_RELEASE"
		buildoptions "/MD"
		optimize "On"

	filter "configurations:Dist"
		defines "QN_DIST"
		buildoptions "/MD"
		optimize "On"