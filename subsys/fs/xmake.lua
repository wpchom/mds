includes("*")

option("subsys::fs::littlefs", function()
    set_category("subsys/fs")
    set_default(false)
end)

target("mds::subsys::fs", function()
    set_kind("object")

    add_files("*.c")

    add_options("subsys::fs::littlefs")
    if get_config("subsys::fs::littlefs") then
        -- add_deps
    end

    add_deps("mds::subsys::include")
end)
