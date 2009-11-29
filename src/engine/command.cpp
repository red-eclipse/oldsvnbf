// command.cpp: implements the parsing and execution of a tiny script language which
// is largely backwards compatible with the quake console language.

#include "engine.h"

identtable *idents = NULL;		// contains ALL vars/commands/aliases

bool overrideidents = false, persistidents = true, worldidents = false, interactive = false;

void clearstack(ident &id)
{
	identstack *stack = id.stack;
	while(stack)
	{
		delete[] stack->action;
		identstack *tmp = stack;
		stack = stack->next;
		delete tmp;
	}
	id.stack = NULL;
}

void clear_command()
{
	enumerate(*idents, ident, i, if(i.type==ID_ALIAS) { DELETEA(i.name); DELETEA(i.action); if(i.stack) clearstack(i); });
	if(idents) idents->clear();
}

void clearoverride(ident &i)
{
    if(i.override==NO_OVERRIDE) return;
	switch(i.type)
	{
		case ID_ALIAS:
			if(i.action[0])
			{
				if(i.action != i.isexecuting) delete[] i.action;
				i.action = newstring("");
			}
			break;
		case ID_VAR:
			*i.storage.i = i.overrideval.i;
			i.changed();
			break;
		case ID_FVAR:
			*i.storage.f = i.overrideval.f;
			i.changed();
			break;
		case ID_SVAR:
			delete[] *i.storage.s;
			*i.storage.s = i.overrideval.s;
			i.changed();
			break;
	}
	i.override = NO_OVERRIDE;
}

void clearoverrides()
{
    enumerate(*idents, ident, i, clearoverride(i));
}

void pushident(ident &id, char *val)
{
	if(id.type != ID_ALIAS) return;
	identstack *stack = new identstack;
	stack->action = id.isexecuting==id.action ? newstring(id.action) : id.action;
	stack->next = id.stack;
	id.stack = stack;
	id.action = val;
}

void popident(ident &id)
{
	if(id.type != ID_ALIAS || !id.stack) return;
	if(id.action != id.isexecuting) delete[] id.action;
	identstack *stack = id.stack;
	id.action = stack->action;
	id.stack = stack->next;
	delete stack;
}

ident *newident(const char *name)
{
	ident *id = idents->access(name);
	if(!id)
	{
		int flags = IDF_COMPLETE;
		if(persistidents) flags |= IDF_PERSIST;
		if(worldidents) flags |= IDF_WORLD;
		ident init(ID_ALIAS, newstring(name), newstring(""), flags);
		id = &idents->access(init.name, init);
	}
	return id;
}

void pusha(const char *name, char *action)
{
	pushident(*newident(name), action);
}

void push(char *name, char *action)
{
	pusha(name, newstring(action));
}

void pop(char *name)
{
	ident *id = idents->access(name);
	if(id) popident(*id);
}

void resetvar(char *name)
{
    ident *id = idents->access(name);
    if(!id) return;
    //if(id->flags&IDF_READONLY) conoutft(CON_MESG, "variable %s is read-only", id->name);
    //else
    clearoverride(*id);
}

COMMAND(push, "ss");
COMMAND(pop, "s");
COMMAND(resetvar, "s");

void aliasa(const char *name, char *action)
{
	ident *b = idents->access(name);
	if(!b)
	{
		int flags = IDF_COMPLETE;
		if(persistidents) flags |= IDF_PERSIST;
		if(worldidents) flags |= IDF_WORLD;
		ident d(ID_ALIAS, newstring(name), action, flags);
		if(overrideidents && d.flags&IDF_OVERRIDE) d.override = OVERRIDDEN;
#ifdef STANDALONE
		idents->access(d.name, d);
#else
		ident &c = idents->access(d.name, d);
		client::editvar(&c, interactive && !overrideidents);
#endif
	}
	else if(b->type != ID_ALIAS)
	{
		conoutf("\frcannot redefine builtin %s with an alias", name);
		delete[] action;
	}
	else
	{
		if(b->action != b->isexecuting) delete[] b->action;
		b->action = action;
		if(overrideidents && b->flags&IDF_OVERRIDE) b->override = OVERRIDDEN;
		else
		{
			if(b->override != NO_OVERRIDE) b->override = NO_OVERRIDE;
			if(persistidents)
			{
				if(!(b->flags & IDF_PERSIST)) b->flags |= IDF_PERSIST;
			}
			else if(b->flags & IDF_PERSIST) b->flags &= ~IDF_PERSIST;
			if(b->type == ID_ALIAS && worldidents && !(b->flags & IDF_WORLD))
				b->flags |= IDF_WORLD;

#ifndef STANDALONE
			client::editvar(b, interactive && !overrideidents);
#endif
		}
	}
}

void alias(const char *name, const char *action) { aliasa(name, newstring(action)); }

COMMAND(alias, "ss");

void worldalias(const char *name, const char *action)
{
	worldidents = true;
	persistidents = false;
	alias(name, action);
	persistidents = true;
	worldidents = false;
}
COMMAND(worldalias, "ss");

// variable's and commands are registered through globals, see cube.h

