struct ctfstate
{
    static const int FLAGRADIUS = 16;

    struct flag
    {
        vec droploc, spawnloc;
        int team, score, droptime;
#ifdef CTFSERV
        int owner;
#else
        bool pickup;
        fpsent *owner;
        extentity *ent;
        vec interploc;
        float interpangle;
        int interptime;
#endif

        flag() { reset(); }

        void reset()
        {
        	team = -1;
            droploc = spawnloc = vec(0, 0, 0);
#ifdef CTFSERV
            owner = -1;
#else
            pickup = false;
            owner = NULL;
            interploc = vec(0, 0, 0);
            interpangle = 0;
            interptime = 0;
#endif
            score = 0;
            droptime = 0;
        }
    };
    vector<flag> flags;

    void reset()
    {
        flags.setsize(0);
    }

    void addflag(int i, int t, const vec &o)
    {
        while(!flags.inrange(i)) flags.add();
		flag &f = flags[i];
		f.reset();
		f.spawnloc = o;
		f.team = t;
    }

#ifdef CTFSERV
    void takeflag(int i, int owner)
#else
    void takeflag(int i, fpsent *owner)
#endif
    {
    	if (flags.inrange(i))
    	{
			flag &f = flags[i];
			f.owner = owner;
#ifndef CTFSERV
			f.pickup = false;
#endif
    	}
    }

    void dropflag(int i, const vec &o, int droptime)
    {
    	if (flags.inrange(i))
    	{
			flag &f = flags[i];
			f.droploc = o;
			f.droptime = droptime;
#ifdef CTFSERV
			f.owner = -1;
#else
			f.pickup = false;
			f.owner = NULL;
#endif
    	}
    }

    void returnflag(int i)
    {
    	if (flags.inrange(i))
    	{
			flag &f = flags[i];
			f.droptime = 0;
#ifdef CTFSERV
			f.owner = -1;
#else
			f.pickup = false;
			f.owner = NULL;
#endif
    	}
    }

	int numteams(bool multi)
	{
		return multi ? TEAM_MAX : TEAM_MAX/2;
	}

	int teamflag(const char *s, bool multi)
	{
		loopi(numteams(multi))
		{
			if (!strcmp(s, teamnames[i])) return i;
		}
		return -1;
	}

	const char *flagteam(int t, bool multi)
	{
		if (t<=0 || t>=numteams(multi)) return teamnames[0];
		return teamnames[t];
	}
};

#ifdef CTFSERV
struct ctfservmode : ctfstate, servmode
{
    static const int RESETFLAGTIME = 10000;

    bool notgotflags;

    ctfservmode(GAMESERVER &sv) : servmode(sv), notgotflags(false) {}

    void reset(bool empty)
    {
        ctfstate::reset();
        notgotflags = !empty;
    }

    void dropflag(clientinfo *ci)
    {
        if(notgotflags) return;
        loopv(flags) if(flags[i].owner==ci->clientnum)
        {
            ivec o(vec(ci->state.o).mul(DMF));
            sendf(-1, 1, "ri6", SV_DROPFLAG, ci->clientnum, i, o.x, o.y, o.z);
            ctfstate::dropflag(i, o.tovec().div(DMF), sv.gamemillis);
        }
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        dropflag(ci);
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        dropflag(ci);
    }

    bool canchangeteam(clientinfo *ci, const char *oldteam, const char *newteam)
    {
    	loopi(m_ttwo(sv.gamemode, sv.mutators) ? TEAM_MAX : TEAM_MAX/2)
    	{
			if (!strcmp(newteam, teamnames[i])) return true;
    	}
    	return false;
    }

