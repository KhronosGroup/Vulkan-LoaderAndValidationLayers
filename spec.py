#!/usr/bin/python3 -i

import sys
import xml.etree.ElementTree as etree
#import codecs
import collections

spec_filename = "vkspec.html" # can override w/ '-spec <filename>' option
out_filename = "vk_validation_error_messages.h" # can override w/ '-out <filename>' option
db_filename = "vk_validation_error_database.txt" # can override w/ '-gendb <filename>' option
gen_db = False # set to True when '-gendb <filename>' option provided
spec_compare = False # set to True with '-compare <db_filename>' option
spec_url = "https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html"
error_msg_prefix = "For more information refer to Vulkan Spec Section"
ns = {'ns': 'http://www.w3.org/1999/xhtml'}

def printHelp():
    print "Usage: python spec.py [-spec <specfile.html>] [-out <headerfile.h>] [-gendb <databasefile.txt>] [-compare <databasefile.txt>] [-help]"
    print "\n Default script behavior is to parse the specfile and generate a header of unique error enums and corresponding error messages based on the specfile.\n"
    print "  Default specfile is vkspec.html"
    print "  Default headerfile vk_validation_error_messages.h"
    print "  Default databasefile is vk_validation_error_database.txt"
    print "\nIf '-gendb' option is specified then a database file is generated that stores the list of enums and their error messages"
    print "\nIf '-compare' option is specified then the given database file will be read in as the baseline for generating the new specfile"

