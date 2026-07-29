target("fsm", function()
    set_kind("object")
    set_default(false)

    add_files("*.c")

    add_deps("mds::include")
end)
