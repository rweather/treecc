/*
 * gen_ruby.c - Generate Ruby source code from "treecc" input files.
 *
 * Copyright (C) 2001, 2002  Southern Storm Software, Pty Ltd.
 *
 * Hacked by Peter Minten <silvernerd@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "system.h"
#include "input.h"
#include "info.h"
#include "gen.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Declare the type definitions for a node type.
 */
static void DeclareTypeDefs(TreeCCContext *context,
						    TreeCCNode *node)
{
	if((node->flags & TREECC_NODE_ENUM) != 0)
	{
		int counter = 0;
		/* Define an enumerated type */
		TreeCCStream *stream = node->source;
		TreeCCNode *child;
		TreeCCStreamPrint(stream, "class %s\n", node->name);
		child = node->firstChild;
		while(child != 0)
		{
			if((child->flags & TREECC_NODE_ENUM_VALUE) != 0)
			{
				TreeCCStreamPrint(stream, "\t%s = %i,\n", child->name, counter++);
			}
			child = child->nextSibling;
		}
		TreeCCStreamPrint(stream, "end\n\n");
	}
}

/*
 * Output the parameters for a node creation function.
 */
static int CreateParams(TreeCCContext *context, TreeCCStream *stream,
						TreeCCNode *node, int needComma)
{
	TreeCCField *field;
	if(node->parent)
	{
		needComma = CreateParams(context, stream, node->parent, needComma);
	}
	field = node->fields;
	while(field != 0)
	{
		if((field->flags & TREECC_FIELD_NOCREATE) == 0)
		{
			if(needComma)
			{
				TreeCCStreamPrint(stream, ", ");
			}
			/* Don't name the types, Ruby figures that out */
			TreeCCStreamPrint(stream, "%s", /*field->type,*/ field->name);
			needComma = 1;
		}
		field = field->next;
	}
	return needComma;
}

/*
 * Output the parameters to call an inherited constructor.
 */
static int InheritParamsSource(TreeCCContext *context, TreeCCStream *stream,
							   TreeCCNode *node, int needComma)
{
	TreeCCField *field;
	if(node->parent)
	{
		needComma = InheritParamsSource(context, stream,
									    node->parent, needComma);
	}
	field = node->fields;
	while(field != 0)
	{
		if((field->flags & TREECC_FIELD_NOCREATE) == 0)
		{
			if(needComma)
			{
				TreeCCStreamPrint(stream, ", ");
			}
			TreeCCStreamPrint(stream, "%s", field->name);
			needComma = 1;
		}
		field = field->next;
	}
	return needComma;
}

/* 
 * Implement the virtual methods that have implementations in a node type.
 */
