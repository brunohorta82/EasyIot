Import("env")
my_flags = env.ParseFlags(env['BUILD_FLAGS'])

env.Replace(PROGNAME="OnOfre_firmware_%s_%s" % (env["PIOENV"] , my_flags.get("CPPDEFINES")[0][1]))