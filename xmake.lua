
add_rules("mode.debug", "mode.release")

-- Set C++23 standard  
set_languages("c++23")

-- Add compiler-specific flags for better C++23 support and platform detection
if is_plat("linux") then
    add_cxxflags("-fconcepts-diagnostics-depth=5") -- Better concept error messages
    add_cxxflags("-fcoroutines") -- Enable coroutines
    -- Platform-specific architecture detection
    if is_arch("x86_64") then
        add_defines("__x86_64__")
    elseif is_arch("arm64", "aarch64") then
        add_defines("__aarch64__")
    end
elseif is_plat("macosx") then
    -- macOS architecture detection
    if is_arch("x86_64") then
        add_defines("__x86_64__")  
    elseif is_arch("arm64", "aarch64") then
        add_defines("__aarch64__")
    end
end

target("MiJIT")
    set_kind("binary")
    add_files("main.cpp")
    set_warnings("all", "extra")
    add_cxxflags("-pedantic")
    
    -- Ensure modern C++ features are available (with fallback)
    if is_plat("linux", "macosx") then
        -- Try C++23 first, fallback to C++20, then C++17
        add_cxxflags("-std=c++23", "-std=c++20", "-std=c++17", {force = false})
    end
    
    -- Link required libraries
    if is_plat("linux") then
        add_syslinks("pthread") -- For threading support
    end
