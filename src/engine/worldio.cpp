// worldio.cpp: loading & saving of maps and savegames

#include "pch.h"
#include "engine.h"

void backup(char *name, char *backupname)
{
	string backupfile;
	s_strcpy(backupfile, findfile(backupname, "wb"));
	remove(backupfile);
	rename(findfile(name, "wb"), backupfile);
#ifdef BFRONTIER // verbosity support
	if (verbose >= 2) console("backup of '%s' made to '%s'", CON_RIGHT, name, backupname);
#endif
}

#ifdef BFRONTIER // map extensions
string cgzname, pcfname, mcfname, picname, mapname, extname;
int extversion = 0;
#else
string cgzname, bakname, pcfname, mcfname, picname;
#endif

#ifdef BFRONTIER // only one level of backups
VARP(savebak, 0, 1, 1);
#else
VARP(savebak, 0, 2, 2);
#endif

void cutogz(char *s)
{
	char *ogzp = strstr(s, ".ogz");
	if(ogzp) *ogzp = '\0';
}

void setnames(const char *fname, const char *cname = 0)
{
	if(!cname) cname = fname;
#ifdef BFRONTIER // mapname defined globally
	string name, pakname, cfgname;
#else
	string name, pakname, mapname, cfgname;
#endif
	s_strncpy(name, cname, 100);
	cutogz(name);
	char *slash = strpbrk(name, "/\\");
	if(slash)
	{
		s_strncpy(pakname, name, slash-name+1);
		s_strcpy(cfgname, slash+1);
	}
	else
	{
		s_strcpy(pakname, "base");
		s_strcpy(cfgname, name);
	}
	if(strpbrk(fname, "/\\")) s_strcpy(mapname, fname);
	else s_sprintf(mapname)("base/%s", fname);
	cutogz(mapname);

	s_sprintf(cgzname)("packages/%s.ogz", mapname);
#ifdef BFRONTIER // map extension file
	s_sprintf(extname)("packages/%s.etz", mapname);
#else
	if(savebak==1) s_sprintf(bakname)("packages/%s.BAK", mapname);
	else s_sprintf(bakname)("packages/%s_%d.BAK", mapname, lastmillis);
#endif
	s_sprintf(pcfname)("packages/%s/package.cfg", pakname);
	s_sprintf(mcfname)("packages/%s/%s.cfg",	  pakname, cfgname);
	s_sprintf(picname)("packages/%s.jpg", mapname);

	path(cgzname);
#ifdef BFRONTIER // map extension file
	path(extname);
#else
	path(bakname);
#endif
	path(picname);
}

ushort readushort(gzFile f)
{
	ushort t;
	gzread(f, &t, sizeof(ushort));
	endianswap(&t, sizeof(ushort), 1);
	return t;
}

void writeushort(gzFile f, ushort u)
{
	endianswap(&u, sizeof(ushort), 1);
	gzwrite(f, &u, sizeof(ushort));
}

enum { OCTSAV_CHILDREN = 0, OCTSAV_EMPTY, OCTSAV_SOLID, OCTSAV_NORMAL, OCTSAV_LODCUBE };

