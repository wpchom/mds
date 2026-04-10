target("button", function()
    set_kind("object")

    add_includedirs("../../include/components", { public = true })

    add_files("*.c")
end)
