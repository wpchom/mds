target("mds::device", function()
    set_kind("object")

    add_files("**.c")

    add_deps("mds::include")
end)
