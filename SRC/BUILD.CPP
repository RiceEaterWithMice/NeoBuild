// Build.cpp
//

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#if !defined(_WIN32)
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if !defined(_WIN32)
#define _stricmp strcasecmp
#endif

template <class A, class B>
static inline auto min(A a, B b) -> decltype((a < b) ? a : b) { return (a < b) ? a : b; }
template <class A, class B>
static inline auto max(A a, B b) -> decltype((a > b) ? a : b) { return (a > b) ? a : b; }

#ifndef MAXXDIM
#define MAXXDIM 2048
#endif
#ifndef MAXYDIM
#define MAXYDIM 1536
#endif
#ifndef MAXSECTORS
#define MAXSECTORS 1024
#endif
#ifndef MAXWALLS
#define MAXWALLS 8192
#endif
#ifndef MAXSPRITES
#define MAXSPRITES 4096
#endif
#ifndef MAXSTATUS
#define MAXSTATUS 1024
#endif
#ifndef MAXTILES
#define MAXTILES 4096
#endif
#ifndef MAXPALOOKUPS
#define MAXPALOOKUPS 256
#endif
#ifndef MAXSPRITESONSCREEN
#define MAXSPRITESONSCREEN 4096
#endif
#ifndef MAXPSKYTILES
#define MAXPSKYTILES 256
#endif

#ifndef MAXVOXELS
#define MAXVOXELS 512
#endif
#ifndef MAXVOXMIPS
#define MAXVOXMIPS 5
#endif

#define MAXCLIPNUM 512
#define MAXPERMS 512
#define MAXWALLSB 2048
#define MAXCLIPDIST 1024

static_assert(sizeof(long) == 4, "This classic BUILD drop-in assumes 32-bit long.");
#if INTPTR_MAX > INT32_MAX
#pragma message("Warning: classic BUILD drop-in mode is intended for Win32/x86. x64 requires changing old external waloff/frameplace pointer ABI.")
#endif


#include "BUILD.H"

static_assert(sizeof(sectortype) == 40, "sectortype must match BUILD v7 map layout");
static_assert(sizeof(walltype) == 32, "walltype must match BUILD v7 map layout");
static_assert(sizeof(spritetype) == 44, "spritetype must match BUILD v7 map layout");


#if INTPTR_MAX > INT32_MAX
#define BUILDGL_PTR_TO_LONG(p) static_cast<long>(reinterpret_cast<std::intptr_t>(p) & 0xffffffff)
#else
#define BUILDGL_PTR_TO_LONG(p) reinterpret_cast<long>(p)
#endif

// -----------------------------------------------------------------------------
// Public BUILD globals.
// -----------------------------------------------------------------------------
// These intentionally use normal C++ linkage to match the classic Duke/BUILD
// Visual Studio projects, whose .CPP files include BUILD.H without extern "C".
short numsectors = 0, numwalls = 0;
long numtiles = MAXTILES;
sectortype sector[MAXSECTORS] = {};
walltype wall[MAXWALLS] = {};
spritetype sprite[MAXSPRITES] = {};
spritetype tsprite[MAXSPRITESONSCREEN] = {};

short headspritesect[MAXSECTORS + 1] = {};
short prevspritesect[MAXSPRITES] = {};
short nextspritesect[MAXSPRITES] = {};
short headspritestat[MAXSTATUS + 1] = {};
short prevspritestat[MAXSPRITES] = {};
short nextspritestat[MAXSPRITES] = {};

long xdim = 640, ydim = 480, bytesperline = 640, imageSize = 640 * 480;
long windowx1 = 0, windowy1 = 0, windowx2 = 639, windowy2 = 479;
short startumost[MAXXDIM] = {}, startdmost[MAXXDIM] = {};
long ylookup[MAXYDIM + 1] = {};
long frameplace = 0, pageoffset = 0, qsetmode = 0;
char vidoption = 1;
long numpages = 1, maxpages = 1;
long activepage = 0, visualpage = 0, numframes = 0;
volatile long totalclock = 0;
long randomseed = 17;
long beforedrawrooms = 1, totalarea = 0, transarea = 0;
char automapping = 0, showinvisibility = 0;
long visibility = 512, parallaxvisibility = 512;
char parallaxtype = 2;
long parallaxyoffs = 0, parallaxyscale = 65536;
short pskyoff[MAXPSKYTILES] = {};
short pskybits = 0;

short tilesizx[MAXTILES] = {}, tilesizy[MAXTILES] = {};
long picanm[MAXTILES] = {};
long waloff[MAXTILES] = {};
char walock[MAXTILES] = {};
char gotpic[(MAXTILES + 7) >> 3] = {};
char picsiz[MAXTILES] = {};
char tilefilenum[MAXTILES] = {};
long tilefileoffs[MAXTILES] = {};
long artversion = 1, mapversion = 7, artsize = 0, cachesize = 0;

unsigned char palette[768] = {};
static unsigned char basepalette[768] = {};
static bool basepalettevalid = false;
char* palookup[MAXPALOOKUPS] = {};
short numpalookups = 32;
char* transluc = nullptr;
char paletteloaded = 0;
unsigned char britable[16][64] = {};
char textfont[1024] = {};
char smalltextfont[1024] = {};

short sintable[2048] = {};
short radarang[1280 * 2] = { };
char show2dsector[(MAXSECTORS + 7) >> 3] = {};
char show2dwall[(MAXWALLS + 7) >> 3] = {};
char show2dsprite[(MAXSPRITES + 7) >> 3] = {};
char gotsector[(MAXSECTORS + 7) >> 3] = {};
long spritesortcnt = 0;

short searchit = 0;
long searchx = -1, searchy = 0;
short searchsector = -1, searchwall = -1, searchstat = -1;

long globalposx = 0, globalposy = 0, globalposz = 0, globalhoriz = 100;
short globalang = 0, globalcursectnum = 0;
long cosglobalang = 0, singlobalang = 0;
long cosviewingrangeglobalang = 0, sinviewingrangeglobalang = 0;
long xyaspect = 65536, yxaspect = 65536, viewingrange = 65536, viewingrangerecip = 65536;
long xdimen = 640, ydimen = 480, halfxdimen = 320, xdimenrecip = 0, xdimenscale = 65536, xdimscale = 65536;
long globalpicnum = 0, globalpal = 0, globalshade = 0, globalorientation = 0;
long globalxpanning = 0, globalypanning = 0, globalyscale = 0, globalzd = 0, globalshiftval = 0;
long globvis = 0, globalvisibility = 512, globalhisibility = 512, globalpisibility = 512, globalcisibility = 512;
long wx1 = 0, wy1 = 0, wx2 = 0, wy2 = 0, viewoffset = 0;

long stereowidth = 23040, stereopixelwidth = 28, ostereopixelwidth = -1;
volatile long stereomode = 0, whiteband = 0, blackband = 0;
volatile char oa1 = 0, o3c2 = 0, ortca = 0, ortcb = 0, overtbits = 0, laststereoint = 0;
long voxoff[MAXVOXELS][MAXVOXMIPS] = {};
long voxlock[MAXVOXELS][MAXVOXMIPS] = {};

// Extra Duke3D/game-side globals that older modules often expect from the
// engine/platform layer.
char numenvsnds = 0;
char inputloc = 0;
char vgacompatible = 1;
char cachedebug = 0;


// Forward declarations for original engine entry points used before their bodies.
extern "C" {
    void loadtile(short tilenume);
    int animateoffs(short tilenum, short fakevar);
    void initspritelists();
    short insertsprite(short sectnum, short statnum);
    short insertspritesect(short sectnum);
    short insertspritestat(short statnum);
    int deletesprite(short spritenum);
    int deletespritesect(short deleteme);
    int deletespritestat(short deleteme);
    void updatesector(long x, long y, short* sectnum);
    int inside(long x, long y, short sectnum);
    void setview(long x1, long y1, long x2, long y2);
    void setaspect(long daxrange, long daaspect);
    void scansector(short sectnum);
    void drawalls(long bunch);
    void drawmasks();
    void nextpage();
    long getceilzofslope(short sectnum, long dax, long day);
    long getflorzofslope(short sectnum, long dax, long day);
    void getzsofslope(short sectnum, long dax, long day, long* ceilz, long* florz);
    int getangle(long xvect, long yvect);
    void dorotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2);
    int lintersect(long x1, long y1, long z1, long x2, long y2, long z2, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz);
    int rintersect(long x1, long y1, long z1, long vx, long vy, long vz, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz);
    int clipinsidebox(long x, long y, short wallnum, long walldist);
    void agecache();
}
void faketimerhandler();

