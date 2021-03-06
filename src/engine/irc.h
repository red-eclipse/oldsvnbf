enum { IRCC_NONE = 0, IRCC_JOINING, IRCC_JOINED, IRCC_KICKED, IRCC_BANNED };
enum { IRCCT_NONE = 0, IRCCT_AUTO };
struct ircchan
{
    int state, type, relay, lastjoin;
    string name, friendly, passkey;
#ifndef STANDALONE
    vector<char *> lines;
#endif

    ircchan() { reset(); }
    ~ircchan() { reset(); }

    void reset()
    {
        state = IRCC_NONE;
        type = IRCCT_NONE;
        relay = lastjoin = 0;
        name[0] = friendly[0] = passkey[0] = 0;
#ifndef STANDALONE
        loopv(lines) DELETEA(lines[i]);
        lines.shrink(0);
#endif
    }
};
enum { IRCT_NONE = 0, IRCT_CLIENT, IRCT_RELAY, IRCT_MAX };
enum { IRC_DISC = 0, IRC_ATTEMPT, IRC_CONN, IRC_ONLINE, IRC_MAX };
struct ircnet
{
    int type, state, port, lastattempt;
    string name, serv, nick, ip, passkey, authname, authpass;
    ENetAddress address;
    ENetSocket sock;
    vector<ircchan> channels;
    vector<char *> lines;
    uchar input[4096];

    ircnet() { reset(); }
    ~ircnet() { reset(); }

    void reset()
    {
        type = IRCT_NONE;
        state = IRC_DISC;
        port = lastattempt = 0;
        name[0] = serv[0] = nick[0] = ip[0] = passkey[0] = authname[0] = authpass[0] = 0;
        channels.shrink(0);
        loopv(lines) DELETEA(lines[i]);
        lines.shrink(0);
    }
};

extern vector<ircnet *> ircnets;

extern ircnet *ircfind(const char *name);
extern void ircestablish(ircnet *n);
extern void ircsend(ircnet *n, const char *msg, ...);
extern void ircoutf(int relay, const char *msg, ...);
extern int ircrecv(ircnet *n, int timeout = 0);
extern char *ircread(ircnet *n);
extern void ircnewnet(int type, const char *name, const char *serv, int port, const char *nick, const char *ip = "", const char *passkey = "");
extern ircchan *ircfindchan(ircnet *n, const char *name);
extern bool ircjoin(ircnet *n, ircchan *c);
extern bool ircenterchan(ircnet *n, const char *name);
extern bool ircnewchan(int type, const char *name, const char *channel, const char *friendly = "", const char *passkey = "", int relay = 0);
extern void ircparse(ircnet *n, char *reply);
extern void ircdiscon(ircnet *n);
extern void irccleanup();
extern void ircslice();
