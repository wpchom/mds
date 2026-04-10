target("fsm", function () 
    set_kind("object")

    add_includedirs(
        "../../include/kernel",
        "../../include/components",
        { public = true }
    )

    add_files("*.c")
end)
