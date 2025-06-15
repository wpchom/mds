option("conf", function()
end)

option("core", function()
    set_showmenu(true)
    set_values("cortex-m/thumbv7em", "risc-v/riscv")
end)

option("priority_max", function()
    set_default(32)
end)

target("kernel", function()
    set_kind("object")

    add_options("conf")
    if has_config("conf") then
        add_defines("CONFIG_MDS_CONFIG_FILE=\"$(conf)\"", {
            public = true
        })
    end

    add_includedirs("../../include", {
        public = true
    })

    add_files("log.c", "object.c", "device.c")

    add_options("core")
    if has_config("core") then
        add_files("core/$(core).c")
    end

    add_files("mem/**.c")

    add_options("priority_max")
    if (get_config("priority_max") == "0") then
        add_files("nosys.c")
    else
        add_files("sys/**.c")
    end

    -- deps
    includes("../library")
    add_deps("library")

end)
