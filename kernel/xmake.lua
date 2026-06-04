option("kernel::archcore", function()
    set_default(nil)
end)

option("kernel::nosys", function()
    set_default(false)
end)

target("mds::kernel", function()
    set_kind("object")

    add_options("kernel::archcore", "kernel::nosys")

    add_files("critical.c", "device.c", "log.c", "lpc.c", "object.c", "utils.c")

    if get_config("kernel::archcore") then
        add_files(path.join(os.scriptdir(), "arch", get_config("kernel::archcore") .. ".c"))
    end

    if get_config("kernel::nosys") then
        add_files("nosys.c")
        add_defines("CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX=0", {
            public = true
        })
    else
        add_files("sys/**.c")
    end

    add_files("mm/*.c")

    add_deps("mds::include")
end)