int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), int flags)
{
	if(!idents) idents = new identtable;
	ident v(ID_VAR, name, min, cur, max, storage, (void *)fun, flags);
	idents->access(name, v);
	return cur;
}

float fvariable(const char *name, float min, float cur, float max, float *storage, void (*fun)(), int flags)
{
	if(!idents) idents = new identtable;
	ident v(ID_FVAR, name, min, cur, max, storage, (void *)fun, flags);
	idents->access(name, v);
	return cur;
}

char *svariable(const char *name, const char *cur, char **storage, void (*fun)(), int flags)
{
	if(!idents) idents = new identtable;
	ident v(ID_SVAR, name, newstring(cur), newstring(cur), storage, (void *)fun, flags);
	idents->access(name, v);
	return v.val.s;
}

#define _GETVAR(id, vartype, name, retval) \
	ident *id = idents->access(name); \
	if(!id || id->type!=vartype) return retval;
#define GETVAR(id, name, retval) _GETVAR(id, ID_VAR, name, retval)
void setvar(const char *name, int i, bool dofunc)
{
	GETVAR(id, name, );
	*id->storage.i = clamp(i, id->minval, id->maxval);
	if(dofunc) id->changed();
	if(verbose >= 4) conoutf("\fd%s set to %d", id->name, *id->storage.i);
}
void setfvar(const char *name, float f, bool dofunc)
{
	_GETVAR(id, ID_FVAR, name, );
	*id->storage.f = clamp(f, id->minvalf, id->maxvalf);
	if(dofunc) id->changed();
	if(verbose >= 4) conoutf("\fd%s set to %s", id->name, floatstr(*id->storage.f));
}
void setsvar(const char *name, const char *str, bool dofunc)
{
	_GETVAR(id, ID_SVAR, name, );
	*id->storage.s = exchangestr(*id->storage.s, str);
	if(dofunc) id->changed();
	if(verbose >= 4) conoutf("\fd%s set to %s", id->name, *id->storage.s);
}
int getvar(const char *name)
{
	GETVAR(id, name, 0);
	return *id->storage.i;
}
int getvarmin(const char *name)
{
	GETVAR(id, name, 0);
	return id->minval;
}
int getvarmax(const char *name)
{
	GETVAR(id, name, 0);
	return id->maxval;
}
bool identexists(const char *name) { return idents->access(name)!=NULL; }
ident *getident(const char *name) { return idents->access(name); }

void touchvar(const char *name)
{
	ident *id = idents->access(name);
	if(id) switch(id->type)
	{
		case ID_VAR:
		case ID_FVAR:
		case ID_SVAR:
			id->changed();
			break;
	}
}

const char *getalias(const char *name)
{
	ident *i = idents->access(name);
	return i && i->type==ID_ALIAS ? i->action : "";
}

bool addcommand(const char *name, void (*fun)(), const char *narg)
{
	if(!idents) idents = new identtable;
	ident c(ID_COMMAND, name, narg, (void *)fun, (void *)NULL);
	idents->access(name, c);
	return false;
}

void addident(const char *name, ident *id)
{
	if(!idents) idents = new identtable;
	idents->access(name, *id);
}

static vector<vector<char> *> wordbufs;
static int bufnest = 0;

char *parseexp(const char *&p, int right);

void parsemacro(const char *&p, int level, vector<char> &wordbuf)
{
	int escape = 1;
	while(*p=='@') p++, escape++;
	if(level > escape)
	{
		while(escape--) wordbuf.add('@');
		return;
	}
	if(*p=='(')
	{
		char *ret = parseexp(p, ')');
		if(ret)
		{
			for(char *sub = ret; *sub; ) wordbuf.add(*sub++);
			delete[] ret;
		}
		return;
	}
	static vector<char> ident;
	ident.setsizenodelete(0);
	while(isalnum(*p) || *p=='_') ident.add(*p++);
	ident.add(0);
	const char *alias = getalias(ident.getbuf());
	while(*alias) wordbuf.add(*alias++);
}

const char *parsestring(const char *p)
{
    for(; *p; p++) switch(*p)
    {
        case '\r':
        case '\n':
        case '\"':
            return p;
        case '^':
            if(*++p) break;
            return p;
    }
    return p;
}

int escapestring(char *dst, const char *src, const char *end)
{
    char *start = dst;
    while(src < end)
    {
        int c = *src++;
        if(c == '^')
        {
            if(src >= end) break;
            int e = *src++;
            switch(e)
            {
                case 'n': *dst++ = '\n'; break;
                case 't': *dst++ = '\t'; break;
                case 'f': *dst++ = '\f'; break;
                default: *dst++ = e; break;
            }
        }
        else *dst++ = c;
    }
    return dst - start;
}

