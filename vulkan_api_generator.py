#!/usr/bin/python3 -i
#
# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
# Copyright (c) 2015-2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Mark Lobodzinski <mark@lunarg.com>

import os,re,sys
import xml.etree.ElementTree as etree
from generator import *
from collections import namedtuple


# VulkanApiGeneratorOptions - subclass of GeneratorOptions.
#
# Adds options used by VulkanApiOutputGenerator object during
# vulkan header api generator construction.
#
# Additional members
#   prefixText - list of strings to prefix generated header with
#     (usually a copyright statement + calling convention macros).
#   protectFile - True if multiple inclusion protection should be
#     generated (based on the filename) around the entire header.
#   protectFeature - True if #ifndef..#endif protection should be
#     generated around a feature interface in the header file.
#   genFuncPointers - True if function pointer typedefs should be
#     generated
#   protectProto - If conditional protection should be generated
#     around prototype declarations, set to either '#ifdef'
#     to require opt-in (#ifdef protectProtoStr) or '#ifndef'
#     to require opt-out (#ifndef protectProtoStr). Otherwise
#     set to None.
#   protectProtoStr - #ifdef/#ifndef symbol to use around prototype
#     declarations, if protectProto is set
#   apicall - string to use for the function declaration prefix,
#     such as APICALL on Windows.
#   apientry - string to use for the calling convention macro,
#     in typedefs, such as APIENTRY.
#   apientryp - string to use for the calling convention macro
#     in function pointer typedefs, such as APIENTRYP.
#   indentFuncProto - True if prototype declarations should put each
#     parameter on a separate line
#   indentFuncPointer - True if typedefed function pointers should put each
#     parameter on a separate line
#   alignFuncParam - if nonzero and parameters are being put on a
#     separate line, align parameter names at the specified column
class VulkanApiGeneratorOptions(GeneratorOptions):
    def __init__(self,
                 filename = None,
                 directory = '.',
                 apiname = None,
                 profile = None,
                 versions = '.*',
                 emitversions = '.*',
                 defaultExtensions = None,
                 addExtensions = None,
                 removeExtensions = None,
                 sortProcedure = regSortFeatures,
                 prefixText = "",
                 genFuncPointers = True,
                 protectFile = True,
                 protectFeature = True,
                 protectProto = None,
                 protectProtoStr = None,
                 apicall = '',
                 apientry = '',
                 apientryp = '',
                 indentFuncProto = True,
                 indentFuncPointer = False,
                 alignFuncParam = 0):
        GeneratorOptions.__init__(self, filename, directory, apiname, profile,
                                  versions, emitversions, defaultExtensions,
                                  addExtensions, removeExtensions, sortProcedure)
        self.prefixText      = prefixText
        self.genFuncPointers = genFuncPointers
        self.prefixText      = None
        self.protectFile     = protectFile
        self.protectFeature  = protectFeature
        self.protectProto    = protectProto
        self.protectProtoStr = protectProtoStr
        self.apicall         = apicall
        self.apientry        = apientry
        self.apientryp       = apientryp
        self.indentFuncProto = indentFuncProto
        self.indentFuncPointer = indentFuncPointer
        self.alignFuncParam  = alignFuncParam

