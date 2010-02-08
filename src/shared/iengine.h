// the interface the game uses to access the engine

extern int kidmode, verbose, curtime, lastmillis, totalmillis, timescale, paused;
extern int servertype, serverport, servermasterport;
extern char *servermaster, *serverip;
extern void fatal(const char *s, ...);
extern void console(int type, const char *s, ...);
extern void conoutft(int type, const char *s, ...);
extern void conoutf(const char *s, ...);
extern void updatetimer();

#ifdef __GNUC__
#define _dbg_ fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#else
#define _dbg_ fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
#endif

struct generic { const char *name; uchar id; };

extern void lightent(extentity &e, float height = 8);
extern void lightreaching(const vec &target, vec &color, vec &dir, extentity *e = 0, float ambient = 0.4f);
extern const extentity *brightestlight(const vec &target, const vec &dir);

enum { RAY_BB = 1, RAY_POLY = 3, RAY_ALPHAPOLY = 7, RAY_ENTS = 9, RAY_CLIPMAT = 16, RAY_SKIPFIRST = 32, RAY_EDITMAT = 64, RAY_SHADOW = 128, RAY_PASS = 256 };

extern float raycube   (const vec &o, const vec &ray,     float radius = 0, int mode = RAY_CLIPMAT, int size = 0, extentity *t = 0);
extern float raycubepos(const vec &o, const vec &ray, vec &hit, float radius = 0, int mode = RAY_CLIPMAT, int size = 0);
extern float rayfloor  (const vec &o, vec &floor, int mode = 0, float radius = 0);
extern bool  raycubelos(const vec &o, const vec &dest, vec &hitpos);

struct Texture;

extern void settexture(const char *name, int clamp = 0);

// octaedit

enum { EDIT_FACE = 0, EDIT_TEX, EDIT_MAT, EDIT_FLIP, EDIT_COPY, EDIT_PASTE, EDIT_ROTATE, EDIT_REPLACE, EDIT_DELCUBE, EDIT_REMIP };

struct selinfo
{
    int corner;
    int cx, cxs, cy, cys;
    ivec o, s;
    int grid, orient;
    int size() const    { return s.x*s.y*s.z; }
    int us(int d) const { return s[d]*grid; }
    bool operator==(const selinfo &sel) const { return o==sel.o && s==sel.s && grid==sel.grid && orient==sel.orient; }
};

struct editinfo;

extern bool editmode;

extern void freeeditinfo(editinfo *&e);
extern void rendereditcursor();
extern void pruneundos(int maxremain = 0);
extern bool noedit(bool view = false);
extern void toggleedit();
extern void mpeditface(int dir, int mode, selinfo &sel, bool local);
extern void mpedittex(int tex, int allfaces, selinfo &sel, bool local);
extern void mpeditmat(int matid, selinfo &sel, bool local);
extern void mpflip(selinfo &sel, bool local);
extern void mpcopy(editinfo *&e, selinfo &sel, bool local);
extern void mppaste(editinfo *&e, selinfo &sel, bool local);
extern void mprotate(int cw, selinfo &sel, bool local);
extern void mpreplacetex(int oldtex, int newtex, selinfo &sel, bool local);
extern void mpdelcube(selinfo &sel, bool local);
extern void mpremip(bool local);

// console
extern int changedkeys;

extern void keypress(int code, bool isdown, int cooked);
extern char *getcurcommand();
extern void resetcomplete();
extern void complete(char *s);
extern const char *searchbind(const char *action, int type);
extern void searchbindlist(const char *action, int type, int limit, const char *sep, const char *pretty, vector<char> &names);

struct bindlist
{
    vector<char> names;
    int lastsearch;

    bindlist() : lastsearch(-1) {}

    const char *search(const char *action, int type = 0, int limit = 5, const char *sep = "or", const char *pretty = "\fw")
    {
        if(names.empty() || lastsearch != changedkeys)
        {
            names.setsize(0);
            searchbindlist(action, type, limit, sep, pretty, names);
            lastsearch = changedkeys;
        }
        return names.getbuf();
    }
};

