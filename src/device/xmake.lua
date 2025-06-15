target("device", function()
    set_kind("object")

    add_files("**.c")

    -- deps
    includes("../kernel")
    add_deps("kernel")

end)