# VulkanApiOutputGenerator - subclass of OutputGenerator.
# Generates Vulkan API data for helper functions
#
# ---- methods ----
# VulkanApiOutputGenerator(errFile, warnFile, diagFile) - args as for
#   OutputGenerator. Defines additional internal state.
# ---- methods overriding base class ----
# beginFile(genOpts)
# endFile()
# beginFeature(interface, emit)
# endFeature()
# genGroup(groupinfo,name)
# genEnum(enuminfo, name)
# genCmd(cmdinfo)
class VulkanApiOutputGenerator(OutputGenerator):
    """Generate ParamChecker code based on XML element attributes"""
    # This is an ordered list of sections in the header file.
    ALL_SECTIONS = ['command']
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        self.INDENT_SPACES = 4
        # Commands to ignore
        self.blacklist = []
        # Header version
        self.headerVersion = None
        # Internal state - accumulators for different inner block text
        self.sections = dict([(section, []) for section in self.ALL_SECTIONS])
        self.structNames = []                 # List of Vulkan struct typenames
        self.stypes = []                      # Values from the VkStructureType enumeration
        self.commands = []                    # List of CommandData records for all Vulkan commands
        self.structMembers = []               # List of StructMemberData records for all Vulkan structs
        self.dispatchable_objects = []        # List of all dispatchable objects
        self.non_dispatchable_objects = []    # List of all non-dispatchable objects
        self.feature_objects = []             # List of objects defined for this particular feature
        self.feature_protos = ''              # String containing prototype definitions for a particular feature
        self.feature_names = []               # List of feature names, tracked across begin/end feature calls
        # Named tuples to store struct and command data
        self.StructType = namedtuple('StructType', ['name', 'value'])
        self.CommandParam = namedtuple('CommandParam', ['type', 'name', 'cdecl'])
        self.CommandData = namedtuple('CommandData', ['name', 'params', 'cdecl'])
        self.StructMemberData = namedtuple('StructMemberData', ['name', 'members'])
    #
    # Get the category of a type
    def getTypeCategory(self, typename):
        types = self.registry.tree.findall("types/type")
        for elem in types:
            if (elem.find("name") is not None and elem.find('name').text == typename) or elem.attrib.get('name') == typename:
                return elem.attrib.get('category')
    #
    # Check if a parent object is dispatchable or not
    def isHandleTypeNonDispatchable(self, handletype):
        handle = self.registry.tree.find("types/type/[name='" + handletype + "'][@category='handle']")
        if handle is not None and handle.find('type').text == 'VK_DEFINE_NON_DISPATCHABLE_HANDLE':
            return True
        else:
            return False
    #
    # Called once at the beginning of each run
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        # User-supplied prefix text, if any (list of strings)
        if (genOpts.prefixText):
            for s in genOpts.prefixText:
                write(s, file=self.outFile)
        # File Comment
        file_comment = '"""Vulkan API description -- THIS FILE IS GENERATED - DO NOT EDIT"""\n'
        file_comment += '# See vulkan_api_generator.py for modifications\n'
        write(file_comment, file=self.outFile)
        # Copyright Notice
        copyright = ''
        copyright += '# Copyright (c) 2015-2016 The Khronos Group Inc.\n'
        copyright += '# Copyright (c) 2015-2016 Valve Corporation'
        copyright += '# Copyright (c) 2015-2016 LunarG, Inc.'
        copyright += '# Copyright (c) 2015-2016 Google Inc.\n'
        copyright += '#\n'
        copyright += '# Licensed under the Apache License, Version 2.0 (the "License");\n'
        copyright += '# you may not use this file except in compliance with the License.\n'
        copyright += '# You may obtain a copy of the License at\n'
        copyright += '#\n'
        copyright += '#     http://www.apache.org/licenses/LICENSE-2.0 \n'
        copyright += '#\n'
        copyright += '# Unless required by applicable law or agreed to in writing, software\n'
        copyright += '# distributed under the License is distributed on an "AS IS" BASIS,\n'
        copyright += '# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n'
        copyright += '# See the License for the specific language governing permissions and\n'
        copyright += '# limitations under the License.\n'
        copyright += '#\n'
        copyright += '# Author: Chia-I Wu <olv@lunarg.com>\n'
        copyright += '# Author: Jon Ashburn <jon@lunarg.com>\n'
        copyright += '# Author: Courtney Goeltzenleuchter <courtney@LunarG.com>\n'
        copyright += '# Author: Tobin Ehlis <tobin@lunarg.com>\n'
        copyright += '# Author: Tony Barbour <tony@LunarG.com>\n'
        copyright += '# Author: Gwan-gyeong Mun <kk.moon@samsung.com>\n'
        copyright += '# Author: Mark Lobodzinski <mark@lunarg.com>\n'
        copyright += '#\n'
        write(copyright, file=self.outFile)
        # Python Class Definitions
        param_class = ''
        param_class += 'class Param(object):\n'
        param_class += '    """Function parameter"""\n'
        param_class += '\n'
        param_class += '    def __init__(self, ty, name):\n'
        param_class += '        self.ty = ty\n'
        param_class += '        self.name = name\n'
        param_class += '\n'
        proto_class = ''
        proto_class += 'class Proto(object):\n'
        proto_class += '    """Function prototype"""\n'
        proto_class += '\n'
        proto_class += '    def __init__(self, ret, name, params=[]):\n'
        proto_class += '        # Prototype has only a param\n'
        proto_class += '        if not isinstance(params, list):\n'
        proto_class += '            params = [params]\n'
        proto_class += '\n'
        proto_class += '        self.ret = ret\n'
        proto_class += '        self.name = name\n'
        proto_class += '        self.params = params\n'
        proto_class += '\n'
        proto_class += '    def c_params(self, need_type=True, need_name=True):\n'
        proto_class += '        """Return the parameter list in C."""\n'
        proto_class += '        if self.params and (need_type or need_name):\n'
        proto_class += '            if need_type and need_name:\n'
        proto_class += '                return ", ".join([param.c() for param in self.params])\n'
        proto_class += '            elif need_type:\n'
        proto_class += '                return ", ".join([param.ty for param in self.params])\n'
        proto_class += '            else:\n'
        proto_class += '                return ", ".join([param.name for param in self.params])\n'
        proto_class += '        else:\n'
        proto_class += '            return "void" if need_type else ""\n'
        proto_class += '\n'
        proto_class += '    def c_decl(self, name, attr="", typed=False, need_param_names=True):\n'
        proto_class += '        """Return a named declaration in C."""\n'
        proto_class += '        if typed:\n'
        proto_class += '            return "%s (%s*%s)(%s)" % (\n'
        proto_class += '                self.ret,\n'
        proto_class += '                attr + "_PTR " if attr else "",\n'
        proto_class += '                name,\n'
        proto_class += '                self.c_params(need_name=need_param_names))\n'
        proto_class += '        else:\n'
        proto_class += '            return "%s%s %s%s(%s)" % (\n'
        proto_class += '                attr + "_ATTR " if attr else "",\n'
        proto_class += '                self.ret,\n'
        proto_class += '                attr + "_CALL " if attr else "",\n'
        proto_class += '                name,\n'
        proto_class += '                self.c_params(need_name=need_param_names))\n'
        proto_class += '\n'
        proto_class += '    def c_pretty_decl(self, name, attr=""):\n'
        proto_class += '        """Return a named declaration in C, with vulkan.h formatting."""\n'
        proto_class += '        plist = []\n'
        proto_class += '        for param in self.params:\n'
        proto_class += '            idx = param.ty.find("[")\n'
        proto_class += '            if idx < 0:\n'
        proto_class += '                idx = len(param.ty)\n'
        proto_class += '            pad = 44 - idx\n'
        proto_class += '            if pad <= 0:\n'
        proto_class += '                pad = 1\n'
        proto_class += '\n'
        proto_class += '            plist.append("    %s%s%s%s" % (param.ty[:idx], " " * pad, param.name, param.ty[idx:]))\n'
        proto_class += '\n'
        proto_class += r'        return "%s%s %s%s(\n%s)" % ('
        proto_class += '\n'
        proto_class += '                attr + "_ATTR " if attr else "",\n'
        proto_class += '                self.ret,\n'
        proto_class += '                attr + "_CALL " if attr else "",\n'
        proto_class += '                name,\n'
        proto_class += r'                ",\n".join(plist))'
        proto_class += '\n\n'
        proto_class += '    def c_func(self, prefix="", attr=""):\n'
        proto_class += '        """Return the prototype in C."""\n'
        proto_class += '        return self.c_decl(prefix + self.name, attr=attr, typed=False)\n'
        proto_class += '\n'
        proto_class += '    def c_call(self):\n'
        proto_class += '        """Return a call to the prototype in C."""\n'
        proto_class += '        return "%s(%s)" % (self.name, self.c_params(need_type=False))\n'
        proto_class += '\n'
        proto_class += '    def object_in_params(self):\n'
        proto_class += '        """Return the params that are simple VK objects and are inputs."""\n'
        proto_class += '        return [param for param in self.params if param.ty in objects]\n'
        proto_class += '\n'
        proto_class += '    def __repr__(self):\n'
        proto_class += '        param_strs = []\n'
        proto_class += '        for param in self.params:\n'
        proto_class += '            param_strs.append(str(param))\n'
        proto_class += r'        param_str = "    [%s]" % (",\n     ".join(param_strs))'
        proto_class += '\n\n'
        proto_class += r'        return "Proto(\"%s\", \"%s\",\n%s)" % (self.ret, self.name, param_str)'
        proto_class += '\n'
        extension_class = ''
        extension_class += 'class Extension(object):\n'
        extension_class += '    def __init__(self, name, headers, objects, protos, ifdef = None):\n'
        extension_class += '        self.name = name\n'
        extension_class += '        self.headers = headers\n'
        extension_class += '        self.objects = objects\n'
        extension_class += '        self.protos = protos\n'
        extension_class += '        self.ifdef = ifdef\n'
        extension_class += '\n'
        # Write to file
        write(param_class, file=self.outFile)
        write(proto_class, file=self.outFile)
        write(extension_class, file=self.outFile)

    def endFile(self):
        # Output dispatch list
        if self.dispatchable_objects:
            dispatchable_object_list = ''
            dispatchable_object_list += 'object_dispatch_list = [\n'
            for object in self.dispatchable_objects:
                dispatchable_object_list += '    "%s",\n' % object
            dispatchable_object_list += ']'
            write(dispatchable_object_list, file=self.outFile)

        # Output non-dispatch list
        if self.non_dispatchable_objects:
            non_dispatchable_object_list = ''
            non_dispatchable_object_list += 'object_non_dispatch_list = [\n'
            for object in self.non_dispatchable_objects:
                non_dispatchable_object_list += '    "%s",\n' % object
            non_dispatchable_object_list += ']'
            write(non_dispatchable_object_list, file=self.outFile)

        # Create extension lists depending on configuration
        # Modify these lists when adding new extensions!
        linux_wsi_types = ['xlib_', 'xcb_', 'wayland_', 'mir_']
        # List containing recognized platform-specific WSI extensions
        wsi_extensions = ['VK_KHR_android_surface',
                          'VK_KHR_win32_surface',
                          'VK_KHR_xlib_surface',
                          'VK_KHR_xcb_surface',
                          'VK_KHR_wayland_surface',
                          'VK_KHR_mir_surface']
        # This list contains core (vk_version_1_0) plus non-platform-specific WSI extensions
        base_extensions = ['VK_VERSION_1_0',
                           'VK_KHR_surface',
                           'VK_KHR_swapchain',
                           'VK_KHR_display_swapchain']
        # Lists that will accumulate the platform-specific wsi extensions
        win32_wsi   = []
        android_wsi = []
        linux_wsi   = []
        # Lists that will accumulate platform-specific non-WSI extensions
        win32_only   = []
        android_only = []
        linux_only   = []
        # List that will accumulate the base, or common, extensions
        common_exts = []
        # List that will accumulate the rest of the extensions
        other_exts  = []

        # Go through the list of extensions, dropping names into the correct buckets
        for ext in self.feature_names:
            # Pull out windows-specific extensions
            if 'win32' in ext:
                if ext in wsi_extensions:
                    win32_wsi.append(ext)
                else:
                    win32_only.append(ext)
            elif 'android' in ext:
                if ext in wsi_extensions:
                    android_wsi.append(ext)
                else:
                    android_only.append(ext)