// Internal renderer globals retained from the original names where useful.
static short umost[MAXXDIM] = {}, dmost[MAXXDIM] = {};
static short uplc[MAXXDIM] = {}, dplc[MAXXDIM] = {};
static short uwall[MAXXDIM] = {}, dwall[MAXXDIM] = {};
static long swall[MAXXDIM] = {}, lwall[MAXXDIM + 4] = {};
static short maskwall[MAXWALLSB] = {};
static long maskwallcnt = 0;
static spritetype* tspriteptr[MAXSPRITESONSCREEN] = {};
static long spritesx[MAXSPRITESONSCREEN] = {}, spritesy[MAXSPRITESONSCREEN + 1] = {}, spritesz[MAXSPRITESONSCREEN] = {};
static short sectorborder[256] = {};
static short sectorbordercnt = 0;
static linetype clipit[MAXCLIPNUM] = {};
static short clipobjectval[MAXCLIPNUM] = {};
static short clipsectorlist[MAXCLIPNUM] = {};
static short clipnum = 0, clipsectnum = 0;
static short hitwalls[4] = {};
long clipmoveboxtracenum = 3;
static long hitscangoalx = (1 << 29) - 1, hitscangoaly = (1 << 29) - 1;
static long curbrightness = 0;
static long lastageclock = 0;
static long capturecount = 0;
static bool g_mouseInit = false;
static POINT g_lastMousePoint = { 0, 0 };
static char artfilename[260] = "tiles000.art";
static long numtilefiles = 0;
static short artfil = -1;
static long inpreparemirror = 0, mirrorsx1 = 0, mirrorsx2 = 0, mirrorsy1 = 0, mirrorsy2 = 0;
static long pointhighlight = -1, linehighlight = -1, highlightcnt = 0, editstatus = 0;
static long permanentupdate = 0, buffermode = 0, origbuffermode = 0, validmodecnt = 0;

// -----------------------------------------------------------------------------
// Fixed-point and utility replacements for old pragmas/Watcom asm.
// -----------------------------------------------------------------------------
static inline long klabs(long v) { return v < 0 ? -v : v; }
static inline long ksgn(long v) { return (v > 0) - (v < 0); }

static inline long scale(long a, long b, long c)
{
    if (c == 0) return 0;
    return static_cast<long>((static_cast<std::int64_t>(a) * static_cast<std::int64_t>(b)) / c);
}
static inline long mulscale(long a, long b, long shift)
{
    return static_cast<long>((static_cast<std::int64_t>(a) * static_cast<std::int64_t>(b)) >> shift);
}
static inline long divscale(long a, long b, long shift)
{
    if (b == 0) return 0;
    return static_cast<long>((static_cast<std::int64_t>(a) << shift) / b);
}
#define DEF_MULSCALE(N) static inline long mulscale##N(long a, long b){ return static_cast<long>((static_cast<std::int64_t>(a)*static_cast<std::int64_t>(b)) >> (N)); }
#define DEF_DMULSCALE(N) static inline long dmulscale##N(long a,long b,long c,long d){ return static_cast<long>(((static_cast<std::int64_t>(a)*b)+(static_cast<std::int64_t>(c)*d)) >> (N)); }
#define DEF_DIVSCALE(N) static inline long divscale##N(long a,long b){ if(!b) return 0; return static_cast<long>((static_cast<std::int64_t>(a) << (N)) / b); }
DEF_MULSCALE(1) DEF_MULSCALE(2) DEF_MULSCALE(3) DEF_MULSCALE(4) DEF_MULSCALE(5) DEF_MULSCALE(6) DEF_MULSCALE(7) DEF_MULSCALE(8)
DEF_MULSCALE(9) DEF_MULSCALE(10) DEF_MULSCALE(11) DEF_MULSCALE(12) DEF_MULSCALE(13) DEF_MULSCALE(14) DEF_MULSCALE(15) DEF_MULSCALE(16)
DEF_MULSCALE(17) DEF_MULSCALE(18) DEF_MULSCALE(19) DEF_MULSCALE(20) DEF_MULSCALE(21) DEF_MULSCALE(22) DEF_MULSCALE(23) DEF_MULSCALE(24)
DEF_MULSCALE(25) DEF_MULSCALE(26) DEF_MULSCALE(27) DEF_MULSCALE(28) DEF_MULSCALE(29) DEF_MULSCALE(30) DEF_MULSCALE(31) DEF_MULSCALE(32)
DEF_DMULSCALE(2) DEF_DMULSCALE(3) DEF_DMULSCALE(4) DEF_DMULSCALE(5) DEF_DMULSCALE(6) DEF_DMULSCALE(8) DEF_DMULSCALE(9) DEF_DMULSCALE(10)
DEF_DMULSCALE(12) DEF_DMULSCALE(14) DEF_DMULSCALE(15) DEF_DMULSCALE(16) DEF_DMULSCALE(17) DEF_DMULSCALE(24) DEF_DMULSCALE(25) DEF_DMULSCALE(28) DEF_DMULSCALE(32)
DEF_DIVSCALE(12) DEF_DIVSCALE(14) DEF_DIVSCALE(15) DEF_DIVSCALE(16) DEF_DIVSCALE(18) DEF_DIVSCALE(19) DEF_DIVSCALE(20) DEF_DIVSCALE(24) DEF_DIVSCALE(28) DEF_DIVSCALE(30) DEF_DIVSCALE(32)
#undef DEF_MULSCALE
#undef DEF_DMULSCALE
#undef DEF_DIVSCALE

char pow2char[8] = { 1,2,4,8,16,32,64,128 };
long pow2long[32] =
{
    1L,2L,4L,8L,16L,32L,64L,128L,256L,512L,1024L,2048L,4096L,8192L,16384L,32768L,
    65536L,131072L,262144L,524288L,1048576L,2097152L,4194304L,8388608L,
    16777216L,33554432L,67108864L,134217728L,268435456L,536870912L,1073741824L,2147483647L
};

static void buildgl_clearbufbyte(void* dst, long count, long value)
{
    if (!dst || count <= 0) return;
    std::memset(dst, static_cast<unsigned char>(value & 255), static_cast<size_t>(count));
}
static void buildgl_clearbuf(void* dst, long count, long value)
{
    if (!dst || count <= 0) return;
    auto* p = static_cast<long*>(dst);
    for (long i = 0; i < count; ++i) p[i] = value;
}
static void buildgl_copybufbyte(const void* src, void* dst, long count)
{
    if (!src || !dst || count <= 0) return;
    std::memcpy(dst, src, static_cast<size_t>(count));
}
static void buildgl_copybuf(const void* src, void* dst, long count)
{
    if (!src || !dst || count <= 0) return;
    std::memcpy(dst, src, static_cast<size_t>(count) * sizeof(long));
}

extern "C" void clearbufbyte(long dst, long count, long value)
{
    buildgl_clearbufbyte(reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count, value);
}
extern "C" void clearbuf(long dst, long count, long value)
{
    buildgl_clearbuf(reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count, value);
}
extern "C" void copybufbyte(long src, long dst, long count)
{
    buildgl_copybufbyte(reinterpret_cast<const void*>(static_cast<std::intptr_t>(src)),
        reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count);
}
extern "C" void copybuf(long src, long dst, long count)
{
    buildgl_copybuf(reinterpret_cast<const void*>(static_cast<std::intptr_t>(src)),
        reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count);
}
static void swaplong(long* a, long* b) { long t = *a; *a = *b; *b = t; }
static void swapshort(short* a, short* b) { short t = *a; *a = *b; *b = t; }

extern "C" void* kmalloc(size_t size) { return std::malloc(size); }
extern "C" void kfree(void* buffer) { std::free(buffer); }
extern "C" void* kkmalloc(size_t size) { return std::malloc(size); }
extern "C" void kkfree(void* buffer) { std::free(buffer); }

// GRP-aware file table compatible with old kopen/kread style calls.
//
// BUILD GRP archives use this layout:
//   12 bytes: "KenSilverman"
//    4 bytes: little-endian number of files
//   repeated directory entries: 12-byte filename + 4-byte size
//   then each file payload concatenated in directory order.
//
// kopen4load() searches loose files first, then all mounted GRP files from newest
// to oldest. This makes local files override packaged assets while still letting
// palette.dat, tables.dat, ART files, and maps be loaded from a GRP.
namespace buildfile
{
    struct GroupEntry
    {
        std::string name;
        long offset = 0;
        long size = 0;
    };
    struct GroupFile
    {
        FILE* fp = nullptr;
        std::string path;
        std::vector<GroupEntry> entries;
    };
    struct KFile
    {
        FILE* fp = nullptr;
        bool fromGroup = false;
        long base = 0;
        long length = 0;
        long pos = 0;
    };

