option("lzma", function()
    set_default(false)
    set_showmenu(true)
    set_description("Enable LZMA compressed boot upgrade image")
    set_category("subsys/boot")
end)

if has_config("lzma") then
    add_requires("lzma", {system = false})
end

target("boot", function()
    set_kind("object")
    set_default(false)

    add_files("boot.c")

    add_options("lzma")
    if has_config("lzma") then
        add_packages("lzma")
        add_files("boot_lzma.c")
        add_defines("CONFIG_MDS_BOOT_UPGRADE_WITH_LZMA=1")
    end

    add_deps("mds::include")
end)
