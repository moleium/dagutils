// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include <math/namemap.h>
#include <memory/dag_mem.h>
#include "datablock.h"

TMatrix TMatrix::IDENT(1), TMatrix::ZERO(0);

static void makeFullPathFromRelative(String &path, const char *base_filename)
{
  if (path.empty() || !base_filename)
    return;

  if (path[0] == '/' || path[0] == '\\')
    return;

  const char *last_slash = strrchr(base_filename, '/');
  const char *last_bslash = strrchr(base_filename, '\\');
  const char *last_colon = strrchr(base_filename, ':');
  const char *last = last_slash > last_bslash ? last_slash : last_bslash;
  if (last_colon > last)
    last = last_colon;

  if (last)
  {
    path.insert(0, base_filename, last - base_filename + 1);
  }
}


#define EOF_CHAR 0


class DataBlockParser
{
public:
  struct SyntaxErrorException
  {
    const char *msg;

    SyntaxErrorException(const char *s) : msg(s) {}
  };

  Tab<char> &buffer;

  const char *text, *curp, *textend;
  const char *fileName;
  int curLine;

  Tab<String> includeStack;

  DataBlockParser(Tab<char> &buf, const char *fn) :
    buffer(buf), text(buf.data()), curp(buf.data()), textend(buf.data() + buf.size() - 1), curLine(1), fileName(fn)
  {
    for (int i = 0; i < buffer.size(); ++i)
      if (buffer[i] == EOF_CHAR)
        buffer[i] = ' ';
    includeStack.push_back(String(fn));
  }

  void updatePointers()
  {
    int pos = curp - text;
    text = buffer.data();
    textend = text + buffer.size() - 1;
    curp = text + pos;
  }

  __forceinline void syntaxError(const char *msg) { throw SyntaxErrorException(msg); }

  __forceinline bool endOfText() { return curp >= textend; }

  void skipWhite();
  bool getIdent(String &);
  void getValue(String &);
  void parse(DataBlock &, bool isTop);
};


void DataBlockParser::skipWhite()
{
  for (;;)
  {
    if (endOfText())
      break;

    char c = *curp++;

    if (c == EOF_CHAR)
    {
      if (!includeStack.empty())
      {
        includeStack.pop_back();
        if (!includeStack.empty())
          fileName = includeStack.back().c_str();
      }
      continue;
    }

    if (c == '\r')
    {
      if (!endOfText() && *curp == '\n')
      {
        ++curp;
        ++curLine;
      }
      continue;
    }
    else if (c == '\n')
    {
      ++curLine;
      continue;
    }

    if (c == ' ' || c == '\t' || c == '\x1A')
      continue;
    else if (c == '/')
    {
      if (!endOfText())
      {
        char nc = *curp++;
        if (nc == '/')
        {
          while (!endOfText())
          {
            char c2 = *curp++;
            if (c2 == '\r' || c2 == '\n')
              break;
          }
          continue;
        }
        else if (nc == '*')
        {
          int cnt = 1;
          while (curp + 2 <= textend)
          {
            if (curp[0] == '/' && curp[1] == '*')
            {
              curp += 2;
              ++cnt;
            }
            else if (curp[0] == '*' && curp[1] == '/')
            {
              curp += 2;
              if (--cnt <= 0)
                break;
            }
            else
              ++curp;
          }
          continue;
        }
        else
          --curp;
      }

      --curp;
      break;
    }
    else
    {
      --curp;
      break;
    }
  }
}


bool DataBlockParser::getIdent(String &name)
{
  for (;;)
  {
    skipWhite();

    if (endOfText())
      break;

    char c = *curp;
    if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
    {
      const char *ident = curp;
      for (++curp; !endOfText(); ++curp)
      {
        c = *curp;
        if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
          break;
      }
      int len = curp - ident;
      name.setStr(ident, len);
      return true;
    }
    else
      break;
  }
  return false;
}


void DataBlockParser::getValue(String &value)
{
  value.clear();

  const char *valptr = curp;
  char qc = 0;
  if (*valptr == '"' || *valptr == '\'')
  {
    qc = *valptr++;
    ++curp;
  }

  for (;;)
  {
    if (endOfText())
      syntaxError("unexpected EOF");

    char c = *curp;

    if (qc)
    {
      if (c == qc)
      {
        ++curp;
        skipWhite();
        if (*curp == ';')
          ++curp;
        break;
      }
      else if (c == '\r' || c == '\n' || c == EOF_CHAR)
        syntaxError("unclosed string");
      else if (c == '~')
      {
        ++curp;

        if (endOfText())
          syntaxError("unclosed string");

        c = *curp;
        if (c == 'r')
          c = '\r';
        else if (c == 'n')
          c = '\n';
        else if (c == 't')
          c = '\t';
      }
    }
    else
    {
      if (c == ';' || c == '\r' || c == '\n' || c == EOF_CHAR)
      {
        if (c == ';')
          ++curp;
        break;
      }
    }

    value.push_back(c);
    ++curp;
  }

  if (!qc)
  {
    int i;
    for (i = value.size() - 1; i >= 0; --i)
      if (value[i] != ' ' && value[i] != '\t')
        break;
    ++i;
    if (i < value.size())
      value.resize(i);
  }
  value.push_back('\0');
  value.pop_back(); // Keep buffer as c-string for caller but length correct
}


