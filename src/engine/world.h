
enum							// hardcoded texture numbers
{
	DEFAULT_SKY = 0,
	DEFAULT_LIQUID,
	DEFAULT_WALL,
	DEFAULT_FLOOR,
	DEFAULT_CEIL
};

#define MAPVERSION 24			// bump if map format changes, see worldio.cpp
#ifdef BFRONTIER

#endif

struct header					// map file format header
{
	char head[4];				// "OCTA"
	int version;				// any >8bit quantity is little endian
	int headersize;			 // sizeof(header)
	int worldsize;
	int numents;
#ifdef BFRONTIER
	int gamever, lightmaps, revision, reserved2, reserved3;
	uchar reserved[28];
#else
	int waterlevel;
	int lightmaps;
	int mapprec, maple, mapllod;
	uchar ambient; // 0
	uchar watercolour[3]; // 1 2 3 
	uchar mapwlod; // 4
	uchar lerpangle, lerpsubdiv, lerpsubdivsize; // 5 6 7
	uchar mapbe; // 8
	uchar skylight[3]; // 9 10 11
	uchar lavacolour[3]; // 12 13 14
	uchar reserved[1+12]; // 15...
#endif
	char maptitle[128];
};
#ifndef BFRONTIER // moved to iengine.h
enum							// cube empty-space materials
{
	MAT_AIR = 0,				// the default, fill the empty space with air
	MAT_WATER,				  // fill with water, showing waves at the surface
	MAT_CLIP,					// collisions always treat cube as solid
	MAT_GLASS,				  // behaves like clip but is blended blueish
	MAT_NOCLIP,				 // collisions always treat cube as empty
	MAT_LAVA,					// fill with lava
	MAT_EDIT					// basis for the edit volumes of the above materials
};
#endif
enum 
{ 
	MATSURF_NOT_VISIBLE = 0,
	MATSURF_VISIBLE,
	MATSURF_EDIT_ONLY
};

#define isliquid(mat) ((mat)==MAT_WATER || (mat)==MAT_LAVA)
#define isclipped(mat) ((mat) >= MAT_CLIP && (mat) < MAT_NOCLIP)

// VVEC_FRAC must be between 0..3
#ifdef BFRONTIER // smaller editing grid size deliciousness
#define VVEC_FRAC 3
#else
#define VVEC_FRAC 1
#endif
#define VVEC_INT (15-VVEC_FRAC)
#define VVEC_BITS (VVEC_INT + VVEC_FRAC)

#define VVEC_INT_MASK	 ((1<<(VVEC_INT-1))-1)
#define VVEC_INT_COORD(n) (((n)&VVEC_INT_MASK)<<VVEC_FRAC)

struct vvec : svec
{
	vvec() {}
	vvec(short x, short y, short z) : svec(x, y, z) {}
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

struct vertexffc : vvec {};
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