class Specification:
    def __init__(self):
        self.tree   = None
        self.val_error_dict = collections.OrderedDict() # string for enum is key that references text for output message
        self.delimiter = '~^~' # delimiter for db file
    def loadFile(self, file):
        """Load an API registry XML file into a Registry object and parse it"""
        self.tree = etree.parse(file)
        #self.tree.write("tree_output.xhtml")
        #self.tree = etree.parse("tree_output.xhtml")
        self.parseTree()
    def updateDict(self, updated_dict):
        """Assign internal dict to use updated_dict"""
        self.val_error_dict = updated_dict
    def parseTree(self):
        """Parse the registry Element, once created"""
        print "Parsing spec file..."
        valid_usage = False # are we under a valid usage branch?
        unique_enum_id = 0
        self.root = self.tree.getroot()
        #print "ROOT: %s" % self.root
        prev_heading = '' # Last seen section heading or sub-heading
        prev_link = '' # Last seen link id within the spec
        for tag in self.root.iter(): # iterate down tree
            # Grab most recent section heading and link
            if tag.tag in ['{http://www.w3.org/1999/xhtml}h2', '{http://www.w3.org/1999/xhtml}h3']:
                if tag.get('class') != 'title':
                    continue
                #print "Found heading %s" % (tag.tag)
                prev_heading = "".join(tag.itertext())
                # Insert a space between heading number & title
                sh_list = prev_heading.rsplit('.', 1)
                prev_heading = '. '.join(sh_list)
                prev_link = tag[0].get('id')
                #print "Set prev_heading %s to have link of %s" % (prev_heading.encode("ascii", "ignore"), prev_link.encode("ascii", "ignore"))
            elif tag.tag == '{http://www.w3.org/1999/xhtml}a': # grab any intermediate links
                if tag.get('id') != None:
                    prev_link = tag.get('id')
                    #print "Updated prev link to %s" % (prev_link)
            elif tag.tag == '{http://www.w3.org/1999/xhtml}strong': # identify valid usage sections
                if None != tag.text and 'Valid Usage' in tag.text:
                    valid_usage = True
                else:
                    valid_usage = False
            elif tag.tag == '{http://www.w3.org/1999/xhtml}li' and valid_usage: # grab actual valid usage requirements
                error_msg_str = "%s '%s' which states '%s' (%s#%s)" % (error_msg_prefix, prev_heading, "".join(tag.itertext()).replace('\n', ''), spec_url, prev_link)
                enum_str = "VALIDATION_ERROR_%d" % (unique_enum_id)
                self.val_error_dict[enum_str.encode("ascii", "ignore")] = error_msg_str.encode("ascii", "ignore")
                unique_enum_id = unique_enum_id + 1
                #print "dict contents: %s:" % (self.val_error_dict)
                #print "Added enum to dict: %s" % (enum_str.encode("ascii", "ignore"))
        #print "Validation Error Dict has a total of %d unique errors and contents are:\n%s" % (unique_enum_id, self.val_error_dict)
    def genHeader(self, header_file):
        """Generate a header file based on the contents of a parsed spec"""
        print "Generating header..."
        file_contents = []
        file_contents.append('// Copyright here')
        file_contents.append('#pragma once')
        file_contents.append('// Comment on enum declaration')
        enum_decl = ['enum UNIQUE_VALIDATION_ERROR_CODE {']
        error_string_map = ['std::unordered_map<int, char const *const> validation_error_map{']
        for enum in self.val_error_dict:
            enum_decl.append('    %s = %s,' % (enum, enum.split('_')[-1]))
            error_string_map.append('    {%s, "%s"},' % (enum, self.val_error_dict[enum]))
        enum_decl.append('};')
        error_string_map.append('};')
        file_contents.extend(enum_decl)
        file_contents.append('// Comment on enum error map')
        file_contents.extend(error_string_map)
        #print "File contents: %s" % (file_contents)
        with open(header_file, "w") as outfile:
            outfile.write("\n".join(file_contents))
    def analyze(self):
        """Print out some stats on the valid usage dict"""
        # Create dict for # of occurences of identical strings
        str_count_dict = {}
        unique_id_count = 0
        for enum in self.val_error_dict:
            err_str = self.val_error_dict[enum]
            if err_str in str_count_dict:
                #print "Found repeat error string"
                str_count_dict[err_str] = str_count_dict[err_str] + 1
            else:
                str_count_dict[err_str] = 1
            unique_id_count = unique_id_count + 1
        print "Processed %d unique_ids" % (unique_id_count)
        repeat_string = 0
        for es in str_count_dict:
            if str_count_dict[es] > 1:
                repeat_string = repeat_string + 1
                #print "String '%s' repeated %d times" % (es, repeat_string)
        print "Found %d repeat strings" % (repeat_string)
    def genDB(self, db_file):
        """Generate a database of check_enum, check_coded?, testname, error_string"""
        db_lines = []
        for enum in self.val_error_dict:
            #print "delimiter: %s, id: %s, str: %s" % (self.delimiter, enum, self.val_error_dict[enum]) 
            db_lines.append("%s%sN%sNone%s%s" % (enum, self.delimiter, self.delimiter, self.delimiter, self.val_error_dict[enum]))
        with open(db_file, "w") as outfile:
            outfile.write("\n".join(db_lines))
    def readDB(self, db_file):
        """Read a db file into a dict"""
        db_dict = {}
        max_id = 0
        with open(db_file, "r") as infile:
            for line in infile:
                if line.startswith('#'):
                    continue
                line = line.strip()
                db_line = line.split(self.delimiter)
                db_dict[db_line[0]] = db_line[3]
                unique_id = int(db_line[0].split('_')[-1])
                if unique_id > max_id:
                    max_id = unique_id
        return (db_dict, max_id)
    # Compare unique ids from original database to data generated from updated spec
    # 1. If a new id and error code exactly match original, great
    # 2. If new id is not in original, but exact error code is, need to use original error code
    # 3. If new id and new error are not in original, make sure new id picks up from end of original list
    # 4. If new id in original, but error strings don't match then:
    #   4a. If error string has exact match in original, update new to use original
    #   4b. If error string not in original, may be updated error message, manually address
    def compareDB(self, orig_db_dict, max_id):
        """Compare orig database dict to new dict, report out findings, and return potential new dict for parsed spec"""
        # First create reverse dicts of err_strings to IDs
        next_id = max_id + 1
        orig_err_to_id_dict = {}
        # Create an updated dict in-place that will be assigned to self.val_error_dict when done
        updated_val_error_dict = collections.OrderedDict()
        for enum in orig_db_dict:
            orig_err_to_id_dict[orig_db_dict[enum]] = enum
        new_err_to_id_dict = {}
        for enum in self.val_error_dict:
            new_err_to_id_dict[self.val_error_dict[enum]] = enum
        ids_parsed = 0
        # Now parse through new dict and figure out what to do with non-matching things
        for enum in self.val_error_dict:
            ids_parsed = ids_parsed + 1
            enum_list = enum.split('_') # grab sections of enum for use below
            if enum in orig_db_dict:
                if self.val_error_dict[enum] == orig_db_dict[enum]:
                    print "Exact match for enum %s" % (enum)
                    # Nothing to see here
                    if enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (enum)
                    updated_val_error_dict[enum] = self.val_error_dict[enum]
                elif self.val_error_dict[enum] in orig_err_to_id_dict:
                    # Same value w/ different error id, need to anchor to original id
                    print "Need to switch new id %s to original id %s" % (enum, orig_err_to_id_dict[self.val_error_dict[enum]])
                    # Update id at end of new enum to be same id from original enum
                    enum_list[-1] = orig_err_to_id_dict[self.val_error_dict[enum]].split('_')[-1]
                    new_enum = "_".join(enum_list)
                    if new_enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (new_enum)
                    updated_val_error_dict[new_enum] = self.val_error_dict[enum]
                else:
                    # Completely new id, need to pick it up from end of original unique ids
                    enum_list[-1] = str(next_id)
                    new_enum = "_".join(enum_list)
                    next_id = next_id + 1
                    print "MANUALLY VERIFY: Updated new enum %s to be unique %s. Make sure new error msg is actually unique and not just changed" % (enum, new_enum)
                    print "   New error string: %s" % (self.val_error_dict[enum])
                    if new_enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (new_enum)
                    updated_val_error_dict[new_enum] = self.val_error_dict[enum]
            else: # new enum is not in orig db
                if self.val_error_dict[enum] in orig_err_to_id_dict:
                    print "New enum %s not in orig dict, but exact error message matches original unique id %s" % (enum, orig_err_to_id_dict[self.val_error_dict[enum]])
                    # Update new unique_id to use original
                    enum_list[-1] = orig_err_to_id_dict[self.val_error_dict[enum]].split('_')[-1]
                    new_enum = "_".join(enum_list)
                    if new_enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (new_enum)
                    updated_val_error_dict[new_enum] = self.val_error_dict[enum]
                else:
                    enum_list[-1] = str(next_id)
                    new_enum = "_".join(enum_list)
                    next_id = next_id + 1
                    print "Completely new id and error code, update new id from %s to unique %s" % (enum, new_enum)
                    if new_enum in updated_val_error_dict:
                        print "ERROR: About to overwrite entry for %s" % (new_enum)
                    updated_val_error_dict[new_enum] = self.val_error_dict[enum]
        # Assign parsed dict to be the udpated dict based on db compare
        print "In compareDB parsed %d entries" % (ids_parsed)
        return updated_val_error_dict
    def validateUpdateDict(self, update_dict):
        """Compare original dict vs. update dict and make sure that all of the checks are still there"""
        # Currently just make sure that the same # of checks as the original checks are there
        #orig_ids = {}
        orig_id_count = len(self.val_error_dict)
        #update_ids = {}
        update_id_count = len(update_dict)
        if orig_id_count != update_id_count:
            print "Original dict had %d unique_ids, but updated dict has %d!" % (orig_id_count, update_id_count)
            return False
        print "Original dict and updated dict both have %d unique_ids. Great." % (orig_id_count)
        return True
        # TODO : include some more analysis

