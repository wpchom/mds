namespace("kernel", function()
    option("archcore", function()
        set_default(nil)
    end)

    option("confile", function()
        set_default(nil)
    end)

    option("nosys", function()
        set_default(false)
    end)
end)

target("kernel", function()
    set_kind("object")

    add_includedirs("../include/kernel", { public = true })

    add_options("archcore", "confile", "nosys")

    add_headerfiles("../include/(kernel/**.h)")

    if get_config("kernel::confile") then
        add_defines("MDS_CONFIG_FILE=\"" .. get_config("kernel::confile") .. "\"", { public = true })
    end

    if is_plat("macosx") then
        add_defines(
            "CONFIG_MDS_INIT_SECTION=\"rodata,init.mdsInit.\"",
            "CONFIG_MDS_LOG_FORMAT_SECTION=\"rodata,logfmt.\"",
            { public = true })
    end

    add_files("critical.c", "device.c", "log.c", "lpc.c", "object.c", "utils.c")

    if get_config("kernel::archcore") then
        add_files(path.join(os.scriptdir(), "arch", get_config("kernel::archcore") .. "*.c"))
    end

    if get_config("kernel::nosys") then
        add_files("nosys.c")
        add_defines("CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX=0", { public = true })
    else
        add_files("sys/**.c")
    end

    add_files("mm/*.c")
end)
