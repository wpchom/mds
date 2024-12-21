-- includes
includes("src", "src/sys", "src/mem")

-- target
target("mds_kernel", function()

    set_kind("static")

    -- common
    add_files("src/*.c")
    add_includedirs("inc", {
        public = true
    })
    add_headerfiles("inc/**.h", {
        -- prefixdir = "kernel",
        -- install = false
    })

    -- src/arch/
    on_load(function(target)
        local archfile = os.scriptdir() .. "/src/arch/" .. get_config("arch") .. ".c"
        if not os.exists(archfile) then
            raise("arch file not found: %s", archfile)
        else
            target:add("files", (archfile))
        end
    end)

    -- src/mem/
    add_files("src/mem/*.c")

    -- src/sys/
    add_files("src/sys/*.c")

    -- options
    add_options("use_assert", {
        public = true
    })

    -- add_options("clock_tick_freq", "timer_skiplit_level", "timer_skiplit_shift", "timer_thread_enable")
    -- add_options("kernel_stats_enable", "kernel_hook_enable")

    -- fix macosx test
    if is_plat("macosx") then
        add_defines("MDS_LOG_SECTION=\",.logstr.\"", "MDS_INIT_SECTION=\",.mds.init.\"", {
            public = true
        })
        add_defines("MDS_SYSMEM_HEAP_SECTION=\",.heap\"")
    end
end)
