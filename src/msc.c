/***************************************************************************
 *
 * $Id$
 *
 * The message sequence parser ADT.
 * Copyright (C) 2009 Michael C McTernan, Michael.McTernan.2001@cs.bris.ac.uk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "safe.h"
#include "msc.h"

/***************************************************************************
 * Structures
 ***************************************************************************/

struct MscEntityTag
{
    char                *label;
    struct MscAttribTag *attr;
    struct MscEntityTag *next;
};

struct MscEntityListTag
{
    unsigned int         elements;
    struct MscEntityTag *head, *tail;
};

struct MscArcTag
{
    char                *src, *dst;
    MscArcType           type;
    unsigned int         inputLine;
    struct MscAttribTag *attr;
    struct MscArcTag    *next;
};

struct MscArcListTag
{
    unsigned int         elements;
    unsigned int         parElements;
    struct MscArcTag    *head, *tail;
};


struct MscAttribTag
{
    MscAttribType       type;
    char                *value;
    struct MscAttribTag *next;
};

struct MscOptTag
{
    MscOptType          type;
    char               *value;
    struct MscOptTag   *next;
};

struct MscTag
{
    struct MscOptTag        *optList;
    struct MscEntityListTag *entityList;
    struct MscArcListTag    *arcList;
};

/***************************************************************************
 * Local Functions
 ***************************************************************************/

/** Find come attrbute in an attribute list.
 *
 * \param[in] attr  Head of the linked list to search.
 * \param[in] a     The attribute type to find.
 * \retval  NULL    If the attribute was not found or the passed list was NULL.
 */
static const char *findAttrib(const struct MscAttribTag *attr, MscAttribType a)
{
    while(attr != NULL && attr->type != a)
    {
        attr = attr->next;
    }

    if(attr != NULL)
    {
        return attr->value;
    }
    else
    {
        return NULL;
    }
}


/** Free the memory underlying a list of attributes.
 */
static void freeAttribList(struct MscAttribTag *attr)
{
    while(attr)
    {
        struct MscAttribTag *next = attr->next;
        free(attr->value);
        free(attr);
        attr = next;
    }
}

/***************************************************************************
 * Option Functions
 ***************************************************************************/

/* Allocate some option and set it's value.
 */
struct MscOptTag *MscAllocOpt(MscOptType  type,
                              char       *value)
{
    struct MscOptTag *a = malloc_s(sizeof(struct MscOptTag));

    a->type  = type;
    a->value = value;
    a->next  = NULL;

    return a;
}

/* Link one or two options together.
 */
struct MscOptTag *MscLinkOpt(struct MscOptTag *head,
                             struct MscOptTag *newHead)
{
    struct MscOptTag *tail = newHead;

    assert(newHead);

    /* Find the end of the list */
    while(tail->next)
    {
        tail = tail->next;
    }

    tail->next = head;
    return newHead;
}

/* MscPrettyOptType
 *  Returns a string that is the human readable form of the option
 *  code passed to the function.
 */
const char *MscPrettyOptType(MscOptType t)
{
    switch(t)
    {
        case MSC_OPT_HSCALE:       return "hscale";
        case MSC_OPT_WIDTH:        return "width";
        case MSC_OPT_ARCGRADIENT:  return "arcgradient";
        case MSC_OPT_WORDWRAPARCS: return "wordwraparcs";
    }

    return "<unknown>";
}

struct MscOptTag *MscFindOpt(struct MscOptTag *list,
                             MscOptType        type)
{
    struct MscOptTag *elem = list;

    while(elem && elem->type != type)
    {
        elem = elem->next;
    }

    if(elem && elem->type == type)
    {
        return elem;
    }

    return NULL;
}

void MscPrintOptList(struct MscOptTag *list)
{
    struct MscOptTag *elem = list;

    while(elem)
    {
        printf("%p: %s=%s\n", elem, MscPrettyOptType(elem->type), elem->value);
        elem = elem->next;
    }
}

/***************************************************************************
 * Entity Functions
 ***************************************************************************/

/* MscAllocEntity
 *  Allocate some entity and set it's name.
 */