static void ImplementVirtuals(TreeCCContext *context, TreeCCStream *stream,
							  TreeCCNode *node, TreeCCNode *actualNode)
{
	TreeCCVirtual *virt;
	TreeCCParam *param;
	TreeCCOperationCase *operCase;
	int declareCase, abstractCase;
	TreeCCNode *tempNode;
	int num, first;
	int needComma;
	if(node->parent)
	{
		ImplementVirtuals(context, stream, node->parent, actualNode);
	}
	virt = node->virtuals;
	while(virt != 0)
	{
		/* Determine if we need a definition for this virtual,
		   and whether the definition is real or abstract */
		operCase = TreeCCOperationFindCase(context, actualNode, virt->name);
		if(!operCase)
		{
			tempNode = actualNode->parent;
			abstractCase = 1;
			while(tempNode != 0)
			{
				operCase = TreeCCOperationFindCase
								(context, tempNode, virt->name);
				if(operCase != 0)
				{
					abstractCase = 0;
					break;
				}
				tempNode = tempNode->parent;
			}
			declareCase = abstractCase;
		}
		else
		{
			declareCase = 1;
			abstractCase = 0;
		}
		if(declareCase)
		{
			if(abstractCase)
			{
				if(node == actualNode)
				{
					TreeCCStreamPrint(stream, "\tdef %s(", virt->name);
				}
				else
				{
					/* Inherit the "abstract" definition from the parent */
					virt = virt->next;
					continue;
				}
			}
			else
			{
				if(node == actualNode)
				{
					TreeCCStreamPrint(stream, "\tdef %s(", virt->name);
				}
				else
				{
					TreeCCStreamPrint(stream, "\tdef %s(", virt->name);
				}
			}
			param = virt->oper->params;
			needComma = 0;
			num = 1;
			first = 1;
			while(param != 0)
			{
				if(needComma)
				{
					TreeCCStreamPrint(stream, ", ");
				}
				if(first)
				{
					/* Skip the first argument, which corresponds to "this" */
					if(!(param->name))
					{
						++num;
					}
					first = 0;
				}
				else
				{
					if(param->name)
					{
						TreeCCStreamPrint(stream, "%s", param->name);
					}
					else
					{
						TreeCCStreamPrint(stream, "P%d__", num);
						++num;
					}
					needComma = 1;
				}
				param = param->next;
			}
			if(!abstractCase)
			{
				TreeCCStreamPrint(stream, ")\n");
				TreeCCStreamLine(stream, operCase->codeLinenum,
								 operCase->codeFilename);
				TreeCCStreamPrint(stream, "\t");
				if(!(virt->oper->params->name) ||
				   !strcmp(virt->oper->params->name, "self"))
				{
					/* The first parameter is called "this", so we don't
					   need to declare it at the head of the function */
					TreeCCStreamCodeIndent(stream, operCase->code, 1);
				}
				else
				{
					/* The first parameter is called something else,
					   so create a temporary variable to hold "this" */
				   	TreeCCStreamPrint(stream, "\n\t\t%s %s = self\n\t",
									  actualNode->name,
									  virt->oper->params->name);
					TreeCCStreamCodeIndent(stream, operCase->code, 1);
				}
				TreeCCStreamPrint(stream, "end\n");
				TreeCCStreamFixLine(stream);
				TreeCCStreamPrint(stream, "\n");
			}
			else
			{
				TreeCCStreamPrint(stream, ")\n\n");
			}
		}
		virt = virt->next;
	}
}

/*
 * Build the type declarations for a node type.
 */
