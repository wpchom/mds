set_project("mds")

add_rules("mode.debug", "mode.release")

namespace("mds", function()
    option("confile", function()
        set_showmenu(true)
        set_description("User config header path, copied into builddir as mds_config.h")
    end)

    target("include", function()
        set_kind("headeronly")
        set_default(false)

        add_options("confile")
        if get_config("confile") then
            add_configfiles(get_config("confile"), {filename = "mds_config.h", onlycopy = true})
            add_includedirs("$(builddir)", {public = true})
            add_headerfiles("$(builddir)/mds_config.h", {prefixdir = "mds"})
        end

        add_headerfiles("include/(**.h)")
        add_includedirs("include/", {public = true})
    end)

    includes("src/kernel", "src/device", "src/subsys")
end)

target("mds", function()
    set_kind("static")

    add_deps("mds::kernel", "mds::device", "mds::subsys", {public = true})
end)
