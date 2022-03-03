// Copyright (c) 2022-2023 Robert A. Alfieri
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// The MIT License (MIT)
// 
// Copyright (c) 2015 Johnathan
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//
// EGM96.h - GPS Altitude Offset Calculator
//
// Refer to README.md for usage.
//
#include <string>
#include <cmath>
#include <iostream>

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

class EGM96
{
public:
    EGM96( std::string path="WW15MGH.DAC" );
    ~EGM96();

    double getOffset( double latitude, double longitude );

private:
    const uint8_t * data;
    size_t          data_len; 

    static constexpr uint32_t NUM_ROWS = 721;
    static constexpr uint32_t NUM_COLS = 1440;
    double   INTERVAL;
    double   INTERVAL_DEGREE;

    static inline void edie( std::string msg )          { std::cout << "ERROR: " << msg << "\n"; exit( 1 ); }
    static inline void egmassert( bool expr, std::string msg ) { if ( !expr ) edie( msg ); }

    static inline double toDegree( double radians )     { return radians * (180.0/M_PI ); }
    static inline double fromDegree( double degrees )   { return degrees * (M_PI/180.0);  }

    template<typename T> 
    static inline T *    aligned_alloc( uint64_t cnt );
    static        bool   file_read( std::string file_path, char *& start, char *& end );

    inline int16_t readInt16BE( size_t index )          { return (data[index+0] << 8) | (data[index+1] << 0); }

    inline int16_t getPostOffset( uint32_t row, uint32_t col )
    {
        size_t k = row*NUM_COLS + col;
        egmassert( k < (data_len*2), "getPostOffset row,col outside range" );
        return readInt16BE( k*2 );
    }
};

EGM96::EGM96( std::string path )
{
    INTERVAL = fromDegree( 15/60 );
    INTERVAL_DEGREE = toDegree( INTERVAL );

    char * start;
    char * end;
    file_read( path, start, end );
    data = reinterpret_cast<const uint8_t *>( start );
    data_len = end-start;
}

EGM96::~EGM96()
{
    delete data;
    data = nullptr;
}

template<typename T>
inline T * EGM96::aligned_alloc( uint64_t cnt )
{
    void * mem = nullptr;
    posix_memalign( &mem, sysconf( _SC_PAGESIZE ), cnt*sizeof(T) );
    return reinterpret_cast<T *>( mem );
}

bool EGM96::file_read( std::string file_path, char *& start, char *& end )
{
    const char * fname = file_path.c_str();
    int fd = open( fname, O_RDONLY );
    if ( fd < 0 ) std::cout << "file_read() error reading " << file_path << ": " << strerror( errno ) << "\n";
    egmassert( fd >= 0, "could not open file " + file_path + " - open() error: " + strerror( errno ) );

    struct stat file_stat;
    int status = fstat( fd, &file_stat );
    if ( status < 0 ) {
        close( fd );
        egmassert( 0, "could not stat file " + std::string(fname) + " - stat() error: " + strerror( errno ) );
    }
    size_t size = file_stat.st_size;

    // this read should behave like an mmap() inside the o/s kernel and be as fast
    start = aligned_alloc<char>( size );
    if ( start == nullptr ) {
        close( fd );
        egmassert( 0, "could not read file " + std::string(fname) + " - malloc() error: " + strerror( errno ) );
    }
    end = start + size;

    if ( ::read( fd, start, size ) <= 0 ) {
        close( fd );
        egmassert( 0, "could not read() file " + std::string(fname) + " - read error: " + strerror( errno ) );
    }

    close( fd );
    return true;
}

double EGM96::getOffset( double latitude, double longitude ) 
{
    egmassert( latitude >= -90.0 && latitude <= 90.0, "latitude must be between -90.0 .. 90.0 degrees" );
    egmassert( longitude >= -180.0 && longitude <= 180.0, "longitude must be between -180.0 .. 180.0 degrees" );

    longitude = (longitude >= 0.0) ? longitude : (longitude + 360.0); // normalize to 0.0 .. 360.0

    uint32_t topRow;
    if ( latitude <= -90.0 ) {
        topRow = NUM_ROWS - 2;
    } else {
        topRow = (90.0 - latitude) / INTERVAL_DEGREE + 0.5;
    }
    uint32_t bottomRow = topRow + 1;

    uint32_t leftCol;
    uint32_t rightCol;
    if ( longitude >= (360.0 - INTERVAL_DEGREE) ) {
        leftCol = NUM_COLS - 1;
        rightCol = 0;
    } else {
        leftCol = longitude / INTERVAL_DEGREE + 0.5;
        rightCol = leftCol + 1;
    }

    uint32_t latTop = 90.0 - topRow * INTERVAL_DEGREE;
    uint32_t lonLeft = leftCol * INTERVAL_DEGREE;

    int16_t ul = getPostOffset( topRow, leftCol );
    int16_t ll = getPostOffset( bottomRow, leftCol );
    int16_t lr = getPostOffset( bottomRow, rightCol );
    int16_t ur = getPostOffset( topRow, rightCol );

    double u = (longitude - lonLeft) / INTERVAL_DEGREE;
    double v = (latTop - latitude) / INTERVAL_DEGREE;

    double pll = (1.0 - u) * (1.0 - v);
    double plr = (1.0 - u) * v;
    double pur = u * v;
    double pul = u * (1.0 - v);

    double offset = pll * ll + plr * lr + pur * ur + pul * ul;

    return offset / 100.0;
};