static void BuildTypeDecls(TreeCCContext *context,
						   TreeCCNode *node)
{
	TreeCCStream *stream;
	int needComma;
	/*const char *constructorAccess; NOT USED*/
	TreeCCField *field;
	int isAbstract;

	/* Ignore if this is an enumerated type node */
	if((node->flags & (TREECC_NODE_ENUM | TREECC_NODE_ENUM_VALUE)) != 0)
	{
		return;
	}

	/* Determine if this class has abstract virtuals */
	isAbstract = TreeCCNodeHasAbstracts(context, node);

	/* Output the class header */
	stream = node->source;
	if(node->parent)
	{
		/* Inherit from a specified parent type */
		/* Ruby doesn't know abstract */	
		/*   
		if(isAbstract)
		{
			TreeCCStreamPrint(stream, "public abstract class %s : %s\n{\n",
							  node->name, node->parent->name);
		}
		else
		{
			TreeCCStreamPrint(stream, "public class %s : %s\n{\n",
							  node->name, node->parent->name);
		}
		*/
		TreeCCStreamPrint(stream, "class %s < %s\n",
						  node->name, node->parent->name);
	}
	else
	{
		/* This type is the base of a class hierarchy */
		/* Ruby doesn't know abstract */
/*		if(isAbstract)
		{
			TreeCCStreamPrint(stream, "public abstract class %s\n{\n",
							  node->name);
		}
		else
		{
			TreeCCStreamPrint(stream, "public class %s\n{\n", node->name);
		}*/

		TreeCCStreamPrint(stream, "class %s\n", node->name);

		/* Declare the node kind member variable */
		/* No declaration needed in Ruby */

		/* Declare the filename and linenum fields if we are tracking lines */
		/* Not needed */

		/* Declare the public methods for access to the above fields */
		/* Ruby has handy accessor creating stuff */
		/*TreeCCStreamPrint(stream,
				"\tpublic int getKind() { return kind__; }\n");*/
		TreeCCStreamPrint(stream,
				"\tattr_reader :kind\n");
			
		if(context->track_lines)
		{
			/* A same kind of hack here*/
			/*TreeCCStreamPrint(stream,
				"\tpublic String getFilename() { return filename__; }\n");
			TreeCCStreamPrint(stream,
				"\tpublic long getLinenum() { return linenum__; }\n");
			TreeCCStreamPrint(stream,
			 	"\tpublic void setFilename(String filename) "
					"{ filename__ = filename; }\n");
			TreeCCStreamPrint(stream,
				"\tpublic void setLinenum(long linenum) "
					"{ linenum__ = linenum; }\n");*/
			TreeCCStreamPrint(stream,
				"\tattr_accessor :linenum, :filename\n");
		}
		TreeCCStreamPrint(stream, "\n");
	}

	/* Declare the kind value */
	/* Stick to the Ruby convention of constants, start with Uppercase,
	   continue with lowercase */
	/* The parent doesn't matter, so don't check it */
	TreeCCStreamPrint(stream, "\tKind = %d;\n\n",
					  node->number);

	/* Declare the constructor for the node type */
	/* A constructor is always public (I hope) anyway I don't expect
	   Ruby to cause troubles here */

	/* The constructor is ALWAYS called initialize */
	TreeCCStreamPrint(stream, "\tdef initialize(");
	if(context->reentrant)
	{
		TreeCCStreamPrint(stream, "state__");
		needComma = 1;

	}
	else
	{
		needComma = 0;
	}
	
	CreateParams(context, stream, node, needComma);

	TreeCCStreamPrint(stream, ")\n");
	
	/* Enter the super call */
	/* Call the parent class constructor */
	if(node->parent)
	{
		/* Do not use base, Ruby uses super for that */
		/*TreeCCStreamPrint(stream, "\t\t: base(");*/
		TreeCCStreamPrint(stream, "\t\tsuper(");
		if(context->reentrant)
		{
			TreeCCStreamPrint(stream, "@state");
			needComma = 1;
		}
		else
		{
			needComma = 0;
		}
		CreateParams(context, stream, node, needComma);		
		InheritParamsSource(context, stream, node->parent, needComma);
		TreeCCStreamPrint(stream, ")\n");
	}
	
	/* Set the node kind */	
	TreeCCStreamPrint(stream, "\t\t@kind = Kind;\n");

	/* Track the filename and line number if necessary */
	if(context->track_lines && !(node->parent))
	{
		if(context->reentrant)
		{
			TreeCCStreamPrint(stream,
					"\t\t@filename = @state.currFilename\n");
			TreeCCStreamPrint(stream,
					"\t\t@linenum = @state.currLinenum\n");
		}
		else
		{
			TreeCCStreamPrint(stream,
					"\t\t@filename = %s.state.currFilename();\n",
					context->state_type);
			TreeCCStreamPrint(stream,
					"\t\t@linenum = %s.state.currLinenum();\n",
					context->state_type);
		}
	}

	/* Initialize the fields that are specific to this node type */
	field = node->fields;
	while(field != 0)
	{
		if((field->flags & TREECC_FIELD_NOCREATE) == 0)
		{
			TreeCCStreamPrint(stream, "\t\tself.%s = %s;\n",
							  field->name, field->name);
		}
		else if(field->value)
		{
			TreeCCStreamPrint(stream, "\t\tself.%s = %s;\n",
							  field->name, field->value);
		}
		field = field->next;
	}
	TreeCCStreamPrint(stream, "\tend\n\n");

	/* Implement the virtual functions */
	ImplementVirtuals(context, stream, node, node);

	/* Declare the "isA" and "getKindName" helper methods */

	TreeCCStreamPrint(stream, "\tdef isA(kind)\n");

	TreeCCStreamPrint(stream, "\t\tif(@kind == Kind) then\n");
	TreeCCStreamPrint(stream, "\t\t\treturn true;\n");
	TreeCCStreamPrint(stream, "\t\telse\n");
	if(node->parent)
	{
		TreeCCStreamPrint(stream, "\t\t\treturn super(kind);\n");
	}
	else
	{
		TreeCCStreamPrint(stream, "\t\t\treturn 0;\n");
	}
	TreeCCStreamPrint(stream, "\tend\n\n");

	TreeCCStreamPrint(stream, "\tdef kindname\n");

	TreeCCStreamPrint(stream, "\t\treturn \"%s\"\n", node->name);
	TreeCCStreamPrint(stream, "\tend\n");

	/* Output the class footer */
	TreeCCStreamPrint(stream, "end\n\n");
}

