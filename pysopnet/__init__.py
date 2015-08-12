from libpysopnet import *

# Arbitrary class to get metaclass Boost.Python uses for all wrapped classes.
BOOST_PYTHON_METACLASS = point3.__class__

class _injector(object):
    class __metaclass__(BOOST_PYTHON_METACLASS):
        def __init__(self, name, bases, dict):
            for b in bases:
                if type(b) not in (self, type):
                    for k, v in dict.items():
                        setattr(b, k, v)
            return type.__init__(self, name, bases, dict)

class _extend_point3(_injector, point3):
    def __unicode__(self):
        return u'(%s, %s, %s)' % (self.x, self.y, self.z)

    def __str__(self):
        return unicode(self).encode('utf-8')