    void changeteam(clientinfo *ci, const char *oldteam, const char *newteam)
    {
        dropflag(ci);
    }

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(notgotflags) return;
        static const dynent dummy;
        vec o(newpos);
        o.z -= dummy.height;
        loopv(flags)
        {
            flag &relay = flags[i];
            loopvk(flags)
            {
				flag &goal = flags[k];

				if(relay.owner==ci->clientnum && goal.team==teamflag(ci->team, m_ttwo(sv.gamemode, sv.mutators)) && goal.owner<0 && !goal.droptime && o.dist(goal.spawnloc) < FLAGRADIUS)
				{
					returnflag(i);
					goal.score++;
					sendf(-1, 1, "ri5", SV_SCOREFLAG, ci->clientnum, i, k, goal.score);
				}
            }
        }
    }

    void takeflag(clientinfo *ci, int i)
    {
        if(notgotflags || ci->state.state!=CS_ALIVE || !ci->team[0]) return;

		if (flags.inrange(i))
		{
			flag &f = flags[i];
			if(f.team == teamflag(ci->team, m_ttwo(sv.gamemode, sv.mutators)))
			{
				if(!f.droptime || f.owner>=0) return;
				ctfstate::returnflag(i);
				sendf(-1, 1, "ri3", SV_RETURNFLAG, ci->clientnum, i);
			}
			else
			{
				if(f.owner>=0) return;
				ctfstate::takeflag(i, ci->clientnum);
				sendf(-1, 1, "ri3", SV_TAKEFLAG, ci->clientnum, i);
			}
		}
    }

    void update()
    {
        if(sv.minremain<0 || notgotflags) return;
        loopv(flags)
        {
            flag &f = flags[i];
            if(f.owner<0 && f.droptime && sv.gamemillis - f.droptime >= RESETFLAGTIME)
            {
                returnflag(i);
                sendf(-1, 1, "ri2", SV_RESETFLAG, i);
            }
        }
    }

    void initclient(clientinfo *ci, ucharbuf &p, bool connecting)
    {
        putint(p, SV_INITFLAGS);
        putint(p, flags.length());
        loopv(flags)
        {
            flag &f = flags[i];
            putint(p, f.team);
            putint(p, f.score);
            putint(p, f.owner);
            if(f.owner<0)
            {
                putint(p, f.droptime ? 1 : 0);
                if(f.droptime)
                {
                    putint(p, int(f.droploc.x*DMF));
                    putint(p, int(f.droploc.y*DMF));
                    putint(p, int(f.droploc.z*DMF));
                }
            }
        }
    }

    void parseflags(ucharbuf &p)
    {
    	int numflags = getint(p);
        loopi(numflags)
        {
            vec o;
            int t = getint(p);
            loopk(3) o[k] = getint(p)/DMF;
            if(notgotflags) addflag(i, t, o);
        }
        notgotflags = false;
    }
} ctfmode;
#else
struct ctfclient : ctfstate
{
    static const int RESPAWNSECS = 5;

    GAMECLIENT &cl;
    float radarscale;

    ctfclient(GAMECLIENT &cl) : cl(cl), radarscale(0)
    {
    }

    void preload()
    {
        static const char *flagmodels[2] = { "flags/red", "flags/blue" };
        loopi(2) loadmodel(flagmodels[i], -1, true);
    }

    void drawradar(float x, float y, float s)
    {
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x,   y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x+s, y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x+s, y+s);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x,   y+s);
    }

    void drawblips(int x, int y, int s, int i)
    {
    	if (flags.inrange(i))
    	{
			flag &f = flags[i];
			settexture(f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? "textures/blip_blue.png" : "textures/blip_red.png");
			float scale = radarscale<=0 || radarscale>cl.maxradarscale() ? cl.maxradarscale() : radarscale;
			vec dir(f.owner ? f.owner->o : (f.droptime ? f.droploc : f.spawnloc));
			dir.sub(cl.player1->o);
			dir.z = 0.0f;
			float dist = dir.magnitude();
			if(dist >= scale) dir.mul(scale/dist);
			dir.rotate_around_z(-cl.player1->yaw*RAD);
			glBegin(GL_QUADS);
			drawradar(x + s*0.5f*0.95f*(1.0f+dir.x/scale), y + s*0.5f*0.95f*(1.0f+dir.y/scale), 0.05f*s);
			glEnd();
    	}
    }

    void drawhud(int w, int h)
    {
#if 0
        if(cl.player1->state == CS_ALIVE)
        {
            loopv(flags)
            {
                flag &f = flags[i];
                if(f.team != teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) && f.owner == cl.player1)
                {
                    cl.drawicon(320, 0, 1820, 1650);
                    break;
                }
            }
        }
