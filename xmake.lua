target("mds", function()
    set_kind("static")

    -- kernel
    includes("src/kernel")
    add_deps("kernel")

    -- device
    includes("src/device")
    add_deps("device")

    -- component
    includes("src/component")
    add_deps("component")

end)
