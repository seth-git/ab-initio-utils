env = Environment(CPPFLAGS='-Wall', LIBS='-lrt', CXX = 'mpiCC', CXXFLAGS='-std=c++14')
random = Split('random/mersenne.cpp random/random.cc')
src = [Glob('*.cc'),
       Glob('translation/*.cc'),
       Glob('xsd/*.cc'),
       Glob('handbook/*.cc'),
       Glob('matrix/*.cc'),
       Glob('externalEnergy/*.cc'),
       Glob('internalEnergy/*.cc'),
       Glob('globalmin/*.cc'),
       Glob('globalmin/initialization/*.cc'),
       Glob('globalmin/simulatedAnnealing/*.cc')
      ] + random
env.Program(target = 'aiu', source = src)
