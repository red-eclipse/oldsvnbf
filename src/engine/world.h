
enum							// hardcoded texture numbers
{
	DEFAULT_SKY = 0,
	DEFAULT_LIQUID,
	DEFAULT_WALL,
	DEFAULT_FLOOR,
	DEFAULT_CEIL
};

#define OCTAVERSION 29			// diverged at ver 25
#define MAPVERSION 38			// bump if map format changes, see worldio.cpp

struct binary
{
	char head[4];
	int version, headersize;
};

#define OCTASTRLEN 260
struct octacompat25 : binary
{
	int worldsize;
	int numents;
	int waterlevel;
	int lightmaps;
	int lightprecision, lighterror, lightlod;
    uchar ambient;
    uchar watercolour[3];
    uchar blendmap;
    uchar lerpangle, lerpsubdiv, lerpsubdivsize;
    uchar bumperror;
    uchar skylight[3];
    uchar lavacolour[3];
    uchar waterfallcolour[3];
    uchar reserved[10];
	char maptitle[128];
};

struct octacompat28 : binary
{
    int worldsize;
    int numents;
    int numpvs;
    int lightmaps;
    int lightprecision, lighterror, lightlod;
    uchar ambient;
    uchar watercolour[3];
    uchar blendmap;
    uchar lerpangle, lerpsubdiv, lerpsubdivsize;
    uchar bumperror;
    uchar skylight[3];
    uchar lavacolour[3];
    uchar waterfallcolour[3];
    uchar reserved[10];
    char maptitle[128];
};

struct octa : binary
{
    int worldsize;
    int numents;
    int numpvs;
    int lightmaps;
    int blendmap;
    int numvars;
};

struct bfgz : binary
{
    int worldsize, numents, numpvs, lightmaps, blendmap;
    int gamever, revision;
    char gameid[4];
};

struct bfgzcompat33 : binary
{
    int worldsize, numents, numpvs, lightmaps, blendmap;
    int gamever, revision;
    char maptitle[128], gameid[4];
};

struct bfgzcompat32 : binary
{
    int worldsize, numents, numpvs, lightmaps;
    int gamever, revision;
    char maptitle[128], gameid[4];
};

struct bfgzcompat25 : binary
{
	int worldsize, numents, lightmaps;
	int gamever, revision;
	char maptitle[128], gameid[4];
};


struct entcompat
{
    vec o;
    short attr[5];
    uchar type;
    uchar reserved;
};

#define WATER_AMPLITUDE 0.8f
#define WATER_OFFSET 1.1f

enum
{
	MATSURF_NOT_VISIBLE = 0,
	MATSURF_VISIBLE,
	MATSURF_EDIT_ONLY
};

#define isliquid(mat) ((mat)==MAT_WATER || (mat)==MAT_LAVA)
#define isclipped(mat) ((mat)==MAT_GLASS)
#define isdeadly(mat) ((mat)==MAT_LAVA)

#define TEX_SCALE 8.0f

// VVEC_FRAC must be between 0..3
#define VVEC_FRAC 3
#define VVEC_INT (16-VVEC_FRAC)
#define VVEC_BITS (VVEC_INT + VVEC_FRAC)

#define VVEC_INT_MASK     ((1<<(VVEC_INT-1))-1)
#define VVEC_INT_COORD(n) (((n)&VVEC_INT_MASK)<<VVEC_FRAC)

struct vvec : svec
{
	vvec() {}
	vvec(ushort x, ushort y, ushort z) : svec(x, y, z) {}
	vvec(int x, int y, int z) : svec(VVEC_INT_COORD(x), VVEC_INT_COORD(y), VVEC_INT_COORD(z)) {}
	vvec(const int *i) : svec(VVEC_INT_COORD(i[0]), VVEC_INT_COORD(i[1]), VVEC_INT_COORD(i[2])) {}

	void mask(int f) { f <<= VVEC_FRAC; f |= (1<<VVEC_FRAC)-1; x &= f; y &= f; z &= f; }

	ivec toivec() const					{ return ivec(x, y, z).div(1<<VVEC_FRAC); }
	ivec toivec(int x, int y, int z) const { ivec t = toivec(); t.x += x&~VVEC_INT_MASK; t.y += y&~VVEC_INT_MASK; t.z += z&~VVEC_INT_MASK; return t; }
	ivec toivec(const ivec &o) const		{ return toivec(o.x, o.y, o.z); }

	vec tovec() const					{ return vec(x, y, z).div(1<<VVEC_FRAC); }
	vec tovec(int x, int y, int z) const { vec t = tovec(); t.x += x&~VVEC_INT_MASK; t.y += y&~VVEC_INT_MASK; t.z += z&~VVEC_INT_MASK; return t; }
	vec tovec(const ivec &o) const		{ return tovec(o.x, o.y, o.z); }
};

struct vertexffc : vvec { short reserved; };
struct fvertexffc : vec {};
struct vertexff : vertexffc { short u, v; };
struct fvertexff : fvertexffc { short u, v; };
struct vertex : vertexff { bvec n; };
struct fvertex : fvertexff { bvec n; };

extern int floatvtx;

#define VTXSIZE \
	(renderpath==R_FIXEDFUNCTION ? \
		(floatvtx ? (nolights ? sizeof(fvertexffc) : sizeof(fvertexff)) : (nolights ? sizeof(vertexffc) : sizeof(vertexff))) : \
		(floatvtx ? sizeof(fvertex) : sizeof(vertex)))

