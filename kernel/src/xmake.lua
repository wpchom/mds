-- log
option("log_build_level", function()
    set_default("info")
    set_showmenu(true)
    set_category("log")
    set_description("Set the log build level")
    set_values("off", "track", "fatal", "error", "warn", "info", "debug", "trace")

    before_check(function(option)
        if option:value() == "off" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_OFF")
        elseif option:value() == "track" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_OFF")
        elseif option:value() == "fatal" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_FATAL")
        elseif option:value() == "error" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_ERROR")
        elseif option:value() == "warn" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_WARN")
        elseif option:value() == "info" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_INFO")
        elseif option:value() == "debug" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_DEBUG")
        elseif option:value() == "trace" then
            option:add("defines", "MDS_LOG_BUILD_LEVEL=MDS_LOG_LEVEL_TRACE")
        end
    end)
end)

option("use_assert", function()
    set_default(false)
    set_showmenu(true)
    set_category("log")
    set_description("Enable/disable assert")

    add_defines("MDS_USE_ASSERT=1")
end)

-- object
option("object_name_size", function()
    set_default(7)
    set_showmenu(true)
    set_category("objcet")
    set_description("Set the object name size")

    before_check(function(option)
        option:add("defines", "MDS_OBJECT_NAME_SIZE=" .. option:value())
    end)
end)