void DataBlockParser::parse(DataBlock &blk, bool isTop)
{
  for (;;)
  {
    skipWhite();

    if (endOfText())
      break;


    if (*curp == '}')
    {
      if (isTop)
        syntaxError("unexpected '}' in top block");
      ++curp;
      break;
    }

    const char *start = curp;

    String name;
    if (!getIdent(name))
      syntaxError("expected identifier");

    skipWhite();
    if (endOfText())
      syntaxError("unexpected EOF");

    if (*curp == '{')
    {
      ++curp;
      DataBlock *nb = new DataBlock(&blk);
      nb->setBlockName(name);
      blk.addBlock(nb);
      parse(*nb, false);
    }
    else if (*curp == ':')
    {
      ++curp;
      String typeName;
      if (!getIdent(typeName))
        syntaxError("expected type identifier");

      int type = DataBlock::TYPE_NONE;
      if (typeName.length() == 1)
      {
        if (typeName[0] == 't')
          type = DataBlock::TYPE_STRING;
        else if (typeName[0] == 'i')
          type = DataBlock::TYPE_INT;
        else if (typeName[0] == 'b')
          type = DataBlock::TYPE_BOOL;
        else if (typeName[0] == 'c')
          type = DataBlock::TYPE_E3DCOLOR;
        else if (typeName[0] == 'r')
          type = DataBlock::TYPE_REAL;
        else if (typeName[0] == 'm')
          type = DataBlock::TYPE_MATRIX;
        else
          syntaxError("unknown type ");
      }
      else if (typeName.length() == 2)
      {
        if (typeName[0] == 'p')
        {
          if (typeName[1] == '2')
            type = DataBlock::TYPE_POINT2;
          else if (typeName[1] == '3')
            type = DataBlock::TYPE_POINT3;
          else if (typeName[1] == '4')
            type = DataBlock::TYPE_POINT4;
          else
            syntaxError("unknown type");
        }
        else
          syntaxError("unknown type");
      }
      else if (typeName.length() == 3)
      {
        if (typeName[0] == 'i')
        {
          if (typeName[1] == 'p')
          {
            if (typeName[2] == '2')
              type = DataBlock::TYPE_IPOINT2;
            else if (typeName[2] == '3')
              type = DataBlock::TYPE_IPOINT3;
            else
              syntaxError("unknown type");
          }
          else
            syntaxError("unknown type");
        }
        else
          syntaxError("unknown type");
      }
      else
        syntaxError("unknown type");

      skipWhite();

      if (endOfText())
        syntaxError("unexpected EOF");

      if (*curp++ != '=')
        syntaxError("expected '='");

      skipWhite();

      if (endOfText())
        syntaxError("unexpected EOF");

      String value;
      getValue(value);
      blk.addParam(name, type, value, curLine, fileName);
    }
    else if (stricmp(name, "include") == 0)
    {
      String value;
      getValue(value);

      int offset = start - text;
      int count = curp - start;
      buffer.erase(buffer.begin() + offset, buffer.begin() + offset + count);
      curp = start;

      makeFullPathFromRelative(value, fileName);

      const char *baseFileName = fileName;

      includeStack.push_back(value);
      fileName = includeStack.back().c_str();

      FILE *h = fopen(value, "r+b");

      if (!h)
      {
        debug("can't open include file '%s' for '%s'\n", (const char *)value, baseFileName);
        syntaxError("can't open include file");
      }

      fseek(h, 0, SEEK_END);
      long len = ftell(h);
      fseek(h, 0, SEEK_SET);

      if (len < 0)
      {
        fclose(h);
        syntaxError("error loading include file");
      }

      Tab<char> buf;
      buf.resize(len);

      if (fread(buf.data(), len, 1, h) != 1)
      {
        fclose(h);
        syntaxError("error loading include file");
      }
      fclose(h);

      int pos = curp - text;
      buffer.insert(buffer.begin() + pos, buf.begin(), buf.end());

      for (int i = 0; i < len; ++i)
        if (buffer[pos + i] == EOF_CHAR)
          buffer[pos + i] = ' ';
      buffer.insert(buffer.begin() + pos + len, EOF_CHAR);


      updatePointers();
    }
    else
      syntaxError("syntax error");
  }
}