char *parseexp(const char *&p, int right)		  // parse any nested set of () or []
{
	if(bufnest++>=wordbufs.length()) wordbufs.add(new vector<char>);
	vector<char> &wordbuf = *wordbufs[bufnest-1];
	int left = *p++;
	for(int brak = 1; brak; )
	{
        size_t n = strcspn(p, "\r@\"/()[]");
        wordbuf.put(p, n);
        p += n;

		int c = *p++;
        switch(c)
        {
            case '\r': continue;
            case '@':
                if(left == '[') { parsemacro(p, brak, wordbuf); continue; }
                break;
            case '\"':
            {
                wordbuf.add(c);
                const char *end = parsestring(p);
                wordbuf.put(p, end - p);
                p = end;
                if(*p=='\"') wordbuf.add(*p++);
                continue;
            }
            case '/':
                if(*p=='/')
                {
                    p += strcspn(p, "\n\0");
                    continue;
                }
                break;
            case '\0':
                p--;
                conoutf("\frmissing \"%c\"", right);
                wordbuf.setsizenodelete(0);
                bufnest--;
                return NULL;
            case '(': case '[': if(c==left) brak++; break;
            case ')': case ']': if(c==right) brak--; break;
        }
        wordbuf.add(c);
	}
	wordbuf.pop();
	char *s;
	if(left=='(')
	{
		wordbuf.add(0);
		char *ret = executeret(wordbuf.getbuf());					// evaluate () exps directly, and substitute result
		wordbuf.pop();
		s = ret ? ret : newstring("");
	}
	else
	{
		s = newstring(wordbuf.getbuf(), wordbuf.length());
	}
	wordbuf.setsizenodelete(0);
	bufnest--;
	return s;
}

char *lookup(char *n)							// find value of ident referenced with $ in exp
{
	ident *id = idents->access(n+1);
	if(id) switch(id->type)
	{
		case ID_VAR: { defformatstring(t)(id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i); return exchangestr(n, t); }
		case ID_FVAR: return exchangestr(n, floatstr(*id->storage.f));
		case ID_SVAR: return exchangestr(n, *id->storage.s);
		case ID_ALIAS: return exchangestr(n, id->action);
	}
	conoutf("\frunknown alias lookup: %s", n+1);
	return n;
}

char *parseword(const char *&p, int arg, int &infix)					   // parse single argument, including expressions
{
	for(;;)
	{
		p += strspn(p, " \t\r");
		if(p[0]!='/' || p[1]!='/') break;
		p += strcspn(p, "\n\0");
	}
	if(*p=='\"')
	{
		p++;
        const char *end = parsestring(p);
        char *s = newstring(end - p);
        s[escapestring(s, p, end)] = '\0';
        p = end;
		if(*p=='\"') p++;
		return s;
	}
	if(*p=='(') return parseexp(p, ')');
	if(*p=='[') return parseexp(p, ']');
	const char *word = p;
	for(;;)
	{
		p += strcspn(p, "/; \t\r\n\0");
		if(p[0]!='/' || p[1]=='/') break;
		else if(p[1]=='\0') { p++; break; }
		p += 2;
	}
	if(p-word==0) return NULL;
	if(arg==1 && p-word==1) switch(*word)
	{
		case '=': infix = *word; break;
	}
	char *s = newstring(word, p-word);
	if(*s=='$') return lookup(s);			   // substitute variables
	return s;
}

char *parsetext(const char *&p)
{
	for(;;)
	{
		p += strspn(p, " \t\r");
		if(p[0] != '/' || p[1] != '/') break;
		p += strcspn(p, "\n\0");
	}
	if(*p=='\"')
	{
		p++;
        const char *end = parsestring(p);
        char *s = newstring(end - p);
        s[escapestring(s, p, end)] = '\0';
        p = end;
		if(*p=='\"') p++;
		return s;
	}
	const char *word = p;
	for(;;)
	{
		p += strcspn(p, "/; \t\r\n\0");
		if(p[0] != '/' || p[1] == '/') break;
		else if(p[1] == '\0') { p++; break; }
		p += 2;
	}
	if(p-word != 0)
	{
		char *s = newstring(word, p-word);
		return s;
	}
	return NULL;
}

char *conc(char **w, int n, bool space)
{
	int len = space ? max(n-1, 0) : 0;
	loopj(n) len += (int)strlen(w[j]);
	char *r = newstring("", len);
	loopi(n)
	{
		strcat(r, w[i]);  // make string-list out of all arguments
		if(i==n-1) break;
		if(space) strcat(r, " ");
	}
	return r;
}

VARN(numargs, _numargs, 25, 0, 0);

static inline bool isinteger(char *c)
{
    return isdigit(c[0]) || ((c[0]=='+' || c[0]=='-' || c[0]=='.') && isdigit(c[1]));
}

#define parseint(s) strtol((s), NULL, 0)

char *commandret = NULL;