    static std::vector<GroupFile> g_groups;
    static KFile g_kfiles[256];

    static std::string basenameOnly(const char* filename)
    {
        if (!filename) return {};
        std::string s(filename);
        size_t p = s.find_last_of("/\\");
        if (p != std::string::npos) s = s.substr(p + 1);
        return s;
    }
    static std::string normalizeName(const char* filename)
    {
        std::string s = basenameOnly(filename);
        while (!s.empty() && (s.back() == '\0' || s.back() == ' ')) s.pop_back();
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    }
    static FILE* tryOpenDisk(const char* filename)
    {
        if (!filename || !filename[0]) return nullptr;
        FILE* f = std::fopen(filename, "rb");
        if (f) return f;

        std::string upper(filename);
        for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        f = std::fopen(upper.c_str(), "rb");
        if (f) return f;

        std::string lower(filename);
        for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return std::fopen(lower.c_str(), "rb");
    }


#if defined(_WIN32)
    static int osOpenReadPath(const char* path)
    {
        return _open(path, _O_RDONLY | _O_BINARY);
    }
    static int osOpenWritePath(const char* path)
    {
        return _open(path, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE);
    }
    static int osOpenAppendPath(const char* path)
    {
        return _open(path, _O_CREAT | _O_APPEND | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE);
    }
    static int osReadHandle(int fd, void* buf, long len)
    {
        return _read(fd, buf, static_cast<unsigned int>(len));
    }
    static int osWriteHandle(int fd, const void* buf, long len)
    {
        return _write(fd, buf, static_cast<unsigned int>(len));
    }
    static long osSeekHandle(int fd, long off, int whence)
    {
        return static_cast<long>(_lseek(fd, off, whence));
    }
    static int osCloseHandle(int fd)
    {
        return _close(fd);
    }
#else
    static int osOpenReadPath(const char* path)
    {
        return open(path, O_RDONLY);
    }
    static int osOpenWritePath(const char* path)
    {
        return open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    }
    static int osOpenAppendPath(const char* path)
    {
        return open(path, O_CREAT | O_APPEND | O_WRONLY, 0644);
    }
    static int osReadHandle(int fd, void* buf, long len)
    {
        return static_cast<int>(read(fd, buf, static_cast<size_t>(len)));
    }
    static int osWriteHandle(int fd, const void* buf, long len)
    {
        return static_cast<int>(write(fd, buf, static_cast<size_t>(len)));
    }
    static long osSeekHandle(int fd, long off, int whence)
    {
        return static_cast<long>(lseek(fd, off, whence));
    }
    static int osCloseHandle(int fd)
    {
        return close(fd);
    }
#endif

    static int tryOpenDiskReadHandle(const char* filename)
    {
        if (!filename || !filename[0]) return -1;

        int fd = osOpenReadPath(filename);
        if (fd >= 0) return fd;

        std::string upper(filename);
        for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        fd = osOpenReadPath(upper.c_str());
        if (fd >= 0) return fd;

        std::string lower(filename);
        for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return osOpenReadPath(lower.c_str());
    }

    static bool groupEntryExists(const char* filename)
    {
        const std::string want = normalizeName(filename);
        if (want.empty()) return false;

        for (auto git = g_groups.rbegin(); git != g_groups.rend(); ++git)
            for (const GroupEntry& e : git->entries)
                if (e.name == want) return true;
        return false;
    }

    static int createTempHandleForGroupEntry(GroupFile& grp, const GroupEntry& entry)
    {
#if defined(_WIN32)
        char tempDir[MAX_PATH + 1] = {};
        char tempPath[MAX_PATH + 1] = {};
        DWORD n = GetTempPathA(MAX_PATH, tempDir);
        if (n == 0 || n > MAX_PATH) std::strcpy(tempDir, ".\\");
        if (!GetTempFileNameA(tempDir, "bgl", 0, tempPath)) return -1;
        int fd = _open(tempPath, _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY | _O_TEMPORARY, _S_IREAD | _S_IWRITE);
#else
        char tempPath[] = "/tmp/buildgl_grp_XXXXXX";
        int fd = mkstemp(tempPath);
        if (fd >= 0) unlink(tempPath);
#endif
        if (fd < 0) return -1;

        std::fseek(grp.fp, entry.offset, SEEK_SET);
        long remaining = entry.size;
        unsigned char buffer[16384];
        while (remaining > 0)
        {
            const long want = min<long>(remaining, static_cast<long>(sizeof(buffer)));
            const long got = static_cast<long>(std::fread(buffer, 1, static_cast<size_t>(want), grp.fp));
            if (got <= 0)
            {
                osCloseHandle(fd);
                return -1;
            }
            long written = 0;
            while (written < got)
            {
                int did = osWriteHandle(fd, buffer + written, got - written);
                if (did <= 0)
                {
                    osCloseHandle(fd);
                    return -1;
                }
                written += did;
            }
            remaining -= got;
        }

        osSeekHandle(fd, 0, SEEK_SET);
        return fd;
    }

    static int openGroupAsDiskCompatibleHandle(const char* filename)
    {
        const std::string want = normalizeName(filename);
        if (want.empty()) return -1;

        for (auto git = g_groups.rbegin(); git != g_groups.rend(); ++git)
        {
            GroupFile& grp = *git;
            for (const GroupEntry& e : grp.entries)
                if (e.name == want)
                    return createTempHandleForGroupEntry(grp, e);
        }
        return -1;
    }
    static short allocHandle()
    {
        for (int i = 1; i < 256; ++i)
            if (!g_kfiles[i].fp) return static_cast<short>(i);
        return -1;
    }
    static short openDiskHandle(const char* filename)
    {
        FILE* f = tryOpenDisk(filename);
        if (!f) return -1;
        short h = allocHandle();
        if (h < 0) { std::fclose(f); return -1; }
        std::fseek(f, 0, SEEK_END);
        long len = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        g_kfiles[h] = KFile{};
        g_kfiles[h].fp = f;
        g_kfiles[h].length = len;
        return h;
    }
    static short openGroupHandle(const char* filename)
    {
        std::string want = normalizeName(filename);
        if (want.empty()) return -1;

        for (auto git = g_groups.rbegin(); git != g_groups.rend(); ++git)
        {
            GroupFile& grp = *git;
            for (const GroupEntry& e : grp.entries)
            {
                if (e.name != want) continue;
                short h = allocHandle();
                if (h < 0) return -1;
                g_kfiles[h] = KFile{};
                g_kfiles[h].fp = grp.fp;
                g_kfiles[h].fromGroup = true;
                g_kfiles[h].base = e.offset;
                g_kfiles[h].length = e.size;
                g_kfiles[h].pos = 0;
                return h;
            }
        }
        return -1;
    }
}

extern "C" long initgroupfile(char* filename)
{
    FILE* f = buildfile::tryOpenDisk(filename);
    if (!f) return -1;

    char magic[12] = {};
    long count = 0;
    if (std::fread(magic, 1, 12, f) != 12 || std::memcmp(magic, "KenSilverman", 12) != 0 ||
        std::fread(&count, 4, 1, f) != 1 || count < 0 || count > 65536)
    {
        std::fclose(f);
        return -1;
    }

    buildfile::GroupFile grp;
    grp.fp = f;
    grp.path = filename ? filename : "";
    grp.entries.reserve(static_cast<size_t>(count));

    long dataOffset = 16 + count * 16;
    for (long i = 0; i < count; ++i)
    {
        char rawname[13] = {};
        long size = 0;
        if (std::fread(rawname, 1, 12, f) != 12 || std::fread(&size, 4, 1, f) != 1 || size < 0)
        {
            std::fclose(f);
            return -1;
        }
        buildfile::GroupEntry e;
        e.name = buildfile::normalizeName(rawname);
        e.offset = dataOffset;
        e.size = size;
        dataOffset += size;
        grp.entries.push_back(e);
    }

    buildfile::g_groups.push_back(std::move(grp));
    return count;
}

extern "C" void uninitgroupfile()
{
    for (auto& kf : buildfile::g_kfiles)
    {
        if (kf.fp && !kf.fromGroup) std::fclose(kf.fp);
        kf = buildfile::KFile{};
    }
    for (auto& grp : buildfile::g_groups)
        if (grp.fp) std::fclose(grp.fp);
    buildfile::g_groups.clear();
}

