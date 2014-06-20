#!/usr/bin/python

import libpysopnet as ps

print "Increasing wrapper verbosity"

ps.setLogLevel(3);

print "Creating a poject configuration"

config = ps.ProjectConfiguration()
config.setBackendType(ps.BackendType.Local)

print "setting volume size"
config.setVolumeSize(ps.point3(1024, 1024, 20))

print "setting block size"
config.setBlockSize(ps.point3(512, 512, 10))
bs = config.getBlockSize()
print "[" + str(bs.x) + ", " + str(bs.y) + ", " + str(bs.z) + "]"

print "setting core size"
config.setCoreSize(ps.point3(1, 1, 1))

print
print
print "Creating a SolutionGuarantor"

solutionGuarantor = ps.SolutionGuarantor()

print
print
print "Requesting a block"

request = ps.point3(0,0,0)
solutionGuarantorParameters = ps.SolutionGuarantorParameters()
missing = solutionGuarantor.fill(request, solutionGuarantorParameters, config);

print "Missing points:"
for p in missing:
  print "[" + str(p.x) + ", " + str(p.y) + ", " + str(p.z) + "]"