struct MscEntityTag *MscAllocEntity(char *entityName)
{
    struct MscEntityTag *e = malloc_s(sizeof(struct MscEntityListTag));

    e->label = entityName;
    e->attr  = NULL;
    e->next  = NULL;

    return e;
}


/* MscLinkEntity
 *  Link some entity onto a list, possibly producing a new head element.
 */
struct MscEntityListTag *MscLinkEntity(struct MscEntityListTag *list,
                                       struct MscEntityTag     *elem)
{
    /* Check if the list has been allocated or not */
    if(list == NULL)
    {
        list = zalloc_s(sizeof(struct MscEntityListTag));
    }

    /* Check for an empty list */
    if(list->head == NULL)
    {
        list->head = list->tail = elem;
    }
    else
    {
        /* Add to tail */
        list->tail->next = elem;
        list->tail = elem;
    }

    /* Increment count of elements */
    list->elements++;

    return list;
}


void MscPrintEntityList(struct MscEntityListTag *list)
{
    struct MscEntityTag *elem = list->head;

    while(elem)
    {
        printf("%p: %s\n", elem, elem->label);
        MscPrintAttrib(elem->attr);
        elem = elem->next;
    }
}

/***************************************************************************
 * Arc Functions
 ***************************************************************************/

/* MscAllocArc
 *  Allocate an arc, filling in the src and dst entities.
 */
struct MscArcTag *MscAllocArc(char        *srcEntity,
                              char        *dstEntity,
                              MscArcType   type,
                              unsigned int inputLine)
{
    struct MscArcTag *a = malloc_s(sizeof(struct MscArcTag));

    /* A discontinuity arcs are not between entities */
    if(type == MSC_ARC_DISCO)
    {
        assert(srcEntity == NULL && dstEntity == NULL);
    }

    a->inputLine = inputLine;
    a->src  = srcEntity;
    a->dst  = dstEntity;
    a->type = type;
    a->next = NULL;
    a->attr = NULL;

    return a;
}


/* MscLinkArc
 *  Link some entity onto a list, possibly producing a new head element.
 */
struct MscArcListTag *MscLinkArc(struct MscArcListTag *list,
                                 struct MscArcTag     *elem)
{
    /* Check if the list has been allocated or not */
    if(list == NULL)
    {
        list = zalloc_s(sizeof(struct MscArcListTag));
    }

    /* Check for an empty list */
    if(list->head == NULL)
    {
        list->head = list->tail = elem;
    }
    else
    {
        /* Add to tail */
        list->tail->next = elem;
        list->tail = elem;
    }

    /* Increment count of elements */
    list->elements++;
    if(elem->type == MSC_ARC_PARALLEL)
    {
        /* A parallel arc is a place holder, and indicates the next arc
         *  is on the same line.  It also needs to account itself, so subtract
         *  two here.
         */
        list->parElements += 2;
    }

    return list;
}


/* MscPrintArcList
 *  Dump and arc list.
 */
void MscPrintArcList(struct MscArcListTag *list)
{
    struct MscArcTag *elem = list->head;

    while(elem)
    {
        printf("%p: '%s' -> '%s'\n", elem, elem->src, elem->dst);
        MscPrintAttrib(elem->attr);

        elem = elem->next;
    }
}


/***************************************************************************
 * Attribute functions
 ***************************************************************************/

/* MscAllocAttrib
 *  Allocate some attribute.
 */
struct MscAttribTag *MscAllocAttrib(MscAttribType  type,
                                    char          *value)
{
    struct MscAttribTag *a = malloc_s(sizeof(struct MscAttribTag));

    a->type  = type;
    a->value = value;
    a->next  = NULL;

    return a;
}


/* MscLinkAttrib
 *  Link some attributes.  The ordering of attributes is semi-important
 *  so the list is grown from the tail.
 */
struct MscAttribTag *MscLinkAttrib(struct MscAttribTag *head,
                                   struct MscAttribTag *newHead)
{
    struct MscAttribTag *tail = newHead;

    assert(newHead);

    /* Find the end of the list */
    while(tail->next)
    {
        tail = tail->next;
    }

    tail->next = head;
    return newHead;
}