#            elif (wsitype in ext for wsitype in linux_wsi_types):
            elif any(wsitype in ext for wsitype in linux_wsi_types):
                if ext in wsi_extensions:
                    linux_wsi.append(ext)
                else:
                    linux_only.append(ext)
            elif ext in base_extensions:
                common_exts.append(ext)
            else:
                other_exts.append(ext)

        ext_handling = ''
        ext_handling += '#\n'
        ext_handling += '# Build lists of extensions for use by other codegen utilites\n'
        ext_handling += 'import sys\n'
        ext_handling += '\n'
        ext_handling += "# Set up platform-specific display servers\n"
        ext_handling += "android_display_servers = ['Android']\n"
        ext_handling += "linux_display_servers   = ['Xcb', 'Xlib', 'Wayland', 'Mir', 'Display']\n"
        ext_handling += "win32_display_servers   = ['Win32']\n"
        ext_handling += "\n"
        # Display server lists
        ext_handling += "# Define platform-specific WSI extensions\n"
        ext_handling += "android_wsi_exts = ["
        if android_wsi:
            for item in android_wsi:
                ext_handling += "%s, " % item
        ext_handling += "]\n"
        ext_handling += "linux_wsi_exts   = ["
        if linux_wsi:
            for item in linux_wsi:
                ext_handling += "%s, " % item
        ext_handling += "]\n"
        ext_handling += "win32_wsi_exts   = ["
        if win32_wsi:
            for item in win32_wsi:
                ext_handling += "%s, " % item
        ext_handling += "]\n"
        ext_handling += "\n"
        # Platform-only lists
        ext_handling += "# Define non-WSI platform-specific extensions\n"
        ext_handling += "android_only_exts = ["
        if android_only:
            for item in android_only:
                ext_handling += "%s, " % item
        ext_handling += "]\n"
        ext_handling += "linux_only_exts   = ["
        if linux_only:
            for item in linux_only:
                ext_handling += "%s, " % item
        ext_handling += "]\n"
        ext_handling += "win32_only_exts   = ["
        if win32_only:
            for item in win32_only:
                ext_handling += "%s, " % item
        ext_handling += "]\n"
        ext_handling += "\n"
        # Output list of extensions that go in ALL lists
        ext_handling += "# Define extensions common to all configurations\n"
        ext_handling += "common_exts = ["
        for item in common_exts:
            ext_handling += "%s, " % item
        ext_handling += "]\n"
        ext_handling += "\n"
        ext_handling += "# Define extensions not exported by the loader\n"
        ext_handling += "non_exported_exts = [\n"
        for item in other_exts:
            ext_handling += "    %s,\n" % item
        ext_handling += "    ]\n"
        ext_handling += "\n"
        # And build final lists depending on parameters to calling script
        ext_handling += "extensions = common_exts\n"
        ext_handling += "extensions_all = non_exported_exts\n"
        ext_handling += "\n"
        ext_handling += "if sys.argv[1] in win32_display_servers:\n"
        ext_handling += "    extensions += win32_wsi_exts\n"
        ext_handling += "    extensions_all += extensions + win32_only_exts\n"
        ext_handling += "elif sys.argv[1] in linux_display_servers:\n"
        ext_handling += "    extensions += linux_wsi_exts\n"
        ext_handling += "    extensions_all += extensions + linux_only_exts\n"
        ext_handling += "elif sys.argv[1] in android_display_servers:\n"
        ext_handling += "    extensions += android_wsi_exts\n"
        ext_handling += "    extensions_all += extensions + android_only_exts\n"
        ext_handling += "else:\n"
        ext_handling += "    extensions += win32_wsi_exts + linux_wsi_exts + android_wsi_exts\n"
        ext_handling += "    extensions_all += extensions + win32_only_exts + linux_only_exts + android_only_exts\n"
        write(ext_handling, file=self.outFile)
        # Create Vulkan API data description structures
        structs = ''
        structs += 'object_type_list = object_dispatch_list + object_non_dispatch_list\n'
        structs += '\n'
        structs += 'headers = []\n'
        structs += 'objects = []\n'
        structs += 'protos = []\n'
        structs += 'for ext in extensions:\n'
        structs += '    headers.extend(ext.headers)\n'
        structs += '    objects.extend(ext.objects)\n'
        structs += '    protos.extend(ext.protos)\n'
        structs += '\n'
        structs += 'proto_names = [proto.name for proto in protos]\n'
        structs += '\n'
        structs += 'headers_all = []\n'
        structs += 'objects_all = []\n'
        structs += 'protos_all = []\n'
        structs += 'for ext in extensions_all:\n'
        structs += '    headers_all.extend(ext.headers)\n'
        structs += '    objects_all.extend(ext.objects)\n'
        structs += '    protos_all.extend(ext.protos)\n'
        structs += '\n'
        structs += 'proto_all_names = [proto.name for proto in protos_all]\n'
        write(structs, file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFile(self)
    #
    # beginFeature is called before any commands in a given extension are processed
    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)
        # C-specific
        # Accumulate includes, defines, types, enums, function pointer typedefs,
        # end function prototypes separately for this feature. They're only
        # printed in endFeature().
        self.headerVersion = None
        self.sections = dict([(section, []) for section in self.ALL_SECTIONS])
        self.structNames = []
        self.stypes = []
        self.feature_objects = []
        self.feature_protos = ''
        # Create feature extension class
        feature = ''
        feature += '%s = Extension(\n' % self.featureName
        feature += '    name="%s",\n' % self.featureName
        feature += '    headers=["vulkan/vulkan.h"],'
        write(feature, file=self.outFile)
    #
    # endFeature gets called at the after the commands for a particular extension have been processed
    def endFeature(self):
        # Output protection (ifdef) if it exists
        ifdefs = ''
        if (self.featureExtraProtect != None):
            ifdefs += '    ifdef="%s",' % self.featureExtraProtect
        else:
            ifdefs += '    ifdef="",'
        write(ifdefs, file=self.outFile)
        # Output extension objects[] list
        object_list = ''
        if self.feature_objects:
            object_list += '    objects=[\n'
            for object in self.feature_objects:
                object_list += '        "%s",\n' % object
            object_list += '    ],'
        else:
            object_list += '    objects=[],'
        write(object_list, file=self.outFile)
        # Output extension protos[] list
        protos = self.feature_protos
        if protos:
            protos = '    protos=[\n%s' % self.feature_protos
            protos += '    ],\n'
        else:
            protos = '    protos=[],'
        write(protos, file=self.outFile)
        self.feature_names.append(self.featureName)
        write(')\n', file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFeature(self)
    #
    # Capture command parameter info to be used for param
    # check code generation.
    def genCmd(self, cmdinfo, name):
        OutputGenerator.genCmd(self, cmdinfo, name)
        params = cmdinfo.elem.findall('param')
        # Get param info
        paramsInfo = []
        for param in params:
            paramInfo = self.getTypeNameTuple(param)
            cdecl = self.makeCParamDecl(param, 0)
            paramsInfo.append(self.CommandParam(type=paramInfo[0], name=paramInfo[1], cdecl=cdecl))
        self.commands.append(self.CommandData(name=name, params=paramsInfo, cdecl=self.makeCDecls(cmdinfo.elem)[0]))
        # Remove 'vk' from proto name before output
        proto_name = cmdinfo.elem.find('proto/name').text
        proto_name = proto_name[2:]
        self.feature_protos += '        Proto("%s", "%s",\n' % (cmdinfo.elem.find('proto/type').text, proto_name)
        self.feature_protos += '            [\n'
        # Collect and format API parameter data
        for param in params:
            # Get type and name of param
            info = self.getTypeNameTuple(param)
            type = info[0]
            name = info[1]
            self.feature_protos += '             Param("%s", "%s"),\n' %  (type, name)
            if self.getTypeCategory(type) == 'handle':
                if type not in self.feature_objects:
                    if type not in self.dispatchable_objects and type not in self.non_dispatchable_objects:
                        self.feature_objects.append(type)
                if self.isHandleTypeNonDispatchable(type) == True:
                    if type not in self.non_dispatchable_objects:
                        self.non_dispatchable_objects.append(type)
                else:
                    if type not in self.dispatchable_objects:
                        self.dispatchable_objects.append(type)
        self.feature_protos += '            ]),\n'
    #
    # Retrieve the type and name for a parameter
    def getTypeNameTuple(self, param):
        type = ''
        name = ''
        for elem in param:
            if elem.tag == 'type':
                type = noneStr(elem.text)
            elif elem.tag == 'name':
                name = noneStr(elem.text)
        return (type, name)
