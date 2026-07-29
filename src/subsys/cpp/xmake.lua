target("cpp", function()
    set_kind("object")
    set_default(false)

    add_files("crt.cpp")

    add_deps("mds::include")
end)