if __name__ == "__main__":
    i = 1
    while (i < len(sys.argv)):
        arg = sys.argv[i]
        i = i + 1
        if (arg == '-spec'):
            spec_filename = sys.argv[i]
            i = i + 1
        elif (arg == '-out'):
            out_filename = sys.argv[i]
            i = i + 1
        elif (arg == '-gendb'):
            db_filename = sys.argv[i]
            gen_db = True
            i = i + 1
        elif (arg == '-compare'):
            db_filename = sys.argv[i]
            spec_compare = True
            i = i + 1
        elif (arg in ['-help', '-h']):
            printHelp()
            sys.exit()
    print "Using spec file (-spec) to '%s'" % (spec_filename)
    print "Writing out file (-out) to '%s'" % (out_filename)
    spec = Specification()
    spec.loadFile(spec_filename)
    #spec.parseTree()
    #spec.genHeader(out_filename)
    spec.analyze()
    if (spec_compare):
        # Read in old spec info from db file
        (orig_db_dict, max_id) = spec.readDB(db_filename)
        # New spec data should already be read into self.val_error_dict
        updated_dict = spec.compareDB(orig_db_dict, max_id)
        update_valid = spec.validateUpdateDict(updated_dict)
        if update_valid:
            spec.updateDict(updated_dict)
        else:
            sys.exit()
    if (gen_db):
        spec.genDB(db_filename)
    spec.genHeader(out_filename)


        # <div class="sidebar">
        #   <div class="titlepage">
        #     <div>
        #       <div>
        #         <p class="title">
        #           <strong>Valid Usage</strong> # When we get to this guy, we know we're under interesting sidebar
        #         </p>
        #       </div>
        #     </div>
        #   </div>
        # <div class="itemizedlist">
        #   <ul class="itemizedlist" style="list-style-type: disc; ">
        #     <li class="listitem">
        #       <em class="parameter">
        #         <code>device</code>
        #       </em>
        #       <span class="normative">must</span> be a valid
        #       <code class="code">VkDevice</code> handle
        #     </li>
        #     <li class="listitem">
        #       <em class="parameter">
        #         <code>commandPool</code>
        #       </em>
        #       <span class="normative">must</span> be a valid
        #       <code class="code">VkCommandPool</code> handle
        #     </li>
        #     <li class="listitem">
        #       <em class="parameter">
        #         <code>flags</code>
        #       </em>
        #       <span class="normative">must</span> be a valid combination of
        #       <code class="code">
        #         <a class="link" href="#VkCommandPoolResetFlagBits">VkCommandPoolResetFlagBits</a>
        #       </code> values
        #     </li>
        #     <li class="listitem">
        #       <em class="parameter">
        #         <code>commandPool</code>
        #       </em>
        #       <span class="normative">must</span> have been created, allocated, or retrieved from
        #       <em class="parameter">
        #         <code>device</code>
        #       </em>
        #     </li>
        #     <li class="listitem">All
        #       <code class="code">VkCommandBuffer</code>
        #       objects allocated from
        #       <em class="parameter">
        #         <code>commandPool</code>
        #       </em>
        #       <span class="normative">must</span> not currently be pending execution
        #     </li>
        #   </ul>
        # </div>
        # </div>