/* MscArcLinkAttrib
 *  Attach some attributes to some arc.
 */
void MscArcLinkAttrib(struct MscArcTag    *arc,
                      struct MscAttribTag *att)
{
    if(arc->attr)
    {
        arc->attr = MscLinkAttrib(arc->attr, att);
    }
    else
    {
        arc->attr = att;
    }
}


/* MscEntityLinkAttrib
 *  Attach some attributes to some entity.
 */
void MscEntityLinkAttrib(struct MscEntityTag *ent,
                         struct MscAttribTag *att)
{
    if(ent->attr)
    {
        ent->attr = MscLinkAttrib(ent->attr, att);
    }
    else
    {
        ent->attr = att;
    }
}


/* MscPrintAttrib
 *  String a human readable version of the passed attribute list to stdout.
 */
void MscPrintAttrib(const struct MscAttribTag *att)
{
    while(att)
    {
        printf("  %s = %s\n", MscPrettyAttribType(att->type), att->value);
        att = att->next;
    }

}


/* MscPrettyAttribType
 *  Returns a string that is the human readable form of the attribute
 *  code passed to the function.
 */
const char *MscPrettyAttribType(MscAttribType t)
{
    switch(t)
    {
        case MSC_ATTR_LABEL:             return "label";
        case MSC_ATTR_URL:               return "url";
        case MSC_ATTR_ID:                return "id";
        case MSC_ATTR_IDURL:             return "idurl";
        case MSC_ATTR_LINE_COLOUR:       return "linecolour";
        case MSC_ATTR_TEXT_COLOUR:       return "textcolour";
        case MSC_ATTR_TEXT_BGCOLOUR:     return "textbgcolour";
        case MSC_ATTR_ARC_LINE_COLOUR:   return "arclinecolour";
        case MSC_ATTR_ARC_TEXT_COLOUR:   return "arctextcolour";
        case MSC_ATTR_ARC_TEXT_BGCOLOUR: return "arctextbgcolour";
        case MSC_ATTR_NO_ARROWS:         return "noarrows";
        case MSC_ATTR_BI_ARROWS:         return "biarrows";
        case MSC_ATTR_ARC_SKIP:          return "arcskip";
    }

    return "<unknown>";
}


/***************************************************************************
 * MSC Functions
 ***************************************************************************/

struct MscTag *MscAlloc(struct MscOptTag        *optList,
                        struct MscEntityListTag *entityList,
                        struct MscArcListTag    *arcList)
{
    struct MscTag *m = malloc_s(sizeof(struct MscTag));

    /* Copy the lists */
    m->optList    = optList;
    m->entityList = entityList;
    m->arcList    = arcList;

    return m;
}

void MscFree(struct MscTag *m)
{
    struct MscOptTag    *opt    = m->optList;
    struct MscEntityTag *entity = m->entityList->head;
    struct MscArcTag    *arc    = m->arcList->head;

    while(opt)
    {
        struct MscOptTag *next = opt->next;

        free(opt->value);
        free(opt);

        opt = next;
    }

    while(entity)
    {
        struct MscEntityTag *next = entity->next;

        freeAttribList(entity->attr);
        free(entity->label);
        free(entity);

        entity = next;
    }

    while(arc)
    {
        struct MscArcTag *next = arc->next;

        freeAttribList(arc->attr);
        if(arc->src != arc->dst)
        {
            free(arc->dst);
            free(arc->src);
        }
        else
        {
            free(arc->src);
        }
        free(arc);

        arc = next;
    }

    free(m->entityList);
    free(m->arcList);
    free(m);
}

void MscPrint(struct MscTag *m)
{
    printf("Option list (%d options)\n", MscGetNumOpts(m));
    MscPrintOptList(m->optList);

    printf("Entity list (%d entities, %d parallel)\n",
           MscGetNumEntities(m), MscGetNumParallelArcs(m));
    MscPrintEntityList(m->entityList);

    printf("\nArc list (%d arcs)\n", MscGetNumArcs(m));
    MscPrintArcList(m->arcList);
}

unsigned int MscGetNumEntities(struct MscTag *m)
{
    return m->entityList->elements;
}

