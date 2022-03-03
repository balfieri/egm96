EGM96 GPS Altitude Offset Calculator

This repo provies a single header file that can be used to convert
between GPS altitude (relative to the EGM96 ellipsoid) and MSL (mean 
sea level) given latitude and longitude. Specifically the
method getOffset( lat, lon ) calculates the height above
mean sea level of the EGM96 ellipsoid. Adding this offset
to GPS altitude yields MSL altitude (aka true altitude).

The file WW15MGH.DAC from this directory must also be present.

Note: I always compile with the -std=c++17 option. You may encounter
problems if you use a much early C++ standard.

Here's how to convert from GPS altitude to MSL:

 #include "EGM96.h"
 ...
 EGM96 gps96;   // may pass an alternate .DAC file path
 double latitude = <some latitude>;
 double longitude = <some longitude>;
 double gps_alt = <some GPS altitude>
 double msl_alt = gps_alt + gps96.getOffset( longitude, latitude );

latitude must be between -90.0 .. 90.0 degrees (inclusive).

longitude must be between -180.0 .. 180.0 degrees (inclusive). 

This C++ implementation was created by converting the Javascript
found at https://github.com/jleppert/egm96.

Refer to http://cddis.gsfc.nasa.gov/926/egm96/egm96.html for more
details on the egm96 dataset.

The LICENSE.md file reflects that author's license as well as mine,
which are both MIT.

Bob Alfieri
<br>
Chapel Hill, NC
