"""
Test setting breakpoints using a scripted resolver
"""

from __future__ import print_function


import os
import time
import re
import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class TestScriptedResolver(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    NO_DEBUG_INFO_TESTCASE = True

    def test_scripted_resolver(self):
        """Use a scripted resolver to set a by symbol name breakpoint"""
        self.build()
        self.do_test()

    def test_search_depths(self):
        """ Make sure we are called at the right depths depending on what we return
            from __get_depth__"""
        self.build()
        self.do_test_depths()

    def test_command_line(self):
        """ Make sure we are called at the right depths depending on what we return
            from __get_depth__"""
        self.build()
        self.do_test_cli()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)

    def make_target_and_import(self):
        target = lldbutil.run_to_breakpoint_make_target(self)
        interp = self.dbg.GetCommandInterpreter()
        error = lldb.SBError()

        script_name = os.path.join(self.getSourceDir(), "resolver.py")
        source_name = os.path.join(self.getSourceDir(), "main.c")

        command = "command script import " + script_name
        result = lldb.SBCommandReturnObject()
        interp.HandleCommand(command, result)
        self.assertTrue(result.Succeeded(), "com scr imp failed: %s"%(result.GetError()))
        return target

    def make_extra_args(self):
        json_string = '{"symbol":"break_on_me", "test1": "value1"}'
        json_stream = lldb.SBStream()
        json_stream.Print(json_string)
        extra_args = lldb.SBStructuredData()
        error = extra_args.SetFromJSON(json_stream)
        self.assertTrue(error.Success(), "Error making SBStructuredData: %s"%(error.GetCString()))
        return extra_args

    def do_test(self):
        """This reads in a python file and sets a breakpoint using it."""

        target = self.make_target_and_import()
        extra_args = self.make_extra_args()

        file_list = lldb.SBFileSpecList()
        module_list = lldb.SBFileSpecList()

        # Make breakpoints with this resolver using different filters, first ones that will take:
        right = []
        # one with no file or module spec - this one should fire:
        right.append(target.BreakpointCreateFromScript("resolver.Resolver", extra_args, module_list, file_list))

        # one with the right source file and no module - should also fire:
        file_list.Append(lldb.SBFileSpec("main.c"))
        right.append(target.BreakpointCreateFromScript("resolver.Resolver", extra_args, module_list, file_list))
        # Make sure the help text shows up in the "break list" output:
        self.expect("break list", substrs=["I am a python breakpoint resolver"], msg="Help is listed in break list")

        # one with the right source file and right module - should also fire:
        module_list.Append(lldb.SBFileSpec("a.out"))
        right.append(target.BreakpointCreateFromScript("resolver.Resolver", extra_args, module_list, file_list))

        # And one with no source file but the right module:
        file_list.Clear()
        right.append(target.BreakpointCreateFromScript("resolver.Resolver", extra_args, module_list, file_list))

        # Make sure these all got locations:
        for i in range (0, len(right)):
            self.assertTrue(right[i].GetNumLocations() >= 1, "Breakpoint %d has no locations."%(i))

        # Now some ones that won't take:

        module_list.Clear()
        file_list.Clear()
        wrong = []

        # one with the wrong module - should not fire:
        module_list.Append(lldb.SBFileSpec("noSuchModule"))
        wrong.append(target.BreakpointCreateFromScript("resolver.Resolver", extra_args, module_list, file_list))

        # one with the wrong file - also should not fire:
        file_list.Clear()
        module_list.Clear()
        file_list.Append(lldb.SBFileSpec("noFileOfThisName.xxx"))
        wrong.append(target.BreakpointCreateFromScript("resolver.Resolver", extra_args, module_list, file_list))
        
        # Now make sure the CU level iteration obeys the file filters:
        file_list.Clear()
        module_list.Clear()
        file_list.Append(lldb.SBFileSpec("no_such_file.xxx"))
        wrong.append(target.BreakpointCreateFromScript("resolver.ResolverCUDepth", extra_args, module_list, file_list))

        # And the Module filters:
        file_list.Clear()
        module_list.Clear()
        module_list.Append(lldb.SBFileSpec("NoSuchModule.dylib"))
        wrong.append(target.BreakpointCreateFromScript("resolver.ResolverCUDepth", extra_args, module_list, file_list))

        # Now make sure the Function level iteration obeys the file filters:
        file_list.Clear()
        module_list.Clear()
        file_list.Append(lldb.SBFileSpec("no_such_file.xxx"))
        wrong.append(target.BreakpointCreateFromScript("resolver.ResolverFuncDepth", extra_args, module_list, file_list))

        # And the Module filters:
        file_list.Clear()
        module_list.Clear()
        module_list.Append(lldb.SBFileSpec("NoSuchModule.dylib"))
        wrong.append(target.BreakpointCreateFromScript("resolver.ResolverFuncDepth", extra_args, module_list, file_list))

        # Make sure these didn't get locations:
        for i in range(0, len(wrong)):
            self.assertEqual(wrong[i].GetNumLocations(), 0, "Breakpoint %d has locations."%(i))

        # Now run to main and ensure we hit the breakpoints we should have:

        lldbutil.run_to_breakpoint_do_run(self, target, right[0])
        
        # Test the hit counts:
        for i in range(0, len(right)):
            self.assertEqual(right[i].GetHitCount(), 1, "Breakpoint %d has the wrong hit count"%(i))

        for i in range(0, len(wrong)):
            self.assertEqual(wrong[i].GetHitCount(), 0, "Breakpoint %d has the wrong hit count"%(i))

    def do_test_depths(self):
        """This test uses a class variable in resolver.Resolver which gets set to 1 if we saw
           compile unit and 2 if we only saw modules.  If the search depth is module, you get passed just
           the modules with no comp_unit.  If the depth is comp_unit you get comp_units.  So we can use
           this to test that our callback gets called at the right depth."""

        target = self.make_target_and_import()
        extra_args = self.make_extra_args()

        file_list = lldb.SBFileSpecList()
        module_list = lldb.SBFileSpecList()
        module_list.Append(lldb.SBFileSpec("a.out"))

        # Make a breakpoint that has no __get_depth__, check that that is converted to eSearchDepthModule:
        bkpt = target.BreakpointCreateFromScript("resolver.Resolver", extra_args, module_list, file_list)
        self.assertTrue(bkpt.GetNumLocations() > 0, "Resolver got no locations.")
        self.expect("script print resolver.Resolver.got_files", substrs=["2"], msg="Was only passed modules")
        
        # Make a breakpoint that asks for modules, check that we didn't get any files:
        bkpt = target.BreakpointCreateFromScript("resolver.ResolverModuleDepth", extra_args, module_list, file_list)
        self.assertTrue(bkpt.GetNumLocations() > 0, "ResolverModuleDepth got no locations.")
        self.expect("script print resolver.Resolver.got_files", substrs=["2"], msg="Was only passed modules")
        
        # Make a breakpoint that asks for compile units, check that we didn't get any files:
        bkpt = target.BreakpointCreateFromScript("resolver.ResolverCUDepth", extra_args, module_list, file_list)
        self.assertTrue(bkpt.GetNumLocations() > 0, "ResolverCUDepth got no locations.")
        self.expect("script print resolver.Resolver.got_files", substrs=["1"], msg="Was passed compile units")

        # Make a breakpoint that returns a bad value - we should convert that to "modules" so check that:
        bkpt = target.BreakpointCreateFromScript("resolver.ResolverBadDepth", extra_args, module_list, file_list)
        self.assertTrue(bkpt.GetNumLocations() > 0, "ResolverBadDepth got no locations.")
        self.expect("script print resolver.Resolver.got_files", substrs=["2"], msg="Was only passed modules")

        # Make a breakpoint that searches at function depth:
        bkpt = target.BreakpointCreateFromScript("resolver.ResolverFuncDepth", extra_args, module_list, file_list)
        self.assertTrue(bkpt.GetNumLocations() > 0, "ResolverFuncDepth got no locations.")
        self.expect("script print resolver.Resolver.got_files", substrs=["3"], msg="Was only passed modules")
        self.expect("script print resolver.Resolver.func_list", substrs=["break_on_me", "main", "test_func"], msg="Saw all the functions")

    def do_test_cli(self):
        target = self.make_target_and_import()

        lldbutil.run_break_set_by_script(self, "resolver.Resolver", extra_options="-k symbol -v break_on_me")
