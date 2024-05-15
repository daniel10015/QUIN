workspace "QUIN"
	architecture "x64"
configurations
{
	"Debug",
	"Release",
	"Dist"
}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

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
		"%{prj.name}/vendor/spdlog/include"
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