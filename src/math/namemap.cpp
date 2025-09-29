// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <string.h>
#include <stdlib.h>
#include "namemap.h"
#include "debug/dag_debug.h"
#include "util/dag_string.h"
#include "memory/dag_mem.h"

BaseNameMap::BaseNameMap() {}

BaseNameMap::~BaseNameMap() { clear(); }


void BaseNameMap::clear() {
    names.clear();
}


void BaseNameMap::save(FILE *f) const
{
  int n = names.size();
  fwrite(&n, sizeof(n), 1, f);

  for (int i = 0; i < names.size(); ++i)
  {
    int len = (int)strlen(names[i]);
    if (len >= (1 << 16))
      debug("NameMap string is too long to save");
    unsigned short l = len;
    fwrite(&l, sizeof(l), 1, f);
    if (l)
      fwrite(names[i].c_str(), l, 1, f);
  }
}


void BaseNameMap::load(FILE *f)
{
  clear();

  int n;
  fread(&n, sizeof(n), 1, f);
  names.resize(n);

  for (int i = 0; i < names.size(); ++i)
  {
    unsigned short l;
    fread(&l, sizeof(l), 1, f);
    char *s = (char *)memalloc(l + 1, strmem);

    if (l)
      fread(s, l, 1, f);
    s[l] = 0;
    
    names[i] = s;

    memfree(s, strmem);
  }
}


const char *BaseNameMap::getName(int i) const
{
  if (i < 0 || i >= names.size())
    return NULL;
  return names[i].c_str();
}


void BaseNameMap::copyFrom(const BaseNameMap &nm)
{
  names = nm.names;
}


int NameMap::getNameId(const char *name) const
{
  if (!name)
    return -1;

  for (int i = 0; i < names.size(); ++i)
    if (names[i] == name)
      return i;
  return -1;
}


int NameMap::addNameId(const char *name)
{
  if (!name)
    return -1;

  int i;
  for (i = 0; i < names.size(); ++i)
    if (names[i] == name)
      return i;

  names.push_back(String(name));
  return names.size() - 1;
}


int NameMapCI::getNameId(const char *name) const
{
  if (!name)
    return -1;

  for (int i = 0; i < names.size(); ++i)
    if (stricmp(names[i].c_str(), name) == 0)
      return i;
  return -1;
}


int NameMapCI::addNameId(const char *name)
{
  if (!name)
    return -1;

  int i;
  for (i = 0; i < names.size(); ++i)
    if (stricmp(names[i].c_str(), name) == 0)
      return i;

  names.push_back(String(name));
  return names.size() - 1;
}