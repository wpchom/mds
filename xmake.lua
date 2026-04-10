namespace("mds", function()
    includes("kernel", "device", "components")
end)

target("mds", function()
    set_kind("static")

    add_deps("mds::kernel", "mds::device", "mds::components", { public = true })
end)
