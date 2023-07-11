Import("env")
if env.IsIntegrationDump():
    Return()

print("ONOFRE TEMPLATES:")
print("GENERIC")
print("DUAL_LIGHT")
print("COVER")
print("BHPZEM")
print("GATE")
print("Please Enter template name name:")
model = input()
env.Append(
    CPPDEFINES=[model],
)
my_flags = env.ParseFlags(env['BUILD_FLAGS'])

env.Replace(PROGNAME="firmware_%s_%s" % (model , my_flags.get("CPPDEFINES")[0][1]))