#endif

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        int x = 1800*w/h*34/40, y = 1800*1/40, s = 1800*w/h*5/40;
        glColor3f(1, 1, 1);
        settexture("textures/radar.png");
        glBegin(GL_QUADS);
        drawradar(float(x), float(y), float(s));
        glEnd();
        loopv(flags)
        {
            flag &f = flags[i];
            if(!f.ent || ((f.owner || f.droptime) && lastmillis%1000 >= 500)) continue;
            drawblips(x, y, s, i);
        }
        if(cl.player1->state == CS_DEAD)
        {
            int wait = respawnwait();
            if(wait>=0)
            {
                glPushMatrix();
                glLoadIdentity();
                glOrtho(0, w*900/h, 900, 0, -1, 1);
                draw_textf("%d", (x+s/2)/2-(wait>=10 ? 28 : 16), (y+s/2)/2-32, wait);
                glPopMatrix();
            }
        }
        glDisable(GL_BLEND);
    }

    vec interpflagpos(flag &f, float &angle)
    {
        vec pos = f.owner ? vec(f.owner->abovehead()).add(vec(0, 0, 1)) : (f.droptime ? f.droploc : f.spawnloc);
        angle = f.owner ? f.owner->yaw : (f.ent ? f.ent->attr1 : 0);
        if(pos.x < 0) return pos;
        if(f.interptime && f.interploc.x >= 0)
        {
            float t = min((lastmillis - f.interptime)/500.0f, 1.0f);
            pos.lerp(f.interploc, pos, t);
            angle += (1-t)*(f.interpangle - angle);
        }
        return pos;
    }

    vec interpflagpos(flag &f) { float angle; return interpflagpos(f, angle); }

    void render()
    {
        loopv(flags)
        {
            flag &f = flags[i];
            if(!f.ent || (!f.owner && f.droptime && f.droploc.x < 0)) continue;
            const char *flagname = f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? "flags/blue" : "flags/red";
            float angle;
            vec pos = interpflagpos(f, angle);
            rendermodel(!f.droptime && !f.owner ? &f.ent->light : NULL, flagname, ANIM_MAPMODEL|ANIM_LOOP,
                        interpflagpos(f), angle, 0,
                        MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED | (f.droptime || f.owner ? MDL_LIGHT : 0));

            vec above(pos);
            above.z += enttype[BASE].height;
            s_sprintfd(info)("@%s flag", flagteam(f.team, m_ttwo(cl.gamemode, cl.mutators)));
			particle_text(above, info, f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? 16 : 13, 1);
        }
    }

    void setupflags()
    {
        reset();
        int x = 0;
        loopv(cl.et.ents)
        {
            extentity *e = cl.et.ents[i];
            if(e->type!=BASE || e->attr2<1 || e->attr2>numteams(m_ttwo(cl.gamemode, cl.mutators))) continue;
            addflag(x, e->attr2-1, e->o);
            flags[e->attr2-1].ent = e;
            x++;
        }
        vec center(0, 0, 0);
        loopv(flags) center.add(flags[i].spawnloc);
        center.div(2);
        radarscale = 0;
        loopv(flags) radarscale = max(radarscale, 2*center.dist(flags[i].spawnloc));
    }

    void sendflags(ucharbuf &p)
    {
        putint(p, SV_INITFLAGS);
		putint(p, flags.length());
        loopv(flags)
        {
            flag &f = flags[i];
            putint(p, f.team);
            loopk(3) putint(p, int(f.spawnloc[k]*DMF));
        }
    }

    void parseflags(ucharbuf &p, bool commit)
    {
    	int numflags = getint(p);
        loopi(numflags)
        {
        	while (!flags.inrange(i)) flags.add();
            flag &f = flags[i];
            int team = getint(p), score = getint(p), owner = getint(p), dropped = 0;
            vec droploc(f.spawnloc);
            if(owner<0)
            {
                dropped = getint(p);
                if(dropped) loopk(3) droploc[k] = getint(p)/DMF;
            }
            if(commit)
            {
				f.team = team;
                f.score = score;
                f.owner = owner>=0 ? (owner==cl.player1->clientnum ? cl.player1 : cl.newclient(owner)) : NULL;
                f.droptime = dropped;
                f.droploc = droploc;
                f.interptime = 0;

                if(dropped)
                {
                    if(!cl.ph.droptofloor(f.droploc, 2, 0)) f.droploc = vec(-1, -1, -1);
                }
            }
        }
    }

    void dropflag(fpsent *d, int i, const vec &droploc)
    {
    	if (flags.inrange(i))
		{
			flag &f = flags[i];
			f.interploc = interpflagpos(f, f.interpangle);
			f.interptime = lastmillis;
			ctfstate::dropflag(i, droploc, 1);
			if(!cl.ph.droptofloor(f.droploc, 2, 0))
			{
				f.droploc = vec(-1, -1, -1);
				f.interptime = 0;
			}
			conoutf("%s dropped %s flag", d==cl.player1 ? "you" : cl.colorname(d), f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
			playsound(S_FLAGDROP);
		}
    }

    void flagexplosion(int i, const vec &loc)
    {
    	if (flags.inrange(i))
    	{
    		flag &f = flags[i];
			int ftype;
			vec color;
			if(f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators))) { ftype = 36; color = vec(0.25f, 0.25f, 1); }
			else { ftype = 35; color = vec(1, 0.25f, 0.25f); }
			particle_fireball(loc, 30, ftype);
			adddynlight(loc, 35, color, 900, 100);
			particle_splash(0, 150, 300, loc);
    	}
    }

    void flageffect(int i, const vec &from, const vec &to)
    {
    	if (flags.inrange(i))
    	{
    		flag &f = flags[i];
			vec fromexp(from), toexp(to);
			if(from.x >= 0)
			{
				fromexp.z += 8;
				flagexplosion(i, fromexp);
			}
			if(to.x >= 0)
			{
				toexp.z += 8;
				flagexplosion(i, toexp);
			}
			if(from.x >= 0 && to.x >= 0)
				particle_flare(fromexp, toexp, 600, f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? 30 : 29);
    	}
    }

    void returnflag(fpsent *d, int i)
    {
    	if (flags.inrange(i))
    	{
			flag &f = flags[i];
			flageffect(i, interpflagpos(f), f.spawnloc);
			f.interptime = 0;
			ctfstate::returnflag(i);
			conoutf("%s returned %s flag", d==cl.player1 ? "you" : cl.colorname(d), f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
			playsound(S_FLAGRETURN);
    	}
    }

    void resetflag(int i)
    {
    	if (flags.inrange(i))
    	{
			flag &f = flags[i];
			flageffect(i, interpflagpos(f), f.spawnloc);
			f.interptime = 0;
			ctfstate::returnflag(i);
			conoutf("%s flag reset", f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
			playsound(S_FLAGRESET);
    	}
    }

	int findscore(const char *team)
	{
		int s = 0, t = teamflag(team, m_ttwo(cl.gamemode, cl.mutators));
		loopv(flags)
		{
			flag &f = flags[i];
			if (f.team == t) s += f.score;
		}
		return s;
	}

    void scoreflag(fpsent *d, int relay, int goal, int score)
    {
    	if (flags.inrange(goal))
    	{
			flag &f = flags[goal];
			flageffect(goal, flags[goal].spawnloc, flags[relay].spawnloc);
			f.score = score;
			f.interptime = 0;
			ctfstate::returnflag(relay);
			if(d!=cl.player1)
			{
				s_sprintfd(ds)("@%d", score);
				particle_text(d->abovehead(), ds, 9);
			}
			conoutf("%s scored for %s team", d==cl.player1 ? "you" : cl.colorname(d), f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
			playsound(S_FLAGSCORE);
    	}
    }

    void takeflag(fpsent *d, int i)
    {
    	if (flags.inrange(i))
    	{
			flag &f = flags[i];
			f.interploc = interpflagpos(f, f.interpangle);
			f.interptime = lastmillis;
			conoutf("%s %s %s flag", d==cl.player1 ? "you" : cl.colorname(d), f.droptime ? "picked up" : "stole", f.team==teamflag(cl.player1->team, m_ttwo(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
			ctfstate::takeflag(i, d);
			playsound(S_FLAGPICKUP);
    	}
    }

    void checkflags(fpsent *d)
    {
        vec o = d->o;
        o.z -= d->height;
        loopv(flags)
        {
            flag &f = flags[i];
            if(!f.ent || f.owner || (f.droptime ? f.droploc.x<0 : f.team==teamflag(d->team, m_ttwo(cl.gamemode, cl.mutators)))) continue;
            if(o.dist(f.droptime ? f.droploc : f.spawnloc) < FLAGRADIUS)
            {
                if(f.pickup) continue;
                cl.cc.addmsg(SV_TAKEFLAG, "ri", i);
                f.pickup = true;
            }
            else f.pickup = false;
       }
    }

    int respawnwait()
    {
        return max(0, RESPAWNSECS-(lastmillis-cl.player1->lastpain)/1000);
    }
} ctf;
#endif

