includes("kernel", "device", "subsys")

option("confile", function()
    set_default(nil)
end)

target("mds::include", function()
    set_kind("headeronly")
    set_default(false)

    add_options("confile")
    on_load(function(target)
        if get_config("confile") then
            os.cp(get_config("confile"), path.join(get_config("builddir"), "mds_config.h"))
            target:add("includedirs", get_config("builddir"), {
                public = true
            })
        end
    end)

    add_headerfiles("include/(**.h)")
    add_includedirs("include/", {
        public = true
    })
end)

target("mds", function()
    set_kind("static")

    add_headerfiles("include/(**.h)")

    on_load(function(target)
        if get_config("confile") then
            target:add("headerfiles", "$(builddir)/mds_config.h", {
                prefixdir = "mds"
            })
        end
    end)

    add_deps("mds::kernel", "mds::device", "mds::subsys", {
        public = true
    })
end)
