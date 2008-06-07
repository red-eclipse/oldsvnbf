struct fpsrender
{
	GAMECLIENT &cl;

	IVARP(targetlaser, 0, 0, 1);

	fpsrender(GAMECLIENT &_cl) : cl(_cl) {}

	vector<fpsent *> bestplayers;
    vector<int> bestteams;

	void renderplayer(fpsent *d, bool local)
	{
		int team = m_team(cl.gamemode, cl.mutators) ? d->team : TEAM_NEUTRAL;
        int lastaction = 0, animflags = 0, animdelay = 0;

		if(cl.intermission && d->state != CS_DEAD)
		{
			lastaction = lastmillis;
			animflags = ANIM_LOSE|ANIM_LOOP;
			animdelay = 1000;
			if(m_team(cl.gamemode, cl.mutators)) loopv(bestteams) { if(bestteams[i] == d->team) { animflags = ANIM_WIN|ANIM_LOOP; break; } }
			else if(bestplayers.find(d)>=0) animflags = ANIM_WIN|ANIM_LOOP;
		}
        else if (d->state == CS_ALIVE && d->lasttaunt && lastmillis-d->lasttaunt<1000 && lastmillis-lastaction>animdelay)
		{
			lastaction = d->lasttaunt;
			animflags = ANIM_TAUNT;
			animdelay = 1000;
		}
		else if (isgun(d->gunselect) && lastmillis-d->gunlast[d->gunselect] < d->gunwait[d->gunselect])
		{
			switch(d->gunstate[d->gunselect])
			{
				case GUNSTATE_SWITCH: animflags = ANIM_SWITCH; break;
				case GUNSTATE_SHOOT: animflags = d->gunselect != GUN_GL ? ANIM_SHOOT : ANIM_THROW; break;
				case GUNSTATE_RELOAD: animflags = d->gunselect != GUN_GL ? ANIM_RELOAD : ANIM_HOLD; break;
				case GUNSTATE_NONE:
				default: break;
			}
			lastaction = d->gunlast[d->gunselect];
			animdelay = d->gunwait[d->gunselect] + 50;
		}
        modelattach a[4] = { { NULL }, { NULL }, { NULL }, { NULL } };
		static const char *vweps[] = { "weapons/pistol/vwep", "weapons/shotgun/vwep", "weapons/chaingun/vwep", "weapons/grenades/vwep", "weapons/flamer/vwep", "weapons/rifle/vwep", "weapons/rockets/vwep"};
        int ai = 0;
        if (d->gunselect > -1 && d->gunselect < NUMGUNS)
		{
            a[ai].name = vweps[d->gunselect];
            a[ai].tag = "tag_weapon";
            a[ai].anim = ANIM_VWEP|ANIM_LOOP;
            a[ai].basetime = 0;
            ai++;
		}
        renderclient(d, local, teamtype[team].mdl, a[0].name ? a : NULL, animflags, animdelay, lastaction, cl.intermission ? 0 : d->lastpain);

		if(team) s_sprintf(d->info)("%s (%s)", cl.colorname(d, NULL, "@"), teamtype[team].name);
		else s_sprintf(d->info)("%s", cl.colorname(d, NULL, "@"));
		if(!local) part_text(d->abovehead(), d->info, 10, 1, teamtype[team].colour);
	}

	void render()
	{
		if(cl.intermission)
		{
			if(m_team(cl.gamemode, cl.mutators)) { bestteams.setsize(0); cl.sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); cl.sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

		fpsent *d;
        loopv(cl.players) if((d = cl.players[i]) && d->state!=CS_SPECTATOR &&
				d->state!=CS_SPAWNING && (d->state!=CS_DEAD || d->superdamage<50))
					renderplayer(d, false);
		if(cl.player1->state == CS_ALIVE || (cl.player1->state == CS_DEAD && cl.player1->superdamage < 50))
			renderplayer(cl.player1, true);

		cl.et.render();
		cl.pj.render();
		if(m_stf(cl.gamemode)) cl.stf.render();
        else if(m_ctf(cl.gamemode)) cl.ctf.render();

        cl.bot.render();

		endmodelbatches();

		if(targetlaser() && rendernormally)
		{
			renderprimitive(true);
			vec v = cl.ws.gunorigin(cl.player1->gunselect, cl.player1->o, worldpos, cl.player1);
			renderline(v, worldpos, 0.2f, 0.0f, 0.0f, false);
			renderprimitive(false);
		}
	}

    void preload()
    {
		loadmodel("player", -1, true);
    }
} fr;
