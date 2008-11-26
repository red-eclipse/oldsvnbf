// the interface the engine uses to run the gameplay module

namespace entities
{
	extern void editent(int i);
	extern void readent(gzFile &g, int mtype, int mver, char *gid, int gver, int id, entity &e);
	extern void writeent(gzFile &g, int id, entity &e);
	extern void initents(gzFile &g, int mtype, int mver, char *gid, int gver);
	extern float dropheight(entity &e);
	extern void fixentity(extentity &e);
	extern bool cansee(extentity &e);;
	extern const char *findname(int type);
	extern int findtype(char *type);
	extern bool maylink(int type, int ver = 0);
	extern bool canlink(int index, int node, bool msg = false);
	extern bool linkents(int index, int node, bool add, bool local, bool toggle);
	extern extentity *newent();
	extern vector<extentity *> &getents();
	extern void drawparticles();
}

namespace client
{
	extern void gamedisconnect(int clean);
	extern void parsepacketclient(int chan, ucharbuf &p);
	extern int sendpacketclient(ucharbuf &p, bool &reliable);
	extern void gameconnect(bool _remote);
	extern bool allowedittoggle(bool edit);
	extern void edittoggled(bool edit);
	extern void writeclientinfo(FILE *f);
	extern void toserver(int flags, char *text);
	extern bool sendcmd(int nargs, char *cmd, char *arg);
	extern void editvar(ident *id, bool local);
	extern void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0);
	extern void changemap(const char *name);
	extern bool ready();
	extern int state();
	extern int otherclients();
	extern int numchannels();
	extern int servercompare(serverinfo *a, serverinfo *b);
	extern int serverbrowser(g3d_gui *g);
}

namespace world
{
	extern bool clientoption(char *arg);
	extern void updateworld();
	extern void newmap(int size);
	extern void startmap(const char *name);
	extern void drawhud(int w, int h);
	extern void drawpointers(int w, int h);
	extern bool allowmove(physent *d);
	extern dynent *iterdynents(int i);
	extern int numdynents();
	extern void gamemenus();
	extern void lighteffects(dynent *d, vec &color, vec &dir);
	extern void adddynlights();
	extern void particletrack(particle *p, uint type, int &ts, vec &o, vec &d, bool lastpass);
	extern bool gethudcolour(vec &colour);
	extern vec headpos(physent *d, float off = 0.f);
	extern vec feetpos(physent *d, float off = 0.f);
	extern bool mousemove(int dx, int dy, int x, int y, int w, int h);
	extern void project(int w, int h);
	extern void recomputecamera(int w, int h);
	extern void loadworld(gzFile &f, int maptype);
	extern void saveworld(gzFile &f);
	extern int localplayers();
	extern bool gui3d();
	extern char *gametitle();
	extern char *gametext();
	extern int numanims();
	extern void findanims(const char *pattern, vector<int> &anims);
	extern void render();
	extern void renderavatar(bool early);
	extern bool isthirdperson();
	extern void start();
}

namespace server
{
	extern void srvmsgf(int cn, const char *s, ...);
	extern void srvoutf(int cn, const char *s, ...);
	extern bool serveroption(char *arg);
	extern void *newinfo();
	extern void deleteinfo(void *ci);
	extern int numclients(int exclude = -1, bool nospec = true, bool noai = false);
	extern void clientdisconnect(int n, bool local = false);
	extern int clientconnect(int n, uint ip, bool local = false);
	extern void recordpacket(int chan, void *data, int len);
	extern void parsepacket(int sender, int chan, bool reliable, ucharbuf &p);
	extern bool sendpackets();
	extern int welcomepacket(ucharbuf &p, int n, ENetPacket *packet);
	extern void queryreply(ucharbuf &req, ucharbuf &p);
	extern void serverupdate();
	extern void changemap(const char *s, int mode = -1, int muts = -1);
	extern const char *gameid();
	extern char *gamename(int mode, int muts);
	extern void modecheck(int *mode, int *muts);
	extern int gamever();
	extern const char *choosemap(const char *suggest);
	extern bool canload(char *type);
	extern void start();
}
