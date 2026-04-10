namespace("components", function()
    includes("*")

    option("algo", function()
        set_default(true)
    end)

    option("boot", function()
        set_default(false)
    end)

    option("button", function()
        set_default(false)
    end)

    option("fs", function()
        set_default(false)
    end)

    option("fsm", function()
        set_default(false)
    end)

    option("topic", function()
        set_default(false)
    end)
end)

target("components", function()
    set_kind("object")

    add_options("components::algo")
    if get_config("components::algo") then
        add_deps("components::algo", { public = true })
    end

    add_options("components::boot")
    if get_config("components::boot") then
        add_deps("components::boot", { public = true })
    end

    add_options("components::button")
    if get_config("components::button") then
        add_deps("components::button", { public = true })
    end

    add_options("components::fs")
    if get_config("components::fs") then
        add_deps("components::fs", { public = true })
    end

    add_options("components::fsm")
    if get_config("components::fsm") then
        add_deps("components::fsm", { public = true })
    end

    add_options("components::topic")
    if get_config("components::topic") then
        add_deps("components::topic", {public = true})
    end
end)
