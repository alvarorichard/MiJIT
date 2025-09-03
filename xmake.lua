
add_rules("mode.debug", "mode.release")

-- Set C++23 standard
set_languages("c++23")

-- Add compiler-specific flags for better C++23 support
if is_plat("linux") then
    add_cxxflags("-fconcepts-diagnostics-depth=5") -- Better concept error messages
    add_cxxflags("-fcoroutines") -- Enable coroutines
    add_cxxflags("-fmodules-ts") -- Enable modules (experimental)
end

target("MiJIT")
    set_kind("binary")
    add_files("main.cpp")
    set_warnings("all", "extra")
    add_cxxflags("-pedantic")
    
    -- Ensure C++23 features are available
    if is_plat("linux") then
        add_cxxflags("-std=c++23")
    end
    
    -- Link required libraries
    if is_plat("linux") then
        add_syslinks("pthread") -- For threading support
    end