extern "C" short kopen4load(char* filename, char searchfirst)
{
    // BUILD's searchfirst flag is used by loadboard() to force archive lookup
    // for names ending with 255. 0 = loose-file first, nonzero = GRP first.
    short h = -1;
    if (searchfirst == 0)
    {
        h = buildfile::openDiskHandle(filename);
        if (h >= 0) return h;
        return buildfile::openGroupHandle(filename);
    }
    h = buildfile::openGroupHandle(filename);
    if (h >= 0) return h;
    return buildfile::openDiskHandle(filename);
}

extern "C" short kopen4loadfrommod(char* filename, char searchfirst)
{
    return kopen4load(filename, searchfirst);
}

extern "C" long kread(short fil, void* buf, long len)
{
    if (fil <= 0 || fil >= 256 || !buildfile::g_kfiles[fil].fp || !buf || len < 0) return -1;
    buildfile::KFile& kf = buildfile::g_kfiles[fil];
    if (kf.fromGroup)
    {
        long avail = max(0L, kf.length - kf.pos);
        long todo = min(len, avail);
        std::fseek(kf.fp, kf.base + kf.pos, SEEK_SET);
        long got = static_cast<long>(std::fread(buf, 1, static_cast<size_t>(todo), kf.fp));
        kf.pos += got;
        return got;
    }
    long got = static_cast<long>(std::fread(buf, 1, static_cast<size_t>(len), kf.fp));
    kf.pos = std::ftell(kf.fp);
    return got;
}

extern "C" long uncompress(char* lzwinbuf, long compleng, char* lzwoutbuf);
extern "C" long compress(char* lzwinbuf, long uncompleng, char* lzwoutbuf);

namespace buildlzw
{
    static constexpr long LZWSIZE = 16384;
    static constexpr long LZWBUFSIZE = LZWSIZE + (LZWSIZE >> 4) + 16;
    static std::vector<unsigned char> lzwbuf1;
    static std::vector<short> lzwbuf2;
    static std::vector<short> lzwbuf3;
    static std::vector<unsigned char> lzwbuf4;
    static std::vector<unsigned char> lzwbuf5;

    static void ensure()
    {
        if (lzwbuf1.empty()) lzwbuf1.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf2.empty()) lzwbuf2.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf3.empty()) lzwbuf3.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf4.empty()) lzwbuf4.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf5.empty()) lzwbuf5.assign(static_cast<size_t>(LZWBUFSIZE), 0);
    }
    static unsigned short read16le(const void* p)
    {
        const auto* b = static_cast<const unsigned char*>(p);
        return static_cast<unsigned short>(b[0] | (b[1] << 8));
    }
    static void write16le(void* p, long v)
    {
        auto* b = static_cast<unsigned char*>(p);
        b[0] = static_cast<unsigned char>(v & 255);
        b[1] = static_cast<unsigned char>((v >> 8) & 255);
    }
    static long getbits(const unsigned char* in, long compleng, long bitcnt, long numbits)
    {
        unsigned long v = 0;
        for (long b = 0; b < numbits; ++b)
        {
            long bit = bitcnt + b;
            long byteIndex = bit >> 3;
            if (byteIndex >= compleng) break;
            if (in[byteIndex] & (1u << (bit & 7))) v |= (1UL << b);
        }
        return static_cast<long>(v);
    }
    static bool readBlockK(short fil, long& outLen)
    {
        unsigned char lenbuf[2] = {};
        if (kread(fil, lenbuf, 2) != 2) return false;
        long clen = static_cast<long>(read16le(lenbuf));
        if (clen < 0 || clen > LZWBUFSIZE) return false;
        if (kread(fil, lzwbuf5.data(), clen) != clen) return false;
        outLen = ::uncompress(reinterpret_cast<char*>(lzwbuf5.data()), clen, reinterpret_cast<char*>(lzwbuf4.data()));
        if (outLen < 0 || outLen > LZWSIZE) return false;
        return true;
    }
    static bool readBlockFile(FILE* fil, long& outLen)
    {
        unsigned char lenbuf[2] = {};
        if (!fil || std::fread(lenbuf, 1, 2, fil) != 2) return false;
        long clen = static_cast<long>(read16le(lenbuf));
        if (clen < 0 || clen > LZWBUFSIZE) return false;
        if (std::fread(lzwbuf5.data(), 1, static_cast<size_t>(clen), fil) != static_cast<size_t>(clen)) return false;
        outLen = ::uncompress(reinterpret_cast<char*>(lzwbuf5.data()), clen, reinterpret_cast<char*>(lzwbuf4.data()));
        if (outLen < 0 || outLen > LZWSIZE) return false;
        return true;
    }
}

extern "C" long uncompress(char* lzwinbuf, long compleng, char* lzwoutbuf)
{
    buildlzw::ensure();
    if (!lzwinbuf || !lzwoutbuf || compleng < 4) return 0;

    const auto* in = reinterpret_cast<const unsigned char*>(lzwinbuf);
    auto* out = reinterpret_cast<unsigned char*>(lzwoutbuf);
    long uncompleng = static_cast<long>(buildlzw::read16le(in));
    long strtot = static_cast<long>(buildlzw::read16le(in + 2));
    if (uncompleng < 0 || uncompleng > buildlzw::LZWSIZE) return 0;

    if (strtot == 0)
    {
        long n = min(uncompleng, max(0L, compleng - 4));
        if (n > 0) std::memcpy(out, in + 4, static_cast<size_t>(n));
        if (n < uncompleng) std::memset(out + n, 0, static_cast<size_t>(uncompleng - n));
        return uncompleng;
    }

    for (long i = 255; i >= 0; --i)
    {
        buildlzw::lzwbuf2[static_cast<size_t>(i)] = static_cast<short>(i);
        buildlzw::lzwbuf3[static_cast<size_t>(i)] = static_cast<short>(i);
    }

    long currstr = 256;
    long bitcnt = 32;
    long outbytecnt = 0;
    long numbits = 8;
    long oneupnumbits = 256;

    if (strtot > buildlzw::LZWBUFSIZE) strtot = buildlzw::LZWBUFSIZE;
    while (currstr < strtot && outbytecnt < uncompleng)
    {
        long dat = buildlzw::getbits(in, compleng, bitcnt, numbits) & (oneupnumbits - 1);
        bitcnt += numbits;
        if ((dat & ((oneupnumbits >> 1) - 1)) > ((currstr - 1) & ((oneupnumbits >> 1) - 1)))
        {
            dat &= ((oneupnumbits >> 1) - 1);
            bitcnt--;
        }

        buildlzw::lzwbuf3[static_cast<size_t>(currstr)] = static_cast<short>(dat);

        long leng = 0;
        long walk = dat;
        while (walk >= 256 && walk < currstr && leng < buildlzw::LZWBUFSIZE)
        {
            buildlzw::lzwbuf1[static_cast<size_t>(leng++)] = static_cast<unsigned char>(buildlzw::lzwbuf2[static_cast<size_t>(walk)] & 255);
            walk = buildlzw::lzwbuf3[static_cast<size_t>(walk)];
        }
        if (walk < 0 || walk > 255) walk = 0;

        out[outbytecnt++] = static_cast<unsigned char>(walk);
        for (long i = leng - 1; i >= 0 && outbytecnt < uncompleng; --i)
            out[outbytecnt++] = buildlzw::lzwbuf1[static_cast<size_t>(i)];

        buildlzw::lzwbuf2[static_cast<size_t>(currstr - 1)] = static_cast<short>(walk);
        buildlzw::lzwbuf2[static_cast<size_t>(currstr)] = static_cast<short>(walk);
        currstr++;
        if (currstr > oneupnumbits)
        {
            numbits++;
            oneupnumbits <<= 1;
            if (numbits > 15) numbits = 15;
        }
    }

    if (outbytecnt < uncompleng) std::memset(out + outbytecnt, 0, static_cast<size_t>(uncompleng - outbytecnt));
    return uncompleng;
}

extern "C" long compress(char* lzwinbuf, long uncompleng, char* lzwoutbuf)
{
    // Valid BUILD LZW block in the uncompressed fallback form. This is smaller-code,
    // deterministic, and remains readable by the original uncompress() path.
    if (!lzwinbuf || !lzwoutbuf || uncompleng < 0) return 0;
    if (uncompleng > buildlzw::LZWSIZE) uncompleng = buildlzw::LZWSIZE;
    buildlzw::write16le(lzwoutbuf, uncompleng);
    buildlzw::write16le(lzwoutbuf + 2, 0);
    if (uncompleng > 0) std::memcpy(lzwoutbuf + 4, lzwinbuf, static_cast<size_t>(uncompleng));
    return uncompleng + 4;
}