#define SEARCHBINDCACHE(def) static bindlist __##def; const char *def = __##def.search

// menus
extern void newgui(char *name, char *contents, char *initscript = NULL);
extern void showgui(const char *name, int tab = 0);

// world
extern bool emptymap(int factor, bool force = false, char *mname = NULL, bool nocfg = false);
extern bool enlargemap(bool force);
extern int findentity(int type, int index, vector<int> &attr);
extern void mpeditent(int i, const vec &o, int type, vector<int> &attr, bool local = true);
extern int getworldsize();
extern int getmapversion();

// main
struct igame;
extern void keyrepeat(bool on);

// rendertext
extern char *savecolour, *restorecolour, *green, *blue, *yellow, *red, *gray, *magenta, *orange, *white, *black, *cyan;

enum
{
    TEXT_SHADOW         = 1<<0,
    TEXT_NO_INDENT      = 1<<1,
    TEXT_UPWARD         = 1<<2,

    TEXT_ALIGN          = 3<<8,
    TEXT_LEFT_JUSTIFY   = 0<<8,
    TEXT_CENTERED       = 1<<8,
    TEXT_RIGHT_JUSTIFY  = 2<<8,

    TEXT_LEFT_UP        = TEXT_UPWARD|TEXT_LEFT_JUSTIFY,
    TEXT_CENTER_UP      = TEXT_UPWARD|TEXT_CENTERED,
    TEXT_RIGHT_UP       = TEXT_UPWARD|TEXT_RIGHT_JUSTIFY
};

extern bool setfont(const char *name);
extern bool pushfont(const char *name);
extern bool popfont(int num = 1);
extern int draw_text(const char *str, int rleft, int rtop, int r = 255, int g = 255, int b = 255, int a = 255, int flags = TEXT_SHADOW, int cursor = -1, int maxwidth = -1);
extern int draw_textx(const char *fstr, int left, int top, int r = 255, int g = 255, int b = 255, int a = 255, int flags = TEXT_SHADOW, int cursor = -1, int maxwidth = -1, ...);
extern int draw_textf(const char *fstr, int left, int top, ...);
extern int text_width(const char *str, int flags = 0);
extern void text_bounds(const char *str, int &width, int &height, int maxwidth = -1, int flags = 0);
extern int text_visible(const char *str, int hitx, int hity, int maxwidth = -1, int flags = 0);
extern void text_pos(const char *str, int cursor, int &cx, int &cy, int maxwidth, int flags = 0);

// renderva
enum
{
    DL_SHRINK = 1<<0,
    DL_EXPAND = 1<<1,
    DL_FLASH  = 1<<2,
    DL_KEEP   = 1<<3
};

extern void adddynlight(const vec &o, float radius, const vec &color, int fade = 0, int peak = 0, int flags = 0, float initradius = 0, const vec &initcolor = vec(0, 0, 0), physent *owner = NULL);
extern void dynlightreaching(const vec &target, vec &color, vec &dir);
extern void removetrackeddynlights(physent *owner);
extern void makelightfx(extentity &e, extentity &f);

// rendergl
extern vec worldpos, camerapos, camdir, camright, camup;
extern void getscreenres(int &w, int &h);
extern void gettextres(int &w, int &h);

// renderparticles
enum
{
    PT_PART = 0,
    PT_TAPE,
    PT_TRAIL,
    PT_TEXT,
    PT_FIREBALL,
    PT_LIGHTNING,
    PT_FLARE,
    PT_PORTAL,
    PT_ICON,
    PT_LINE,
    PT_TRIANGLE,
    PT_ELLIPSE,
    PT_CONE,

    PT_MOD      = 1<<8,
    PT_RND4     = 1<<9,     // uses random image quarters
    PT_HFLIP    = 1<<10,    // uses random horizontal flipping
    PT_VFLIP    = 1<<11,    // uses random vertical flipping
    PT_ROT      = 1<<12,    // uses random rotation
    PT_LERP     = 1<<13,    // use very sparingly - order of blending issues
    PT_GLARE    = 1<<14,    // use glare when available
    PT_SOFT     = 1<<15,    // use soft quad rendering when available
    PT_ONTOP    = 1<<16,    // render on top of everything else, remove depth testing
    PT_FEW      = 1<<17,    // allocate smaller number of particles
    PT_SHRINK   = 1<<18,    // shrink particle as it fades
    PT_GROW     = 1<<19,    // grow particle as it fades
    PT_FLIP     = PT_HFLIP | PT_VFLIP | PT_ROT
};