char *executeret(const char *p)			   // all evaluation happens here, recursively
{
	const int MAXWORDS = 25;					// limit, remove
	char *w[MAXWORDS];
	char *retval = NULL;
	#define setretval(v) { char *rv = v; if(rv) retval = rv; }
	for(bool cont = true; cont;)				// for each ; seperated statement
	{
		int numargs = MAXWORDS, infix = 0;
		loopi(MAXWORDS)						 // collect all argument values
		{
            w[i] = parseword(p, i, infix);   // parse and evaluate exps
            if(!w[i]) { numargs = i; break; }
		}

		p += strcspn(p, ";\n\0");
		cont = *p++!=0;						 // more statements if this isn't the end of the string
		char *c = w[0];
		if(!c || !*c) continue;						// empty statement

		DELETEA(retval);

		if(infix)
		{
			switch(infix)
			{
				case '=':
					aliasa(c, numargs>2 ? w[2] : newstring(""));
					w[2] = NULL;
					break;
			}
		}
		else
		{
			ident *id = idents->access(c);
			if(!id || (id->flags&IDF_SERVER && id->type!=ID_COMMAND && id->type!=ID_CCOMMAND) || id->flags&IDF_CLIENT)
			{
				if(!isinteger(c))
				{
					bool found = false;
					char *exargs = NULL;
					if(numargs > 1) exargs = conc(w+1, numargs-1, true);
					if(id && id->flags&IDF_SERVER && id->type!=ID_COMMAND && id->type!=ID_CCOMMAND) found = server::servcmd(numargs, c, exargs ? exargs : "");
#ifndef STANDALONE
					else if(!id || id->flags&IDF_CLIENT) found = client::sendcmd(numargs, c, exargs ? exargs : "");
#endif
					if(exargs) delete[] exargs;
					if(!found) conoutf("\frunknown command: %s", c);
				}
				setretval(newstring(c));
			}
			else switch(id->type)
			{
				case ID_CCOMMAND:
				case ID_COMMAND:					 // game defined commands
				{
					void *v[MAXWORDS];
					union
					{
						int i;
						float f;
					} nstor[MAXWORDS];
					int n = 0, wn = 0;
					char *cargs = NULL, *dargs = NULL;
                    const char *clast = "";
					if(id->type==ID_CCOMMAND) v[n++] = id->self;
					for(const char *a = id->narg; *a; clast = a, a++, n++) switch(*a)
					{
                        case 's': v[n] = ++wn < numargs ? w[wn] : (char *)""; break;
                        case 'i': nstor[n].i = ++wn < numargs ? parseint(w[wn]) : 0;  v[n] = &nstor[n].i; break;
                        case 'f': nstor[n].f = ++wn < numargs ? atof(w[wn]) : 0.0f; v[n] = &nstor[n].f; break;
#ifndef STANDALONE
                        case 'D': nstor[n].i = addreleaseaction(dargs = conc(w, numargs, true)) ? 1 : 0; v[n] = &nstor[n].i; break;
#endif
                        case 'V': v[n++] = w+1; nstor[n].i = numargs-1; v[n] = &nstor[n].i; break;
                        case 'C': if(!cargs) cargs = conc(w+1, numargs-1, true); v[n] = cargs; break;
						default: fatal("builtin declared with illegal type");
					}
					char *exargs = NULL;
					if(*clast == 's' && wn < numargs-1) { exargs = conc(w+wn, numargs-wn, true); v[n-1] = exargs; }
					switch(n)
					{
						case 0: ((void (__cdecl *)())id->fun)(); break;
						case 1: ((void (__cdecl *)(void *))id->fun)(v[0]); break;
						case 2: ((void (__cdecl *)(void *, void *))id->fun)(v[0], v[1]); break;
						case 3: ((void (__cdecl *)(void *, void *, void *))id->fun)(v[0], v[1], v[2]); break;
						case 4: ((void (__cdecl *)(void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3]); break;
						case 5: ((void (__cdecl *)(void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4]); break;
						case 6: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5]); break;
						case 7: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6]); break;
						case 8: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]); break;
						default: fatal("builtin declared with too many args (use V?)");
					}
					if(cargs) delete[] cargs;
					if(dargs) delete[] dargs;
					if(exargs) delete[] exargs;
					setretval(commandret);
					commandret = NULL;
					break;
				}

				case ID_VAR:						// game defined variables
                    if(numargs <= 1) conoutft(CON_MESG, id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "\fd%s = 0x%.6X" : "\fd%s = 0x%X") : "\fd%s = %d", c, *id->storage.i);      // var with no value just prints its current value
					else if(id->minval>id->maxval) conoutf("\frvariable %s is read-only", id->name);
					else
					{
#ifndef STANDALONE
						#define WORLDVAR \
							if (!worldidents && !editmode && id->flags&IDF_WORLD) \
							{ \
								conoutft(CON_MESG, "\frcannot set world variable %s outside editmode", id->name); \
								break; \
							}
#endif

						#define OVERRIDEVAR(saveval, resetval, clearval) \
							if(overrideidents && id->flags&IDF_OVERRIDE) \
							{ \
								if(id->flags&IDF_PERSIST) \
								{ \
									conoutft(CON_MESG, "\frcannot override persistent variable %s", id->name); \
									break; \
								} \
								if(id->override==NO_OVERRIDE) { saveval; id->override = OVERRIDDEN; } \
                                else { clearval; } \
							} \
							else \
                            { \
                                if(id->override!=NO_OVERRIDE) { resetval; id->override = NO_OVERRIDE; } \
                                clearval; \
                            }
#ifndef STANDALONE
						WORLDVAR;
#endif
						OVERRIDEVAR(id->overrideval.i = *id->storage.i, , )
						int i1 = parseint(w[1]);
                        if(id->flags&IDF_HEX && numargs > 2)
                        {
                            i1 <<= 16;
                            i1 |= parseint(w[2])<<8;
                            if(numargs > 3) i1 |= parseint(w[3]);
                        }
						if(i1<id->minval || i1>id->maxval)
						{
							i1 = i1<id->minval ? id->minval : id->maxval;				// clamp to valid range
                            conoutft(CON_MESG,
                                id->flags&IDF_HEX ?
                                    (id->minval <= 255 ? "\frvalid range for %s is %d..0x%X" : "\frvalid range for %s is 0x%X..0x%X") :
                                    "\frvalid range for %s is %d..%d",
                                id->name, id->minval, id->maxval);
						}
						*id->storage.i = i1;
						id->changed();											 // call trigger function if available
#ifndef STANDALONE
						client::editvar(id, interactive && !overrideidents);
#endif
					}
					break;

				case ID_FVAR:
					if(numargs <= 1) conoutft(CON_MESG, "\fd%s = %s", c, floatstr(*id->storage.f));
					else if(id->minvalf>id->maxvalf) conoutft(CON_MESG, "\frvariable %s is read-only", id->name);
					else
					{
#ifndef STANDALONE
						WORLDVAR;
#endif
						OVERRIDEVAR(id->overrideval.f = *id->storage.f, , );
						float f1 = atof(w[1]);
						if(f1<id->minvalf || f1>id->maxvalf)
						{
							f1 = f1<id->minvalf ? id->minvalf : id->maxvalf;				// clamp to valid range
							conoutft(CON_MESG, "\frvalid range for %s is %s..%s", id->name, floatstr(id->minvalf), floatstr(id->maxvalf));
						}
						*id->storage.f = f1;
						id->changed();
#ifndef STANDALONE
						client::editvar(id, interactive && !overrideidents);
#endif
					}
					break;

				case ID_SVAR:
					if(numargs <= 1) conoutft(CON_MESG, strchr(*id->storage.s, '"') ? "%s = [%s]" : "%s = \"%s\"", c, *id->storage.s);
					else
					{
						char *exargs = NULL, *val = w[1];
						if(numargs > 2) val = exargs = conc(w+1, numargs-1, true);
#ifndef STANDALONE
						WORLDVAR;
#endif
						OVERRIDEVAR(id->overrideval.s = *id->storage.s, delete[] id->overrideval.s, delete[] *id->storage.s);
						*id->storage.s = newstring(val);
						id->changed();
						if(exargs) delete[] exargs;
#ifndef STANDALONE
						client::editvar(id, interactive && !overrideidents);
#endif
					}
					break;

				case ID_ALIAS:							  // alias, also used as functions and (global) variables
				{
					delete[] w[0];
					static vector<ident *> argids;
					for(int i = 1; i<numargs; i++)
					{
						if(i > argids.length())
						{
							defformatstring(argname)("arg%d", i);
							argids.add(newident(argname));
						}
						pushident(*argids[i-1], w[i]); // set any arguments as (global) arg values so functions can access them
					}
					_numargs = numargs-1;
					bool wasoverriding = overrideidents;
					if(id->flags&IDF_OVERRIDE && id->override!=NO_OVERRIDE) overrideidents = true;
					char *wasexecuting = id->isexecuting;
					id->isexecuting = id->action;
					setretval(executeret(id->action));
					if(id->isexecuting != id->action && id->isexecuting != wasexecuting) delete[] id->isexecuting;
					id->isexecuting = wasexecuting;
					overrideidents = wasoverriding;
					for(int i = 1; i<numargs; i++) popident(*argids[i-1]);
					continue;
				}
			}
		}
		loopj(numargs) if(w[j]) delete[] w[j];
	}
	return retval;
}

