/*
 *
 * (c) Copyright 1999 - 2022 Verific Design Automation Inc.
 * All rights reserved.
 *
 * This source code belongs to Verific Design Automation Inc.
 * It is considered trade secret and confidential, and is not to be used
 * by parties who have not received written authorization
 * from Verific Design Automation Inc.
 *
 * Only authorized users are allowed to use, copy and modify
 * this software provided that the above copyright notice
 * remains in all copies of this software.
 *
 *
*/
#include <iostream>
#include "Array.h"          // Make class Array available

#include "VeriId.h"         // Definitions of all identifier definition tree nodes

#include "Message.h"        // Make message handlers available

#include "TextBasedDesignMod.h"  // Text-Based Design-Modification (TextBasedDesignMod) utility

#include "DataBase.h" // Make (hierarchical netlist) database API available
#include "veri_file.h"      // Make verilog reader available
#include "VeriModule.h"     // Definition of a VeriModule and VeriPrimitive
#include "VeriExpression.h" // Definitions of all verilog expression tree nodes
#include "VeriModuleItem.h" // Definitions of all verilog module item tree nodes
#include "VeriStatement.h"  // Make VeriCaseStatement class available
#include "VeriVisitor.h"    // For visitor patterns
#include "veri_prune.h" 

#ifdef USE_COMREAD
#include "Commands.h"
#include "ComRead.h"
#endif

#ifdef VERIFIC_NAMESPACE
using namespace Verific ;
#endif

/****************************************************************************************

                                NOTE TO THE READER:

    The following is a more advanced example to demonstrate file manipulation utility
    using APIs of 'TextBasedDesignMod' class. For a simple example, please see the other
    example application.

    * We will not change the parse tree itself, but will write out modified verilog
      of the source file using the file manipulation utility.

    * We do the following:
        1. Add a comment in the file in a specific location.
        2. Remove a particular construct from the first top level module.

    * The name of the design file and the output file are taken from the command line
      argument along with the secure directory path. If no arguments are specified,
      we default to hard-coded values.

*****************************************************************************************/

// Visit class to visit all case statements of a module
class VeriCaseStmtVisit : public VeriVisitor
{
public:
    VeriCaseStmtVisit(TextBasedDesignMod *file_obj) ;
    virtual ~VeriCaseStmtVisit() ;

    // Overwrite only the visit routine of case statement
    virtual void VERI_VISIT(VeriCaseStatement, node) ;

private:
    TextBasedDesignMod *_tbdm ;
} ;

// Visit class to visit assignment so that it can be converted
// to gate instantiation
class VeriAssignReplace : public VeriVisitor
{
public:
    VeriAssignReplace(TextBasedDesignMod *file_obj, VeriModule *mod) ;
    virtual ~VeriAssignReplace() ;

    // Overwrite only the visit routine of VeriContinuousAssign
    virtual void VERI_VISIT(VeriContinuousAssign, node) ;

private:
    TextBasedDesignMod *_tbdm ;
    VeriModule   *_mod ;
} ;

////////////////////////////////////////////////////////////////////////////
////////////              VeriCaseStmtVisit class               ////////////
////////////////////////////////////////////////////////////////////////////

VeriCaseStmtVisit::VeriCaseStmtVisit(TextBasedDesignMod *file_obj) :
    VeriVisitor(),
    _tbdm(file_obj)
{
}

VeriCaseStmtVisit::~VeriCaseStmtVisit()
{
}

void VeriCaseStmtVisit::VERI_VISIT(VeriCaseStatement, node)
{
    // Get the starting location of the case statement. We will use this information
    // to put comment before case statement
    linefile_type start_location = node.StartingLinefile() ;

    // Call InsertBefore routine of 'TextBasedDesignMod' class to insert comment before case statement
    _tbdm->InsertBefore(start_location, "// synopsys parallel_case\n") ;
}

////////////////////////////////////////////////////////////////////////////

