target("component", function()
    set_kind("object")

    -- deps
    includes("../kernel", "../device")
    add_deps("kernel", "device")

    add_includedirs("../../include/component")

    add_files("algo/**.c")

end)