int execute(const char *p)
{
	char *ret = executeret(p);
	int i = 0;
	if(ret) { i = parseint(ret); delete[] ret; }
	return i;
}

bool execfile(const char *cfgfile, bool msg)
{
	string s;
	copystring(s, cfgfile);
	char *buf = loadfile(s, NULL);
	if(!buf)
    {
        if(msg) conoutf("\frcould not read %s", cfgfile);
        return false;
    }
	execute(buf);
	delete[] buf;
	if (verbose >= 3) conoutf("\fdloaded script %s", cfgfile);
	return true;
}

#ifndef STANDALONE
static int sortidents(ident **x, ident **y)
{
    return strcmp((*x)->name, (*y)->name);
}

void writeescapedstring(stream *f, const char *s)
{
    f->putchar('"');
    for(; *s; s++) switch(*s)
    {
        case '\n': f->write("^n", 2); break;
        case '\t': f->write("^t", 2); break;
        case '\f': f->write("^f", 2); break;
        case '"': f->write("^\"", 2); break;
        default: f->putchar(*s); break;
    }
    f->putchar('"');
}

void writecfg()
{
	stream *f = openfile("config.cfg", "w");

	if(!f) return;
	client::writeclientinfo(f);
	if(ENG_DEVEL) f->printf("\n// written with a development version, no other settings are saved\n");
	else
	{
		f->printf("if (= $version %d) [\n", ENG_VERSION);
		vector<ident *> ids;
		enumerate(*idents, ident, id, ids.add(&id));
		ids.sort(sortidents);
		loopv(ids)
		{
			ident &id = *ids[i];
			bool saved = false;
			if(id.flags&IDF_PERSIST) switch(id.type)
			{
				case ID_VAR: saved = true; f->printf((id.flags&IDF_HEX ? (id.maxval==0xFFFFFF ? "\t%s 0x%.6X\n" : "\t%s 0x%X\n") : "\t%s %d\n"), id.name, *id.storage.i); break;
				case ID_FVAR: saved = true; f->printf("\t%s %s\n", id.name, floatstr(*id.storage.f)); break;
				case ID_SVAR: saved = true; f->printf("\t%s ", id.name); writeescapedstring(f, *id.storage.s); f->putchar('\n'); break;
			}
			if(saved && !(id.flags&IDF_COMPLETE)) f->printf("\tsetcomplete \"%s\" 0\n", id.name);
		}
		loopv(ids)
		{
			ident &id = *ids[i];
			bool saved = false;
			if(id.flags&IDF_PERSIST) switch(id.type)
			{
				case ID_ALIAS:
				{
					if(id.override==NO_OVERRIDE && id.action[0])
					{
						saved = true;
						f->printf("\t\"%s\" = [%s]\n", id.name, id.action);
					}
					break;
				}
			}
			if(saved && !(id.flags&IDF_COMPLETE)) f->printf("\tsetcomplete \"%s\" 0\n", id.name);
		}
		writebinds(f);
		writecompletions(f);
		f->printf("] [ echo \"\frWARNING: config from different version ignored, if you wish to save settings between version please use autoexec.cfg\" ]\n");
	}
	f->printf("\n");
	delete f;
}

