target("device", function()
    set_kind("object")

    add_includedirs(
        "../include/kernel",
        "../include/device",
        { public = true }
    )

    add_headerfiles("../include/(device/**.h)")

    add_files("**.c")
end)
