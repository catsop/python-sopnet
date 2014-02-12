#!/usr/bin/python

import libpysopnet as ps

print "Increasing wrapper verbosity"

ps.setLogLevel(3);

print "Creating a poject configuration"

config = ps.ProjectConfiguration()

print "setting Django URL"
config.setDjangoUrl("http://django.url")
print config.getDjangoUrl()

print "setting block size"
config.setBlockSize(ps.point3(16, 16, 8))
bs = config.getBlockSize()
print "[" + str(bs.x) + ", " + str(bs.y) + ", " + str(bs.z) + "]"

print "Creating a SliceGuarantor"

sliceGuarantor = ps.SliceGuarantor()

print "Requesting a block"

request = ps.point3(5,5,2)
parameters = ps.SliceGuarantorParameters()
sliceGuarantor.fill(request, parameters, config)

print "Creating a SegmentGuarantor"

segmentGuarantor           = ps.SegmentGuarantor()

print "Requesting a block"

segmentGuarantorParameters = ps.SegmentGuarantorParameters()
missing = segmentGuarantor.fill(request, segmentGuarantorParameters, config);

print "Missing points:"
for p in missing:
  print "[" + str(p.x) + ", " + str(p.y) + ", " + str(p.z) + "]"
