includes("*")

target("mds::subsys::include", function()
    set_kind("headeronly")
    set_default(false)
    add_deps("mds::include")
end)

target("mds::subsys", function()
    set_kind("object")

    add_deps("mds::subsys::algo")
    add_deps("mds::subsys::boot")
    add_deps("mds::subsys::button")
    add_deps("mds::subsys::cpp")
    add_deps("mds::subsys::fs")
    add_deps("mds::subsys::fsm")
    add_deps("mds::subsys::topic")

end)