extern "C" long kdfread(void* buffer, long dasizeof, long count, short fil)
{
    buildlzw::ensure();
    if (!buffer || dasizeof <= 0 || count <= 0) return 0;
    if (dasizeof > buildlzw::LZWSIZE) { count *= dasizeof; dasizeof = 1; }

    auto* ptr = static_cast<unsigned char*>(buffer);
    long k = 0, kgoal = 0;
    if (!buildlzw::readBlockK(fil, kgoal)) return -1;
    if (dasizeof > kgoal) return -1;

    std::memcpy(ptr, buildlzw::lzwbuf4.data(), static_cast<size_t>(dasizeof));
    k += dasizeof;

    for (long i = 1; i < count; ++i)
    {
        if (k >= kgoal)
        {
            if (!buildlzw::readBlockK(fil, kgoal)) return -1;
            k = 0;
        }
        for (long j = 0; j < dasizeof; ++j)
        {
            unsigned char delta = (k + j < kgoal) ? buildlzw::lzwbuf4[static_cast<size_t>(k + j)] : 0;
            ptr[j + dasizeof] = static_cast<unsigned char>((ptr[j] + delta) & 255);
        }
        k += dasizeof;
        ptr += dasizeof;
    }
    return count;
}

extern "C" long dfread(void* buffer, long dasizeof, long count, FILE* fil)
{
    buildlzw::ensure();
    if (!buffer || !fil || dasizeof <= 0 || count <= 0) return 0;
    if (dasizeof > buildlzw::LZWSIZE) { count *= dasizeof; dasizeof = 1; }

    auto* ptr = static_cast<unsigned char*>(buffer);
    long k = 0, kgoal = 0;
    if (!buildlzw::readBlockFile(fil, kgoal)) return -1;
    if (dasizeof > kgoal) return -1;

    std::memcpy(ptr, buildlzw::lzwbuf4.data(), static_cast<size_t>(dasizeof));
    k += dasizeof;

    for (long i = 1; i < count; ++i)
    {
        if (k >= kgoal)
        {
            if (!buildlzw::readBlockFile(fil, kgoal)) return -1;
            k = 0;
        }
        for (long j = 0; j < dasizeof; ++j)
        {
            unsigned char delta = (k + j < kgoal) ? buildlzw::lzwbuf4[static_cast<size_t>(k + j)] : 0;
            ptr[j + dasizeof] = static_cast<unsigned char>((ptr[j] + delta) & 255);
        }
        k += dasizeof;
        ptr += dasizeof;
    }
    return count;
}

extern "C" long dfwrite(void* buffer, long dasizeof, long count, FILE* fil)
{
    buildlzw::ensure();
    if (!buffer || !fil || dasizeof <= 0 || count <= 0) return 0;
    if (dasizeof > buildlzw::LZWSIZE) { count *= dasizeof; dasizeof = 1; }

    auto* ptr = static_cast<unsigned char*>(buffer);
    long k = 0;
    std::memcpy(buildlzw::lzwbuf4.data(), ptr, static_cast<size_t>(dasizeof));
    k = dasizeof;

    auto flushBlock = [&](long bytes) -> bool
        {
            long clen = ::compress(reinterpret_cast<char*>(buildlzw::lzwbuf4.data()), bytes, reinterpret_cast<char*>(buildlzw::lzwbuf5.data()));
            unsigned char lenbuf[2];
            buildlzw::write16le(lenbuf, clen);
            return std::fwrite(lenbuf, 1, 2, fil) == 2 &&
                std::fwrite(buildlzw::lzwbuf5.data(), 1, static_cast<size_t>(clen), fil) == static_cast<size_t>(clen);
        };

    if (k > buildlzw::LZWSIZE - dasizeof)
    {
        if (!flushBlock(k)) return -1;
        k = 0;
    }

    for (long i = 1; i < count; ++i)
    {
        for (long j = 0; j < dasizeof; ++j)
            buildlzw::lzwbuf4[static_cast<size_t>(k + j)] = static_cast<unsigned char>((ptr[j + dasizeof] - ptr[j]) & 255);
        k += dasizeof;
        if (k > buildlzw::LZWSIZE - dasizeof)
        {
            if (!flushBlock(k)) return -1;
            k = 0;
        }
        ptr += dasizeof;
    }

    if (k > 0 && !flushBlock(k)) return -1;
    return count;
}


extern "C" void kclose(short fil)
{
    if (fil > 0 && fil < 256 && buildfile::g_kfiles[fil].fp)
    {
        if (!buildfile::g_kfiles[fil].fromGroup) std::fclose(buildfile::g_kfiles[fil].fp);
        buildfile::g_kfiles[fil] = buildfile::KFile{};
    }
}

extern "C" long klseek(short fil, long offset, long whence)
{
    if (fil <= 0 || fil >= 256 || !buildfile::g_kfiles[fil].fp) return -1;
    buildfile::KFile& kf = buildfile::g_kfiles[fil];
    if (kf.fromGroup)
    {
        long newpos = 0;
        if (whence == SEEK_SET) newpos = offset;
        else if (whence == SEEK_CUR) newpos = kf.pos + offset;
        else if (whence == SEEK_END) newpos = kf.length + offset;
        else return -1;
        kf.pos = min(max(newpos, 0L), kf.length);
        return kf.pos;
    }
    if (std::fseek(kf.fp, offset, whence) != 0) return -1;
    kf.pos = std::ftell(kf.fp);
    return kf.pos;
}

extern "C" long kfilelength(short fil)
{
    if (fil <= 0 || fil >= 256 || !buildfile::g_kfiles[fil].fp) return -1;
    buildfile::KFile& kf = buildfile::g_kfiles[fil];
    if (kf.fromGroup) return kf.length;
    long cur = std::ftell(kf.fp);
    std::fseek(kf.fp, 0, SEEK_END);
    long len = std::ftell(kf.fp);
    std::fseek(kf.fp, cur, SEEK_SET);
    return len;
}

extern "C" long ksqrt(long num)
{
    if (num <= 0) return 0;
    return static_cast<long>(std::sqrt(static_cast<double>(num)));
}
extern "C" long krecip(long num)
{
    if (num == 0) return 0x7fffffff;
    return divscale30(1, num);
}

LRESULT CALLBACK Engine_KeyboardWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// -----------------------------------------------------------------------------
// OpenGL 1.x renderer backend.
// -----------------------------------------------------------------------------
namespace buildgl
{
    static constexpr double PI = 3.1415926535897932384626433832795;

#ifndef BUILDGL_XY_SCALE_DIVISOR
#define BUILDGL_XY_SCALE_DIVISOR 1024.0f
#endif
#ifndef BUILDGL_Z_SCALE_DIVISOR
    // BUILD stores Z with finer granularity than X/Y. The previous 8192 divisor
    // made vertical distances about 2x too tall compared with sprites and classic
    // Duke3D room proportions. 16384 keeps a 64px, yrepeat-64 sprite near the same
    // GL height as its BUILD Z extent (64*64*4).
#define BUILDGL_Z_SCALE_DIVISOR 16384.0f
#endif
#ifndef BUILDGL_CLOSE_MAP_UNITS
// Keep the renderer close plane in BUILD map units, then convert it to GL
// units for gluPerspective(). A fixed GL near value of 0.05 clips at about
// 51 BUILD XY units when BUILDGL_XY_SCALE_DIVISOR is 1024.
#define BUILDGL_CLOSE_MAP_UNITS 4.0
#endif
#ifndef BUILDGL_FAR_PLANE
#define BUILDGL_FAR_PLANE 10000.0
#endif
#ifndef BUILDGL_HORIZONTAL_FOV_DEGREES
// gluPerspective() wants vertical FOV. Classic BUILD/Duke-style tuning is much
// closer to a ~90 degree horizontal FOV; using 90 as vertical FOV made the view
// show too much ceiling/floor and exaggerated Z.
#define BUILDGL_HORIZONTAL_FOV_DEGREES 90.0
#endif
#ifndef BUILDGL_VERTICAL_FOV_DEGREES
// Set this > 1.0 to override the horizontal-FOV-derived value directly.
#define BUILDGL_VERTICAL_FOV_DEGREES 0.0
#endif
#ifndef BUILDGL_SKY_SCREENSPACE
// Classic BUILD draws parallax skies as a screen-space angular backdrop.
// Keep this enabled unless you explicitly want the old GL cylinder fallback.
#define BUILDGL_SKY_SCREENSPACE 1
#endif
#ifndef BUILDGL_SKY_VERTICAL_PIXELS
// BUILD's original view is 320x200. Use virtual 200px vertical sky math so
// changing the Win32/OpenGL resolution does not stretch the sky texture.
#define BUILDGL_SKY_VERTICAL_PIXELS 200.0f
#endif
#ifndef BUILDGL_FP_OFF_WIDTH
// Duke3D's classic software frame buffer is 320x200.  Override these at
// compile time if your fork really uses a 320x220 compatibility surface.
#define BUILDGL_FP_OFF_WIDTH 320
#endif
#ifndef BUILDGL_FP_OFF_HEIGHT
#define BUILDGL_FP_OFF_HEIGHT 200
#endif
#ifndef BUILDGL_FP_OFF_POT_BACKING
// Keep the visible FP_OFF surface fixed at BUILDGL_FP_OFF_WIDTH x
// BUILDGL_FP_OFF_HEIGHT.  Set this to 1 only for ancient GL drivers that reject
// non-power-of-two GL_TEXTURE_2D uploads; texture coordinates will still crop
// back to the fixed virtual surface.
#define BUILDGL_FP_OFF_POT_BACKING 0
#endif

