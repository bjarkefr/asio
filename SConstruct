import os.path
import string

import SCons.Defaults
import SCons.Tool
import SCons.Util

ProtocAction = SCons.Action.Action('protoc -I=${SOURCES.dir} --cpp_out=${TARGET.dir} ${SOURCES}')

def protocEmitter(target, source, env):
    targetBase, targetExt = os.path.splitext(SCons.Util.to_String(target[0]))
    target = [targetBase + '.pb.cc', targetBase + '.pb.h']
    return (target, source)

def generate(env):
    c_file, cxx_file = SCons.Tool.createCFileBuilders(env)
    cxx_file.add_action('.proto', ProtocAction)
    cxx_file.add_emitter('.proto', protocEmitter)

def exists(env):
    return env.Detect(['protoc'])

env = Environment()
pre_libs = [];
if env['PLATFORM'] == 'win32':
	env = Environment(tools = ['mingw']);
	pre_libs = ['ws2_32', 'mingw32'];
generate(env);

#PrecompAction = SCons.Action.Action('${CXX} ${CCFLAGS} ${SOURCES} -o ${TARGET}')

#ProtocBuilder = SCons.Builder.Builder(	action = PrecompAction,
#					source_scanner = SCons.Scanner.C.CScanner(),
#					suffix = '.h.gch')

#def generate(env):
#    env.Append(BUILDERS = {
#	'Gch': env.Builder(action = PrecompAction, target_factory = env.fs.File)
#    })

generate(env);
env.Decider('timestamp-newer');

env['CCFLAGS']='-g3 -Wall -std=c++11 -pedantic -I/usr/local/include'

#precompiled = env.Gch('precompiled/all.h.gch', 'precompiled/all.h')

server_source = Glob('src/server/*.cc')# + Glob('src/Utils/Algorithm/*.cc')
client_source = Glob('src/client/*.cc')

server_program = env.Program('server',
    server_source,
    LIBS=pre_libs + ['boost_system', 'protobuf', 'pthread'],
#    CXX='/home/bjarke/gstlfilt/gfilt',
    CCFLAGS='${CCFLAGS} -D_WIN32_WINNT=0x0601')

client_program = env.Program('client',
	client_source,
	LIBS=pre_libs + ['boost_system', 'protobuf', 'pthread'],
	CCFLAGS='${CCFLAGS} -D_WIN32_WINNT=0x0601')
	
#env.Requires(generate_program) #, precompiled)