unsigned int MscGetNumArcs(Msc m)
{
    return m->arcList->elements;
}

unsigned int MscGetNumParallelArcs(Msc m)
{
    return m->arcList->parElements;
}

unsigned int MscGetNumOpts(Msc m)
{
    struct MscOptTag *elem  = m->optList;
    unsigned int      count = 0;

    while(elem)
    {
        count++;
        elem = elem->next;
    }

    return count;
}

int MscGetEntityIndex(struct MscTag *m, const char *label)
{
    struct MscEntityTag *entity = m->entityList->head;
    int                  c      = 0;

    assert(label);

    while(entity != NULL && strcmp(entity->label, label) != 0)
    {
        entity = entity->next;
        c++;
    }

    if(entity == NULL)
    {
        return -1;
    }
    else
    {
        return c;
    }
}


MscEntityIter MscEntityIterBegin(struct MscTag *m)
{
    MscEntityIter i = { m->entityList->head };
    return i;
}


bool MscEntityIterEnd(MscEntityIter *i)
{
    return i->entity == NULL;
}


void MscNextEntity(MscEntityIter *i)
{
    i->entity = i->entity->next;
}


const char *MscGetEntAttrib(MscEntityIter *i, MscAttribType a)
{
    const char *r = findAttrib(i->entity->attr, a);

    /* If the entity label was sought but not found, return entity name */
    return r == NULL && a == MSC_ATTR_LABEL ? i->entity->label : r;
}


const char *MscGetEntIdxAttrib(Msc m, unsigned int entIdx, MscAttribType a)
{
    MscEntityIter i = MscEntityIterBegin(m);

    /* Find the entity */
    while(entIdx > 0 && !MscEntityIterEnd(&i))
    {
        MscNextEntity(&i);
        entIdx--;
    }

    /* Search the attribute list if the entity was found */
    return !MscEntityIterEnd(&i) ? MscGetEntAttrib(&i, a) : NULL;
}


MscArcIter MscArcIterBegin(struct MscTag *m)
{
    MscArcIter i = { m->arcList->head };
    return i;
}


bool MscArcIterEnd(MscArcIter *i)
{
    return i->arc == NULL;
}


void MscNextArc(MscArcIter *i)
{
    i->arc = i->arc->next;
}


const char *MscGetArcSource(MscArcIter *i)
{
    return i->arc->src;
}


const char *MscGetArcDest(MscArcIter *i)
{
    return i->arc->dst;
}


MscArcType MscGetArcType(MscArcIter *i)
{
    return i->arc->type;
}


const char *MscGetArcAttrib(MscArcIter *i, MscAttribType a)
{
    struct MscAttribTag *attr = i->arc->attr;

    while(attr != NULL && attr->type != a)
    {
        attr = attr->next;
    }

    return attr != NULL ? attr->value : NULL;
}


unsigned int MscGetArcInputLine(MscArcIter *i)
{
    return i->arc->inputLine;
}


bool MscGetOptAsFloat(struct MscTag *m, MscOptType type, float *const f)
{
    struct MscOptTag *opt = MscFindOpt(m->optList, type);

    if(opt != NULL)
    {
        *f = (float)atof(opt->value);
        return *f != 0.0f;
    }

    return false;
}


bool MscGetOptAsBoolean(struct MscTag *m, MscOptType type, bool *const b)
{
    struct MscOptTag *opt = MscFindOpt(m->optList, type);

    if(opt != NULL)
    {
        const char *v = opt->value;

        if(strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 ||
           strcasecmp(v, "on") == 0 || strcasecmp(v, "1") == 0)
        {
            *b = true;
            return true;
        }
        else if(strcasecmp(v, "false") == 0 || strcasecmp(v, "no") == 0 ||
                strcasecmp(v, "off") == 0 || strcasecmp(v, "0") == 0)
        {
            return *b = false;
            return true;
        }
        else
        {
            fprintf(stderr, "Warning: Unrecognised boolean option value '%s'.  Valid values are 'true',\n"
                            "         'false', 'yes', 'no', 'on', 'off', '1' and '0'.\n",
                            v);
            return false;
        }
    }

    return false;
}

/* END OF FILE */
