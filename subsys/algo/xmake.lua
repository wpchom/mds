target("mds::subsys::algo", function ()
    set_kind("object")

    add_files("*.c")

    add_deps("mds::subsys::include")
end)
