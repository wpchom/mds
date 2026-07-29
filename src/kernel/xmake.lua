option("archcore", function()
    set_showmenu(true)
    set_description("Arch core source, e.g. arm/thumbv7m builds arch/arm/thumbv7m.c")
    set_category("kernel")
end)

option("nosys", function()
    set_default(false)
    set_showmenu(true)
    set_description("Build without sys layer, fall back to nosys.c stubs")
    set_category("kernel")
end)

target("kernel", function()
    set_kind("object")
    set_default(false)

    add_options("archcore", "nosys")

    add_files("critical.c", "device.c", "log.c", "lpc.c", "object.c", "utils.c")
    add_files("mm/*.c")

    if get_config("archcore") then
        add_files("arch/" .. get_config("archcore") .. ".c")
    end

    if has_config("nosys") then
        add_files("nosys.c")
        add_defines("CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX=0", {public = true})
    else
        add_files("sys/**.c")
    end

    add_deps("mds::include")
end)