char *create_buffer_str(const char *s)
{
  if (!s) s = "";
  size_t n = strlen(s) + 1;
  char *p = (char *)memalloc(n, strmem);
  memcpy(p, s, n);
  return p;
}


DataBlock *DataBlock::emptyBlock = NULL;
// static const int currentVersion = _MAKE4C('1.1');//_MAKE4C('1.0');

DataBlock::DataBlock(const DataBlock *blk) : nameId(-1), nameMap(blk->nameMap), valid(blk->valid), dataSrc(blk->dataSrc) {}


void DataBlock::setBlockName(const char *name)
{
  // G_ASSERT(nameMap);
  nameId = nameMap->addNameId(name);
}

int DataBlock::addBlock(DataBlock *blk)
{
  if (!blk)
    return -1;
  blocks.push_back(blk);
  return blocks.size() - 1;
}

int DataBlock::addParam(const char *name, int type, const char *value, int line, const char *filename)
{
  params.emplace_back();
  Param &p = params.back();

  // G_ASSERT(nameMap);
  p.nameId = nameMap->addNameId(name);
  p.type = type;
  switch (type)
  {
    case TYPE_STRING:
    {
      p.value.s = create_buffer_str(value);
    }
    break;
    case TYPE_INT: p.value.i = strtol(value, NULL, 0); break;
    case TYPE_REAL: p.value.r = strtod(value, NULL); break;
    case TYPE_POINT2:
    {
      p.value.p2 = Point2(0.f, 0.f);
      int res = sscanf(value, " %f , %f", &p.value.p2.x, &p.value.p2.y);
      if (res != 2)
        debug("invalid point2 value in line %d of '%s'\n", line, filename);
    }
    break;
    case TYPE_POINT3:
    {
      p.value.p3 = Point3(0.f, 0.f, 0.f);
      int res = sscanf(value, " %f , %f , %f", &p.value.p3.x, &p.value.p3.y, &p.value.p3.z);
      if (res != 3)
        debug("invalid point3 value in line %d of '%s'\n", line, filename);
    }
    break;
    case TYPE_POINT4:
    {
      p.value.p4 = Point4(0.f, 0.f, 0.f, 0.f);
      int res = sscanf(value, " %f , %f , %f , %f", &p.value.p4.x, &p.value.p4.y, &p.value.p4.z, &p.value.p4.w);
      if (res != 4)
        debug("invalid point4 value in line %d of '%s'\n", line, filename);
    }
    break;
    case TYPE_IPOINT2:
    {
      p.value.ip2 = IPoint2(0, 0);
      int res = sscanf(value, " %i , %i", &p.value.ip2.x, &p.value.ip2.y);
      if (res != 2)
        debug("invalid ipoint2 value in line %d of '%s'\n", line, filename);
    }
    break;
    case TYPE_IPOINT3:
    {
      p.value.ip3 = IPoint3(0, 0, 0);
      int res = sscanf(value, " %i , %i , %i", &p.value.ip3.x, &p.value.ip3.y, &p.value.ip3.z);
      if (res != 3)
        debug("invalid ipoint3 value in line %d of '%s'\n", line, filename);
    }
    break;
    case TYPE_BOOL:
    {
      if (stricmp(value, "yes") == 0 || stricmp(value, "on") == 0 || stricmp(value, "true") == 0 || stricmp(value, "1") == 0)
        p.value.b = true;
      else if (stricmp(value, "no") == 0 || stricmp(value, "off") == 0 || stricmp(value, "false") == 0 || stricmp(value, "0") == 0)
        p.value.b = false;
      else
      {
        p.value.b = false;
        debug("invalid boolean value '%s' in line %d of '%s'\n", value, line, filename);
      }
    }
    break;
    case TYPE_E3DCOLOR:
    {
      int r = 255, g = 255, b = 255, a = 255;

      int res = sscanf(value, " %d , %d , %d , %d", &r, &g, &b, &a);
      //== check value range
      p.value.c.r = r;
      p.value.c.g = g;
      p.value.c.b = b;
      p.value.c.a = a;

      if (res < 3)
        debug("invalid e3dcolor value in line %d of '%s'\n", line, filename);
    }
    break;
    case TYPE_MATRIX:
    {
      p.value.tm = TMatrix::IDENT;
      int res = sscanf(value,
        "[[ %f , %f , %f ] [ %f , %f , %f ] "
        "[ %f , %f , %f ] [ %f , %f , %f ]]",
        &p.value.tm.m[0][0], &p.value.tm.m[0][1], &p.value.tm.m[0][2], &p.value.tm.m[1][0], &p.value.tm.m[1][1], &p.value.tm.m[1][2],
        &p.value.tm.m[2][0], &p.value.tm.m[2][1], &p.value.tm.m[2][2], &p.value.tm.m[3][0], &p.value.tm.m[3][1], &p.value.tm.m[3][2]);

      if (res != 12)
        debug("invalid TMatrix value in line %d of '%s'\n", line, filename);

      break;
    }
    default:
      debug("error");
      // G_ASSERT(0);
  }

  return params.size() - 1;
}


