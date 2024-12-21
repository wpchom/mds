-- clock
option("clock_tick_freq", function()
    set_default(1000)
    set_showmenu(true)
    set_category("clock")
    set_description("Set the clock tick frequency")

    before_check(function(option)
        option:add("defines", "MDS_CLOCK_TICK_FREQ=" .. option:value(), {
            public = true
        })
    end)
end)

-- timer
option("timer_skiplit_level", function()
    set_default(1)
    set_showmenu(true)
    set_category("timer")
    set_description("Set the timer skip level")
end)

option("timer_skiplit_shift", function()
    set_default(2)
    set_showmenu(true)
    set_category("timer")
    set_description("Set the timer skip shift")
end)

option("timer_thread_enable", function()
    set_default(false)
    set_showmenu(true)
    set_category("timer")
    set_description("Enable/disable timer thread")

    add_defines("MDS_TIMER_THREAD_ENABLE=1", {
        public = true
    })
end)

-- kernel
option("kernel_stats_enable", function()
    set_default(false)
    set_showmenu(true)
    set_category("kernel")
    set_description("Enable/disable kernel stats")

    add_defines("MDS_KERNEL_STATS_ENABLE=1", {
        public = true
    })
end)

option("kernel_hook_enable", function()
    set_default(false)
    set_showmenu(true)
    set_category("kernel")
    set_description("Enable/disable kernel hook")

    add_defines("MDS_KERNEL_HOOK_ENABLE=1", {
        public = true
    })
end)

