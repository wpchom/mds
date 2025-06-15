option("minilib", function()
    set_default(false)
end)

target("library", function()
    set_kind("object")

    add_includedirs("../../include")
    add_files("**.c")

    add_options("minilib")
    if get_config("minilib") then
        add_defines("CONFIG_MDS_LIBRARY_MINIABLE=1")
    else
        add_defines("CONFIG_MDS_LIBRARY_MINIABLE=0")
    end

    -- ld section for macos
    on_load(function(target)
        if is_plat("macosx") then
            target:add("defines", "CONFIG_MDS_INIT_SECTION=\"__TEXT__,init_\"", {
                public = true
            })
            target:add("defines", "CONFIG_MDS_LOG_FORMAT_SECTION=\"__TEXT__,logfmt_\"", {
                public = true
            })
        end
    end)

end)
