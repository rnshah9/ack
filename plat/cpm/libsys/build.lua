acklibrary {
    name = "internal",
    hdrs = {
		"./asm.h",
		"./cpmsys.h",
	}
}

local bdos_calls = {
    [ 0] = "cpm_exit",
    [ 1] = "cpm_conin",
    [ 2] = "cpm_conout",
    [ 3] = "cpm_auxin",
    [ 4] = "cpm_auxout",
    [ 5] = "cpm_lstout",
    [ 6] = "cpm_conio",
    [ 7] = "cpm_get_iobyte",
    [ 8] = "cpm_set_iobyte",
    [ 9] = "cpm_printstring",
    [10] = "cpm_readline",
    [11] = "cpm_const",
    [12] = "cpm_get_version",
    [13] = "cpm_reset_disk_system",
    [14] = "cpm_select_drive",
    [15] = "cpm_open_file",
    [16] = "cpm_close_file",
    [17] = "cpm_findfirst",
    [18] = "cpm_findnext",
    [19] = "cpm_delete_file",
    [20] = "cpm_read_sequential",
    [21] = "cpm_write_sequential",
    [22] = "cpm_make_file",
    [23] = "cpm_rename_file",
    [24] = "cpm_get_login_vector",
    [25] = "cpm_get_current_drive",
    [26] = "cpm_set_dma",
    [27] = "cpm_get_allocation_vector",
    [28] = "cpm_write_protect_drive",
    [29] = "cpm_get_readonly_vector",
    [30] = "cpm_set_file_attributes",
    [31] = "cpm_get_dpb",
    [32] = "cpm_get_set_user",
    [33] = "cpm_read_random",
    [34] = "cpm_write_random",
    [35] = "cpm_seek_to_end",
    [36] = "cpm_seek_to_seq_pos",
    [37] = "cpm_reset_drives",
    [40] = "cpm_write_random_filled",
}

local bios_calls = {
    [ 6] = "cpm_bios_const",
    [ 9] = "cpm_bios_conin",
    [12] = "cpm_bios_conout",
    [15] = "cpm_bios_list",
    [18] = "cpm_bios_punch",
    [21] = "cpm_bios_reader",
    [24] = "cpm_bios_home",
    -- Special: [27] = "cpm_bios_seldsk",
    [30] = "cpm_bios_settrk",
    [33] = "cpm_bios_setsec",
    [36] = "cpm_bios_setdma",
    [39] = "cpm_bios_read",
    [42] = "cpm_bios_write",
    [45] = "cpm_bios_listst",
    -- Special: [48] = "cpm_bios_sectran", 
}

local trap_calls = {
    "EARRAY",
    "EBADGTO",
    "EBADLAE",
    "EBADLIN",
    "EBADMON",
    "EBADPC",
    "EBADPTR",
    "ECASE",
    "ECONV",
    "EFDIVZ",
    "EFOVFL",
    "EFUND",
    "EFUNFL",
    "EHEAP",
    "EIDIVZ",
    "EILLINS",
    "EIOVFL",
    "EIUND",
    "EMEMFLT",
    "EODDZ",
    "ERANGE",
    "ESET",
    "ESTACK",
    "EUNIMPL",
}

local generated = {}
for n, name in pairs(bdos_calls) do
    generated[#generated+1] = normalrule {
        name = name,
        ins = { "./make_bdos_call.sh" },
        outleaves = { name..".s" },
        commands = {
            "%{ins[1]} "..n.." "..name.." > %{outs}"
        }
    }
end
for n, name in pairs(bios_calls) do
    generated[#generated+1] = normalrule {
        name = name,
        ins = { "./make_bios_call.sh" },
        outleaves = { name..".s" },
        commands = {
            "%{ins[1]} "..n.." "..name.." > %{outs}"
        }
    }
end
for _, name in pairs(trap_calls) do
    generated[#generated+1] = normalrule {
        name = name,
        ins = { "./make_trap.sh" },
        outleaves = { name..".s" },
        commands = {
            "%{ins[1]} "..name:lower().." "..name.." > %{outs}"
        }
    }
end

acklibrary {
    name = "lib",
    srcs = {
		"./_bdos.s",
		"./_bios_raw.s",
		"./_bios.s",
		"./bios_sectran.s",
		"./bios_seldsk.s",
		"./brk.c",
		"./close.c",
		"./cpm_overwrite_ccp.s",
		"./cpm_printstring0.s",
		"./cpm_read_random_safe.c",
		"./creat.c",
		-- "./errno.s",
		"./fcb.c",
		"./fd.c",
		"./getpid.c",
		"./_hol0.s",
		"./_inn2.s",
		"./isatty.c",
		"./kill.c",
		"./lseek.c",
		"./open.c",
		"./read.c",
		"./signal.c",
		"./time.c",
		"./_trap.s",
		"./write.c",
        generated
    },
	deps = {
		"lang/cem/libcc.ansi/headers+headers",
        "plat/cpm/include+headers",
        "+internal",
	},
    vars = {
        plat = "cpm"
    }
}

