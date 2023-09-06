import os
import platform

project_name = 'hotel-booking'
target_client = 'client'
target_server = 'server'

env = Environment()

# Specify the binary name for the executable
server = env.Program(os.path.join('bin', target_server), source=['src/server.c'])
client = env.Program(os.path.join('bin', target_client), source=['src/client.c'])

# Link client with pthread
env.Append(LIBS=['pthread'])
env.AppendUnique(LINKFLAGS=['-pthread'])
env.Append(LIBS=['sqlite3'])

# Link server with pthread, crypt, and sqlite3 (if not macOS)
if platform.system() != 'Darwin':
    env.Append(LIBS=['crypt'])
    env.AppendUnique(LINKFLAGS=['-pthread'])