/*
 * Declare the parameters for a factory method in the state type.
 */
static int FactoryCreateParams(TreeCCContext *context, TreeCCStream *stream,
							   TreeCCNode *node, int needComma)
{
	TreeCCField *field;
	if(node->parent)
	{
		needComma = FactoryCreateParams(context, stream,
										node->parent, needComma);
	}
	field = node->fields;
	while(field != 0)
	{
		if((field->flags & TREECC_FIELD_NOCREATE) == 0)
		{
			if(needComma)
			{
				TreeCCStreamPrint(stream, ", ");
			}
			/* Delete the types */
			TreeCCStreamPrint(stream, "%s", field->name);
			needComma = 1;
		}
		field = field->next;
	}
	return needComma;
}

/*
 * Output invocation parameters for a call to a constructor
 * from within a factory method.
 */
static int FactoryInvokeParams(TreeCCContext *context, TreeCCStream *stream,
							   TreeCCNode *node, int needComma)
{
	TreeCCField *field;
	if(node->parent)
	{
		needComma = FactoryInvokeParams(context, stream,
										node->parent, needComma);
	}
	field = node->fields;
	while(field != 0)
	{
		if((field->flags & TREECC_FIELD_NOCREATE) == 0)
		{
			if(needComma)
			{
				TreeCCStreamPrint(stream, ", ");
			}
			TreeCCStreamPrint(stream, "%s", field->name);
			needComma = 1;
		}
		field = field->next;
	}
	return needComma;
}

/*
 * Implement the create function for a node type.
 */
static void ImplementCreateFuncs(TreeCCContext *context, TreeCCNode *node)
{
	TreeCCStream *stream;

	/* Ignore if this is an enumerated type node */
	if((node->flags & (TREECC_NODE_ENUM | TREECC_NODE_ENUM_VALUE)) != 0)
	{
		return;
	}

	/* Ignore this if it is an abstract node */
	if((node->flags & TREECC_NODE_ABSTRACT) != 0)
	{
		return;
	}

	/* Determine which stream to write to */
	if(context->commonSource)
	{
		stream = context->commonSource;
	}
	else
	{
		stream = context->sourceStream;
	}

	/* Output the start of the function definition */
	TreeCCStreamPrint(stream, "\tdef %s %sCreate(",
					  node->name, node->name);

	/* Output the parameters for the create function */
	FactoryCreateParams(context, stream, node, 0);

	/* Output the body of the creation function */
	if(context->abstract_factory)
	{
		TreeCCStreamPrint(stream, ")\n");
		TreeCCStreamPrint(stream, "raise \"Abstract method called: %s\\n\"\n", node->name);		
	}
	else
	{
		TreeCCStreamPrint(stream, ")\n");
		TreeCCStreamPrint(stream, "\t\treturn %s.new(this", node->name);
		FactoryInvokeParams(context, stream, node, 1);
		TreeCCStreamPrint(stream, ")\n");
		TreeCCStreamPrint(stream, "\tend\n\n");
	}
}