COMMAND(writecfg, "");
#endif

// below the commands that implement a small imperative language. thanks to the semantics of
// () and [] expressions, any control construct can be defined trivially.

void intret(int v) { defformatstring(b)("%d", v); commandret = newstring(b); }

const char *floatstr(float v)
{
	static int n = 0;
	static string t[3];
	n = (n + 1)%3;
	formatstring(t[n])(v==int(v) ? "%.1f" : "%.7g", v);
	return t[n];
}

void floatret(float v)
{
	commandret = newstring(floatstr(v));
}

ICOMMAND(if, "sss", (char *cond, char *t, char *f), commandret = executeret(cond[0] && (!isinteger(cond) || parseint(cond)) ? t : f));
ICOMMANDN(?, iif, "sss", (char *cond, char *t, char *f), result(cond[0] && (!isinteger(cond) || parseint(cond)) ? t : f));
ICOMMAND(loop, "sis", (char *var, int *n, char *body),
{
	if(*n<=0) return;
	ident *id = newident(var);
	if(id->type!=ID_ALIAS) return;
	loopi(*n)
	{
		if(i) sprintf(id->action, "%d", i);
		else pushident(*id, newstring("0", 16));
		execute(body);
	}
	popident(*id);
});
ICOMMAND(while, "ss", (char *cond, char *body), while(execute(cond)) execute(body));	// can't get any simpler than this :)

void concat(const char *s) { commandret = newstring(s); }
void result(const char *s) { commandret = newstring(s); }

void concatword(char **args, int *numargs)
{
	commandret = conc(args, *numargs, false);
}

void format(char **args, int *numargs)
{
	vector<char> s;
	char *f = args[0];
	while(*f)
	{
		int c = *f++;
		if(c == '%')
		{
			int i = *f++;
			if(i >= '1' && i <= '9')
			{
				i -= '0';
				const char *sub = i < *numargs ? args[i] : "";
				while(*sub) s.add(*sub++);
			}
			else s.add(i);
		}
		else s.add(c);
	}
	s.add('\0');
	result(s.getbuf());
}

#define whitespaceskip s += strspn(s, "\n\t ")
#define elementskip *s=='"' ? (++s, s += strcspn(s, "\"\n\0"), s += *s=='"') : s += strcspn(s, "\n\t \0")

void explodelist(const char *s, vector<char *> &elems)
{
	whitespaceskip;
	while(*s)
	{
		const char *elem = s;
		elementskip;
		elems.add(*elem=='"' ? newstring(elem+1, s-elem-(s[-1]=='"' ? 2 : 1)) : newstring(elem, s-elem));
		whitespaceskip;
	}
}

char *indexlist(const char *s, int pos)
{
	whitespaceskip;
    loopi(pos)
    {
        elementskip;
        whitespaceskip;
        if(!*s) break;
    }
	const char *e = s;
	elementskip;
	if(*e=='"')
	{
		e++;
		if(s[-1]=='"') --s;
	}
	return newstring(e, s-e);
}

int listlen(const char *s)
{
	int n = 0;
	whitespaceskip;
	for(; *s; n++) elementskip, whitespaceskip;
	return n;
}