enum
{
    PART_TELEPORT = 0, PART_ICON,
    PART_LINE, PART_TRIANGLE, PART_ELLIPSE, PART_CONE,
    PART_FIREBALL_LERP, PART_PLASMA_LERP, PART_FLARE_LERP, PART_MUZZLE_FLARE_LERP,
    PART_SMOKE_LERP_SOFT, PART_SMOKE_LERP, PART_HINT_LERP_SOFT, PART_HINT_LERP,
    PART_SMOKE_SOFT, PART_SMOKE, PART_HINT_SOFT, PART_HINT,
    PART_BLOOD,
    PART_EDIT, PART_EDIT_ONTOP,
    PART_SPARK,
    PART_FIREBALL_SOFT, PART_FIREBALL,
    PART_PLASMA_SOFT, PART_PLASMA,
    PART_ELECTRIC_SOFT, PART_ELECTRIC,
    PART_FLAME,
    PART_FLARE, PART_MUZZLE_FLARE,
    PART_MUZZLE_FLASH,
    PART_SNOW,
    PART_TEXT, PART_TEXT_ONTOP,
    PART_EXPLOSION, PART_EXPLOSION_NO_GLARE,
    PART_LIGHTNING,
    PART_LENS_FLARE,
    PART_MAX
};

struct particle
{
    vec o, d, m;
    int collide, fade, grav, millis;
    bvec color;
    uchar flags;
    float size, blend;
    union
    {
        const char *text;         // will call delete[] on this only if it starts with an @
        float val;
        struct
        {
            uchar color2[3];
            uchar progress;
        };
    };
    physent *owner;
};

extern void regular_part_create(int type, int fade, const vec &p, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL, int delay = 0);
extern void part_create(int type, int fade, const vec &p, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL);
extern void regular_part_splash(int type, int num, int fade, const vec &p, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0, int radius = 150, int delay = 0);
extern void part_splash(int type, int num, int fade, const vec &p, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0, int radius = 4);
extern void part_trail(int ptype, int fade, const vec &s, const vec &e, int color = 0xFFFFFF, float size = .8f, float blend = 1, int grav = 0, int collide = 0);
extern void part_text(const vec &s, const char *t, int type = PART_TEXT, int fade = 1, int color = 0xFFFFFF, float size = 2, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL);
extern void part_textcopy(const vec &s, const char *t, int type = PART_TEXT, int fade = 1, int color = 0xFFFFFF, float size = 2, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL);
extern void part_flare(const vec &p, const vec &dest, int fade, int type, int color = 0xFFFFFF, float size = 2, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL);
extern void regular_part_fireball(const vec &dest, float maxsize, int type, int fade = 1, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0);
extern void part_fireball(const vec &dest, float maxsize, int type, int fade = 1, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0);
extern void part_spawn(const vec &o, const vec &v, float z, uchar type, int amt = 1, int fade = 1, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0);
extern void part_flares(const vec &o, const vec &v, float z1, const vec &d, const vec &w, float z2, uchar type, int amt = 1, int fade = 1, int color = 0xFFFFFF, float size = 4, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL);
extern void part_portal(const vec &o, float size, float blend = 1, float yaw = 0, float pitch = 0, int type = PART_TELEPORT, int fade = 1, int color = 0xFFFFFF);
extern void part_icon(const vec &o, Texture *tex, float size = 2, float blend = 1, int grav = 0, int collide = 0, int fade = 1, int color = 0xFFFFFF, float start = 0, float length = 1, physent *pl = NULL);
extern void part_line(const vec &o, const vec &v, float size = 1, float blend = 1, int fade = 1, int color = 0xFFFFFF, int type = PART_LINE);
extern void part_triangle(const vec &o, float yaw, float pitch, float size = 1, float blend = 1, int fade = 1, int color = 0xFFFFFF, bool fill = true, int type = PART_TRIANGLE);
extern void part_dir(const vec &o, float yaw, float pitch, float size = 1, float blend = 1, int fade = 1, int color = 0x0000FF, bool fill = true);
extern void part_trace(const vec &o, const vec &v, float size = 1, float blend = 1, int fade = 1, int color = 0xFFFFFF, bool fill = true);
extern void part_ellipse(const vec &o, const vec &v, float size = 1, float blend = 1, int fade = 1, int color = 0xFFFFFF, int axis = 0, bool fill = false, int type = PART_ELLIPSE);
extern void part_radius(const vec &o, const vec &v, float size = 1, float blend = 1, int fade = 1, int color = 0x00FFFF, bool fill = false);
extern void part_cone(const vec &o, const vec &dir, float radius, float angle = 0.f, float size = 1, float blend = 1, int fade = 1, int color = 0x00FFFF, bool fill = false, int type = PART_CONE);

