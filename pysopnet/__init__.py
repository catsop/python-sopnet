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

def _underscore_to_camel(word, first_capital=False):
    return ''.join(x.capitalize() if first_capital or i else x or '_' for i, x in enumerate(word.split('_')))

def _rebuild_enum(enum, name):
    return enum.names[name]

def _register_enum_pickler(enum):
    import copy_reg
    def reduce_enum(e):
        return (_rebuild_enum, (type(e), e.name))
    copy_reg.constructor(enum)
    copy_reg.pickle(enum, reduce_enum)

# Allow LogLevel to be pickled, for easy use with celery.
_register_enum_pickler(LogLevel)

class _extend_point3(_injector, point3):
    def __unicode__(self):
        return u'(%s, %s, %s)' % (self.x, self.y, self.z)

    def __str__(self):
        return unicode(self).encode('utf-8')

# Allow ProjectConfiguration to be pickled, for easy use with celery.
class _extend_ProjectConfiguration(_injector, ProjectConfiguration):
    def __getstate__(self):
        state = {'catmaid_stacks': {}}
        for stack_type in StackType.values.itervalues():
            stack = self.getCatmaidStack(stack_type)
            stack_dict = {}
            for name in ['id', 'segmentation_id', 'image_base', 'file_extension',
                         'tile_source_type', 'tile_width', 'tile_height',
                         'width', 'height', 'depth',
                         'res_x', 'res_y', 'res_z', 'scale']:
                stack_dict[name] = getattr(stack, _underscore_to_camel(name))
            state['catmaid_stacks'][stack_type.name] = stack_dict

        block_size = self.getBlockSize()
        state['block_size'] = [block_size.x, block_size.y, block_size.z]
        core_size = self.getCoreSize()
        state['core_size'] = [core_size.x, core_size.y, core_size.z]
        volume_size = self.getVolumeSize()
        state['volume_size'] = [volume_size.x, volume_size.y, volume_size.z]

        optional_params = [
                'component_directory',
                'postgre_sql_database', 'postgre_sql_host', 'postgre_sql_port',
                'postgre_sql_user', 'postgre_sql_password']
        for param_name in optional_params:
            state[param_name] = getattr(self, 'get' + _underscore_to_camel(param_name, True))()
        state['backend_type'] = self.getBackendType().name

        return state

    def __setstate__(self, state):
        for stack_type_name, stack_dict in state['catmaid_stacks'].iteritems():
            stack = StackDescription()
            for name in ['id', 'segmentation_id', 'image_base', 'file_extension',
                         'tile_source_type', 'tile_width', 'tile_height',
                         'width', 'height', 'depth',
                         'res_x', 'res_y', 'res_z', 'scale']:
                setattr(stack, _underscore_to_camel(name), stack_dict[name])
            stack = self.setCatmaidStack(StackType.names[stack_type_name], stack)

        block_size = state['block_size']
        self.setBlockSize(point3(block_size[0], block_size[1], block_size[2]))
        core_size = state['core_size']
        self.setCoreSize(point3(core_size[0], core_size[1], core_size[2]))
        volume_size = state['volume_size']
        self.setVolumeSize(point3(volume_size[0], volume_size[1], volume_size[2]))

        optional_params = [
                'component_directory',
                'postgre_sql_database', 'postgre_sql_host', 'postgre_sql_port',
                'postgre_sql_user', 'postgre_sql_password']
        for param_name in optional_params:
            getattr(self, 'set' + _underscore_to_camel(param_name, True))(state[param_name])
        self.setBackendType(BackendType.names[state['backend_type']])
