namespace("subsys", function()
    includes("*")
end)

target("subsys", function()
    set_kind("object")
    set_default(false)

    add_deps("subsys::algo", "subsys::boot", "subsys::button", "subsys::cpp",
             "subsys::fs", "subsys::fsm", "subsys::topic")
end)