extern void removetrackedparticles(physent *pl = NULL);
extern int particletext, maxparticledistance;

extern particle *newparticle(const vec &o, const vec &d, int fade, int type, int color = 0xFFFFFF, float size = 2, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL);
extern void create(int type, int color, int fade, const vec &p, float size = 2, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL);
extern void regularcreate(int type, int color, int fade, const vec &p, float size = 2, float blend = 1, int grav = 0, int collide = 0, physent *pl = NULL, int delay = 0);
extern void splash(int type, int color, int radius, int num, int fade, const vec &p, float size = 2, float blend = 1, int grav = 0, int collide = 0);
extern void regularsplash(int type, int color, int radius, int num, int fade, const vec &p, float size = 2, float blend = 1, int grav = 0, int collide = 0, int delay = 0);
extern void regularshape(int type, int radius, int color, int dir, int num, int fade, const vec &p, float size = 2, float blend = 1, int grav = 0, int collide = 0, float vel = 1);
extern void regularflame(int type, const vec &p, float radius, float height, int color, int density = 3, int fade = 500, float size = 2, float blend = 1, int grav = -1, int collide = 0, float vel = 1);

#if 0
enum { PARTTYPE_CREATE = 0, PARTTYPE_SPLASH, PARTTYPE_SHAPE, PARTTYPE_FIRE, PARTTYPE_FLAME, PARTTYPE_ICON, PARTTYPE_FLARE, PARTTYPE_MAX };

struct mapparticle
{
    const char *name;
    int part, type, colour, grav, fade, attr;
    float radius, height, size, blend, vel;

    mapparticle() : name(NULL) {}
    ~mapparticle()
    {
        if(name) delete name;
    }
};
extern vector<mapparticle> mapparts;
#endif

// decal
enum
{
    DECAL_SMOKE = 0,
    DECAL_SCORCH, DECAL_SCORCH_SHORT,
    DECAL_BLOOD,
    DECAL_BULLET,
    DECAL_ENERGY,
    DECAL_STAIN
};

extern void adddecal(int type, const vec &center, const vec &surface, float radius, const bvec &color = bvec(0xFF, 0xFF, 0xFF), int info = 0);

// worldio
extern void setnames(const char *fname, int type);
extern bool load_world(const char *mname, bool temp = false);
extern void save_world(const char *mname, bool nodata = false, bool forcesave = false);
extern uint getmapcrc();

// physics
extern bool ellipsecollide(physent *d, const vec &dir, const vec &o, const vec &center, float yaw, float xr, float yr, float hi, float lo);
extern bool rectcollide(physent *d, const vec &dir, const vec &o, float xr, float yr,  float hi, float lo, uchar visible = 0xFF);
extern bool collide(physent *d, const vec &dir = vec(0, 0, 0), float cutoff = 0, bool playercol = true);
extern bool plcollide(physent *d, const vec &dir = vec(0, 0, 0));
extern bool plcollide(physent *d, const vec &dir, physent *o);
extern float pltracecollide(physent *d, const vec &o, const vec &ray, float maxdist);
extern float tracecollide(physent *d, const vec &o, const vec &ray, float maxdist, int mode = RAY_CLIPMAT|RAY_ALPHAPOLY, bool playercol = true);
extern void vecfromyawpitch(float yaw, float pitch, int move, int strafe, vec &m);
extern void vectoyawpitch(const vec &v, float &yaw, float &pitch);
extern bool intersect(physent *d, const vec &from, const vec &to, float &dist);
extern bool overlapsbox(const vec &d, float h1, float r1, const vec &v, float h2, float r2);
extern const vector<physent *> &checkdynentcache(int x, int y);
extern void updatedynentcache(physent *d);
extern void cleardynentcache();

