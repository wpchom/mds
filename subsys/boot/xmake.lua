add_requires("lzma", {
    optional = true,
    system = false
})

option("subsys::boot::lzma", function()
    set_category("subsys/boot")
    set_default(false)
end)

target("mds::subsys::boot", function()
    set_kind("object")

    add_files("boot.c")

    add_options("subsys::boot::lzma")
    if get_config("subsys::boot::lzma") then
        add_packages("lzma")
        add_files("boot_lzma.c")
        add_defines("CONFIG_MDS_BOOT_UPGRADE_WITH_LZMA=1")
    end

    add_deps("mds::subsys::include")
end)
