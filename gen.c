/*
 * gen.c - Generate code to "treecc" output files.
 *
 * Copyright (C) 2001  Southern Storm Software, Pty Ltd.
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
#include "errors.h"

#ifdef	__cplusplus
extern	"C" {
#endif

void TreeCCGenerate(TreeCCContext *context)
{
	switch(context->language)
	{
		case TREECC_LANG_C:
		{
			TreeCCGenerateC(context);
		}
		break;

		case TREECC_LANG_CPP:
		{
			TreeCCGenerateCPP(context);
		}
		break;

		case TREECC_LANG_JAVA:
		{
			TreeCCGenerateJava(context);
		}
		break;

		case TREECC_LANG_CSHARP:
		{
			TreeCCGenerateCSharp(context);
		}
		break;
	}
}

/*
 * Generate selectors for one case of a "switch" statement.
 */
static int GenerateSelectors(TreeCCContext *context, TreeCCStream *stream,
							 const TreeCCNonVirtual *nonVirt, TreeCCNode *node,
							 int markBit, int triggerNum)
{
	/* If the mark bit is already set, then we've already handled this case */
	if((node->flags & markBit) != 0)
	{
		return 0;
	}

	/* Mark the node as already visited */
	node->flags |= markBit;

	/* Generate a selector for this node */
	(*(nonVirt->genSelector))(context, stream, node, triggerNum);

	/* Generate selectors for the children */
	node = node->firstChild;
	while(node != 0)
	{
		GenerateSelectors(context, stream, nonVirt, node, markBit, triggerNum);
		node = node->nextSibling;
	}

	/* We have done something non-trivial */
	return 1;
}

/*
 * Determine if the outer trigger levels of two operation cases match.
 */
static int LevelsMatch(TreeCCOperationCase *operCase1,
					   TreeCCOperationCase *operCase2,
					   int triggerNum)
{
	TreeCCTrigger *trigger1 = operCase1->triggers;
	TreeCCTrigger *trigger2 = operCase2->triggers;
	while(triggerNum > 0)
	{
		if(trigger1->node != trigger2->node)
		{
			return 0;
		}
		trigger1 = trigger1->next;
		trigger2 = trigger2->next;
	}
	return 1;
}

/*
 * Generate code for a "switch" on a trigger.
 */
static TreeCCOperationCase *GenerateSwitch
			(TreeCCContext *context, TreeCCStream *stream,
			 const TreeCCNonVirtual *nonVirt, TreeCCOperation *oper,
			 TreeCCOperationCase *operCase, int triggerNum)
{
	TreeCCParam *param;
	int num, markBit;
	int paramNum;
	TreeCCOperationCase *firstCase;
	TreeCCTrigger *trigger;
	TreeCCNode *node;
	int isEnum;

	/* Generate the head of the switch for this level */
	param = oper->params;
	num = 0;
	paramNum = 1;
	while(param != 0)
	{
		if((param->flags & TREECC_PARAM_TRIGGER) != 0)
		{
			if(num == triggerNum)
			{
				break;
			}
			++num;
		}
		if(!(param->name))
		{
			++paramNum;
		}
		param = param->next;
	}
	if(!param)
	{
		/* Output the code for the case */
		if((oper->flags & TREECC_OPER_INLINE) != 0)
		{
			(*(nonVirt->genCaseInline))(context, stream, operCase,
										triggerNum - 1);
		}
		else
		{
			(*(nonVirt->genCaseCall))(context, stream, operCase,
									  operCase->number, triggerNum - 1);
		}
		return operCase->next;
	}
	node = TreeCCNodeFindByType(context, param->type);
	isEnum = ((node->flags & TREECC_NODE_ENUM) != 0);
	if(param->name)
	{
		(*(nonVirt->genSwitchHead))(context, stream, param->name,
								    triggerNum, isEnum);
	}
	else
	{
		char paramName[64];
		sprintf(paramName, "P%d__", paramNum);
		(*(nonVirt->genSwitchHead))(context, stream, paramName,
									triggerNum, isEnum);
	}

	/* Recurse to handle the next-inner level of switch statements */
	markBit = TREECC_NODE_MARK(triggerNum);
	TreeCCNodeClearMarking(context, markBit);
	firstCase = operCase;
	do
	{
		/* Output the "case" statements to match this operation case */
		trigger = operCase->triggers;
		num = 0;
		while(trigger != 0 && num < triggerNum)
		{
			++num;
			trigger = trigger->next;
		}
		if(trigger)
		{
			if(!GenerateSelectors(context, stream, nonVirt,
							      trigger->node, markBit, triggerNum))
			{
				/* We already output code for this with another case */
				TreeCCErrorOnLine(context->input, operCase->filename,
								  operCase->linenum,
								  "this operation case duplicates another");
			}
		}
		(*(nonVirt->genEndSelectors))(context, stream, triggerNum);

		/* Generate the next level of "switch"'s */
		operCase = GenerateSwitch(context, stream, nonVirt,
								  oper, operCase, triggerNum + 1);

		/* Terminate the "case" statement */
		(*(nonVirt->genEndCase))(context, stream, triggerNum);
	}
	while(operCase != 0 && LevelsMatch(firstCase, operCase, triggerNum));

	/* Generate the end of the switch for this level */
	(*(nonVirt->genEndSwitch))(context, stream, triggerNum);

	/* Return the next case to be handled to the caller */
	return operCase;
}