/*DLLEXPORT*/ DataBlock::DataBlock(const DataBlock &from) :
  nameMap(from.nameMap), nameId(from.nameId), valid(from.valid), dataSrc(from.dataSrc)
{
  setParamsFrom(&from);

  int num = from.blockCount();
  for (int i = 0; i < num; ++i)
    addNewBlock(from.getBlock(i));
}


/*DLLEXPORT*/ DataBlock *DataBlock::addBlock(const char *name)
{
  DataBlock *blk = getBlockByName(getNameId(name));
  if (blk)
    return blk;

  return addNewBlock(name);
}


/*DLLEXPORT*/ DataBlock *DataBlock::addNewBlock(const char *name)
{
  DataBlock *nb = new DataBlock(this);
  nb->setBlockName(name);
  addBlock(nb);
  return nb;
}


/*DLLEXPORT*/ bool DataBlock::removeBlock(const char *name)
{
  int nameId = getNameId(name);
  if (nameId < 0)
    return false;

  bool removed = false;

  for (int i = blocks.size() - 1; i >= 0; --i)
    if (blocks[i] && blocks[i]->getBlockNameId() == nameId)
    {
      delete (blocks[i]);
      blocks.erase(blocks.begin() + i);
      removed = true;
    }

  return removed;
}


/*DLLEXPORT*/ bool DataBlock::removeParam(const char *name)
{
  int nameId = getNameId(name);
  if (nameId < 0)
    return false;

  bool removed = false;

  for (int i = params.size() - 1; i >= 0; --i)
    if (params[i].nameId == nameId)
    {
      params.erase(params.begin() + i);
      removed = true;
    }

  return removed;
}


/*DLLEXPORT*/ void DataBlock::setParamsFrom(const DataBlock *blk)
{
  if (!blk)
    return;

  params.clear();

  int num = blk->paramCount();
  for (int i = 0; i < num; ++i)
  {
    const char *name = blk->getParamName(i);

    switch (blk->getParamType(i))
    {
      case TYPE_STRING: addStr(name, blk->getStr(i)); break;
      case TYPE_INT: addInt(name, blk->getInt(i)); break;
      case TYPE_REAL: addReal(name, blk->getReal(i)); break;
      case TYPE_POINT2: addPoint2(name, blk->getPoint2(i)); break;
      case TYPE_POINT3: addPoint3(name, blk->getPoint3(i)); break;
      case TYPE_POINT4: addPoint4(name, blk->getPoint4(i)); break;
      case TYPE_IPOINT2: addIPoint2(name, blk->getIPoint2(i)); break;
      case TYPE_IPOINT3: addIPoint3(name, blk->getIPoint3(i)); break;
      case TYPE_BOOL: addBool(name, blk->getBool(i)); break;
      case TYPE_E3DCOLOR: addE3dcolor(name, blk->getE3dcolor(i)); break;
      case TYPE_MATRIX: addTm(name, blk->getTm(i)); break;
      default:
        debug("error");
        // G_ASSERT(0);
    }
  }
}


/*DLLEXPORT*/ DataBlock *DataBlock::addNewBlock(const DataBlock *blk, const char *as_name)
{
  if (!blk)
    return NULL;

  DataBlock *newBlk = addNewBlock(as_name ? as_name : blk->getBlockName());
  // G_ASSERT(newBlk);

  newBlk->setParamsFrom(blk);

  // add sub-blocks
  int num = blk->blockCount();
  for (int i = 0; i < num; ++i)
    newBlk->addNewBlock(blk->getBlock(i));

  return newBlk;
}


void DataBlock::setFrom(const DataBlock *from)
{
  clearData();

  if (!from)
    return;

  setParamsFrom(from);

  for (int i = 0; i < from->blockCount(); ++i)
    addNewBlock(from->getBlock(i));
}


/*DLLEXPORT*/ int DataBlock::setStr(const char *name, const char *value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_STRING)
    return addStr(name, value);

  memfree(params[id].value.s, strmem);
  params[id].value.s = create_buffer_str(value);
  return id;
}

/*DLLEXPORT*/ int DataBlock::setBool(const char *name, bool value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_BOOL)
    return addBool(name, value);

  params[id].value.b = value;

  return id;
}

/*DLLEXPORT*/ int DataBlock::setInt(const char *name, int value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_INT)
    return addInt(name, value);

  params[id].value.i = value;

  return id;
}

/*DLLEXPORT*/ int DataBlock::setReal(const char *name, real value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_REAL)
    return addReal(name, value);

  params[id].value.r = value;

  return id;
}

