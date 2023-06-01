Import("env")
if env.IsIntegrationDump():
    Return()

print("Enter template name name:")
print("NO_FEATURES")
print("DUAL_LIGHT")
print("COVER")
print("BHPZEM")
print("GATE")
model = input()
env.Append(
    CPPDEFINES=[model],
)
my_flags = env.ParseFlags(env['BUILD_FLAGS'])

env.Replace(PROGNAME="firmware_%s_%s" % (model , my_flags.get("CPPDEFINES")[1][1]))