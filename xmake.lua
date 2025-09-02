
add_rules("mode.debug", "mode.release")


set_languages("c++14")


target("hello_0")
    set_kind("binary")
    add_files("hello_0.cpp")
    set_warnings("all") 
    add_cxxflags("-pedantic")