    static constexpr float XY_SCALE = 1.0f / BUILDGL_XY_SCALE_DIVISOR;
    static constexpr int FP_OFF_WIDTH = (BUILDGL_FP_OFF_WIDTH > 0) ? BUILDGL_FP_OFF_WIDTH : 320;
    static constexpr int FP_OFF_HEIGHT = (BUILDGL_FP_OFF_HEIGHT > 0) ? BUILDGL_FP_OFF_HEIGHT : 200;
    static constexpr float Z_SCALE = 1.0f / BUILDGL_Z_SCALE_DIVISOR;

    struct Vec2 { double x, y; };
    struct Vec3 { float x, y, z; };

    struct GLState
    {
        HWND hwnd = nullptr;
        HDC hdc = nullptr;
        HGLRC hglrc = nullptr;
        int w = 640, h = 480;
        bool ready = false;
        bool quit = false;
        bool frameActive = false;
        bool opaqueDrawn = false;
        bool maskedDrawn = false;
        // Textures are cached per BUILD lookup palette. A tile's source ART
        // data is still loaded once, but palette remaps such as access-card
        // colors, enemy variants, sector tints, and HUD palettes need distinct
        // GL textures because this fixed-function path uploads true RGBA.
        std::array<std::array<GLuint, MAXPALOOKUPS>, MAXTILES> tex{};
        std::array<std::array<unsigned char, MAXPALOOKUPS>, MAXTILES> texReady{};
        std::array<std::array<unsigned char, MAXPALOOKUPS>, MAXTILES> texDirty{};
        std::array<unsigned char, MAXTILES> fallbackTile{};
        std::vector<unsigned char> tileData[MAXTILES];

        // Compatibility path for old code that asks for Duke3D's classic
        // 320x200 linear 8-bit framebuffer through FP_OFF().  When FP_OFF() is
        // called, that fixed virtual surface is considered dirty for this frame,
        // converted through the current BUILD palette, uploaded, and drawn as
        // the last full-screen pass immediately before SwapBuffers().
        std::vector<unsigned char> fpOffBuffer;
        std::vector<unsigned char> fpOffRgba;
        unsigned char* fpOffSource = nullptr;
        GLuint fpOffTex = 0;
        bool fpOffTexReady = false;
        bool fpOffDirtyThisFrame = false;
        int fpOffTexW = 0;
        int fpOffTexH = 0;
    } g;

    static void syncClientSize(int width, int height, bool resetView)
    {
        g.w = max(1, width);
        g.h = max(1, height);

        xdim = g.w;
        ydim = g.h;
        bytesperline = xdim;
        imageSize = xdim * ydim;

        for (long y = 0, p = 0; y <= ydim && y <= MAXYDIM; ++y, p += bytesperline)
            ylookup[y] = p;

        if (resetView)
            ::setview(0, 0, xdim - 1, ydim - 1);
    }

    static Vec3 buildToGl(long x, long y, long z)
    {
        return Vec3{ static_cast<float>(x) * XY_SCALE, static_cast<float>(-z) * Z_SCALE, static_cast<float>(y) * XY_SCALE };
    }
    static double radFromBuildAngle(short ang)
    {
        return (static_cast<double>(ang & 2047) * 2.0 * PI) / 2048.0;
    }
    static float shadeFactor(long shade)
    {
        float f = 1.0f - static_cast<float>(shade) / 64.0f;
        if (f < 0.18f) f = 0.18f;
        if (f > 1.25f) f = 1.25f;
        return f;
    }
    static void glColorShade(long shade, float alpha = 1.0f)
    {
        const float f = shadeFactor(shade);
        glColor4f(f, f, f, alpha);
    }

    static double clampDouble(double v, double lo, double hi)
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    static double computeVerticalFovDegrees(double aspect)
    {
        aspect = clampDouble(aspect, 0.10, 10.0);

        const double directVerticalFov = static_cast<double>(BUILDGL_VERTICAL_FOV_DEGREES);
        if (directVerticalFov > 1.0)
            return clampDouble(directVerticalFov, 30.0, 120.0);

        const double horizontalFov = clampDouble(static_cast<double>(BUILDGL_HORIZONTAL_FOV_DEGREES), 45.0, 140.0) * PI / 180.0;
        const double verticalFov = 2.0 * std::atan(std::tan(horizontalFov * 0.5) / aspect);
        return clampDouble(verticalFov * 180.0 / PI, 30.0, 120.0);
    }

    static double horizToPitchSlope(long horiz, double verticalFovDegrees)
    {
        // globalhoriz is centered on 100 in the classic 320x200 view. Convert it
        // to a view slope using the actual vertical FOV instead of assuming a
        // 90-degree vertical lens.
        const double halfFovSlope = std::tan((verticalFovDegrees * PI / 180.0) * 0.5);
        return (100.0 - static_cast<double>(horiz)) * (halfFovSlope / 100.0);
    }

    static float clampFloat(float v, float lo, float hi)
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    static signed char signedPicanmByte(short pic, int shift)
    {
        if ((unsigned short)pic >= MAXTILES) return 0;
        return static_cast<signed char>((picanm[pic] >> shift) & 255);
    }

#ifndef BUILDGL_FOG_DENSITY_DIVISOR
    // The renderer scales BUILD X/Y map units down by 1024 before OpenGL sees them.
    // Fog therefore needs a much stronger world-space density than the previous
    // 262144 divisor. 65536 makes sector visibility changes visible at normal
    // Duke-sized room distances without instantly blanketing default sectors.
#define BUILDGL_FOG_DENSITY_DIVISOR 65536.0f
#endif
#ifndef BUILDGL_FOG_STRENGTH
// Extra user-facing multiplier for quick tuning. Raise to 1.5f/2.0f if you
// want heavier fog globally, or lower it if stock maps become too hazy.
#define BUILDGL_FOG_STRENGTH 1.0f
#endif
#ifndef BUILDGL_FOG_MAX_DENSITY
#define BUILDGL_FOG_MAX_DENSITY 0.45f
#endif
#ifndef BUILDGL_FOG_MODE
// GL_EXP gives visible room-scale falloff; GL_EXP2 was too weak up close with
// this port's scaled coordinates.
#define BUILDGL_FOG_MODE GL_EXP
#endif

    static long combinedVisibilityForSector(short sectnum)
    {
        // sector[].visibility is an 8-bit BUILD visibility modifier, not an
        // absolute fog-density override. A sector value of 0 means normal
        // visibility because the classic factor is (visibility + 16).
        const long baseVisibility = max(0L, visibility);
        unsigned char sectorFactor = 16;

        if (sectnum >= 0 && sectnum < numsectors)
        {
            const int sectorVis = static_cast<signed char>(sector[sectnum].visibility);
            sectorFactor = static_cast<unsigned char>(sectorVis + 16);
        }

        return baseVisibility * static_cast<long>(sectorFactor);
    }

    static float fogDensityForSector(short sectnum)
    {
        const long combinedVisibility = combinedVisibilityForSector(sectnum);
        if (combinedVisibility <= 0) return 0.0f;

        // BUILD map coordinates are scaled down before reaching OpenGL, so the
        // old palette-lookup visibility value must be converted to GL-world
        // density. This default keeps normal Duke visibility visible at room
        // distance while letting sector visibility values noticeably thicken or
        // clear the fog.
        const float density = (static_cast<float>(combinedVisibility) / BUILDGL_FOG_DENSITY_DIVISOR) *
            BUILDGL_FOG_STRENGTH;
        return clampFloat(density, 0.0f, BUILDGL_FOG_MAX_DENSITY);
    }