/*
 * Implement the state type in the source stream.
 */
static void ImplementStateType(TreeCCContext *context, TreeCCStream *stream)
{
	TreeCCStreamPrint(stream, "class %s\n",
					  context->state_type);


	/* Singleton handling for non-reentrant systems */
	if(!(context->reentrant))
	{
		TreeCCStreamPrint(stream, "\tdef state\n");
		TreeCCStreamPrint(stream, "\t\tif(@state != null) return @state\n");
		TreeCCStreamPrint(stream, "\t\t@state = %s.new()\n",
						  context->state_type);
		TreeCCStreamPrint(stream, "\t\treturn @state;\n");
		TreeCCStreamPrint(stream, "\tend\n\n");
	}

	/* Implement the constructor */
	if(context->reentrant)
	{
		/* No constructor */
	}
	else
	{
		TreeCCStreamPrint(stream, "\tdef intialize \n \t\t@state = self \n \tend\n\n");
	}

	/* Implement the create functions for all of the node types */
	if(context->reentrant)
	{
		TreeCCNodeVisitAll(context, ImplementCreateFuncs);
	}

	/* Implement the line number tracking methods */
	if(context->track_lines)
	{
		TreeCCStreamPrint(stream,
			"\tdef currFilename \n \t\treturn null \n\tend\n");
		TreeCCStreamPrint(stream,
			"\tdef currLinenum \n \t\treturn 0 \n\tend\n\n");
	}

	/* Declare the end of the state type */
	TreeCCStreamPrint(stream, "end\n\n");
}

/*
 * Write out header information for all streams.
 */
static void WriteRubyHeaders(TreeCCContext *context)
{
	TreeCCStream *stream = context->streamList;
	while(stream != 0)
	{
		if(!(stream->isHeader))
		{
			TreeCCStreamPrint(stream,
					"# %s.  Generated automatically by treecc \n\n",
					stream->embedName);
			if(context->namespace)
			{
				TreeCCStreamPrint(stream, "module %s\nbegin\n\n",
								  context->namespace);
			}
			/*Ruby doesn't require a System lib to be included*/
			/*TreeCCStreamPrint(stream, "using System;\n\n");*/
			TreeCCStreamSourceTopCS(stream);
		}
		if(stream->defaultFile)
		{
			/* Reset the dirty flag if this is a default stream,
			   because we don't want to write out the final file
			   if it isn't actually written to in practice */
			stream->dirty = 0;
		}
		stream = stream->nextStream;
	}
}

/*
 * Write out footer information for all streams.
 */
static void WriteRubyFooters(TreeCCContext *context)
{
	TreeCCStream *stream = context->streamList;
	while(stream != 0)
	{
		if(stream->defaultFile && !(stream->dirty))
		{
			/* Clear the default file's contents, which we don't need */
			TreeCCStreamClear(stream);
		}
		else if(!(stream->isHeader))
		{
			TreeCCStreamSourceBottom(stream);
			if(context->namespace)
			{
				TreeCCStreamPrint(stream, "end\n");
			}
		}
		stream = stream->nextStream;
	}
}

void TreeCCGenerateRuby(TreeCCContext *context)
{
	/* Write all stream headers */
	WriteRubyHeaders(context);

	/* Generate the contents of the source stream */
	TreeCCNodeVisitAll(context, DeclareTypeDefs);
	if(context->commonSource)
	{
		ImplementStateType(context, context->commonSource);
	}
	else
	{
		ImplementStateType(context, context->sourceStream);
	}
	TreeCCNodeVisitAll(context, BuildTypeDecls);
	TreeCCGenerateNonVirtuals(context, &TreeCCNonVirtualFuncsJava);

	/* Write all stream footers */
	WriteRubyFooters(context);
}

#ifdef	__cplusplus
};
#endif
