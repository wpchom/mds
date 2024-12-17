target("mds_kernel")
    set_kind("static")

    if is_os("macosx") then
        add_defines("MDS_LOG_SECTION=\",.logstr.\"")
        add_defines("MDS_INIT_SECTION=\",.mds.init.\"")
        add_defines("MDS_SYSMEM_HEAP_SECTION=\",.heap\"")
    end

    add_includedirs("inc")

    add_files("src/*.c")
    add_files("src/sys/*.c")
    add_files("src/mem/*.c")

target_end()
