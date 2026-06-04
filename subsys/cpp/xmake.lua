target("mds::subsys::cpp", function()
    set_kind("object")

    add_files("crt.cpp")

    add_deps("mds::subsys::include")
end)
