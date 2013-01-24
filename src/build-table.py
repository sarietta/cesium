#!/usr/bin/env python
""" Usage: call with <filename>
"""

import os
import pdb
import sys
import clang.cindex

def TraverseAST(node, callback, data):
    for child in node.get_children():
        TraverseAST(child, callback, data);
    result = callback(node);
    if (result != None):
        data.append(result);

def GetFileFunctionCalls(filename, AST):
    root = AST.cursor;
    def PermitFunctionCalls(node):
        if (node.kind == clang.cindex.CursorKind.DECL_REF_EXPR and os.path.abspath(str(filename)) == os.path.abspath(str(node.location.file))):
            return {hash(node.referenced.get_usr()): node.displayname};
        else:
            return None;
    calls = [];
    TraverseAST(root, PermitFunctionCalls, calls);
    return calls;

def GetFileFunctionDefinitions(filename, AST):
    root = AST.cursor;
    def PermitFunctionDefs(node):
        if (node.kind == clang.cindex.CursorKind.CXX_METHOD and os.path.abspath(str(filename)) == os.path.abspath(str(node.location.file))):
            #pdb.set_trace();
            return {hash(node.get_usr()): node.displayname};
        else:
            return None;
    decls = [];
    TraverseAST(root, PermitFunctionDefs, decls);
    return decls;

index = clang.cindex.Index.create()
AST = index.parse(None, ["-x", "c++", sys.argv[1]]);

calls = GetFileFunctionCalls(sys.argv[1], AST);
decls = GetFileFunctionDefinitions(sys.argv[1], AST);
print calls;
print decls;