/*
 * Assign positions to the node types in a non-virtual operation.
 */
static void AssignTriggerPosns(TreeCCContext *context, TreeCCOperation *oper)
{
	TreeCCParam *param;
	TreeCCNode *type;
	param = oper->params;
	while(param != 0)
	{
		if((param->flags & TREECC_PARAM_TRIGGER) != 0)
		{
			type = TreeCCNodeFindByType(context, param->type);
			if(type)
			{
				TreeCCNodeAssignPositions(type);
			}
		}
		param = param->next;
	}
}

/*
 * Forward declaration.
 */
static void GenerateMultiSwitch(TreeCCContext *context,
								TreeCCStream *stream,
							    const TreeCCNonVirtual *nonVirt,
								TreeCCOperation *oper,
								TreeCCOperationCase **sortedCases,
								int base, int multiplier,
								TreeCCParam *nextParam,
								int triggerNum);

/*
 * Scan down the hierarchy to generate cases for a multi-trigger operation.
 */
static void GenerateMultiScan(TreeCCContext *context,
							  TreeCCStream *stream,
						      const TreeCCNonVirtual *nonVirt,
							  TreeCCOperation *oper,
							  TreeCCOperationCase **sortedCases,
							  int base, int multiplier,
							  TreeCCParam *nextParam,
							  int triggerNum,
							  TreeCCNode *node)
{
	/* Ignore abstract node types */
	if((node->flags & TREECC_NODE_ABSTRACT) == 0)
	{
		/* Generate a selector for this node */
		(*(nonVirt->genSelector))(context, stream, node, triggerNum);
		(*(nonVirt->genEndSelectors))(context, stream, triggerNum);

		/* Go in one level for this node */
		GenerateMultiSwitch(context, stream, nonVirt, oper, sortedCases,
						    base + node->position * multiplier,
					  		multiplier * nextParam->size, nextParam->next,
					  	    triggerNum + 1);

		/* Terminate the case for the above selector */
		(*(nonVirt->genEndCase))(context, stream, triggerNum);
	}

	/* Scan all of the children */
	node = node->firstChild;
	while(node != 0)
	{
		GenerateMultiScan(context, stream, nonVirt, oper, sortedCases,
						  base, multiplier, nextParam, triggerNum, node);
		node = node->nextSibling;
	}
}

/*
 * Generate code for a "switch" on a multi-trigger operation.
 */
