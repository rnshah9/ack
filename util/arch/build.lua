cprogram {
	name = "aal",
	srcs = { "./archiver.c" },
	deps = {
		"h+emheaders",
		"modules/src/data+lib",
		"modules/src/object+lib",
		"modules/src/print+lib",
		"modules/src/string+lib",
		"modules/src/system+lib"
	},
	vars = {
		["+cflags"] = "-DAAL"
	}
}

installable {
	name = "pkg",
	map = {
		["$(INSDIR)/bin/aal"] = "+aal",
		["$(INSDIR)/share/man/man1/aal.1"] = "./aal.1"
	}
}