int DataBlock::setPoint2(const char *name, const Point2 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_POINT2)
    return addPoint2(name, value);

  params[id].value.p2 = value;

  return id;
}

int DataBlock::setPoint3(const char *name, const Point3 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_POINT3)
    return addPoint3(name, value);

  params[id].value.p3 = value;

  return id;
}

int DataBlock::setPoint4(const char *name, const Point4 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_POINT4)
    return addPoint4(name, value);

  params[id].value.p4 = value;

  return id;
}

int DataBlock::setIPoint2(const char *name, const IPoint2 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_IPOINT2)
    return addIPoint2(name, value);

  params[id].value.ip2 = value;

  return id;
}

int DataBlock::setIPoint3(const char *name, const IPoint3 &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_IPOINT3)
    return addIPoint3(name, value);

  params[id].value.ip3 = value;

  return id;
}

/*DLLEXPORT*/ int DataBlock::setE3dcolor(const char *name, const E3DCOLOR value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_E3DCOLOR)
    return addE3dcolor(name, value);

  params[id].value.c = value;

  return id;
}


int DataBlock::setTm(const char *name, const TMatrix &value)
{
  int id = findParam(name);
  if (id < 0 || params[id].type != TYPE_MATRIX)
    return addTm(name, value);

  params[id].value.tm = value;
  return id;
}


/*DLLEXPORT*/ int DataBlock::addStr(const char *name, const char *value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_STRING;
  p.value.s = create_buffer_str(value);
  return params.size() - 1;
}

/*DLLEXPORT*/ int DataBlock::addBool(const char *name, bool value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_BOOL;
  p.value.b = value;
  return params.size() - 1;
}

/*DLLEXPORT*/ int DataBlock::addInt(const char *name, int value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_INT;
  p.value.i = value;
  return params.size() - 1;
}

/*DLLEXPORT*/ int DataBlock::addReal(const char *name, real value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_REAL;
  p.value.r = value;
  return params.size() - 1;
}

int DataBlock::addPoint2(const char *name, const Point2 &value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_POINT2;
  p.value.p2 = value;
  return params.size() - 1;
}

int DataBlock::addPoint3(const char *name, const Point3 &value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_POINT3;
  p.value.p3 = value;
  return params.size() - 1;
}

int DataBlock::addPoint4(const char *name, const Point4 &value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_POINT4;
  p.value.p4 = value;
  return params.size() - 1;
}

int DataBlock::addIPoint2(const char *name, const IPoint2 &value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_IPOINT2;
  p.value.ip2 = value;
  return params.size() - 1;
}

int DataBlock::addIPoint3(const char *name, const IPoint3 &value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_IPOINT3;
  p.value.ip3 = value;
  return params.size() - 1;
}

/*DLLEXPORT*/ int DataBlock::addE3dcolor(const char *name, const E3DCOLOR value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_E3DCOLOR;
  p.value.c = value;
  return params.size() - 1;
}

/*DLLEXPORT*/ int DataBlock::addTm(const char *name, const TMatrix &value)
{
  params.emplace_back();
  Param &p = params.back();
  p.nameId = nameMap->addNameId(name);
  p.type = TYPE_MATRIX;
  p.value.tm = value;
  return params.size() - 1;
}


/*DLLEXPORT*/ DataBlock::DataBlock() : nameId(-1), nameMap(NULL), valid(true), dataSrc(SRC_UNKNOWN) { nameMap = new NameMap; }

/*DLLEXPORT*/ DataBlock::~DataBlock()
{
  nameId = -1;
  clearData();
  delete nameMap;
  nameMap = NULL;
}

/*DLLEXPORT*/ DataBlock::DataBlock(const char *filename) : nameId(-1), nameMap(NULL), valid(true), dataSrc(SRC_UNKNOWN)
{
  nameMap = new NameMap;
  load(filename);
}

// reset class (clear all data & names)
/*DLLEXPORT*/ void DataBlock::reset()
{
  nameId = -1;
  delete nameMap;
  nameMap = new NameMap;
  clearData();
}


// delete all children
/*DLLEXPORT*/ void DataBlock::clearData()
{
  params.clear();
  for (int i = 0; i < blocks.size(); ++i)
    delete blocks[i];
  blocks.clear();
}


/*DLLEXPORT*/ bool DataBlock::loadText(Tab<char> &text, const char *filename)
{
  reset();

  text.push_back('\0');
  debug("text %i", text.size());
  DataBlockParser parser(text, filename);

  try
  {
    parser.parse(*this, true);
  }
  catch (DataBlockParser::SyntaxErrorException e)
  {
    debug("DataBlock error in line %d of '%s':\n  %s\n", parser.curLine, filename ? filename : "<unknown>", e.msg);

    if (!paramCount())
      reset();

    valid = false;
    return false;
  }

  valid = true;
  return true;
}