void savec(cube *c, gzFile f, bool nolms)
{
	loopi(8)
	{
		if(c[i].children && (!c[i].ext || !c[i].ext->surfaces))
		{
			gzputc(f, OCTSAV_CHILDREN);
			savec(c[i].children, f, nolms);
		}
		else
		{
			int oflags = 0;
			if(c[i].ext && c[i].ext->merged) oflags |= 0x80;
			if(c[i].children) gzputc(f, oflags | OCTSAV_LODCUBE);
			else if(isempty(c[i])) gzputc(f, oflags | OCTSAV_EMPTY);
			else if(isentirelysolid(c[i])) gzputc(f, oflags | OCTSAV_SOLID);
			else
			{
				gzputc(f, oflags | OCTSAV_NORMAL);
				gzwrite(f, c[i].edges, 12);
			}
			loopj(6) writeushort(f, c[i].texture[j]);
			uchar mask = 0;
			if(c[i].ext)
			{
				if(c[i].ext->material != MAT_AIR) mask |= 0x80;
				if(c[i].ext->normals && !nolms)
				{
					mask |= 0x40;
					loopj(6) if(c[i].ext->normals[j].normals[0] != bvec(128, 128, 128)) mask |= 1 << j;
				}
			}
			// save surface info for lighting
			if(!c[i].ext || !c[i].ext->surfaces || nolms)
			{
				gzputc(f, mask);
				if(c[i].ext)
				{
					if(c[i].ext->material != MAT_AIR) gzputc(f, c[i].ext->material);
					if(c[i].ext->normals && !nolms) loopj(6) if(mask & (1 << j))
					{
						loopk(sizeof(surfaceinfo)) gzputc(f, 0);
						gzwrite(f, &c[i].ext->normals[j], sizeof(surfacenormals));
					} 
				}
			}
			else
			{
				loopj(6) if(c[i].ext->surfaces[j].lmid >= LMID_RESERVED) mask |= 1 << j;
				gzputc(f, mask);
				if(c[i].ext->material != MAT_AIR) gzputc(f, c[i].ext->material);
				loopj(6) if(mask & (1 << j))
				{
					surfaceinfo tmp = c[i].ext->surfaces[j];
					endianswap(&tmp.x, sizeof(ushort), 3);
					gzwrite(f, &tmp, sizeof(surfaceinfo));
					if(c[i].ext->normals) gzwrite(f, &c[i].ext->normals[j], sizeof(surfacenormals));
				}
			}
			if(c[i].ext && c[i].ext->merged)
			{
				gzputc(f, c[i].ext->merged | (c[i].ext->mergeorigin ? 0x80 : 0));
				if(c[i].ext->mergeorigin)
				{
					gzputc(f, c[i].ext->mergeorigin);
					int index = 0;
					loopj(6) if(c[i].ext->mergeorigin&(1<<j))
					{
						mergeinfo tmp = c[i].ext->merges[index++];
						endianswap(&tmp, sizeof(ushort), 4);
						gzwrite(f, &tmp, sizeof(mergeinfo));
					}
				}
			}
			if(c[i].children) savec(c[i].children, f, nolms);
		}
	}
}

cube *loadchildren(gzFile f);

void loadc(gzFile f, cube &c)
{
	bool haschildren = false;
	int octsav = gzgetc(f);
	switch(octsav&0x7)
	{
		case OCTSAV_CHILDREN:
			c.children = loadchildren(f);
			return;

		case OCTSAV_LODCUBE: haschildren = true;	break;
		case OCTSAV_EMPTY:  emptyfaces(c);		  break;
		case OCTSAV_SOLID:  solidfaces(c);		  break;
		case OCTSAV_NORMAL: gzread(f, c.edges, 12); break;

		default:
			fatal("garbage in map");
	}
	loopi(6) c.texture[i] = hdr.version<14 ? gzgetc(f) : readushort(f);
	if(hdr.version < 7) loopi(3) gzgetc(f); //gzread(f, c.colour, 3);
	else
	{
		uchar mask = gzgetc(f);
		if(mask & 0x80) ext(c).material = gzgetc(f);
		if(mask & 0x3F)
		{
			uchar lit = 0, bright = 0;
			newsurfaces(c);
			if(mask & 0x40) newnormals(c);
			loopi(6)
			{
				if(mask & (1 << i))
				{
					gzread(f, &c.ext->surfaces[i], sizeof(surfaceinfo));
					endianswap(&c.ext->surfaces[i].x, sizeof(ushort), 3);
					if(hdr.version < 10) ++c.ext->surfaces[i].lmid;
					if(hdr.version < 18)
					{
						if(c.ext->surfaces[i].lmid >= LMID_AMBIENT1) ++c.ext->surfaces[i].lmid;
						if(c.ext->surfaces[i].lmid >= LMID_BRIGHT1) ++c.ext->surfaces[i].lmid;
					}
					if(hdr.version < 19)
					{
						if(c.ext->surfaces[i].lmid >= LMID_DARK) c.ext->surfaces[i].lmid += 2;
					}
					if(mask & 0x40) gzread(f, &c.ext->normals[i], sizeof(surfacenormals));
				}
				else c.ext->surfaces[i].lmid = LMID_AMBIENT;
				if(c.ext->surfaces[i].lmid == LMID_BRIGHT) bright |= 1 << i;
				else if(c.ext->surfaces[i].lmid != LMID_AMBIENT) lit |= 1 << i;
			}
			if(!lit) 
			{
				freesurfaces(c);
				if(bright) brightencube(c);
			}
		}
		if(hdr.version >= 20)
		{
			if(octsav&0x80)
			{
				int merged = gzgetc(f);
				ext(c).merged = merged&0x3F;
				if(merged&0x80)
				{
					c.ext->mergeorigin = gzgetc(f);
					int nummerges = 0;
					loopi(6) if(c.ext->mergeorigin&(1<<i)) nummerges++;
					if(nummerges)
					{
						c.ext->merges = new mergeinfo[nummerges];
						loopi(nummerges)
						{
							gzread(f, &c.ext->merges[i], sizeof(mergeinfo));
							endianswap(&c.ext->merges[i], sizeof(ushort), 4);
						}
					}
				}
			}	
		}				
	}
	c.children = (haschildren ? loadchildren(f) : NULL);
}