static void GenerateMultiSwitch(TreeCCContext *context,
								TreeCCStream *stream,
							    const TreeCCNonVirtual *nonVirt,
								TreeCCOperation *oper,
								TreeCCOperationCase **sortedCases,
								int base, int multiplier,
								TreeCCParam *nextParam,
								int triggerNum)
{
	TreeCCNode *type;
	int isEnum;

	/* Scan for the next trigger parameter */
	while(nextParam != 0 && (nextParam->flags & TREECC_PARAM_TRIGGER) == 0)
	{
		nextParam = nextParam->next;
	}

	/* If we are out of triggers, then we need to output the case code */
	if(!nextParam)
	{
		if((oper->flags & TREECC_OPER_INLINE) != 0)
		{
			(*(nonVirt->genCaseInline))(context, stream, sortedCases[base],
										triggerNum - 1);
		}
		else
		{
			(*(nonVirt->genCaseCall))(context, stream, sortedCases[base],
									  sortedCases[base]->number,
									  triggerNum - 1);
		}
		return;
	}

	/* Generate the head of the switch for this level */
	type = TreeCCNodeFindByType(context, nextParam->type);
	isEnum = ((type->flags & TREECC_NODE_ENUM) != 0);
	if(nextParam->name)
	{
		(*(nonVirt->genSwitchHead))(context, stream, nextParam->name,
								    triggerNum, isEnum);
	}
	else
	{
		char paramName[64];
		int paramNum = 0;
		TreeCCParam *param = oper->params;
		while(param != 0 && param != nextParam)
		{
			if(!(param->name))
			{
				++paramNum;
			}
			param = param->next;
		}
		sprintf(paramName, "P%d__", paramNum);
		(*(nonVirt->genSwitchHead))(context, stream, paramName,
									triggerNum, isEnum);
	}

	/* Scan down the node type hierarchy for this trigger level */
	GenerateMultiScan(context, stream, nonVirt, oper, sortedCases,
					  base, multiplier, nextParam, triggerNum, type);

	/* Generate the end of the switch for this level */
	(*(nonVirt->genEndSwitch))(context, stream, triggerNum);
}

/*
 * Generate code for a specific non-virtual operation.
 */
static void GenerateNonVirtual(TreeCCContext *context,
							   const TreeCCNonVirtual *nonVirt,
							   TreeCCOperation *oper)
{
	TreeCCStream *stream;
	TreeCCOperationCase *operCase;
	int number;

	/* Determine which stream to write to */
	if(context->language == TREECC_LANG_JAVA)
	{
		if(oper->className)
		{
			stream = TreeCCStreamGetJava(context, oper->className);
		}
		else
		{
			stream = TreeCCStreamGetJava(context, oper->name);
		}
	}
	else
	{
		stream = oper->source;
	}

	/* Output start declarations for the operation */
	(*(nonVirt->genStart))(context, stream, oper);

	/* If the operation is not inline, then output functions for all cases */
	if((oper->flags & TREECC_OPER_INLINE) == 0)
	{
		number = 1;
		operCase = oper->firstCase;
		while(operCase != 0)
		{
			(*(nonVirt->genCaseFunc))(context, stream, operCase, number);
			operCase->number = number++;
			operCase = operCase->next;
		}
	}

	/* Output the entry point for the operation */
	(*(nonVirt->genEntry))(context, stream, oper);

	/* Generate the the switch statement for the outer-most level */
	if(oper->numTriggers <= 1)
	{
		GenerateSwitch(context, stream, nonVirt, oper, oper->firstCase, 0);
	}
	else
	{
		AssignTriggerPosns(context, oper);
		GenerateMultiSwitch(context, stream, nonVirt, oper,
							oper->sortedCases, 0, 1, oper->params, 0);
	}

	/* Output the exit point for the operation */
	(*(nonVirt->genExit))(context, stream, oper);
}

void TreeCCGenerateNonVirtuals(TreeCCContext *context,
							   const TreeCCNonVirtual *nonVirt)
{
	unsigned int hash;
	TreeCCOperation *oper;
	for(hash = 0; hash < TREECC_HASH_SIZE; ++hash)
	{
		oper = context->operHash[hash];
		while(oper != 0)
		{
			if((oper->flags & TREECC_OPER_VIRTUAL) == 0)
			{
				GenerateNonVirtual(context, nonVirt, oper);
			}
			oper = oper->nextHash;
		}
	}
}

#ifdef	__cplusplus
};
#endif
