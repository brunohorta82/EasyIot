Import("env")

build_tag = "v2.2"

env.Replace(PROGNAME="firmware_%s" % build_tag)