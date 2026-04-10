namespace("fs", function()
    includes("*")

    option("littlefs", function()
        set_default(false)
    end)
end)

target("fs", function()
    set_kind("object")

    add_includedirs(
        "../../include/kernel",
        "../../include/components",
        { public = true }
    )

    add_files("*.c")

    add_options("fs::littlefs")
    if get_config("fs::littlefs") then
        add_deps("fs::littlefs", { public = true })
    end
end)