# <div class="sidebar">
#   <div class="titlepage">
#     <div>
#       <div>
#         <p class="title">
#           <strong>Valid Usage</strong>
#         </p>
#       </div>
#     </div>
#   </div>
#   <div class="itemizedlist">
#     <ul class="itemizedlist" style="list-style-type: disc; ">
#       <li class="listitem">The <em class="parameter"><code>queueFamilyIndex</code></em> member of any given element of <em class="parameter"><code>pQueueCreateInfos</code></em> <span class="normative">must</span> be unique within <em class="parameter"><code>pQueueCreateInfos</code></em>
#       </li>
#     </ul>
#   </div>
# </div>
# <div class="sidebar">
#   <div class="titlepage">
#     <div>
#       <div>
#         <p class="title">
#           <strong>Valid Usage (Implicit)</strong>
#         </p>
#       </div>
#     </div>
#   </div>
#   <div class="itemizedlist"><ul class="itemizedlist" style="list-style-type: disc; "><li class="listitem">
#<em class="parameter"><code>sType</code></em> <span class="normative">must</span> be <code class="code">VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO</code>
#</li><li class="listitem">
#<em class="parameter"><code>pNext</code></em> <span class="normative">must</span> be <code class="literal">NULL</code>
#</li><li class="listitem">
#<em class="parameter"><code>flags</code></em> <span class="normative">must</span> be <code class="literal">0</code>
#</li><li class="listitem">
#<em class="parameter"><code>pQueueCreateInfos</code></em> <span class="normative">must</span> be a pointer to an array of <em class="parameter"><code>queueCreateInfoCount</code></em> valid <code class="code">VkDeviceQueueCreateInfo</code> structures
#</li>