// rendermodel
enum { MDL_CULL_VFC = 1<<0, MDL_CULL_DIST = 1<<1, MDL_CULL_OCCLUDED = 1<<2, MDL_CULL_QUERY = 1<<3, MDL_SHADOW = 1<<4, MDL_DYNSHADOW = 1<<5, MDL_LIGHT = 1<<6, MDL_DYNLIGHT = 1<<7, MDL_FULLBRIGHT = 1<<8, MDL_NORENDER = 1<<9 };

struct model;
struct modelattach
{
    const char *tag, *name;
    int anim, basetime;
    vec *pos;
    model *m;

    modelattach() : tag(NULL), name(NULL), anim(-1), basetime(0), pos(NULL), m(NULL) {}
    modelattach(const char *tag, const char *name, int anim = -1, int basetime = 0) : tag(tag), name(name), anim(anim), basetime(basetime), pos(NULL), m(NULL) {}
    modelattach(const char *tag, vec *pos) : tag(tag), name(NULL), anim(-1), basetime(0), pos(pos), m(NULL) {}
};

extern void startmodelbatches();
extern void endmodelbatches();
extern void rendermodel(entitylight *light, const char *mdl, int anim, const vec &o, float yaw = 0, float pitch = 0, float roll = 0, int cull = MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED | MDL_LIGHT, dynent *d = NULL, modelattach *a = NULL, int basetime = 0, int basetime2 = 0, float trans = 1, float size = 1);
extern void abovemodel(vec &o, const char *mdl);
extern void rendershadow(dynent *d);
extern void setbbfrommodel(dynent *d, const char *mdl, float size = 1);

// ragdoll

extern bool validragdoll(dynent *d, int millis);
extern void moveragdoll(dynent *d, bool smooth);
extern void cleanragdoll(dynent *d);
extern vec ragdollcenter(dynent *d);

// server
#define MAXCLIENTS 256                  // in a multiplayer game, can be arbitrarily changed
#define MAXTRANS 5000                  // max amount of data to swallow in 1 go

extern int maxclients;

enum { DISC_NONE = 0, DISC_EOP, DISC_CN, DISC_KICK, DISC_TAGT, DISC_IPBAN, DISC_PRIVATE, DISC_MAXCLIENTS, DISC_TIMEOUT, DISC_OVERFLOW, DISC_NUM };

extern void *getinfo(int i);
extern const char *gethostname(int i);
extern void sendf(int cn, int chan, const char *format, ...);
extern void sendfile(int cn, int chan, stream *file, const char *format = "", ...);
extern void sendpacket(int cn, int chan, ENetPacket *packet, int exclude = -1);
extern int getnumclients();
extern uint getclientip(int n);
extern void putint(ucharbuf &p, int n);
extern void putint(packetbuf &p, int n);
extern void putint(vector<uchar> &p, int n);
extern int getint(ucharbuf &p);
extern void putuint(ucharbuf &p, int n);
extern void putuint(packetbuf &p, int n);
extern void putuint(vector<uchar> &p, int n);
extern int getuint(ucharbuf &p);
extern void putfloat(ucharbuf &p, float f);
extern void putfloat(packetbuf &p, float f);
extern void putfloat(vector<uchar> &p, float f);
extern float getfloat(ucharbuf &p);
extern void sendstring(const char *t, ucharbuf &p);
extern void sendstring(const char *t, packetbuf &p);
extern void sendstring(const char *t, vector<uchar> &p);
extern void getstring(char *t, ucharbuf &p, int len = MAXTRANS);
extern void filtertext(char *dst, const char *src, bool newline = true, bool colour = true, bool whitespace = true, int len = sizeof(string)-1);
extern const char *escapetext(const char *src, bool quoteonly = true);
extern void disconnect_client(int n, int reason);
extern void kicknonlocalclients(int reason);
extern void sendqueryreply(ucharbuf &p);
extern bool resolverwait(const char *name, ENetAddress *address);
extern int connectwithtimeout(ENetSocket sock, const char *hostname, ENetAddress &address);

