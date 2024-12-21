option("sysmem_with_ops", function()
    set_default("llff")
    set_category("sysmem")

    before_check(function(option)
        if option:value() == "llff" then
            option:add("defines", "MDS_SYSMEM_HEAP_OPS=G_MDS_MEMHEAP_OPS_LLFF")
        elseif option:value() == "tlsf" then
            option:add("defines", "MDS_SYSMEM_HEAP_OPS=G_MDS_MEMHEAP_OPS_TSLF")
        else
            assert("Unknown sysmem heap ops: " .. option:value())
        end
    end)

end)

option("sysmem_heap_size", function()
    set_default(0x4000)
    set_category("sysmem")

end)
