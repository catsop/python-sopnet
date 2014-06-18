#!/usr/bin/python

import libpysopnet as ps

print "Increasing wrapper verbosity"

ps.setLogLevel(4);

print "Creating a poject configuration"

config = ps.ProjectConfiguration()

print "setting Django URL"
config.setDjangoUrl("http://django.url")
print config.getDjangoUrl()

print "setting volume size"
config.setVolumeSize(ps.point3(1024, 1024, 20))

print "setting block size"
config.setBlockSize(ps.point3(16, 16, 8))
bs = config.getBlockSize()
print "[" + str(bs.x) + ", " + str(bs.y) + ", " + str(bs.z) + "]"

print "setting core size"
config.setCoreSize(ps.point3(2, 2, 1))

print
print
print "Creating a SliceGuarantor"

sliceGuarantor = ps.SliceGuarantor()

print
print
print "Requesting block (0,0,0)"

request = ps.point3(0,0,0)
parameters = ps.SliceGuarantorParameters()
sliceGuarantor.fill(request, parameters, config)

print
print
print "Requesting block (0,0,20)"

request = ps.point3(0,0,20)
parameters = ps.SliceGuarantorParameters()
sliceGuarantor.fill(request, parameters, config)

print
print
print "Creating a SegmentGuarantor"

segmentGuarantor           = ps.SegmentGuarantor()

print
print
print "Requesting a block"

segmentGuarantorParameters = ps.SegmentGuarantorParameters()
missing = segmentGuarantor.fill(request, segmentGuarantorParameters, config);

print "Missing points:"
for p in missing:
  print "[" + str(p.x) + ", " + str(p.y) + ", " + str(p.z) + "]"

print
print
print "Creating a SolutionGuarantor"

solutionGuarantor           = ps.SolutionGuarantor()

print
print
print "Requesting a block"

request = ps.point3(0,0,0)
solutionGuarantorParameters = ps.SolutionGuarantorParameters()
solutionGuarantorParameters.setCorePadding(2);
missing = solutionGuarantor.fill(request, solutionGuarantorParameters, config);

print "Missing points:"
for p in missing:
  print "[" + str(p.x) + ", " + str(p.y) + ", " + str(p.z) + "]"