extern bool findoctadir(const char *name, bool fallback = false);
extern void trytofindocta(bool fallback = true);
extern void setlocations(bool wanthome = true);

// client
struct serverinfo
{
    enum { WAITING = 999 };
    enum { UNRESOLVED = 0, RESOLVING, RESOLVED };

    string name;
    string map;
    string sdesc;
    int numplayers, ping, resolved, port;
    vector<int> attr;
    vector<char *> players;
    ENetAddress address;

    serverinfo(uint ip, int port)
     : numplayers(0), ping(WAITING), resolved(ip==ENET_HOST_ANY ? UNRESOLVED : RESOLVED), port(port)
    {
        name[0] = map[0] = sdesc[0] = '\0';
        address.host = ip;
        address.port = port+1;
        reset();
    }
    ~serverinfo() { reset(); }

    void reset() { loopvrev(players) { DELETEP(players[i]); players.remove(i); } players.setsize(0); }
};
extern vector<serverinfo *> servers;
extern void sendclientpacket(ENetPacket *packet, int chan);
extern void flushclient();
extern void disconnect(int onlyclean = 0, int async = 0);
extern bool multiplayer(bool msg = true);
extern void neterr(const char *s);
extern void gets2c();

// crypto
extern void genprivkey(const char *seed, vector<char> &privstr, vector<char> &pubstr);
extern bool hashstring(const char *str, char *result, int maxlen);
extern void answerchallenge(const char *privstr, const char *challenge, vector<char> &answerstr);
extern void *parsepubkey(const char *pubstr);
extern void freepubkey(void *pubkey);
extern void *genchallenge(void *pubkey, const void *seed, int seedlen, vector<char> &challengestr);
extern void freechallenge(void *answer);
extern bool checkchallenge(const char *answerstr, void *correct);

// gui
enum { GUI_DOWN = 1<<0, GUI_UP = 1<<1, GUI_ALT = 1<<2, GUI_PRESSED = 1<<3, GUI_ROLLOVER = 1<<4, GUI_DRAGGED = 1<<5 };
enum { EDITORREADONLY = 0, EDITORFOCUSED, EDITORUSED, EDITORFOREVER };

struct Texture;
struct guient
{
    virtual ~guient() {}

    virtual void start(int starttime, float basescale, int *tab = NULL, bool allowinput = true, bool wantstitle = true) = 0;
    virtual void end() = 0;

    virtual int text(const char *text, int color, const char *icon = NULL) = 0;
    int textf(const char *fmt, int color, const char *icon = NULL, ...)
    {
        defvformatstring(str, icon, fmt);
        return text(str, color, icon);
    }
    virtual int button(const char *text, int color, const char *icon = NULL, bool faded = true) = 0;
    int buttonf(const char *fmt, int color, const char *icon = NULL, bool faded = true, ...)
    {
        defvformatstring(str, faded, fmt);
        return button(str, color, icon, faded);
    }
    virtual void background(int color, int parentw = 0, int parenth = 0) = 0;

    virtual void pushlist(bool merge = false) {}
    virtual int poplist() { return 0; }