cube *loadchildren(gzFile f)
{
	cube *c = newcubes();
	loopi(8) loadc(f, c[i]);
	// TODO: remip c from children here
	return c;
}

void save_world(char *mname, bool nolms)
{
#ifdef BFRONTIER // verbosity support
	int savingstart = SDL_GetTicks();
#endif
	if(!*mname) mname = cl->getclientmap();
	setnames(*mname ? mname : "untitled");
#ifdef BFRONTIER // map extensions, alternate backups, blood frontier
	gzFile f = opengzfile(cgzname, "wb9");
	if(!f) { conoutf("error saving '%s' to '%s': file error", mapname, cgzname); return; }
	gzFile g = opengzfile(extname, "wb9");
	if (!g) { conoutf("could not write extension to %s", extname); return; }
	FILE *h = openfile(mcfname, "w");
	if (!h) { conoutf("could not write config to %s", extname); return; }
	
	if(savebak)
	{
		long int baktime = time(NULL);
		string bakname;

		show_out_of_renderloop_progress(0, "saving backups...");

		s_sprintf(bakname)("packages/%s_%li.ogz", mapname, baktime);
		path(bakname);
		backup(cgzname, bakname);

		show_out_of_renderloop_progress(0.33f, "saving backups...");

		s_sprintf(bakname)("packages/%s_%li.etz", mapname, baktime);
		path(bakname);
		backup(extname, bakname);

		show_out_of_renderloop_progress(0.66f, "saving backups...");

		s_sprintf(bakname)("packages/%s_%li.cfg", mapname, baktime);
		path(bakname);
		backup(mcfname, bakname);
	}
#else
	if(savebak) backup(cgzname, bakname);
	gzFile f = opengzfile(cgzname, "wb9");
	if(!f) { conoutf("could not write map to %s", cgzname); return; }
#endif
	hdr.version = MAPVERSION;
	hdr.numents = 0;
	const vector<extentity *> &ents = et->getents();
#ifdef BFRONTIER // extended entities
	int numents = 0;
	loopv(ents)
	{
		if(ents[i]->type!=ET_EMPTY)
		{
			if (et->isext(ents[i]->type)) numents++;
			else hdr.numents++;
		}
	}
	char ehead[4];

	strncpy(ehead, "EXTZ", 4);
	gzwrite(g, &ehead, 4);

	gzputint(g, EXTVERSION);
	gzputint(g, numents);

	hdr.lightmaps = nolms ? 0 : lightmaps.length();
	header tmp = hdr;
	endianswap(&tmp.version, sizeof(int), 9);
	gzwrite(f, &tmp, sizeof(header));

	string gametype;
	s_strcpy(gametype, "fps"); // we only generate fps maps
	gzputc(f, (int)strlen(gametype));
	gzwrite(f, sv->gameident(), (int)strlen(gametype)+1);
#else
	loopv(ents) if(ents[i]->type!=ET_EMPTY) hdr.numents++;
	hdr.lightmaps = nolms ? 0 : lightmaps.length();
	header tmp = hdr;
	endianswap(&tmp.version, sizeof(int), 9);
	gzwrite(f, &tmp, sizeof(header));

	gzputc(f, (int)strlen(cl->gameident()));
	gzwrite(f, cl->gameident(), (int)strlen(cl->gameident())+1);
#endif
	writeushort(f, et->extraentinfosize());
	vector<char> extras;
	cl->writegamedata(extras);
	writeushort(f, extras.length());
	gzwrite(f, extras.getbuf(), extras.length());
	
#ifdef BFRONTIER // extended entities, world variables, verbosity support
	writeushort(f, texmru.length());
	loopv(texmru) writeushort(f, texmru[i]);
	char *ebuf = new char[et->extraentinfosize()];

	int ecount = 0;
	loopv(ents) // extended
	{
		show_out_of_renderloop_progress(float(i)/float(ents.length()), "saving entities...");
		if(ents[i]->type!=ET_EMPTY)
		{
			entity tmp = *ents[i];
			endianswap(&tmp.o, sizeof(int), 3);
			endianswap(&tmp.attr1, sizeof(short), 5);

			if (et->isext(ents[i]->type))
			{
				gzwrite(g, &tmp, sizeof(entity));
				et->writeext(g, hdr.numents, i, ecount, *ents[i]);
				ecount++;
			}
			else
			{
				gzwrite(f, &tmp, sizeof(entity));
				et->writeent(*ents[i], ebuf);
				if(et->extraentinfosize()) gzwrite(f, ebuf, et->extraentinfosize());
			}
		}
	}
	delete[] ebuf;

	if (verbose >= 2)
	{
		console("saved %d entities and %d extended entities", CON_RIGHT, hdr.numents, ecount);
	}

	savec(worldroot, f, nolms);
	if(!nolms) loopv(lightmaps)
	{
		LightMap &lm = lightmaps[i];
		show_out_of_renderloop_progress(float(i)/float(lightmaps.length()), "saving lightmaps...");
		gzputc(f, lm.type | (lm.unlitx>=0 ? 0x80 : 0));
		if(lm.unlitx>=0)
		{
			writeushort(f, ushort(lm.unlitx));
			writeushort(f, ushort(lm.unlity));
		}
		gzwrite(f, lm.data, sizeof(lm.data));
	}
	if (verbose >= 2) console("saved %d lightmaps", CON_RIGHT, lightmaps.length());

	// config
	fprintf(h, "// %s (%s) config generated by Blood Frontier\n\n", mapname, hdr.maptitle);

	// skybox
	fprintf(h, "loadsky \"%s\" %.f\n\n", lastsky, spinsky);

	// world variables
	int numvars = 0;
	enumerate(*idents, ident, id, {
		show_out_of_renderloop_progress(float(i)/float((*idents).size), "saving world variables...");
		
		if (id._type == ID_VAR && id._world)
		{
			fprintf(h, "%s %d\n", id._name, *id._storage);
			numvars++;
		}
	});
	if (numvars) fprintf(h, "\n");

	if (verbose >= 2) console("saved %d variables", CON_RIGHT, numvars);

	// texture slots
	#define saveslot(s,b) \
		if (b) \
		{ \
			if (s.shader) \
			{ \
				fprintf(h, "setshader %s\n", s.shader->name); \
				loopvj(s.params) \
				{ \
					fprintf(h, "set%sparam", s.params[j].type == SHPARAM_UNIFORM ? "uniform" : (s.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")); \
					if (s.params[j].type == SHPARAM_UNIFORM) fprintf(h, " \"%s\"", s.params[j].name); \
					else fprintf(h, " %d", s.params[j].index);\
					loopk(4) fprintf(h, " %d", s.params[j].val[k]); \
					fprintf(h, "\n"); \
				} \
				fprintf(h, "\n"); \
			} \
		} \
		loopvj(s.sts) \
		{ \
			fprintf(h, "texture"); \
			if (b) fprintf(h, " %s", findtexturename(s.sts[j].type)); \
			else if (!j) fprintf(h, " %s", findmaterialname(i)); \
			else fprintf(h, " 0"); \
			fprintf(h, " \"%s\"", s.sts[j].name); \
			fprintf(h, " %d %d %d %.f\n", \
				s.sts[j].rotation, s.sts[j].xoffset, s.sts[j].yoffset, s.sts[j].scale); \
		} \
		if (b) \
		{ \
			if (s.autograss) \
			{ \
				fprintf(h, "autograss \"%s\"\n", s.autograss); \
			} \
		} \
		fprintf(h, "\n"); \

	loopi(MAT_EDIT) 
	{
		show_out_of_renderloop_progress(float(i)/float(MAT_EDIT), "saving material slots...");

		if (i == MAT_WATER || i == MAT_LAVA)
		{
			saveslot(materialslots[i], false);
		}
	}
	fprintf(h, "\n");
	if (verbose >= 2) console("saved %d material slots", CON_RIGHT, MAT_EDIT);

	loopv(slots)
	{
		show_out_of_renderloop_progress(float(i)/float(slots.length()), "saving texture slots...");
		saveslot(slots[i], true);
	}
	if (slots.length()) fprintf(h, "\n");
	if (verbose >= 2) console("saved %d texture slots", CON_RIGHT, slots.length());

	loopv(mapmodels)
	{
		show_out_of_renderloop_progress(float(i)/float(mapmodels.length()), "saving mapmodel slots...");
		fprintf(h, "mmodel \"%s\" %d\n", mapmodels[i].name, mapmodels[i].tex);
	}
	if (mapmodels.length()) fprintf(h, "\n");
	if (verbose >= 2) console("saved %d mapmodel slots", CON_RIGHT, mapmodels.length());

	loopv(mapsounds)
	{
		show_out_of_renderloop_progress(float(i)/float(mapsounds.length()), "saving mapsound slots...");
		fprintf(h, "mapsound \"%s\" %d %d\n", mapsounds[i].s->name, mapsounds[i].vol, mapsounds[i].maxuses);
	}
	if (mapsounds.length()) fprintf(h, "\n");
	if (verbose >= 2) console("saved %d mapsound slots", CON_RIGHT, mapsounds.length());

	show_out_of_renderloop_progress(0, "saving world...");
	cl->saveworld(mname);

	fclose(h);
	if (verbose) console("saved file '%s'", CON_RIGHT, mcfname, EXTVERSION);
	gzclose(g);
	if (verbose) console("saved file '%s' version %d", CON_RIGHT, extname, EXTVERSION);
	gzclose(f);
	if (verbose) console("saved file '%s' version %d", CON_RIGHT, cgzname, MAPVERSION);

	conoutf("saved map '%s' in %.1f secs", mapname, (SDL_GetTicks()-savingstart)/1000.0f);
#else
	
	writeushort(f, texmru.length());
	loopv(texmru) writeushort(f, texmru[i]);
	char *ebuf = new char[et->extraentinfosize()];
	loopv(ents)
	{
		if(ents[i]->type!=ET_EMPTY)
		{
			entity tmp = *ents[i];
			endianswap(&tmp.o, sizeof(int), 3);
			endianswap(&tmp.attr1, sizeof(short), 5);
			gzwrite(f, &tmp, sizeof(entity));
			et->writeent(*ents[i], ebuf);
			if(et->extraentinfosize()) gzwrite(f, ebuf, et->extraentinfosize());
		}
	}
	delete[] ebuf;

	savec(worldroot, f, nolms);
	if(!nolms) loopv(lightmaps)
	{
		LightMap &lm = lightmaps[i];
		gzputc(f, lm.type | (lm.unlitx>=0 ? 0x80 : 0));
		if(lm.unlitx>=0)
		{
			writeushort(f, ushort(lm.unlitx));
			writeushort(f, ushort(lm.unlity));
		}
		gzwrite(f, lm.data, sizeof(lm.data));
	}

	gzclose(f);
	conoutf("wrote map file %s", cgzname);
#endif
}

void swapXZ(cube *c)
{	
	loopi(8) 
	{
		swap(uint,	c[i].faces[0],	c[i].faces[2]);
		swap(ushort, c[i].texture[0], c[i].texture[4]);
		swap(ushort, c[i].texture[1], c[i].texture[5]);
		if(c[i].ext && c[i].ext->surfaces)
		{
			swap(surfaceinfo, c[i].ext->surfaces[0], c[i].ext->surfaces[4]);
			swap(surfaceinfo, c[i].ext->surfaces[1], c[i].ext->surfaces[5]);
		}
		if(c[i].children) swapXZ(c[i].children);
	}
}

void load_world(const char *mname, const char *cname)		// still supports all map formats that have existed since the earliest cube betas!
{
	int loadingstart = SDL_GetTicks();
	setnames(mname, cname);
	gzFile f = opengzfile(cgzname, "rb9");
#ifdef BFRONTIER // map extensions, verbosity support
	extversion = 0;
	if(!f) { conoutf("error loading '%s' from '%s': file error", mapname, cgzname); return; }
	header newhdr;
	gzread(f, &newhdr, sizeof(header));
	endianswap(&newhdr.version, sizeof(int), 9);
	
	if(strncmp(newhdr.head, "OCTA", 4)!=0) { conoutf("error loading '%s' from '%s': malformatted header", mapname, cgzname); gzclose(f); return; }
	if(newhdr.version>MAPVERSION) { conoutf("error loading '%s' from '%s': requires a newer version of Blood Frontier", mapname, cgzname); gzclose(f); return; }
	hdr = newhdr;
	
	gzFile g = opengzfile(extname, "rb9");
	if(!g) { conoutf("error loading '%s' from '%s': file error", mapname, extname); return; }
	
	char ehead[4];
	gzread(g, &ehead, 4);
	extversion = gzgetint(g);
	
	if ((extversion >= 4 && strncmp(ehead, "EXTZ", 4)!=0) ||
		(extversion <= 3 && strncmp(ehead, "ENTZ", 4)!=0)) // from our old school days
	{
		conoutf("error loading '%s' from '%s': malformatted header", mapname, extname);
		gzclose(g);
		extversion = 0;
		return;
	}
	else if (extversion > EXTVERSION)
	{
		conoutf("error loading '%s' from '%s': requires a newer version of Blood Frontier", mapname, extname);
		gzclose(g);
		extversion = 0;
		return;
	}

	resetmap(true);

	show_out_of_renderloop_progress(0, "loading world...");
	cl->loadworld(cname ? cname : mname);
#else
	if(!f) { conoutf("could not read map %s", cgzname); return; }
	header newhdr;
	gzread(f, &newhdr, sizeof(header));
	endianswap(&newhdr.version, sizeof(int), 9);
	if(strncmp(newhdr.head, "OCTA", 4)!=0) { conoutf("map %s has malformatted header", cgzname); gzclose(f); return; }
	if(newhdr.version>MAPVERSION) { conoutf("map %s requires a newer version of cube 2", cgzname); gzclose(f); return; }
	hdr = newhdr;
	resetmap();
#endif
	Texture *mapshot = textureload(picname, 0, true, false);
    computescreen(mname, mapshot!=notexture ? mapshot : NULL);
	if(hdr.version<=20) conoutf("loading older / less efficient map format, may benefit from \"calclight 2\", then \"savecurrentmap\"");
	if(!hdr.ambient) hdr.ambient = 25;
	if(!hdr.lerpsubdivsize)
	{
		if(!hdr.lerpangle) hdr.lerpangle = 44;
		hdr.lerpsubdiv = 2;
		hdr.lerpsubdivsize = 4;
	}
	setvar("lightprecision", hdr.mapprec ? hdr.mapprec : 32);
	setvar("lighterror", hdr.maple ? hdr.maple : 8);
	setvar("bumperror", hdr.mapbe ? hdr.mapbe : 3);
	setvar("lightlod", hdr.mapllod);
	setvar("lodsize", hdr.mapwlod);
	setvar("ambient", hdr.ambient);
#ifndef BFRONTIER // fullbright and fullbrightlevel, world variable support
	setvar("fullbright", 0);
#endif
	setvar("lerpangle", hdr.lerpangle);
	setvar("lerpsubdiv", hdr.lerpsubdiv);
	setvar("lerpsubdivsize", hdr.lerpsubdivsize);
	
	string gametype;
	s_strcpy(gametype, "fps");

	bool samegame = true;
	int eif = 0;
	if(hdr.version>=16)
	{
		int len = gzgetc(f);
		gzread(f, gametype, len+1);
	}
#ifdef BFRONTIER // game canload control
	if(!sv->canload(gametype, hdr.version))
#else
	if(strcmp(gametype, cl->gameident())!=0)
#endif
	{
		samegame = false;
		conoutf("WARNING: loading map from %s game, ignoring entities except for lights/mapmodels)", gametype);
	}
	if(hdr.version>=16)
	{
		eif = readushort(f);
		int extrasize = readushort(f);
		vector<char> extras;
		loopj(extrasize) extras.add(gzgetc(f));
		if(samegame) cl->readgamedata(extras);
	}
	
	show_out_of_renderloop_progress(0, "clearing world...");

	texmru.setsize(0);
	if(hdr.version<14)
	{
		uchar oldtl[256];
		gzread(f, oldtl, sizeof(oldtl));
		loopi(256) texmru.add(oldtl[i]);
	}
	else
	{
		ushort nummru = readushort(f);
		loopi(nummru) texmru.add(readushort(f));
	}

	freeocta(worldroot);
	worldroot = NULL;

	show_out_of_renderloop_progress(0, "loading entities...");

	vector<extentity *> &ents = et->getents();
	char *ebuf = new char[et->extraentinfosize()];
	loopi(hdr.numents)
	{
#ifdef BFRONTIER // verbosity
		show_out_of_renderloop_progress(float(i)/float(hdr.numents), "loading entities...");
#endif
		extentity &e = *et->newentity();
		ents.add(&e);
		gzread(f, &e, sizeof(entity));
		endianswap(&e.o, sizeof(int), 3);
		endianswap(&e.attr1, sizeof(short), 5);
		e.spawned = false;
		e.inoctanode = false;
		if(samegame)
		{
			if(et->extraentinfosize()) gzread(f, ebuf, et->extraentinfosize());
			et->readent(e, ebuf); 
		}
		else
		{
			loopj(eif) gzgetc(f);
		}
		if(hdr.version <= 14 && e.type >= ET_MAPMODEL && e.type <= 16)
		{
			if(e.type == 16) e.type = ET_MAPMODEL;
			else e.type++;
		}
		if(hdr.version <= 20 && e.type >= ET_ENVMAP) e.type++;
		if(hdr.version <= 21 && e.type >= ET_PARTICLES) e.type++;
		if(hdr.version <= 22 && e.type >= ET_SOUND) e.type++;
		if(hdr.version <= 23 && e.type >= ET_SPOTLIGHT) e.type++;
		if(!samegame)
		{
			if(e.type>=ET_GAMESPECIFIC || hdr.version<=14)
			{
				ents.pop();
				continue;
			}
		}
		if(!insideworld(e.o))
		{
			if(e.type != ET_LIGHT && e.type != ET_SPOTLIGHT)
			{
				conoutf("warning: ent outside of world: enttype[%s] index %d (%f, %f, %f)", et->entname(e.type), i, e.o.x, e.o.y, e.o.z);
			}
		}
		if(hdr.version <= 14 && e.type == ET_MAPMODEL)
		{
			e.o.z += e.attr3;
			if(e.attr4) conoutf("warning: mapmodel ent (index %d) uses texture slot %d", i, e.attr4);
			e.attr3 = e.attr4 = 0;
		}
	}
	delete[] ebuf;
#ifdef BFRONTIER // verbosity
	if (verbose >= 2) console("loaded %d entities", CON_RIGHT, hdr.numents);
#endif

	show_out_of_renderloop_progress(0, "loading octree...");
	worldroot = loadchildren(f);

	if(hdr.version <= 11)
		swapXZ(worldroot);

	if(hdr.version <= 8)
		converttovectorworld();

	show_out_of_renderloop_progress(0, "validating...");
	validatec(worldroot, hdr.worldsize>>1);

	if(hdr.version >= 7) loopi(hdr.lightmaps)
	{
		show_out_of_renderloop_progress(i/(float)hdr.lightmaps, "loading lightmaps...");
		LightMap &lm = lightmaps.add();
		if(hdr.version >= 17)
		{
			int type = gzgetc(f);
			lm.type = type&0xF;
			if(hdr.version >= 20 && type&0x80)
			{
				lm.unlitx = readushort(f);
				lm.unlity = readushort(f);
			}
		}
		gzread(f, lm.data, 3 * LM_PACKW * LM_PACKH);
		lm.finalize();
	}

#ifdef BFRONTIER // verbosity, map extensions, extended entities, world variables
	if (verbose >= 2) console("loaded %d lightmaps", CON_RIGHT, hdr.lightmaps);

	gzclose(f);
	if (verbose) console("loaded file '%s' version %d", CON_RIGHT, cgzname, hdr.version);

	if (extversion >= 1)
	{
		int numents = gzgetint(g); // number of extents
	
		loopi(numents)
		{
			show_out_of_renderloop_progress(float(i)/float(numents), "loading extended entities...");
	
			extentity &e = *et->newentity();
			ents.add(&e);
			gzread(g, &e, sizeof(entity));
			endianswap(&e.o, sizeof(int), 3);
			endianswap(&e.attr1, sizeof(short), 5);
			e.spawned = false;
			e.inoctanode = false;
			
			if (extversion <= 3) e.type -= 74;

			if (!samegame)
			{
				if (e.type >= ET_GAMESPECIFIC || hdr.version <= 14)
				{
					ents.pop();
					continue;
				}
			}

			if (!insideworld(e.o))
			{
				conoutf("warning: ent outside of world: enttype[%s] index %d (%f, %f, %f)", et->entname(e.type), i, e.o.x, e.o.y, e.o.z);
			}
			
			et->readext(g, extversion, hdr.numents, i, e);
		}
		
		if (verbose >= 2) console("loaded %d entities", CON_RIGHT, numents);
	}

	gzclose(g);
	if (verbose) console("loaded file '%s' version %d", CON_RIGHT, extname, extversion);

	conoutf("loaded map '%s' in %.1f secs", mapname, (SDL_GetTicks()-loadingstart)/1000.0f);
	console("%s: %s", CON_CENTER|CON_LEFT, sv->gametitle(), hdr.maptitle);
#else
	gzclose(f);

	conoutf("read map %s (%.1f seconds)", cgzname, (SDL_GetTicks()-loadingstart)/1000.0f);
	conoutf("%s", hdr.maptitle);

	overrideidents = true;
	execfile("data/default_map_settings.cfg");
	execfile(pcfname);
	execfile(mcfname);
	overrideidents = false;
#endif

	loopv(ents)
	{
		extentity &e = *ents[i];
		if(e.type==ET_MAPMODEL && e.attr2 >= 0)
		{
			mapmodelinfo &mmi = getmminfo(e.attr2);
			if(!&mmi) conoutf("could not find map model: %d", e.attr2);
			else if(!loadmodel(NULL, e.attr2, true)) conoutf("could not load model: %s", mmi.name);
		}
	}

	initlights();
#ifdef BFRONTIER // fix the difference in gridsizes on older maps
	mpremip(true);
#endif
	allchanged(true);

    computescreen(mname, mapshot!=notexture ? mapshot : NULL);
	attachentities();

#ifdef BFRONTIER // verbosity
	show_out_of_renderloop_progress(0, "starting world...");
#endif
	startmap(cname ? cname : mname);
}

void savecurrentmap() { save_world(cl->getclientmap()); }
void savemap(char *mname) { save_world(mname); }

COMMAND(savemap, "s");
COMMAND(savecurrentmap, "");

void writeobj(char *name)
{
	bool oldVBO = hasVBO;
	hasVBO = false;
	allchanged();
	s_sprintfd(fname)("%s.obj", name);
    FILE *f = openfile(path(fname), "w"); 
	if(!f) return;
#ifdef BFRONTIER // blood frontier
	fprintf(f, "# obj file octa world\n");
#else
	fprintf(f, "# obj file of sauerbraten level\n");
#endif
	extern vector<vtxarray *> valist;
	loopv(valist)
	{
		vtxarray &va = *valist[i];
		uchar *verts = (uchar *)va.vbuf;
		if(!verts) continue;
		int vtxsize = VTXSIZE;
		loopj(va.verts) 
		{
			vvec vv;
			if(floatvtx) { vec &f = *(vec *)verts; loopk(3) vv[k] = short(f[k]); }
			else vv = *(vvec *)verts;
			vec v = vv.tovec(va.x, va.y, va.z);
			if(vv.x&((1<<VVEC_FRAC)-1)) fprintf(f, "v %.3f ", v.x); else fprintf(f, "v %d ", int(v.x));
			if(vv.y&((1<<VVEC_FRAC)-1)) fprintf(f, "%.3f ", v.y); else fprintf(f, "%d ", int(v.y));
			if(vv.z&((1<<VVEC_FRAC)-1)) fprintf(f, "%.3f\n", v.z); else fprintf(f, "%d\n", int(v.z));
			verts += vtxsize;
		}
		ushort *ebuf = va.l0.ebuf;
        loopi(va.l0.tris)
		{
			fprintf(f, "f");
			for(int k = 0; k<3; k++) fprintf(f, " %d", ebuf[k]-va.verts);
			ebuf += 3;
			fprintf(f, "\n");
		}
	}
	fclose(f);
	hasVBO = oldVBO;
	allchanged();
}  
	
COMMAND(writeobj, "s"); 

#ifdef BFRONTIER // external map title
char *getmaptitle() { return hdr.maptitle; }
#endif
