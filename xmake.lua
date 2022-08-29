add_requires("tl_expected", { optional = true })
add_requires("frozen")

add_rules("mode.debug",
          "mode.release",
          "mode.asan")

target("module2header")
    set_kind("binary")
    set_languages("cxxlatest", "clatest")

    on_load(function (target)
        import("lib.detect.check_cxsnippets")
        local has_std_expected = check_cxsnippets("static_assert(__cpp_lib_expected &&  && __cpp_lib_expected >= 202202L)")
        if not has_std_expected then
            target:add("packages", "tl_expected", {public = true})
        end
    end)

    add_packages("frozen")

    add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", { public = true })

    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_cxxflags("/bigobj", "/permissive-", "/Zc:wchar_t", "/Zc:__cplusplus", "/Zc:externConstexpr", "/Zc:inline", "/Zc:lambda", "/Zc:preprocessor", "/Zc:referenceBinding", "/Zc:strictStrings", "/Zc:throwingNew")
    add_cxflags("/wd4251") -- Disable warning: class needs to have dll-interface to be used by clients of class blah blah blah
    add_cxflags("/wd4297")
    add_cxflags("/wd5063")
    add_cxflags("/wd5260")
    add_cxflags("/wd5050")
    add_cxflags("/wd4005")
    add_cxflags("/wd4611")

    add_files("src/main.cpp")

    set_policy("build.c++.modules", true)
