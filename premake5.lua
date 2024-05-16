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
	kind "StaticLib"
	language "C"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	--location "QUIN/vendor/GLFW"

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
		"QUIN/vendor/GLFW/src/window.c"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

		files
		{
			"QUIN/vendor/GLFW/src/win32_init.c",
			"QUIN/vendor/GLFW/src/win32_joystick.c",
			"QUIN/vendor/GLFW/src/win32_monitor.c",
			"QUIN/vendor/GLFW/src/win32_time.c",
			"QUIN/vendor/GLFW/src/win32_thread.c",
			"QUIN/vendor/GLFW/src/win32_window.c",
			"QUIN/vendor/GLFW/src/wgl_context.c",
			"QUIN/vendor/GLFW/src/egl_context.c",
			"QUIN/vendor/GLFW/src/osmesa_context.c"
		}

		defines 
		{ 
			"_GLFW_WIN32",
			"_CRT_SECURE_NO_WARNINGS"
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
	pchsource "Quin/src/qnpch.h" -- will be ignored on other build systems

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/vendor/GLFW/include"
	}

	links
	{
		"GLFW",
		-- will need to link vulkan later
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
		defines "QN_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "QN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "QN_DIST"
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
		"%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"QUIN/vendor/spdlog/include",
		"QUIN/src"
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

	filter "configurations:Debug"
		defines "QN_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "QN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "QN_DIST"
		optimize "On"