    virtual void allowautotab(bool on) = 0;
    virtual bool shouldtab() { return false; }
    virtual void tab(const char *name = NULL, int color = 0xFFFFFF) = 0;
    virtual int title(const char *text, int color = 0xFFFFFF, const char *icon = NULL) = 0;
    virtual int image(Texture *t, float scale, bool overlaid = false) = 0;
    virtual int texture(Texture *t, float scale, int rotate = 0, int xoff = 0, int yoff = 0, Texture *glowtex = NULL, const vec &glowcolor = vec(1, 1, 1), Texture *layertex = NULL) = 0;
    virtual int slice(Texture *t, float scale, float start = 0, float end = 1, const char *text = NULL) = 0;
    virtual void slider(int &val, int vmin, int vmax, int color, char *label = NULL, bool reverse = false, bool scroll = false) = 0;
    virtual void separator() = 0;
    virtual void progress(float percent, float scale) = 0;
    virtual void strut(float size = 1) = 0;
    virtual void pushfont(const char *font) = 0;
    virtual void popfont() = 0;
    virtual void space(float size = 1) = 0;
    virtual char *field(const char *name, int color, int length, int height = 0, const char *initval = NULL, int initmode = 0) = 0;
    virtual char *keyfield(const char *name, int color, int length, int height = 0, const char *initval = NULL, int initmode = 0) = 0;
    virtual void fieldline(const char *name, const char *str) = 0;
    virtual void fieldclear(const char *name, const char *init = "") = 0;
    virtual int fieldedit(const char *name) = 0;
    virtual void fieldscroll(const char *name, int n = -1) = 0;
};

struct guicb
{
    virtual ~guicb() {}
    int starttime() { extern int totalmillis; return totalmillis; }
    virtual void gui(guient &g, bool firstpass) = 0;
};

extern char *guioverlaytex, *guislidertex;

namespace UI
{
    extern bool isopen, ready;
    extern bool keypress(int code, bool isdown, int cooked);
    extern void setup();
    extern void update();
    extern void render();
    extern bool active(bool pass = true);
    extern bool hit(bool on, bool act);
    extern void addcb(guicb *cb);
    extern void limitscale(float scale);
}

// client
enum { ST_EMPTY, ST_LOCAL, ST_TCPIP, ST_REMOTE };

struct clientdata
{
    int type;
    int num;
    ENetPeer *peer;
    string hostname;
    void *info;
};

extern void process(ENetPacket *packet, int sender, int chan);
extern void send_welcome(int n);
extern void delclient(int n);
extern int addclient(int type = ST_EMPTY);
extern ENetHost *serverhost;

// world

extern bool inside;
extern physent *hitplayer;
extern int hitflags;
extern vec wall, hitsurface;
extern float walldistance;

enum { HITFLAG_NONE = 0, HITFLAG_LEGS = 1<<0, HITFLAG_TORSO = 1<<1, HITFLAG_HEAD = 1<<2, HITFLAG_FULL = 1<<4 };

enum
{
    MAP_BFGZ = 0,
    MAP_OCTA,
    MAP_MAX
};

enum
{
    MATF_VOLUME_SHIFT = 0,
    MATF_CLIP_SHIFT   = 3,
    MATF_FLAG_SHIFT   = 5,

    MATF_VOLUME = 7 << MATF_VOLUME_SHIFT,
    MATF_CLIP   = 3 << MATF_CLIP_SHIFT,
    MATF_FLAGS  = 7 << MATF_FLAG_SHIFT
};

enum // cube empty-space materials
{
    MAT_AIR   = 0,                      // the default, fill the empty space with air
    MAT_WATER = 1 << MATF_VOLUME_SHIFT, // fill with water, showing waves at the surface
    MAT_LAVA  = 2 << MATF_VOLUME_SHIFT, // fill with lava
    MAT_GLASS = 3 << MATF_VOLUME_SHIFT, // behaves like clip but is blended blueish

    MAT_NOCLIP = 1 << MATF_CLIP_SHIFT,  // collisions always treat cube as empty
    MAT_CLIP   = 2 << MATF_CLIP_SHIFT,  // collisions always treat cube as solid
    MAT_AICLIP = 3 << MATF_CLIP_SHIFT,  // clip monsters only

    MAT_DEATH  = 1 << MATF_FLAG_SHIFT,  // force player suicide
    MAT_LADDER = 2 << MATF_FLAG_SHIFT,  // acts as ladder (move up/down)
    MAT_EDIT   = 4 << MATF_FLAG_SHIFT   // edit-only surfaces
};