void at(char *s, int *pos)
{
	commandret = indexlist(s, *pos);
}

void substr(char *s, int *start, int *count)
{
	int len = strlen(s), offset = clamp(*start, 0, len);
	commandret = newstring(&s[offset], *count <= 0 ? len - offset : min(*count, len - offset));
}

void getalias_(char *s)
{
	result(getalias(s));
}

ICOMMAND(exec, "s", (char *file), execfile(file));
COMMAND(concat, "C");
COMMAND(result, "s");
COMMAND(concatword, "V");
COMMAND(format, "V");
COMMAND(at, "si");
COMMAND(substr, "sii");
ICOMMAND(listlen, "s", (char *s), intret(listlen(s)));
COMMANDN(getalias, getalias_, "s");

void looplist(const char *var, const char *list, const char *body, bool search)
{
    ident *id = newident(var);
    if(id->type!=ID_ALIAS) { if(search) intret(-1); return; }
    int n = 0;
    for(const char *s = list;;)
    {
        whitespaceskip;
        if(!*s) { if(search) intret(-1); break; }
        const char *start = s;
        elementskip;
        const char *end = s;
        if(*start=='"') { start++; if(end[-1]=='"') --end; }
        char *val = newstring(start, end-start);
        if(n++) aliasa(id->name, val);
        else pushident(*id, val);
        if(execute(body) && search) { intret(n-1); break; }
    }
    if(n) popident(*id);
}

void prettylist(const char *s, const char *conj)
{
	vector<char> p;
	whitespaceskip;
	for(int len = listlen(s), n = 0; *s; n++)
	{
		const char *elem = s;
		elementskip;
		p.put(elem, s - elem);
		if(n+1 < len)
		{
			if(len > 2 || !conj[0]) p.add(',');
			if(n+2 == len && conj[0])
			{
				p.add(' ');
				p.put(conj, strlen(conj));
			}
			p.add(' ');
		}
		whitespaceskip;
	}
	p.add('\0');
	result(p.getbuf());
}
COMMAND(prettylist, "ss");

ICOMMAND(listfind, "sss", (char *var, char *list, char *body), looplist(var, list, body, true));
ICOMMAND(looplist, "sss", (char *var, char *list, char *body), looplist(var, list, body, false));
ICOMMAND(loopfiles, "ssss", (char *var, char *dir, char *ext, char *body),
{
    ident *id = newident(var);
    if(id->type!=ID_ALIAS) return;
    vector<char *> files;
    listfiles(dir, ext[0] ? ext : NULL, files);
    loopv(files)
    {
        char *file = files[i];
        //bool redundant = false;
        //loopj(i) if(!strcmp(files[j], file)) { redundant = true; break; }
        //if(redundant) { delete[] file; continue; }
        if(i) aliasa(id->name, file);
        else pushident(*id, file);
        execute(body);
    }
    if(files.length()) popident(*id);
});

ICOMMANDN(+, add, "ii", (int *a, int *b), intret(*a + *b));
ICOMMANDN(*, mul, "ii", (int *a, int *b), intret(*a * *b));
ICOMMANDN(-, sub, "ii", (int *a, int *b), intret(*a - *b));
ICOMMANDN(+f, addf, "ff", (float *a, float *b), floatret(*a + *b));
ICOMMANDN(*f, mulf, "ff", (float *a, float *b), floatret(*a * *b));
ICOMMANDN(-f, subf, "ff", (float *a, float *b), floatret(*a - *b));
ICOMMANDN(=, eq, "ii", (int *a, int *b), intret((int)(*a == *b)));
ICOMMANDN(!=, neq, "ii", (int *a, int *b), intret((int)(*a != *b)));
ICOMMANDN(<, lt, "ii", (int *a, int *b), intret((int)(*a < *b)));
ICOMMANDN(>, gt, "ii", (int *a, int *b), intret((int)(*a > *b)));
ICOMMANDN(<=, lteq, "ii", (int *a, int *b), intret((int)(*a <= *b)));
ICOMMANDN(>=, gteq, "ii", (int *a, int *b), intret((int)(*a >= *b)));
ICOMMANDN(=f, eqf, "ff", (float *a, float *b), intret((int)(*a == *b)));
ICOMMANDN(!=f, neqf, "ff", (float *a, float *b), intret((int)(*a != *b)));
ICOMMANDN(<f, ltf, "ff", (float *a, float *b), intret((int)(*a < *b)));
ICOMMANDN(>f, gtf, "ff", (float *a, float *b), intret((int)(*a > *b)));
ICOMMANDN(<=f, lteqf, "ff", (float *a, float *b), intret((int)(*a <= *b)));
ICOMMANDN(>=f, gteqf, "ff", (float *a, float *b), intret((int)(*a >= *b)));
ICOMMANDN(^, xora, "ii", (int *a, int *b), intret(*a ^ *b));
ICOMMANDN(!, anda, "i", (int *a), intret(*a == 0));
ICOMMANDN(&, bita, "ii", (int *a, int *b), intret(*a & *b));
ICOMMANDN(|, orba, "ii", (int *a, int *b), intret(*a | *b));
ICOMMANDN(~, nota, "i", (int *a), intret(~*a));
ICOMMANDN(^~, notx, "ii", (int *a, int *b), intret(*a ^ ~*b));
ICOMMANDN(&~, notb, "ii", (int *a, int *b), intret(*a & ~*b));
ICOMMANDN(|~, noto, "ii", (int *a, int *b), intret(*a | ~*b));
ICOMMANDN(<<, lsft, "ii", (int *a, int *b), intret(*a << *b));
ICOMMANDN(>>, rsft, "ii", (int *a, int *b), intret(*a >> *b));
ICOMMANDN(&&, and, "ss", (char *a, char *b), intret(execute(a)!=0 && execute(b)!=0));
ICOMMANDN(||, or, "ss", (char *a, char *b), intret(execute(a)!=0 || execute(b)!=0));

