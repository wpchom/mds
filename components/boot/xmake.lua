add_requires("lzma", { optional = true, system = false })

namespace("boot", function()
    option("lzma", function()
        set_default(false)
    end)
end)

target("boot", function()
    set_kind("object")

    add_includedirs("../../include/components", { public = true })

    add_files("mds_boot.c")

    add_options("boot::lzma")
    if get_config("boot::lzma") then
        add_packages("lzma")
        add_files("boot_lzma.c")
        add_defines("CONFIG_MDS_BOOT_UPGRADE_WITH_LZMA=1")
    end
end)