int prune_verilog (const char *file_name, const char *out_file_name, const char *file_base, gb_constructs &gb)
{
#ifndef VERIFIC_LINEFILE_INCLUDES_COLUMNS
    Message::PrintLine("This application example requires the compile-flag VERIFIC_LINEFILE_INCLUDES_COLUMNS (located in util/VerificSystem.h) to be active in order to run!") ;
#else
    // The following operations will be done by this application on the file:
    // 1 . File specified in the first command line argument will be analyzed.
    // 2 . Handle for file modification utility (TextBasedDesignMod) will be created.
    // 3 . Using linefile information from analyzed parse tree, file modification utility
    //     will modify the design file (not the parse tree). We perform these modifications:
    //       (a). Add a comment attribute before the case statements to make them parallel.
    //       (b). Remove the last module item from the first top level module.
    // 4 . Modified file will be written in another file that can be specified as the second
    //     command line argument, if not specified, the original file is overwritten!

    // Analyze the design file(in verilog2k mode), if there is any error, don't proceed further!
#ifdef USE_COMREAD
    const char *def_args[] = {"analyze", file_name, "-verilog_2000"} ;
    int def_argc = 3 ;

    Command *mainline = new ComAnalyze() ;

    if(!mainline->ParseArgs(def_argc, def_args)) {
        mainline->Usage() ;
        delete mainline ;
        return 1 ;
    }

    if(!mainline->Process(0)) {
        mainline->Usage() ;
        delete mainline ;
        return 2 ;
    }
    delete mainline ;
#else
    if (!veri_file::Analyze(file_name, veri_file::VERILOG_2K /*v2k*/)) return 1 ;
#endif

    // Instantiate the class 'TextBasedDesignMod', which is file modification utility.
    // It takes the directory path in which we can modify the files. If no directory path
    // is specified, all files can be modified and overwritten. The directory 'src' is
    // specified as the secure directory here. So we can modify and overwrite the files only
    // under this directory tree. Any other file outside of that tree can be modified but
    // cannot be overwritten!
    TextBasedDesignMod file_base_comment(file_base) ;

    // We will perform the manipulations as said before on the first top level module of the specified file:

    // Get all the top level modules
    Array *all_top_modules = veri_file::GetTopModules() ;
    // Get a handle of the first top level module, if any.
    VeriModule *mod = (all_top_modules && all_top_modules->Size()) ? (VeriModule *)all_top_modules->GetFirst() : 0 ;
    if (!mod) {
        Message::PrintLine("Cannot find a top level module") ;
        delete all_top_modules ;
        return 3 ;
    }

    VeriModule *moduleg = (VeriModule *)all_top_modules->GetFirst();
    std::string TM = moduleg->GetName();
    std::cout << TM << " is TP" <<std::endl;
    delete all_top_modules ;

    /* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ *
     *              1.  Make case statements parallel                     *
     * \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/ */

    // Instantiate visit class object which makes case statement parallel. This visitor class
    //
    //
    // is written to visit every case statement of the module. Please look into
    // VeriCaseStmtVisit::Visit(VeriCaseStatement &node) function in this file.
    VeriCaseStmtVisit visit_obj(&file_base_comment) ;
    mod->Accept(visit_obj) ;

    /* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ *
     *                 2. Remove the last module item                     *
     * \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/ */

    // We want to remove a specific module item from the module. To remove it we should
    // know the starting location and ending location of this item in the design file.
    // We will remove the last module item.

    // Get the module item list of module.
    Array *items = mod->GetModuleItems() ;
    VeriModuleItem *module_item;
    unsigned i;
    FOREACH_ARRAY_ITEM(items, i, module_item)
    {
        if (!module_item)
            continue;
        if(module_item->IsInstantiation()) {
        	std::string mod_name = module_item->GetModuleName();
        	std::cout << "Inst name is " << mod_name << std::endl;
            std::string no_param_name;
            // reducing a correctly named parametrized module MyModule(par1=99) to MyModule
            // discuss with thierry !
            for (auto k : mod_name)
                if ('(' == k)
                    break;
                else
                    no_param_name.push_back(k);
        	for (const auto& element : gb.gb_mods) {
                std::string str = element.first;
        	    if (str == no_param_name) {
                    VeriIdDef *id ;
        	        unsigned m ;

        	        Array *insts = module_item->GetInstances();
        	        FOREACH_ARRAY_ITEM(insts, m, id) {
        	        	if (!id) cout << "NOT an ID" << endl;
        	        	else printf("Got an ID\n");
        	        	const char *inst_name = id->InstName() ;
        	        	if (id) Message::Info(id->Linefile(),"here '", inst_name, "' is the name of an instance") ;

                        VeriIdDef *formal ;
                        VeriIdDef *actual_id ;
                        VeriExpression *actual ;
                        const char *formal_name ;
                         const char *actual_name ;
        	        	VeriExpression *expr ;
        	        	unsigned k ;
        	        	Array *port_conn_arr = id->GetPortConnects() ;
        	        	FOREACH_ARRAY_ITEM(port_conn_arr, k, expr) {
                            std::cout << "ID_VERIPORTCONNECT" << std::endl;
                            formal_name = expr->NamedFormal() ;
                            formal = module_item->GetPort(formal_name) ;
                            actual = expr->GetConnection() ;
                            if (actual->GetClassId() == ID_VERICONCAT) {
                                std::cout << "got the reason" << std::endl;
                                Array *expr_arr = actual->GetExpressions();
                                unsigned i;
                                VeriExpression *pexpr;
                                FOREACH_ARRAY_ITEM(expr_arr, i, pexpr)
                                {
                                    actual_id = (pexpr) ? pexpr->FullId() : 0 ;
                                    actual_name = actual_id->Name();
                                    std::cout << "ACN : " << actual_name << "   FN : " << formal_name << std::endl;
                                }
                            } else if (actual->GetClassId() == ID_VERIINDEXEDID) {
                                VeriIndexedId *indexed_id = static_cast<VeriIndexedId*>(actual) ;
                                unsigned port_dir = indexed_id->PortExpressionDir() ;
                                unsigned port_size = indexed_id->FindSize();
                                const char *port_name = indexed_id->GetName() ; // Get port name
                                Message::Info(indexed_id->Linefile(),"here '", port_name, "' is an indexed id in a port declaration") ;
                                std::cout << "it is indexed and dir " << port_size << std::endl;
                            }  
                            else {
                                actual_id = (actual) ? actual->FullId() : 0 ;
                                actual_name = actual_id->Name();
                                std::cout << "ACN : " << actual_name << "   FN : " << formal_name << std::endl;
                            }
        	        	}
        	        }
        	    	// Get the starting location and ending location of this module item.
        	    	linefile_type start_linefile = module_item->StartingLinefile() ;
        	    	linefile_type end_linefile = module_item->EndingLinefile() ;

        	    	// Now we want to remove the sections occupied by the gearbox constructs
        	    	// from the file. We know the starting location and ending location of
        	    	// these items. Call 'Replace' routine of 'TextBasedDesignMod' utility for this.
        	    	file_base_comment.Replace(start_linefile, end_linefile, 0 /* no replace, only remove */) ;
        	    }
        	}
        }
    }

    // Get the last module item, it is the item we want to remove
    VeriModuleItem *mod_item = (items && items->Size()) ? (VeriModuleItem*)items->GetLast() : 0 ;
    if (mod_item) {
        // Get the starting location and ending location of this module item.
        linefile_type start_linefile = mod_item->StartingLinefile() ;
        linefile_type end_linefile = mod_item->EndingLinefile() ;

        // Now we want to remove the section occupied by the last module item
        // from the file. We know the starting location and ending location of
        // this item. Call 'Replace' routine of 'TextBasedDesignMod' utility for this.
        file_base_comment.Replace(start_linefile, end_linefile, 0 /* no replace, only remove */) ;
    } else {
        Message::PrintLine("No module item found in module ", mod->Name()) ;
    }

    /* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ *
     *                Write modified source file to a file                *
     * \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/ */
    Message::PrintLine("Writing the design to file ", out_file_name) ;

    // Write the modified file
    file_base_comment.WriteFile(file_name, out_file_name) ;

    // Remove all analyzed modules
    veri_file::RemoveAllModules() ;

#endif // #ifndef VERIFIC_LINEFILE_INCLUDES_COLUMNS

    return 0 ; // Status OK
}