    static void applySectorFog(short sectnum)
    {
        const float density = fogDensityForSector(sectnum);
        if (density <= 0.0f)
        {
            glDisable(GL_FOG);
            return;
        }

        const GLfloat fogColor[4] =
        {
            static_cast<GLfloat>((static_cast<unsigned char>(palette[0]) << 2) / 255.0f),
            static_cast<GLfloat>((static_cast<unsigned char>(palette[1]) << 2) / 255.0f),
            static_cast<GLfloat>((static_cast<unsigned char>(palette[2]) << 2) / 255.0f),
            1.0f
        };
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, BUILDGL_FOG_MODE);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_DENSITY, density);
        glHint(GL_FOG_HINT, GL_DONT_CARE);
    }

#ifndef BUILDGL_AUTOMAP_VIRTUAL_WIDTH
#define BUILDGL_AUTOMAP_VIRTUAL_WIDTH 320.0
#endif
#ifndef BUILDGL_AUTOMAP_VIRTUAL_HEIGHT
#define BUILDGL_AUTOMAP_VIRTUAL_HEIGHT 200.0
#endif

    static long scaleAutomapDeltaX(long x)
    {
        return static_cast<long>(std::lround(
            static_cast<double>(x) * static_cast<double>(max(1, g.w)) / BUILDGL_AUTOMAP_VIRTUAL_WIDTH));
    }

    static long scaleAutomapDeltaY(long y)
    {
        return static_cast<long>(std::lround(
            static_cast<double>(y) * static_cast<double>(max(1, g.h)) / BUILDGL_AUTOMAP_VIRTUAL_HEIGHT));
    }



    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        switch (msg)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_CHAR:
        case WM_ACTIVATEAPP:
        case WM_KILLFOCUS:
            return Engine_KeyboardWndProc(hwnd, msg, wp, lp);

        case WM_CLOSE: g.quit = true; DestroyWindow(hwnd); return 0;
        case WM_DESTROY: g.quit = true; PostQuitMessage(0); return 0;
        case WM_SIZE:
            syncClientSize(LOWORD(lp), HIWORD(lp), true);
            return 0;
        default: return DefWindowProc(hwnd, msg, wp, lp);
        }
    }

    static bool createWindow(int width, int height, const char* title)
    {
        if (g.ready)
        {
            if (g.hwnd)
            {
                DWORD style = static_cast<DWORD>(GetWindowLongA(g.hwnd, GWL_STYLE));
                RECT wr{ 0, 0, width, height };
                AdjustWindowRect(&wr, style, FALSE);
                SetWindowPos(g.hwnd, nullptr, 0, 0, wr.right - wr.left, wr.bottom - wr.top, SWP_NOMOVE | SWP_NOZORDER);

                RECT cr{};
                GetClientRect(g.hwnd, &cr);
                syncClientSize(cr.right - cr.left, cr.bottom - cr.top, true);
            }
            else
            {
                syncClientSize(width, height, true);
            }
            return true;
        }

        HINSTANCE inst = GetModuleHandle(nullptr);
        WNDCLASSA wc{};
        wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = wndProc;
        wc.hInstance = inst;
        wc.lpszClassName = "BuildGL1DropInWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassA(&wc);

        DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        RECT rc{ 0, 0, width, height };
        AdjustWindowRect(&rc, style, FALSE);

        g.hwnd = CreateWindowA(wc.lpszClassName, title ? title : "Duke Nukem 3D",
            style, CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, inst, nullptr);
        if (!g.hwnd) return false;

        g.hdc = GetDC(g.hwnd);
        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cAlphaBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;
        int pf = ChoosePixelFormat(g.hdc, &pfd);
        if (pf <= 0 || !SetPixelFormat(g.hdc, pf, &pfd)) return false;

        g.hglrc = wglCreateContext(g.hdc);
        if (!g.hglrc || !wglMakeCurrent(g.hdc, g.hglrc)) return false;

        g.ready = true;
        RECT cr{};
        GetClientRect(g.hwnd, &cr);
        syncClientSize(cr.right - cr.left, cr.bottom - cr.top, true);

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.05f);
        glDisable(GL_CULL_FACE);
        glClearDepth(1.0);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        return true;
    }

    static void destroy()
    {
        if (g.hglrc)
        {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(g.hglrc);
            g.hglrc = nullptr;
        }
        if (g.hwnd && g.hdc)
        {
            ReleaseDC(g.hwnd, g.hdc);
            g.hdc = nullptr;
        }
        if (g.hwnd)
        {
            DestroyWindow(g.hwnd);
            g.hwnd = nullptr;
        }
        g.ready = false;
    }

    static void pump()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    static long slopeZ(short sectnum, long x, long y, bool ceiling)
    {
        if (sectnum < 0 || sectnum >= numsectors) return 0;
        sectortype& sec = sector[sectnum];
        long z = ceiling ? sec.ceilingz : sec.floorz;
        short stat = ceiling ? sec.ceilingstat : sec.floorstat;
        if ((stat & 2) == 0) return z;
        if (sec.wallptr < 0 || sec.wallptr >= numwalls) return z;
        walltype& wal = wall[sec.wallptr];
        if (wal.point2 < 0 || wal.point2 >= numwalls) return z;
        walltype& wal2 = wall[wal.point2];
        long dx = wal2.x - wal.x;
        long dy = wal2.y - wal.y;
        long len = ksqrt(dx * dx + dy * dy) << 5;
        if (len == 0) return z;
        long j = dmulscale3(dx, y - wal.y, -dy, x - wal.x);
        long heinum = ceiling ? sec.ceilingheinum : sec.floorheinum;
        return z + scale(heinum, j, len);
    }

    static std::vector<std::vector<int>> collectLoops(short sectnum)
    {
        std::vector<std::vector<int>> loops;
        if (sectnum < 0 || sectnum >= numsectors) return loops;
        int start = sector[sectnum].wallptr;
        int end = start + sector[sectnum].wallnum;
        if (start < 0 || end > numwalls) return loops;

        std::vector<char> seen(static_cast<size_t>(end - start), 0);
        for (int wi = start; wi < end; ++wi)
        {
            if (seen[static_cast<size_t>(wi - start)]) continue;
            std::vector<int> loop;
            int cur = wi;
            for (int guard = 0; guard < MAXWALLS; ++guard)
            {
                if (cur < start || cur >= end) break;
                if (seen[static_cast<size_t>(cur - start)] && cur != wi) break;
                seen[static_cast<size_t>(cur - start)] = 1;
                loop.push_back(cur);
                int next = wall[cur].point2;
                if (next == wi) break;
                cur = next;
            }
            if (loop.size() >= 3) loops.push_back(loop);
        }
        return loops;
    }

    static double area2(const std::vector<Vec2>& p)
    {
        double a = 0.0;
        for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++)
            a += (p[j].x * p[i].y - p[i].x * p[j].y);
        return a * 0.5;
    }
    static double cross(const Vec2& a, const Vec2& b, const Vec2& c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }
    static bool pointInTri(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c)
    {
        const double c1 = cross(a, b, p);
        const double c2 = cross(b, c, p);
        const double c3 = cross(c, a, p);
        return (c1 >= -1e-7 && c2 >= -1e-7 && c3 >= -1e-7);
    }
    static std::vector<int> triangulate(const std::vector<Vec2>& points)
    {
        std::vector<int> out;
        const int n = static_cast<int>(points.size());
        if (n < 3) return out;

        std::vector<int> idx(n);
        for (int i = 0; i < n; ++i) idx[i] = i;
        if (area2(points) < 0.0) std::reverse(idx.begin(), idx.end());

        int guard = 0;
        while (idx.size() > 3 && guard++ < n * n)
        {
            bool clipped = false;
            int m = static_cast<int>(idx.size());
            for (int i = 0; i < m; ++i)
            {
                int i0 = idx[(i + m - 1) % m], i1 = idx[i], i2 = idx[(i + 1) % m];
                const Vec2& a = points[i0]; const Vec2& b = points[i1]; const Vec2& c = points[i2];
                if (cross(a, b, c) <= 1e-7) continue;
                bool contains = false;
                for (int j = 0; j < m; ++j)
                {
                    int q = idx[j];
                    if (q == i0 || q == i1 || q == i2) continue;
                    if (pointInTri(points[q], a, b, c)) { contains = true; break; }
                }
                if (contains) continue;
                out.push_back(i0); out.push_back(i1); out.push_back(i2);
                idx.erase(idx.begin() + i);
                clipped = true;
                break;
            }
            if (!clipped) break;
        }
        if (idx.size() == 3) { out.push_back(idx[0]); out.push_back(idx[1]); out.push_back(idx[2]); }
        return out;
    }

    static std::string makeArtName(int fileNum)
    {
        std::string path = artfilename[0] ? artfilename : "tiles000.art";

        char digits[4];
        std::snprintf(digits, sizeof(digits), "%03d", fileNum % 1000);

        // Match the original BUILD convention while also supporting callers
        // that pass a directory-qualified ART path. The old code always edited
        // buf[5..7], which only works for exactly "tiles000.art" and makes
        // higher ART files fail to load when the name is "C:\...\tiles000.art".
        const size_t slash = path.find_last_of("/\\");
        const size_t nameStart = (slash == std::string::npos) ? 0 : slash + 1;
        size_t dot = path.find_last_of('.');
        if (dot == std::string::npos || dot < nameStart) dot = path.size();

        size_t digitRun = std::string::npos;
        if (dot >= nameStart + 3)
        {
            for (size_t i = dot; i-- > nameStart + 2; )
            {
                if (std::isdigit(static_cast<unsigned char>(path[i])) &&
                    std::isdigit(static_cast<unsigned char>(path[i - 1])) &&
                    std::isdigit(static_cast<unsigned char>(path[i - 2])))
                {
                    digitRun = i - 2;
                    break;
                }
            }
        }

        if (digitRun != std::string::npos)
        {
            path[digitRun + 0] = digits[0];
            path[digitRun + 1] = digits[1];
            path[digitRun + 2] = digits[2];
            return path;
        }

        const std::string dir = (nameStart == 0) ? std::string() : path.substr(0, nameStart);
        char leaf[32];
        std::snprintf(leaf, sizeof(leaf), "tiles%03d.art", fileNum % 1000);
        return dir + leaf;
    }

    static void markTileDirty(short tilenum)
    {
        if ((unsigned short)tilenum >= MAXTILES) return;
        for (int pal = 0; pal < MAXPALOOKUPS; ++pal)
            g.texDirty[tilenum][pal] = 1;
    }

    static void markPaletteDirty(long palnum)
    {
        if (palnum < 0 || palnum >= MAXPALOOKUPS)
        {
            for (short tile = 0; tile < MAXTILES; ++tile)
                markTileDirty(tile);
            return;
        }

        for (int tile = 0; tile < MAXTILES; ++tile)
            g.texDirty[tile][palnum] = 1;
    }

    static void markAllTexturesDirty()
    {
        for (short tile = 0; tile < MAXTILES; ++tile)
            markTileDirty(tile);
    }

    static void purgeTextureCache()
    {
        if (g.ready && g.hdc && g.hglrc)
            wglMakeCurrent(g.hdc, g.hglrc);

        for (int tile = 0; tile < MAXTILES; ++tile)
        {
            for (int pal = 0; pal < MAXPALOOKUPS; ++pal)
            {
                if (g.texReady[tile][pal])
                {
                    GLuint t = g.tex[tile][pal];
                    if (g.ready && t != 0) glDeleteTextures(1, &t);
                    g.tex[tile][pal] = 0;
                    g.texReady[tile][pal] = 0;
                }
                g.texDirty[tile][pal] = 1;
            }
        }

        if (g.fpOffTexReady)
        {
            GLuint t = g.fpOffTex;
            if (g.ready && t != 0) glDeleteTextures(1, &t);
            g.fpOffTex = 0;
            g.fpOffTexReady = false;
            g.fpOffTexW = 0;
            g.fpOffTexH = 0;
        }
    }

    // Duke3D palette swaps live in LOOKUP.DAT, not in PALETTE.DAT's shade
    // ramps. Keep the renderer's true-color texture cache keyed by these 256
    // entry remap tables so sprite/wall/sector pal values pick the intended
    // color variant instead of accidentally indexing a darkness table.
    static std::array<std::array<unsigned char, 256>, MAXPALOOKUPS> lookupPal{};
    static std::array<unsigned char, MAXPALOOKUPS> lookupPalReady{};

    static void resetLookupPalettes()
    {
        for (int pal = 0; pal < MAXPALOOKUPS; ++pal)
        {
            for (int i = 0; i < 256; ++i)
                lookupPal[pal][i] = static_cast<unsigned char>(i);
            lookupPalReady[pal] = (pal == 0) ? 1 : 0;
        }
    }

    static void installLookupPalette(unsigned char palnum, const unsigned char* table)
    {
        const int pal = static_cast<int>(palnum);
        if (pal < 0 || pal >= MAXPALOOKUPS || !table) return;
        for (int i = 0; i < 256; ++i)
            lookupPal[pal][i] = table[i];
        lookupPalReady[pal] = 1;
        markPaletteDirty(pal);
    }

    static void installIdentityLookupPalette(long palnum)
    {
        if (palnum < 0 || palnum >= MAXPALOOKUPS) return;
        for (int i = 0; i < 256; ++i)
            lookupPal[palnum][i] = static_cast<unsigned char>(i);
        lookupPalReady[palnum] = (palnum == 0) ? 1 : 0;
        markPaletteDirty(palnum);
    }

    static void loadLookupDatPalettes()
    {
        short fil = ::kopen4load((char*)"lookup.dat", 0);
        if (fil < 0) fil = ::kopen4load((char*)"LOOKUP.DAT", 0);
        if (fil < 0) return;

        unsigned char count = 0;
        if (::kread(fil, &count, 1) != 1)
        {
            ::kclose(fil);
            return;
        }

        for (int n = 0; n < static_cast<int>(count); ++n)
        {
            unsigned char palnum = 0;
            unsigned char table[256] = {};
            if (::kread(fil, &palnum, 1) != 1) break;
            if (::kread(fil, table, 256) != 256) break;
            installLookupPalette(palnum, table);
        }
        ::kclose(fil);
    }

    static unsigned char remapPaletteIndex(unsigned char idx, unsigned char palnum)
    {
        if (idx == 255) return idx;

        const int palSlot = static_cast<int>(palnum);
        if (palSlot > 0 && palSlot < MAXPALOOKUPS && lookupPalReady[palSlot])
            return lookupPal[palSlot][idx];

        // makepalookup() can create runtime remaps after LOOKUP.DAT has been
        // read. Use row 0 of those generated tables as a last-resort remap, but
        // do not fall back to palookup[0]: PALETTE.DAT palookup[0] is a shade
        // ramp, not a sprite palette swap.
        if (palSlot > 0 && palSlot < MAXPALOOKUPS && palookup[palSlot])
            return static_cast<unsigned char>(palookup[palSlot][idx]);

        return idx;
    }

    static bool tileHasArt(short tilenum)
    {
        return (unsigned short)tilenum < MAXTILES &&
            tilesizx[tilenum] > 0 && tilesizy[tilenum] > 0;
    }

    static void ensureFallbackTile(short tilenum)
    {
        if (tilenum < 0 || tilenum >= MAXTILES) return;
        if (tilesizx[tilenum] > 0 && tilesizy[tilenum] > 0 && waloff[tilenum]) return;
        tilesizx[tilenum] = 64; tilesizy[tilenum] = 64;
        g.tileData[tilenum].resize(64 * 64);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                g.tileData[tilenum][x * 64 + y] = static_cast<unsigned char>(((x >> 3) ^ (y >> 3)) & 1 ? 180 : 96);
        waloff[tilenum] = BUILDGL_PTR_TO_LONG(g.tileData[tilenum].data());
        g.fallbackTile[tilenum] = 1;
        picsiz[tilenum] = (6 | (6 << 4));
        markTileDirty(tilenum);
    }

    static void uploadTexture(short tilenum, unsigned char palnum)
    {
        if (tilenum < 0 || tilenum >= MAXTILES) tilenum = 0;
        const int palSlot = static_cast<int>(palnum) & (MAXPALOOKUPS - 1);

        if (!waloff[tilenum]) ensureFallbackTile(tilenum);
        if (g.texReady[tilenum][palSlot] && !g.texDirty[tilenum][palSlot]) return;

        const int w = max(1L, tilesizx[tilenum]);
        const int h = max(1L, tilesizy[tilenum]);
        const unsigned char* src = reinterpret_cast<const unsigned char*>(waloff[tilenum]);
        std::vector<unsigned char> rgba(static_cast<size_t>(w) * h * 4);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                unsigned char idx = src[x * h + y]; // ART is column-major.
                unsigned char* d = &rgba[(static_cast<size_t>(y) * w + x) * 4];
                if (idx == 255)
                {
                    d[0] = d[1] = d[2] = 0; d[3] = 0;
                }
                else
                {
                    idx = remapPaletteIndex(idx, static_cast<unsigned char>(palSlot));
                    d[0] = static_cast<unsigned char>(static_cast<unsigned char>(palette[idx * 3 + 0]) << 2);