ICOMMAND(div, "ii", (int *a, int *b), intret(*b ? *a / *b : 0));
ICOMMAND(mod, "ii", (int *a, int *b), intret(*b ? *a % *b : 0));
ICOMMAND(divf, "ff", (float *a, float *b), floatret(*b ? *a / *b : 0));
ICOMMAND(modf, "ff", (float *a, float *b), floatret(*b ? fmod(*a, *b) : 0));
ICOMMAND(min, "ii", (int *a, int *b), intret(min(*a, *b)));
ICOMMAND(max, "ii", (int *a, int *b), intret(max(*a, *b)));
ICOMMAND(minf, "ff", (float *a, float *b), floatret(min(*a, *b)));
ICOMMAND(maxf, "ff", (float *a, float *b), floatret(max(*a, *b)));

ICOMMAND(rnd, "ii", (int *a, int *b), intret(*a - *b > 0 ? rnd(*a - *b) + *b : *b));
ICOMMAND(strcmp, "ss", (char *a, char *b), intret(strcmp(a,b)==0));
ICOMMAND(echo, "C", (char *s), conoutft(CON_SELF, "%s", s));
ICOMMAND(strstr, "ss", (char *a, char *b), { char *s = strstr(a, b); intret(s ? s-a : -1); });
ICOMMAND(strlen, "s", (char *s), intret(strlen(s)));

char *strreplace(const char *s, const char *oldval, const char *newval)
{
	vector<char> buf;

	int oldlen = strlen(oldval);
	for(;;)
	{
		const char *found = strstr(s, oldval);
		if(found)
		{
			while(s < found) buf.add(*s++);
			for(const char *n = newval; *n; n++) buf.add(*n);
			s = found + oldlen;
		}
		else
		{
			while(*s) buf.add(*s++);
			buf.add('\0');
			return newstring(buf.getbuf(), buf.length());
		}
	}
}

ICOMMAND(strreplace, "sss", (char *a, char *b, char *c), commandret = strreplace(a, b, c));

struct sleepcmd
{
	int millis;
	char *command;
	int flags;
};
vector<sleepcmd> sleepcmds;

void addsleep(int *msec, char *cmd)
{
	sleepcmd &s = sleepcmds.add();
	s.millis = *msec+lastmillis;
	s.command = newstring(cmd);
	s.flags = (overrideidents ? IDF_OVERRIDE : 0)|(worldidents ? IDF_WORLD : 0)|(persistidents ? IDF_PERSIST : 0);
}

ICOMMAND(sleep, "is", (int *a, char *b), addsleep(a, b));

void checksleep(int millis)
{
	loopv(sleepcmds)
	{
		sleepcmd &s = sleepcmds[i];
		if(s.millis && millis>s.millis)
		{
			bool oldpersistidents = persistidents,
				 oldoverrideidents = overrideidents,
				 oldworldidents = worldidents;
			persistidents = (s.flags&IDF_PERSIST)!=0;
			overrideidents = (s.flags&IDF_OVERRIDE)!=0;
			worldidents = (s.flags&IDF_WORLD)!=0;
			char *cmd = s.command; // execute might create more sleep commands
			s.command = NULL;
			execute(cmd);
			delete[] cmd;
			persistidents = oldpersistidents;
			overrideidents = oldoverrideidents;
			worldidents = oldworldidents;
			if(sleepcmds.inrange(i) && !sleepcmds[i].command) sleepcmds.remove(i--);
		}
	}
}

void clearsleep(bool clearoverrides, bool clearworlds)
{
	int len = 0;
	loopv(sleepcmds) if(sleepcmds[i].command)
	{
		if((clearoverrides && sleepcmds[i].flags&IDF_OVERRIDE) ||
			(clearworlds && sleepcmds[i].flags&IDF_WORLD))
				delete[] sleepcmds[i].command;
		else sleepcmds[len++] = sleepcmds[i];
	}
	sleepcmds.setsize(len);
}

ICOMMAND(clearsleep, "ii", (int *a, int *b), clearsleep(*a!=0 || overrideidents, *b!=0 || worldidents));
ICOMMAND(exists, "ss", (char *a, char *b), intret(fileexists(a, *b ? b : "r")));
ICOMMAND(getmillis, "", (), intret(lastmillis));