void DataBlock::load(FILE *cb, class NameMap &stringMap)
{
}

/*DLLEXPORT*/ bool DataBlock::loadText(char *text, int len, const char *filename)
{
  Tab<char> buf;
  buf.insert(buf.end(), text, text + len);
  buf.push_back('\0');

  return loadText(buf, filename);
}


bool DataBlock::load(const char *fname)
{
  reset();
  if (!fname || !*fname)
  {
    valid = false;
    return false;
  }

  String fileName(fname);
  FILE *h = fopen(fileName, "r+b");
  if (!h)
  {
    debug("can't open include file '%s'\n", (const char *)fileName);
    valid = false;
    return false;
  }
  int len = filelength(fileno(h));
  if (len == -1L || len < 0)
  {
    fclose(h);
    debug("error loading include file");
    valid = false;
    return false;
  }
  return loadFromStream(h, fileName);
}


bool DataBlock::loadFromStream(FILE *f, const char *fname)
{
  reset();
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (len <= 0)
  {
    fclose(f);
    return true;
  }

  Tab<char> text;
  text.resize(len);
  if (fread(text.data(), len, 1, f) != 1)
  {
    debug("unable to read file to buf");
    reset();
    fclose(f);
    valid = false;
    return false;
  }

  bool res = loadText(text, fname);
  fclose(f);
  return res;
}


void DataBlock::doLoadFromStream(FILE *crd)
{
}


// Saving

static void writeIndent(FILE *cb, int n)
{
  if (n <= 0)
    return;
  for (; n >= 8; n -= 8)
    fwrite("        ", 8, 1, cb);
  for (; n >= 2; n -= 2)
    fwrite("  ", 2, 1, cb);
  for (; n >= 1; n--)
    fwrite(" ", 1, 1, cb);
}

static void writeString(FILE *cb, const char *s)
{
  if (!s || !*s)
    return;
  int l = (int)strlen(s);
  fwrite(s, l, 1, cb);
}

static void writeStringValue(FILE *cb, const char *s)
{
  if (!s)
    s = "";

  fwrite("\"", 1, 1, cb);

  for (; *s; ++s)
  {
    char c = *s;
    if (c == '~')
      fwrite("~~", 2, 1, cb);
    else if (c == '"')
      fwrite("~\"", 2, 1, cb);
    else if (c == '\r')
      fwrite("~r", 2, 1, cb);
    else if (c == '\n')
      fwrite("~n", 2, 1, cb);
    else if (c == '\t')
      fwrite("~t", 2, 1, cb);
    else
      fwrite(&c, 1, 1, cb);
  }

  fwrite("\"", 1, 1, cb);
}

/*DLLEXPORT*/ void DataBlock::save(FILE *cb, class NameMap &stringMap) const
{
}


/*DLLEXPORT*/ void DataBlock::saveText(FILE *cb, int level) const
{
  int i;
  for (i = 0; i < params.size(); ++i)
  {
    const Param &p = params[i];

    writeIndent(cb, level * 2);
    writeString(cb, getName(p.nameId));
    switch (p.type)
    {
      case TYPE_STRING:
        writeString(cb, ":t=");
        writeStringValue(cb, p.value.s);
        break;
      case TYPE_BOOL:
      {
        writeString(cb, ":b=");
        char buf[32];
        sprintf(buf, "%s", p.value.b ? "yes" : "no");
        writeString(cb, buf);
      }
      break;
      case TYPE_INT:
      {
        writeString(cb, ":i=");
        char buf[32];
        sprintf(buf, "%d", p.value.i);
        writeString(cb, buf);
      }
      break;
      case TYPE_REAL:
      {
        writeString(cb, ":r=");
        char buf[64];
        sprintf(buf, "%g", p.value.r);
        writeString(cb, buf);
      }
      break;
      case TYPE_POINT2:
      {
        writeString(cb, ":p2=");
        char buf[128];
        sprintf(buf, "%g, %g", p.value.p2.x, p.value.p2.y);
        writeString(cb, buf);
      }
      break;
      case TYPE_POINT3:
      {
        writeString(cb, ":p3=");
        char buf[128];
        sprintf(buf, "%g, %g, %g", p.value.p3.x, p.value.p3.y, p.value.p3.z);
        writeString(cb, buf);
      }
      break;
      case TYPE_POINT4:
      {
        writeString(cb, ":p4=");
        char buf[160];
        sprintf(buf, "%g, %g, %g, %g", p.value.p4.x, p.value.p4.y, p.value.p4.z, p.value.p4.w);
        writeString(cb, buf);
      }
      break;
      case TYPE_IPOINT2:
      {
        writeString(cb, ":ip2=");
        char buf[128];
        sprintf(buf, "%d, %d", p.value.ip2.x, p.value.ip2.y);
        writeString(cb, buf);
      }
      break;
      case TYPE_IPOINT3:
      {
        writeString(cb, ":ip3=");
        char buf[128];
        sprintf(buf, "%d, %d, %d", p.value.ip3.x, p.value.ip3.y, p.value.ip3.z);
        writeString(cb, buf);
      }
      break;
      case TYPE_E3DCOLOR:
      {
        writeString(cb, ":c=");
        char buf[128];
        sprintf(buf, "%d, %d, %d, %d", p.value.c.r, p.value.c.g, p.value.c.b, p.value.c.a);
        writeString(cb, buf);
      }
      break;
      case TYPE_MATRIX:
      {
        writeString(cb, ":m=");
        char buf[256];
        sprintf(buf, "[[%g, %g, %g] [%g, %g, %g] [%g, %g, %g] [%g, %g, %g]]", p.value.tm.getcol(0).x, p.value.tm.getcol(0).y,
          p.value.tm.getcol(0).z, p.value.tm.getcol(1).x, p.value.tm.getcol(1).y, p.value.tm.getcol(1).z, p.value.tm.getcol(2).x,
          p.value.tm.getcol(2).y, p.value.tm.getcol(2).z, p.value.tm.getcol(3).x, p.value.tm.getcol(3).y, p.value.tm.getcol(3).z);
        writeString(cb, buf);
        break;
      }
      default: debug("unknown type");
    }
    fwrite("\r\n", 2, 1, cb);
  }

  if (!params.empty() && !blocks.empty())
  {
    writeIndent(cb, level * 2);
    fwrite("\r\n", 2, 1, cb);
  }
  for (i = 0; i < blocks.size(); ++i)
  {
    DataBlock &b = *blocks[i];
    if (!&b)
      continue;

    writeIndent(cb, level * 2);
    writeString(cb, getName(b.nameId));
    fwrite("{\r\n", 3, 1, cb);

    b.saveText(cb, level + 1);

    writeIndent(cb, level * 2);
    fwrite("}\r\n", 3, 1, cb);

    if (i != blocks.size() - 1)
      fwrite("\r\n", 2, 1, cb);
  }
}

/*DLLEXPORT*/ bool DataBlock::saveToTextFile(const char *filename) const
{
  String fileName(filename);
  FILE *h = fopen(fileName, "w+b");

  if (!h)
  {
    debug("cant open '%s' file for writing", filename);
    return false;
  }

  saveText(h);
  fclose(h);
  return true;
}


/*DLLEXPORT*/ void DataBlock::fillNameMap(NameMap *stringMap) const
{
  if (!stringMap)
    return;

  int i;
  for (i = 0; i < params.size(); ++i)
  {
    const Param &p = params[i];
    if (p.type != TYPE_STRING)
      continue;
    stringMap->addNameId(p.value.s);
  }

  for (i = 0; i < blocks.size(); ++i)
  {
    DataBlock &b = *blocks[i];
    if (!&b)
      continue;
    b.fillNameMap(stringMap);
  }
}


bool DataBlock::saveToBinaryFile(const char *filename) const
{
  return true;
}


// Names

/*DLLEXPORT*/ int DataBlock::getNameId(const char *name) const
{
  if (!nameMap)
    return -1;
  return nameMap->getNameId(name);
}

/*DLLEXPORT*/ const char *DataBlock::getName(int nid) const
{
  if (!nameMap)
    return NULL;
  return nameMap->getName(nid);
}

// Sub-blocks

/*DLLEXPORT*/ DataBlock *DataBlock::getBlock(int i) const
{
  if (i < 0 || i >= blocks.size())
    return NULL;
  return blocks[i];
}

/*DLLEXPORT*/ DataBlock *DataBlock::getBlockByName(int nid, int after) const
{
  for (int i = after + 1; i < blocks.size(); ++i)
    if (blocks[i])
      if (blocks[i]->nameId == nid)
        return blocks[i];
  return NULL;
}

// Parameters

/*DLLEXPORT*/ int DataBlock::getParamType(int i) const
{
  if (i < 0 || i >= params.size())
    return TYPE_NONE;
  return params[i].type;
}

/*DLLEXPORT*/ int DataBlock::getParamNameId(int i) const
{
  if (i < 0 || i >= params.size())
    return -1;
  return params[i].nameId;
}

/*DLLEXPORT*/ const char *DataBlock::getStr(int i) const
{
  if (i < 0 || i >= params.size())
    return NULL;
  if (params[i].type != TYPE_STRING)
    return NULL;
  return params[i].value.s;
}

/*DLLEXPORT*/ int DataBlock::getInt(int i) const
{
  if (i < 0 || i >= params.size())
    return 0;
  if (params[i].type != TYPE_INT)
    return 0;
  return params[i].value.i;
}

/*DLLEXPORT*/ bool DataBlock::getBool(int i) const
{
  if (i < 0 || i >= params.size())
    return false;
  if (params[i].type != TYPE_BOOL)
    return false;
  return params[i].value.b;
}

/*DLLEXPORT*/ real DataBlock::getReal(int i) const
{
  if (i < 0 || i >= params.size())
    return 0;
  if (params[i].type != TYPE_REAL)
    return 0;
  return params[i].value.r;
}

Point2 DataBlock::getPoint2(int i) const
{
  if (i < 0 || i >= params.size())
    return Point2(0.f, 0.f);
  if (params[i].type != TYPE_POINT2)
    return Point2(0.f, 0.f);
  return params[i].value.p2;
}

Point3 DataBlock::getPoint3(int i) const
{
  if (i < 0 || i >= params.size())
    return Point3(0.f, 0.f, 0.f);
  if (params[i].type != TYPE_POINT3)
    return Point3(0.f, 0.f, 0.f);
  return params[i].value.p3;
}

Point4 DataBlock::getPoint4(int i) const
{
  if (i < 0 || i >= params.size())
    return Point4(0.f, 0.f, 0.f, 0.f);
  if (params[i].type != TYPE_POINT4)
    return Point4(0.f, 0.f, 0.f, 0.f);
  return params[i].value.p4;
}

IPoint2 DataBlock::getIPoint2(int i) const
{
  if (i < 0 || i >= params.size())
    return IPoint2(0, 0);
  if (params[i].type != TYPE_IPOINT2)
    return IPoint2(0, 0);
  return params[i].value.ip2;
}

IPoint3 DataBlock::getIPoint3(int i) const
{
  if (i < 0 || i >= params.size())
    return IPoint3(0, 0, 0);
  if (params[i].type != TYPE_IPOINT3)
    return IPoint3(0, 0, 0);
  return params[i].value.ip3;
}

/*DLLEXPORT*/ E3DCOLOR DataBlock::getE3dcolor(int i) const
{
  if (i < 0 || i >= params.size())
    return E3DCOLOR(0, 0, 0, 0);
  if (params[i].type != TYPE_E3DCOLOR)
    return E3DCOLOR(0, 0, 0, 0);
  return params[i].value.c;
}


TMatrix DataBlock::getTm(int i) const
{
  if (i < 0 || i >= params.size())
    return TMatrix::IDENT;
  if (params[i].type != TYPE_MATRIX)
    return TMatrix::IDENT;
  return params[i].value.tm;
}


/*DLLEXPORT*/ int DataBlock::findParam(int nid, int after) const
{
  for (int i = after + 1; i < params.size(); ++i)
    if (params[i].nameId == nid)
      return i;
  return -1;
}

/*DLLEXPORT*/ const char *DataBlock::getStr(const char *name, const char *def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_STRING)
    return def;
  return params[i].value.s;
}

/*DLLEXPORT*/ int DataBlock::getInt(const char *name, int def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_INT)
    return def;
  return params[i].value.i;
}

/*DLLEXPORT*/ bool DataBlock::getBool(const char *name, bool def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_BOOL)
    return def;
  return params[i].value.b;
}

/*DLLEXPORT*/ real DataBlock::getReal(const char *name, real def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_REAL)
    return def;
  return params[i].value.r;
}

Point2 DataBlock::getPoint2(const char *name, const Point2 &def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_POINT2)
    return def;
  return params[i].value.p2;
}

Point3 DataBlock::getPoint3(const char *name, const Point3 &def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_POINT3)
    return def;
  return params[i].value.p3;
}

Point4 DataBlock::getPoint4(const char *name, const Point4 &def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_POINT4)
    return def;
  return params[i].value.p4;
}

IPoint2 DataBlock::getIPoint2(const char *name, const IPoint2 &def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_IPOINT2)
    return def;
  return params[i].value.ip2;
}

IPoint3 DataBlock::getIPoint3(const char *name, const IPoint3 &def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_IPOINT3)
    return def;
  return params[i].value.ip3;
}

/*DLLEXPORT*/ E3DCOLOR DataBlock::getE3dcolor(const char *name, E3DCOLOR def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_E3DCOLOR)
    return def;
  return params[i].value.c;
}


TMatrix DataBlock::getTm(const char *name, TMatrix &def) const
{
  int i = findParam(name);
  if (i < 0)
    return def;
  if (params[i].type != TYPE_MATRIX)
    return def;
  return params[i].value.tm;
}


/*DLLEXPORT*/ DataBlock::Param::Param() : nameId(-1), type(TYPE_NONE) { memset(&value, 0, sizeof(value)); }

/*DLLEXPORT*/ DataBlock::Param::~Param()
{
  if (type == TYPE_STRING)
    memfree(value.s, strmem);
}